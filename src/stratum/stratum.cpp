// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratum.h>

#include <arith_uint256.h>
#include <chainparams.h>
#include <key_io.h>
#include <logging.h>
#include <pow/pow.h>
#include <script/standard.h>
#include <shutdown.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <validation.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/thread.h>

#include <compat.h>

namespace stratum {

static std::unique_ptr<StratumServer> g_stratumServer;

// --- StratumServer ---

StratumServer::StratumServer(const StratumConfig &config,
                             Chainstate &chainstate,
                             const CTxMemPool *mempool,
                             const CChainParams &chainParams,
                             ChainstateManager &chainman)
    : m_config(config), m_chainstate(chainstate), m_chainParams(chainParams),
      m_chainman(chainman) {

    // Determine coinbase script
    CScript coinbaseScript;
    if (!m_config.coinbaseAddress.empty()) {
        CTxDestination dest =
            DecodeDestination(m_config.coinbaseAddress, chainParams);
        if (IsValidDestination(dest)) {
            coinbaseScript = GetScriptForDestination(dest);
        }
    }
    if (coinbaseScript.empty()) {
        coinbaseScript = CScript() << OP_TRUE;
    }

    // Build upstream pool list from config
    std::vector<UpstreamPool> pools;
    for (const auto &entry : m_config.upstreamPools) {
        UpstreamPool pool;
        pool.host = entry.host;
        pool.port = entry.port;
        pool.username = entry.username;
        pool.password = entry.password;
        pool.priority = entry.priority;
        pools.push_back(pool);
    }

    m_router = std::make_unique<StratumRouter>(
        config, pools, chainstate, mempool, chainParams, chainman,
        coinbaseScript);

    m_auxMgr = std::make_unique<StratumAuxManager>(
        chainstate, mempool, chainParams, coinbaseScript);
}

StratumServer::~StratumServer() {
    Stop();
}

bool StratumServer::Start() {
    m_startTime = GetTime();

    // Thread-safety: BroadcastJob/BroadcastRawNotify are called from the
    // validation thread while the event loop runs on m_eventThread.
#ifdef WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif

    m_eventBase = event_base_new();
    if (!m_eventBase) {
        LogPrintf("Stratum: failed to create event base\n");
        return false;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(m_config.port);
    if (inet_pton(AF_INET, m_config.bind.c_str(), &sin.sin_addr) <= 0) {
        sin.sin_addr.s_addr = INADDR_ANY;
    }

    m_listener = evconnlistener_new_bind(
        m_eventBase, OnAccept, this,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr *)&sin, sizeof(sin));

    if (!m_listener) {
        LogPrintf("Stratum: failed to bind to %s:%d\n", m_config.bind,
                  m_config.port);
        event_base_free(m_eventBase);
        m_eventBase = nullptr;
        return false;
    }

    m_running.store(true);
    m_interrupt.store(false);

    // Wire router callbacks so it can push data to our miners
    RouterDownstreamCallbacks routerCbs;
    routerCbs.broadcastNotify = [this](const std::string &raw) {
        BroadcastRawNotify(raw);
    };
    routerCbs.broadcastDifficulty = [this](double diff) {
        BroadcastDifficultyAll(diff);
    };
    routerCbs.submitResult = [this](int64_t minerId, bool accepted,
                                     const std::string &error) {
        HandleProxySubmitResult(minerId, accepted, error);
    };
    routerCbs.tierChanged = [this](RoutingTier tier,
                                    const std::string &detail) {
        LogPrintf("Stratum: routing tier → %s (%s)\n",
                  TierName(tier), detail);
    };
    m_router->SetCallbacks(std::move(routerCbs));

    // Start the router (connects proxies, evaluates tiers)
    if (!m_router->Start()) {
        LogPrintf("Stratum: failed to start router\n");
    }

    // Register for chain tip notifications
    RegisterValidationInterface(this);

    // Register stats HTTP endpoint
    RegisterStratumHTTPHandlers([this]() { return GetStats(); });

    // Start event loop in a background thread
    m_eventThread = std::thread([this]() { EventLoop(); });

    LogPrintf("Stratum: server started on %s:%d (tier: %s)\n",
              m_config.bind, m_config.port,
              TierName(m_router->GetActiveTier()));

    return true;
}

void StratumServer::Interrupt() {
    m_interrupt.store(true);
    if (m_eventBase) {
        event_base_loopbreak(m_eventBase);
    }
}

void StratumServer::Stop() {
    if (!m_running.load()) {
        return;
    }

    Interrupt();

    if (m_eventThread.joinable()) {
        m_eventThread.join();
    }

    if (m_router) {
        m_router->Stop();
    }

    UnregisterValidationInterface(this);
    UnregisterStratumHTTPHandlers();

    {
        LOCK(m_cs);
        for (auto &[id, session] : m_sessions) {
            if (session->bev) {
                bufferevent_free(session->bev);
                session->bev = nullptr;
            }
        }
        m_sessions.clear();
    }

    if (m_listener) {
        evconnlistener_free(m_listener);
        m_listener = nullptr;
    }

    if (m_eventBase) {
        event_base_free(m_eventBase);
        m_eventBase = nullptr;
    }

    m_running.store(false);

    LogPrintf("Stratum: server stopped\n");
}

void StratumServer::EventLoop() {
    while (!m_interrupt.load() && !ShutdownRequested()) {
        // Run event loop with a 1-second timeout for periodic checks
        struct timeval tv = {1, 0};
        event_base_loopexit(m_eventBase, &tv);
        event_base_dispatch(m_eventBase);

        if (!m_interrupt.load() && !ShutdownRequested()) {
            PeriodicMaintenance();
        }
    }
}

// --- libevent callbacks ---

void StratumServer::OnAccept(struct evconnlistener *, int fd,
                              struct sockaddr *addr, int, void *ctx) {
    auto *server = static_cast<StratumServer *>(ctx);
    server->HandleAccept(fd, addr);
}

void StratumServer::OnRead(struct bufferevent *bev, void *ctx) {
    auto *pair = static_cast<std::pair<StratumServer *, uint32_t> *>(ctx);
    pair->first->HandleRead(pair->second);
}

void StratumServer::OnEvent(struct bufferevent *bev, short events, void *ctx) {
    auto *pair = static_cast<std::pair<StratumServer *, uint32_t> *>(ctx);
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT)) {
        pair->first->HandleDisconnect(pair->second);
        delete pair;
    }
}

