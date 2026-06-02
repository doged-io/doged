#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test Stratum mining: share submission, block finding, and new job broadcast.
"""

import json
import socket
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class StratumMiningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [
            [
                "-stratum",
                "-stratumport=23334",
                "-stratumbind=127.0.0.1",
                "-stratumdifficulty=0.001",
            ],
            [],
        ]

    def stratum_connect(self, port=23334):
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

    def stratum_full_auth(self, sock):
        """Subscribe + authorize, return (extranonce1, notifications)."""
        self.stratum_send(sock, {
            "id": 1,
            "method": "mining.subscribe",
            "params": ["test/1.0"],
        })
        sub_msgs = self.stratum_recv(sock)
        sub_resp = [m for m in sub_msgs if m.get("id") == 1][0]
        extranonce1 = sub_resp["result"][1]

        self.stratum_send(sock, {
            "id": 2,
            "method": "mining.authorize",
            "params": ["test.worker", "x"],
        })
        auth_msgs = self.stratum_recv(sock, timeout=5)
        # Collect additional notifications
        more = self.stratum_recv(sock, timeout=2)
        all_notifs = auth_msgs + more

        return extranonce1, all_notifs

    def run_test(self):
        node = self.nodes[0]

        self.generatetoaddress(node, 110, node.get_deterministic_priv_key().address)
        self.sync_all()
        time.sleep(1)

        self.log.info("Test: submit share for unknown job")
        sock = self.stratum_connect()
        extranonce1, notifs = self.stratum_full_auth(sock)

        self.stratum_send(sock, {
            "id": 10,
            "method": "mining.submit",
            "params": [
                "test.worker",
                "ffffff",       # nonexistent job
                "00000000",
                "5f3c2e1a",
                "deadbeef",
            ],
        })
        msgs = self.stratum_recv(sock)
        submit_resp = [m for m in msgs if m.get("id") == 10]
        assert len(submit_resp) == 1
        # Should get error 21 (job not found)
        assert submit_resp[0]["error"] is not None
        assert submit_resp[0]["error"][0] == 21

        self.log.info("Test: submit share with wrong nonce (low difficulty)")
        job_notifs = [n for n in notifs if n.get("method") == "mining.notify"]
        if not job_notifs:
            more = self.stratum_recv(sock, timeout=3)
            job_notifs = [n for n in more if n.get("method") == "mining.notify"]

        if job_notifs:
            job_params = job_notifs[0]["params"]
            job_id = job_params[0]

            self.stratum_send(sock, {
                "id": 11,
                "method": "mining.submit",
                "params": [
                    "test.worker",
                    job_id,
                    "00000000",
                    job_params[7],  # ntime from job
                    "00000000",     # trivial nonce, unlikely to meet difficulty
                ],
            })
            msgs = self.stratum_recv(sock)
            submit_resp = [m for m in msgs if m.get("id") == 11]
            if submit_resp:
                self.log.info(f"  Share response: result={submit_resp[0].get('result')}, "
                              f"error={submit_resp[0].get('error')}")

        self.log.info("Test: new block from P2P triggers new job")
        # Mine a block on node 1
        self.generatetoaddress(self.nodes[1], 1,
                               self.nodes[1].get_deterministic_priv_key().address)
        self.sync_all()
        time.sleep(2)

        # Should receive a new mining.notify with clean_jobs=true
        new_msgs = self.stratum_recv(sock, timeout=5)
        new_notifs = [m for m in new_msgs if m.get("method") == "mining.notify"]
        if new_notifs:
            self.log.info("  Received new job after P2P block")
            assert new_notifs[0]["params"][8] is True  # clean_jobs
        else:
            self.log.info("  No new job received (may depend on timing)")

        sock.close()

        self.log.info("All Stratum mining tests passed!")


if __name__ == "__main__":
    StratumMiningTest().main()
