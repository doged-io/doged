// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <primitives/auxpow.h>
#include <primitives/baseheader.h>
#include <primitives/blockhash.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/time.h>

/**
 * Nodes collect new transactions into a block, hash them into a hash tree, and
 * scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements. When they solve the proof-of-work, they broadcast the block to
 * everyone and the block is added to the block chain. The first transaction in
 * the block is a special one that creates a new coin owned by the creator of
 * the block.
 */
class CBlockHeader : public CBaseBlockHeader {
public:
    std::shared_ptr<CAuxPow> auxpow;

    template <typename Stream> inline void Serialize(Stream &s) const {
        s << *(CBaseBlockHeader *)this;

        if (VersionHasAuxPow(nVersion)) {
            if (!auxpow) {
                throw std::ios_base::failure(
                    strprintf("Missing auxpow in header %s, version %08x that "
                              "claims to have it",
                              GetHash().ToString(), nVersion));
            }
            s << *auxpow;
        }
    }

    template <typename Stream> inline void Unserialize(Stream &s) {
        s >> *(CBaseBlockHeader *)this;

        if (VersionHasAuxPow(nVersion)) {
            auxpow.reset(new CAuxPow());
            s >> *auxpow;
        } else {
            auxpow.reset();
        }
    }

    void SetNull() {
        CBaseBlockHeader::SetNull();
        auxpow.reset();
    }
};

class CBlock : public CBlockHeader {
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // Memory-only flags for caching expensive checks
    // CheckBlock()
    mutable bool fChecked;
    // CheckMerkleRoot()
    mutable bool m_checked_merkle_root{false};

    CBlock() { SetNull(); }

    CBlock(const CBlockHeader &header) {
        SetNull();
        *(static_cast<CBlockHeader *>(this)) = header;
    }

    SERIALIZE_METHODS(CBlock, obj) {
        READWRITEAS(CBlockHeader, obj);
        READWRITE(obj.vtx);
    }

    void SetNull() {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
        m_checked_merkle_root = false;
    }

    CBlockHeader GetBlockHeader() const {
        CBlockHeader block;
        block.nVersion = nVersion;
        block.hashPrevBlock = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime = nTime;
        block.nBits = nBits;
        block.nNonce = nNonce;
        block.auxpow = auxpow;
        return block;
    }

    std::string ToString() const;
};

/**
 * Describes a place in the block chain to another node such that if the other
 * node doesn't have the same branch, it can find a recent common trunk.  The
 * further back it is, the further before the fork it may be.
 */
struct CBlockLocator {
    std::vector<BlockHash> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(std::vector<BlockHash> &&vHaveIn)
        : vHave(std::move(vHaveIn)) {}

    SERIALIZE_METHODS(CBlockLocator, obj) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH)) {
            READWRITE(nVersion);
        }
        READWRITE(obj.vHave);
    }

    void SetNull() { vHave.clear(); }

    bool IsNull() const { return vHave.empty(); }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
