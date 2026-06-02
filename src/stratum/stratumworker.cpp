// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumworker.h>

#include <util/strencodings.h>
#include <util/time.h>
#include <util/translation.h>

#include <algorithm>
#include <cmath>

namespace stratum {

StratumWorker::StratumWorker(uint32_t sessionId,
                             const std::string &extranonce1,
                             double initialDifficulty)
    : m_state(State::CONNECTED), m_sessionId(sessionId),
      m_extranonce1(extranonce1), m_difficulty(initialDifficulty) {
    m_connectTime = GetTime();
    m_lastActivityTime = m_connectTime;
    m_lastRetargetTime = m_connectTime;
}

util::Result<UniValue> StratumWorker::HandleSubscribe(const UniValue &params) {
    if (m_state != State::CONNECTED) {
        return {{_("Already subscribed")}};
    }

    m_state = State::SUBSCRIBED;

    // Build subscription response:
    // [[["mining.set_difficulty","sub1"],["mining.notify","sub2"]], extranonce1,
    // extranonce2_size]
    UniValue subscriptions(UniValue::VARR);

    UniValue diffSub(UniValue::VARR);
    diffSub.push_back("mining.set_difficulty");
    diffSub.push_back(HexStr(std::vector<uint8_t>(m_extranonce1.begin(),
                                                   m_extranonce1.end()))
                          .substr(0, 8));
    subscriptions.push_back(diffSub);

    UniValue notifySub(UniValue::VARR);
    notifySub.push_back("mining.notify");
    notifySub.push_back(HexStr(std::vector<uint8_t>(m_extranonce1.begin(),
                                                     m_extranonce1.end()))
                            .substr(0, 8));
    subscriptions.push_back(notifySub);

    UniValue result(UniValue::VARR);
    result.push_back(subscriptions);
    result.push_back(m_extranonce1);
    result.push_back(static_cast<int>(EXTRANONCE2_SIZE));

    return result;
}

util::Result<UniValue> StratumWorker::HandleAuthorize(const UniValue &params) {
    if (m_state == State::CONNECTED) {
        return {{_("Must subscribe before authorizing")}};
    }
    if (m_state == State::AUTHORIZED || m_state == State::MINING) {
        return {{_("Already authorized")}};
    }

    if (!params.isArray() || params.size() < 1) {
        return {{_("mining.authorize requires at least a username")}};
    }

    m_workerName = params[0].get_str();
    m_state = State::AUTHORIZED;

    return UniValue(true);
}

void StratumWorker::RecordShareAccepted(double difficulty) {
    m_sharesAccepted++;
    int64_t now = GetTime();
    m_lastActivityTime = now;
    m_shareTimestamps.push_back(now);

    // Keep only last 100 timestamps for vardiff
    if (m_shareTimestamps.size() > 100) {
        m_shareTimestamps.erase(m_shareTimestamps.begin());
    }
}

void StratumWorker::RecordShareRejected() {
    m_sharesRejected++;
    m_lastActivityTime = GetTime();
}

void StratumWorker::RecordShareStale() {
    m_sharesStale++;
    m_lastActivityTime = GetTime();
}

void StratumWorker::SetDifficulty(double diff) {
    if (diff < MIN_DIFFICULTY) {
        diff = MIN_DIFFICULTY;
    }
    if (diff > MAX_DIFFICULTY) {
        diff = MAX_DIFFICULTY;
    }
    m_difficulty = diff;
}

bool StratumWorker::ShouldRetargetDifficulty(const StratumConfig &config,
                                              int64_t now) const {
    if (!config.useVarDiff) {
        return false;
    }
    if (m_shareTimestamps.size() < 3) {
        return false;
    }
    return (now - m_lastRetargetTime) >= (int64_t)config.varDiffRetargetTime;
}

double StratumWorker::CalcNewDifficulty(const StratumConfig &config,
                                        int64_t now) const {
    if (m_shareTimestamps.size() < 2) {
        return m_difficulty;
    }

    // Average time between shares over the retarget window
    int64_t windowStart = m_shareTimestamps.front();
    int64_t windowEnd = m_shareTimestamps.back();
    double elapsed = static_cast<double>(windowEnd - windowStart);
    if (elapsed <= 0) {
        return m_difficulty;
    }

    double avgShareTime = elapsed / (m_shareTimestamps.size() - 1);
    double targetTime = config.varDiffTargetTime;

    // Ratio: if shares come too fast, ratio > 1 → increase difficulty
    double ratio = targetTime / avgShareTime;

    // Dampen changes to avoid oscillation
    double newDiff = m_difficulty * ratio;

    // Enforce bounds
    if (newDiff < MIN_DIFFICULTY) {
        newDiff = MIN_DIFFICULTY;
    }
    if (newDiff > MAX_DIFFICULTY) {
        newDiff = MAX_DIFFICULTY;
    }

    return newDiff;
}

StratumWorker::Stats StratumWorker::GetStats() const {
    Stats s;
    s.sharesAccepted = m_sharesAccepted;
    s.sharesRejected = m_sharesRejected;
    s.sharesStale = m_sharesStale;
    s.currentDifficulty = m_difficulty;
    s.lastShareTime = m_shareTimestamps.empty() ? 0 : m_shareTimestamps.back();
    s.workerName = m_workerName;
    s.state = m_state;

    // Estimate hashrate from recent share rate: H/s ≈ (diff × 2^32) / time
    if (m_sharesAccepted > 0 && m_shareTimestamps.size() >= 2) {
        int64_t span = m_shareTimestamps.back() - m_shareTimestamps.front();
        if (span > 0) {
            double sharesPerSec =
                static_cast<double>(m_shareTimestamps.size() - 1) / span;
            s.estimatedHashrate =
                sharesPerSec * m_difficulty * 4294967296.0; // 2^32
        }
    }

    return s;
}

bool StratumWorker::IsTimedOut(int timeoutSec, int64_t now) const {
    return (now - m_lastActivityTime) > timeoutSec;
}

void StratumWorker::Touch(int64_t now) {
    m_lastActivityTime = now;
}

// --- ExtranonceMgr ---

std::string ExtranonceMgr::Next() {
    uint32_t val = m_counter.fetch_add(1);
    // 4 bytes → 8 hex chars
    uint8_t bytes[4];
    bytes[0] = (val >> 24) & 0xff;
    bytes[1] = (val >> 16) & 0xff;
    bytes[2] = (val >> 8) & 0xff;
    bytes[3] = val & 0xff;
    return HexStr(Span<const uint8_t>(bytes, 4));
}

} // namespace stratum
