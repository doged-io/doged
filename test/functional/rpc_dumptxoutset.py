# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the generation of UTXO snapshots using `dumptxoutset`.
"""
import hashlib
from pathlib import Path

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class DumptxoutsetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Test a trivial usage of the dumptxoutset RPC command."""
        node = self.nodes[0]
        mocktime = node.getblockheader(node.getblockhash(0))["time"] + 1
        node.setmocktime(mocktime)
        self.generate(node, COINBASE_MATURITY)

        FILENAME = "txoutset.dat"
        out = node.dumptxoutset(FILENAME, "latest")
        expected_path = Path(node.datadir) / self.chain / FILENAME

        assert expected_path.is_file()

        assert_equal(out["coins_written"], 100)
        assert_equal(out["base_height"], 100)
        assert_equal(out["path"], str(expected_path))
        # Blockhash should be deterministic based on mocked time.
        assert_equal(
            out["base_hash"],
            "0c64f61cda125314ff31071f73c11ed17629c56b3085c3e608caa3e54dc1fe34",
        )

        with open(str(expected_path), "rb") as f:
            digest = hashlib.sha256(f.read()).hexdigest()
            # UTXO snapshot hash should be deterministic based on mocked time.
            assert_equal(
                digest,
                "8e26b58d89e9afb2bae1a02e48629e2161b99c60d5730234d2fd9b4bba28aa43",
            )

        assert_equal(
            out["txoutset_hash"],
            "f44ed9c4e538bd29271d7ef6e3802b6ac3aef329801813da54e985fa2e2fc94f",
        )
        assert_equal(out["nchaintx"], 101)

        # Specifying a path to an existing file will fail.
        assert_raises_rpc_error(
            -8, f"{FILENAME} already exists", node.dumptxoutset, FILENAME, "latest"
        )

        self.log.info("Test that dumptxoutset with unknown dump type fails")
        assert_raises_rpc_error(
            -8,
            'Invalid snapshot type "bogus" specified. Please specify "rollback" or "latest"',
            node.dumptxoutset,
            "utxos.dat",
            "bogus",
        )


if __name__ == "__main__":
    DumptxoutsetTest().main()
