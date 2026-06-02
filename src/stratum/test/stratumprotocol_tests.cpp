// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumprotocol.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumprotocol_tests, BasicTestingSetup)

// --- StratumLineBuffer tests ---

BOOST_AUTO_TEST_CASE(linebuffer_empty) {
    StratumLineBuffer buf;
    auto lines = buf.ExtractLines();
    BOOST_CHECK(lines.empty());
    BOOST_CHECK_EQUAL(buf.Size(), 0u);
}

BOOST_AUTO_TEST_CASE(linebuffer_single_complete_line) {
    StratumLineBuffer buf;
    std::string data = "{\"id\":1}\n";
    buf.Append(data.c_str(), data.size());
    auto lines = buf.ExtractLines();
    BOOST_CHECK_EQUAL(lines.size(), 1u);
    BOOST_CHECK_EQUAL(lines[0], "{\"id\":1}");
    BOOST_CHECK_EQUAL(buf.Size(), 0u);
}

BOOST_AUTO_TEST_CASE(linebuffer_partial_then_complete) {
    StratumLineBuffer buf;
    std::string part1 = "{\"id\"";
    std::string part2 = ":1}\n";
    buf.Append(part1.c_str(), part1.size());
    auto lines1 = buf.ExtractLines();
    BOOST_CHECK(lines1.empty());

    buf.Append(part2.c_str(), part2.size());
    auto lines2 = buf.ExtractLines();
    BOOST_CHECK_EQUAL(lines2.size(), 1u);
    BOOST_CHECK_EQUAL(lines2[0], "{\"id\":1}");
}

BOOST_AUTO_TEST_CASE(linebuffer_multiple_lines_single_chunk) {
    StratumLineBuffer buf;
    std::string data = "aaa\nbbb\nccc\n";
    buf.Append(data.c_str(), data.size());
    auto lines = buf.ExtractLines();
    BOOST_CHECK_EQUAL(lines.size(), 3u);
    BOOST_CHECK_EQUAL(lines[0], "aaa");
    BOOST_CHECK_EQUAL(lines[1], "bbb");
    BOOST_CHECK_EQUAL(lines[2], "ccc");
}

BOOST_AUTO_TEST_CASE(linebuffer_no_trailing_newline) {
    StratumLineBuffer buf;
    std::string data = "partial";
    buf.Append(data.c_str(), data.size());
    auto lines = buf.ExtractLines();
    BOOST_CHECK(lines.empty());
    BOOST_CHECK_EQUAL(buf.Size(), 7u);
}

BOOST_AUTO_TEST_CASE(linebuffer_empty_lines_filtered) {
    StratumLineBuffer buf;
    std::string data = "\n\n\n";
    buf.Append(data.c_str(), data.size());
    auto lines = buf.ExtractLines();
    BOOST_CHECK(lines.empty());
}

BOOST_AUTO_TEST_CASE(linebuffer_crlf_handling) {
    StratumLineBuffer buf;
    std::string data = "hello\r\n";
    buf.Append(data.c_str(), data.size());
    auto lines = buf.ExtractLines();
    BOOST_CHECK_EQUAL(lines.size(), 1u);
    BOOST_CHECK_EQUAL(lines[0], "hello");
}

BOOST_AUTO_TEST_CASE(linebuffer_overflow_detection) {
    StratumLineBuffer buf;
    // Create a line that's too long
    std::string longLine(MAX_LINE_LENGTH + 100, 'x');
    longLine += "\n";
    buf.Append(longLine.c_str(), longLine.size());
    buf.ExtractLines();
    BOOST_CHECK(buf.OverflowDetected());
}

// --- ParseStratumLine tests ---

BOOST_AUTO_TEST_CASE(parse_subscribe_request) {
    std::string line =
        R"({"id":1,"method":"mining.subscribe","params":["test/1.0"]})";
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *req = std::get_if<StratumRequest>(&*result);
    BOOST_REQUIRE(req != nullptr);
    BOOST_CHECK_EQUAL(req->id, 1);
    BOOST_CHECK_EQUAL(req->method, "mining.subscribe");
    BOOST_CHECK(req->params.isArray());
    BOOST_CHECK_EQUAL(req->params.size(), 1u);
}

