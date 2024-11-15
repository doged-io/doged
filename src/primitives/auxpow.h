// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_AUXPOW_H
#define BITCOIN_PRIMITIVES_AUXPOW_H

#include <cstdint>
#include <primitives/baseheader.h>
#include <primitives/transaction.h>

/** Bit that indicates a block has auxillary PoW. Bits below that are
 * interpreted as the "traditional" Bitcoin version. */
static constexpr int32_t VERSION_AUXPOW_BIT_POS = 8;
static constexpr int32_t VERSION_AUXPOW_BIT = 1 << VERSION_AUXPOW_BIT_POS;

/** Position of the bits reserved for the auxpow chain ID. */
static constexpr int32_t VERSION_CHAIN_ID_BIT_POS = 16;

/** Chain ID used by Dogecoin. */
static constexpr int32_t AUXPOW_CHAIN_ID = 0x62;

/** Max allowed chain ID */
static constexpr uint32_t MAX_ALLOWED_CHAIN_ID =
    (1 << (32 - VERSION_CHAIN_ID_BIT_POS)) - 1;

/**
 * Build version bits from the given parameters, with AuxPow disabled.
 * @param nChainId The auxpow chain ID.
 * @param nLowVersionBits The low version bits below the AuxPow flag.
 */
int32_t MakeVersionWithChainId(uint32_t nChainId, uint32_t nLowVersionBits);

/**
 * Set the AuxPow flag bit in the nVersion.
 * @param nVersion A block's nVersion.
 * @param hasAuxPow Whether the AuxPow flag should be set.
 */
int32_t VersionWithAuxPow(uint32_t nVersion, bool hasAuxPow);

/**
 * Extract the low version bits, which are interpreted as the "traditional"
 * Bitcoin version. The upper bits are used to signal presence of AuxPow and
 * to set the chain ID.
 */
inline uint32_t VersionLowBits(int32_t nVersion) {
    return nVersion & (VERSION_AUXPOW_BIT - 1);
}

/**
 * Extract the chain ID from the nVersion.
 */
inline uint32_t VersionChainId(int32_t nVersion) {
    return uint32_t(nVersion) >> VERSION_CHAIN_ID_BIT_POS;
}

/**
 * Check if the auxpow flag is set in nVersion.
 */
inline bool VersionHasAuxPow(int32_t nVersion) {
    return nVersion & VERSION_AUXPOW_BIT;
}

/**
 * Check whether this is a "legacy" block without chain ID.
 */
inline bool VersionIsLegacy(int32_t nVersion) {
    return nVersion == 1
           // Dogecoin: We have a random v2 block with no AuxPoW, treat as
           // legacy
           || nVersion == 2;
}

/**
 * Data for the merge-mining auxpow. This is a merkle tx (the parent block's
 * coinbase tx) that can be verified to be in the parent block, and this
 * transaction's input (the coinbase script) contains the reference
 * to the actual merge-mined block.
 */
class CAuxPow {
public:
    /** The coinbase tx of the parent block encoding the merge-mined block */
    CTransactionRef coinbaseTx;
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int32_t nIndex;

    /** The merkle branch connecting the aux block to our coinbase. */
    std::vector<uint256> vChainMerkleBranch;

    /** Merkle tree index of the aux block header in the coinbase. */
    int32_t nChainIndex;

    /** Parent block header (on which the real PoW is done). */
    CBaseBlockHeader parentBlock;

    CAuxPow() {}

    SERIALIZE_METHODS(CAuxPow, obj) {
        READWRITE(obj.coinbaseTx, obj.hashBlock, obj.vMerkleBranch, obj.nIndex,
                  obj.vChainMerkleBranch, obj.nChainIndex, obj.parentBlock);
    }
};

#endif // BITCOIN_PRIMITIVES_AUXPOW_H
