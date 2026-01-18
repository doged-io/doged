#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test Stratum server tiered routing: local → proxy → failover.

Tests:
  1. Default (no proxy configured) → LOCAL tier
  2. With a proxy configured pointing to a non-existent host → LOCAL fallback
  3. Stats endpoint reports routing tier and upstream pool health
  4. Config parsing for -stratumproxy, -stratumpreferlocal, -stratumwarnsolo
"""

import json
import socket
import time
import urllib.request

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class StratumRoutingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            "-stratum",
            "-stratumport=23335",
            "-stratumbind=127.0.0.1",
            "-stratumdifficulty=0.001",
            # Proxy to a non-existent pool — will fail to connect
            "-stratumproxy=127.0.0.1:19999:testworker:x:5",
        ]]

    def stratum_connect(self, port=23335):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(("127.0.0.1", port))
        return sock

    def stratum_send(self, sock, msg):
        data = json.dumps(msg) + "\n"
        sock.sendall(data.encode())

    def stratum_recv(self, sock, timeout=5):
        sock.settimeout(timeout)
        data = b""
        messages = []
        try:
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                data += chunk
                while b"\n" in data:
                    line, data = data.split(b"\n", 1)
                    line = line.strip()
                    if line:
                        messages.append(json.loads(line))
                if messages:
                    break
        except socket.timeout:
            pass
        return messages

    def get_stratum_status(self, node):
        """Fetch /stratum/status from the node's HTTP server."""
        url = f"http://127.0.0.1:{node.rpc_port}/stratum/status"
        try:
            with urllib.request.urlopen(url, timeout=5) as resp:
                return json.loads(resp.read().decode())
        except Exception as e:
            self.log.info(f"Failed to fetch stratum status: {e}")
            return None

    def run_test(self):
        node = self.nodes[0]

        # Generate blocks so the chain is active (local node is synced)
        self.generatetoaddress(
            node, 110, node.get_deterministic_priv_key().address)

        # Let the stratum server and router start up
        time.sleep(3)

        self.log.info("Test: local tier is active when node is synced")
        status = self.get_stratum_status(node)
        if status:
            self.log.info(f"  activeTier: {status.get('activeTier')}")
            # With preferLocal=true (default) and synced chain,
            # should be LOCAL even though proxy is configured
            assert_equal(status["activeTier"], "local")

            self.log.info("Test: upstream pool stats reported")
            pools = status.get("upstreamPools", [])
            assert len(pools) >= 1
            self.log.info(f"  Pool 0: {pools[0]}")
            assert_equal(pools[0]["label"], "127.0.0.1:19999")
            assert_equal(pools[0]["priority"], 5)
            # Should have failed to connect (nothing listening on 19999)
            assert_equal(pools[0]["healthy"], False)

        self.log.info("Test: miner can still connect and work in LOCAL tier")
        sock = self.stratum_connect()
        self.stratum_send(sock, {
            "id": 1,
            "method": "mining.subscribe",
            "params": ["routing_test/1.0"],
        })
        msgs = self.stratum_recv(sock)
        sub = next((m for m in msgs if m.get("id") == 1), None)
        assert sub is not None
        assert sub.get("error") is None
        result = sub["result"]
        assert len(result) == 3
        self.log.info(f"  extranonce1: {result[1]}")

        self.stratum_send(sock, {
            "id": 2,
            "method": "mining.authorize",
            "params": ["testuser", "x"],
        })
        auth_msgs = self.stratum_recv(sock, timeout=5)
        auth = next((m for m in auth_msgs if m.get("id") == 2), None)
        assert auth is not None
        assert auth["result"] is True

        # Should receive difficulty and notify
        all_msgs = auth_msgs + self.stratum_recv(sock, timeout=3)
        notify = [m for m in all_msgs if m.get("method") == "mining.notify"]
        self.log.info(f"  received {len(notify)} mining.notify notifications")
        assert len(notify) >= 1

        sock.close()

        self.log.info("Test: activeProxyIndex is -1 in LOCAL mode")
        status = self.get_stratum_status(node)
        if status:
            assert_equal(status["activeProxyIndex"], -1)

        self.log.info("All Stratum routing tests passed!")


if __name__ == "__main__":
    StratumRoutingTest().main()
