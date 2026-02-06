// Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stratum_client.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>

// Minimal JSON helpers (no external deps)
#include "json.h"

StratumClient::StratumClient(const std::string &host, uint16_t port,
                             const std::string &user, const std::string &pass,
                             StratumCallbacks cbs)
    : m_host(host), m_port(port), m_user(user), m_pass(pass),
      m_cbs(std::move(cbs)) {}

StratumClient::~StratumClient() {
    Disconnect();
}

bool StratumClient::Connect() {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(m_port);
    int rv = getaddrinfo(m_host.c_str(), portStr.c_str(), &hints, &res);
    if (rv != 0 || !res) {
        std::cerr << "DNS failed: " << gai_strerror(rv) << "\n";
        return false;
    }

    m_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_sockfd < 0) {
        freeaddrinfo(res);
        return false;
    }

    // Non-blocking connect with 10s timeout
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

    int cr = connect(m_sockfd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (cr < 0 && errno != EINPROGRESS) {
        close(m_sockfd);
        m_sockfd = -1;
        return false;
    }
    if (cr < 0) {
        struct pollfd pfd{m_sockfd, POLLOUT, 0};
        if (poll(&pfd, 1, 10000) <= 0) {
            close(m_sockfd);
            m_sockfd = -1;
            return false;
        }
        int sockerr = 0;
        socklen_t len = sizeof(sockerr);
        getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &len);
        if (sockerr != 0) {
            close(m_sockfd);
            m_sockfd = -1;
            return false;
        }
    }

    fcntl(m_sockfd, F_SETFL, flags);
    int one = 1;
    setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    m_connected.store(true);
    m_interrupt.store(false);

    if (!DoSubscribe()) {
        Disconnect();
        return false;
    }
    if (!DoAuthorize()) {
        Disconnect();
        return false;
    }

    m_readThread = std::thread([this]() { ReadLoop(); });

    if (m_cbs.onStateChange) {
        m_cbs.onStateChange(true, "connected");
    }
    return true;
}

void StratumClient::Disconnect() {
    m_interrupt.store(true);
    m_connected.store(false);
    if (m_sockfd >= 0) {
        shutdown(m_sockfd, SHUT_RDWR);
        close(m_sockfd);
        m_sockfd = -1;
    }
    if (m_readThread.joinable()) {
        m_readThread.join();
    }
}

bool StratumClient::SendJson(const std::string &json) {
    std::lock_guard<std::mutex> lock(m_writeMutex);
    if (m_sockfd < 0) return false;
    std::string data = json + "\n";
    ssize_t sent = send(m_sockfd, data.data(), data.size(), MSG_NOSIGNAL);
    return sent == (ssize_t)data.size();
}

std::string StratumClient::RecvLine(int timeoutSec) {
    std::string line;
    char c;
    auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSec);

    while (std::chrono::steady_clock::now() < deadline) {
        int remaining = std::max(
            1, (int)std::chrono::duration_cast<std::chrono::milliseconds>(
                   deadline - std::chrono::steady_clock::now())
                   .count());
        struct pollfd pfd;
        pfd.fd = m_sockfd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int rv = poll(&pfd, 1, remaining);
        if (rv <= 0) break;
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) break;
        ssize_t n = recv(m_sockfd, &c, 1, 0);
        if (n <= 0) break;
        if (c == '\n') return line;
        line += c;
    }
    return line;
}

bool StratumClient::DoSubscribe() {
    std::string req =
        R"({"id":1,"method":"mining.subscribe","params":["doged-miner/1.0"]})";
    if (!SendJson(req)) return false;

    for (int attempt = 0; attempt < 5; ++attempt) {
        std::string line = RecvLine(10);
        if (line.empty()) continue;

        auto j = json::parse(line);
        if (!j.is_object()) continue;

        // Check if this is a notification (no "id" or null id)
        if (j.contains("method")) {
            // Handle early notifications
            std::string method = j["method"].get_string();
            if (method == "mining.set_difficulty" && m_cbs.onDifficulty) {
                auto &params = j["params"];
                if (params.is_array() && params.size() > 0) {
                    m_cbs.onDifficulty(params[0].get_number());
                }
            }
            continue;
        }

        if (!j.contains("id") || j["id"].get_number() != 1) continue;

        auto &result = j["result"];
        if (!result.is_array() || result.size() < 3) return false;

        m_extranonce1 = result[1].get_string();
        m_extranonce2Size = (int)result[2].get_number();
        return true;
    }
    return false;
}

