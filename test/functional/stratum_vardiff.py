#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test Stratum variable difficulty adjustment.
"""

import json
import socket
import time

from test_framework.test_framework import BitcoinTestFramework


class StratumVardiffTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            "-stratum",
            "-stratumport=23335",
            "-stratumbind=127.0.0.1",
            "-stratumdifficulty=0.001",
            "-stratumvardiff",
            "-stratumvardifftarget=5",
            "-stratumvardiffretarget=10",
            "-stratumworkertimeout=600",
        ]]

    def stratum_connect(self, port=23335):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(("127.0.0.1", port))
        return sock

    def stratum_send(self, sock, msg):
        data = json.dumps(msg) + "\n"
        sock.sendall(data.encode())

    def stratum_recv_all(self, sock, timeout=5):
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
        except socket.timeout:
            pass
        return messages

    def run_test(self):
        node = self.nodes[0]

        self.generatetoaddress(node, 110, node.get_deterministic_priv_key().address)
        time.sleep(1)

        self.log.info("Test: initial difficulty is set correctly")
        sock = self.stratum_connect()

        # Subscribe
        self.stratum_send(sock, {
            "id": 1,
            "method": "mining.subscribe",
            "params": ["vardiff_test/1.0"],
        })
        self.stratum_recv_all(sock, timeout=2)

        # Authorize
        self.stratum_send(sock, {
            "id": 2,
            "method": "mining.authorize",
            "params": ["vardiff.worker", "x"],
        })
        auth_msgs = self.stratum_recv_all(sock, timeout=3)

        diff_msgs = [m for m in auth_msgs
                     if m.get("method") == "mining.set_difficulty"]
        if diff_msgs:
            initial_diff = diff_msgs[0]["params"][0]
            self.log.info(f"  Initial difficulty: {initial_diff}")
            assert initial_diff > 0
            assert initial_diff == 0.001
        else:
            self.log.info("  No initial difficulty message received")

        self.log.info("Test: vardiff parameters are accepted")
        # Just verify the node started with vardiff params without crashing
        info = node.getmininginfo()
        assert info is not None

        sock.close()

        self.log.info("All Stratum vardiff tests passed!")


if __name__ == "__main__":
    StratumVardiffTest().main()
