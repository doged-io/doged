# Copyright (c) 2024 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Dogecoin's old script rule enforcement via -legacyscriptrules."""

from test_framework.address import (
    ADDRESS_ECREG_P2SH_OP_TRUE,
    ADDRESS_ECREG_UNSPENDABLE,
    SCRIPT_UNSPENDABLE,
    SCRIPTSIG_OP_TRUE,
)
from test_framework.blocktools import (
    VERSION_CHAIN_ID_BITS,
    create_block,
    create_coinbase,
)
from test_framework.hash import hash160
from test_framework.key import ECKey
from test_framework.messages import COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.p2p import P2PDataStore
from test_framework.script import (
    OP_AND,
    OP_BIN2NUM,
    OP_CAT,
    OP_CHECKDATASIG,
    OP_CHECKDATASIGVERIFY,
    OP_CHECKSIG,
    OP_CHECKSIGVERIFY,
    OP_DIV,
    OP_ENDIF,
    OP_EQUAL,
    OP_HASH160,
    OP_IF,
    OP_MOD,
    OP_NOP,
    OP_NUM2BIN,
    OP_OR,
    OP_REVERSEBYTES,
    OP_SPLIT,
    OP_XOR,
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

        def make_block_with_tx(node, tx):
            tx.rehash()
            block = create_block(
                int(node.getbestblockhash(), 16),
                create_coinbase(node.getblockcount() + 1),
            )
            block.nVersion = VERSION_CHAIN_ID_BITS | 4
            block.vtx += [tx]
            block.hashMerkleRoot = block.calc_merkle_root()
            block.solve()
            return block

        def check_mined_tx_fails(node, peer, tx, reject_reason):
            block = make_block_with_tx(node, tx)
            peer.send_blocks_and_test(
                [block],
                node,
                success=False,
                reject_reason=reject_reason,
                expect_disconnect=True,
            )
            node.disconnect_p2ps()
            return node.add_p2p_connection(P2PDataStore())

        def check_mined_tx_success(node, peer, tx):
            block = make_block_with_tx(node, tx)
            peer.send_blocks_and_test([block], node)

        # These opcodes are disabled in legacy mode:
        disabled_opcodes = [
            OP_CAT,
            OP_SPLIT,
            OP_AND,
            OP_OR,
            OP_XOR,
            OP_DIV,
            OP_MOD,
            OP_NUM2BIN,
            OP_BIN2NUM,
        ]
        # These opcodes are removed (i.e. allowed in OP_IF OP_ENDIF)
        removed_opcodes = [OP_REVERSEBYTES, OP_CHECKDATASIG, OP_CHECKDATASIGVERIFY]

        # Build tx with scripts having those opcodes in P2SH outputs
        coinvalue = 5000000000
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(cointxid, 16), 0), SCRIPTSIG_OP_TRUE)]

        for opcode in disabled_opcodes + removed_opcodes:
            tx.vout += [
                CTxOut(2000, p2sh([opcode, OP_NOP])),
                CTxOut(2000, p2sh([OP_IF, opcode, OP_ENDIF])),
            ]

        tx.vout += [
            CTxOut(2000, p2sh([OP_CHECKSIG])),
            CTxOut(2000, p2sh([OP_CHECKSIGVERIFY])),
        ]

        tx.vout.append(CTxOut(coinvalue - 200000, SCRIPT_UNSPENDABLE))

        txid = node.sendrawtransaction(tx.serialize().hex())
        nonlegacy_node.sendrawtransaction(tx.serialize().hex())

        check_mined_tx_success(node, peer, tx)

        for opcode_idx, opcode in enumerate(disabled_opcodes):
            out_idx = opcode_idx * 2

            spend_tx = CTransaction()
            spend_tx.vin = [
                CTxIn(
                    COutPoint(int(txid, 16), out_idx),
                    CScript([b"xx", b"yy", CScript([opcode, OP_NOP])]),
                )
            ]
            pad_tx(spend_tx)
            assert_raises_rpc_error(
                -26,
                "mandatory-script-verify-flag-failed (Attempted to use a disabled opcode)",
                node.sendrawtransaction,
                spend_tx.serialize().hex(),
            )
            peer = check_mined_tx_fails(
                node, peer, spend_tx, "blk-bad-inputs, parallel script check failed"
            )
            del spend_tx

            # Disabled opcodes are also banned in OP_IF OP_ENDIF branches
            spend_if_tx = CTransaction()
            spend_if_tx.vin = [
                CTxIn(
                    COutPoint(int(txid, 16), out_idx + 1),
                    CScript([True, False, CScript([OP_IF, opcode, OP_ENDIF])]),
                )
            ]
            pad_tx(spend_if_tx)
            assert_raises_rpc_error(
                -26,
                "mandatory-script-verify-flag-failed (Attempted to use a disabled opcode)",
                node.sendrawtransaction,
                spend_if_tx.serialize().hex(),
            )
            peer = check_mined_tx_fails(
                node, peer, spend_if_tx, "blk-bad-inputs, parallel script check failed"
            )
            # Non-legacy nodes don't have these opcodes disabled
            nonlegacy_node.sendrawtransaction(spend_if_tx.serialize().hex())
            check_mined_tx_success(nonlegacy_node, nonlegacy_peer, spend_if_tx)
            del spend_if_tx

        for opcode_idx, opcode in enumerate(removed_opcodes):
            out_idx = (len(disabled_opcodes) + opcode_idx) * 2

            # Removed opcodes are just "misunderstood", they did nothing wrong
            spend_tx = CTransaction()
            spend_tx.vin = [
                CTxIn(
                    COutPoint(int(txid, 16), out_idx),
                    CScript([b"xx", b"yy", CScript([opcode, OP_NOP])]),
                )
            ]
            pad_tx(spend_tx)
            assert_raises_rpc_error(
                -26,
                "mandatory-script-verify-flag-failed (Opcode missing or not understood)",
                node.sendrawtransaction,
                spend_tx.serialize().hex(),
            )
            peer = check_mined_tx_fails(
                node, peer, spend_tx, "blk-bad-inputs, parallel script check failed"
            )
            del spend_tx

            # Removed opcodes are allowed in OP_IF OP_ENDIF branches
            spend_if_tx = CTransaction()
            spend_if_tx.vin = [
                CTxIn(
                    COutPoint(int(txid, 16), out_idx + 1),
                    CScript([True, False, CScript([OP_IF, opcode, OP_ENDIF])]),
                )
            ]
            pad_tx(spend_if_tx)
            node.sendrawtransaction(spend_if_tx.serialize().hex())
            nonlegacy_node.sendrawtransaction(spend_if_tx.serialize().hex())
            check_mined_tx_success(nonlegacy_node, nonlegacy_peer, spend_if_tx)
            del spend_if_tx

        # Generate a key pair
        private_key = ECKey()
        private_key.set(b"Schnorr!" * 4, True)
        # get uncompressed public key serialization
        public_key = private_key.get_pubkey().get_bytes()

        checksig_out_idx = len(tx.vout) - 3

        # Schnorr sigs in OP_CHECKSIG are not allowed in legacy mode
        spend_tx = CTransaction()
        spend_tx.vin = [CTxIn(COutPoint(int(txid, 16), checksig_out_idx))]
        pad_tx(spend_tx)
        script = CScript([OP_CHECKSIG])
        (sighash, _) = SignatureHash(script, spend_tx, 0, 0x01)
        txsig = private_key.sign_schnorr(sighash) + b"\x01"
        spend_tx.vin[0].scriptSig = CScript([txsig, public_key, script])

        # Regtest has SCRIPT_VERIFY_NULLFAIL, which comes in handy here
        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Signature must be zero for failed CHECK(MULTI)SIG operation)",
            node.sendrawtransaction,
            spend_tx.serialize().hex(),
        )
        peer = check_mined_tx_fails(
            node, peer, spend_tx, "blk-bad-inputs, parallel script check failed"
        )
        # Schnorr sigs allowed on XEC with FORKID
        sighash = SignatureHashForkId(script, spend_tx, 0, 0x41, 2000)
        txsig = private_key.sign_schnorr(sighash) + b"\x41"
        spend_tx.vin[0].scriptSig = CScript([txsig, public_key, script])
        nonlegacy_node.sendrawtransaction(spend_tx.serialize().hex())
        check_mined_tx_success(nonlegacy_node, nonlegacy_peer, spend_tx)

        # Schnorr sigs in OP_CHECKSIGVERIFY are not allowed in legacy mode
        spend_tx = CTransaction()
        spend_tx.vin = [CTxIn(COutPoint(int(txid, 16), checksig_out_idx + 1))]
        pad_tx(spend_tx)
        script = CScript([OP_CHECKSIGVERIFY])
        (sighash, _) = SignatureHash(script, spend_tx, 0, 0x01)
        txsig = private_key.sign_schnorr(sighash) + b"\x01"
        spend_tx.vin[0].scriptSig = CScript([True, txsig, public_key, script])

        assert_raises_rpc_error(
            -26,
            "mandatory-script-verify-flag-failed (Signature must be zero for failed CHECK(MULTI)SIG operation)",
            node.sendrawtransaction,
            spend_tx.serialize().hex(),
        )
        peer = check_mined_tx_fails(
            node, peer, spend_tx, "blk-bad-inputs, parallel script check failed"
        )
        sighash = SignatureHashForkId(script, spend_tx, 0, 0x41, 2000)
        txsig = private_key.sign_schnorr(sighash) + b"\x41"
        spend_tx.vin[0].scriptSig = CScript([True, txsig, public_key, script])
        nonlegacy_node.sendrawtransaction(spend_tx.serialize().hex())
        check_mined_tx_success(nonlegacy_node, nonlegacy_peer, spend_tx)


if __name__ == "__main__":
    LegacyScriptRulesTest().main()
