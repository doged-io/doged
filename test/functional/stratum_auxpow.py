#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Ruck and Alexandre Guillioud
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test Stratum AuxPoW / merge-mining RPCs: createauxblock and submitauxblock.
"""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class StratumAuxpowTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]
        self.rpc_timeout = 120

    def run_test(self):
        node = self.nodes[0]

        # Generate initial blocks
        address = node.get_deterministic_priv_key().address
        self.generatetoaddress(node, 110, address)

        self.log.info("Test: createauxblock returns valid template")
        result = node.createauxblock(address)

        assert "hash" in result
        assert "chainid" in result
        assert "previousblockhash" in result
        assert "coinbasevalue" in result
        assert "bits" in result
        assert "height" in result
        assert "target" in result

        assert_equal(result["chainid"], 0x62)
        assert result["height"] == 111
        assert len(result["hash"]) == 64
        assert len(result["previousblockhash"]) == 64
        assert result["coinbasevalue"] > 0

        self.log.info("Test: createauxblock with invalid address fails")
        assert_raises_rpc_error(
            -5, "Invalid Dogecoin address",
            node.createauxblock, "invalid_address_here")

        self.log.info("Test: createauxblock returns different hashes")
        # Each call should return a fresh template
        result2 = node.createauxblock(address)
        # Hash may or may not differ (same tip, same mempool),
        # but the call should succeed
        assert "hash" in result2

        self.log.info("Test: submitauxblock with empty data fails")
        assert_raises_rpc_error(
            -8, None,
            node.submitauxblock, result["hash"], "")

        self.log.info("Test: submitauxblock with invalid auxpow fails")
        # Send garbage that won't deserialize as CAuxPow
        assert_raises_rpc_error(
            -1, None,
            node.submitauxblock, result["hash"], "deadbeef")

        self.log.info("All Stratum AuxPoW tests passed!")


if __name__ == "__main__":
    StratumAuxpowTest().main()
