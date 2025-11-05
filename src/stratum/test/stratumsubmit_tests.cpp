// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumsubmit.h>

#include <arith_uint256.h>
#include <crypto/scrypt.h>
#include <primitives/auxpow.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <set>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumsubmit_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(parse_submit_valid) {
    UniValue params(UniValue::VARR);
    params.push_back("worker.1");
    params.push_back("1a");
    params.push_back("00000000");
    params.push_back("5f3c2e1a");
    params.push_back("deadbeef");

    auto result = ParseSubmitParams(params);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK_EQUAL(result->workerName, "worker.1");
    BOOST_CHECK_EQUAL(result->jobId, 0x1a);
    BOOST_CHECK_EQUAL(result->extranonce2, "00000000");
    BOOST_CHECK_EQUAL(result->ntime, "5f3c2e1a");
    BOOST_CHECK_EQUAL(result->nonce, "deadbeef");
}

BOOST_AUTO_TEST_CASE(parse_submit_missing_fields) {
    UniValue params(UniValue::VARR);
    params.push_back("worker");
    params.push_back("1");

    auto result = ParseSubmitParams(params);
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(parse_submit_invalid_hex) {
    UniValue params(UniValue::VARR);
    params.push_back("worker");
    params.push_back("1");
    params.push_back("00000000");
    params.push_back("5f3c2e1a");
    params.push_back("ZZZZZZZZ"); // not hex

    auto result = ParseSubmitParams(params);
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(parse_submit_wrong_extranonce2_length) {
    UniValue params(UniValue::VARR);
    params.push_back("worker");
    params.push_back("1");
    params.push_back("0000");      // too short, expected 8 chars
    params.push_back("5f3c2e1a");
    params.push_back("deadbeef");

    auto result = ParseSubmitParams(params);
    BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(difficulty_to_target_one) {
    arith_uint256 target = DifficultyToTarget(1.0);
    arith_uint256 diff1;
    diff1.SetCompact(0x1d00ffff);
    BOOST_CHECK(target == diff1);
}

BOOST_AUTO_TEST_CASE(difficulty_to_target_high) {
    arith_uint256 target1 = DifficultyToTarget(1.0);
    arith_uint256 target1024 = DifficultyToTarget(1024.0);
    // Higher difficulty → lower target
    BOOST_CHECK(target1024 < target1);
    // Approximately 1024x smaller
    arith_uint256 ratio = target1 / target1024;
    BOOST_CHECK(ratio >= 1000 && ratio <= 1048);
}

BOOST_AUTO_TEST_CASE(scrypt_pow_hash_differs_from_sha256) {
    CBaseBlockHeader header;
    header.nVersion = 0x00620004;
    header.hashPrevBlock = BlockHash();
    header.hashMerkleRoot.SetNull();
    header.nTime = 1234567890;
    header.nBits = 0x1d00ffff;
    header.nNonce = 42;

    BlockHash powHash = ScryptPowHash(header);
    BlockHash sha256Hash = header.GetHash();

    // Scrypt PoW hash must differ from SHA-256d block hash
    BOOST_CHECK(powHash != sha256Hash);
}

BOOST_AUTO_TEST_CASE(validate_share_rejected_duplicate) {
    // Create a job from the test chain
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    auto jobResult = mgr.CreateJob(true);
    BOOST_REQUIRE(jobResult.has_value());

    ShareSubmission sub;
    sub.workerName = "test";
    sub.jobId = jobResult->jobId;
    sub.extranonce2 = "00000000";
    sub.ntime = jobResult->ntime;
    sub.nonce = "deadbeef";

    std::set<std::string> nonces;
    const auto &consensus = m_node.chainman->GetParams().GetConsensus();

    // First submission (will be low diff but not duplicate)
    ValidateShare(*jobResult, "00000001", sub, 0.001, consensus, nonces);

    // Same nonce again → duplicate
    ShareResult result =
        ValidateShare(*jobResult, "00000001", sub, 0.001, consensus, nonces);
    BOOST_CHECK(result == ShareResult::REJECTED_DUPLICATE);
}

BOOST_AUTO_TEST_SUITE_END()
