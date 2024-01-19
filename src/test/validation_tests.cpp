// Copyright (c) 2011-2019 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>

#include <chainparams.h>
#include <clientversion.h>
#include <common/system.h>
#include <config.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <core_io.h>
#include <hash.h>
#include <net.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <cstdio>
#include <tuple>
#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(validation_tests, TestingSetup)

// First 20 blocks on the Dogecoin network, with their coinbase subsidy
const std::vector<std::tuple<std::string, int64_t>> DOGECOIN_BLOCKS = {
    // Genesis block has 88 DOGE blockreward, but not used in the tests here.
    {"1a91e3dace36e2be3bf030a65679fe821aa1d6ef92e7c9902eb318182c355691", 88},
    {"82bc68038f6034c0596b6e313729793a887fded6e92a31fbdf70863f89d9bea2", 68416},
    {"ea5380659e02a68c073369e502125c634b2fb0aaf351b9360c673368c4f20c96",
     729752},
    {"76f80a8a81e6f6669d340651723b874f97395c4dbda200f8b024df4c6566a92c", 11183},
    {"df363f95151d8c38b1cf0ee8375d571c9a869d9e37489ba058982ace19bcdee4",
     324378},
    {"f21dc70cb44c180261e31a222202678602d605e7697332cb2395386fa309ad3b",
     262711},
    {"f34986a114a2f58f48ce5593e5e6006666243fb003a2ffa489d980d8de825428",
     863413},
    {"3ca7e813da5c72b0817c4d4789cd4896f49baf5e40f67a59a63a63ea2498d604",
     141339},
    {"ed6b216e69b57915eda3a43036016c5667b35d61606f1c61ba10fb17c0e7a063",
     857035},
    {"0ddd48852cb794c7534841a2cd3507e40255b1707fac14d65d16cc791672e5e2",
     515758},
    {"31bf9377a7f52b97bc7ec001a107b3c3c0f7ff629767c2bcc4e27873dad5fc21",
     890107},
    {"45b727adf6b098223d8360332f51256cf00dd9a3501cf09d50767264e733fe72",
     241677},
    {"30fd7677bc28675e8663feef4f4b6ceab936407a3a365e4af4cc7e14b62bdd3d",
     279377},
    {"4a991ce1ca12b6160776a978f5ac0635b9d497c2516fdfa37894a4d7bfe2f66e",
     909605},
    {"f95c467d4ed4c0082b7da6b5fbffd60e33ab55ad0541358f945d7243674058b0",
     348510},
    {"9ff299652aba8a4de7f8edfa42500de0e47525486674c23ad68d3147d19514a1",
     837643},
    {"fbc2620f0cb7f490d3cba09ddadb9c79fca7187180d7cf9053537e3bc872c1fb",
     214794},
    {"0c120ab190655673a709bc92ad86f80dc1cd9f11f9e0f09ebc5e6a3058b73002",
     434206},
    {"da0e2362cc1d1cd48c8eb70e578c97f00d9a530985ba36027eb7e3fba98c74ae",
     190398},
    {"7f34d92c06a2b38cac860d62026e716c9a73a759363891b1bf7d0cd465c6acba",
     653360},
};

BOOST_AUTO_TEST_CASE(subsidy_mainnet_first_20_blocks) {
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const Consensus::Params &params = chainParams->GetConsensus();
    for (int32_t height = 1; height < (int32_t)DOGECOIN_BLOCKS.size();
         ++height) {
        Amount subsidy = GetBlockSubsidy(
            height, params, uint256S(std::get<0>(DOGECOIN_BLOCKS[height - 1])));
        BOOST_CHECK_EQUAL(subsidy, std::get<1>(DOGECOIN_BLOCKS[height]) * COIN);
    }
}

