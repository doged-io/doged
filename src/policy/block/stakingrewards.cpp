// Copyright (c) 2023 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/block/stakingrewards.h>

#include <avalanche/avalanche.h>
#include <avalanche/processor.h>
#include <blockindex.h>
#include <consensus/amount.h>
#include <logging.h>
#include <primitives/block.h>

#include <vector>

/**
 * Percentage of the block reward to be sent to staking rewards.
 * FIXME This is a placeholder for now and the current ratio is for testing
 * purpose only.
 */
static constexpr int STAKING_REWARD_RATIO = 25;

bool StakingRewardsPolicy::operator()(BlockPolicyValidationState &state) {
    if (!m_blockIndex.pprev) {
        return true;
    }

    assert(m_block.vtx.size());

    const BlockHash blockhash = m_blockIndex.GetBlockHash();

    CScript winner;
    if (!g_avalanche || !isAvalancheEnabled(gArgs) ||
        !gArgs.GetBoolArg("-avalanchestakingrewards",
                          AVALANCHE_DEFAULT_STAKING_REWARDS) ||
        !g_avalanche->getStakingRewardWinner(m_blockIndex.pprev->GetBlockHash(),
                                             winner)) {
        LogPrint(BCLog::AVALANCHE,
                 "Staking rewards for block %s: not ready yet\n",
                 blockhash.ToString());
        return true;
    }

    const Amount required = GetStakingRewardsAmount(m_blockReward);
    for (auto &o : m_block.vtx[0]->vout) {
        if (o.nValue < required) {
            // This output doesn't qualify because its amount is too low.
            continue;
        }

        if (o.scriptPubKey == winner) {
            return true;
        }
    }

    LogPrint(BCLog::AVALANCHE,
             "Staking rewards for block %s: payout script mismatch!\n",
             blockhash.ToString());

    return state.Invalid(BlockPolicyValidationResult::POLICY_VIOLATION,
                         "policy-bad-staking-reward",
                         strprintf("Block %s violates staking reward policy",
                                   m_blockIndex.GetBlockHash().ToString()));
}

Amount GetStakingRewardsAmount(const Amount &coinbaseValue) {
    return STAKING_REWARD_RATIO * coinbaseValue / 100;
}
