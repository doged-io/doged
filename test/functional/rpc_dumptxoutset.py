# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the generation of UTXO snapshots using `dumptxoutset`.
"""
import hashlib
from pathlib import Path

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
        self.generate(node, 100)

        FILENAME = "txoutset.dat"
        out = node.dumptxoutset(FILENAME)
        expected_path = Path(node.datadir) / self.chain / FILENAME

        assert expected_path.is_file()

        assert_equal(out["coins_written"], 100)
        assert_equal(out["base_height"], 100)
        assert_equal(out["path"], str(expected_path))
        # Blockhash should be deterministic based on mocked time.
        assert_equal(
            out["base_hash"],
            "6890d3cbe20b85abc59a223d98bc939d4b1769f025d72dbed48b412b532a8f04",
        )

        with open(str(expected_path), "rb") as f:
            digest = hashlib.sha256(f.read()).hexdigest()
            # UTXO snapshot hash should be deterministic based on mocked time.
            assert_equal(
                digest,
                "222244b01e3044031180e2ac49326dfa14af2b168114b4217d0221afd2fc2717",
            )

        assert_equal(
            out["txoutset_hash"],
            "a8224e73309c8ded8bf63dc1d1ed851cd2334114243e70d0289e7964ce557aca",
        )
        assert_equal(out["nchaintx"], 101)

        # Specifying a path to an existing file will fail.
        assert_raises_rpc_error(
            -8, f"{FILENAME} already exists", node.dumptxoutset, FILENAME
        )


if __name__ == "__main__":
    DumptxoutsetTest().main()
