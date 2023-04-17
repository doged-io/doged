// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow/pow.h>

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <util/chaintype.h>

#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(daa_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target) {
    const auto consensus =
        CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    BlockHash hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target) {
    const auto consensus =
        CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    BlockHash hash;
    unsigned int nBits = ~0x00800000;
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target) {
    const auto consensus =
        CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    BlockHash hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target) {
    const auto consensus =
        CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    BlockHash hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = BlockHash(ArithToUint256(hash_arith));
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target) {
    const auto consensus =
        CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    BlockHash hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = BlockHash(ArithToUint256(hash_arith));
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test) {
    DummyConfig config(ChainTypeToString(ChainType::MAIN));

    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime =
            1269211443 +
            i * config.GetChainParams().GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; // target 0x7fffff000...
        blocks[i].nChainWork =
            i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i])
              : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(
            *p1, *p2, *p3, config.GetChainParams().GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

static CBlockIndex GetBlockIndex(CBlockIndex *pindexPrev, int64_t nTimeInterval,
                                 uint32_t nBits) {
    CBlockIndex block;
    block.pprev = pindexPrev;
    block.nHeight = pindexPrev->nHeight + 1;
    block.nTime = pindexPrev->nTime + nTimeInterval;
    block.nBits = nBits;

    block.nChainWork = pindexPrev->nChainWork + GetBlockProof(block);
    return block;
}

BOOST_AUTO_TEST_CASE(retargeting_test) {
    DummyConfig config(ChainTypeToString(ChainType::MAIN));

    std::vector<CBlockIndex> blocks(115);

    const CChainParams &chainParams = config.GetChainParams();
    const Consensus::Params &params = chainParams.GetConsensus();
    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    arith_uint256 currentPow = powLimit >> 1;
    uint32_t initialBits = currentPow.GetCompact();

    // Genesis block.
    blocks[0] = CBlockIndex();
    blocks[0].nHeight = 0;
    blocks[0].nTime = 1269211443;
    blocks[0].nBits = initialBits;

    blocks[0].nChainWork = GetBlockProof(blocks[0]);

    // Pile up some blocks.
    for (size_t i = 1; i < 100; i++) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], params.nPowTargetSpacing,
                                  initialBits);
    }

    CBlockHeader blkHeaderDummy;

    // We start getting 2h blocks time. For the first 5 blocks, it doesn't
    // matter as the MTP is not affected. For the next 5 block, MTP difference
    // increases but stays below 12h.
    for (size_t i = 100; i < 110; i++) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 2 * 3600, initialBits);
        BOOST_CHECK_EQUAL(
            GetNextWorkRequired(&blocks[i], &blkHeaderDummy, chainParams),
            initialBits);
    }

    // Difficulty remains unchanged
    blocks[110] = GetBlockIndex(&blocks[109], 2 * 3600, initialBits);
    currentPow.SetCompact(currentPow.GetCompact());
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[110], &blkHeaderDummy, chainParams),
        currentPow.GetCompact());

    // As we continue with 2h blocks, difficulty remains unchanged.
    blocks[111] =
        GetBlockIndex(&blocks[110], 2 * 3600, currentPow.GetCompact());
    currentPow.SetCompact(currentPow.GetCompact());
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[111], &blkHeaderDummy, chainParams),
        currentPow.GetCompact());

    // Difficulty remains unchanged.
    blocks[112] =
        GetBlockIndex(&blocks[111], 2 * 3600, currentPow.GetCompact());
    currentPow.SetCompact(currentPow.GetCompact());
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[112], &blkHeaderDummy, chainParams),
        currentPow.GetCompact());

    // Difficulty remains unchanged, so we don't go below powLimit anyway.
    blocks[113] =
        GetBlockIndex(&blocks[112], 2 * 3600, currentPow.GetCompact());
    currentPow.SetCompact(currentPow.GetCompact());
    BOOST_CHECK(powLimit.GetCompact() != currentPow.GetCompact());
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[113], &blkHeaderDummy, chainParams),
        currentPow.GetCompact());

    // Difficulty remains unchanged, so we don't go below powLimit anyway.
    blocks[114] = GetBlockIndex(&blocks[113], 2 * 3600, powLimit.GetCompact());
    BOOST_CHECK(powLimit.GetCompact() != currentPow.GetCompact());
    BOOST_CHECK_EQUAL(
        GetNextWorkRequired(&blocks[114], &blkHeaderDummy, chainParams),
        powLimit.GetCompact());
}

BOOST_AUTO_TEST_SUITE_END()
