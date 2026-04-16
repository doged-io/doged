// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMJOB_H
#define BITCOIN_STRATUM_STRATUMJOB_H

#include <primitives/block.h>
#include <script/script.h>
#include <uint256.h>
#include <univalue.h>
#include <util/result.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class CChainParams;
class Chainstate;
class CTxMemPool;

namespace node {
struct CBlockTemplate;
}

namespace stratum {

/**
 * Data to embed in the parent chain's coinbase transaction for merge-mining.
 * Moved here (from stratumaux.h) so StratumJob can hold it without circular
 * includes.
 */
struct MergeMineCommitment {
    std::vector<uint8_t> coinbasePayload; // fabe6d6d + root + treesize + nonce
    uint256 chainMerkleRoot;
    uint32_t nTreeSize = 0;
    uint32_t nMergeMineNonce = 0;
    uint32_t nChainIndex = 0;
    std::vector<uint256> chainMerkleBranch;
};

/**
 * Per-chain target snapshot stored in each StratumJob so that share
 * validation can independently check every external chain's nBits.
 */
struct AuxChainTarget {
    std::string chainName;
    uint32_t chainId = 0;
    uint256 auxHash;
    uint32_t nBits = 0;
};

struct StratumJob {
    uint64_t jobId;
    std::shared_ptr<CBlock> block;
    std::shared_ptr<node::CBlockTemplate> blockTemplate;
    std::string coinbase1; // hex: serialized coinbase up to extranonce
    std::string coinbase2; // hex: serialized coinbase after extranonce
    std::vector<std::string> merkleBranches; // hex hashes
    std::string prevHash;                    // hex, 32 bytes reversed
    std::string version;                     // hex, 4 bytes
    std::string nbits;                       // hex, 4 bytes
    std::string ntime;                       // hex, 4 bytes
    bool cleanJobs = false;
    uint32_t nBitsRaw = 0;
    int32_t nVersionRaw = 0;
    int height = 0;

    MergeMineCommitment mergeCommitment;
    std::vector<AuxChainTarget> auxChainTargets;
};

class StratumJobManager {
public:
    StratumJobManager(Chainstate &chainstate, const CTxMemPool *mempool,
                      const CChainParams &params,
                      const CScript &coinbaseScript,
                      size_t extranonce1Size, size_t extranonce2Size);

    /**
     * Create a new job from the current chain tip.
     * Calls BlockAssembler::CreateNewBlock internally.
     */
    util::Result<StratumJob> CreateJob(bool cleanJobs);

    /** Format mining.notify params for a job. */
    UniValue FormatNotifyParams(const StratumJob &job) const;

    /** Retrieve a cached job by ID. */
    const StratumJob *GetJob(uint64_t jobId) const;

    /** Remove old jobs, keeping the most recent keepCount. */
    void PruneJobs(size_t keepCount);

    /** Check if the chain tip changed since last job creation. */
    bool HasNewTip() const;

    size_t JobCount() const;

private:
    Chainstate &m_chainstate;
    const CTxMemPool *m_mempool;
    const CChainParams &m_params;
    CScript m_coinbaseScript;
    size_t m_extranonce1Size;
    size_t m_extranonce2Size;
    uint64_t m_nextJobId = 1;
    std::map<uint64_t, StratumJob> m_jobs;
    BlockHash m_lastTipHash;

    std::pair<std::string, std::string>
    SplitCoinbase(const CTransaction &coinbaseTx) const;

    std::vector<uint256> ComputeMerkleBranches(const CBlock &block) const;
};

/**
 * Convert a 256-bit hash to the byte-swapped hex format Stratum uses for
 * prevhash (groups of 4 bytes reversed within each 4-byte word).
 */
std::string HashToStratumHex(const uint256 &hash);

/** Encode a uint32 as 8-char little-endian hex. */
std::string Uint32ToStratumHex(uint32_t val);

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMJOB_H
