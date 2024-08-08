// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/auxpow.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(dogecoin_block_version_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(MakeVersionWithChainId_test) {
    BOOST_CHECK_EQUAL(MakeVersionWithChainId(0, 0), 0);
    BOOST_CHECK_EQUAL(MakeVersionWithChainId(1, 0), 0x10000);
    BOOST_CHECK_EQUAL(MakeVersionWithChainId(AUXPOW_CHAIN_ID, 0), 0x620000);

    BOOST_CHECK_EQUAL(MakeVersionWithChainId(0, 0xab), 0xab);
    BOOST_CHECK_EQUAL(MakeVersionWithChainId(1, 0xab), 0x100ab);
    BOOST_CHECK_EQUAL(MakeVersionWithChainId(AUXPOW_CHAIN_ID, 0xab), 0x6200ab);

    // nChainId is validated
    BOOST_CHECK_EQUAL(MakeVersionWithChainId(MAX_ALLOWED_CHAIN_ID, 0),
                      0xffff0000);
    BOOST_CHECK_THROW(MakeVersionWithChainId(MAX_ALLOWED_CHAIN_ID + 1, 0),
                      std::invalid_argument);
    BOOST_CHECK_THROW(MakeVersionWithChainId(0x70000000, 0),
                      std::invalid_argument);
    BOOST_CHECK_THROW(MakeVersionWithChainId(0x10000, 0x100),
                      std::invalid_argument);

    // nLowVersionBits is validated
    BOOST_CHECK_THROW(MakeVersionWithChainId(0, 0x100), std::invalid_argument);
    BOOST_CHECK_THROW(MakeVersionWithChainId(0, 0x70000000),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(VersionWithAuxPow_test) {
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0, false), 0);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x100, false), 0);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0xab, false), 0xab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x1ab, false), 0xab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x620000, false), 0x620000);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x620100, false), 0x620000);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x6200ab, false), 0x6200ab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x6201ab, false), 0x6200ab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0xffff00ab, false), 0xffff00ab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0xffff01ab, false), 0xffff00ab);

    BOOST_CHECK_EQUAL(VersionWithAuxPow(0, true), 0x100);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x100, true), 0x100);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0xab, true), 0x1ab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x1ab, true), 0x1ab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x620000, true), 0x620100);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x620100, true), 0x620100);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x6200ab, true), 0x6201ab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0x6201ab, true), 0x6201ab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0xffff00ab, true), 0xffff01ab);
    BOOST_CHECK_EQUAL(VersionWithAuxPow(0xffff01ab, true), 0xffff01ab);
}

BOOST_AUTO_TEST_SUITE_END()
