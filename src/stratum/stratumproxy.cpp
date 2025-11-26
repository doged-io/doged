// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumproxy.h>

#include <logging.h>
#include <tinyformat.h>
#include <util/time.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace stratum {

StratumProxyConn::StratumProxyConn(const UpstreamPool &pool,
                                   const ProxyCallbacks &callbacks)
    : m_pool(pool), m_callbacks(callbacks) {}

StratumProxyConn::~StratumProxyConn() {
    Disconnect();
}

std::string StratumProxyConn::GetLabel() const {
    return strprintf("%s:%d", m_pool.host, m_pool.port);
}

bool StratumProxyConn::Connect() {
    if (m_connected.load()) {
        return true;
    }

    LogPrintf("Stratum proxy: connecting to %s\n", GetLabel());

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = strprintf("%d", m_pool.port);
    int rv = getaddrinfo(m_pool.host.c_str(), portStr.c_str(), &hints, &res);
    if (rv != 0 || !res) {
        RecordError(strprintf("DNS resolution failed: %s", gai_strerror(rv)));
        return false;
    }

    m_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_sockfd < 0) {
        RecordError(strprintf("socket() failed: %s", strerror(errno)));
        freeaddrinfo(res);
        return false;
    }

    // Non-blocking connect with timeout
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

    int connectRv = connect(m_sockfd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (connectRv < 0 && errno != EINPROGRESS) {
        RecordError(strprintf("connect() failed: %s", strerror(errno)));
        close(m_sockfd);
        m_sockfd = -1;
        return false;
    }

    if (connectRv < 0) {
        // Wait for connection to complete
        struct pollfd pfd;
        pfd.fd = m_sockfd;
        pfd.events = POLLOUT;
        int pollRv = poll(&pfd, 1, CONNECT_TIMEOUT_SEC * 1000);
        if (pollRv <= 0) {
            RecordError("Connection timed out");
            close(m_sockfd);
            m_sockfd = -1;
            return false;
        }
        int sockerr = 0;
        socklen_t len = sizeof(sockerr);
        getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &len);
        if (sockerr != 0) {
            RecordError(strprintf("connect() error: %s", strerror(sockerr)));
            close(m_sockfd);
            m_sockfd = -1;
            return false;
        }
    }

    // Back to blocking for simplicity in the read thread
    fcntl(m_sockfd, F_SETFL, flags);

    // TCP keepalive
    int one = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
    setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    m_connected.store(true);
    m_interrupt.store(false);
    {
        std::lock_guard<std::mutex> lock(m_healthMutex);
        m_health.connected = true;
        m_health.connectTime = GetTime();
        m_health.consecutiveErrors = 0;
    }

    LogPrintf("Stratum proxy: connected to %s\n", GetLabel());

    // Subscribe + authorize
    if (!DoSubscribe()) {
        Disconnect();
        return false;
    }
    if (!DoAuthorize()) {
        Disconnect();
        return false;
    }

    if (m_callbacks.onStateChange) {
        m_callbacks.onStateChange(true, "connected");
    }

    // Start read thread
    m_readThread = std::thread([this]() { ReadLoop(); });

    return true;
}

void StratumProxyConn::Disconnect() {
    m_interrupt.store(true);
    m_connected.store(false);
    m_authorized.store(false);

    if (m_sockfd >= 0) {
        shutdown(m_sockfd, SHUT_RDWR);
        close(m_sockfd);
        m_sockfd = -1;
    }

    if (m_readThread.joinable()) {
        m_readThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(m_healthMutex);
        m_health.connected = false;
        m_health.authorized = false;
    }

    if (m_callbacks.onStateChange) {
        m_callbacks.onStateChange(false, "disconnected");
    }

    LogPrint(BCLog::STRATUM, "Stratum proxy: disconnected from %s\n",
             GetLabel());
}

bool StratumProxyConn::IsHealthy() const {
    std::lock_guard<std::mutex> lock(m_healthMutex);
    if (!m_health.connected || !m_health.authorized) {
        return false;
    }
    if (m_health.consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
        return false;
    }
    int64_t now = GetTime();
    if (m_health.lastNotifyTime > 0 &&
        (now - m_health.lastNotifyTime) > HEALTHY_NOTIFY_STALE_SEC) {
        return false;
    }
    return true;
}

ProxyHealth StratumProxyConn::GetHealth() const {
    std::lock_guard<std::mutex> lock(m_healthMutex);
    return m_health;
}

