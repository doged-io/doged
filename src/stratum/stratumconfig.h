// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMCONFIG_H
#define BITCOIN_STRATUM_STRATUMCONFIG_H

#include <cstdint>
#include <script/standard.h>
#include <string>
#include <util/result.h>
#include <vector>

class ArgsManager;

namespace stratum {

static constexpr uint16_t DEFAULT_STRATUM_PORT = 3333;
static constexpr double DEFAULT_STRATUM_DIFFICULTY = 0.5;
static constexpr bool DEFAULT_STRATUM_VARDIFF = true;
static constexpr double DEFAULT_VARDIFF_TARGET_TIME = 15.0;
static constexpr double DEFAULT_VARDIFF_RETARGET_TIME = 90.0;
static constexpr double DEFAULT_VARDIFF_VARIANCE = 0.25;
static constexpr int DEFAULT_MAX_WORKERS = 1024;
static constexpr int DEFAULT_WORKER_TIMEOUT_SEC = 300;
static constexpr size_t DEFAULT_JOB_CACHE_SIZE = 8;
static constexpr double MIN_DIFFICULTY = 0.0000001;
static constexpr double MAX_DIFFICULTY = 1e12;

/**
 * A remote upstream pool endpoint for proxy/failover routing.
 * Parsed from -stratumproxy=host:port:user:pass:priority
 */
struct StratumPoolEntry {
    std::string host;
    uint16_t port = 3333;
    std::string username;
    std::string password = "x";
    int priority = 0;
};

struct StratumConfig {
    bool enabled = false;
    std::string bind = "0.0.0.0";
    uint16_t port = DEFAULT_STRATUM_PORT;
    double defaultDifficulty = DEFAULT_STRATUM_DIFFICULTY;
    bool useVarDiff = DEFAULT_STRATUM_VARDIFF;
    double varDiffTargetTime = DEFAULT_VARDIFF_TARGET_TIME;
    double varDiffRetargetTime = DEFAULT_VARDIFF_RETARGET_TIME;
    double varDiffVariance = DEFAULT_VARDIFF_VARIANCE;
    int maxWorkers = DEFAULT_MAX_WORKERS;
    int workerTimeoutSec = DEFAULT_WORKER_TIMEOUT_SEC;
    size_t jobCacheSize = DEFAULT_JOB_CACHE_SIZE;
    std::string coinbaseAddress;

    // Tiered routing
    bool preferLocal = true;     // Use local node when synced (tier 1)
    bool warnSoloMining = true;  // Warn when falling through to local-only
    std::vector<StratumPoolEntry> upstreamPools; // Proxy targets (tier 2+)
};

void RegisterStratumArgs(ArgsManager &args);
util::Result<StratumConfig> ParseStratumConfig(const ArgsManager &args);

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMCONFIG_H
