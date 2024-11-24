# Copyright (c) 2024 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test default dogecoin fees.
"""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, remove_from_config


class DogecoinFeesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-usedogeunit=1"]]
        self.rpc_timeout = 240

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]

        # Remove test framework configs added in util.py
        remove_from_config(
            node.datadir, ["minrelaytxfee=10", "blockmintxfee=10", "fallbackfee=200"]
        )

        self.restart_node(0)

        assert_equal(node.getnetworkinfo()["relayfee"], Decimal("0.001"))
        assert_equal(node.getmempoolinfo()["minrelaytxfee"], Decimal("0.001"))
        assert_equal(node.getmempoolinfo()["mempoolminfee"], Decimal("0.001"))

        address = node.getnewaddress()
        address2 = node.getnewaddress()

        self.generatetoaddress(node, 110, address)

        txid = node.sendtoaddress(address2, 10)

        # -paytxfee is 10x that of the relay fee
        tx = node.gettransaction(txid)
        assert_equal(tx["fee"], Decimal("-0.0022500"))


if __name__ == "__main__":
    DogecoinFeesTest().main()
