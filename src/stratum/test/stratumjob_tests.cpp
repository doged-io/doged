// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumjob.h>

#include <primitives/auxpow.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace stratum;

BOOST_FIXTURE_TEST_SUITE(stratumjob_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(hash_to_stratum_hex_length) {
    uint256 hash;
    hash.SetNull();
    std::string hex = HashToStratumHex(hash);
    BOOST_CHECK_EQUAL(hex.size(), 64u);
}

BOOST_AUTO_TEST_CASE(uint32_to_stratum_hex_format) {
    // 0x12345678 in little-endian hex = "78563412"
    std::string hex = Uint32ToStratumHex(0x12345678);
    BOOST_CHECK_EQUAL(hex, "78563412");
}

BOOST_AUTO_TEST_CASE(uint32_to_stratum_hex_zero) {
    std::string hex = Uint32ToStratumHex(0);
    BOOST_CHECK_EQUAL(hex, "00000000");
}

BOOST_AUTO_TEST_CASE(create_job_returns_valid) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    auto result = mgr.CreateJob(true);
    BOOST_REQUIRE(result.has_value());
    BOOST_CHECK(result->jobId > 0);
    BOOST_CHECK(!result->coinbase1.empty());
    BOOST_CHECK(!result->coinbase2.empty());
    BOOST_CHECK(!result->prevHash.empty());
    BOOST_CHECK(!result->version.empty());
    BOOST_CHECK(!result->nbits.empty());
    BOOST_CHECK(!result->ntime.empty());
    BOOST_CHECK(result->cleanJobs);
}

BOOST_AUTO_TEST_CASE(job_version_has_chain_id) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    auto result = mgr.CreateJob(true);
    BOOST_REQUIRE(result.has_value());

    // Chain ID should be 0x62 in bits 16-31
    BOOST_CHECK_EQUAL(VersionChainId(result->nVersionRaw), AUXPOW_CHAIN_ID);
}

BOOST_AUTO_TEST_CASE(job_version_no_auxpow_flag) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    auto result = mgr.CreateJob(true);
    BOOST_REQUIRE(result.has_value());

    // Direct mining template should NOT have AuxPoW flag set
    BOOST_CHECK(!VersionHasAuxPow(result->nVersionRaw));
}

BOOST_AUTO_TEST_CASE(notify_params_format) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    auto result = mgr.CreateJob(true);
    BOOST_REQUIRE(result.has_value());

    UniValue params = mgr.FormatNotifyParams(*result);
    BOOST_CHECK(params.isArray());
    BOOST_CHECK_EQUAL(params.size(), 9u);

    // params[0] = jobId (string)
    BOOST_CHECK(params[0].isStr());
    // params[1] = prevHash (64 hex chars)
    BOOST_CHECK_EQUAL(params[1].get_str().size(), 64u);
    // params[4] = merkle branches (array)
    BOOST_CHECK(params[4].isArray());
    // params[5] = version (8 hex chars)
    BOOST_CHECK_EQUAL(params[5].get_str().size(), 8u);
    // params[6] = nbits (8 hex chars)
    BOOST_CHECK_EQUAL(params[6].get_str().size(), 8u);
    // params[7] = ntime (8 hex chars)
    BOOST_CHECK_EQUAL(params[7].get_str().size(), 8u);
    // params[8] = clean_jobs (bool)
    BOOST_CHECK(params[8].isBool());
}

BOOST_AUTO_TEST_CASE(get_job_by_id) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    auto result = mgr.CreateJob(false);
    BOOST_REQUIRE(result.has_value());

    const StratumJob *found = mgr.GetJob(result->jobId);
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK_EQUAL(found->jobId, result->jobId);
}

BOOST_AUTO_TEST_CASE(get_job_unknown_id) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    BOOST_CHECK(mgr.GetJob(99999) == nullptr);
}

BOOST_AUTO_TEST_CASE(prune_jobs) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    for (int i = 0; i < 10; i++) {
        mgr.CreateJob(false);
    }
    BOOST_CHECK_EQUAL(mgr.JobCount(), 10u);

    mgr.PruneJobs(5);
    BOOST_CHECK_EQUAL(mgr.JobCount(), 5u);
}

BOOST_AUTO_TEST_CASE(sequential_job_ids) {
    CScript scriptPubKey = CScript() << OP_TRUE;
    StratumJobManager mgr(m_node.chainman->ActiveChainstate(),
                          m_node.mempool.get(),
                          m_node.chainman->GetParams(),
                          scriptPubKey, 4, 4);

    auto r1 = mgr.CreateJob(false);
    auto r2 = mgr.CreateJob(false);
    BOOST_REQUIRE(r1.has_value() && r2.has_value());
    BOOST_CHECK(r2->jobId > r1->jobId);
}

BOOST_AUTO_TEST_SUITE_END()