void StratumServer::HandleAccept(int fd, struct sockaddr *addr) {
    LOCK(m_cs);

    if ((int)m_sessions.size() >= m_config.maxWorkers) {
        close(fd);
        LogPrint(BCLog::STRATUM, "Stratum: rejected connection (max workers)\n");
        return;
    }

    uint32_t sessionId = m_nextSessionId++;
    std::string extranonce1 = m_extranonceMgr.Next();

    auto session = std::make_unique<ClientSession>();
    session->sessionId = sessionId;
    session->worker = std::make_unique<StratumWorker>(
        sessionId, extranonce1, m_config.defaultDifficulty);

    struct bufferevent *bev =
        bufferevent_socket_new(m_eventBase, fd,
                               BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    session->bev = bev;

    // Store context for callbacks
    auto *cbCtx = new std::pair<StratumServer *, uint32_t>(this, sessionId);
    bufferevent_setcb(bev, OnRead, nullptr, OnEvent, cbCtx);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    struct timeval tv = {m_config.workerTimeoutSec, 0};
    bufferevent_set_timeouts(bev, &tv, nullptr);

    m_sessions[sessionId] = std::move(session);
    m_totalConnections++;

    char addrStr[INET_ADDRSTRLEN] = {};
    if (addr && addr->sa_family == AF_INET) {
        inet_ntop(AF_INET, &((struct sockaddr_in *)addr)->sin_addr, addrStr,
                  sizeof(addrStr));
    }
    LogPrint(BCLog::STRATUM, "Stratum: new connection #%d from %s\n",
             sessionId, addrStr);
}

void StratumServer::HandleRead(uint32_t sessionId) {
    LOCK(m_cs);
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return;
    }
    auto &session = *it->second;

    struct evbuffer *input = bufferevent_get_input(session.bev);
    size_t len = evbuffer_get_length(input);
    if (len == 0) {
        return;
    }

    std::vector<char> buf(len);
    evbuffer_remove(input, buf.data(), len);

    session.lineBuffer.Append(buf.data(), len);

    if (session.lineBuffer.OverflowDetected()) {
        LogPrint(BCLog::STRATUM, "Stratum: line overflow from #%d\n",
                 sessionId);
        HandleDisconnect(sessionId);
        return;
    }

    auto lines = session.lineBuffer.ExtractLines();
    for (const auto &line : lines) {
        auto msgResult = ParseStratumLine(line);
        if (msgResult) {
            ProcessMessage(session, *msgResult);
        }
    }

    session.worker->Touch(GetTime());
}

