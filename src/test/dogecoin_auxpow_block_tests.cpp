// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow/auxpow.h>
#include <pow/pow.h>
#include <primitives/auxpow.h>
#include <validation.h>

#include <test/lcg.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(dogecoin_auxpow_block_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(mine_auxpow_blocks_test) {
    ChainstateManager &chainman = *Assert(m_node.chainman);
    const Config &config = GetConfig();
    const Consensus::Params &consensus = config.GetChainParams().GetConsensus();
    const BlockValidationOptions blockopts(config);

    CBlock block =
        CreateAuxPowBlock({}, CScript() << OP_1, 0x63, 0x12345678, {uint256()},
                          {uint256(), uint256()}, chainman.ActiveChainstate());

    // Block must have the auxpow version bit set if it has auxpow
    {
        CBlock block = CreateAuxPowBlock(
            {}, CScript() << OP_1, 0x63, 0x12345678, {uint256()},
            {uint256(), uint256()}, chainman.ActiveChainstate());
        block.nVersion = VersionWithAuxPow(block.nVersion, false);
        BlockValidationState state;
        BOOST_CHECK(!CheckBlock(block, state, consensus, blockopts));
        BOOST_CHECK_EQUAL(state.ToString(), "high-hash, proof of work failed");
    }

    // Block must NOT have the auxpow version bit set if it does NOT has auxpow
    {
        CBlock block = CreateAuxPowBlock(
            {}, CScript() << OP_1, 0x63, 0x12345678, {uint256()},
            {uint256(), uint256()}, chainman.ActiveChainstate());
        block.auxpow.reset();
        BlockValidationState state;
        BOOST_CHECK(!CheckBlock(block, state, consensus, blockopts));
        BOOST_CHECK_EQUAL(state.ToString(), "high-hash, proof of work failed");
    }

    // CheckAuxBlockHash failed: Parent chain ID can't be our chain ID
    {
        CBlock block = CreateAuxPowBlock(
            {}, CScript() << OP_1, AUXPOW_CHAIN_ID, 0x12345678, {uint256()},
            {uint256(), uint256()}, chainman.ActiveChainstate());
        BlockValidationState state;
        BOOST_CHECK(!CheckBlock(block, state, consensus, blockopts));
        BOOST_CHECK_EQUAL(state.ToString(), "high-hash, proof of work failed");
    }

    // CheckAuxBlockHash failed: nIndex must be 0
    {
        CBlock block = CreateAuxPowBlock(
            {}, CScript() << OP_1, 0x63, 0x12345678, {uint256()},
            {uint256(), uint256()}, chainman.ActiveChainstate());
        block.auxpow->nIndex = 1;
        BlockValidationState state;
        BOOST_CHECK(!CheckBlock(block, state, consensus, blockopts));
        BOOST_CHECK_EQUAL(state.ToString(), "high-hash, proof of work failed");
    }

    // High-hash on parent block
    {
        CBlock block = CreateAuxPowBlock(
            {}, CScript() << OP_1, 0x63, 0x12345678, {uint256()},
            {uint256(), uint256()}, chainman.ActiveChainstate());

        // Ensure parent block's nonce DOESNT mine the block
        while (CheckProofOfWork(block.auxpow->parentBlock.GetPowHash(),
                                block.nBits, consensus)) {
            ++block.auxpow->parentBlock.nNonce;
        }

        BlockValidationState state;
        BOOST_CHECK(!CheckBlock(block, state, consensus, blockopts));
        BOOST_CHECK_EQUAL(state.ToString(), "high-hash, proof of work failed");
    }

    // Valid block
    {
        CBlock block = CreateAuxPowBlock(
            {}, CScript() << OP_1, 0x63, 0x12345678, {uint256()},
            {uint256(), uint256()}, chainman.ActiveChainstate());
        BlockValidationState state;
        BOOST_CHECK(CheckBlock(block, state, consensus, blockopts));
    }
}

uint256 GenHash(MMIXLinearCongruentialGenerator &lcg) {
    uint256 hash;
    for (uint8_t &byte : hash) {
        byte = lcg.next();
    }
    return hash;
}

BOOST_AUTO_TEST_CASE(mine_auxpow_blocks_rng_test) {
    MMIXLinearCongruentialGenerator lcg;

    // Randomly generate a lot of configurations and test for successes.
    for (size_t testCase = 0; testCase < 100; testCase++) {
        // Generate random parameters

        uint32_t parentChainId = lcg.next() % 0x10000;
        if (parentChainId == AUXPOW_CHAIN_ID) {
            // Cannot have parent chain ID equal to our chain ID
            parentChainId = (parentChainId + 1) % 0x10000;
        }

        const uint32_t chainMerkleHeight = lcg.next() % 31;
        const uint32_t nMergeMineNonce = lcg.next();
        const uint32_t coinbaseMerkleHeight = lcg.next() % 32;

        std::vector<uint256> chainMerkleBranch;
        for (size_t i = 0; i < chainMerkleHeight; ++i) {
            chainMerkleBranch.push_back(GenHash(lcg));
        }
        std::vector<uint256> coinbaseMerkleBranch;
        for (size_t i = 0; i < coinbaseMerkleHeight; ++i) {
            coinbaseMerkleBranch.push_back(GenHash(lcg));
        }

        CreateAndProcessAuxPowBlock({}, CScript() << OP_1, parentChainId,
                                    nMergeMineNonce, chainMerkleBranch,
                                    coinbaseMerkleBranch);
    }
}

BOOST_AUTO_TEST_SUITE_END()
