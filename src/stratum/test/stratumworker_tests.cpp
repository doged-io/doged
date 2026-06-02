// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumworker.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <set>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumworker_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(initial_state_connected) {
    StratumWorker worker(1, "00000001", 1.0);
    BOOST_CHECK(worker.GetState() == StratumWorker::State::CONNECTED);
    BOOST_CHECK_EQUAL(worker.GetSessionId(), 1u);
    BOOST_CHECK_EQUAL(worker.GetExtranonce1(), "00000001");
}

BOOST_AUTO_TEST_CASE(subscribe_transitions_to_subscribed) {
    StratumWorker worker(1, "abcd1234", 1.0);
    UniValue params(UniValue::VARR);
    params.push_back("test/1.0");

    auto result = worker.HandleSubscribe(params);
    BOOST_CHECK(result.has_value());
    BOOST_CHECK(worker.GetState() == StratumWorker::State::SUBSCRIBED);
}

BOOST_AUTO_TEST_CASE(subscribe_returns_extranonce) {
    StratumWorker worker(1, "abcd1234", 1.0);
    UniValue params(UniValue::VARR);
    auto result = worker.HandleSubscribe(params);
    BOOST_REQUIRE(result.has_value());

    // Result should be [subscriptions, extranonce1, extranonce2_size]
    BOOST_CHECK(result->isArray());
    BOOST_CHECK_EQUAL(result->size(), 3u);
    BOOST_CHECK_EQUAL((*result)[1].get_str(), "abcd1234");
    BOOST_CHECK_EQUAL((*result)[2].getInt<int>(),
                      (int)StratumWorker::EXTRANONCE2_SIZE);
}

BOOST_AUTO_TEST_CASE(authorize_requires_subscribed) {
    StratumWorker worker(1, "00000001", 1.0);
    UniValue params(UniValue::VARR);
    params.push_back("worker.1");
    params.push_back("pass");

    auto result = worker.HandleAuthorize(params);
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(authorize_sets_worker_name) {
    StratumWorker worker(1, "00000001", 1.0);

    UniValue subParams(UniValue::VARR);
    worker.HandleSubscribe(subParams);

    UniValue authParams(UniValue::VARR);
    authParams.push_back("worker.1");
    authParams.push_back("x");
    auto result = worker.HandleAuthorize(authParams);

    BOOST_CHECK(result.has_value());
    BOOST_CHECK_EQUAL(worker.GetWorkerName(), "worker.1");
}

BOOST_AUTO_TEST_CASE(authorize_transitions_to_authorized) {
    StratumWorker worker(1, "00000001", 1.0);

    UniValue subParams(UniValue::VARR);
    worker.HandleSubscribe(subParams);

    UniValue authParams(UniValue::VARR);
    authParams.push_back("user");
    worker.HandleAuthorize(authParams);

    BOOST_CHECK(worker.GetState() == StratumWorker::State::AUTHORIZED);
}

BOOST_AUTO_TEST_CASE(double_subscribe_rejected) {
    StratumWorker worker(1, "00000001", 1.0);
    UniValue params(UniValue::VARR);

    auto r1 = worker.HandleSubscribe(params);
    BOOST_CHECK(r1.has_value());

    auto r2 = worker.HandleSubscribe(params);
    BOOST_CHECK(!r2.has_value());
}

BOOST_AUTO_TEST_CASE(share_accepted_updates_stats) {
    StratumWorker worker(1, "00000001", 1.0);
    worker.RecordShareAccepted(1.0);
    auto stats = worker.GetStats();
    BOOST_CHECK_EQUAL(stats.sharesAccepted, 1u);
    BOOST_CHECK_EQUAL(stats.sharesRejected, 0u);
}

BOOST_AUTO_TEST_CASE(share_rejected_updates_stats) {
    StratumWorker worker(1, "00000001", 1.0);
    worker.RecordShareRejected();
    auto stats = worker.GetStats();
    BOOST_CHECK_EQUAL(stats.sharesRejected, 1u);
}

BOOST_AUTO_TEST_CASE(share_stale_updates_stats) {
    StratumWorker worker(1, "00000001", 1.0);
    worker.RecordShareStale();
    auto stats = worker.GetStats();
    BOOST_CHECK_EQUAL(stats.sharesStale, 1u);
}

BOOST_AUTO_TEST_CASE(timeout_detection) {
    StratumWorker worker(1, "00000001", 1.0);
    int64_t now = GetTime();
    worker.Touch(now - 400);
    BOOST_CHECK(worker.IsTimedOut(300, now));
}

BOOST_AUTO_TEST_CASE(timeout_not_reached) {
    StratumWorker worker(1, "00000001", 1.0);
    int64_t now = GetTime();
    worker.Touch(now);
    BOOST_CHECK(!worker.IsTimedOut(300, now));
}

BOOST_AUTO_TEST_CASE(difficulty_bounded) {
    StratumWorker worker(1, "00000001", 1.0);
    worker.SetDifficulty(0.0000001);
    BOOST_CHECK(worker.GetCurrentDifficulty() >= MIN_DIFFICULTY);

    worker.SetDifficulty(1e15);
    BOOST_CHECK(worker.GetCurrentDifficulty() <= MAX_DIFFICULTY);
}

BOOST_AUTO_TEST_CASE(vardiff_initial_no_retarget) {
    StratumWorker worker(1, "00000001", 1.0);
    StratumConfig config;
    config.useVarDiff = true;
    config.varDiffRetargetTime = 90;

    // No shares yet
    BOOST_CHECK(!worker.ShouldRetargetDifficulty(config, GetTime() + 200));
}

// --- ExtranonceMgr tests ---

BOOST_AUTO_TEST_CASE(extranonce_mgr_unique) {
    ExtranonceMgr mgr;
    std::set<std::string> seen;
    for (int i = 0; i < 1000; i++) {
        std::string en = mgr.Next();
        BOOST_CHECK(seen.insert(en).second);
    }
    BOOST_CHECK_EQUAL(seen.size(), 1000u);
}

BOOST_AUTO_TEST_CASE(extranonce_mgr_format) {
    ExtranonceMgr mgr;
    std::string en = mgr.Next();
    BOOST_CHECK_EQUAL(en.size(), 8u);
    // Must be valid hex
    for (char c : en) {
        BOOST_CHECK(
            (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

BOOST_AUTO_TEST_CASE(extranonce2_size_constant) {
    BOOST_CHECK_EQUAL(StratumWorker::EXTRANONCE2_SIZE, 4u);
}

BOOST_AUTO_TEST_SUITE_END()
