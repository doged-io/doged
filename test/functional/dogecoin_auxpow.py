# Copyright (c) 2024 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test whether Dogecoin auxpow blocks are accepted.
"""

import time

from test_framework.address import P2SH_OP_TRUE
from test_framework.blocktools import (
    VERSION_CHAIN_ID_BITS,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    MERGE_MINE_PREFIX,
    VERSION_AUXPOW_BIT,
    CAuxPow,
    COutPoint,
    CTxIn,
    calc_expected_merkle_tree_index,
    ser_uint256,
    uint256_from_compact,
)
from test_framework.script import CScript
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class DogecoinAuxpowTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]
        self.rpc_timeout = 240

    def run_test(self):
        node = self.nodes[0]

        mocktime = int(time.time())
        node.setmocktime(mocktime)

        height = 1
        coinbase_tx = create_coinbase(height)
        coinbase_tx.vout[0].scriptPubKey = P2SH_OP_TRUE
        coinbase_tx.rehash()
        block = create_block(
            int(node.getbestblockhash(), 16), coinbase_tx, mocktime + 1100
        )
        block.nVersion = VERSION_CHAIN_ID_BITS | VERSION_AUXPOW_BIT | 5
        block.rehash()
        block_hash = bytes.fromhex(block.hash)

        # nIndex = 1 not allowed
        block.auxpow = CAuxPow()
        block.auxpow.nIndex = 1
        with node.assert_debug_log(["AuxPow nIndex must be 0"]):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Parent chain ID 0x62 not allowed
        block.auxpow = CAuxPow()
        block.auxpow.parentBlock.nVersion = 0x620000
        with node.assert_debug_log(["AuxPow parent has our chain ID"]):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # len(vChainMerkleBranch) >= 31 not allowed
        block.auxpow = CAuxPow()
        block.auxpow.vChainMerkleBranch = [0] * 31
        with node.assert_debug_log(["AuxPow chain merkle branch too long"]):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # vMerkleBranch doesn't prove coinbase tx
        block.auxpow = CAuxPow()
        block.auxpow.parentBlock.hashMerkleRoot = 0x1234
        with node.assert_debug_log(["AuxPow merkle root incorrect"]):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Coinbase tx has no inputs
        block.auxpow = CAuxPow()
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256
        with node.assert_debug_log(["AuxPow coinbase transaction missing input"]):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Chain merkle root not found in coinbase scriptSig
        block.auxpow = CAuxPow()
        block.auxpow.coinbaseTx.vin = [CTxIn()]
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256
        with node.assert_debug_log(
            ["AuxPow missing chain merkle root in parent coinbase"]
        ):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Duplicate merge mine prefix
        block.auxpow = CAuxPow()
        coinbase_script = CScript(MERGE_MINE_PREFIX + MERGE_MINE_PREFIX + block_hash)
        block.auxpow.coinbaseTx.vin = [CTxIn(COutPoint(), coinbase_script)]
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256
        with node.assert_debug_log(["Multiple merged mining prefixes in coinbase"]):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Padding bytes between merge mine prefix and chain merkle root
        block.auxpow = CAuxPow()
        coinbase_script = CScript(MERGE_MINE_PREFIX + b"hola" + block_hash)
        block.auxpow.coinbaseTx.vin = [CTxIn(COutPoint(), coinbase_script)]
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256
        with node.assert_debug_log(
            ["Merged mining prefix is not just before chain merkle root"]
        ):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Too many padding bytes before the chain merkle root
        block.auxpow = CAuxPow()
        coinbase_script = CScript(bytes(21) + block_hash)
        block.auxpow.coinbaseTx.vin = [CTxIn(COutPoint(), coinbase_script)]
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256
        with node.assert_debug_log(
            [
                "AuxPow chain merkle root can have at most 20 preceding bytes of the parent coinbase"
            ]
        ):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Missing nTreeSize and nMergeMineNonce (with prefix)
        block.auxpow = CAuxPow()
        coinbase_script = CScript(MERGE_MINE_PREFIX + block_hash)
        block.auxpow.coinbaseTx.vin = [CTxIn(COutPoint(), coinbase_script)]
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256
        with node.assert_debug_log(
            ["AuxPow missing chain merkle tree size and nonce in parent coinbase"]
        ):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Missing nTreeSize and nMergeMineNonce (without prefix)
        block.auxpow = CAuxPow()
        coinbase_script = CScript(block_hash)
        block.auxpow.coinbaseTx.vin = [CTxIn(COutPoint(), coinbase_script)]
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256
        with node.assert_debug_log(
            ["AuxPow missing chain merkle tree size and nonce in parent coinbase"]
        ):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Bad nTreeSize
        block.auxpow = CAuxPow()
        coinbase_script = CScript(
            MERGE_MINE_PREFIX + block_hash + b"\xff\xff\xff\xff\xff\xff\xff\xff"
        )
        block.auxpow.coinbaseTx.vin = [CTxIn(COutPoint(), coinbase_script)]
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256
        with node.assert_debug_log(
            ["AuxPow merkle branch size does not match parent coinbase"]
        ):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        block.auxpow = CAuxPow()
        coinbase_script = CScript(
            MERGE_MINE_PREFIX + block_hash + b"\x01\0\0\0\xff\xff\xff\xff"
        )
        block.auxpow.coinbaseTx.vin = [CTxIn(COutPoint(), coinbase_script)]
        block.auxpow.coinbaseTx.rehash()
        block.auxpow.parentBlock.hashMerkleRoot = block.auxpow.coinbaseTx.sha256

        # Parent header has high hash
        target = uint256_from_compact(block.nBits)
        block.auxpow.parentBlock.rehashPow()
        while block.auxpow.parentBlock.powHash < target:
            block.auxpow.parentBlock.nNonce += 1  # un-solve block
            block.auxpow.parentBlock.rehashPow()
        with node.assert_debug_log(["Auxillary header proof of work failed"]):
            assert_equal(node.submitblock(block.serialize().hex()), "high-hash")

        # Success!
        block.solve()
        assert_equal(node.submitblock(block.serialize().hex()), None)
        assert_equal(node.getbestblockhash(), block.hash)

        # Add another block with each a merkle branch of 1 (instead of 0)
        height = 2
        coinbase_tx = create_coinbase(height)
        coinbase_tx.vout[0].scriptPubKey = P2SH_OP_TRUE
        coinbase_tx.rehash()
        block2 = create_block(block.sha256, coinbase_tx, mocktime + 1101)
        block2.nVersion = VERSION_CHAIN_ID_BITS | VERSION_AUXPOW_BIT | 0xFF
        block2.rehash()
        block_hash = bytes.fromhex(block.hash)

        nMerkleNonce = 0xD4D3D2D1
        nChainIndex = 1
        while calc_expected_merkle_tree_index(nMerkleNonce, 0x62, 1) != nChainIndex:
            nMerkleNonce += 1

        block2.auxpow = CAuxPow()
        chain_merkle_root = block2.get_merkle_root(
            [ser_uint256(555), ser_uint256(block2.sha256)]
        )
        coinbase_script = CScript(
            MERGE_MINE_PREFIX
            + ser_uint256(chain_merkle_root)[::-1]
            + b"\x02\0\0\0"
            + nMerkleNonce.to_bytes(4, "little")
        )
        block2.auxpow.coinbaseTx.vin = [CTxIn(COutPoint(), coinbase_script)]
        block2.auxpow.coinbaseTx.rehash()
        block2.auxpow.vChainMerkleBranch = [555]
        block2.auxpow.nChainIndex = 1
        block2.auxpow.vMerkleBranch = [444]
        block2.auxpow.parentBlock.hashMerkleRoot = block2.get_merkle_root(
            [ser_uint256(block2.auxpow.coinbaseTx.sha256), ser_uint256(444)]
        )
        block2.solve()

        assert_equal(node.submitblock(block2.serialize().hex()), None)
        assert_equal(node.getbestblockhash(), block2.hash)


if __name__ == "__main__":
    DogecoinAuxpowTest().main()
