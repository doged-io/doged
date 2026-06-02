// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumstats.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumstats_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(stats_initial_zeroes) {
    StratumServerStats stats;
    BOOST_CHECK_EQUAL(stats.totalConnections, 0u);
    BOOST_CHECK_EQUAL(stats.activeWorkers, 0u);
    BOOST_CHECK_EQUAL(stats.totalSharesAccepted, 0u);
    BOOST_CHECK_EQUAL(stats.totalSharesRejected, 0u);
    BOOST_CHECK_EQUAL(stats.totalSharesStale, 0u);
    BOOST_CHECK_EQUAL(stats.blocksFound, 0u);
}

BOOST_AUTO_TEST_CASE(stats_json_format) {
    StratumServerStats stats;
    stats.startTime = 1000;
    stats.totalConnections = 42;
    stats.activeWorkers = 5;
    stats.totalSharesAccepted = 100;
    stats.totalSharesRejected = 3;
    stats.blocksFound = 1;
    stats.networkDifficulty = 12345.6;
    stats.chainHeight = 500;

    StratumWorker::Stats ws;
    ws.workerName = "miner.1";
    ws.currentDifficulty = 1.5;
    ws.sharesAccepted = 50;
    ws.sharesRejected = 1;
    ws.estimatedHashrate = 1000000;
    ws.state = StratumWorker::State::AUTHORIZED;
    stats.workers.push_back(ws);

    UniValue json = FormatStatsJson(stats);

    BOOST_CHECK(json.isObject());
    BOOST_CHECK(json.exists("uptime"));
    BOOST_CHECK(json.exists("totalConnections"));
    BOOST_CHECK(json.exists("activeWorkers"));
    BOOST_CHECK(json.exists("totalSharesAccepted"));
    BOOST_CHECK(json.exists("totalSharesRejected"));
    BOOST_CHECK(json.exists("blocksFound"));
    BOOST_CHECK(json.exists("networkDifficulty"));
    BOOST_CHECK(json.exists("chainHeight"));
    BOOST_CHECK(json.exists("workers"));

    BOOST_CHECK_EQUAL(json["totalConnections"].getInt<uint64_t>(), 42u);
    BOOST_CHECK_EQUAL(json["activeWorkers"].getInt<uint32_t>(), 5u);

    const UniValue &workers = json["workers"];
    BOOST_CHECK(workers.isArray());
    BOOST_CHECK_EQUAL(workers.size(), 1u);
    BOOST_CHECK_EQUAL(workers[0]["name"].get_str(), "miner.1");
    BOOST_CHECK_EQUAL(workers[0]["state"].get_str(), "authorized");
}

BOOST_AUTO_TEST_CASE(stats_empty_workers) {
    StratumServerStats stats;
    UniValue json = FormatStatsJson(stats);
    BOOST_CHECK(json["workers"].isArray());
    BOOST_CHECK_EQUAL(json["workers"].size(), 0u);
}

BOOST_AUTO_TEST_CASE(stats_worker_states) {
    StratumServerStats stats;

    StratumWorker::Stats w1;
    w1.state = StratumWorker::State::CONNECTED;
    w1.workerName = "w1";
    stats.workers.push_back(w1);

    StratumWorker::Stats w2;
    w2.state = StratumWorker::State::SUBSCRIBED;
    w2.workerName = "w2";
    stats.workers.push_back(w2);

    StratumWorker::Stats w3;
    w3.state = StratumWorker::State::MINING;
    w3.workerName = "w3";
    stats.workers.push_back(w3);

    UniValue json = FormatStatsJson(stats);
    const UniValue &workers = json["workers"];
    BOOST_CHECK_EQUAL(workers[0]["state"].get_str(), "connected");
    BOOST_CHECK_EQUAL(workers[1]["state"].get_str(), "subscribed");
    BOOST_CHECK_EQUAL(workers[2]["state"].get_str(), "mining");
}

BOOST_AUTO_TEST_SUITE_END()
