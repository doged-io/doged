// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow/pow.h>

#include <chain.h>
#include <chainparams.h>
#include <config.h>

#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

std::vector<CBlockIndex> MakeMockBlocks(size_t length, int32_t startHeight) {
    std::vector<CBlockIndex> blocks(length);
    for (size_t i = 0; i < length; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = startHeight + i;
    }
    return blocks;
}

BOOST_FIXTURE_TEST_SUITE(dogecoin_daa_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_first_daa) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks = MakeMockBlocks(240, 0);

    CBlockHeader header;
    blocks[0].nTime = 1386325540; // Block # 0

    // f9533416310fc4484cf43405a858b06afc9763ad401d267c1835d77e7d225a4e
    CBlockIndex *pindexLast = &blocks[239];
    BOOST_CHECK_EQUAL(pindexLast->nHeight, 239);
    pindexLast->nTime = 1386475638; // Block #239
    pindexLast->nBits = 0x1e0ffff0;

    const uint32_t expected_nbits = 0x1e0fffff;
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(pindexLast, &header, config.GetChainParams()),
        expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(
        config.GetChainParams().GetConsensus(), pindexLast->nHeight + 1,
        pindexLast->nBits, expected_nbits));
}

BOOST_AUTO_TEST_CASE(get_next_work_pre_digishield) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks = MakeMockBlocks(241, 9359);

    CBlockHeader header;
    blocks[0].nTime = 1386942008; // Block # 9359

    CBlockIndex *pindexLast = &blocks[240];
    BOOST_CHECK_EQUAL(pindexLast->nHeight, 9599);
    pindexLast->nTime = 1386954113;
    pindexLast->nBits = 0x1c1a1206;

    const uint32_t expected_nbits = 0x1c15ea59;
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(pindexLast, &header, config.GetChainParams()),
        expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(
        config.GetChainParams().GetConsensus(), pindexLast->nHeight + 1,
        pindexLast->nBits, expected_nbits));
}

BOOST_AUTO_TEST_CASE(get_next_work_digishield) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks = MakeMockBlocks(2, 144999);

    CBlockHeader header;
    blocks[0].nTime = 1395094427; // Block # 144999

    // First hard-fork at 145,000, which applies to block 145,001 onwards
    CBlockIndex *pindexLast = &blocks[1];
    BOOST_CHECK_EQUAL(pindexLast->nHeight, 145000);
    pindexLast->nTime = 1395094679;
    pindexLast->nBits = 0x1b499dfd;

    const uint32_t expected_nbits = 0x1b671062;
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(pindexLast, &header, config.GetChainParams()),
        expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(
        config.GetChainParams().GetConsensus(), pindexLast->nHeight + 1,
        pindexLast->nBits, expected_nbits));
}

BOOST_AUTO_TEST_CASE(get_next_work_digishield_modulated_upper) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks = MakeMockBlocks(2, 145106);

    CBlockHeader header;
    blocks[0].nTime = 1395100835; // Block # 145,106

    // Test the upper bound on modulated time using mainnet block #145,107
    CBlockIndex *pindexLast = &blocks[1];
    BOOST_CHECK_EQUAL(pindexLast->nHeight, 145107);
    pindexLast->nTime = 1395101360;
    pindexLast->nBits = 0x1b3439cd;

    const uint32_t expected_nbits = 0x1b4e56b3;
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(pindexLast, &header, config.GetChainParams()),
        expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(
        config.GetChainParams().GetConsensus(), pindexLast->nHeight + 1,
        pindexLast->nBits, expected_nbits));
    BOOST_CHECK(!PermittedDifficultyTransition(
        config.GetChainParams().GetConsensus(), pindexLast->nHeight + 1,
        pindexLast->nBits, expected_nbits + 1));
}

BOOST_AUTO_TEST_CASE(get_next_work_digishield_modulated_lower) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks = MakeMockBlocks(2, 149422);

    CBlockHeader header;
    blocks[0].nTime = 1395380517; // Block # 149,422

    // Test the lower bound on modulated time using mainnet block #149,423
    CBlockIndex *pindexLast = &blocks[1];
    BOOST_CHECK_EQUAL(pindexLast->nHeight, 149423);
    pindexLast->nTime = 1395380447;
    pindexLast->nBits = 0x1b446f21;
    const uint32_t expected_nbits = 0x1b335358;
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(pindexLast, &header, config.GetChainParams()),
        expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(
        config.GetChainParams().GetConsensus(), pindexLast->nHeight + 1,
        pindexLast->nBits, expected_nbits));
    BOOST_CHECK(!PermittedDifficultyTransition(
        config.GetChainParams().GetConsensus(), pindexLast->nHeight + 1,
        pindexLast->nBits, expected_nbits - 1));
}

BOOST_AUTO_TEST_CASE(get_next_work_digishield_rounding) {
    DummyConfig config(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks = MakeMockBlocks(2, 145000);

    CBlockHeader header;
    blocks[0].nTime = 1395094679;

    // Test case for correct rounding of modulated time - this depends on
    // handling of integer division, and is not obvious from the code
    CBlockIndex *pindexLast = &blocks[1];
    BOOST_CHECK_EQUAL(pindexLast->nHeight, 145001);
    pindexLast->nTime = 1395094727;
    pindexLast->nBits = 0x1b671062;
    const uint32_t expected_nbits = 0x1b6558a4;
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(pindexLast, &header, config.GetChainParams()),
        expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(
        config.GetChainParams().GetConsensus(), pindexLast->nHeight + 1,
        pindexLast->nBits, expected_nbits));
}

BOOST_AUTO_TEST_SUITE_END()
