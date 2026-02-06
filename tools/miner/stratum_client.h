// Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DOGED_TOOLS_MINER_STRATUM_CLIENT_H
#define DOGED_TOOLS_MINER_STRATUM_CLIENT_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct StratumJob {
    std::string jobId;
    std::string prevHash;
    std::string coinbase1;
    std::string coinbase2;
    std::vector<std::string> merkleBranches;
    std::string version;
    std::string nbits;
    std::string ntime;
    bool cleanJobs = false;
};

struct StratumCallbacks {
    std::function<void(const StratumJob &)> onJob;
    std::function<void(double)> onDifficulty;
    std::function<void(int64_t, bool, const std::string &)> onSubmitResult;
    std::function<void(bool, const std::string &)> onStateChange;
};

class StratumClient {
public:
    StratumClient(const std::string &host, uint16_t port,
                  const std::string &user, const std::string &pass,
                  StratumCallbacks cbs);
    ~StratumClient();

    bool Connect();
    void Disconnect();
    bool IsConnected() const { return m_connected.load(); }

    void SubmitShare(const std::string &jobId, const std::string &extranonce2,
                     const std::string &ntime, const std::string &nonce);

    const std::string &GetExtranonce1() const { return m_extranonce1; }
    int GetExtranonce2Size() const { return m_extranonce2Size; }

private:
    std::string m_host;
    uint16_t m_port;
    std::string m_user;
    std::string m_pass;
    StratumCallbacks m_cbs;

    int m_sockfd = -1;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_interrupt{false};
    std::thread m_readThread;

    std::string m_extranonce1;
    int m_extranonce2Size = 4;

    std::mutex m_writeMutex;
    int64_t m_nextId = 10;

    void ReadLoop();
    bool SendJson(const std::string &json);
    bool DoSubscribe();
    bool DoAuthorize();
    std::string RecvLine(int timeoutSec = 10);
};

#endif
