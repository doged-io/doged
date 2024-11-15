# Copyright (c) 2024 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Dogecoin's old script flags via -legacyscriptrules."""

from test_framework.address import (
    ADDRESS_ECREG_P2SH_OP_TRUE,
    ADDRESS_ECREG_UNSPENDABLE,
    SCRIPTSIG_OP_TRUE,
)
from test_framework.blocktools import (
    VERSION_CHAIN_ID_BITS,
    create_block,
    create_coinbase,
    make_conform_to_ctor,
)
from test_framework.hash import hash160
from test_framework.key import ECKey
from test_framework.messages import COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.p2p import P2PDataStore
from test_framework.script import (
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_ENDIF,
    OP_EQUAL,
    OP_HASH160,
    OP_IF,
    OP_NOP,
    OP_RETURN,
    OP_TRUE,
    CScript,
)
from test_framework.signature_hash import SignatureHash, SignatureHashForkId
from test_framework.test_framework import BitcoinTestFramework
from test_framework.txtools import pad_tx
from test_framework.util import assert_raises_rpc_error


class LegacyScriptRulesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-legacyscriptrules"], []]

    def run_test(self):
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PDataStore())
        nonlegacy_node = self.nodes[1]
        nonlegacy_peer = nonlegacy_node.add_p2p_connection(P2PDataStore())

        coinblockhash = self.generatetoaddress(node, 1, ADDRESS_ECREG_P2SH_OP_TRUE)[0]
        cointxid = node.getblock(coinblockhash)["tx"][0]

        self.generatetoaddress(node, 99, ADDRESS_ECREG_UNSPENDABLE)

        def p2sh(opcodes):
            return CScript([OP_HASH160, hash160(CScript(opcodes)), OP_EQUAL])

        def make_block_with_txs(node, txs):
            [tx.rehash() for tx in txs]
            block = create_block(
                int(node.getbestblockhash(), 16),
                create_coinbase(node.getblockcount() + 1),
            )
            block.nVersion = VERSION_CHAIN_ID_BITS | 4
            block.vtx += txs
            make_conform_to_ctor(block)
            block.hashMerkleRoot = block.calc_merkle_root()
            block.solve()
            return block

        def check_mined_txs_fail(node, peer, txs, reject_reason):
            block = make_block_with_txs(node, txs)
            peer.send_blocks_and_test(
                [block],
                node,
                success=False,
                reject_reason=reject_reason,
                expect_disconnect=True,
            )
            node.disconnect_p2ps()
            return node.add_p2p_connection(P2PDataStore())

        def check_mined_txs_success(node, peer, txs):
            block = make_block_with_txs(node, txs)
            peer.send_blocks_and_test([block], node)

        # Flags on XEC that are not on Dogecoin:
        # Mandatory flags:
        # - SCRIPT_ENABLE_SIGHASH_FORKID
        # - SCRIPT_ENABLE_SCHNORR_MULTISIG
        # - SCRIPT_ENFORCE_SIGCHECKS (not tested here)
        # - All other flags except SCRIPT_VERIFY_P2SH
        #   (which the only mandatory flag on Dogecoin)
        # Standard flags:
        # - SCRIPT_VERIFY_SIGPUSHONLY

        # Flags on Dogecoin that are not on XEC:
        # Mandatory flags:
        # - SCRIPT_VERIFY_LEGACY_RULES (tested in feature_legacy_script_rules)
        # Standard flags:
        # - SCRIPT_VERIFY_NULLDUMMY
        # - SCRIPT_VERIFY_MINIMALIF

        coinvalue = 5000000000
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(cointxid, 16), 0), SCRIPTSIG_OP_TRUE)]
        tx.vout = [
            CTxOut(2000, p2sh([OP_CHECKSIG])),
            CTxOut(2000, p2sh([OP_CHECKSIG])),
            CTxOut(2000, p2sh([OP_CHECKMULTISIG])),
            CTxOut(2000, p2sh([OP_CHECKSIG])),
            CTxOut(2000, p2sh([OP_CHECKMULTISIG])),
            CTxOut(2000, p2sh([OP_CHECKMULTISIG])),
            CTxOut(2000, p2sh([OP_IF, OP_ENDIF])),
            CTxOut(2000, CScript([OP_TRUE])),
            CTxOut(coinvalue - 30000, p2sh([OP_TRUE])),
        ]
        tx.rehash()
        txid = tx.hash

        # Fork both nodes, CLEANSTACK not enforced on Dogecoin
        tx_noncleanstack = CTransaction()
        tx_noncleanstack.vin = [
            CTxIn(COutPoint(int(txid, 16), 8), CScript([b"jonny", CScript([OP_TRUE])]))
        ]
        pad_tx(tx_noncleanstack)

        check_mined_txs_success(node, peer, [tx, tx_noncleanstack])

        nonlegacy_peer = check_mined_txs_fail(
            nonlegacy_node,
            nonlegacy_peer,
            [tx, tx_noncleanstack],
            "blk-bad-inputs, parallel script check failed",
        )
        check_mined_txs_success(nonlegacy_node, nonlegacy_peer, [tx])

        private_key = ECKey()
        private_key.set(b"FGsltwthghobStwbihsnpbhel316....", True)
        public_key = private_key.get_pubkey().get_bytes()

        script = CScript([OP_CHECKSIG])
        tx_sighash = CTransaction()
        tx_sighash.vin = [CTxIn(COutPoint(int(txid, 16), 0))]
        pad_tx(tx_sighash)

        sighash = SignatureHashForkId(script, tx_sighash, 0, 0x41, 2000)
        txsig = private_key.sign_ecdsa(sighash) + b"\x41"
        tx_sighash_bip143 = CTransaction(tx_sighash)
        tx_sighash_bip143.vin[0].scriptSig = CScript([txsig, public_key, script])

        (sighash, _) = SignatureHash(script, tx_sighash, 0, 0x01)
        txsig = private_key.sign_ecdsa(sighash) + b"\x01"
        tx_sighash_legacy = CTransaction(tx_sighash)
        tx_sighash_legacy.vin[0].scriptSig = CScript([txsig, public_key, script])

        # SIGHASH_FORKID doesn't work on Dogecoin, OP_CHECKSIG uses legacy sighash
        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Script evaluated without error but finished with a false/empty top stack element)",
            node.sendrawtransaction,
            tx_sighash_bip143.serialize().hex(),
        )
        peer = check_mined_txs_fail(
            node,
            peer,
            [tx_sighash_bip143],
            "blk-bad-inputs, parallel script check failed",
        )

        # Legacy sighash not allowed on XEC
        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Signature must use SIGHASH_FORKID)",
            nonlegacy_node.sendrawtransaction,
            tx_sighash_legacy.serialize().hex(),
        )
        nonlegacy_peer = check_mined_txs_fail(
            nonlegacy_node,
            nonlegacy_peer,
            [tx_sighash_legacy],
            "blk-bad-inputs, parallel script check failed",
        )

        # Legacy sighash ok on Dogecoin
        node.sendrawtransaction(tx_sighash_legacy.serialize().hex())
        check_mined_txs_success(node, peer, [tx_sighash_legacy])

        # SIGHASH_FORKID ok on XEC
        nonlegacy_node.sendrawtransaction(tx_sighash_bip143.serialize().hex())
        check_mined_txs_success(nonlegacy_node, nonlegacy_peer, [tx_sighash_bip143])

        # Test SIGHASH_BUG (0x20)
        script = CScript([OP_CHECKSIG])
        tx_sighash_bug = CTransaction()
        pad_tx(tx_sighash_bug)
        tx_sighash_bug.vin = [CTxIn(COutPoint(int(txid, 16), 1))]

        (sighash, _) = SignatureHash(script, tx_sighash_bug, 0, 0x21)
        txsig = private_key.sign_ecdsa(sighash) + b"\x21"
        tx_sighash_bug.vin[0].scriptSig = CScript([txsig, public_key, script])

        # Not allowed in mempool
        assert_raises_rpc_error(
            -26,
            "non-mandatory-script-verify-flag (Signature hash type missing or not understood)",
            node.sendrawtransaction,
            tx_sighash_bug.serialize().hex(),
        )
        # But: 0x20 is allowed in blocks
        check_mined_txs_success(node, peer, [tx_sighash_bug])

        # Non-legacy mode forces known sighash type
        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Signature hash type missing or not understood)",
            nonlegacy_node.sendrawtransaction,
            tx_sighash_bug.serialize().hex(),
        )
        nonlegacy_peer = check_mined_txs_fail(
            nonlegacy_node,
            nonlegacy_peer,
            [tx_sighash_bug],
            "blk-bad-inputs, parallel script check failed",
        )

        # SIGHASH_BUG for OP_CHECKMULTISIG
        script = CScript([OP_CHECKMULTISIG])
        tx_multi_sighash_bug = CTransaction()
        pad_tx(tx_multi_sighash_bug)
        tx_multi_sighash_bug.vin = [CTxIn(COutPoint(int(txid, 16), 2))]

        # Not allowed in mempool
        (sighash, _) = SignatureHash(script, tx_multi_sighash_bug, 0, 0x21)
        txsig = private_key.sign_ecdsa(sighash) + b"\x21"
        tx_multi_sighash_bug.vin[0].scriptSig = CScript([0, txsig, 1, public_key, 1, script])
        assert_raises_rpc_error(
            -26,
            "non-mandatory-script-verify-flag (Signature hash type missing or not understood)",
            node.sendrawtransaction,
            tx_multi_sighash_bug.serialize().hex(),
        )
        # But: 0x20 allowed in blocks in OP_CHECKMULTISIG
        check_mined_txs_success(node, peer, [tx_multi_sighash_bug])

        # Non-legacy mode forces known sighash type
        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Signature hash type missing or not understood)",
            nonlegacy_node.sendrawtransaction,
            tx_multi_sighash_bug.serialize().hex(),
        )
        nonlegacy_peer = check_mined_txs_fail(
            nonlegacy_node,
            nonlegacy_peer,
            [tx_multi_sighash_bug],
            "blk-bad-inputs, parallel script check failed",
        )

        # Test SIGHASH_FORKID with legacy signatures
        script = CScript([OP_CHECKSIG])
        tx_sighash_bork = CTransaction()
        pad_tx(tx_sighash_bork)
        tx_sighash_bork.vin = [CTxIn(COutPoint(int(txid, 16), 3))]

        (sighash, _) = SignatureHash(script, tx_sighash_bork, 0, 0x41)
        txsig = private_key.sign_ecdsa(sighash) + b"\x41"
        tx_sighash_bork.vin[0].scriptSig = CScript([txsig, public_key, script])

        # SIGHASH_FORKID not allowed in mempool on Dogecoin
        assert_raises_rpc_error(
            -26,
            "non-mandatory-script-verify-flag (Illegal use of SIGHASH_FORKID)",
            node.sendrawtransaction,
            tx_sighash_bork.serialize().hex(),
        )
        # But: allowed in blocks, as long as legacy signatures are used
        check_mined_txs_success(node, peer, [tx_sighash_bork])

        # Non-legacy doesn't support legacy signatures
        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Signature must be zero for failed CHECK(MULTI)SIG operation)",
            nonlegacy_node.sendrawtransaction,
            tx_sighash_bork.serialize().hex(),
        )
        nonlegacy_peer = check_mined_txs_fail(
            nonlegacy_node,
            nonlegacy_peer,
            [tx_sighash_bork],
            "blk-bad-inputs, parallel script check failed",
        )

        # SIGHASH_FORKID for OP_CHECKMULTISIG with legacy signatures
        script = CScript([OP_CHECKMULTISIG])
        tx_multi_sighash_bork = CTransaction()
        pad_tx(tx_multi_sighash_bork)
        tx_multi_sighash_bork.vin = [CTxIn(COutPoint(int(txid, 16), 4))]

        (sighash, _) = SignatureHash(script, tx_multi_sighash_bork, 0, 0x41)
        txsig = private_key.sign_ecdsa(sighash) + b"\x41"
        tx_multi_sighash_bork.vin[0].scriptSig = CScript([0, txsig, 1, public_key, 1, script])
        assert_raises_rpc_error(
            -26,
            "non-mandatory-script-verify-flag (Illegal use of SIGHASH_FORKID)",
            node.sendrawtransaction,
            tx_multi_sighash_bork.serialize().hex(),
        )
        check_mined_txs_success(node, peer, [tx_multi_sighash_bork])

        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Signature must be zero for failed CHECK(MULTI)SIG operation)",
            nonlegacy_node.sendrawtransaction,
            tx_multi_sighash_bork.serialize().hex(),
        )
        nonlegacy_peer = check_mined_txs_fail(
            nonlegacy_node,
            nonlegacy_peer,
            [tx_multi_sighash_bork],
            "blk-bad-inputs, parallel script check failed",
        )

        # Nulldummy enforced on Dogecoin, on XEC it's a bitfield
        tx_nulldummy = CTransaction()
        tx_nulldummy.vin = [
            CTxIn(
                COutPoint(int(txid, 16), 5),
                CScript([20, 0, public_key, 1, CScript([OP_CHECKMULTISIG])]),
            )
        ]
        tx_nulldummy.vout = [CTxOut(0, CScript([OP_RETURN]))]
        assert_raises_rpc_error(
            -26,
            "non-mandatory-script-verify-flag (Dummy CHECKMULTISIG argument must be zero)",
            node.sendrawtransaction,
            tx_nulldummy.serialize().hex(),
        )
        check_mined_txs_success(node, peer, [tx_nulldummy])

        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Bitfield's bit out of the expected range)",
            nonlegacy_node.sendrawtransaction,
            tx_nulldummy.serialize().hex(),
        )
        nonlegacy_peer = check_mined_txs_fail(
            nonlegacy_node,
            nonlegacy_peer,
            [tx_nulldummy],
            "blk-bad-inputs, parallel script check failed",
        )

        # MINIMALIF enforced on Dogecoin, not on XEC
        tx_minimalif = CTransaction()
        tx_minimalif.vin = [
            CTxIn(
                COutPoint(int(txid, 16), 6),
                CScript([OP_TRUE, 1337, CScript([OP_IF, OP_ENDIF])]),
            )
        ]
        pad_tx(tx_minimalif)
        assert_raises_rpc_error(
            -26,
            "non-mandatory-script-verify-flag (OP_IF/NOTIF argument must be minimal)",
            node.sendrawtransaction,
            tx_minimalif.serialize().hex(),
        )
        # Allowed in blocks in Dogecoin
        check_mined_txs_success(node, peer, [tx_minimalif])
        # Always allowed on XEC
        nonlegacy_node.sendrawtransaction(tx_minimalif.serialize().hex())
        check_mined_txs_success(nonlegacy_node, nonlegacy_peer, [tx_minimalif])

        # Non-push opcodes allowed in scriptSigs on Dogecoin in blocks
        tx_signonpush = CTransaction()
        tx_signonpush.vin = [CTxIn(COutPoint(int(txid, 16), 7), CScript([OP_NOP]))]
        pad_tx(tx_signonpush)
        check_mined_txs_success(node, peer, [tx_signonpush])
        # Disallowed on XEC
        nonlegacy_peer = check_mined_txs_fail(
            nonlegacy_node,
            nonlegacy_peer,
            [tx_signonpush],
            "blk-bad-inputs, parallel script check failed",
        )


if __name__ == "__main__":
    LegacyScriptRulesTest().main()
