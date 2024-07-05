// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2016 Daniel Kraft
// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_AUXPOW_H
#define BITCOIN_PRIMITIVES_AUXPOW_H

#include <primitives/baseheader.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

#include <vector>

class CBlockHeader;
namespace Consensus {
struct Params;
} // namespace Consensus

const int32_t AUXPOW_CHAIN_ID = 0x62;

/** 4-byte prefix for merge-mining data in the coinbase. */
static const std::array<uint8_t, 4> MERGE_MINE_PREFIX{{0xfa, 0xbe, 'm', 'm'}};

/**
 * Data for the merge-mining auxpow.  This is a merkle tx (the parent block's
 * coinbase tx) that can be verified to be in the parent block, and this
 * transaction's input (the coinbase script) contains the reference
 * to the actual merge-mined block.
 */
class CAuxPow {
public:
    CTransactionRef tx;
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int32_t nIndex;

    /** The merkle branch connecting the aux block to our coinbase. */
    std::vector<uint256> vChainMerkleBranch;

    /** Merkle tree index of the aux block header in the coinbase. */
    int32_t nChainIndex;

    /** Parent block header (on which the real PoW is done). */
    CBaseBlockHeader parentBlock;

public:
    /* Prevent accidental conversion. */
    inline explicit CAuxPow(CTransactionRef txIn) : tx(txIn) {}

    inline CAuxPow() {}

    SERIALIZE_METHODS(CAuxPow, obj) {
        READWRITE(obj.tx, obj.hashBlock, obj.vMerkleBranch, obj.nIndex,
                  obj.vChainMerkleBranch, obj.nChainIndex, obj.parentBlock);
    }

    /**
     * Check the auxpow, given the merge-mined block's hash and our chain ID.
     * Note that this does not verify the actual PoW on the parent block!  It
     * just confirms that all the merkle branches are valid.
     * @param hashAuxBlock Hash of the merge-mined block.
     * @param nChainId The auxpow chain ID of the block to check.
     * @param params Consensus parameters.
     * @return True if the auxpow is valid.
     */
    bool check(const uint256 &hashAuxBlock, int32_t nChainId,
               const Consensus::Params &params) const;

    /**
     * Get the parent block's hash.  This is used to verify that it
     * satisfies the PoW requirement.
     * @return The parent block hash.
     */
    inline BlockHash getParentBlockPowHash() const {
        return parentBlock.GetPowHash();
    }

    /**
     * Calculate the expected index in the merkle tree.
     * @param nNonce The coinbase's nonce value.
     * @param nChainId The chain ID.
     * @param merkleHeight The merkle block height.
     * @return The expected index for the aux hash.
     */
    static int32_t getExpectedIndex(uint32_t nNonce, int32_t nChainId,
                                    uint32_t merkleHeight);

    /**
     * Calc the root of a merkle branch.
     */
    static uint256 CalcMerkleBranch(uint256 hash,
                                    const std::vector<uint256> &vMerkleBranch,
                                    int32_t nIndex);
};

#endif // BITCOIN_PRIMITIVES_AUXPOW_H
