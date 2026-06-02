// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumconfig.h>

#include <common/args.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumconfig_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(parse_defaults) {
    ArgsManager args;
    RegisterStratumArgs(args);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(!result->enabled);
    BOOST_CHECK_EQUAL(result->port, DEFAULT_STRATUM_PORT);
    BOOST_CHECK_EQUAL(result->bind, "0.0.0.0");
    BOOST_CHECK_EQUAL(result->defaultDifficulty, DEFAULT_STRATUM_DIFFICULTY);
    BOOST_CHECK_EQUAL(result->useVarDiff, DEFAULT_STRATUM_VARDIFF);
    BOOST_CHECK_EQUAL(result->maxWorkers, DEFAULT_MAX_WORKERS);
    BOOST_CHECK_EQUAL(result->workerTimeoutSec, DEFAULT_WORKER_TIMEOUT_SEC);
}

BOOST_AUTO_TEST_CASE(parse_enabled) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum"};
    std::string error;
    args.ParseParameters(2, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(result->enabled);
}

BOOST_AUTO_TEST_CASE(parse_custom_port) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum", "-stratumport=9999"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result->port, 9999);
}

BOOST_AUTO_TEST_CASE(parse_invalid_port) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum", "-stratumport=99999"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(parse_bind_address) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum", "-stratumbind=127.0.0.1"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result->bind, "127.0.0.1");
}

BOOST_AUTO_TEST_CASE(parse_max_workers_zero) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum", "-stratummaxworkers=0"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(parse_worker_timeout_zero) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum", "-stratumworkertimeout=0"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
