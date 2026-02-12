// Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DOGED_TOOLS_MINER_MINING_ENGINE_H
#define DOGED_TOOLS_MINER_MINING_ENGINE_H

#include "stratum_client.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#ifdef HAVE_OPENCL
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#endif

struct MiningStats {
    std::atomic<uint64_t> totalHashes{0};
    std::atomic<uint64_t> acceptedShares{0};
    std::atomic<uint64_t> rejectedShares{0};
    std::atomic<uint64_t> foundBlocks{0};
};

struct MiningWork {
    StratumJob job;
    std::string extranonce1;
    int extranonce2Size = 4;
    double difficulty = 1.0;
    uint32_t extranonce2Counter = 0;

    // Parsed header fields for the miner
    uint32_t header[20]; // 80-byte header as uint32
    uint32_t target[8];  // 32-byte target
};

// Utility: build the 80-byte header + target from a stratum job
bool BuildMiningWork(MiningWork &work);

// Convert difficulty to a 256-bit target
void DifficultyToTarget(double difficulty, uint32_t target[8]);

// Hex conversion helpers
std::vector<uint8_t> HexToBytes(const std::string &hex);
std::string BytesToHex(const uint8_t *data, size_t len);
uint32_t SwapEndian32(uint32_t v);

// --- CPU Mining ---

struct CpuMinerConfig {
    int threads = 1;
};

class CpuMiner {
public:
    CpuMiner(const CpuMinerConfig &config);

    // Mine a range of nonces. Calls foundCb(nonce) for each hit.
    // Returns total hashes computed.
    uint64_t ScanNonces(const MiningWork &work, uint32_t nonceStart,
                        uint32_t nonceEnd,
                        std::function<void(uint32_t)> foundCb,
                        std::atomic<bool> &interrupt);

private:
    CpuMinerConfig m_config;
};

// --- GPU Mining (OpenCL) ---

#ifdef HAVE_OPENCL

struct GpuDevice {
    cl_device_id id;
    std::string name;
    uint64_t globalMem;
    uint32_t computeUnits;
};

std::vector<GpuDevice> ListGpuDevices();

struct GpuMinerConfig {
    int deviceIndex = 0;
    int workSize = 65536;   // global work size (nonces per batch)
    int localSize = 256;    // local work group size
    std::string kernelPath; // path to scrypt.cl
};

class GpuMiner {
public:
    GpuMiner(const GpuMinerConfig &config);
    ~GpuMiner();

    bool Init();
    void Release();

    uint64_t ScanNonces(const MiningWork &work, uint32_t nonceStart,
                        std::function<void(uint32_t)> foundCb,
                        std::atomic<bool> &interrupt);

    const GpuDevice &GetDevice() const { return m_device; }

private:
    GpuMinerConfig m_config;
    GpuDevice m_device;

    cl_context m_context = nullptr;
    cl_command_queue m_queue = nullptr;
    cl_program m_program = nullptr;
    cl_kernel m_kernel = nullptr;

    cl_mem m_headerBuf = nullptr;
    cl_mem m_targetBuf = nullptr;
    cl_mem m_vBuf = nullptr;
    cl_mem m_resultsBuf = nullptr;

    bool m_initialized = false;
};

#endif // HAVE_OPENCL

#endif
