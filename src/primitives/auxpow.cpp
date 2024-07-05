// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 Vince Durham
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2016 Daniel Kraft
// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/auxpow.h>

#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <logging.h>
#include <script/script.h>
#include <streams.h>

#include <algorithm>

bool CAuxPow::check(const uint256 &hashAuxBlock, int32_t nChainId,
                    const Consensus::Params &params) const {
    if (nIndex != 0) {
        return error("AuxPow nIndex must be 0");
    }

    if (params.enforceStrictAuxPowChainId &&
        parentBlock.GetChainId() == nChainId) {
        return error("AuxPow parent has our chain ID");
    }

    if (vChainMerkleBranch.size() > 30) {
        return error("AuxPow chain merkle branch too long");
    }

    // Check that the chain merkle root is in the coinbase
    uint256 nRootHash =
        CalcMerkleBranch(hashAuxBlock, vChainMerkleBranch, nChainIndex);
    // Root hash in coinbase scriptSig is big endian
    std::reverse(nRootHash.begin(), nRootHash.end());

    // Check that we are in the parent block merkle tree
    if (CalcMerkleBranch(tx->GetHash(), vMerkleBranch, nIndex) !=
        parentBlock.hashMerkleRoot) {
        return error("AuxPow merkle root incorrect");
    }

    const CScript coinbaseScript = tx->vin[0].scriptSig;

    // Check that the same work is not submitted twice to our chain.

    // Find the merge-mined prefix in the coinbase script
    CScript::const_iterator pcPrefix =
        std::search(coinbaseScript.begin(), coinbaseScript.end(),
                    MERGE_MINE_PREFIX.begin(), MERGE_MINE_PREFIX.end());

    // Find the root hash in the coinbase script
    CScript::const_iterator pcData =
        std::search(coinbaseScript.begin(), coinbaseScript.end(),
                    nRootHash.begin(), nRootHash.end());

    SpanReader mergeMineCbData(
        SER_NETWORK, PROTOCOL_VERSION,
        Span((uint8_t *)&*pcData, (uint8_t *)&*coinbaseScript.end()));

    // Make sure we found the root hash in the coinbase
    if (mergeMineCbData.size() == 0) {
        return error("AuxPow missing chain merkle root in parent coinbase");
    }

    if (pcPrefix != coinbaseScript.end()) {
        // Enforce only one chain merkle root by checking that a single instance
        // of the merged mining header exists just before.
        if (coinbaseScript.end() !=
            std::search(pcPrefix + 1, coinbaseScript.end(),
                        MERGE_MINE_PREFIX.begin(), MERGE_MINE_PREFIX.end())) {
            return error("Multiple merged mining headers in coinbase");
        }
        if (pcPrefix + MERGE_MINE_PREFIX.size() != pcData) {
            return error(
                "Merged mining header is not just before chain merkle root");
        }
    } else {
        // For backward compatibility: Merge-mine prefix not found.
        // Enforce only one chain merkle root by checking that it starts early
        // in the coinbase. 8-12 bytes are enough to encode extraNonce and
        // nBits.
        if (pcData - coinbaseScript.begin() > 20) {
            return error("AuxPow chain merkle root must start in the first 20 "
                         "bytes of the parent coinbase");
        }
    }

    // Ensure we are at a deterministic point in the merkle leaves by hashing
    // a nonce and our chain ID and comparing to the index.
    uint256 h;
    mergeMineCbData >> h;
    if (mergeMineCbData.size() < 8) {
        return error("AuxPow missing chain merkle tree size and nonce in "
                     "parent coinbase");
    }

    uint32_t nSize;
    mergeMineCbData >> nSize;
    const uint32_t merkleHeight = vChainMerkleBranch.size();
    if (nSize != (1U << merkleHeight)) {
        return error(
            "AuxPow merkle branch size does not match parent coinbase");
    }

    uint32_t nNonce;
    mergeMineCbData >> nNonce;
    if (nChainIndex != getExpectedIndex(nNonce, nChainId, merkleHeight)) {
        return error("AuxPow wrong index");
    }

    return true;
}

int32_t CAuxPow::getExpectedIndex(uint32_t nNonce, int32_t nChainId,
                                  uint32_t merkleHeight) {
    // Choose a pseudo-random slot in the chain merkle tree
    // but have it be fixed for a size/nonce/chain combination.
    //
    // This prevents the same work from being used twice for the
    // same chain while reducing the chance that two chains clash
    // for the same slot.

    /* This computation can overflow the uint32 used.  This is not an issue,
       though, since we take the mod against a power-of-two in the end anyway.
       This also ensures that the computation is, actually, consistent
       even if done in 64 bits as it was in the past on some systems.
       Note that h is always <= 30 (enforced by the maximum allowed chain
       merkle branch length), so that 32 bits are enough for the computation. */

    uint32_t rand = nNonce;
    rand = rand * 1103515245 + 12345;
    rand += nChainId;
    rand = rand * 1103515245 + 12345;

    return rand % (1 << merkleHeight);
}

uint256 CAuxPow::CalcMerkleBranch(uint256 hash,
                                  const std::vector<uint256> &vMerkleBranch,
                                  int nIndex) {
    if (nIndex == -1) {
        return uint256();
    }
    for (const uint256 &merkleHash : vMerkleBranch) {
        if (nIndex & 1) {
            hash = Hash(Span(merkleHash), Span(hash));
        } else {
            hash = Hash(Span(hash), Span(merkleHash));
        }
        nIndex >>= 1;
    }
    return hash;
}
