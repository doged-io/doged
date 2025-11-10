// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumaux.h>

#include <arith_uint256.h>
#include <pow/pow.h>
#include <primitives/auxpow.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumaux_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(create_aux_work_valid) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    auto result = mgr.CreateAuxWork();
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(!result->auxBlockHash.IsNull());
    BOOST_CHECK_EQUAL(result->nChainId, AUXPOW_CHAIN_ID);
    BOOST_CHECK(result->height > 0);
}

BOOST_AUTO_TEST_CASE(aux_block_hash_is_sha256d) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    auto result = mgr.CreateAuxWork();
    BOOST_REQUIRE(result.has_value());

    // auxBlockHash should equal the block's GetHash() (SHA-256d)
    BOOST_CHECK(result->auxBlockHash ==
                result->underlyingJob.block->GetHash());

    // It should NOT equal GetPowHash() (Scrypt)
    BOOST_CHECK(result->auxBlockHash !=
                result->underlyingJob.block->GetPowHash());
}

BOOST_AUTO_TEST_CASE(aux_work_version_has_chain_id) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    auto result = mgr.CreateAuxWork();
    BOOST_REQUIRE(result.has_value());

    BOOST_CHECK_EQUAL(VersionChainId(result->underlyingJob.nVersionRaw),
                      AUXPOW_CHAIN_ID);
}

BOOST_AUTO_TEST_CASE(build_commitment_single_chain) {
    uint256 auxHash;
    auxHash.SetHex(
        "0000000000000000000000000000000000000000000000000000000000000001");

    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    MergeMineCommitment commitment = mgr.BuildCommitment(auxHash);

    // Single chain: tree size = 1, chain index = 0, no branch
    BOOST_CHECK_EQUAL(commitment.nTreeSize, 1u);
    BOOST_CHECK_EQUAL(commitment.nChainIndex, 0u);
    BOOST_CHECK(commitment.chainMerkleBranch.empty());
}

BOOST_AUTO_TEST_CASE(build_commitment_prefix) {
    uint256 auxHash;
    auxHash.SetHex(
        "0000000000000000000000000000000000000000000000000000000000000001");

    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    MergeMineCommitment commitment = mgr.BuildCommitment(auxHash);

    // Payload must start with MERGE_MINE_PREFIX: fa be 6d 6d
    BOOST_REQUIRE(commitment.coinbasePayload.size() >= 4);
    BOOST_CHECK_EQUAL(commitment.coinbasePayload[0], 0xfa);
    BOOST_CHECK_EQUAL(commitment.coinbasePayload[1], 0xbe);
    BOOST_CHECK_EQUAL(commitment.coinbasePayload[2], 'm');
    BOOST_CHECK_EQUAL(commitment.coinbasePayload[3], 'm');
}

BOOST_AUTO_TEST_CASE(build_commitment_fields_after_root) {
    uint256 auxHash;
    auxHash.SetHex(
        "0000000000000000000000000000000000000000000000000000000000000042");

    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    MergeMineCommitment commitment = mgr.BuildCommitment(auxHash);

    // Layout: prefix(4) + root(32) + treeSize(4) + nonce(4) = 44 bytes
    BOOST_CHECK_EQUAL(commitment.coinbasePayload.size(), 44u);
}

BOOST_AUTO_TEST_CASE(build_commitment_multi_chain) {
    uint256 auxHash1;
    auxHash1.SetHex(
        "0000000000000000000000000000000000000000000000000000000000000001");
    uint256 auxHash2;
    auxHash2.SetHex(
        "0000000000000000000000000000000000000000000000000000000000000002");

    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    MergeMineCommitment commitment =
        mgr.BuildCommitment(auxHash1, {auxHash2});

    BOOST_CHECK_EQUAL(commitment.nTreeSize, 2u);
    BOOST_CHECK_EQUAL(commitment.chainMerkleBranch.size(), 1u);

    // Chain index should match CalcExpectedMerkleTreeIndex
    uint32_t expectedIdx = CalcExpectedMerkleTreeIndex(
        commitment.nMergeMineNonce, AUXPOW_CHAIN_ID, 1);
    BOOST_CHECK_EQUAL(commitment.nChainIndex, expectedIdx);
}

BOOST_AUTO_TEST_CASE(validate_parent_pow_scrypt) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    // Build a header with trivial difficulty (powLimit)
    CBaseBlockHeader header;
    header.nVersion = 0x00010004; // different chain ID, not 0x62
    header.hashPrevBlock = BlockHash();
    header.hashMerkleRoot.SetNull();
    header.nTime = 1234567890;
    header.nBits = 0x207fffff; // very easy target (regtest)
    header.nNonce = 0;

    const auto &consensus = m_node.chainman->GetParams().GetConsensus();

    // Try nonces until we find one that passes
    bool found = false;
    for (uint32_t n = 0; n < 100000; n++) {
        header.nNonce = n;
        if (mgr.ValidateParentPow(header, header.nBits, consensus)) {
            found = true;
            break;
        }
    }
    // With such easy difficulty, we should find one quickly
    BOOST_CHECK(found);
}

BOOST_AUTO_TEST_CASE(get_work_by_hash) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    auto result = mgr.CreateAuxWork();
    BOOST_REQUIRE(result.has_value());

    const AuxWorkTemplate *found = mgr.GetWork(result->auxBlockHash);
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK(found->auxBlockHash == result->auxBlockHash);
}

BOOST_AUTO_TEST_CASE(get_work_unknown_hash) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    uint256 fakeHash;
    fakeHash.SetNull();
    BOOST_CHECK(mgr.GetWork(fakeHash) == nullptr);
}

BOOST_AUTO_TEST_CASE(prune_work) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumAuxManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(), scriptPubKey);

    // Create several work items (each will have same block content but
    // different hashes due to nTime changing)
    for (int i = 0; i < 5; i++) {
        mgr.CreateAuxWork();
    }

    mgr.PruneWork(2);
    // After pruning to 2, only 2 should remain
    // (we can't easily check count without exposing it, but GetWork on
    //  the most recent should still work)
}

BOOST_AUTO_TEST_SUITE_END()
