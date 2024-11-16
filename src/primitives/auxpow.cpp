// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <primitives/auxpow.h>
#include <stdexcept>
#include <streams.h>
#include <tinyformat.h>
#include <util/result.h>
#include <util/strencodings.h>
#include <util/translation.h>

int32_t MakeVersionWithChainId(uint32_t nChainId, uint32_t nLowVersionBits) {
    // Ensure nChainId and nLowVersionBits are in a valid range
    if (nLowVersionBits >= VERSION_AUXPOW_BIT) {
        throw std::invalid_argument(
            strprintf("nLowVersionBits out of range: 0x%x >= 0x%x",
                      nLowVersionBits, VERSION_AUXPOW_BIT));
    }
    if (nChainId > MAX_ALLOWED_CHAIN_ID) {
        throw std::invalid_argument(
            strprintf("nChainId out of range: 0x%x > 0x%x", nChainId,
                      MAX_ALLOWED_CHAIN_ID));
    }
    return (nChainId << VERSION_CHAIN_ID_BIT_POS) | nLowVersionBits;
}

int32_t VersionWithAuxPow(uint32_t nVersion, bool hasAuxPow) {
    if (hasAuxPow) {
        return nVersion | VERSION_AUXPOW_BIT;
    } else {
        return nVersion & ~VERSION_AUXPOW_BIT;
    }
}

uint256 ComputeMerkleRootForBranch(uint256 hash,
                                   const std::vector<uint256> &vMerkleBranch,
                                   uint32_t nIndex) {
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

util::Result<ParsedAuxPowCoinbase>
ParsedAuxPowCoinbase::Parse(const CScript &scriptCoinbase, uint256 hashRoot) {
    // Root hash in coinbase scriptSig is big endian
    std::reverse(hashRoot.begin(), hashRoot.end());

    // Find the root hash in the coinbase script
    CScript::const_iterator pRootHash =
        std::search(scriptCoinbase.begin(), scriptCoinbase.end(),
                    hashRoot.begin(), hashRoot.end());

    // Make sure we found the root hash in the coinbase
    if (pRootHash == scriptCoinbase.end()) {
        return {{_("AuxPow missing chain merkle root in parent coinbase")}};
    }

    // Bytespan from the root hash to the end of the coinbase
    SpanReader mergeMineCbData(
        SER_NETWORK, PROTOCOL_VERSION,
        Span((uint8_t *)&*pRootHash, (uint8_t *)&*scriptCoinbase.end()));

    // Find the merge-mined prefix in the coinbase script
    CScript::const_iterator pPrefix =
        std::search(scriptCoinbase.begin(), scriptCoinbase.end(),
                    MERGE_MINE_PREFIX.begin(), MERGE_MINE_PREFIX.end());

    if (pPrefix != scriptCoinbase.end()) {
        // If we found the prefix:
        // Ensure we don't find any other merge mine prefixes
        if (scriptCoinbase.end() !=
            std::search(pPrefix + 1, scriptCoinbase.end(),
                        MERGE_MINE_PREFIX.begin(), MERGE_MINE_PREFIX.end())) {
            return {{_("Multiple merged mining prefixes in coinbase")}};
        }

        // Ensure merge mine data is right after the prefix
        if (pPrefix + MERGE_MINE_PREFIX.size() != pRootHash) {
            return {{_(
                "Merged mining prefix is not just before chain merkle root")}};
        }
    } else {
        // For backward compatibility: Merge-mine prefix not found.
        // Enforce only one chain merkle root by checking that it starts early
        // in the coinbase. 8-12 bytes are enough to encode extraNonce and
        // nBits.
        if (pRootHash - scriptCoinbase.begin() > 20) {
            return {{_("AuxPow chain merkle root can have at most 20 preceding "
                       "bytes of the parent coinbase")}};
        }
    }

    // Skip 32 bytes for the root hash
    uint256 h;
    mergeMineCbData >> h;

    // We need 8 bytes for the next two fields
    if (mergeMineCbData.size() < 8) {
        return {{_("AuxPow missing chain merkle tree size and nonce in "
                   "parent coinbase")}};
    }

    // Read number of leaves in the merkle tree
    uint32_t nTreeSize;
    mergeMineCbData >> nTreeSize;

    // Read a nonce to make the index in the tree random
    uint32_t nMergeMineNonce;
    mergeMineCbData >> nMergeMineNonce;

    return {{
        .nTreeSize = nTreeSize,
        .nMergeMineNonce = nMergeMineNonce,
    }};
}

uint32_t CalcExpectedMerkleTreeIndex(uint32_t nNonce, uint32_t nChainId,
                                     uint32_t merkleHeight) {
    // Choose a pseudo-random slot in the chain merkle tree but have it be fixed
    // for a size/nonce/chain combination.
    // This prevents the same work from being used twice for the same chain
    // while reducing the chance that two chains clash for the same slot.

    // This computation can overflow the uint32 used. This is not an issue,
    // though, since we take the mod against a power-of-two in the end anyway.
    // This also ensures that the computation is, actually, consistent even
    // if done in 64 bits as it was in the past on some systems.
    // Note that h is always <= 30 (enforced by the maximum allowed chain merkle
    // branch length), so that 32 bits are enough for the computation.

    const uint32_t TWIST_FACTOR = 1103515245;
    const uint32_t TWIST_OFFSET = 12345;

    uint32_t rand = nNonce;
    rand = rand * TWIST_FACTOR + TWIST_OFFSET;
    rand += nChainId;
    rand = rand * TWIST_FACTOR + TWIST_OFFSET;

    return rand % (1 << merkleHeight);
}
