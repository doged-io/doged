# Copyright (c) 2024 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test coinbase maturity upgrades."""

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

        # Generate blocks until we're 241 blocks before upgrade height
        UPGRADE_HEIGHT = 1450
        OLD_MATURITY = 100
        DIGISHIELD_MATURITY = 240
        self.generatetoaddress(
            node,
            UPGRADE_HEIGHT - DIGISHIELD_MATURITY - OLD_MATURITY - 2,
            ADDRESS_ECREG_UNSPENDABLE,
        )
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT - DIGISHIELD_MATURITY - 1)

        # Get us two coins with 239 and 240 maturity at upgrade
        mature240hash = self.generatetoaddress(node, 1, ADDRESS_ECREG_P2SH_OP_TRUE)[0]
        mature240txid = node.getblock(mature240hash)["tx"][0]
        mature239hash = self.generatetoaddress(node, 1, ADDRESS_ECREG_P2SH_OP_TRUE)[0]
        mature239txid = node.getblock(mature239hash)["tx"][0]
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT - DIGISHIELD_MATURITY + 1)

        self.generatetoaddress(
            node, DIGISHIELD_MATURITY - OLD_MATURITY - 3, ADDRESS_ECREG_UNSPENDABLE
        )
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT - OLD_MATURITY - 2)

        # Generate two coins with 99 and 100 maturity one block before upgrade
        mature100hash = self.generatetoaddress(node, 1, ADDRESS_ECREG_P2SH_OP_TRUE)[0]
        mature100txid = node.getblock(mature100hash)["tx"][0]
        mature99hash = self.generatetoaddress(node, 1, ADDRESS_ECREG_P2SH_OP_TRUE)[0]
        mature99txid = node.getblock(mature99hash)["tx"][0]
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT - OLD_MATURITY)

        self.generatetoaddress(node, OLD_MATURITY - 2, ADDRESS_ECREG_UNSPENDABLE)
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT - 2)

        # After that many blocks, we had 8 halvings
        coinvalue >>= 8

        # For the mempool, we are still 1 block before upgrade.
        # We can spend the 100 mature coin fine, ...
        spend100tx = CTransaction()
        spend100tx.vin = [
            CTxIn(COutPoint(int(mature100txid, 16), 0), SCRIPTSIG_OP_TRUE)
        ]
        spend100tx.vout = [CTxOut(coinvalue - 10000, SCRIPT_UNSPENDABLE)]
        pad_tx(spend100tx)
        node.sendrawtransaction(spend100tx.serialize().hex())

        # ...but not the 99 mature coin
        spend99tx = CTransaction()
        spend99tx.vin = [CTxIn(COutPoint(int(mature99txid, 16), 0), SCRIPTSIG_OP_TRUE)]
        spend99tx.vout = [CTxOut(coinvalue - 10000, SCRIPT_UNSPENDABLE)]
        pad_tx(spend99tx)
        assert_raises_rpc_error(
            -26,
            "bad-txns-premature-spend-of-coinbase, tried to spend coinbase at depth 99",
            node.sendrawtransaction,
            spend99tx.serialize().hex(),
        )

        # Can't mine the 99 mature coin spend, ...
        block = create_block(
            int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount() + 1)
        )
        block.nVersion = 4
        block.vtx += [spend99tx]
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        peer.send_blocks_and_test(
            [block],
            node,
            success=False,
            reject_reason="bad-txns-premature-spend-of-coinbase, tried to spend coinbase at depth 99",
            expect_disconnect=True,
            timeout=5,
        )
        peer = self.reconnect_p2p()

        # ... but we can mine the 100 mature coin spend
        # Also, this activates the upgrade
        minedhash1 = self.generatetoaddress(node, 1, ADDRESS_ECREG_UNSPENDABLE)[0]
        assert_equal(
            node.getrawtransaction(spend100tx.hash, 2)["blockhash"], minedhash1
        )
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT - 1)

        # For the mempool the upgrade is now activated, so we can't spend a 239 mature coin
        spend239tx = CTransaction()
        spend239tx.vin = [
            CTxIn(COutPoint(int(mature239txid, 16), 0), SCRIPTSIG_OP_TRUE)
        ]
        spend239tx.vout = [CTxOut(coinvalue - 10000, SCRIPT_UNSPENDABLE)]
        pad_tx(spend239tx)
        assert_raises_rpc_error(
            -26,
            "bad-txns-premature-spend-of-coinbase, tried to spend coinbase at depth 239",
            node.sendrawtransaction,
            spend239tx.serialize().hex(),
        )

        # But, we can spend the 240 mature coin
        spend240tx = CTransaction()
        spend240tx.vin = [
            CTxIn(COutPoint(int(mature240txid, 16), 0), SCRIPTSIG_OP_TRUE)
        ]
        spend240tx.vout = [CTxOut(coinvalue - 10000, SCRIPT_UNSPENDABLE)]
        pad_tx(spend240tx)
        node.sendrawtransaction(spend240tx.serialize().hex())

        # Can't mine the 239 mature coin spend, ...
        block = create_block(
            int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount() + 1)
        )
        block.nVersion = 4
        block.vtx += [spend239tx]
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

        # ... but we can mine the 240 mature coin spend
        minedhash2 = self.generatetoaddress(node, 1, ADDRESS_ECREG_UNSPENDABLE)[0]
        assert_equal(
            node.getrawtransaction(spend240tx.hash, 2)["blockhash"], minedhash2
        )
        assert_equal(node.getblockcount(), UPGRADE_HEIGHT)


if __name__ == "__main__":
    UpgradeCoinbaseMaturityTest().main()