void StratumServer::HandleDisconnect(uint32_t sessionId) {
    LOCK(m_cs);
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return;
    }

    if (it->second->bev) {
        bufferevent_free(it->second->bev);
        it->second->bev = nullptr;
    }
    m_sessions.erase(it);

    LogPrint(BCLog::STRATUM, "Stratum: client #%d disconnected\n", sessionId);
}

void StratumServer::ProcessMessage(ClientSession &session,
                                    const StratumMessage &msg) {
    if (auto *req = std::get_if<StratumRequest>(&msg)) {
        if (req->method == "mining.subscribe") {
            HandleSubscribe(session, *req);
        } else if (req->method == "mining.authorize") {
            HandleAuthorize(session, *req);
        } else if (req->method == "mining.submit") {
            HandleSubmit(session, *req);
        } else {
            // Unknown method
            UniValue err(UniValue::VARR);
            err.push_back(20);
            err.push_back("Unknown method");
            err.push_back(UniValue(UniValue::VNULL));
            SendResponse(session, req->id, UniValue(UniValue::VNULL), err);
        }
    }
    // Notifications and responses from client are ignored
}

void StratumServer::HandleSubscribe(ClientSession &session,
                                     const StratumRequest &req) {
    auto result = session.worker->HandleSubscribe(req.params);
    if (!result) {
        UniValue err(UniValue::VARR);
        err.push_back(25);
        err.push_back(util::ErrorString(result).original);
        err.push_back(UniValue(UniValue::VNULL));
        SendResponse(session, req.id, UniValue(UniValue::VNULL), err);
        return;
    }
    SendResponse(session, req.id, *result, UniValue(UniValue::VNULL));
}

void StratumServer::HandleAuthorize(ClientSession &session,
                                     const StratumRequest &req) {
    auto result = session.worker->HandleAuthorize(req.params);
    if (!result) {
        UniValue err(UniValue::VARR);
        err.push_back(24);
        err.push_back(util::ErrorString(result).original);
        err.push_back(UniValue(UniValue::VNULL));
        SendResponse(session, req.id, UniValue(UniValue::VNULL), err);
        return;
    }
    SendResponse(session, req.id, *result, UniValue(UniValue::VNULL));

    // Send initial difficulty
    SendDifficulty(session.sessionId, m_config.defaultDifficulty);

    // Send current job if in local mode
    if (m_router && m_router->GetActiveTier() == RoutingTier::LOCAL) {
        auto *jobMgr = m_router->GetJobManager();
        if (jobMgr && jobMgr->JobCount() > 0) {
            auto jobResult = jobMgr->CreateJob(false);
            if (jobResult) {
                UniValue notifyParams =
                    jobMgr->FormatNotifyParams(*jobResult);
                std::string data =
                    SerializeNotify("mining.notify", notifyParams);
                SendToClient(session, data);
            }
        }
    }
    // In proxy mode, the next upstream mining.notify will be relayed
}

