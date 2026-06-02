// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumaux.h>

#include <arith_uint256.h>
#include <chainparams.h>
#include <config.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <node/miner.h>
#include <pow/pow.h>
#include <primitives/auxpow.h>
#include <streams.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <validation.h>

namespace stratum {

StratumAuxManager::StratumAuxManager(Chainstate &chainstate,
                                     const CTxMemPool *mempool,
                                     const CChainParams &params,
                                     const CScript &coinbaseScript)
    : m_chainstate(chainstate), m_mempool(mempool), m_params(params),
      m_coinbaseScript(coinbaseScript) {}

util::Result<AuxWorkTemplate> StratumAuxManager::CreateAuxWork() {
    LOCK(cs_main);

    const CBlockIndex *pindexPrev = m_chainstate.m_chain.Tip();
    if (!pindexPrev) {
        return {{_("No chain tip available")}};
    }

    node::BlockAssembler assembler(::GetConfig(), m_chainstate, m_mempool);
    auto blockTemplate = assembler.CreateNewBlock(m_coinbaseScript);
    if (!blockTemplate) {
        return {{_("Failed to create block template")}};
    }

    CBlock &block = blockTemplate->block;

    // Build the StratumJob underlying this aux work
    StratumJob job;
    job.jobId = 0; // aux work uses hash as key, not jobId
    job.block = std::make_shared<CBlock>(block);
    job.nBitsRaw = block.nBits;
    job.nVersionRaw = block.nVersion;
    job.height = pindexPrev->nHeight + 1;

    // Compute the aux block hash (SHA-256d of the 80-byte header, NOT Scrypt)
    uint256 auxBlockHash = block.GetHash();

    // Compute network target from nBits
    arith_uint256 target;
    NBitsToTarget(m_params.GetConsensus(), block.nBits, target);

    Amount coinbaseValue = Amount::zero();
    if (!block.vtx.empty() && !block.vtx[0]->vout.empty()) {
        for (const auto &out : block.vtx[0]->vout) {
            coinbaseValue += out.nValue;
        }
    }

    AuxWorkTemplate work;
    work.auxBlockHash = auxBlockHash;
    work.nChainId = AUXPOW_CHAIN_ID;
    work.prevBlockHash = block.hashPrevBlock;
    work.coinbaseValue = coinbaseValue;
    work.nBits = block.nBits;
    work.height = job.height;
    work.target = target;
    work.underlyingJob = std::move(job);

    m_pendingWork[auxBlockHash] = work;

    return work;
}

MergeMineCommitment StratumAuxManager::BuildCommitment(
    const uint256 &auxBlockHash,
    const std::vector<uint256> &otherAuxHashes) const {

    MergeMineCommitment commitment;

    // Build the chain merkle tree
    std::vector<uint256> leaves;
    leaves.push_back(auxBlockHash);
    for (const auto &h : otherAuxHashes) {
        leaves.push_back(h);
    }

    // Tree size must be a power of 2
    uint32_t treeSize = 1;
    uint32_t merkleHeight = 0;
    while (treeSize < leaves.size()) {
        treeSize <<= 1;
        merkleHeight++;
    }

    // Pad to power-of-2 with zero hashes
    while (leaves.size() < treeSize) {
        leaves.push_back(uint256());
    }

    // Find the correct nonce that places Dogecoin at the expected index
    // using the CalcExpectedMerkleTreeIndex LCG
    uint32_t dogeIndex = 0;
    uint32_t nonce = 0;
    if (merkleHeight > 0) {
        for (nonce = 0; nonce < 0xFFFFFFFF; nonce++) {
            uint32_t idx = CalcExpectedMerkleTreeIndex(nonce, AUXPOW_CHAIN_ID,
                                                       merkleHeight);
            if (idx < treeSize) {
                dogeIndex = idx;
                break;
            }
        }
        // Place the Doge hash at the correct index
        if (dogeIndex != 0) {
            std::swap(leaves[0], leaves[dogeIndex]);
        }
    }

    commitment.nTreeSize = treeSize;
    commitment.nMergeMineNonce = nonce;
    commitment.nChainIndex = dogeIndex;

    // Compute chain merkle branch for the Doge leaf
    if (merkleHeight == 0) {
        commitment.chainMerkleRoot = auxBlockHash;
    } else {
        // Build the merkle tree and extract the branch for dogeIndex
        std::vector<uint256> level = leaves;
        size_t index = dogeIndex;
        while (level.size() > 1) {
            size_t siblingIdx = index ^ 1;
            if (siblingIdx < level.size()) {
                commitment.chainMerkleBranch.push_back(level[siblingIdx]);
            } else {
                commitment.chainMerkleBranch.push_back(level[index]);
            }
            std::vector<uint256> nextLevel;
            for (size_t i = 0; i < level.size(); i += 2) {
                uint256 left = level[i];
                uint256 right = (i + 1 < level.size()) ? level[i + 1] : left;
                nextLevel.push_back(Hash(Span(left), Span(right)));
            }
            level = nextLevel;
            index /= 2;
        }
        commitment.chainMerkleRoot = level[0];
    }

    // Build the coinbase payload: MERGE_MINE_PREFIX + root (big-endian) +
    // treeSize (LE) + nonce (LE)
    commitment.coinbasePayload.clear();
    commitment.coinbasePayload.insert(commitment.coinbasePayload.end(),
                                       MERGE_MINE_PREFIX.begin(),
                                       MERGE_MINE_PREFIX.end());

    // Root hash in big-endian (reversed)
    uint256 rootBE = commitment.chainMerkleRoot;
    std::reverse(rootBE.begin(), rootBE.end());
    commitment.coinbasePayload.insert(commitment.coinbasePayload.end(),
                                       rootBE.begin(), rootBE.end());

    // Tree size (4 bytes, little-endian)
    uint32_t ts = commitment.nTreeSize;
    commitment.coinbasePayload.push_back(ts & 0xff);
    commitment.coinbasePayload.push_back((ts >> 8) & 0xff);
    commitment.coinbasePayload.push_back((ts >> 16) & 0xff);
    commitment.coinbasePayload.push_back((ts >> 24) & 0xff);

    // Merge nonce (4 bytes, little-endian)
    uint32_t mn = commitment.nMergeMineNonce;
    commitment.coinbasePayload.push_back(mn & 0xff);
    commitment.coinbasePayload.push_back((mn >> 8) & 0xff);
    commitment.coinbasePayload.push_back((mn >> 16) & 0xff);
    commitment.coinbasePayload.push_back((mn >> 24) & 0xff);

    return commitment;
}

util::Result<std::shared_ptr<CAuxPow>> StratumAuxManager::AssembleAuxPow(
    const AuxWorkTemplate &work,
    const AuxPowSubmission &submission) const {

    auto auxpow = std::make_shared<CAuxPow>();

    // Coinbase must be at index 0
    if (submission.coinbaseMerkleIndex != 0) {
        return {{_("AuxPow coinbase must be at merkle index 0")}};
    }

    // Parent chain ID must not be Dogecoin's
    if (VersionChainId(submission.parentHeader.nVersion) == AUXPOW_CHAIN_ID) {
        return {{_("AuxPow parent has our chain ID")}};
    }

    auxpow->coinbaseTx = submission.parentCoinbaseTx;
    auxpow->hashBlock = submission.parentBlockHash;
    auxpow->vMerkleBranch = submission.coinbaseMerkleBranch;
    auxpow->nIndex = submission.coinbaseMerkleIndex;

    // Build commitment to find chain merkle data
    MergeMineCommitment commitment = BuildCommitment(work.auxBlockHash);
    auxpow->vChainMerkleBranch = commitment.chainMerkleBranch;
    auxpow->nChainIndex = commitment.nChainIndex;

    auxpow->parentBlock = submission.parentHeader;

    // Validate the assembled AuxPow
    const Consensus::Params &consensus = m_params.GetConsensus();
    util::Result<std::monostate> checkResult =
        auxpow->CheckAuxBlockHash(work.auxBlockHash,
                                   VersionChainId(work.underlyingJob.nVersionRaw),
                                   consensus);
    if (!checkResult) {
        return {{util::ErrorString(checkResult)}};
    }

    return auxpow;
}

bool StratumAuxManager::ValidateParentPow(
    const CBaseBlockHeader &parentHeader, uint32_t dogeNBits,
    const Consensus::Params &params) const {
    // PoW is Scrypt on the parent header, compared to Doge's nBits target
    BlockHash powHash = parentHeader.GetPowHash();
    return CheckProofOfWork(powHash, dogeNBits, params);
}

bool StratumAuxManager::SubmitAuxBlock(const AuxWorkTemplate &work,
                                        std::shared_ptr<CAuxPow> auxpow,
                                        ChainstateManager &chainman) {
    auto block = std::make_shared<CBlock>(*work.underlyingJob.block);

    // Set the AuxPoW version flag
    block->nVersion =
        VersionWithAuxPow(work.underlyingJob.nVersionRaw, true);
    block->auxpow = std::move(auxpow);

    bool newBlock = false;
    return chainman.ProcessNewBlock(block, /*force_processing=*/true,
                                    /*min_pow_checked=*/true, &newBlock);
}

const AuxWorkTemplate *
StratumAuxManager::GetWork(const uint256 &auxBlockHash) const {
    auto it = m_pendingWork.find(auxBlockHash);
    if (it == m_pendingWork.end()) {
        return nullptr;
    }
    return &it->second;
}

void StratumAuxManager::PruneWork(size_t keepCount) {
    while (m_pendingWork.size() > keepCount) {
        m_pendingWork.erase(m_pendingWork.begin());
    }
}

} // namespace stratum
