// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BASEHEADER_H
#define BITCOIN_PRIMITIVES_BASEHEADER_H

#include <primitives/blockhash.h>
#include <serialize.h>
#include <uint256.h>
#include <util/time.h>

/**
 * Dogecoin specific: A normal Bitcoin header without auxpow information, for
 * merge-mining.
 *
 * This "intermediate step" in constructing the full header is useful, because
 * it breaks the cyclic dependency between auxpow (referencing a parent block
 * header) and the block header (referencing an auxpow). The parent block
 * header does not have auxpow itself, so it is a "base" header.
 *
 * In dogecoin, this is called CPureBlockHeader.
 */
class CBaseBlockHeader {
public:
    /** Bit that indicates a block has auxillary PoW. Bits below that are
     * interpreted as the "traditional" Bitcoin version. */
    static const int32_t VERSION_AUXPOW_FLAG = (1 << 8);

    /** Bits including and above are reserved for the auxpow chain ID. */
    static const int32_t VERSION_CHAIN_ID_FIRST_BIT = 16;

    // header
    int32_t nVersion;
    BlockHash hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CBaseBlockHeader() { SetNull(); }

    SERIALIZE_METHODS(CBaseBlockHeader, obj) {
        READWRITE(obj.nVersion, obj.hashPrevBlock, obj.hashMerkleRoot,
                  obj.nTime, obj.nBits, obj.nNonce);
    }

    void SetNull() {
        nVersion = 0;
        hashPrevBlock = BlockHash();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const { return (nBits == 0); }

    // In Dogecoin, "block hash" and "PoW hash" are two separate hashes:

    /** "Block hash" is using double SHA-256 and used as the unique identifier
     * of the block, but it doesn't have any PoW done on it. */
    BlockHash GetHash() const;

    /** "PoW hash" is using Scrypt and miners have to solve this hash to be
     * below the target. */
    BlockHash GetPowHash() const;

    NodeSeconds Time() const {
        return NodeSeconds{std::chrono::seconds{nTime}};
    }

    int64_t GetBlockTime() const { return (int64_t)nTime; }

    /**
     * Extract the low version bits, which are interpreted as the "traditional"
     * Bitcoin version. The upper bits are used to signal presence of AuxPow and
     * to set the chain ID.
     */
    inline int32_t LowVersionBits() const {
        return LowBitsFromVersion(nVersion);
    }
    static inline int32_t LowBitsFromVersion(int32_t ver) {
        return ver % VERSION_AUXPOW_FLAG;
    }

    /**
     * Set the version bits and (low and chain ID) in nVersion.
     * Assumes HasAuxPowVersion() is false; should be used for initialization.
     * @param nLowVersionBits The low version bits below the AuxPow flag.
     * @param nChainId The auxpow chain ID.
     */
    void SetVersionBits(int32_t nLowVersionBits, int32_t nChainId);

    /**
     * Extract the chain ID from the nVersion.
     */
    inline int32_t GetChainId() const {
        return nVersion >> VERSION_CHAIN_ID_FIRST_BIT;
    }

    /**
     * Set the chain ID in the nVersion, keeping the other bits unchanged.
     * @param chainId The chain ID to set.
     */
    inline void SetChainId(int32_t chainId) {
        nVersion %= 1 << VERSION_CHAIN_ID_FIRST_BIT;
        nVersion |= chainId << VERSION_CHAIN_ID_FIRST_BIT;
    }

    /**
     * Check if the auxpow flag is set in nVersion.
     */
    inline bool HasAuxPowVersion() const {
        return nVersion & VERSION_AUXPOW_FLAG;
    }

    /**
     * Set the auxpow flag.
     * @param hasAuxPow Whether to mark auxpow as true.
     */
    inline void SetAuxPowVersion(bool hasAuxPow) {
        if (hasAuxPow) {
            nVersion |= VERSION_AUXPOW_FLAG;
        } else {
            nVersion &= ~VERSION_AUXPOW_FLAG;
        }
    }

    /**
     * Check whether this is a "legacy" block without chain ID.
     * @return True if it is.
     */
    inline bool HasLegacyVersion() const {
        return nVersion == 1
               // Dogecoin: We have a random v2 block with no AuxPoW, treat as
               // legacy
               || (nVersion == 2 && GetChainId() == 0);
    }
};

#endif // BITCOIN_PRIMITIVES_BASEHEADER_H
