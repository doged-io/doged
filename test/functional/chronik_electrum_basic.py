# Copyright (c) 2024-present The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test Chronik's electrum interface
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, chronikelectrum_port, get_cli_version

ELECTRUM_PROTOCOL_VERSION = "1.4"


class ChronikElectrumBasic(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [
            [
                "-chronik",
                f"-chronikelectrumbind=127.0.0.1:{chronikelectrum_port(0)}",
                "-chronikscripthashindex=1",
            ]
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_chronik()

    def run_test(self):
        self.node = self.nodes[0]
        self.client = self.node.get_chronik_electrum_client()
        self.test_rpc_errors()
        self.test_ping()
        self.test_server_version()
        # Run this last as it invalidates self.client
        self.test_init_errors()

    def test_rpc_errors(self):
        """Test adherence to the JSON RPC spec (error codes...)
        See https://www.jsonrpc.org/specification
        """
        response = self.client.foobar()
        assert_equal(response.result, None)
        assert_equal(response.error, {"code": -32600, "message": "Invalid request"})

        response = self.client.spam.foo.bar()
        assert_equal(response.result, None)
        assert_equal(response.error, {"code": -32601, "message": "Method not found"})

        response = self.client.server.ping("spam")
        assert_equal(
            response.error, {"code": -32602, "message": "Expected at most 0 parameters"}
        )

    def test_ping(self):
        # This method return {... "result":null} which JSON-decodes to None
        response = self.client.server.ping()
        assert_equal(response.result, None)
        assert_equal(response.error, None)

    def test_server_version(self):
        # We only support version "1.4" (exact match or within min-max range)
        expected_success_response = [
            f"{self.config['environment']['PACKAGE_NAME']} {get_cli_version(self, self.node)}",
            ELECTRUM_PROTOCOL_VERSION,
        ]
        # This is the usual call Electrum ABC will do, with a single version string
        assert_equal(
            self.client.server.version(
                "Electrum ABC 1.2.3", ELECTRUM_PROTOCOL_VERSION
            ).result,
            expected_success_response,
        )

        # The spec says we should support ranges as version tuples
        for version_tuple in (
            ("1.3.0", "1.5.0"),
            ("1.3", "1.5"),
            ("1.3", "1.4"),
            ("1.4", "1.5"),
            ("1.4", "1.4"),
            ["1.3.9", "1.4.1"],
        ):
            assert_equal(
                self.client.server.version("Electrum ABC 1.2.3", version_tuple).result,
                expected_success_response,
            )

        for unsupported_version in (None, 42, "1.3", "1.4.0", "toto"):
            assert_equal(
                self.client.server.version("", unsupported_version).error,
                {"code": 1, "message": "Unsupported protocol version"},
            )
        # Valid unsupported ranges
        for version_tuple in (("1.2", "1.3"), ("1.3.8", "1.3.9"), ("1.5", "1.6")):
            assert_equal(
                self.client.server.version("", version_tuple).error,
                {"code": 1, "message": "Unsupported protocol version"},
            )

        # More than 2 versions provided as range
        assert_equal(
            self.client.server.version("", ("1.4", "1.4", "1.4")).error,
            {"code": 1, "message": "Unsupported protocol version"},
        )

    def test_init_errors(self):
        self.node.stop_node()
        self.node.assert_start_raises_init_error(
            ["-chronik", f"-chronikelectrumbind=127.0.0.1:{chronikelectrum_port(0)}"],
            "Error: The -chronikelectrumbind option requires -chronikscripthashindex to be true.",
        )
        self.node.stop_node()
        self.node.assert_start_raises_init_error(
            [
                "-chronik",
                f"-chronikelectrumbind=127.0.0.1:{chronikelectrum_port(0)}",
                "-chronikscripthashindex=0",
            ],
            "Error: The -chronikelectrumbind option requires -chronikscripthashindex to be true.",
        )

        self.start_node(0, self.extra_args[0])


if __name__ == "__main__":
    ChronikElectrumBasic().main()