void StratumServer::HandleSubmit(ClientSession &session,
                                  const StratumRequest &req) {
    AssertLockHeld(m_cs);
    if (session.worker->GetState() != StratumWorker::State::AUTHORIZED) {
        UniValue err(UniValue::VARR);
        err.push_back(StratumError::UNAUTHORIZED);
        err.push_back("Not authorized");
        err.push_back(UniValue(UniValue::VNULL));
        SendResponse(session, req.id, UniValue(UniValue::VNULL), err);
        return;
    }

    RoutingTier tier = m_router ? m_router->GetActiveTier()
                                : RoutingTier::NONE;

    // --- Proxy mode: forward upstream, response comes via callback ---
    if (tier == RoutingTier::PROXY) {
        // We're already under m_cs from HandleRead → ProcessMessage
        m_pendingProxySubmits[req.id] = session.sessionId;
        if (!m_router->RouteSubmit(req.id, 0, "", req.params, nullptr)) {
            m_pendingProxySubmits.erase(req.id);
            UniValue err(UniValue::VARR);
            err.push_back(20);
            err.push_back("No upstream available");
            err.push_back(UniValue(UniValue::VNULL));
            SendResponse(session, req.id, UniValue(false), err);
        }
        return;
    }

    // --- Local mode: validate share locally ---
    if (tier == RoutingTier::NONE) {
        UniValue err(UniValue::VARR);
        err.push_back(20);
        err.push_back("No mining backend available");
        err.push_back(UniValue(UniValue::VNULL));
        SendResponse(session, req.id, UniValue(false), err);
        return;
    }

    auto subResult = ParseSubmitParams(req.params);
    if (!subResult) {
        UniValue err(UniValue::VARR);
        err.push_back(20);
        err.push_back(util::ErrorString(subResult).original);
        err.push_back(UniValue(UniValue::VNULL));
        SendResponse(session, req.id, UniValue(false), err);
        return;
    }

    auto *jobMgr = m_router->GetJobManager();
    const StratumJob *job = jobMgr ? jobMgr->GetJob(subResult->jobId)
                                    : nullptr;
    if (!job) {
        session.worker->RecordShareStale();
        m_totalSharesStale++;
        UniValue err(UniValue::VARR);
        err.push_back(StratumError::JOB_NOT_FOUND);
        err.push_back("Job not found");
        err.push_back(UniValue(UniValue::VNULL));
        SendResponse(session, req.id, UniValue(false), err);
        return;
    }

    ShareResult result = ValidateShare(
        *job, session.worker->GetExtranonce1(), *subResult,
        session.worker->GetCurrentDifficulty(),
        m_chainParams.GetConsensus(), session.submittedNonces);

    switch (result) {
        case ShareResult::ACCEPTED_BLOCK:
            session.worker->RecordShareAccepted(
                session.worker->GetCurrentDifficulty());
            m_totalSharesAccepted++;
            m_blocksFound++;
            SendResponse(session, req.id, UniValue(true),
                         UniValue(UniValue::VNULL));

            SubmitBlock(*job, session.worker->GetExtranonce1(), *subResult,
                        m_chainman, m_chainParams);

            LogPrintf("Stratum: BLOCK FOUND by worker %s at height %d!\n",
                      session.worker->GetWorkerName(), job->height);
            break;

        case ShareResult::ACCEPTED:
            session.worker->RecordShareAccepted(
                session.worker->GetCurrentDifficulty());
            m_totalSharesAccepted++;
            SendResponse(session, req.id, UniValue(true),
                         UniValue(UniValue::VNULL));
            break;

        case ShareResult::REJECTED_LOW_DIFF: {
            session.worker->RecordShareRejected();
            m_totalSharesRejected++;
            UniValue err(UniValue::VARR);
            err.push_back(StratumError::LOW_DIFFICULTY);
            err.push_back("Low difficulty share");
            err.push_back(UniValue(UniValue::VNULL));
            SendResponse(session, req.id, UniValue(false), err);
            break;
        }

        case ShareResult::REJECTED_DUPLICATE: {
            session.worker->RecordShareRejected();
            m_totalSharesRejected++;
            UniValue err(UniValue::VARR);
            err.push_back(StratumError::DUPLICATE_SHARE);
            err.push_back("Duplicate share");
            err.push_back(UniValue(UniValue::VNULL));
            SendResponse(session, req.id, UniValue(false), err);
            break;
        }

        case ShareResult::REJECTED_STALE: {
            session.worker->RecordShareStale();
            m_totalSharesStale++;
            UniValue err(UniValue::VARR);
            err.push_back(StratumError::JOB_NOT_FOUND);
            err.push_back("Stale share");
            err.push_back(UniValue(UniValue::VNULL));
            SendResponse(session, req.id, UniValue(false), err);
            break;
        }

        case ShareResult::REJECTED_INVALID: {
            session.worker->RecordShareRejected();
            m_totalSharesRejected++;
            UniValue err(UniValue::VARR);
            err.push_back(20);
            err.push_back("Invalid share");
            err.push_back(UniValue(UniValue::VNULL));
            SendResponse(session, req.id, UniValue(false), err);
            break;
        }
    }
}

void StratumServer::SendToClient(ClientSession &session,
                                  const std::string &data) {
    if (!session.bev) {
        return;
    }
    bufferevent_write(session.bev, data.data(), data.size());
}

