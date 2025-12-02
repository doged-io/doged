// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMROUTER_H
#define BITCOIN_STRATUM_STRATUMROUTER_H

#include <script/script.h>
#include <stratum/stratumconfig.h>
#include <stratum/stratumjob.h>
#include <stratum/stratumproxy.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class CChainParams;
class Chainstate;
class ChainstateManager;
class CTxMemPool;

namespace stratum {

/**
 * Which backend is currently providing work to miners.
 */
enum class RoutingTier {
    NONE,  // No backend available — warning state
    LOCAL, // BlockAssembler on local node
    PROXY, // Relaying from an upstream pool
};

std::string TierName(RoutingTier tier);

/**
 * Callbacks fired by the router to the StratumServer so it can push
 * data to connected miners. The server registers these at startup.
 */
struct RouterDownstreamCallbacks {
    // A new job is ready: broadcast mining.notify to all authorized miners
    std::function<void(const std::string &rawNotifyLine)> broadcastNotify;
    // Upstream changed difficulty (proxy mode): broadcast mining.set_difficulty
    std::function<void(double difficulty)> broadcastDifficulty;
    // An upstream submit completed (proxy mode): route result to specific miner
    std::function<void(int64_t minerId, bool accepted,
                       const std::string &error)>
        submitResult;
    // Tier changed — for logging/stats
    std::function<void(RoutingTier newTier, const std::string &detail)>
        tierChanged;
};

/**
 * Tiered work router.
 *
 * Priority order:
 *   1. LOCAL — if the node is synced, use BlockAssembler directly
 *   2. PROXY — relay from the first healthy upstream pool
 *   3. Next PROXY — failover to the next healthy pool
 *   4. NONE — warn; no work source available
 *
 * Almost transparent: miners always get standard mining.notify / submit
 * regardless of which tier is active. On tier switch, a clean_jobs=true
 * notification is sent.
 */
class StratumRouter {
public:
    StratumRouter(const StratumConfig &config,
                  const std::vector<UpstreamPool> &pools,
                  Chainstate &chainstate, const CTxMemPool *mempool,
                  const CChainParams &chainParams,
                  ChainstateManager &chainman,
                  const CScript &coinbaseScript);
    ~StratumRouter();

    void SetCallbacks(RouterDownstreamCallbacks callbacks);

    bool Start();
    void Stop();

    /** Current active tier. */
    RoutingTier GetActiveTier() const { return m_activeTier.load(); }

    /** True if the local node is synced enough to produce blocks. */
    bool IsLocalAvailable() const;

    /** Called by StratumServer when the chain tip changes. */
    void OnNewTip(int height);

    /**
     * Submit a share. Routes to local validation or upstream proxy
     * depending on the active tier.
     *
     * @return true if the submit was routed (not necessarily accepted).
     *         false if no backend is available.
     */
    bool RouteSubmit(int64_t minerId, uint64_t jobId,
                     const std::string &extranonce1,
                     const UniValue &submitParams,
                     const StratumJob **outJob);

    /** Get the local job manager (for local-tier share validation). */
    StratumJobManager *GetJobManager() { return m_jobMgr.get(); }

    /** Get active proxy index (or -1 if not in proxy mode). */
    int GetActiveProxyIndex() const;

    /** Number of configured upstream pools. */
    size_t GetProxyCount() const { return m_proxies.size(); }

    /** Health info for a specific proxy. */
    ProxyHealth GetProxyHealth(size_t index) const;

private:
    StratumConfig m_config;
    std::vector<UpstreamPool> m_pools;

    Chainstate &m_chainstate;
    const CChainParams &m_chainParams;
    ChainstateManager &m_chainman;

    std::unique_ptr<StratumJobManager> m_jobMgr;
    std::vector<std::unique_ptr<StratumProxyConn>> m_proxies;

    RouterDownstreamCallbacks m_downstream;

    std::atomic<RoutingTier> m_activeTier{RoutingTier::NONE};
    std::atomic<int> m_activeProxyIdx{-1};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_warnedSoloMining{false};

    std::thread m_healthThread;
    mutable std::mutex m_tierMutex;

    void HealthCheckLoop();
    void EvaluateAndSwitch();
    void SwitchToLocal();
    void SwitchToProxy(int index);
    void SwitchToNone();
    void ConnectProxy(int index);
    void DisconnectProxy(int index);

    ProxyCallbacks MakeProxyCallbacks(int index);

    void LocalCreateAndBroadcast(bool cleanJobs);

    static constexpr int HEALTH_CHECK_INTERVAL_SEC = 5;
    static constexpr int LOCAL_SYNC_THRESHOLD = 10;
};

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMROUTER_H
