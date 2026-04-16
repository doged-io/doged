// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMAUX_H
#define BITCOIN_STRATUM_STRATUMAUX_H

#include <arith_uint256.h>
#include <primitives/auxpow.h>
#include <primitives/block.h>
#include <script/script.h>
#include <stratum/stratumjob.h>
#include <univalue.h>
#include <util/result.h>

#include <sync.h>

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <vector>

class CChainParams;
class Chainstate;
class ChainstateManager;
class CTxMemPool;

namespace stratum {

/**
 * An aux work template representing a Dogecoin block ready for merge-mining.
 * External miners solve a parent chain block whose coinbase commits to
 * this aux block's hash.
 */
struct AuxWorkTemplate {
    uint256 auxBlockHash;   // SHA-256d of the Doge header (block identity)
    uint32_t nChainId;      // 0x62 for Dogecoin
    BlockHash prevBlockHash;
    Amount coinbaseValue;
    uint32_t nBits;
    int32_t height;
    arith_uint256 target;
    StratumJob underlyingJob;
};

/**
 * Data submitted from a parent chain miner proving work was done.
 */
struct AuxPowSubmission {
    CTransactionRef parentCoinbaseTx;
    uint256 parentBlockHash;
    std::vector<uint256> coinbaseMerkleBranch;
    uint32_t coinbaseMerkleIndex; // must be 0
    CBaseBlockHeader parentHeader;
};

class StratumAuxManager {
public:
    StratumAuxManager(Chainstate &chainstate, const CTxMemPool *mempool,
                      const CChainParams &params,
                      const CScript &coinbaseScript);

    /**
     * Create an aux work template from the current chain tip.
     * Equivalent to a "createauxblock" RPC.
     */
    util::Result<AuxWorkTemplate> CreateAuxWork();

    /**
     * Build the merge-mine commitment data for embedding in a parent coinbase.
     * @param auxBlockHash The SHA-256d hash of the Dogecoin block header.
     * @param otherAuxHashes Hashes of other aux chains (for multi-aux mining).
     */
    MergeMineCommitment
    BuildCommitment(const uint256 &auxBlockHash,
                    const std::vector<uint256> &otherAuxHashes = {}) const;

    /**
     * Assemble a CAuxPow from parent chain submission data.
     * Validates all structural constraints.
     */
    util::Result<std::shared_ptr<CAuxPow>>
    AssembleAuxPow(const AuxWorkTemplate &work,
                   const AuxPowSubmission &submission) const;

    /**
     * Validate that the parent header's Scrypt PoW hash meets Doge's target.
     */
    bool ValidateParentPow(const CBaseBlockHeader &parentHeader,
                           uint32_t dogeNBits,
                           const Consensus::Params &params) const;

    /**
     * Submit a completed merge-mined block with its AuxPoW proof.
     */
    bool SubmitAuxBlock(const AuxWorkTemplate &work,
                        std::shared_ptr<CAuxPow> auxpow,
                        ChainstateManager &chainman);

    /** Retrieve pending work by aux block hash (returns a copy). */
    std::optional<AuxWorkTemplate> GetWork(const uint256 &auxBlockHash) const;

    /** Remove a specific work entry (after successful submission). */
    void RemoveWork(const uint256 &auxBlockHash);

    /** Remove old work items, keeping at most keepCount. */
    void PruneWork(size_t keepCount);

    /**
     * Update the coinbase script (e.g. when a new address is passed via RPC).
     */
    void SetCoinbaseScript(const CScript &script);

private:
    mutable Mutex m_mutex;
    Chainstate &m_chainstate;
    const CTxMemPool *m_mempool;
    const CChainParams &m_params;
    CScript m_coinbaseScript GUARDED_BY(m_mutex);
    std::map<uint256, AuxWorkTemplate> m_pendingWork GUARDED_BY(m_mutex);
    std::deque<uint256> m_workInsertOrder GUARDED_BY(m_mutex);
};

/**
 * Global aux manager lifecycle — initialised once during startup so that
 * createauxblock/submitauxblock RPCs share the same work cache.
 */
void InitGlobalAuxManager(Chainstate &chainstate, const CTxMemPool *mempool,
                          const CChainParams &params);
StratumAuxManager *GetGlobalAuxManager();
void StopGlobalAuxManager();

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMAUX_H
