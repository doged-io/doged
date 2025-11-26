// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMPROXY_H
#define BITCOIN_STRATUM_STRATUMPROXY_H

#include <stratum/stratumprotocol.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace stratum {

struct UpstreamPool {
    std::string host;
    uint16_t port = 3333;
    std::string username;
    std::string password = "x";
    int priority = 0; // lower = higher priority
};

/**
 * Health status of an upstream pool connection.
 */
struct ProxyHealth {
    bool connected = false;
    bool authorized = false;
    int64_t lastNotifyTime = 0;
    int64_t lastErrorTime = 0;
    int consecutiveErrors = 0;
    int64_t connectTime = 0;
    std::string lastError;
};

/**
 * Callbacks the proxy fires when upstream sends us data.
 * The router registers these to receive upstream events transparently.
 */
struct ProxyCallbacks {
    // Upstream sent mining.notify — new work available
    std::function<void(const std::string &rawNotifyLine)> onNotify;
    // Upstream sent mining.set_difficulty
    std::function<void(double difficulty)> onSetDifficulty;
    // Upstream responded to a mining.submit (id → result/error)
    std::function<void(int64_t minerId, bool accepted,
                       const std::string &error)>
        onSubmitResult;
    // Connection state changed
    std::function<void(bool connected, const std::string &reason)>
        onStateChange;
};

/**
 * A single upstream pool connection.
 *
 * Manages a TCP socket to a remote Stratum pool. Subscribes and authorizes
 * on connect. Relays mining.notify and mining.set_difficulty downstream
 * via callbacks. Forwards mining.submit upstream.
 *
 * Almost transparent: the downstream miner doesn't know it's talking
 * through a proxy.
 */
class StratumProxyConn {
public:
    StratumProxyConn(const UpstreamPool &pool, const ProxyCallbacks &callbacks);
    ~StratumProxyConn();

    bool Connect();
    void Disconnect();
    bool IsConnected() const { return m_connected.load(); }
    bool IsHealthy() const;

    /**
     * Forward a miner's mining.submit to the upstream pool.
     * @param minerId  The downstream miner's request ID (for result routing)
     * @param params   The raw submit params from the miner
     */
    void ForwardSubmit(int64_t minerId, const UniValue &params);

    const UpstreamPool &GetPool() const { return m_pool; }
    ProxyHealth GetHealth() const;
    std::string GetLabel() const;

    /** The extranonce1 assigned by the upstream pool (after subscribe). */
    std::string GetUpstreamExtranonce1() const;
    int GetUpstreamExtranonce2Size() const;

private:
    UpstreamPool m_pool;
    ProxyCallbacks m_callbacks;

    int m_sockfd = -1;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_authorized{false};
    std::atomic<bool> m_interrupt{false};
    std::thread m_readThread;

    mutable std::mutex m_writeMutex;
    mutable std::mutex m_healthMutex;

    std::string m_upstreamExtranonce1;
    int m_upstreamExtranonce2Size = 4;

    ProxyHealth m_health;
    StratumLineBuffer m_lineBuffer;
    int64_t m_nextReqId = 100;

    // Map our request IDs to downstream miner IDs for submit routing
    std::mutex m_pendingMutex;
    std::map<int64_t, int64_t> m_pendingSubmits; // upstream_id → miner_id

    void ReadLoop();
    void HandleLine(const std::string &line);
    void HandleResponse(const StratumResponse &resp);
    void HandleNotification(const StratumNotification &notif);
    bool SendRaw(const std::string &data);
    bool DoSubscribe();
    bool DoAuthorize();
    void RecordError(const std::string &err);

    static constexpr int CONNECT_TIMEOUT_SEC = 10;
    static constexpr int HEALTHY_NOTIFY_STALE_SEC = 300;
    static constexpr int MAX_CONSECUTIVE_ERRORS = 5;
    static constexpr int RECONNECT_BACKOFF_SEC = 5;
};

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMPROXY_H
