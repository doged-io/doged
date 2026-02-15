// Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// doged-miner: basic Scrypt GPU/CPU miner with Stratum support.
// Usage:
//   doged-miner -o stratum+tcp://host:port -u worker -p x [--gpu 0] [--cpu 2]

#include "mining_engine.h"
#include "stratum_client.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <thread>

static std::atomic<bool> g_shutdown{false};

static void sighandler(int) {
    g_shutdown.store(true);
}

struct Options {
    std::string host = "127.0.0.1";
    uint16_t port = 3333;
    std::string user = "worker";
    std::string pass = "x";
    int gpuDevice = -1;   // -1 = no GPU
    int cpuThreads = 1;
    std::string kernelPath;
    int gpuWorkSize = 32768;
    bool listDevices = false;
};

static void usage() {
    std::cout <<
        "doged-miner — Scrypt(1024,1,1) Stratum miner\n"
        "\n"
        "Usage:\n"
        "  doged-miner -o stratum+tcp://host:port -u user -p pass [options]\n"
        "\n"
        "Options:\n"
        "  -o <url>         Stratum server URL (stratum+tcp://host:port)\n"
        "  -u <user>        Worker username\n"
        "  -p <pass>        Worker password (default: x)\n"
        "  --gpu <n>        Use GPU device N (default: CPU only)\n"
        "  --cpu <n>        Number of CPU threads (default: 1, 0 to disable)\n"
        "  --kernel <path>  Path to scrypt.cl kernel file\n"
        "  --worksize <n>   GPU global work size (default: 32768)\n"
        "  --list-devices   List available GPU devices and exit\n"
        "  -h, --help       Show this help\n"
        "\n"
        "Examples:\n"
        "  doged-miner -o stratum+tcp://127.0.0.1:3333 -u myworker -p x --cpu 4\n"
        "  doged-miner -o stratum+tcp://pool.example.com:3333 -u w -p x --gpu 0\n"
        "\n";
}

static Options parseArgs(int argc, char *argv[]) {
    Options opts;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if ((arg == "-o" || arg == "--url") && i + 1 < argc) {
            std::string url = argv[++i];
            // Parse stratum+tcp://host:port
            size_t pos = url.find("://");
            if (pos != std::string::npos) url = url.substr(pos + 3);
            pos = url.find(':');
            if (pos != std::string::npos) {
                opts.host = url.substr(0, pos);
                opts.port = (uint16_t)atoi(url.substr(pos + 1).c_str());
            } else {
                opts.host = url;
            }
        } else if ((arg == "-u" || arg == "--user") && i + 1 < argc) {
            opts.user = argv[++i];
        } else if ((arg == "-p" || arg == "--pass") && i + 1 < argc) {
            opts.pass = argv[++i];
        } else if (arg == "--gpu" && i + 1 < argc) {
            opts.gpuDevice = atoi(argv[++i]);
        } else if (arg == "--cpu" && i + 1 < argc) {
            opts.cpuThreads = atoi(argv[++i]);
        } else if (arg == "--kernel" && i + 1 < argc) {
            opts.kernelPath = argv[++i];
        } else if (arg == "--worksize" && i + 1 < argc) {
            opts.gpuWorkSize = atoi(argv[++i]);
        } else if (arg == "--list-devices") {
            opts.listDevices = true;
        } else if (arg == "-h" || arg == "--help") {
            usage();
            exit(0);
        }
    }

    return opts;
}

