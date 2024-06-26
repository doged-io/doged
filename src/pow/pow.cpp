// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow/pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <common/system.h>
#include <consensus/activation.h>
#include <consensus/params.h>
#include <primitives/blockhash.h>

// Dogecoin: Normally minimum difficulty blocks can only occur in between
// retarget blocks. However, once we introduce Digishield every block is
// a retarget, so we need to handle minimum difficulty on all blocks.
bool AllowDigishieldMinDifficultyForBlock(
    const CBlockIndex *pindexLast, const CBlockHeader *pblock,
    const Consensus::Params &params, const Consensus::DaaParams &daaParams) {
    // check if the chain allows minimum difficulty blocks
    if (!daaParams.fPowAllowMinDifficultyBlocks) {
        return false;
    }

    // Allow for a minimum block time if the elapsed time > 2*nTargetSpacing
    return (pblock->GetBlockTime() >
            pindexLast->GetBlockTime() + params.nPowTargetSpacing * 2);
}

uint32_t GetNextWorkRequired(const CBlockIndex *pindexPrev,
                             const CBlockHeader *pblock,
                             const CChainParams &chainParams) {
    // GetNextWorkRequired should never be called on the genesis block
    assert(pindexPrev != nullptr);

    const Consensus::Params &params = chainParams.GetConsensus();

    // Special rule for regtest: we never retarget.
    if (params.fPowNoRetargeting) {
        return pindexPrev->nBits;
    }

    unsigned int nProofOfWorkLimit =
        UintToArith256(params.powLimit).GetCompact();

    const int32_t nHeight = pindexPrev->nHeight;
    const Consensus::DaaParams daaParams = params.DaaParamsAtHeight(nHeight);

    // Dogecoin: Special rules for minimum difficulty blocks with Digishield
    if (AllowDigishieldMinDifficultyForBlock(pindexPrev, pblock, params,
                                             daaParams)) {
        // Special difficulty rule for testnet:
        // If the new block's timestamp is more than 2* nTargetSpacing minutes
        // then allow mining of a min-difficulty block.
        return nProofOfWorkLimit;
    }

    // Only change once per difficulty adjustment interval
    const int64_t defaultInterval =
        params.DifficultyAdjustmentInterval(daaParams);
    const int64_t difficultyAdjustmentInterval =
        daaParams.fDigishieldDifficultyCalculation ? 1 : defaultInterval;
    if ((nHeight + 1) % difficultyAdjustmentInterval != 0) {
        if (daaParams.fPowAllowMinDifficultyBlocks) {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() >
                pindexPrev->GetBlockTime() + params.nPowTargetSpacing * 2) {
                return nProofOfWorkLimit;
            } else {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex *pindex = pindexPrev;
                while (pindex->pprev &&
                       pindex->nHeight % defaultInterval != 0 &&
                       pindex->nBits == nProofOfWorkLimit) {
                    pindex = pindex->pprev;
                }
                return pindex->nBits;
            }
        }
        return pindexPrev->nBits;
    }

    // Litecoin: This fixes an issue where a 51% attack can change difficulty at
    // will. Go back the full period unless it's the first retarget after
    // genesis.
    int32_t blocksToGoBack = difficultyAdjustmentInterval - 1;
    if (nHeight + 1 != difficultyAdjustmentInterval) {
        blocksToGoBack = difficultyAdjustmentInterval;
    }

    // Go back by what we want to be 14 days worth of blocks
    int32_t nHeightFirst = pindexPrev->nHeight - blocksToGoBack;
    assert(nHeightFirst >= 0);

    const CBlockIndex *pindexFirst = pindexPrev->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    const int64_t retargetTimespan = daaParams.nPowTargetTimespan;
    const int64_t nActualTimespan =
        pindexPrev->GetBlockTime() - pindexFirst->GetBlockTime();
    int64_t nModulatedTimespan = nActualTimespan;

    if (daaParams.fDigishieldDifficultyCalculation) {
        // DigiShield implementation
        nModulatedTimespan =
            retargetTimespan + (nModulatedTimespan - retargetTimespan) / 8;
    }

    // Limit adjustment step
    if (nModulatedTimespan < daaParams.nMinTimespan) {
        nModulatedTimespan = daaParams.nMinTimespan;
    } else if (nModulatedTimespan > daaParams.nMaxTimespan) {
        nModulatedTimespan = daaParams.nMaxTimespan;
    }

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexPrev->nBits);
    bnOld = bnNew;
    bnNew *= nModulatedTimespan;
    bnNew /= retargetTimespan;

    if (bnNew > bnPowLimit) {
        bnNew = bnPowLimit;
    }

    return bnNew.GetCompact();
}

