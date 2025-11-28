// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumproxy.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumproxy_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(upstream_pool_defaults) {
    UpstreamPool pool;
    BOOST_CHECK_EQUAL(pool.port, 3333);
    BOOST_CHECK_EQUAL(pool.password, "x");
    BOOST_CHECK_EQUAL(pool.priority, 0);
    BOOST_CHECK(pool.host.empty());
    BOOST_CHECK(pool.username.empty());
}

BOOST_AUTO_TEST_CASE(proxy_health_defaults) {
    ProxyHealth h;
    BOOST_CHECK(!h.connected);
    BOOST_CHECK(!h.authorized);
    BOOST_CHECK_EQUAL(h.lastNotifyTime, 0);
    BOOST_CHECK_EQUAL(h.lastErrorTime, 0);
    BOOST_CHECK_EQUAL(h.consecutiveErrors, 0);
    BOOST_CHECK(h.lastError.empty());
}

BOOST_AUTO_TEST_CASE(proxy_not_connected_initially) {
    UpstreamPool pool;
    pool.host = "127.0.0.1";
    pool.port = 19999;
    pool.username = "testworker";

    ProxyCallbacks cbs;
    StratumProxyConn conn(pool, cbs);

    BOOST_CHECK(!conn.IsConnected());
    BOOST_CHECK(!conn.IsHealthy());
    BOOST_CHECK_EQUAL(conn.GetLabel(), "127.0.0.1:19999");
    BOOST_CHECK(conn.GetUpstreamExtranonce1().empty());
    BOOST_CHECK_EQUAL(conn.GetUpstreamExtranonce2Size(), 4);
}

BOOST_AUTO_TEST_CASE(proxy_health_unhealthy_without_connection) {
    UpstreamPool pool;
    pool.host = "192.168.255.255";
    pool.port = 1;
    pool.username = "test";

    ProxyCallbacks cbs;
    StratumProxyConn conn(pool, cbs);

    ProxyHealth h = conn.GetHealth();
    BOOST_CHECK(!h.connected);
    BOOST_CHECK(!h.authorized);
    BOOST_CHECK(!conn.IsHealthy());
}

BOOST_AUTO_TEST_CASE(proxy_connect_failure_bad_host) {
    UpstreamPool pool;
    pool.host = "this.host.does.not.exist.invalid";
    pool.port = 3333;
    pool.username = "test";

    bool stateChangeCalled = false;
    ProxyCallbacks cbs;
    cbs.onStateChange = [&](bool connected, const std::string &) {
        if (!connected) {
            stateChangeCalled = true;
        }
    };

    StratumProxyConn conn(pool, cbs);
    bool result = conn.Connect();
    BOOST_CHECK(!result);
    BOOST_CHECK(!conn.IsConnected());

    ProxyHealth h = conn.GetHealth();
    BOOST_CHECK(h.consecutiveErrors > 0);
    BOOST_CHECK(!h.lastError.empty());
}

BOOST_AUTO_TEST_CASE(proxy_connect_failure_refused) {
    UpstreamPool pool;
    pool.host = "127.0.0.1";
    pool.port = 1; // Almost certainly nothing listening
    pool.username = "test";

    ProxyCallbacks cbs;
    StratumProxyConn conn(pool, cbs);
    bool result = conn.Connect();
    BOOST_CHECK(!result);
    BOOST_CHECK(!conn.IsConnected());
}

BOOST_AUTO_TEST_CASE(proxy_disconnect_noop_when_not_connected) {
    UpstreamPool pool;
    pool.host = "127.0.0.1";
    pool.port = 19999;
    pool.username = "test";

    ProxyCallbacks cbs;
    StratumProxyConn conn(pool, cbs);
    conn.Disconnect(); // should not crash
    BOOST_CHECK(!conn.IsConnected());
}

BOOST_AUTO_TEST_CASE(proxy_label_format) {
    UpstreamPool pool;
    pool.host = "pool.example.com";
    pool.port = 4444;
    pool.username = "miner1";

    ProxyCallbacks cbs;
    StratumProxyConn conn(pool, cbs);
    BOOST_CHECK_EQUAL(conn.GetLabel(), "pool.example.com:4444");
}

BOOST_AUTO_TEST_SUITE_END()
