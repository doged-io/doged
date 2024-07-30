# Copyright (c) 2024 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test we sign valid Dogecoin legacy signatures for transactions."""

from test_framework.address import ADDRESS_ECREG_UNSPENDABLE
from test_framework.key import ECKey
from test_framework.messages import COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.script import (
    OP_CHECKSIG,
    OP_DUP,
    OP_EQUALVERIFY,
    OP_HASH160,
    CScript,
)
from test_framework.hash import hash160
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error
from test_framework.wallet_util import bytes_to_wif


class LegacyScriptSignrawtransactionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-legacyscriptrules"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        nonlegacy_node = self.nodes[1]

        coinblockhash1 = self.generate(node, 1)[0]
        cointxid1 = node.getblock(coinblockhash1)["tx"][0]

        coinblockhash2 = self.generate(nonlegacy_node, 1)[0]
        cointxid2 = nonlegacy_node.getblock(coinblockhash2)["tx"][0]

        self.generatetoaddress(node, 99, ADDRESS_ECREG_UNSPENDABLE)

        key = ECKey()
        key.set(b"ABCD" * 8, True)
        pubkey = key.get_pubkey().get_bytes()
        p2pkh = CScript(
            [OP_DUP, OP_HASH160, hash160(pubkey), OP_EQUALVERIFY, OP_CHECKSIG]
        )

        coinvalue = 5000000000

        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(cointxid1, 16), 0))]
        tx.vout = [CTxOut(coinvalue - 1000, p2pkh)]

        nonlegacy_tx = CTransaction()
        nonlegacy_tx.vin = [CTxIn(COutPoint(int(cointxid2, 16), 0))]
        nonlegacy_tx.vout = [CTxOut(coinvalue - 1000, p2pkh)]

        legacy_sighashes = [
            "ALL",
            "NONE",
            "SINGLE",
            "ALL|ANYONECANPAY",
            "NONE|ANYONECANPAY",
            "SINGLE|ANYONECANPAY",
        ]
        for sighash in legacy_sighashes:
            result = node.signrawtransactionwithwallet(tx.serialize().hex(), [], sighash)
            assert result["complete"]
            assert_raises_rpc_error(
                -8,
                "Signature must use SIGHASH_FORKID",
                nonlegacy_node.signrawtransactionwithwallet,
                nonlegacy_tx.serialize().hex(),
                [],
                sighash,
            )

        bip143_sighashes = [
            "ALL|FORKID",
            "NONE|FORKID",
            "SINGLE|FORKID",
            "ALL|FORKID|ANYONECANPAY",
            "NONE|FORKID|ANYONECANPAY",
            "SINGLE|FORKID|ANYONECANPAY",
        ]
        for sighash in bip143_sighashes:
            assert_raises_rpc_error(
                -8,
                "Illegal use of SIGHASH_FORKID",
                node.signrawtransactionwithwallet,
                tx.serialize().hex(),
                [],
                sighash,
            )
            result = nonlegacy_node.signrawtransactionwithwallet(
                nonlegacy_tx.serialize().hex(), [], sighash
            )
            assert result["complete"]

        # Legacy tx works
        signed_tx = node.signrawtransactionwithwallet(tx.serialize().hex(), [], "ALL")
        assert signed_tx["complete"]
        txid = node.sendrawtransaction(signed_tx["hex"])
        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Signature must use SIGHASH_FORKID)",
            nonlegacy_node.sendrawtransaction,
            signed_tx["hex"],
        )

        signed_tx = nonlegacy_node.signrawtransactionwithwallet(
            nonlegacy_tx.serialize().hex(), [], "ALL|FORKID"
        )
        assert signed_tx["complete"]
        nonlegacy_txid = nonlegacy_node.sendrawtransaction(signed_tx["hex"])
        # Note: NULLFAIL is not mandatory on legacy, so we just get a FALSE from OP_CHECKSIG
        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Script evaluated without error but finished with a false/empty top stack element)",
            node.sendrawtransaction,
            signed_tx["hex"],
        )

        tx2 = CTransaction()
        tx2.vin = [CTxIn(COutPoint(int(txid, 16), 0))]
        tx2.vout = [CTxOut(coinvalue - 2000, p2pkh)]

        nonlegacy_tx2 = CTransaction()
        nonlegacy_tx2.vin = [CTxIn(COutPoint(int(nonlegacy_txid, 16), 0))]
        nonlegacy_tx2.vout = [CTxOut(coinvalue - 2000, p2pkh)]

        tx2_keys = [bytes_to_wif(key.get_bytes())]
        tx2_outpoints = [
            {
                "txid": txid,
                "vout": 0,
                "scriptPubKey": p2pkh.hex(),
                "amount": tx.vout[0].nValue / 100,
            }
        ]
        nonlegacy_tx2_outpoints = [
            {
                "txid": nonlegacy_txid,
                "vout": 0,
                "scriptPubKey": p2pkh.hex(),
                "amount": tx.vout[0].nValue / 100,
            }
        ]
        for sighash in legacy_sighashes:
            result = node.signrawtransactionwithkey(
                tx2.serialize().hex(), tx2_keys, tx2_outpoints, sighash
            )
            assert result["complete"]
            assert_raises_rpc_error(
                -8,
                "Signature must use SIGHASH_FORKID",
                nonlegacy_node.signrawtransactionwithkey,
                nonlegacy_tx2.serialize().hex(),
                tx2_keys,
                nonlegacy_tx2_outpoints,
                sighash,
            )
        for sighash in bip143_sighashes:
            assert_raises_rpc_error(
                -8,
                "Illegal use of SIGHASH_FORKID",
                node.signrawtransactionwithkey,
                tx2.serialize().hex(),
                tx2_keys,
                tx2_outpoints,
                sighash,
            )
            result = nonlegacy_node.signrawtransactionwithkey(
                nonlegacy_tx2.serialize().hex(),
                tx2_keys,
                nonlegacy_tx2_outpoints,
                sighash,
            )
            assert result["complete"]


if __name__ == "__main__":
    LegacyScriptSignrawtransactionTest().main()
