// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumjob.h>

#include <chainparams.h>
#include <config.h>
#include <consensus/merkle.h>
#include <logging.h>
#include <node/miner.h>
#include <primitives/auxpow.h>
#include <stratum/mergemine.h>
#include <stratum/stratumaux.h>
#include <streams.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <validation.h>

namespace stratum {

StratumJobManager::StratumJobManager(Chainstate &chainstate,
                                     const CTxMemPool *mempool,
                                     const CChainParams &params,
                                     const CScript &coinbaseScript,
                                     size_t extranonce1Size,
                                     size_t extranonce2Size)
    : m_chainstate(chainstate), m_mempool(mempool), m_params(params),
      m_coinbaseScript(coinbaseScript), m_extranonce1Size(extranonce1Size),
      m_extranonce2Size(extranonce2Size) {}

std::pair<std::string, std::string>
StratumJobManager::SplitCoinbase(const CTransaction &coinbaseTx) const {
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << coinbaseTx;
    std::string fullHex = HexStr(ss);
    std::vector<uint8_t> raw = ParseHex(fullHex);

    // Tx layout: [version:4][vin_count:varint][prevout:36]
    //            [scriptSig_len:varint][scriptSig:N][sequence:4]
    //            [vout_count:varint][vouts...][locktime:4]
    //
    // The miner will insert extranonce1 + extranonce2 into the scriptSig,
    // so we must update scriptSig_len to include those extra bytes.

    size_t pos = 0;
    pos += 4;  // version
    pos += 1;  // vin count (always 1 for coinbase)
    pos += 36; // prevout (32 hash + 4 index)

    size_t scriptSigLenPos = pos;

    // Read original scriptSig length (varint)
    uint64_t origScriptSigLen = 0;
    int varintBytes = 0;
    if (raw[pos] < 0xfd) {
        origScriptSigLen = raw[pos];
        varintBytes = 1;
    } else if (raw[pos] == 0xfd) {
        origScriptSigLen = raw[pos + 1] | (raw[pos + 2] << 8);
        varintBytes = 3;
    } else {
        origScriptSigLen = raw[pos + 1] | (raw[pos + 2] << 8) |
                           (raw[pos + 3] << 16) |
                           ((uint64_t)raw[pos + 4] << 24);
        varintBytes = 5;
    }
    pos += varintBytes;

    size_t scriptSigStart = pos;
    size_t scriptSigEnd = pos + origScriptSigLen;

    // New scriptSig length = original + extranonce1 + extranonce2
    uint64_t extranonceSpace = m_extranonce1Size + m_extranonce2Size;
    uint64_t newScriptSigLen = origScriptSigLen + extranonceSpace;

    // Encode the new length as varint
    std::vector<uint8_t> newVarint;
    if (newScriptSigLen < 0xfd) {
        newVarint.push_back((uint8_t)newScriptSigLen);
    } else if (newScriptSigLen <= 0xffff) {
        newVarint.push_back(0xfd);
        newVarint.push_back(newScriptSigLen & 0xff);
        newVarint.push_back((newScriptSigLen >> 8) & 0xff);
    } else {
        newVarint.push_back(0xfe);
        newVarint.push_back(newScriptSigLen & 0xff);
        newVarint.push_back((newScriptSigLen >> 8) & 0xff);
        newVarint.push_back((newScriptSigLen >> 16) & 0xff);
        newVarint.push_back((newScriptSigLen >> 24) & 0xff);
    }

    // Rebuild: [prefix before scriptSig_len] + [new varint] + [scriptSig] =>
    //          coinbase1 (miner appends extranonce1 + extranonce2 here)
    // coinbase2 = [sequence] + [vout...] + [locktime]
    std::vector<uint8_t> cb1Data;
    cb1Data.insert(cb1Data.end(), raw.begin(),
                   raw.begin() + scriptSigLenPos);
    cb1Data.insert(cb1Data.end(), newVarint.begin(), newVarint.end());
    cb1Data.insert(cb1Data.end(), raw.begin() + scriptSigStart,
                   raw.begin() + scriptSigEnd);

    std::vector<uint8_t> cb2Data(raw.begin() + scriptSigEnd, raw.end());

    return {HexStr(cb1Data), HexStr(cb2Data)};
}

std::vector<uint256>
StratumJobManager::ComputeMerkleBranches(const CBlock &block) const {
    std::vector<uint256> branches;
    std::vector<uint256> leaves;
    for (const auto &tx : block.vtx) {
        leaves.push_back(tx->GetHash());
    }

    if (leaves.size() <= 1) {
        return branches;
    }

    // Standard merkle branch for leaf at index 0 (the coinbase)
    std::vector<uint256> level = leaves;
    size_t index = 0;
    while (level.size() > 1) {
        // Sibling of the current index
        size_t siblingIdx = index ^ 1;
        if (siblingIdx < level.size()) {
            branches.push_back(level[siblingIdx]);
        } else {
            branches.push_back(level[index]);
        }

        // Next level
        std::vector<uint256> nextLevel;
        for (size_t i = 0; i < level.size(); i += 2) {
            uint256 left = level[i];
            uint256 right = (i + 1 < level.size()) ? level[i + 1] : left;
            nextLevel.push_back(Hash(Span(left), Span(right)));
        }
        level = nextLevel;
        index /= 2;
    }

    return branches;
}

util::Result<StratumJob> StratumJobManager::CreateJob(bool cleanJobs) {
    LOCK(cs_main);

    const CBlockIndex *pindexPrev = m_chainstate.m_chain.Tip();
    if (!pindexPrev) {
        return {{_("No chain tip available")}};
    }

    node::BlockAssembler assembler(
        ::GetConfig(), m_chainstate, m_mempool);

    auto blockTemplate = assembler.CreateNewBlock(m_coinbaseScript);
    if (!blockTemplate) {
        return {{_("Failed to create block template")}};
    }

    CBlock &block = blockTemplate->block;

    // Inject merge-mine commitment into the coinbase scriptSig before
    // splitting, so the fabe6d6d payload is part of coinbase1 that miners
    // hash over.
    MergeMineCommitment commitment;
    std::vector<AuxChainTarget> auxTargets;

    auto *mm = GetMergeMineManager();
    if (mm && mm->ChainCount() > 0) {
        block.hashMerkleRoot = BlockMerkleRoot(block);

        // Compute DOGE's own aux hash (with AuxPoW version bit)
        CBlockHeader hdr;
        hdr.nVersion = VersionWithAuxPow(block.nVersion, true);
        hdr.hashPrevBlock = block.hashPrevBlock;
        hdr.hashMerkleRoot = block.hashMerkleRoot;
        hdr.nTime = block.nTime;
        hdr.nBits = block.nBits;
        hdr.nNonce = block.nNonce;
        uint256 dogeAuxHash = hdr.GetHash();

        auto allWork = mm->GetAllWorkSorted();
        std::vector<uint256> otherHashes;
        for (const auto &w : allWork) {
            otherHashes.push_back(w.auxHash);
            auxTargets.push_back({w.chainName, w.chainId, w.auxHash, w.nBits});
        }

        auto *auxMgr = GetGlobalAuxManager();
        if (auxMgr) {
            commitment = auxMgr->BuildCommitment(dogeAuxHash, otherHashes);
        }

        if (!commitment.coinbasePayload.empty()) {
            CMutableTransaction mtx(*block.vtx[0]);
            if (!mtx.vin.empty()) {
                CScript &scriptSig = mtx.vin[0].scriptSig;
                scriptSig.insert(scriptSig.end(),
                                 commitment.coinbasePayload.begin(),
                                 commitment.coinbasePayload.end());
                block.vtx[0] = MakeTransactionRef(std::move(mtx));
            }
        }
    }

    StratumJob job;
    job.jobId = m_nextJobId++;
    job.block = std::make_shared<CBlock>(block);
    job.blockTemplate = std::move(blockTemplate);
    job.cleanJobs = cleanJobs;
    job.nBitsRaw = block.nBits;
    job.nVersionRaw = block.nVersion;
    job.height = pindexPrev->nHeight + 1;
    job.mergeCommitment = std::move(commitment);
    job.auxChainTargets = std::move(auxTargets);

    // Split coinbase (now includes fabe6d6d payload if any)
    auto [cb1, cb2] = SplitCoinbase(*block.vtx[0]);
    job.coinbase1 = cb1;
    job.coinbase2 = cb2;

    // Merkle branches
    auto branches = ComputeMerkleBranches(block);
    for (const auto &b : branches) {
        job.merkleBranches.push_back(HexStr(Span(b)));
    }

    // Header fields in Stratum hex format
    job.prevHash = HashToStratumHex(block.hashPrevBlock);
    job.version = Uint32ToStratumHex(block.nVersion);
    job.nbits = Uint32ToStratumHex(block.nBits);
    job.ntime = Uint32ToStratumHex(block.nTime);

    m_lastTipHash = pindexPrev->GetBlockHash();

    uint64_t id = job.jobId;
    m_jobs[id] = job;

    if (!job.auxChainTargets.empty()) {
        LogPrint(BCLog::STRATUM,
                 "Stratum: job %x embeds %zu aux chain(s) in coinbase\n",
                 id, job.auxChainTargets.size());
    }

    return job;
}

UniValue StratumJobManager::FormatNotifyParams(const StratumJob &job) const {
    UniValue params(UniValue::VARR);
    params.push_back(strprintf("%x", job.jobId));
    params.push_back(job.prevHash);
    params.push_back(job.coinbase1);
    params.push_back(job.coinbase2);

    UniValue branches(UniValue::VARR);
    for (const auto &b : job.merkleBranches) {
        branches.push_back(b);
    }
    params.push_back(branches);

    params.push_back(job.version);
    params.push_back(job.nbits);
    params.push_back(job.ntime);
    params.push_back(job.cleanJobs);

    return params;
}

const StratumJob *StratumJobManager::GetJob(uint64_t jobId) const {
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return nullptr;
    }
    return &it->second;
}