// Check that on difficulty adjustments, the new difficulty does not increase
// or decrease beyond the permitted limits.
bool PermittedDifficultyTransition(const Consensus::Params &params,
                                   int64_t height, uint32_t old_nbits,
                                   uint32_t new_nbits) {
    const Consensus::DaaParams daaParams = params.DaaParamsAtHeight(height - 1);

    if (daaParams.fPowAllowMinDifficultyBlocks || params.fPowNoRetargeting) {
        return true;
    }

    // Keeping the same difficulty as the prev block is always permitted,
    // assuming the initial difficulty was valid, so bail out early.
    // The initial difficulty is valid because we start from the genesis block
    // and we stop calling this function as soon as it returns false.
    // This avoids further computation for most blocks prior to DAA forks.
    if (old_nbits == new_nbits) {
        return true;
    }

    // Prior to Digishield, the difficulty could change only every 240 blocks,
    // so we can bail out early if we observe a difficulty change at an
    // unexpected block height.
    if (!IsDigishieldEnabled(params, height - 1) &&
        height % params.DifficultyAdjustmentInterval(daaParams) != 0) {
        return false;
    }

    // Check [0, powLimit] range for all DAA algorithms.
    bool fNegative;
    bool fOverflow;
    arith_uint256 observed_new_target;
    observed_new_target.SetCompact(new_nbits, &fNegative, &fOverflow);
    if (fNegative || observed_new_target == 0 || fOverflow ||
        observed_new_target > UintToArith256(params.powLimit)) {
        return false;
    }

    int64_t smallest_timespan = daaParams.nMinTimespan;
    int64_t largest_timespan = daaParams.nMaxTimespan;

    const arith_uint256 pow_limit = UintToArith256(params.powLimit);
    observed_new_target.SetCompact(new_nbits);

    // Calculate the largest difficulty value possible:
    arith_uint256 largest_difficulty_target;
    largest_difficulty_target.SetCompact(old_nbits);
    largest_difficulty_target *= largest_timespan;
    largest_difficulty_target /= daaParams.nPowTargetTimespan;

    if (largest_difficulty_target > pow_limit) {
        largest_difficulty_target = pow_limit;
    }

    // Round and then compare this new calculated value to what is observed.
    arith_uint256 maximum_new_target;
    maximum_new_target.SetCompact(largest_difficulty_target.GetCompact());
    if (maximum_new_target < observed_new_target) {
        return false;
    }

    // Calculate the smallest difficulty value possible:
    arith_uint256 smallest_difficulty_target;
    smallest_difficulty_target.SetCompact(old_nbits);
    smallest_difficulty_target *= smallest_timespan;
    smallest_difficulty_target /= daaParams.nPowTargetTimespan;

    if (smallest_difficulty_target > pow_limit) {
        smallest_difficulty_target = pow_limit;
    }

    // Round and then compare this new calculated value to what is observed.
    arith_uint256 minimum_new_target;
    minimum_new_target.SetCompact(smallest_difficulty_target.GetCompact());
    if (minimum_new_target > observed_new_target) {
        return false;
    }

    return true;
}

bool CheckProofOfWork(const BlockHash &hash, uint32_t nBits,
                      const Consensus::Params &params) {
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow ||
        bnTarget > UintToArith256(params.powLimit)) {
        return false;
    }

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget) {
        return false;
    }

    return true;
}