void StratumServer::SendResponse(ClientSession &session, int64_t id,
                                  const UniValue &result,
                                  const UniValue &error) {
    SendToClient(session, SerializeResponse(id, result, error));
}

void StratumServer::BroadcastJob(const StratumJob &job) {
    auto *jobMgr = m_router ? m_router->GetJobManager() : nullptr;
    if (!jobMgr) {
        return;
    }
    LOCK(m_cs);
    UniValue notifyParams = jobMgr->FormatNotifyParams(job);
    std::string data = SerializeNotify("mining.notify", notifyParams);

    for (auto &[id, session] : m_sessions) {
        if (session->worker->GetState() == StratumWorker::State::AUTHORIZED) {
            SendToClient(*session, data);
        }
    }
}

void StratumServer::BroadcastRawNotify(const std::string &rawLine) {
    LOCK(m_cs);
    for (auto &[id, session] : m_sessions) {
        if (session->worker->GetState() == StratumWorker::State::AUTHORIZED) {
            SendToClient(*session, rawLine);
        }
    }
}

void StratumServer::BroadcastDifficultyAll(double difficulty) {
    LOCK(m_cs);
    UniValue params(UniValue::VARR);
    params.push_back(difficulty);
    std::string data = SerializeNotify("mining.set_difficulty", params);
    for (auto &[id, session] : m_sessions) {
        if (session->worker->GetState() == StratumWorker::State::AUTHORIZED) {
            SendToClient(*session, data);
        }
    }
}

void StratumServer::HandleProxySubmitResult(int64_t minerId, bool accepted,
                                             const std::string &error) {
    LOCK(m_cs);
    // Find the session that issued this submit
    auto pendingIt = m_pendingProxySubmits.find(minerId);
    if (pendingIt == m_pendingProxySubmits.end()) {
        return;
    }
    uint32_t sessionId = pendingIt->second;
    m_pendingProxySubmits.erase(pendingIt);

    auto sessionIt = m_sessions.find(sessionId);
    if (sessionIt == m_sessions.end()) {
        return;
    }

    auto &session = *sessionIt->second;
    if (accepted) {
        session.worker->RecordShareAccepted(
            session.worker->GetCurrentDifficulty());
        m_totalSharesAccepted++;
        SendResponse(session, minerId, UniValue(true),
                     UniValue(UniValue::VNULL));
    } else {
        session.worker->RecordShareRejected();
        m_totalSharesRejected++;
        UniValue err(UniValue::VARR);
        err.push_back(20);
        err.push_back(error.empty() ? "Rejected by upstream" : error);
        err.push_back(UniValue(UniValue::VNULL));
        SendResponse(session, minerId, UniValue(false), err);
    }
}

void StratumServer::SendDifficulty(uint32_t sessionId, double difficulty) {
    LOCK(m_cs);
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return;
    }

    UniValue params(UniValue::VARR);
    params.push_back(difficulty);
    std::string data = SerializeNotify("mining.set_difficulty", params);
    SendToClient(*it->second, data);
}

StratumServerStats StratumServer::GetStats() const {
    StratumServerStats stats;
    stats.startTime = m_startTime;
    stats.totalConnections = m_totalConnections.load();
    stats.totalSharesAccepted = m_totalSharesAccepted.load();
    stats.totalSharesRejected = m_totalSharesRejected.load();
    stats.totalSharesStale = m_totalSharesStale.load();
    stats.blocksFound = m_blocksFound.load();

    {
        LOCK(m_cs);
        stats.activeWorkers = m_sessions.size();
        for (const auto &[id, session] : m_sessions) {
            stats.workers.push_back(session->worker->GetStats());
        }
    }

    // Routing info
    if (m_router) {
        stats.activeTier = TierName(m_router->GetActiveTier());
        stats.activeProxyIndex = m_router->GetActiveProxyIndex();
        for (size_t i = 0; i < m_router->GetProxyCount(); ++i) {
            ProxyHealth h = m_router->GetProxyHealth(i);
            UpstreamPoolStats ps;
            ps.label = m_config.upstreamPools.size() > i
                           ? (m_config.upstreamPools[i].host + ":" +
                              std::to_string(m_config.upstreamPools[i].port))
                           : "unknown";
            ps.connected = h.connected;
            ps.healthy = h.connected && h.authorized &&
                         h.consecutiveErrors < 5;
            ps.priority = m_config.upstreamPools.size() > i
                              ? m_config.upstreamPools[i].priority
                              : 0;
            ps.consecutiveErrors = h.consecutiveErrors;
            ps.lastError = h.lastError;
            stats.upstreamPools.push_back(ps);
        }
    }

    // Network difficulty from chain tip
    LOCK(cs_main);
    const CBlockIndex *tip = m_chainstate.m_chain.Tip();
    if (tip) {
        stats.chainHeight = tip->nHeight;
        arith_uint256 target;
        if (NBitsToTarget(m_chainParams.GetConsensus(), tip->nBits, target) &&
            target > arith_uint256(0)) {
            arith_uint256 diff1;
            diff1.SetCompact(0x1d00ffff);
            stats.networkDifficulty =
                diff1.getdouble() / target.getdouble();
        }
    }

    return stats;
}