std::string StratumProxyConn::GetUpstreamExtranonce1() const {
    return m_upstreamExtranonce1;
}

int StratumProxyConn::GetUpstreamExtranonce2Size() const {
    return m_upstreamExtranonce2Size;
}

void StratumProxyConn::ForwardSubmit(int64_t minerId,
                                      const UniValue &params) {
    int64_t upstreamId;
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        upstreamId = m_nextReqId++;
        m_pendingSubmits[upstreamId] = minerId;
    }

    std::string data = SerializeRequest(upstreamId, "mining.submit", params);
    if (!SendRaw(data)) {
        // Send failed — report rejection back
        if (m_callbacks.onSubmitResult) {
            m_callbacks.onSubmitResult(minerId, false, "upstream send failed");
        }
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        m_pendingSubmits.erase(upstreamId);
    }
}

// --- Private ---

void StratumProxyConn::ReadLoop() {
    char buf[4096];
    while (!m_interrupt.load() && m_sockfd >= 0) {
        struct pollfd pfd;
        pfd.fd = m_sockfd;
        pfd.events = POLLIN;
        int rv = poll(&pfd, 1, 1000);

        if (rv < 0) {
            if (errno == EINTR) {
                continue;
            }
            RecordError(strprintf("poll() error: %s", strerror(errno)));
            break;
        }
        if (rv == 0) {
            continue;
        }

        ssize_t n = recv(m_sockfd, buf, sizeof(buf), 0);
        if (n <= 0) {
            if (n == 0) {
                LogPrint(BCLog::STRATUM,
                         "Stratum proxy: upstream %s closed connection\n",
                         GetLabel());
            } else {
                RecordError(
                    strprintf("recv() error: %s", strerror(errno)));
            }
            break;
        }

        m_lineBuffer.Append(buf, n);
        auto lines = m_lineBuffer.ExtractLines();
        for (const auto &line : lines) {
            HandleLine(line);
        }
    }

    m_connected.store(false);
    if (m_callbacks.onStateChange) {
        m_callbacks.onStateChange(false, "read loop ended");
    }
}

void StratumProxyConn::HandleLine(const std::string &line) {
    auto msgResult = ParseStratumLine(line);
    if (!msgResult) {
        return;
    }

    std::visit(
        [this, &line](auto &&msg) {
            using T = std::decay_t<decltype(msg)>;
            if constexpr (std::is_same_v<T, StratumResponse>) {
                HandleResponse(msg);
            } else if constexpr (std::is_same_v<T, StratumNotification>) {
                HandleNotification(msg);
            } else if constexpr (std::is_same_v<T, StratumRequest>) {
                // Upstream sending us a request is unusual but possible
                // (e.g. mining.set_extranonce). Ignore for now.
            }
        },
        *msgResult);
}

void StratumProxyConn::HandleResponse(const StratumResponse &resp) {
    // Check if this is a response to one of our submit forwards
    int64_t minerId = -1;
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        auto it = m_pendingSubmits.find(resp.id);
        if (it != m_pendingSubmits.end()) {
            minerId = it->second;
            m_pendingSubmits.erase(it);
        }
    }

    if (minerId >= 0 && m_callbacks.onSubmitResult) {
        bool accepted = resp.result.isBool() && resp.result.get_bool();
        std::string error;
        if (!resp.error.isNull() && resp.error.isArray() &&
            resp.error.size() >= 2) {
            error = resp.error[1].get_str();
        }
        m_callbacks.onSubmitResult(minerId, accepted, error);
    }
}

void StratumProxyConn::HandleNotification(const StratumNotification &notif) {
    if (notif.method == "mining.notify") {
        {
            std::lock_guard<std::mutex> lock(m_healthMutex);
            m_health.lastNotifyTime = GetTime();
            m_health.consecutiveErrors = 0;
        }
        if (m_callbacks.onNotify) {
            // Re-serialize and pass the raw line so router can forward
            std::string raw =
                SerializeNotify(notif.method, notif.params);
            m_callbacks.onNotify(raw);
        }
    } else if (notif.method == "mining.set_difficulty") {
        if (m_callbacks.onSetDifficulty && notif.params.isArray() &&
            notif.params.size() > 0) {
            double diff = notif.params[0].get_real();
            m_callbacks.onSetDifficulty(diff);
        }
    }
}

