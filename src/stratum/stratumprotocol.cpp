// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumprotocol.h>

#include <util/translation.h>

namespace stratum {

void StratumLineBuffer::Append(const char *data, size_t len) {
    if (m_overflow) {
        return;
    }
    m_buffer.append(data, len);
    if (m_buffer.size() > MAX_LINE_LENGTH * 2) {
        m_overflow = true;
        m_buffer.clear();
    }
}

std::vector<std::string> StratumLineBuffer::ExtractLines() {
    std::vector<std::string> lines;
    size_t pos = 0;
    while (true) {
        size_t newline = m_buffer.find('\n', pos);
        if (newline == std::string::npos) {
            break;
        }
        std::string line = m_buffer.substr(pos, newline - pos);
        // Strip trailing \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // Only yield non-empty lines
        if (!line.empty()) {
            if (line.size() > MAX_LINE_LENGTH) {
                m_overflow = true;
                m_buffer.clear();
                return lines;
            }
            lines.push_back(std::move(line));
        }
        pos = newline + 1;
    }
    if (pos > 0) {
        m_buffer.erase(0, pos);
    }
    return lines;
}

void StratumLineBuffer::Clear() {
    m_buffer.clear();
    m_overflow = false;
}

util::Result<StratumMessage> ParseStratumLine(const std::string &line) {
    UniValue val;
    if (!val.read(line)) {
        return {{_("Invalid JSON")}};
    }
    if (!val.isObject()) {
        return {{_("Stratum message must be a JSON object")}};
    }

    bool hasId = val.exists("id") && !val["id"].isNull();
    bool hasMethod = val.exists("method");
    bool hasResult = val.exists("result");
    bool hasError = val.exists("error");

    if (hasMethod && hasId) {
        // Request: has id + method
        StratumRequest req;
        req.id = val["id"].getInt<int64_t>();
        req.method = val["method"].get_str();
        req.params = val.exists("params") ? val["params"] : UniValue(UniValue::VARR);
        return StratumMessage{req};
    }

    if (hasMethod && !hasId) {
        // Notification: has method, no id (or null id)
        StratumNotification notif;
        notif.method = val["method"].get_str();
        notif.params =
            val.exists("params") ? val["params"] : UniValue(UniValue::VARR);
        return StratumMessage{notif};
    }

    if (hasId && (hasResult || hasError)) {
        // Response: has id + result/error
        StratumResponse resp;
        resp.id = val["id"].getInt<int64_t>();
        resp.result =
            val.exists("result") ? val["result"] : UniValue(UniValue::VNULL);
        resp.error =
            val.exists("error") ? val["error"] : UniValue(UniValue::VNULL);
        return StratumMessage{resp};
    }

    return {{_("Cannot determine Stratum message type")}};
}

std::string SerializeNotify(const std::string &method, const UniValue &params) {
    UniValue msg(UniValue::VOBJ);
    msg.pushKV("id", UniValue(UniValue::VNULL));
    msg.pushKV("method", method);
    msg.pushKV("params", params);
    return msg.write() + "\n";
}

std::string SerializeResponse(int64_t id, const UniValue &result,
                              const UniValue &error) {
    UniValue msg(UniValue::VOBJ);
    msg.pushKV("id", id);
    msg.pushKV("result", result);
    msg.pushKV("error", error);
    return msg.write() + "\n";
}

std::string SerializeRequest(int64_t id, const std::string &method,
                             const UniValue &params) {
    UniValue msg(UniValue::VOBJ);
    msg.pushKV("id", id);
    msg.pushKV("method", method);
    msg.pushKV("params", params);
    return msg.write() + "\n";
}

} // namespace stratum
