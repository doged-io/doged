// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <primitives/blockhash.h>
#include <primitives/transaction.h>
#include <primitives/pureheader.h>
#include <primitives/auxpow.h>
#include <serialize.h>
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
class CBlockHeader : public CPureBlockHeader {
public:
    // auxpow (if this is a merge-minded block)
    std::shared_ptr<CAuxPow> auxpow;

    CBlockHeader() { SetNull(); }

    /*SERIALIZE_METHODS(CBlockHeader, obj) {
        READWRITE(obj.nVersion, obj.hashPrevBlock, obj.hashMerkleRoot,
                  obj.nTime, obj.nBits, obj.nNonce);
    }*/

    template <typename Stream> inline void Serialize(Stream &s) const {
        s << *(CPureBlockHeader*)this;
        
        if (this->IsAuxpow()) {
            assert(auxpow);
            s << *auxpow;
        }
    }

    template <typename Stream> inline void Unserialize(Stream &s) {
        s >> *(CPureBlockHeader*)this;
        
        if (this->IsAuxpow()) {
            auxpow.reset(new CAuxPow());
            s >> *auxpow;
        } else {
            auxpow.reset();
        }
    }

    void SetNull() {
        nVersion = 0;
        hashPrevBlock = BlockHash();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        auxpow.reset();
    }

    /**
     * Set the block's auxpow (or unset it).  This takes care of updating
     * the version accordingly.
     * @param apow Pointer to the auxpow to use or NULL.
     */
    void SetAuxpow(CAuxPow* apow);

    bool IsNull() const { return (nBits == 0); }

    //BlockHash GetHash() const;

    BlockHash GetPoWHash() const;

    NodeSeconds Time() const {
        return NodeSeconds{std::chrono::seconds{nTime}};
    }

    int64_t GetBlockTime() const { return (int64_t)nTime; }
};

class CBlock : public CBlockHeader {
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

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
    }

    CBlockHeader GetBlockHeader() const {
        CBlockHeader block;
        block.nVersion = nVersion;
        block.hashPrevBlock = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime = nTime;
        block.nBits = nBits;
        block.nNonce = nNonce;
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

    explicit CBlockLocator(const std::vector<BlockHash> &vHaveIn)
        : vHave(vHaveIn) {}

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