BOOST_AUTO_TEST_CASE(parse_authorize_request) {
    std::string line =
        R"({"id":2,"method":"mining.authorize","params":["user.worker","pass"]})";
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *req = std::get_if<StratumRequest>(&*result);
    BOOST_REQUIRE(req != nullptr);
    BOOST_CHECK_EQUAL(req->id, 2);
    BOOST_CHECK_EQUAL(req->method, "mining.authorize");
    BOOST_CHECK_EQUAL(req->params[0].get_str(), "user.worker");
    BOOST_CHECK_EQUAL(req->params[1].get_str(), "pass");
}

BOOST_AUTO_TEST_CASE(parse_submit_request) {
    std::string line =
        R"({"id":3,"method":"mining.submit","params":["worker","1a","00000000","5f3c2e1a","deadbeef"]})";
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *req = std::get_if<StratumRequest>(&*result);
    BOOST_REQUIRE(req != nullptr);
    BOOST_CHECK_EQUAL(req->id, 3);
    BOOST_CHECK_EQUAL(req->method, "mining.submit");
    BOOST_CHECK_EQUAL(req->params.size(), 5u);
}

BOOST_AUTO_TEST_CASE(parse_response_success) {
    std::string line = R"({"id":1,"result":true,"error":null})";
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *resp = std::get_if<StratumResponse>(&*result);
    BOOST_REQUIRE(resp != nullptr);
    BOOST_CHECK_EQUAL(resp->id, 1);
    BOOST_CHECK(resp->result.get_bool());
    BOOST_CHECK(resp->error.isNull());
}

BOOST_AUTO_TEST_CASE(parse_response_error) {
    std::string line =
        R"({"id":1,"result":null,"error":[21,"Job not found",null]})";
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *resp = std::get_if<StratumResponse>(&*result);
    BOOST_REQUIRE(resp != nullptr);
    BOOST_CHECK(resp->result.isNull());
    BOOST_CHECK(resp->error.isArray());
}

BOOST_AUTO_TEST_CASE(parse_notification) {
    std::string line =
        R"({"method":"mining.set_difficulty","params":[1.5]})";
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *notif = std::get_if<StratumNotification>(&*result);
    BOOST_REQUIRE(notif != nullptr);
    BOOST_CHECK_EQUAL(notif->method, "mining.set_difficulty");
}

BOOST_AUTO_TEST_CASE(parse_invalid_json) {
    auto result = ParseStratumLine("not json at all");
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(parse_missing_method) {
    auto result = ParseStratumLine(R"({"id":1})");
    BOOST_CHECK(!result.has_value());
}

// --- Serialization roundtrip tests ---

BOOST_AUTO_TEST_CASE(serialize_notify_roundtrip) {
    UniValue params(UniValue::VARR);
    params.push_back(1.5);
    std::string serialized = SerializeNotify("mining.set_difficulty", params);

    // Must end with newline
    BOOST_CHECK_EQUAL(serialized.back(), '\n');

    // Parse back
    std::string line = serialized.substr(0, serialized.size() - 1);
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *notif = std::get_if<StratumNotification>(&*result);
    BOOST_REQUIRE(notif != nullptr);
    BOOST_CHECK_EQUAL(notif->method, "mining.set_difficulty");
}

BOOST_AUTO_TEST_CASE(serialize_response_roundtrip) {
    std::string serialized =
        SerializeResponse(42, UniValue(true), UniValue(UniValue::VNULL));
    BOOST_CHECK_EQUAL(serialized.back(), '\n');

    std::string line = serialized.substr(0, serialized.size() - 1);
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *resp = std::get_if<StratumResponse>(&*result);
    BOOST_REQUIRE(resp != nullptr);
    BOOST_CHECK_EQUAL(resp->id, 42);
    BOOST_CHECK(resp->result.get_bool());
}

BOOST_AUTO_TEST_CASE(serialize_request_roundtrip) {
    UniValue params(UniValue::VARR);
    params.push_back("test");
    std::string serialized = SerializeRequest(7, "mining.authorize", params);
    BOOST_CHECK_EQUAL(serialized.back(), '\n');

    std::string line = serialized.substr(0, serialized.size() - 1);
    auto result = ParseStratumLine(line);
    BOOST_CHECK(result.has_value());

    auto *req = std::get_if<StratumRequest>(&*result);
    BOOST_REQUIRE(req != nullptr);
    BOOST_CHECK_EQUAL(req->id, 7);
    BOOST_CHECK_EQUAL(req->method, "mining.authorize");
}

BOOST_AUTO_TEST_SUITE_END()
