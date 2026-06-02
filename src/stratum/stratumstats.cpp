// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumstats.h>

#include <util/time.h>

namespace stratum {

static StatsProvider g_statsProvider;

UniValue FormatStatsJson(const StratumServerStats &stats) {
    UniValue obj(UniValue::VOBJ);

    int64_t now = GetTime();
    obj.pushKV("uptime", now - stats.startTime);
    obj.pushKV("totalConnections", stats.totalConnections);
    obj.pushKV("activeWorkers", stats.activeWorkers);
    obj.pushKV("totalSharesAccepted", stats.totalSharesAccepted);
    obj.pushKV("totalSharesRejected", stats.totalSharesRejected);
    obj.pushKV("totalSharesStale", stats.totalSharesStale);
    obj.pushKV("blocksFound", stats.blocksFound);
    obj.pushKV("networkDifficulty", stats.networkDifficulty);
    obj.pushKV("chainHeight", stats.chainHeight);

    UniValue workersArr(UniValue::VARR);
    for (const auto &w : stats.workers) {
        UniValue wObj(UniValue::VOBJ);
        wObj.pushKV("name", w.workerName);
        wObj.pushKV("difficulty", w.currentDifficulty);
        wObj.pushKV("accepted", w.sharesAccepted);
        wObj.pushKV("rejected", w.sharesRejected);
        wObj.pushKV("stale", w.sharesStale);
        wObj.pushKV("hashrate", w.estimatedHashrate);
        wObj.pushKV("lastShareTime", w.lastShareTime);

        std::string stateStr;
        switch (w.state) {
            case StratumWorker::State::CONNECTED:
                stateStr = "connected";
                break;
            case StratumWorker::State::SUBSCRIBED:
                stateStr = "subscribed";
                break;
            case StratumWorker::State::AUTHORIZED:
                stateStr = "authorized";
                break;
            case StratumWorker::State::MINING:
                stateStr = "mining";
                break;
        }
        wObj.pushKV("state", stateStr);
        workersArr.push_back(wObj);
    }
    obj.pushKV("workers", workersArr);

    // Routing info
    obj.pushKV("activeTier", stats.activeTier);
    obj.pushKV("activeProxyIndex", stats.activeProxyIndex);

    UniValue poolsArr(UniValue::VARR);
    for (const auto &p : stats.upstreamPools) {
        UniValue pObj(UniValue::VOBJ);
        pObj.pushKV("label", p.label);
        pObj.pushKV("connected", p.connected);
        pObj.pushKV("healthy", p.healthy);
        pObj.pushKV("priority", p.priority);
        pObj.pushKV("consecutiveErrors", p.consecutiveErrors);
        pObj.pushKV("lastError", p.lastError);
        poolsArr.push_back(pObj);
    }
    obj.pushKV("upstreamPools", poolsArr);

    return obj;
}

static bool StratumStatusHandler(Config &config, HTTPRequest *req,
                                  const std::string &) {
    if (!g_statsProvider) {
        req->WriteReply(503, "Stratum server not available");
        return true;
    }

    StratumServerStats stats = g_statsProvider();
    UniValue json = FormatStatsJson(stats);

    req->WriteHeader("Content-Type", "application/json");
    req->WriteReply(200, json.write(2) + "\n");
    return true;
}

void RegisterStratumHTTPHandlers(StatsProvider provider) {
    g_statsProvider = std::move(provider);
    RegisterHTTPHandler("/stratum/status", true, StratumStatusHandler);
}

void UnregisterStratumHTTPHandlers() {
    UnregisterHTTPHandler("/stratum/status", true);
    g_statsProvider = nullptr;
}

} // namespace stratum
