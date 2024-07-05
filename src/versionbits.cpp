// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/params.h>
#include <primitives/auxpow.h>
#include <versionbits.h>

int32_t ComputeBlockVersion(const CBlockIndex *pindexPrev,
                            const Consensus::Params &params) {
    // Set valid auxpow version
    return (AUXPOW_CHAIN_ID << CBaseBlockHeader::VERSION_CHAIN_ID_FIRST_BIT) |
           0x04;
}
