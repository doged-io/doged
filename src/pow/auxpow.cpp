// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/params.h>
#include <logging.h>
#include <pow/pow.h>
#include <primitives/block.h>

bool CheckAuxProofOfWork(const CBlockHeader &block,
                         const Consensus::Params &params) {
    if (!CheckProofOfWork(block.GetPowHash(), block.nBits, params)) {
        return error("%s : non-AUX proof of work failed", __func__);
    }

    return true;
}
