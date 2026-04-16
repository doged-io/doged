// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumconfig.h>

#include <common/args.h>
#include <tinyformat.h>
#include <util/translation.h>

#include <cstdlib>

namespace stratum {

void RegisterStratumArgs(ArgsManager &args) {
    args.AddArg("-stratum",
                "Enable the built-in Stratum mining server (default: 0)",
                ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg("-stratumbind=<addr>",
                "Bind Stratum server to this address (default: 0.0.0.0)",
                ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg(
        "-stratumport=<port>",
        strprintf("Listen for Stratum connections on <port> (default: %u)",
                  DEFAULT_STRATUM_PORT),
        ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg(
        "-stratumdifficulty=<n>",
        strprintf("Initial share difficulty for Stratum workers (default: %g)",
                  DEFAULT_STRATUM_DIFFICULTY),
        ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg("-stratumvardiff",
                "Enable variable difficulty for Stratum workers (default: 1)",
                ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg(
        "-stratumvardifftarget=<n>",
        strprintf("Target seconds between shares for vardiff (default: %g)",
                  DEFAULT_VARDIFF_TARGET_TIME),
        ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg(
        "-stratumvardiffretarget=<n>",
        strprintf("Seconds between vardiff retarget checks (default: %g)",
                  DEFAULT_VARDIFF_RETARGET_TIME),
        ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg("-stratummaxworkers=<n>",
                strprintf("Maximum concurrent Stratum workers (default: %d)",
                          DEFAULT_MAX_WORKERS),
                ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg(
        "-stratumworkertimeout=<n>",
        strprintf("Worker idle timeout in seconds (default: %d)",
                  DEFAULT_WORKER_TIMEOUT_SEC),
        ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg("-stratumcoinbase=<address>",
                "Dogecoin address for Stratum coinbase payouts",
                ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg("-stratumproxy=<spec>",
                "Add an upstream pool for proxy/failover routing. Format: "
                "host:port:user[:pass[:priority]]. Can be specified multiple "
                "times. Lower priority = preferred. (default: none)",
                ArgsManager::ALLOW_ANY,
                OptionsCategory::BLOCK_CREATION);
    args.AddArg("-stratumpreferlocal",
                "Prefer local node mining when synced over proxy "
                "(default: 1). Set to 0 to always prefer upstream pools.",
                ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg("-stratumwarnsolo",
                "Log a warning when solo-mining locally with no upstream "
                "pool available (default: 1)",
                ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    args.AddArg("-mergemine=<spec>",
                "Add an external chain for multi-chain merged mining via "
                "RPC. Format: name:host:port:user:pass:chainid[:poll_ms]. "
                "Example: -mergemine=LTC:127.0.0.1:9332:user:pass:2:5000. "
                "Can be specified multiple times.",
                ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
}

util::Result<StratumConfig> ParseStratumConfig(const ArgsManager &args) {
    StratumConfig config;

    config.enabled = args.GetBoolArg("-stratum", false);
    if (!config.enabled) {
        return config;
    }

    config.bind = args.GetArg("-stratumbind", "0.0.0.0");

    int64_t port = args.GetIntArg("-stratumport", DEFAULT_STRATUM_PORT);
    if (port < 1 || port > 65535) {
        return {{_("Stratum port out of range (1-65535)")}};
    }
    config.port = static_cast<uint16_t>(port);

    config.defaultDifficulty = atof(
        args.GetArg("-stratumdifficulty",
                    strprintf("%g", DEFAULT_STRATUM_DIFFICULTY)).c_str());
    if (config.defaultDifficulty < MIN_DIFFICULTY) {
        return {{strprintf(
            Untranslated("Stratum difficulty too low (minimum %g)"),
            MIN_DIFFICULTY)}};
    }
    if (config.defaultDifficulty > MAX_DIFFICULTY) {
        return {{strprintf(
            Untranslated("Stratum difficulty too high (maximum %g)"),
            MAX_DIFFICULTY)}};
    }

    config.useVarDiff = args.GetBoolArg("-stratumvardiff", DEFAULT_STRATUM_VARDIFF);

    config.varDiffTargetTime = atof(
        args.GetArg("-stratumvardifftarget",
                    strprintf("%g", DEFAULT_VARDIFF_TARGET_TIME)).c_str());
    if (config.varDiffTargetTime <= 0) {
        return {{_("Stratum vardiff target time must be positive")}};
    }

    config.varDiffRetargetTime = atof(
        args.GetArg("-stratumvardiffretarget",
                    strprintf("%g", DEFAULT_VARDIFF_RETARGET_TIME)).c_str());
    if (config.varDiffRetargetTime <= 0) {
        return {{_("Stratum vardiff retarget time must be positive")}};
    }

    int64_t maxWorkers =
        args.GetIntArg("-stratummaxworkers", DEFAULT_MAX_WORKERS);
    if (maxWorkers < 1) {
        return {{_("Stratum max workers must be at least 1")}};
    }
    config.maxWorkers = static_cast<int>(maxWorkers);

    int64_t timeout =
        args.GetIntArg("-stratumworkertimeout", DEFAULT_WORKER_TIMEOUT_SEC);
    if (timeout < 1) {
        return {{_("Stratum worker timeout must be at least 1 second")}};
    }
    config.workerTimeoutSec = static_cast<int>(timeout);

    config.coinbaseAddress = args.GetArg("-stratumcoinbase", "");

    // Tiered routing options
    config.preferLocal =
        args.GetBoolArg("-stratumpreferlocal", true);
    config.warnSoloMining =
        args.GetBoolArg("-stratumwarnsolo", true);

    // Parse upstream pool specs: host:port:user[:pass[:priority]]
    for (const auto &spec : args.GetArgs("-stratumproxy")) {
        StratumPoolEntry entry;
        std::vector<std::string> parts;
        std::string token;
        for (char c : spec) {
            if (c == ':' && parts.size() < 4) {
                // Split on first 4 colons only (host:port:user:pass:priority)
                // but host could be IPv4, so port delimiter is after first part
                parts.push_back(token);
                token.clear();
            } else {
                token += c;
            }
        }
        parts.push_back(token);

        if (parts.size() < 3) {
            return {{strprintf(
                Untranslated("-stratumproxy format: host:port:user[:pass[:priority]], got '%s'"),
                spec)}};
        }

        entry.host = parts[0];
        entry.port = static_cast<uint16_t>(atoi(parts[1].c_str()));
        if (entry.port == 0) {
            return {{strprintf(
                Untranslated("Invalid port in -stratumproxy '%s'"), spec)}};
        }
        entry.username = parts[2];
        if (parts.size() > 3 && !parts[3].empty()) {
            entry.password = parts[3];
        }
        if (parts.size() > 4) {
            entry.priority = atoi(parts[4].c_str());
        } else {
            // Auto-assign priority by order of appearance
            entry.priority =
                static_cast<int>(config.upstreamPools.size());
        }

        config.upstreamPools.push_back(entry);
    }

    // Parse merge-mine chain specs: name:host:port:user:pass:chainid[:poll_ms]
    for (const auto &spec : args.GetArgs("-mergemine")) {
        StratumConfig::MergeMineEntry entry;
        std::vector<std::string> parts;
        std::string token;
        for (char c : spec) {
            if (c == ':' && parts.size() < 6) {
                parts.push_back(token);
                token.clear();
            } else {
                token += c;
            }
        }
        parts.push_back(token);

        if (parts.size() < 6) {
            return {{strprintf(
                Untranslated("-mergemine format: name:host:port:user:pass:chainid[:poll_ms], got '%s'"),
                spec)}};
        }

        entry.name = parts[0];
        entry.rpcHost = parts[1];
        entry.rpcPort = static_cast<uint16_t>(atoi(parts[2].c_str()));
        if (entry.rpcPort == 0) {
            return {{strprintf(
                Untranslated("Invalid port in -mergemine '%s'"), spec)}};
        }
        entry.rpcUser = parts[3];
        entry.rpcPassword = parts[4];
        entry.chainId = static_cast<uint32_t>(strtoul(parts[5].c_str(), nullptr, 0));
        if (parts.size() > 6) {
            entry.pollIntervalMs = atoi(parts[6].c_str());
            if (entry.pollIntervalMs < 500) {
                entry.pollIntervalMs = 500;
            }
        }

        config.mergeMineChains.push_back(entry);
    }

    return config;
}

} // namespace stratum
