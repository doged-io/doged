// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/mergemine.h>

#include <algorithm>
#include <logging.h>
#include <shutdown.h>
#include <util/strencodings.h>
#include <util/time.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/http.h>

#include <sstream>

namespace stratum {

static std::unique_ptr<MergeMineManager> g_mergeMineManager;

void InitMergeMineManager() {
    g_mergeMineManager = std::make_unique<MergeMineManager>();
}

MergeMineManager *GetMergeMineManager() {
    return g_mergeMineManager.get();
}

void StopMergeMineManager() {
    if (g_mergeMineManager) {
        g_mergeMineManager->Stop();
        g_mergeMineManager.reset();
    }
}

// --- ExternalChainClient ---

ExternalChainClient::ExternalChainClient(const ExternalChainConfig &cfg)
    : m_cfg(cfg) {}

ExternalChainClient::~ExternalChainClient() = default;

UniValue ExternalChainClient::CallRPC(const std::string &method,
                                      const UniValue &params) {
    UniValue request(UniValue::VOBJ);
    request.pushKV("jsonrpc", "1.0");
    request.pushKV("id", "doged-mergemine");
    request.pushKV("method", method);
    request.pushKV("params", params);

    std::string body = request.write() + "\n";

    // Build HTTP request using libevent
    struct event_base *base = event_base_new();
    if (!base) {
        throw std::runtime_error("Failed to create event base for RPC");
    }

    struct evhttp_connection *conn = evhttp_connection_base_new(
        base, nullptr, m_cfg.rpcHost.c_str(), m_cfg.rpcPort);
    if (!conn) {
        event_base_free(base);
        throw std::runtime_error("Failed to create HTTP connection");
    }

    evhttp_connection_set_timeout(conn, 30);

    struct evhttp_request *req = nullptr;
    struct RpcResponse {
        std::string body;
        int httpCode = 0;
    } rpcResp;

    auto callback = [](struct evhttp_request *req, void *ctx) {
        auto *resp = static_cast<RpcResponse *>(ctx);
        if (!req) {
            return;
        }
        resp->httpCode = evhttp_request_get_response_code(req);
        if (resp->httpCode == 0) {
            return;
        }
        struct evbuffer *buf = evhttp_request_get_input_buffer(req);
        if (!buf) {
            return;
        }
        size_t len = evbuffer_get_length(buf);
        if (len > 0) {
            resp->body.resize(len);
            evbuffer_remove(buf, resp->body.data(), len);
        }
    };

    req = evhttp_request_new(callback, &rpcResp);
    if (!req) {
        evhttp_connection_free(conn);
        event_base_free(base);
        throw std::runtime_error("Failed to create HTTP request");
    }

    struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", "application/json");
    evhttp_add_header(headers, "Connection", "close");

    // Basic auth
    if (!m_cfg.rpcUser.empty()) {
        std::string auth = m_cfg.rpcUser + ":" + m_cfg.rpcPassword;
        std::string encoded = EncodeBase64(auth);
        evhttp_add_header(headers, "Authorization",
                          ("Basic " + encoded).c_str());
    }

    struct evbuffer *output = evhttp_request_get_output_buffer(req);
    evbuffer_add(output, body.data(), body.size());

    int rv = evhttp_make_request(conn, req, EVHTTP_REQ_POST, "/");
    if (rv != 0) {
        evhttp_connection_free(conn);
        event_base_free(base);
        throw std::runtime_error("Failed to make HTTP request");
    }

    event_base_dispatch(base);
    evhttp_connection_free(conn);
    event_base_free(base);

    if (rpcResp.httpCode == 403) {
        throw std::runtime_error(m_cfg.name +
                                 " RPC returned HTTP 403"
                                 " (check rpcallowip on remote node)");
    }

    if (rpcResp.body.empty()) {
        if (rpcResp.httpCode != 0 && rpcResp.httpCode != 200) {
            throw std::runtime_error(
                m_cfg.name + " RPC returned HTTP " +
                std::to_string(rpcResp.httpCode) + " (empty body)");
        }
        throw std::runtime_error("Empty response from " + m_cfg.name +
                                 " RPC");
    }

    UniValue parsed;
    if (!parsed.read(rpcResp.body)) {
        throw std::runtime_error("Failed to parse JSON response from " +
                                 m_cfg.name);
    }

    if (!parsed.isObject()) {
        throw std::runtime_error("Non-object JSON response from " +
                                 m_cfg.name);
    }

    const UniValue &error = parsed["error"];
    if (!error.isNull()) {
        std::string errMsg = error.isObject()
                                 ? error["message"].get_str()
                                 : error.getValStr();
        throw std::runtime_error(m_cfg.name + " RPC error: " + errMsg);
    }

    return parsed["result"];
}

bool ExternalChainClient::RefreshWork(const std::string &address) {
    try {
        UniValue result;
        bool usedBlockTemplate = false;

        // 1. Try createauxblock (Dogecoin-style)
        try {
            UniValue params(UniValue::VARR);
            params.push_back(address);
            result = CallRPC("createauxblock", params);
        } catch (...) {
            // 2. Try getauxblock (Namecoin-style)
            try {
                result = CallRPC("getauxblock", UniValue(UniValue::VARR));
            } catch (...) {
                // 3. Fallback: getblocktemplate (vanilla LTC/BTC)
                UniValue rules(UniValue::VARR);
                rules.push_back("segwit");
                rules.push_back("mweb");
                UniValue tplReq(UniValue::VOBJ);
                tplReq.pushKV("rules", rules);
                UniValue tplParams(UniValue::VARR);
                tplParams.push_back(tplReq);
                result = CallRPC("getblocktemplate", tplParams);
                usedBlockTemplate = true;
            }
        }

        if (!result.isObject()) {
            LogPrintf("MergeMine: %s returned non-object for work\n",
                      m_cfg.name);
            return false;
        }

        LOCK(m_mutex);
        m_currentWork.chainName = m_cfg.name;
        m_currentWork.chainId = m_cfg.chainId;

        if (usedBlockTemplate) {
            // getblocktemplate provides previousblockhash as the reference
            if (result.exists("previousblockhash")) {
                m_currentWork.auxHash =
                    uint256S(result["previousblockhash"].get_str());
            }
        } else if (result.exists("hash")) {
            m_currentWork.auxHash = uint256S(result["hash"].get_str());
        }

        if (result.exists("bits")) {
            std::string bitsStr = result["bits"].get_str();
            m_currentWork.nBits =
                static_cast<uint32_t>(strtoul(bitsStr.c_str(), nullptr, 16));
        }
        if (result.exists("height")) {
            m_currentWork.height =
                static_cast<int32_t>(result["height"].getInt<int64_t>());
        }
        if (result.exists("target")) {
            m_currentWork.target = result["target"].get_str();
        }
        m_currentWork.fetchedAt = GetTime();

        LogPrint(BCLog::MERGEMINE,
                 "MergeMine: refreshed %s work — height=%d hash=%s%s\n",
                 m_cfg.name, m_currentWork.height,
                 m_currentWork.auxHash.GetHex().substr(0, 16),
                 usedBlockTemplate ? " (via getblocktemplate)" : "");

        return true;
    } catch (const std::exception &e) {
        LogPrintf("MergeMine: %s refresh failed — %s\n", m_cfg.name,
                  e.what());
        return false;
    }
}

ExternalSubmitResult
ExternalChainClient::SubmitAuxBlock(const uint256 &hash,
                                   const std::string &auxpowHex) {
    try {
        UniValue params(UniValue::VARR);
        params.push_back(hash.GetHex());
        params.push_back(auxpowHex);

        UniValue result;
        try {
            result = CallRPC("submitauxblock", params);
        } catch (...) {
            try {
                // Fallback for getauxblock-style submission (Namecoin)
                result = CallRPC("getauxblock", params);
            } catch (...) {
                // Fallback for vanilla chains (LTC/BTC) via submitblock
                UniValue sbParams(UniValue::VARR);
                sbParams.push_back(auxpowHex);
                result = CallRPC("submitblock", sbParams);
            }
        }

        return {true, ""};
    } catch (const std::exception &e) {
        return {false, e.what()};
    }
}

ExternalAuxWork ExternalChainClient::GetCurrentWork() const {
    LOCK(m_mutex);
    return m_currentWork;
}

// --- MergeMineManager ---

MergeMineManager::MergeMineManager() = default;

MergeMineManager::~MergeMineManager() {
    Stop();
}

void MergeMineManager::AddChain(const ExternalChainConfig &cfg) {
    LOCK(m_mutex);
    m_chains.push_back(std::make_unique<ExternalChainClient>(cfg));
    LogPrintf("MergeMine: added chain %s (%s:%d, chainId=%d)\n", cfg.name,
              cfg.rpcHost, cfg.rpcPort, cfg.chainId);
}

void MergeMineManager::AddModule(std::unique_ptr<IAuxChainModule> module) {
    LOCK(m_mutex);
    LogPrintf("MergeMine: added module %s (chainId=%d)\n",
              module->Name(), module->ChainId());
    m_chains.push_back(std::move(module));
}

void MergeMineManager::Start(const std::string &payoutAddress) {
    m_payoutAddress = payoutAddress;
    m_running.store(true);

    LOCK(m_mutex);
    for (size_t i = 0; i < m_chains.size(); ++i) {
        m_pollThreads.emplace_back([this, i]() { PollLoop(i); });
    }

    LogPrintf("MergeMine: started polling %zu external chain(s)\n",
              m_chains.size());
}

std::vector<ExternalAuxWork> MergeMineManager::GetAllWorkSorted() const {
    std::vector<ExternalAuxWork> result;
    LOCK(m_mutex);
    for (const auto &chain : m_chains) {
        auto work = chain->GetCurrentWork();
        if (!work.auxHash.IsNull()) {
            result.push_back(work);
        }
    }
    std::sort(result.begin(), result.end(),
              [](const ExternalAuxWork &a, const ExternalAuxWork &b) {
                  return a.chainId < b.chainId;
              });
    return result;
}

void MergeMineManager::Stop() {
    m_running.store(false);
    for (auto &t : m_pollThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    m_pollThreads.clear();
    LogPrintf("MergeMine: stopped\n");
}

void MergeMineManager::PollLoop(size_t chainIndex) {
    IAuxChainModule *client = nullptr;
    int intervalMs = 5000;

    {
        LOCK(m_mutex);
        if (chainIndex >= m_chains.size()) {
            return;
        }
        client = m_chains[chainIndex].get();
        intervalMs = client->PollIntervalMs();
        if (intervalMs <= 0) {
            intervalMs = 5000;
        }
    }

    while (m_running.load() && !ShutdownRequested()) {
        if (client->RefreshWork(m_payoutAddress)) {
            if (m_workCallback) {
                m_workCallback(client->Name(),
                               client->GetCurrentWork());
            }
        }
        // Sleep in short intervals so we can respond to shutdown quickly
        for (int elapsed = 0;
             elapsed < intervalMs && m_running.load() && !ShutdownRequested();
             elapsed += 500) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

std::map<std::string, ExternalAuxWork> MergeMineManager::GetAllWork() const {
    std::map<std::string, ExternalAuxWork> result;
    LOCK(m_mutex);
    for (const auto &chain : m_chains) {
        auto work = chain->GetCurrentWork();
        if (!work.auxHash.IsNull()) {
            result[work.chainName] = work;
        }
    }
    return result;
}

std::vector<uint256> MergeMineManager::GetAuxHashes() const {
    auto sorted = GetAllWorkSorted();
    std::vector<uint256> hashes;
    hashes.reserve(sorted.size());
    for (const auto &w : sorted) {
        hashes.push_back(w.auxHash);
    }
    return hashes;
}

ExternalSubmitResult
MergeMineManager::SubmitToChain(const std::string &chainName,
                                const uint256 &hash,
                                const std::string &auxpowHex) {
    LOCK(m_mutex);
    for (auto &chain : m_chains) {
        if (chain->Name() == chainName) {
            return chain->SubmitAuxBlock(hash, auxpowHex);
        }
    }
    return {false, "Chain not found: " + chainName};
}

std::map<std::string, ExternalSubmitResult>
MergeMineManager::SubmitToAll(const std::string &auxpowHex) {
    std::map<std::string, ExternalSubmitResult> results;
    LOCK(m_mutex);
    for (auto &chain : m_chains) {
        auto work = chain->GetCurrentWork();
        if (!work.auxHash.IsNull()) {
            results[work.chainName] =
                chain->SubmitAuxBlock(work.auxHash, auxpowHex);
        }
    }
    return results;
}

size_t MergeMineManager::ChainCount() const {
    LOCK(m_mutex);
    return m_chains.size();
}

void MergeMineManager::SetWorkCallback(WorkCallback cb) {
    m_workCallback = std::move(cb);
}

} // namespace stratum
