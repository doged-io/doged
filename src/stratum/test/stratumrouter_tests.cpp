// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumrouter.h>

#include <stratum/stratumconfig.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumrouter_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(tier_name_strings) {
    BOOST_CHECK_EQUAL(TierName(RoutingTier::NONE), "none");
    BOOST_CHECK_EQUAL(TierName(RoutingTier::LOCAL), "local");
    BOOST_CHECK_EQUAL(TierName(RoutingTier::PROXY), "proxy");
}

BOOST_AUTO_TEST_CASE(config_proxy_parsing_single) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum",
                          "-stratumproxy=pool.example.com:3333:worker1"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result->upstreamPools.size(), 1);
    BOOST_CHECK_EQUAL(result->upstreamPools[0].host, "pool.example.com");
    BOOST_CHECK_EQUAL(result->upstreamPools[0].port, 3333);
    BOOST_CHECK_EQUAL(result->upstreamPools[0].username, "worker1");
    BOOST_CHECK_EQUAL(result->upstreamPools[0].password, "x");
    BOOST_CHECK_EQUAL(result->upstreamPools[0].priority, 0);
}

BOOST_AUTO_TEST_CASE(config_proxy_parsing_with_password_and_priority) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {
        "test", "-stratum",
        "-stratumproxy=pool.example.com:4444:worker1:mypass:5"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result->upstreamPools.size(), 1);
    BOOST_CHECK_EQUAL(result->upstreamPools[0].port, 4444);
    BOOST_CHECK_EQUAL(result->upstreamPools[0].password, "mypass");
    BOOST_CHECK_EQUAL(result->upstreamPools[0].priority, 5);
}

BOOST_AUTO_TEST_CASE(config_proxy_parsing_multiple) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {
        "test", "-stratum",
        "-stratumproxy=pool1.com:3333:w1",
        "-stratumproxy=pool2.com:4444:w2:pass2:10"};
    std::string error;
    args.ParseParameters(4, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result->upstreamPools.size(), 2);
    BOOST_CHECK_EQUAL(result->upstreamPools[0].host, "pool1.com");
    BOOST_CHECK_EQUAL(result->upstreamPools[0].priority, 0);
    BOOST_CHECK_EQUAL(result->upstreamPools[1].host, "pool2.com");
    BOOST_CHECK_EQUAL(result->upstreamPools[1].priority, 10);
}

BOOST_AUTO_TEST_CASE(config_proxy_parsing_invalid_format) {
    ArgsManager args;
    RegisterStratumArgs(args);

    // Only host:port — missing username
    const char *argv[] = {"test", "-stratum",
                          "-stratumproxy=pool.com:3333"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(config_prefer_local_default) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum"};
    std::string error;
    args.ParseParameters(2, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(result->preferLocal);
    BOOST_CHECK(result->warnSoloMining);
}

BOOST_AUTO_TEST_CASE(config_prefer_local_disabled) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum", "-stratumpreferlocal=0"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(!result->preferLocal);
}

BOOST_AUTO_TEST_CASE(config_warn_solo_disabled) {
    ArgsManager args;
    RegisterStratumArgs(args);

    const char *argv[] = {"test", "-stratum", "-stratumwarnsolo=0"};
    std::string error;
    args.ParseParameters(3, argv, error);

    auto result = ParseStratumConfig(args);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(!result->warnSoloMining);
}

BOOST_AUTO_TEST_SUITE_END()