bool StratumProxyConn::SendRaw(const std::string &data) {
    std::lock_guard<std::mutex> lock(m_writeMutex);
    if (m_sockfd < 0) {
        return false;
    }
    ssize_t sent = send(m_sockfd, data.data(), data.size(), MSG_NOSIGNAL);
    if (sent < 0 || (size_t)sent != data.size()) {
        RecordError(strprintf("send() failed: %s", strerror(errno)));
        return false;
    }
    return true;
}

bool StratumProxyConn::DoSubscribe() {
    UniValue params(UniValue::VARR);
    params.push_back("doged-stratum/1.0");

    std::string req = SerializeRequest(1, "mining.subscribe", params);
    if (!SendRaw(req)) {
        return false;
    }

    // Read the response synchronously (we're still in Connect, no read thread)
    char buf[4096];
    struct pollfd pfd;
    pfd.fd = m_sockfd;
    pfd.events = POLLIN;

    int rv = poll(&pfd, 1, CONNECT_TIMEOUT_SEC * 1000);
    if (rv <= 0) {
        RecordError("Subscribe response timed out");
        return false;
    }

    ssize_t n = recv(m_sockfd, buf, sizeof(buf), 0);
    if (n <= 0) {
        RecordError("Subscribe recv failed");
        return false;
    }

    m_lineBuffer.Append(buf, n);
    auto lines = m_lineBuffer.ExtractLines();

    for (const auto &line : lines) {
        auto msgResult = ParseStratumLine(line);
        if (!msgResult) {
            continue;
        }
        auto *resp = std::get_if<StratumResponse>(&*msgResult);
        if (resp && resp->id == 1) {
            if (resp->result.isArray() && resp->result.size() >= 3) {
                m_upstreamExtranonce1 = resp->result[1].get_str();
                m_upstreamExtranonce2Size = resp->result[2].getInt<int>();
                LogPrint(BCLog::STRATUM,
                         "Stratum proxy: subscribed to %s, "
                         "extranonce1=%s, en2size=%d\n",
                         GetLabel(), m_upstreamExtranonce1,
                         m_upstreamExtranonce2Size);
                return true;
            }
        }
        // Handle any notifications that arrive with the subscribe response
        auto *notif = std::get_if<StratumNotification>(&*msgResult);
        if (notif) {
            HandleNotification(*notif);
        }
    }

    RecordError("Bad subscribe response");
    return false;
}

bool StratumProxyConn::DoAuthorize() {
    UniValue params(UniValue::VARR);
    params.push_back(m_pool.username);
    params.push_back(m_pool.password);

    std::string req = SerializeRequest(2, "mining.authorize", params);
    if (!SendRaw(req)) {
        return false;
    }

    char buf[4096];
    struct pollfd pfd;
    pfd.fd = m_sockfd;
    pfd.events = POLLIN;

    int rv = poll(&pfd, 1, CONNECT_TIMEOUT_SEC * 1000);
    if (rv <= 0) {
        RecordError("Authorize response timed out");
        return false;
    }

    ssize_t n = recv(m_sockfd, buf, sizeof(buf), 0);
    if (n <= 0) {
        RecordError("Authorize recv failed");
        return false;
    }

    m_lineBuffer.Append(buf, n);
    auto lines = m_lineBuffer.ExtractLines();

    for (const auto &line : lines) {
        auto msgResult = ParseStratumLine(line);
        if (!msgResult) {
            continue;
        }
        auto *resp = std::get_if<StratumResponse>(&*msgResult);
        if (resp && resp->id == 2) {
            if (resp->result.isBool() && resp->result.get_bool()) {
                m_authorized.store(true);
                {
                    std::lock_guard<std::mutex> lock(m_healthMutex);
                    m_health.authorized = true;
                }
                LogPrint(BCLog::STRATUM,
                         "Stratum proxy: authorized on %s as %s\n",
                         GetLabel(), m_pool.username);
                return true;
            }
        }
        auto *notif = std::get_if<StratumNotification>(&*msgResult);
        if (notif) {
            HandleNotification(*notif);
        }
    }

    RecordError("Authorization failed");
    return false;
}

void StratumProxyConn::RecordError(const std::string &err) {
    std::lock_guard<std::mutex> lock(m_healthMutex);
    m_health.lastErrorTime = GetTime();
    m_health.lastError = err;
    m_health.consecutiveErrors++;
    LogPrintf("Stratum proxy: [%s] %s\n", GetLabel(), err);
}

} // namespace stratum
