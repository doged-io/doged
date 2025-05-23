# Copyright (c) 2020 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Tests for Bitcoin ABC mining RPCs
"""

from test_framework.address import SCRIPT_UNSPENDABLE_HEX
from test_framework.cdefs import (
    BLOCK_MAXBYTES_MAXSIGCHECKS_RATIO,
    DEFAULT_MAX_BLOCK_SIZE,
)
from test_framework.messages import XEC
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than_or_equal

MINER_FUND_ADDR = "ecregtest:prfhcnyqnl5cgrnmlfmms675w93ld7mvvq9jcw0zsn"
MINER_FUND_LEGACY_ADDR = "2NCXTUCFd1Q3EteVpVVDTrBBoKqvMPAoeEn"

MINER_FUND_RATIO = 32
STAKING_REWARD_RATIO = 10


class AbcMiningRPCTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            [
                "-enableminerfund",
            ],
            [
                "-enableminerfund",
                "-usecashaddr=0",
            ],
        ]

    def setup_network(self):
        self.setup_nodes()
        # Don't connect the nodes

    def run_for_node(self, node, minerFundAddress):
        # Connect to a peer so getblocktemplate will return results
        # (getblocktemplate has a sanity check that ensures it's connected to a
        # network).
        node.add_p2p_connection(P2PInterface())

        # Assert the results of getblocktemplate have expected values. Keys not
        # in 'expected' are not checked.
        def assert_getblocktemplate(expected):
            # Always test these values in addition to those passed in
            expected = {
                **expected,
                **{
                    "sigchecklimit": (
                        DEFAULT_MAX_BLOCK_SIZE // BLOCK_MAXBYTES_MAXSIGCHECKS_RATIO
                    ),
                },
            }

            blockTemplate = node.getblocktemplate()
            for key, value in expected.items():
                assert_equal(blockTemplate[key], value)

        def get_best_coinbase():
            return node.getblock(node.getbestblockhash(), 2)["tx"][0]

        coinbase = get_best_coinbase()
        assert_greater_than_or_equal(len(coinbase["vout"]), 2)
        block_reward = sum([vout["value"] for vout in coinbase["vout"]])

        assert_equal(node.getmempoolinfo()["size"], 0)
        assert_getblocktemplate(
            {
                "coinbasetxn": {
                    "minerfund": {
                        "addresses": [minerFundAddress],
                        "minimumvalue": (block_reward * MINER_FUND_RATIO // 100 * XEC),
                    },
                },
                # Although the coinbase value need not necessarily be the same as
                # the last block due to halvings and fees, we know this to be true
                # since we are not crossing a halving boundary and there are no
                # transactions in the mempool.
                "coinbasevalue": block_reward * XEC,
            }
        )

    def test_simplified_gbt(self):
        self.restart_node(
            0,
            extra_args=["-enableminerfund", "-avalanche=1", "-avalanchestakingrewards"],
        )

        node = self.nodes[0]
        node.add_p2p_connection(P2PInterface())

        # Force a staking reward winner so we have an block template output
        node.setstakingreward(node.getbestblockhash(), SCRIPT_UNSPENDABLE_HEX)

        gbt = node.getblocktemplate()
        assert "minerfund" not in gbt
        assert "stakingrewards" not in gbt
        assert "minerfund" in gbt["coinbasetxn"]
        assert "stakingrewards" in gbt["coinbasetxn"]

        self.restart_node(
            0,
            extra_args=[
                "-enableminerfund",
                "-avalanche=1",
                "-avalanchestakingrewards",
                "-simplegbt",
            ],
        )

        node.add_p2p_connection(P2PInterface())

        node.setstakingreward(node.getbestblockhash(), SCRIPT_UNSPENDABLE_HEX)

        gbt = node.getblocktemplate()
        assert "minerfund" in gbt
        assert "stakingrewards" in gbt
        assert "minerfund" not in gbt["coinbasetxn"]
        assert "stakingrewards" not in gbt["coinbasetxn"]

        coinbase = node.getblock(node.getbestblockhash(), 2)["tx"][0]
        block_reward = sum([vout["value"] for vout in coinbase["vout"]])

        assert_equal(
            gbt["minerfund"]["script"],
            node.validateaddress(MINER_FUND_ADDR)["scriptPubKey"],
        )
        assert_equal(
            gbt["minerfund"]["amount"], block_reward * MINER_FUND_RATIO // 100 * XEC
        )

        assert_equal(gbt["stakingrewards"]["script"], SCRIPT_UNSPENDABLE_HEX)
        assert_equal(
            gbt["stakingrewards"]["amount"],
            block_reward * STAKING_REWARD_RATIO // 100 * XEC,
        )

    def run_test(self):
        self.run_for_node(
            self.nodes[0],
            MINER_FUND_ADDR,
        )
        self.run_for_node(
            self.nodes[1],
            MINER_FUND_LEGACY_ADDR,
        )
        self.test_simplified_gbt()


if __name__ == "__main__":
    AbcMiningRPCTest().main()