size_t StratumServer::GetWorkerCount() const {
    LOCK(m_cs);
    return m_sessions.size();
}

void StratumServer::UpdatedBlockTip(const CBlockIndex *pindexNew,
                                     const CBlockIndex *,
                                     bool fInitialDownload) {
    if (fInitialDownload) {
        return;
    }
    if (!m_running.load()) {
        return;
    }

    LogPrint(BCLog::STRATUM, "Stratum: new tip at height %d\n",
             pindexNew->nHeight);

    // Delegate to the router — it decides whether to create a local job
    // or whether a proxy is handling notifications.
    if (m_router) {
        m_router->OnNewTip(pindexNew->nHeight);
    }
}

void StratumServer::CreateAndBroadcastJob(bool cleanJobs) {
    // Delegate to router for local-mode job creation
    if (m_router && m_router->GetActiveTier() == RoutingTier::LOCAL) {
        auto *jobMgr = m_router->GetJobManager();
        if (!jobMgr) {
            return;
        }
        auto jobResult = jobMgr->CreateJob(cleanJobs);
        if (!jobResult) {
            LogPrintf("Stratum: failed to create job: %s\n",
                      util::ErrorString(jobResult).original);
            return;
        }
        jobMgr->PruneJobs(m_config.jobCacheSize);
        BroadcastJob(*jobResult);
    }
}

void StratumServer::PeriodicMaintenance() {
    LOCK(m_cs);
    int64_t now = GetTime();

    // Clean up timed-out workers
    std::vector<uint32_t> toRemove;
    for (auto &[id, session] : m_sessions) {
        if (session->worker->IsTimedOut(m_config.workerTimeoutSec, now)) {
            toRemove.push_back(id);
            continue;
        }

        // Vardiff retarget
        if (session->worker->ShouldRetargetDifficulty(m_config, now)) {
            double newDiff = session->worker->CalcNewDifficulty(m_config, now);
            if (std::abs(newDiff - session->worker->GetCurrentDifficulty()) /
                    session->worker->GetCurrentDifficulty() >
                m_config.varDiffVariance) {
                session->worker->SetDifficulty(newDiff);
                SendDifficulty(id, newDiff);
            }
        }
    }

    for (uint32_t id : toRemove) {
        LogPrint(BCLog::STRATUM, "Stratum: timing out worker #%d\n", id);
        auto it = m_sessions.find(id);
        if (it != m_sessions.end()) {
            if (it->second->bev) {
                bufferevent_free(it->second->bev);
                it->second->bev = nullptr;
            }
            m_sessions.erase(it);
        }
    }
}

// --- Global lifecycle ---

bool InitStratumServer(const StratumConfig &config, Chainstate &chainstate,
                       const CTxMemPool *mempool,
                       const CChainParams &chainParams,
                       ChainstateManager &chainman) {
    g_stratumServer = std::make_unique<StratumServer>(
        config, chainstate, mempool, chainParams, chainman);
    return true;
}

void StartStratumServer() {
    if (g_stratumServer) {
        if (!g_stratumServer->Start()) {
            LogPrintf("Stratum: failed to start server\n");
            g_stratumServer.reset();
        }
    }
}

void InterruptStratumServer() {
    if (g_stratumServer) {
        g_stratumServer->Interrupt();
    }
}

void StopStratumServer() {
    if (g_stratumServer) {
        g_stratumServer->Stop();
        g_stratumServer.reset();
    }
}

} // namespace stratum
