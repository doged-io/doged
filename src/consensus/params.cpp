// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/activation.h>

#include <consensus/params.h>

namespace Consensus {

DaaParams Params::DaaParamsAtHeight(int32_t nHeight) const {
    DaaParams daaParams;

    const bool hasDigishield = IsDigishieldEnabled(*this, nHeight);

    daaParams.fPowAllowMinDifficultyBlocks = enableTestnetMinDifficulty;
    // Blocks 145000 - 157499 have fPowAllowMinDifficultyBlocks disabled
    if (enableTestnetMinDifficulty && nHeight >= 145000 && nHeight < 157500) {
        daaParams.fPowAllowMinDifficultyBlocks = false;
    }

    daaParams.fDigishieldDifficultyCalculation = hasDigishield;

    daaParams.nPowTargetTimespan = hasDigishield ? 60 : 4 * 60 * 60;

    if (hasDigishield) {
        daaParams.nMinTimespan =
            daaParams.nPowTargetTimespan - (daaParams.nPowTargetTimespan / 4);
        daaParams.nMaxTimespan =
            daaParams.nPowTargetTimespan + (daaParams.nPowTargetTimespan / 2);
    } else if (nHeight > 10000) {
        daaParams.nMinTimespan = daaParams.nPowTargetTimespan / 4;
        daaParams.nMaxTimespan = daaParams.nPowTargetTimespan * 4;
    } else if (nHeight > 5000) {
        daaParams.nMinTimespan = daaParams.nPowTargetTimespan / 8;
        daaParams.nMaxTimespan = daaParams.nPowTargetTimespan * 4;
    } else {
        daaParams.nMinTimespan = daaParams.nPowTargetTimespan / 16;
        daaParams.nMaxTimespan = daaParams.nPowTargetTimespan * 4;
    }

    return daaParams;
}

} // namespace Consensus