void StratumJobManager::PruneJobs(size_t keepCount) {
    while (m_jobs.size() > keepCount) {
        m_jobs.erase(m_jobs.begin());
    }
}

bool StratumJobManager::HasNewTip() const {
    LOCK(cs_main);
    const CBlockIndex *tip = m_chainstate.m_chain.Tip();
    if (!tip) {
        return false;
    }
    return tip->GetBlockHash() != m_lastTipHash;
}

size_t StratumJobManager::JobCount() const {
    return m_jobs.size();
}

std::string HashToStratumHex(const uint256 &hash) {
    // Stratum prevhash: 32 bytes, each 4-byte word byte-swapped
    const uint8_t *data = hash.begin();
    std::string result;
    result.reserve(64);
    for (int i = 0; i < 32; i += 4) {
        for (int j = 3; j >= 0; j--) {
            result += strprintf("%02x", data[i + j]);
        }
    }
    return result;
}

std::string Uint32ToStratumHex(uint32_t val) {
    uint8_t buf[4];
    buf[0] = val & 0xff;
    buf[1] = (val >> 8) & 0xff;
    buf[2] = (val >> 16) & 0xff;
    buf[3] = (val >> 24) & 0xff;
    return HexStr(Span<const uint8_t>(buf, 4));
}

} // namespace stratum
