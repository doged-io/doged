// Copyright (c) 2023 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <avalanche/avalanche.h>
#include <avalanche/processor.h>
#include <avalanche/stakecontender.h>
#include <chainparams.h>
#include <consensus/activation.h>
#include <net_processing.h>
#include <policy/block/stakingrewards.h>
#include <validation.h>

#include <test/util/blockindex.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace avalanche;

struct StakingRewardsActivationTestingSetup : public TestingSetup {
    void checkStakingRewardsActivation(const std::string &net,
                                       const bool expectActivation) {
        SelectParams(net);
        const Consensus::Params &params = Params().GetConsensus();

        // Before Cowperthwaite activation
        const auto activationHeight = params.cowperthwaiteHeight;

        CBlockIndex block;
        block.nHeight = activationHeight - 1;
        BOOST_CHECK(!IsStakingRewardsActivated(params, &block));

        block.nHeight = activationHeight;
        BOOST_CHECK_EQUAL(IsStakingRewardsActivated(params, &block),
                          expectActivation);

        block.nHeight = activationHeight + 1;
        BOOST_CHECK_EQUAL(IsStakingRewardsActivated(params, &block),
                          expectActivation);
    }
};

BOOST_AUTO_TEST_SUITE(stakingrewards_tests)

BOOST_FIXTURE_TEST_CASE(isstakingrewardsactivated,
                        StakingRewardsActivationTestingSetup) {
    checkStakingRewardsActivation("regtest", false);
    checkStakingRewardsActivation("test", false);

    Assert(m_node.args)->ForceSetArg("-avalanchestakingrewards", "1");
    checkStakingRewardsActivation("main", true);
    Assert(m_node.args)->ClearForcedArg("-avalanchestakingrewards");
}

BOOST_FIXTURE_TEST_CASE(stakecontender_computeid, TestChain100Setup) {
    ChainstateManager &chainman = *Assert(m_node.chainman);
    const CBlockIndex *chaintip =
        WITH_LOCK(chainman.GetMutex(), return chainman.ActiveTip());

    ProofId proofid1 = ProofId::fromHex(
        "979dbc3b1351ee12f91f537e04e61fdf93a73d5ebfc317bccd12643b8be87b02");
    BOOST_CHECK_EQUAL(
        "1f40d5f66439e5ba739b4c03c33a2c699e01fde1ae69f3de2e90315f7d8fedca",
        StakeContenderId(chaintip->GetAncestor(0)->GetBlockHash(), proofid1)
            .ToString());

    // Different prevblock should give different hash
    BOOST_CHECK_EQUAL(
        "1a3f06557dfb401aba65e3dcd90e2bd8fa505f74b4366f7d18a84bf3d843a749",
        StakeContenderId(chaintip->GetBlockHash(), proofid1).ToString());

    // So should a different proof id
    ProofId proofid2 = ProofId::fromHex(
        "e01bac293ed39e8d5e06214e7fe0bceb9646ef253ce501dcd7a475f802ab07f1");
    BOOST_CHECK_EQUAL(
        "5247e2093d593ff1301b6a1e636637b71ceb21943deae596885c1d4f501ebb93",
        StakeContenderId(chaintip->GetBlockHash(), proofid2).ToString());
}

BOOST_AUTO_TEST_SUITE_END()
