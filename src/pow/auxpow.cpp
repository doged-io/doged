// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/params.h>
#include <logging.h>
#include <pow/pow.h>
#include <primitives/auxpow.h>
#include <primitives/block.h>

bool CheckAuxProofOfWork(const CBlockHeader &block,
                         const Consensus::Params &params) {
    // Except for legacy blocks with full version 1 or 2, ensure that the chain
    // ID is correct. Legacy blocks are not allowed since the merge-mining
    // start, which is checked in AcceptBlockHeader where the height is known.
    if (params.enforceStrictAuxPowChainId && !VersionIsLegacy(block.nVersion) &&
        VersionChainId(block.nVersion) != AUXPOW_CHAIN_ID) {
        return error("%s: block does not have our chain ID (got %x, expected "
                     "%x, full nVersion %x)",
                     __func__, VersionChainId(block.nVersion), AUXPOW_CHAIN_ID,
                     block.nVersion);
    }

    if (!CheckProofOfWork(block.GetPowHash(), block.nBits, params)) {
        return error("%s: non-AUX proof of work failed", __func__);
    }

    return true;
}
