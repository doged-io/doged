// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMWORKER_H
#define BITCOIN_STRATUM_STRATUMWORKER_H

#include <stratum/stratumconfig.h>
#include <univalue.h>
#include <util/result.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace stratum {

class StratumWorker {
public:
    enum class State { CONNECTED, SUBSCRIBED, AUTHORIZED, MINING };

    struct Stats {
        uint64_t sharesAccepted = 0;
        uint64_t sharesRejected = 0;
        uint64_t sharesStale = 0;
        double currentDifficulty = 0;
        int64_t lastShareTime = 0;
        double estimatedHashrate = 0;
        std::string workerName;
        State state = State::CONNECTED;
    };

    StratumWorker(uint32_t sessionId, const std::string &extranonce1,
                  double initialDifficulty);

    State GetState() const { return m_state; }
    uint32_t GetSessionId() const { return m_sessionId; }
    const std::string &GetExtranonce1() const { return m_extranonce1; }
    const std::string &GetWorkerName() const { return m_workerName; }

    /**
     * Handle mining.subscribe — transitions CONNECTED → SUBSCRIBED.
     * Returns [[[subscription_details], extranonce1, extranonce2_size]].
     */
    util::Result<UniValue> HandleSubscribe(const UniValue &params);

    /**
     * Handle mining.authorize — transitions SUBSCRIBED → AUTHORIZED.
     * Returns true on success.
     */
    util::Result<UniValue> HandleAuthorize(const UniValue &params);

    void RecordShareAccepted(double difficulty);
    void RecordShareRejected();
    void RecordShareStale();

    double GetCurrentDifficulty() const { return m_difficulty; }
    void SetDifficulty(double diff);

    bool ShouldRetargetDifficulty(const StratumConfig &config,
                                  int64_t now) const;
    double CalcNewDifficulty(const StratumConfig &config, int64_t now) const;

    Stats GetStats() const;
    bool IsTimedOut(int timeoutSec, int64_t now) const;
    void Touch(int64_t now);

    static constexpr size_t EXTRANONCE2_SIZE = 4;

private:
    State m_state;
    uint32_t m_sessionId;
    std::string m_extranonce1;
    std::string m_workerName;
    double m_difficulty;
    uint64_t m_sharesAccepted = 0;
    uint64_t m_sharesRejected = 0;
    uint64_t m_sharesStale = 0;
    int64_t m_connectTime;
    int64_t m_lastActivityTime;
    int64_t m_lastRetargetTime;
    std::vector<int64_t> m_shareTimestamps;
};

/**
 * Generates unique extranonce1 values (4 bytes → 8 hex chars).
 * Thread-safe via atomic counter.
 */
class ExtranonceMgr {
public:
    std::string Next();
    void Reset() { m_counter.store(0); }

private:
    std::atomic<uint32_t> m_counter{0};
};

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMWORKER_H
