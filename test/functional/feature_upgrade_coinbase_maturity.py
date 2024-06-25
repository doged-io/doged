# Copyright (c) 2024 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Dogecoin's coinbase maturity upgrade."""

from test_framework.address import (
    ADDRESS_ECREG_P2SH_OP_TRUE,
    ADDRESS_ECREG_UNSPENDABLE,
    SCRIPT_UNSPENDABLE,
    SCRIPTSIG_OP_TRUE,
)
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.p2p import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.txtools import pad_tx
from test_framework.util import assert_equal, assert_raises_rpc_error


class UpgradeCoinbaseMaturityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex"]]

    def reconnect_p2p(self):
        self.nodes[0].disconnect_p2ps()
        return self.nodes[0].add_p2p_connection(P2PDataStore())

    def run_test(self):
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PDataStore())

        coinblockhash = self.generatetoaddress(node, 1, ADDRESS_ECREG_P2SH_OP_TRUE)[0]
        cointxid = node.getblock(coinblockhash)["tx"][0]

        # Only generate 98 blocks, insufficient for maturity
        self.generatetoaddress(node, 98, ADDRESS_ECREG_UNSPENDABLE)
        assert_equal(node.getblockcount(), 99)

        coinvalue = 5000000000
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(cointxid, 16), 0), SCRIPTSIG_OP_TRUE)]
        tx.vout = [CTxOut(coinvalue - 10000, SCRIPT_UNSPENDABLE)]
        pad_tx(tx)

        # Insufficient maturity
        assert_raises_rpc_error(
            -26,
            "bad-txns-premature-spend-of-coinbase, tried to spend coinbase at depth 99",
            node.sendrawtransaction,
            tx.serialize().hex(),
        )

        # Generate 1 more block, coins are now mature
        self.generatetoaddress(node, 1, ADDRESS_ECREG_UNSPENDABLE)
        assert_equal(node.getblockcount(), 100)
        node.sendrawtransaction(tx.serialize().hex())

        # Make sure tx got mined
        minedhash = self.generatetoaddress(node, 1, ADDRESS_ECREG_UNSPENDABLE)[0]
        assert_equal(node.getrawtransaction(tx.hash, 2)["blockhash"], minedhash)

        # Generate blocks until we're 2 blocks before upgrade height
        UPGRADE_HEIGHT = 1450
        OLD_MATURITY = 100
        DIGISHIELD_MATURITY = 240
        self.generatetoaddress(
            node,
            UPGRADE_HEIGHT - 103,
            ADDRESS_ECREG_UNSPENDABLE,
        )
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT - 2)

        # Get us a coin one block before upgrade
        preupgradehash = self.generatetoaddress(node, 1, ADDRESS_ECREG_P2SH_OP_TRUE)[0]
        preupgradetxid = node.getblock(preupgradehash)["tx"][0]
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT - 1)

        # Get us a coin after the upgrade
        postupgradehash = self.generatetoaddress(node, 1, ADDRESS_ECREG_P2SH_OP_TRUE)[0]
        postupgradetxid = node.getblock(postupgradehash)["tx"][0]
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT)

        # After that many blocks, we had 9 halvings
        coinvalue >>= 9

        # Mature first coin to 98 (mempool thinks it's 99)
        self.generatetoaddress(node, OLD_MATURITY - 3, ADDRESS_ECREG_UNSPENDABLE)
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT + OLD_MATURITY - 3)

        # The pre-upgrade coin still needs 1 extra block
        spend_preupgradetx = CTransaction()
        spend_preupgradetx.vin = [
            CTxIn(COutPoint(int(preupgradetxid, 16), 0), SCRIPTSIG_OP_TRUE)
        ]
        spend_preupgradetx.vout = [CTxOut(coinvalue - 10000, SCRIPT_UNSPENDABLE)]
        pad_tx(spend_preupgradetx)
        assert_raises_rpc_error(
            -26,
            "bad-txns-premature-spend-of-coinbase, tried to spend coinbase at depth 99",
            node.sendrawtransaction,
            spend_preupgradetx.serialize().hex(),
        )

        # Can't mine the 99 mature coin spend
        block = create_block(
            int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount() + 1)
        )
        block.nVersion = 4
        block.vtx += [spend_preupgradetx]
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        peer.send_blocks_and_test(
            [block],
            node,
            success=False,
            reject_reason="bad-txns-premature-spend-of-coinbase, tried to spend coinbase at depth 99",
            expect_disconnect=True,
        )
        peer = self.reconnect_p2p()

        # After 1 extra block, pre-upgrade coin is mature
        self.generatetoaddress(node, 1, ADDRESS_ECREG_UNSPENDABLE)
        node.sendrawtransaction(spend_preupgradetx.serialize().hex())

        # Make sure we actually mined the tx
        minedhash1 = self.generatetoaddress(node, 1, ADDRESS_ECREG_UNSPENDABLE)[0]
        assert_equal(
            node.getrawtransaction(spend_preupgradetx.hash, 2)["blockhash"], minedhash1
        )

        # Mature second coin to 238 (mempool thinks it's 239)
        self.generatetoaddress(
            node, DIGISHIELD_MATURITY - OLD_MATURITY - 1, ADDRESS_ECREG_UNSPENDABLE
        )
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT + DIGISHIELD_MATURITY - 2)

        spend_postupgradetx = CTransaction()
        spend_postupgradetx.vin = [
            CTxIn(COutPoint(int(postupgradetxid, 16), 0), SCRIPTSIG_OP_TRUE)
        ]
        spend_postupgradetx.vout = [CTxOut(coinvalue - 10000, SCRIPT_UNSPENDABLE)]
        pad_tx(spend_postupgradetx)
        assert_raises_rpc_error(
            -26,
            "bad-txns-premature-spend-of-coinbase, tried to spend coinbase at depth 239",
            node.sendrawtransaction,
            spend_postupgradetx.serialize().hex(),
        )

        # Can't mine the 239 mature coin spend
        block = create_block(
            int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount() + 1)
        )
        block.nVersion = 4
        block.vtx += [spend_postupgradetx]
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        peer.send_blocks_and_test(
            [block],
            node,
            success=False,
            reject_reason="bad-txns-premature-spend-of-coinbase, tried to spend coinbase at depth 239",
            expect_disconnect=True,
        )
        peer = self.reconnect_p2p()

        # After 1 extra block, post-upgrade coin is mature
        self.generatetoaddress(node, 1, ADDRESS_ECREG_UNSPENDABLE)
        node.sendrawtransaction(spend_postupgradetx.serialize().hex())

        # Make sure we actually mined the tx
        minedhash2 = self.generatetoaddress(node, 1, ADDRESS_ECREG_UNSPENDABLE)[0]
        assert_equal(
            node.getrawtransaction(spend_postupgradetx.hash, 2)["blockhash"], minedhash2
        )


if __name__ == "__main__":
    UpgradeCoinbaseMaturityTest().main()
