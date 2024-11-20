// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_AUXPOW_H
#define BITCOIN_PRIMITIVES_AUXPOW_H

#include <cstdint>
#include <primitives/baseheader.h>
#include <primitives/transaction.h>
#include <util/result.h>

/** Bit that indicates a block has auxillary PoW. Bits below that are
 * interpreted as the "traditional" Bitcoin version. */
static constexpr int32_t VERSION_AUXPOW_BIT_POS = 8;
static constexpr int32_t VERSION_AUXPOW_BIT = 1 << VERSION_AUXPOW_BIT_POS;

/** Position of the bits reserved for the auxpow chain ID. */
static constexpr int32_t VERSION_CHAIN_ID_BIT_POS = 16;

/** Chain ID used by Dogecoin. */
static constexpr uint32_t AUXPOW_CHAIN_ID = 0x62;

/** Max allowed chain ID */
static constexpr uint32_t MAX_ALLOWED_CHAIN_ID =
    (1 << (32 - VERSION_CHAIN_ID_BIT_POS)) - 1;

/** 4-byte prefix for merge-mining data in the coinbase. */
static const std::array<uint8_t, 4> MERGE_MINE_PREFIX{{0xfa, 0xbe, 'm', 'm'}};

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
 * Like ComputeMerkleRoot, but where we have the leaf hash, the merkle branch
 * and the index of the leaf in the tree given.
 * Can be used to verify a merkle proof, by comparing the result to the expected
 * merkle root.
 */
uint256 ComputeMerkleRootForBranch(uint256 hash,
                                   const std::vector<uint256> &vMerkleBranch,
                                   uint32_t nIndex);

/*
 * Choose a pseudo-random slot in the chain merkle tree but have it be fixed for
 * a size/nonce/chain combination.
 */
uint32_t CalcExpectedMerkleTreeIndex(uint32_t nNonce, uint32_t nChainId,
                                     uint32_t merkleHeight);

/** Parsed data from a AuxPow coinbase */
class ParsedAuxPowCoinbase {
public:
    uint32_t nTreeSize;
    uint32_t nMergeMineNonce;

    /**
     * Parse a coinbase of another blockchain for AuxPow data, which searches
     * for the root hash, with one of two kinds of encodings:
     *
     * - With prefix:
     *   FABE6D6D<hashRoot:uint256><nTreeSize:uint32><nNonce:uint32>
     * - Without prefix:
     *   <hashRoot:uint256><nTreeSize:uint32><nNonce:uint32>
     *
     * Also, there's some additional rules:
     * - The root hash is encoded in big-endian
     * - The prefix can only occur at most once
     * - If there's no prefix, the root hash can have at most 20 bytes preceding
     *   it (Note: The Dogecoin source claims the root hash "must start in the
     *   first 20 bytes", but this doesn't match the code).
     */
    static util::Result<ParsedAuxPowCoinbase>
    Parse(const CScript &scriptCoinbase, uint256 hashRoot);
};

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
    /** Index of the tx in the block, must always be 0 (i.e. coinbase). */
    uint32_t nIndex;

    /** The merkle branch connecting the aux block to our coinbase. */
    std::vector<uint256> vChainMerkleBranch;

    /** Merkle tree index of the aux block header in the coinbase. */
    uint32_t nChainIndex;

    /** Parent block header (on which the real PoW is done). */
    CBaseBlockHeader parentBlock;

    CAuxPow() {}

    SERIALIZE_METHODS(CAuxPow, obj) {
        READWRITE(obj.coinbaseTx, obj.hashBlock, obj.vMerkleBranch, obj.nIndex,
                  obj.vChainMerkleBranch, obj.nChainIndex, obj.parentBlock);
    }
};

#endif // BITCOIN_PRIMITIVES_AUXPOW_H