int main(int argc, char *argv[]) {
    Options opts = parseArgs(argc, argv);

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

#ifdef HAVE_OPENCL
    if (opts.listDevices) {
        auto devices = ListGpuDevices();
        if (devices.empty()) {
            std::cout << "No OpenCL GPU devices found.\n";
        } else {
            for (size_t i = 0; i < devices.size(); i++) {
                std::cout << "  GPU " << i << ": " << devices[i].name
                          << " (" << devices[i].computeUnits << " CU, "
                          << (devices[i].globalMem / (1024 * 1024)) << " MB)\n";
            }
        }
        return 0;
    }
#else
    if (opts.listDevices) {
        std::cout << "Built without OpenCL support.\n";
        return 0;
    }
#endif

    if (opts.host.empty()) {
        usage();
        return 1;
    }

    // --- State ---
    std::mutex jobMutex;
    StratumJob currentJob;
    bool hasJob = false;
    double currentDifficulty = 1.0;
    MiningStats stats;

    auto startTime = std::chrono::steady_clock::now();

    // --- Stratum callbacks ---
    StratumCallbacks cbs;

    cbs.onJob = [&](const StratumJob &job) {
        std::lock_guard<std::mutex> lock(jobMutex);
        currentJob = job;
        hasJob = true;
        std::cout << "\r[stratum] New job " << job.jobId
                  << (job.cleanJobs ? " (clean)" : "") << "        \n"
                  << std::flush;
    };

    cbs.onDifficulty = [&](double diff) {
        currentDifficulty = diff;
        std::cout << "\r[stratum] Difficulty: " << diff << "        \n"
                  << std::flush;
    };

    cbs.onSubmitResult = [&](int64_t id, bool accepted,
                             const std::string &error) {
        if (accepted) {
            stats.acceptedShares++;
            std::cout << "\r[stratum] Share ACCEPTED ("
                      << stats.acceptedShares.load() << " total)        \n"
                      << std::flush;
        } else {
            stats.rejectedShares++;
            std::cout << "\r[stratum] Share REJECTED: " << error
                      << "        \n" << std::flush;
        }
    };

    cbs.onStateChange = [&](bool connected, const std::string &reason) {
        std::cout << "\r[stratum] " << (connected ? "Connected" : "Disconnected")
                  << ": " << reason << "        \n" << std::flush;
    };

    // --- Connect ---
    setbuf(stdout, nullptr);
    printf("doged-miner v0.1 — Scrypt(1024,1,1)\n");
    printf("Connecting to %s:%d as %s...\n",
           opts.host.c_str(), opts.port, opts.user.c_str());

    StratumClient client(opts.host, opts.port, opts.user, opts.pass, cbs);
    if (!client.Connect()) {
        std::cerr << "Failed to connect to stratum server.\n";
        return 1;
    }

    std::cout << "Extranonce1: " << client.GetExtranonce1()
              << ", en2 size: " << client.GetExtranonce2Size() << "\n";

    // --- Init mining engine ---
#ifdef HAVE_OPENCL
    std::unique_ptr<GpuMiner> gpuMiner;
    if (opts.gpuDevice >= 0) {
        GpuMinerConfig gcfg;
        gcfg.deviceIndex = opts.gpuDevice;
        gcfg.workSize = opts.gpuWorkSize;
        gcfg.kernelPath = opts.kernelPath;
        if (gcfg.kernelPath.empty()) {
            // Try to find kernel next to executable
            gcfg.kernelPath = "scrypt.cl";
        }

        gpuMiner = std::make_unique<GpuMiner>(gcfg);
        if (!gpuMiner->Init()) {
            std::cerr << "GPU init failed, falling back to CPU\n";
            gpuMiner.reset();
        }
    }
#endif

    CpuMinerConfig ccfg;
    ccfg.threads = opts.cpuThreads;
    CpuMiner cpuMiner(ccfg);

    // --- Mining loop ---
    std::cout << "Mining";
#ifdef HAVE_OPENCL
    if (gpuMiner) std::cout << " [GPU: " << gpuMiner->GetDevice().name << "]";
#endif
    if (opts.cpuThreads > 0) std::cout << " [CPU: " << opts.cpuThreads << " threads]";
    std::cout << "\n\n";

    uint32_t en2Counter = 0;

    while (!g_shutdown.load() && client.IsConnected()) {
        // Wait for a job
        if (!hasJob) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        MiningWork work;
        {
            std::lock_guard<std::mutex> lock(jobMutex);
            work.job = currentJob;
        }
        work.extranonce1 = client.GetExtranonce1();
        work.extranonce2Size = client.GetExtranonce2Size();
        work.difficulty = currentDifficulty;
        work.extranonce2Counter = en2Counter++;

        if (!BuildMiningWork(work)) {
            std::cerr << "Failed to build mining work\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        auto foundCb = [&](uint32_t nonce) {
            // Format nonce as LE hex
            uint8_t nb[4] = {(uint8_t)(nonce & 0xff),
                             (uint8_t)((nonce >> 8) & 0xff),
                             (uint8_t)((nonce >> 16) & 0xff),
                             (uint8_t)((nonce >> 24) & 0xff)};
            std::string nonceHex = BytesToHex(nb, 4);

            char en2[16];
            snprintf(en2, sizeof(en2), "%0*x",
                     work.extranonce2Size * 2, work.extranonce2Counter);

            std::cout << "\r*** SHARE FOUND! nonce=" << nonceHex
                      << "        \n" << std::flush;

            client.SubmitShare(work.job.jobId, std::string(en2),
                               work.job.ntime, nonceHex);
        };

        uint32_t nonceStart = 0;

#ifdef HAVE_OPENCL
        if (gpuMiner) {
            uint64_t h = gpuMiner->ScanNonces(work, nonceStart, foundCb,
                                               g_shutdown);
            stats.totalHashes += h;
        } else
#endif
        {
            // CPU mining: scan a batch of nonces
            uint32_t batchSize = 1024 * opts.cpuThreads;
            uint64_t h = cpuMiner.ScanNonces(work, nonceStart,
                                              nonceStart + batchSize, foundCb,
                                              g_shutdown);
            stats.totalHashes += h;
        }

        // Print hashrate
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        double secs =
            std::chrono::duration<double>(elapsed).count();
        if (secs > 0) {
            double hr = stats.totalHashes.load() / secs;
            const char *unit = "H/s";
            if (hr >= 1e6) { hr /= 1e6; unit = "MH/s"; }
            else if (hr >= 1e3) { hr /= 1e3; unit = "kH/s"; }
            printf("\r[mining] %.2f %s | accepted: %lu | rejected: %lu   ",
                   hr, unit, stats.acceptedShares.load(),
                   stats.rejectedShares.load());
            fflush(stdout);
        }
    }

    std::cout << "\nShutting down...\n";
    client.Disconnect();

    double secs = std::chrono::duration<double>(
                      std::chrono::steady_clock::now() - startTime)
                      .count();
    std::cout << "Total hashes: " << stats.totalHashes.load()
              << " in " << (int)secs << "s\n"
              << "Accepted: " << stats.acceptedShares.load()
              << " | Rejected: " << stats.rejectedShares.load()
              << " | Blocks: " << stats.foundBlocks.load() << "\n";

    return 0;
}
