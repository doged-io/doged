#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test basic Stratum server functionality: connect, subscribe, authorize,
receive jobs, and disconnect.
"""

import json
import socket

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class StratumBasicTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            "-stratum",
            "-stratumport=23333",
            "-stratumbind=127.0.0.1",
            "-stratumdifficulty=0.001",
        ]]

    def stratum_connect(self, port=23333):
        """Create a TCP connection to the Stratum server."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(("127.0.0.1", port))
        return sock

    def stratum_send(self, sock, msg):
        """Send a JSON-RPC message over Stratum."""
        data = json.dumps(msg) + "\n"
        sock.sendall(data.encode())

    def stratum_recv(self, sock, timeout=5):
        """Receive and parse JSON-RPC messages from Stratum."""
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

    def stratum_subscribe(self, sock, req_id=1):
        """Send mining.subscribe and return response."""
        self.stratum_send(sock, {
            "id": req_id,
            "method": "mining.subscribe",
            "params": ["stratum_basic_test/1.0"],
        })
        msgs = self.stratum_recv(sock)
        for msg in msgs:
            if msg.get("id") == req_id:
                return msg
        return None

    def stratum_authorize(self, sock, worker="test.worker", password="x",
                          req_id=2):
        """Send mining.authorize and return response."""
        self.stratum_send(sock, {
            "id": req_id,
            "method": "mining.authorize",
            "params": [worker, password],
        })
        # May receive set_difficulty and notify before auth response
        msgs = self.stratum_recv(sock, timeout=5)
        auth_response = None
        notifications = []
        for msg in msgs:
            if msg.get("id") == req_id:
                auth_response = msg
            else:
                notifications.append(msg)
        # Collect any additional notifications
        more = self.stratum_recv(sock, timeout=2)
        notifications.extend(more)
        return auth_response, notifications

    def run_test(self):
        node = self.nodes[0]

        # Generate some blocks so the chain is active
        self.generatetoaddress(node, 110, node.get_deterministic_priv_key().address)

        # Wait for stratum server to be ready
        self.wait_until(
            lambda: node.debug_log_path.read_text(
                encoding="utf-8", errors="replace"
            ).find("Stratum: server started") != -1,
            timeout=10,
        )

        self.log.info("Test: TCP connection to Stratum server")
        sock = self.stratum_connect()
        assert sock is not None

        self.log.info("Test: mining.subscribe")
        sub_response = self.stratum_subscribe(sock)
        assert sub_response is not None
        assert sub_response.get("error") is None
        result = sub_response["result"]
        # Result: [[subscriptions], extranonce1, extranonce2_size]
        assert len(result) == 3
        extranonce1 = result[1]
        extranonce2_size = result[2]
        assert len(extranonce1) == 8  # 4 bytes = 8 hex chars
        assert extranonce2_size == 4

        self.log.info("Test: mining.authorize")
        auth_response, notifications = self.stratum_authorize(sock)
        assert auth_response is not None
        assert auth_response["result"] is True
        assert auth_response.get("error") is None

        self.log.info("Test: receive mining.set_difficulty")
        diff_notifs = [n for n in notifications
                       if n.get("method") == "mining.set_difficulty"]
        # Should receive at least one difficulty notification
        assert len(diff_notifs) >= 1
        assert diff_notifs[0]["params"][0] > 0

        self.log.info("Test: receive mining.notify")
        notify_notifs = [n for n in notifications
                         if n.get("method") == "mining.notify"]
        if not notify_notifs:
            # May need to wait a bit more
            more = self.stratum_recv(sock, timeout=3)
            notify_notifs = [n for n in more
                             if n.get("method") == "mining.notify"]
        assert len(notify_notifs) >= 1

        job_params = notify_notifs[0]["params"]
        assert len(job_params) == 9
        job_id = job_params[0]
        prev_hash = job_params[1]
        version = job_params[5]
        nbits = job_params[6]
        ntime = job_params[7]
        clean_jobs = job_params[8]

        assert len(prev_hash) == 64
        assert len(version) == 8
        assert len(nbits) == 8
        assert len(ntime) == 8
        assert isinstance(clean_jobs, bool)

        self.log.info("Test: mining.notify version has chain ID 0x62")
        # Version is little-endian hex, parse to uint32
        version_bytes = bytes.fromhex(version)
        version_int = int.from_bytes(version_bytes, "little")
        chain_id = (version_int >> 16) & 0xFFFF
        assert_equal(chain_id, 0x62)

        self.log.info("Test: authorize before subscribe fails")
        sock2 = self.stratum_connect()
        self.stratum_send(sock2, {
            "id": 1,
            "method": "mining.authorize",
            "params": ["user", "pass"],
        })
        msgs = self.stratum_recv(sock2)
        auth_msg = None
        for msg in msgs:
            if msg.get("id") == 1:
                auth_msg = msg
        assert auth_msg is not None
        assert auth_msg.get("error") is not None
        sock2.close()

        self.log.info("Test: multiple concurrent connections get unique extranonce1")
        sock3 = self.stratum_connect()
        sub3 = self.stratum_subscribe(sock3, req_id=10)
        assert sub3 is not None
        extranonce1_2 = sub3["result"][1]
        assert extranonce1 != extranonce1_2

        sock.close()
        sock3.close()

        self.log.info("All Stratum basic tests passed!")


if __name__ == "__main__":
    StratumBasicTest().main()
