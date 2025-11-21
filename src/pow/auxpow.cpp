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
        LogError("%s: block does not have our chain ID (got %x, expected "
                 "%x, full nVersion %x)",
                 __func__, VersionChainId(block.nVersion), AUXPOW_CHAIN_ID,
                 block.nVersion);
        return false;
    }

    // If there is no auxpow, just check the block hash.
    if (!block.auxpow) {
        if (VersionHasAuxPow(block.nVersion)) {
            LogError("%s: no auxpow on block %s with auxpow version %08x",
                     __func__, block.GetHash().ToString(), block.nVersion);
            return false;
        }

        if (!CheckProofOfWork(block.GetPowHash(), block.nBits, params)) {
            LogError("%s: non-AUX proof of work failed", __func__);
            return false;
        }

        return true;
    }

    if (!VersionHasAuxPow(block.nVersion)) {
        // Header encodes auxpow, but version doesn't reflect it
        LogError("%s: AuxPow on block with non-auxpow version", __func__);
        return false;
    }

    util::Result<std::monostate> auxResult = block.auxpow->CheckAuxBlockHash(
        block.GetHash(), VersionChainId(block.nVersion), params);
    if (!auxResult) {
        LogError("%s: AuxPow validity check failed: %s", __func__,
                 ErrorString(auxResult).original);
        return false;
    }

    if (!CheckProofOfWork(BlockHash(block.auxpow->parentBlock.GetPowHash()),
                          block.nBits, params)) {
        LogError("%s: Auxillary header proof of work failed", __func__);
        return false;
    }

    return true;
}