BOOST_AUTO_TEST_CASE(subsidy_first_100k_test) {
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const Consensus::Params &params = chainParams->GetConsensus();
    Amount nSum;
    arith_uint256 prevHash;

    for (int32_t nHeight = 0; nHeight <= 100000; nHeight++) {
        Amount nSubsidy =
            GetBlockSubsidy(nHeight, params, ArithToUint256(prevHash));
        BOOST_CHECK(MoneyRange(nSubsidy));
        BOOST_CHECK(nSubsidy <= 1000000 * COIN);
        nSum += nSubsidy;
        // Use nSubsidy to give us some variation in previous block hash,
        // without requiring full block templates
        // Must shift 200 bits because only bits 200-227 are used for the seed
        prevHash += arith_uint256(nSubsidy / SATOSHI) << 200;
    }

    const Amount expected = int64_t(49555514013LL) * COIN;
    BOOST_CHECK_EQUAL(expected, nSum);
}

BOOST_AUTO_TEST_CASE(subsidy_100k_145k_test) {
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const Consensus::Params &params = chainParams->GetConsensus();
    Amount nSum;
    arith_uint256 prevHash;

    for (int32_t nHeight = 100000; nHeight <= 145000; nHeight++) {
        Amount nSubsidy =
            GetBlockSubsidy(nHeight, params, ArithToUint256(prevHash));
        BOOST_CHECK(MoneyRange(nSubsidy));
        BOOST_CHECK(nSubsidy <= 500000 * COIN);
        nSum += nSubsidy;
        // Use nSubsidy to give us some variation in previous block hash,
        // without requiring full block templates
        // Must shift 200 bits because only bits 200-227 are used for the seed
        prevHash += arith_uint256(nSubsidy / SATOSHI) << 200;
    }

    const Amount expected = int64_t(11029457731LL) * COIN;
    BOOST_CHECK_EQUAL(expected, nSum);
}

// Check the simplified rewards after block 145,000
BOOST_AUTO_TEST_CASE(subsidy_post_145k_test) {
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const Consensus::Params &params = chainParams->GetConsensus();
    const uint256 prevHash;

    for (int32_t nHeight = 145000; nHeight < 600000; nHeight++) {
        Amount nSubsidy = GetBlockSubsidy(nHeight, params, prevHash);
        Amount nExpectedSubsidy = (500000 >> (nHeight / 100000)) * COIN;
        BOOST_CHECK(MoneyRange(nSubsidy));
        BOOST_CHECK_EQUAL(nSubsidy, nExpectedSubsidy);
    }

    // Test reward at 600k+ is constant
    Amount nConstantSubsidy = GetBlockSubsidy(600000, params, prevHash);
    BOOST_CHECK_EQUAL(nConstantSubsidy, 10000 * COIN);

    nConstantSubsidy = GetBlockSubsidy(700000, params, prevHash);
    BOOST_CHECK_EQUAL(nConstantSubsidy, 10000 * COIN);
}

// This test is broken; it is copied from the original dogecoin test suite to
// prevent divergence. The test tweaks prevHash, but the bits determining the
// seed are untouched, such that the seed is always 0, resulting in no test
// coverage for the seed extraction code.
BOOST_AUTO_TEST_CASE(broken_dogecoin_subsidy_first_100k_test) {
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const Consensus::Params &params = chainParams->GetConsensus();
    Amount nSum;
    arith_uint256 prevHash;

    for (int nHeight = 0; nHeight <= 100000; nHeight++) {
        Amount nSubsidy =
            GetBlockSubsidy(nHeight, params, ArithToUint256(prevHash));
        BOOST_CHECK(MoneyRange(nSubsidy));
        BOOST_CHECK(nSubsidy <= 1000000 * COIN);
        nSum += nSubsidy;
        prevHash += nSubsidy / SATOSHI;
    }

    const Amount expected = int64_t(54894174438LL) * COIN;
    BOOST_CHECK_EQUAL(expected, nSum);
}

