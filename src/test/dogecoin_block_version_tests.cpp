// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow/auxpow.h>
#include <pow/pow.h>
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

BOOST_AUTO_TEST_CASE(VersionLowBits_test) {
    BOOST_CHECK_EQUAL(VersionLowBits(0), 0);
    BOOST_CHECK_EQUAL(VersionLowBits(1), 1);
    BOOST_CHECK_EQUAL(VersionLowBits(0xab), 0xab);
    BOOST_CHECK_EQUAL(VersionLowBits(0x100), 0);
    BOOST_CHECK_EQUAL(VersionLowBits(0x6200ab), 0xab);
    BOOST_CHECK_EQUAL(VersionLowBits(0x6201ab), 0xab);
    BOOST_CHECK_EQUAL(VersionLowBits(0xffff0100), 0x00);
    BOOST_CHECK_EQUAL(VersionLowBits(0xffff01ab), 0xab);
}

BOOST_AUTO_TEST_CASE(VersionChainId_test) {
    BOOST_CHECK_EQUAL(VersionChainId(0), 0);
    BOOST_CHECK_EQUAL(VersionChainId(1), 0);
    BOOST_CHECK_EQUAL(VersionChainId(0xab), 0);
    BOOST_CHECK_EQUAL(VersionChainId(0x100), 0);
    BOOST_CHECK_EQUAL(VersionChainId(0x6200ab), AUXPOW_CHAIN_ID);
    BOOST_CHECK_EQUAL(VersionChainId(0x6201ab), AUXPOW_CHAIN_ID);
    BOOST_CHECK_EQUAL(VersionChainId(0xffff0100), 0xffff);
    BOOST_CHECK_EQUAL(VersionChainId(0xffff01ab), 0xffff);
}

BOOST_AUTO_TEST_CASE(VersionHasAuxPow_test) {
    BOOST_CHECK_EQUAL(VersionHasAuxPow(0), false);
    BOOST_CHECK_EQUAL(VersionHasAuxPow(1), false);
    BOOST_CHECK_EQUAL(VersionHasAuxPow(0xab), false);
    BOOST_CHECK_EQUAL(VersionHasAuxPow(0x100), true);
    BOOST_CHECK_EQUAL(VersionHasAuxPow(0x6200ab), false);
    BOOST_CHECK_EQUAL(VersionHasAuxPow(0x6201ab), true);
    BOOST_CHECK_EQUAL(VersionHasAuxPow(0xffff0100), true);
    BOOST_CHECK_EQUAL(VersionHasAuxPow(0xffff01ab), true);
}

BOOST_AUTO_TEST_CASE(VersionIsLegacy_test) {
    BOOST_CHECK_EQUAL(VersionIsLegacy(0), false);
    BOOST_CHECK_EQUAL(VersionIsLegacy(1), true);
    BOOST_CHECK_EQUAL(VersionIsLegacy(2), true);
    BOOST_CHECK_EQUAL(VersionIsLegacy(3), false);
    BOOST_CHECK_EQUAL(VersionIsLegacy(0x100), false);
    BOOST_CHECK_EQUAL(VersionIsLegacy(0x6200ab), false);
    BOOST_CHECK_EQUAL(VersionIsLegacy(0x6201ab), false);
    BOOST_CHECK_EQUAL(VersionIsLegacy(0xffff0100), false);
    BOOST_CHECK_EQUAL(VersionIsLegacy(0xffff01ab), false);
}

void SolveBlock(CBlockHeader &block, const Consensus::Params &params) {
    while (!CheckProofOfWork(block.GetPowHash(), block.nBits, params)) {
        ++block.nNonce;
    }
}

BOOST_AUTO_TEST_CASE(CheckAuxProofOfWork_nVersion_test) {
    const auto chainParams =
        CreateChainParams(*m_node.args, ChainType::REGTEST);
    const Consensus::Params &params = chainParams->GetConsensus();

    CBlockHeader header;
    header.nBits = 0x207fffff;

    header.nVersion = 0; // not allowed
    SolveBlock(header, params);
    BOOST_CHECK(!CheckAuxProofOfWork(header, params));

    header.nVersion = 1; // allowed
    SolveBlock(header, params);
    BOOST_CHECK(CheckAuxProofOfWork(header, params));

    header.nVersion = 2; // allowed
    SolveBlock(header, params);
    BOOST_CHECK(CheckAuxProofOfWork(header, params));

    header.nVersion = 3; // not allowed
    SolveBlock(header, params);
    BOOST_CHECK(!CheckAuxProofOfWork(header, params));

    // With chain ID set, all numbers 0-0xffff allowed.
    // Fixed list is just to speed up the test.
    std::vector<uint32_t> test_bits = {
        0,      1,      2,      3,      4,      5,      6,      7,
        8,      9,      10,     11,     12,     13,     14,     15,
        0x10,   0x15,   0x20,   0x2f,   0x30,   0x3f,   0x70,   0x7f,
        0x80,   0x8f,   0x90,   0x9f,   0xf0,   0xf1,   0xff,   0x100,
        0x101,  0x10f,  0x111,  0x123,  0x700,  0xf00,  0xfff,  0x1000,
        0x1fff, 0x7000, 0x7fff, 0xf000, 0xff00, 0xfff0, 0xfff1, 0xffff,
    };

    // With chain ID set, all bits allowed
    for (uint32_t bits : test_bits) {
        header.nVersion = 0x620000 | bits;
        // Disable auxpow for this test
        header.nVersion &= ~VERSION_AUXPOW_BIT;
        SolveBlock(header, params);
        BOOST_CHECK(CheckAuxProofOfWork(header, params));
    }

    // With chain ID set, only 1 and 2 are allowed
    for (uint32_t bits : test_bits) {
        header.nVersion = bits;
        // Disable auxpow for this test
        header.nVersion &= ~VERSION_AUXPOW_BIT;
        SolveBlock(header, params);
        if (header.nVersion == 1 || header.nVersion == 2) {
            BOOST_CHECK_MESSAGE(CheckAuxProofOfWork(header, params), bits);
        } else {
            BOOST_CHECK_MESSAGE(!CheckAuxProofOfWork(header, params), bits);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
