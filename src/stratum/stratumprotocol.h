// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMPROTOCOL_H
#define BITCOIN_STRATUM_STRATUMPROTOCOL_H

#include <cstddef>
#include <string>
#include <univalue.h>
#include <util/result.h>
#include <variant>
#include <vector>

namespace stratum {

static constexpr size_t MAX_LINE_LENGTH = 16384;

struct StratumRequest {
    int64_t id;
    std::string method;
    UniValue params;
};

struct StratumResponse {
    int64_t id;
    UniValue result;
    UniValue error;
};

struct StratumNotification {
    std::string method;
    UniValue params;
};

using StratumMessage =
    std::variant<StratumRequest, StratumResponse, StratumNotification>;

/**
 * Accumulates raw TCP bytes and extracts complete newline-delimited lines.
 * Handles partial reads, multiple messages per chunk, and CR/LF variations.
 */
class StratumLineBuffer {
public:
    void Append(const char *data, size_t len);
    std::vector<std::string> ExtractLines();
    void Clear();
    size_t Size() const { return m_buffer.size(); }
    bool OverflowDetected() const { return m_overflow; }

private:
    std::string m_buffer;
    bool m_overflow = false;
};

util::Result<StratumMessage> ParseStratumLine(const std::string &line);
std::string SerializeNotify(const std::string &method, const UniValue &params);
std::string SerializeResponse(int64_t id, const UniValue &result,
                              const UniValue &error);
std::string SerializeRequest(int64_t id, const std::string &method,
                             const UniValue &params);

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMPROTOCOL_H