// This test is broken, see broken_dogecoin_subsidy_first_100k_test
BOOST_AUTO_TEST_CASE(broken_dogecoin_subsidy_100k_145k_test) {
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const Consensus::Params &params = chainParams->GetConsensus();
    Amount nSum;
    arith_uint256 prevHash;

    for (int nHeight = 100000; nHeight <= 145000; nHeight++) {
        Amount nSubsidy =
            GetBlockSubsidy(nHeight, params, ArithToUint256(prevHash));
        BOOST_CHECK(MoneyRange(nSubsidy));
        BOOST_CHECK(nSubsidy <= 500000 * COIN);
        nSum += nSubsidy;
        prevHash += nSubsidy / SATOSHI;
    }

    const Amount expected = int64_t(12349960000LL) * COIN;
    BOOST_CHECK_EQUAL(expected, nSum);
}

static void TestBlockSubsidyHalvings(const Consensus::Params &consensusParams) {
    int maxHalvings = 64;
    Amount nInitialSubsidy = 50 * COIN;

    // for height == 0
    Amount nPreviousSubsidy = 2 * nInitialSubsidy;
    BOOST_CHECK_EQUAL(nPreviousSubsidy, 2 * nInitialSubsidy);
    for (int32_t nHalvings = 0; nHalvings < maxHalvings; nHalvings++) {
        int32_t nHeight = nHalvings * consensusParams.nSubsidyHalvingInterval;
        Amount nSubsidy = GetBlockSubsidy(nHeight, consensusParams, uint256());
        BOOST_CHECK(nSubsidy <= nInitialSubsidy);
        BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
        nPreviousSubsidy = nSubsidy;
    }
    BOOST_CHECK_EQUAL(
        GetBlockSubsidy(maxHalvings * consensusParams.nSubsidyHalvingInterval,
                        consensusParams, uint256()),
        Amount::zero());
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval) {
    Consensus::Params consensusParams;
    consensusParams.fPowNoRetargeting = true;
    consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    TestBlockSubsidyHalvings(consensusParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test) {
    // As in Bitcoin
    TestBlockSubsidyHalvings(210000);
    // As in regtest
    TestBlockSubsidyHalvings(150);
    // Just another interval
    TestBlockSubsidyHalvings(1000);
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test) {
    Consensus::Params params;
    params.fPowNoRetargeting = true;
    params.nSubsidyHalvingInterval = 210000; // Bitcoin
    Amount nSum = Amount::zero();
    for (int nHeight = 0; nHeight < 14000000; nHeight += 1000) {
        Amount nSubsidy = GetBlockSubsidy(nHeight, params, uint256());
        BOOST_CHECK(nSubsidy <= 50 * COIN);
        nSum += 1000 * nSubsidy;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, int64_t(2099999997690000LL) * SATOSHI);
}

static CBlock makeLargeDummyBlock(const size_t num_tx) {
    CBlock block;
    block.vtx.reserve(num_tx);

    CTransaction tx;
    for (size_t i = 0; i < num_tx; i++) {
        block.vtx.push_back(MakeTransactionRef(tx));
    }
    return block;
}

/**
 * Test that LoadExternalBlockFile works with the buffer size set below the
 * size of a large block. Currently, LoadExternalBlockFile has the buffer size
 * for CBufferedFile set to 2 * MAX_TX_SIZE. Test with a value of
 * 10 * MAX_TX_SIZE.
 */
BOOST_AUTO_TEST_CASE(validation_load_external_block_file) {
    fs::path tmpfile_name = gArgs.GetDataDirNet() / "block.dat";

    FILE *fp = fopen(fs::PathToString(tmpfile_name).c_str(), "wb+");

    BOOST_CHECK(fp != nullptr);

    const CChainParams &chainparams = m_node.chainman->GetParams();

    // serialization format is:
    // message start magic, size of block, block

    size_t nwritten = fwrite(std::begin(chainparams.DiskMagic()),
                             CMessageHeader::MESSAGE_START_SIZE, 1, fp);

    BOOST_CHECK_EQUAL(nwritten, 1UL);

    CTransaction empty_tx;
    size_t empty_tx_size = GetSerializeSize(empty_tx, CLIENT_VERSION);

    size_t num_tx = (10 * MAX_TX_SIZE) / empty_tx_size;

    CBlock block = makeLargeDummyBlock(num_tx);

    BOOST_CHECK(GetSerializeSize(block, CLIENT_VERSION) > 2 * MAX_TX_SIZE);

    unsigned int size = GetSerializeSize(block, CLIENT_VERSION);
    {
        CAutoFile outs(fp, SER_DISK, CLIENT_VERSION);
        outs << size;
        outs << block;
        outs.release();
    }

    fseek(fp, 0, SEEK_SET);
    BOOST_CHECK_NO_THROW({ m_node.chainman->LoadExternalBlockFile(fp, 0); });
}

//! Test retrieval of valid assumeutxo values.
BOOST_AUTO_TEST_CASE(test_assumeutxo) {
    const auto params = CreateChainParams(*m_node.args, ChainType::REGTEST);

    // These heights don't have assumeutxo configurations associated, per the
    // contents of chainparams.cpp.
    std::vector<int> bad_heights{0, 100, 111, 115, 209, 211};

    for (auto empty : bad_heights) {
        const auto out = params->AssumeutxoForHeight(empty);
        BOOST_CHECK(!out);
    }

    const auto out110 = *params->AssumeutxoForHeight(110);
    BOOST_CHECK_EQUAL(
        out110.hash_serialized.ToString(),
        "fcfa07adecbe5f753b9f062b5e5621dcdd9f998a45968876cb98d350667d745e");
    BOOST_CHECK_EQUAL(out110.nChainTx, 111U);

    const auto out110_2 = *params->AssumeutxoForBlockhash(BlockHash{uint256S(
        "0xd5a3182b833dca6d0c9bad770890080c2639f077e0e880c5ab16f1ba3a27b740")});
    BOOST_CHECK_EQUAL(
        out110_2.hash_serialized.ToString(),
        "fcfa07adecbe5f753b9f062b5e5621dcdd9f998a45968876cb98d350667d745e");
    BOOST_CHECK_EQUAL(out110_2.nChainTx, 111U);
}

BOOST_AUTO_TEST_CASE(block_malleation) {
    // Test utilities that calls `IsBlockMutated` and then clears the validity
    // cache flags on `CBlock`.
    auto is_mutated = [](CBlock &block) {
        bool mutated{IsBlockMutated(block)};
        block.fChecked = false;
        block.m_checked_merkle_root = false;
        return mutated;
    };
    auto is_not_mutated = [&is_mutated](CBlock &block) {
        return !is_mutated(block);
    };

    // Test utilities to create coinbase transactions
    auto create_coinbase_tx = [](bool include_witness = false) {
        CMutableTransaction coinbase;
        coinbase.vin.resize(1);
        coinbase.vout.resize(1);
        coinbase.vout[0].scriptPubKey.resize(38);
        coinbase.vout[0].scriptPubKey[0] = OP_RETURN;
        coinbase.vout[0].scriptPubKey[1] = 0x24;
        coinbase.vout[0].scriptPubKey[2] = 0xaa;
        coinbase.vout[0].scriptPubKey[3] = 0x21;
        coinbase.vout[0].scriptPubKey[4] = 0xa9;
        coinbase.vout[0].scriptPubKey[5] = 0xed;

        auto tx = MakeTransactionRef(coinbase);
        assert(tx->IsCoinBase());
        return tx;
    };
    {
        CBlock block;

        // Empty block is expected to have merkle root of 0x0.
        BOOST_CHECK(block.vtx.empty());
        block.hashMerkleRoot = uint256{1};
        BOOST_CHECK(is_mutated(block));
        block.hashMerkleRoot = uint256{};
        BOOST_CHECK(is_not_mutated(block));

        // Block with a single coinbase tx is mutated if the merkle root is not
        // equal to the coinbase tx's hash.
        block.vtx.push_back(create_coinbase_tx());
        BOOST_CHECK(block.vtx[0]->GetHash() != block.hashMerkleRoot);
        BOOST_CHECK(is_mutated(block));
        block.hashMerkleRoot = block.vtx[0]->GetHash();
        BOOST_CHECK(is_not_mutated(block));

        // Block with two transactions is mutated if the merkle root does not
        // match the double sha256 of the concatenation of the two transaction
        // hashes.
        {
            block.vtx.push_back(MakeTransactionRef(CMutableTransaction{}));
            BOOST_CHECK(is_mutated(block));
            HashWriter hasher;
            hasher << block.vtx[0]->GetId();
            hasher << block.vtx[1]->GetId();
            block.hashMerkleRoot = hasher.GetHash();
            BOOST_CHECK(is_not_mutated(block));
        }

        // Block with two transactions is mutated if any node is duplicate.
        {
            block.vtx[1] = block.vtx[0];
            BOOST_CHECK(is_mutated(block));
            HashWriter hasher;
            hasher << block.vtx[0]->GetId();
            hasher << block.vtx[1]->GetId();
            block.hashMerkleRoot = hasher.GetHash();
            BOOST_CHECK(is_mutated(block));
        }

        // Blocks with 64-byte coinbase transactions are not considered mutated
        block.vtx.clear();
        {
            CMutableTransaction mtx;
            mtx.vin.resize(1);
            mtx.vout.resize(1);
            mtx.vout[0].scriptPubKey.resize(4);
            block.vtx.push_back(MakeTransactionRef(mtx));
            block.hashMerkleRoot = block.vtx.back()->GetHash();
            assert(block.vtx.back()->IsCoinBase());
            assert(GetSerializeSize(block.vtx.back(), PROTOCOL_VERSION) == 64);
        }
        BOOST_CHECK(is_not_mutated(block));
    }

    {
        // Test merkle root malleation

        // Pseudo code to mine transactions tx{1,2,3}:
        //
        // ```
        // loop {
        //   tx1 = random_tx()
        //   tx2 = random_tx()
        //   tx3 = deserialize_tx(txid(tx1) || txid(tx2));
        //   if serialized_siz(tx3) == 64 {
        //     print(hex(tx3))
        //     break
        //   }
        // }
        // ```
        //
        // The `random_tx` function used to mine the txs below simply created
        // empty transactions with a random version field.
        CMutableTransaction tx1;
        BOOST_CHECK(DecodeHexTx(tx1, "ff204bd0000000000000"));
        CMutableTransaction tx2;
        BOOST_CHECK(DecodeHexTx(tx2, "8ae53c92000000000000"));
        CMutableTransaction tx3;
        BOOST_CHECK(DecodeHexTx(
            tx3,
            "cdaf22d00002c6a7f848f8ae4d30054e61dcf3303d6fe01d282163341f06feecc1"
            "0032b3160fcab87bdfe3ecfb769206ef2d991b92f8a268e423a6ef4d485f06"));
        {
            // Verify that double_sha256(txid1||txid2) == txid3
            HashWriter hasher;
            hasher << tx1.GetId();
            hasher << tx2.GetId();
            assert(hasher.GetHash() == tx3.GetHash());
            // Verify that tx3 is 64 bytes in size.
            assert(GetSerializeSize(tx3, PROTOCOL_VERSION) == 64);
        }

        CBlock block;
        block.vtx.push_back(MakeTransactionRef(tx1));
        block.vtx.push_back(MakeTransactionRef(tx2));
        uint256 merkle_root = block.hashMerkleRoot = BlockMerkleRoot(block);
        BOOST_CHECK(is_not_mutated(block));

        // Mutate the block by replacing the two transactions with one 64-byte
        // transaction that serializes into the concatenation of the txids of
        // the transactions in the unmutated block.
        block.vtx.clear();
        block.vtx.push_back(MakeTransactionRef(tx3));
        BOOST_CHECK(!block.vtx.back()->IsCoinBase());
        BOOST_CHECK(BlockMerkleRoot(block) == merkle_root);
        BOOST_CHECK(is_mutated(block));
    }
}

BOOST_AUTO_TEST_SUITE_END()
