// Copyright (c) 2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/activation.h>
#include <util/system.h>

#include <test/util/blockindex.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(activation_tests, BasicTestingSetup)

static void testPastActivation(
    std::function<bool(const Consensus::Params &, const CBlockIndex *)> func,
    const Consensus::Params &params, int activationHeight) {
    BOOST_CHECK(!func(params, nullptr));

    std::array<CBlockIndex, 4> blocks;
    blocks[0].nHeight = activationHeight - 2;
    for (size_t i = 1; i < blocks.size(); ++i) {
        blocks[i].pprev = &blocks[i - 1];
        blocks[i].nHeight = blocks[i - 1].nHeight + 1;
    }

    BOOST_CHECK(!func(params, &blocks[0]));
    BOOST_CHECK(!func(params, &blocks[1]));
    BOOST_CHECK(func(params, &blocks[2]));
    BOOST_CHECK(func(params, &blocks[3]));
}

BOOST_AUTO_TEST_CASE(test_previous_activations_by_height) {
    const auto params = CreateChainParams(CBaseChainParams::MAIN);
    const auto consensus = params->GetConsensus();

    testPastActivation(IsGravitonEnabled, consensus, consensus.gravitonHeight);
    testPastActivation(IsPhononEnabled, consensus, consensus.phononHeight);
    testPastActivation(IsAxionEnabled, consensus, consensus.axionHeight);
    testPastActivation(IsGluonEnabled, consensus, consensus.gluonHeight);
}

BOOST_AUTO_TEST_CASE(iswellingtonenabled) {
    const Consensus::Params &params = Params().GetConsensus();
    const auto activation = gArgs.GetIntArg("-wellingtonactivationtime",
                                            params.wellingtonActivationTime);
    SetMockTime(activation - 1000000);

    BOOST_CHECK(!IsWellingtonEnabled(params, nullptr));

    std::array<CBlockIndex, 12> blocks;
    for (size_t i = 1; i < blocks.size(); ++i) {
        blocks[i].pprev = &blocks[i - 1];
    }
    BOOST_CHECK(!IsWellingtonEnabled(params, &blocks.back()));

    SetMTP(blocks, activation - 1);
    BOOST_CHECK(!IsWellingtonEnabled(params, &blocks.back()));

    SetMTP(blocks, activation);
    BOOST_CHECK(IsWellingtonEnabled(params, &blocks.back()));

    SetMTP(blocks, activation + 1);
    BOOST_CHECK(IsWellingtonEnabled(params, &blocks.back()));
}

BOOST_AUTO_TEST_SUITE_END()
