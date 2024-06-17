// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BASE_HEADER_H
#define BITCOIN_PRIMITIVES_BASE_HEADER_H

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
};

#endif // BITCOIN_PRIMITIVES_BASE_HEADER_H
