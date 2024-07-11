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
    // Except for legacy blocks with full version 1, ensure that the chain ID is
    // correct. Legacy blocks are not allowed since the merge-mining start,
    // which is checked in AcceptBlockHeader where the height is known.
    if (!block.HasLegacyVersion() && params.enforceStrictAuxPowChainId &&
        block.GetChainId() != AUXPOW_CHAIN_ID) {
        return error("%s: block does not have our chain ID (got %x, expected "
                     "%x, full nVersion %x)",
                     __func__, block.GetChainId(), AUXPOW_CHAIN_ID,
                     block.nVersion);
    }

    // If there is no auxpow, just check the block hash.
    if (!block.auxpow) {
        if (block.HasAuxPowVersion()) {
            return error("%s: no auxpow on block %s with auxpow version %08x",
                         __func__, block.GetHash().ToString(), block.nVersion);
        }

        if (!CheckProofOfWork(block.GetPowHash(), block.nBits, params)) {
            return error("%s: non-AUX proof of work failed", __func__);
        }

        return true;
    }

    if (!block.HasAuxPowVersion()) {
        // Header encodes auxpow, but version doesn't reflect it
        return error("%s: AuxPow on block with non-auxpow version", __func__);
    }

    if (!block.auxpow->check(block.GetHash(), block.GetChainId(), params)) {
        return error("%s: AuxPow validity check failed", __func__);
    }

    if (!CheckProofOfWork(BlockHash(block.auxpow->getParentBlockPowHash()),
                          block.nBits, params)) {
        return error("%s: Auxillary header proof of work failed", __func__);
    }

    return true;
}
