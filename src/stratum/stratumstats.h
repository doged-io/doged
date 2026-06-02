// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMSTATS_H
#define BITCOIN_STRATUM_STRATUMSTATS_H

#include <httpserver.h>
#include <stratum/stratumworker.h>
#include <univalue.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class Config;

namespace stratum {

struct UpstreamPoolStats {
    std::string label;
    bool connected = false;
    bool healthy = false;
    int priority = 0;
    int consecutiveErrors = 0;
    std::string lastError;
};

struct StratumServerStats {
    int64_t startTime = 0;
    uint64_t totalConnections = 0;
    uint32_t activeWorkers = 0;
    uint64_t totalSharesAccepted = 0;
    uint64_t totalSharesRejected = 0;
    uint64_t totalSharesStale = 0;
    uint64_t blocksFound = 0;
    double networkDifficulty = 0;
    int chainHeight = 0;
    std::vector<StratumWorker::Stats> workers;

    // Routing info
    std::string activeTier;
    int activeProxyIndex = -1;
    std::vector<UpstreamPoolStats> upstreamPools;
};

/**
 * Callback type: the stratum server provides a function that returns current
 * stats when called.
 */
using StatsProvider = std::function<StratumServerStats()>;

/** Format stats as JSON for the HTTP endpoint. */
UniValue FormatStatsJson(const StratumServerStats &stats);

/** Register /stratum/status HTTP handler. */
void RegisterStratumHTTPHandlers(StatsProvider provider);

/** Unregister /stratum/status HTTP handler. */
void UnregisterStratumHTTPHandlers();

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMSTATS_H