bool StratumClient::DoAuthorize() {
    std::string req = R"({"id":2,"method":"mining.authorize","params":[")" +
                      m_user + R"(",")" + m_pass + R"("]})";
    if (!SendJson(req)) return false;

    for (int attempt = 0; attempt < 10; ++attempt) {
        std::string line = RecvLine(10);
        if (line.empty()) continue;

        auto j = json::parse(line);
        if (!j.is_object()) continue;

        if (j.contains("method")) {
            std::string method = j["method"].get_string();
            if (method == "mining.set_difficulty" && m_cbs.onDifficulty) {
                auto &params = j["params"];
                if (params.is_array() && params.size() > 0) {
                    m_cbs.onDifficulty(params[0].get_number());
                }
            } else if (method == "mining.notify" && m_cbs.onJob) {
                auto &params = j["params"];
                if (params.is_array() && params.size() >= 9) {
                    StratumJob job;
                    job.jobId = params[0].get_string();
                    job.prevHash = params[1].get_string();
                    job.coinbase1 = params[2].get_string();
                    job.coinbase2 = params[3].get_string();
                    auto &branches = params[4];
                    if (branches.is_array()) {
                        for (size_t i = 0; i < branches.size(); ++i) {
                            job.merkleBranches.push_back(
                                branches[i].get_string());
                        }
                    }
                    job.version = params[5].get_string();
                    job.nbits = params[6].get_string();
                    job.ntime = params[7].get_string();
                    job.cleanJobs = params[8].get_bool();
                    m_cbs.onJob(job);
                }
            }
            continue;
        }

        if (j.contains("id") && j["id"].get_number() == 2) {
            auto &result = j["result"];
            return result.is_bool() && result.get_bool();
        }
    }
    return false;
}

void StratumClient::SubmitShare(const std::string &jobId,
                                const std::string &extranonce2,
                                const std::string &ntime,
                                const std::string &nonce) {
    int64_t id;
    {
        std::lock_guard<std::mutex> lock(m_writeMutex);
        id = m_nextId++;
    }
    std::string req = R"({"id":)" + std::to_string(id) +
                      R"(,"method":"mining.submit","params":[")" + m_user +
                      R"(",")" + jobId + R"(",")" + extranonce2 + R"(",")" +
                      ntime + R"(",")" + nonce + R"("]})";
    SendJson(req);
}

void StratumClient::ReadLoop() {
    std::string buffer;
    char buf[4096];

    while (!m_interrupt.load() && m_sockfd >= 0) {
        struct pollfd pfd{m_sockfd, POLLIN, 0};
        int rv = poll(&pfd, 1, 1000);
        if (rv <= 0) continue;

        ssize_t n = recv(m_sockfd, buf, sizeof(buf), 0);
        if (n <= 0) break;

        buffer.append(buf, n);
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (line.empty() || line[0] == '\r') continue;
            if (!line.empty() && line.back() == '\r') line.pop_back();

            auto j = json::parse(line);
            if (!j.is_object()) continue;

            if (j.contains("method")) {
                std::string method = j["method"].get_string();
                if (method == "mining.notify" && m_cbs.onJob) {
                    auto &params = j["params"];
                    if (params.is_array() && params.size() >= 9) {
                        StratumJob job;
                        job.jobId = params[0].get_string();
                        job.prevHash = params[1].get_string();
                        job.coinbase1 = params[2].get_string();
                        job.coinbase2 = params[3].get_string();
                        auto &branches = params[4];
                        if (branches.is_array()) {
                            for (size_t i = 0; i < branches.size(); ++i) {
                                job.merkleBranches.push_back(
                                    branches[i].get_string());
                            }
                        }
                        job.version = params[5].get_string();
                        job.nbits = params[6].get_string();
                        job.ntime = params[7].get_string();
                        job.cleanJobs = params[8].get_bool();
                        m_cbs.onJob(job);
                    }
                } else if (method == "mining.set_difficulty" &&
                           m_cbs.onDifficulty) {
                    auto &params = j["params"];
                    if (params.is_array() && params.size() > 0) {
                        m_cbs.onDifficulty(params[0].get_number());
                    }
                }
            } else if (j.contains("id") && m_cbs.onSubmitResult) {
                int64_t id = (int64_t)j["id"].get_number();
                bool accepted = false;
                std::string error;
                if (j["result"].is_bool()) {
                    accepted = j["result"].get_bool();
                }
                if (j["error"].is_array() && j["error"].size() >= 2) {
                    error = j["error"][1].get_string();
                }
                m_cbs.onSubmitResult(id, accepted, error);
            }
        }
    }

    m_connected.store(false);
    if (m_cbs.onStateChange) {
        m_cbs.onStateChange(false, "disconnected");
    }
}
