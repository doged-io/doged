// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stratum/stratumsubmit.h>

#include <arith_uint256.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <crypto/scrypt.h>
#include <hash.h>
#include <logging.h>
#include <pow/pow.h>
#include <primitives/auxpow.h>
#include <streams.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <validation.h>

namespace stratum {

util::Result<ShareSubmission> ParseSubmitParams(const UniValue &params) {
    if (!params.isArray() || params.size() < 5) {
        return {{_("mining.submit requires 5 parameters")}};
    }

    ShareSubmission sub;
    sub.workerName = params[0].get_str();

    // Job ID is sent as hex string
    std::string jobIdStr = params[1].get_str();
    try {
        sub.jobId = std::stoull(jobIdStr, nullptr, 16);
    } catch (...) {
        return {{_("Invalid job ID")}};
    }

    sub.extranonce2 = params[2].get_str();
    sub.ntime = params[3].get_str();
    sub.nonce = params[4].get_str();

    // Validate hex formats
    if (!IsHex(sub.extranonce2) || sub.extranonce2.size() != 8) {
        return {{_("Invalid extranonce2 (expected 8 hex chars)")}};
    }
    if (!IsHex(sub.ntime) || sub.ntime.size() != 8) {
        return {{_("Invalid ntime (expected 8 hex chars)")}};
    }
    if (!IsHex(sub.nonce) || sub.nonce.size() != 8) {
        return {{_("Invalid nonce (expected 8 hex chars)")}};
    }

    return sub;
}

CBaseBlockHeader ReconstructHeader(const StratumJob &job,
                                   const std::string &extranonce1,
                                   const ShareSubmission &sub) {
    // 1. Reconstruct the full coinbase transaction
    std::string fullCoinbaseHex =
        job.coinbase1 + extranonce1 + sub.extranonce2 + job.coinbase2;
    std::vector<uint8_t> coinbaseBytes = ParseHex(fullCoinbaseHex);

    CDataStream ss(coinbaseBytes, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction coinbaseTx(deserialize, ss);

    // 2. Compute merkle root
    uint256 coinbaseHash = coinbaseTx.GetHash();
    uint256 merkleRoot = coinbaseHash;

    // Apply merkle branches
    for (const auto &branchHex : job.merkleBranches) {
        uint256 branchHash;
        branchHash.SetHex(branchHex);
        merkleRoot = Hash(Span(merkleRoot), Span(branchHash));
    }

    // 3. Build header
    CBaseBlockHeader header;
    header.nVersion = job.nVersionRaw;
    header.hashPrevBlock = job.block->hashPrevBlock;
    header.hashMerkleRoot = merkleRoot;

    // Parse ntime from submission (little-endian hex)
    std::vector<uint8_t> ntimeBytes = ParseHex(sub.ntime);
    header.nTime = ntimeBytes[0] | (ntimeBytes[1] << 8) |
                   (ntimeBytes[2] << 16) | (ntimeBytes[3] << 24);

    header.nBits = job.nBitsRaw;

    // Parse nonce from submission (little-endian hex)
    std::vector<uint8_t> nonceBytes = ParseHex(sub.nonce);
    header.nNonce = nonceBytes[0] | (nonceBytes[1] << 8) |
                    (nonceBytes[2] << 16) | (nonceBytes[3] << 24);

    return header;
}

BlockHash ScryptPowHash(const CBaseBlockHeader &header) {
    return header.GetPowHash();
}

arith_uint256 DifficultyToTarget(double difficulty) {
    // Stratum difficulty 1 target for Scrypt:
    // 0x0000ffff00000000000000000000000000000000000000000000000000000000
    arith_uint256 target;
    target.SetCompact(0x1d00ffff);

    if (difficulty <= 0) {
        return target;
    }

    // target = diff1_target / difficulty
    // To avoid floating point, we use integer division with scaling.
    // For reasonable precision, multiply then divide.
    if (difficulty >= 1.0) {
        // Integer path for difficulty >= 1
        uint64_t diffInt = static_cast<uint64_t>(difficulty);
        if (diffInt > 0 && static_cast<double>(diffInt) == difficulty) {
            return target / diffInt;
        }
    }

    // General floating point path
    // target_new = target * (1/difficulty)
    // Use 64-bit fixed point: multiply target by 1000000, divide by
    // (difficulty * 1000000)
    uint64_t scale = 1000000;
    arith_uint256 scaled = target * scale;
    uint64_t diffScaled = static_cast<uint64_t>(difficulty * scale);
    if (diffScaled == 0) {
        diffScaled = 1;
    }
    return scaled / diffScaled;
}

bool NBitsToTarget(uint32_t nBits, arith_uint256 &target) {
    bool fNegative, fOverflow;
    target.SetCompact(nBits, &fNegative, &fOverflow);
    return !fNegative && !fOverflow && target != 0;
}

ShareResult ValidateShare(const StratumJob &job,
                          const std::string &extranonce1,
                          const ShareSubmission &sub,
                          double workerDifficulty,
                          const Consensus::Params &params,
                          std::set<std::string> &submittedNonces) {
    // Duplicate check: key = jobId + extranonce2 + nonce + ntime
    std::string dupeKey = strprintf("%x:%s:%s:%s", sub.jobId, sub.extranonce2,
                                    sub.nonce, sub.ntime);
    if (submittedNonces.count(dupeKey)) {
        return ShareResult::REJECTED_DUPLICATE;
    }

    CBaseBlockHeader header = ReconstructHeader(job, extranonce1, sub);
    BlockHash powHash = ScryptPowHash(header);

    arith_uint256 hashValue = UintToArith256(powHash);

    // Check against network target (block found!)
    arith_uint256 networkTarget;
    if (!NBitsToTarget(params, job.nBitsRaw, networkTarget)) {
        return ShareResult::REJECTED_INVALID;
    }

    submittedNonces.insert(dupeKey);

    if (hashValue <= networkTarget) {
        return ShareResult::ACCEPTED_BLOCK;
    }

    // Check against worker difficulty target (share)
    arith_uint256 workerTarget = DifficultyToTarget(workerDifficulty);
    if (hashValue <= workerTarget) {
        return ShareResult::ACCEPTED;
    }

    return ShareResult::REJECTED_LOW_DIFF;
}

ShareValidationResult ValidateShareEx(const StratumJob &job,
                                      const std::string &extranonce1,
                                      const ShareSubmission &sub,
                                      double workerDifficulty,
                                      const Consensus::Params &params,
                                      std::set<std::string> &submittedNonces) {
    ShareValidationResult result;

    std::string dupeKey = strprintf("%x:%s:%s:%s", sub.jobId, sub.extranonce2,
                                    sub.nonce, sub.ntime);
    if (submittedNonces.count(dupeKey)) {
        result.duplicateShare = true;
        return result;
    }

    CBaseBlockHeader header = ReconstructHeader(job, extranonce1, sub);
    BlockHash powHash = ScryptPowHash(header);
    arith_uint256 hashValue = UintToArith256(powHash);

    arith_uint256 networkTarget;
    if (!NBitsToTarget(params, job.nBitsRaw, networkTarget)) {
        result.invalidShare = true;
        return result;
    }

    submittedNonces.insert(dupeKey);

    if (hashValue <= networkTarget) {
        result.dogeBlockFound = true;
        result.shareAccepted = true;
    } else {
        arith_uint256 workerTarget = DifficultyToTarget(workerDifficulty);
        if (hashValue <= workerTarget) {
            result.shareAccepted = true;
        } else {
            result.lowDifficulty = true;
            return result;
        }
    }

    // Check each external chain's target independently
    for (const auto &auxTarget : job.auxChainTargets) {
        arith_uint256 chainTarget;
        if (NBitsToTarget(auxTarget.nBits, chainTarget) &&
            hashValue <= chainTarget) {
            result.auxChainsSolved.push_back(auxTarget.chainName);
        }
    }

    return result;
}

bool SubmitBlock(const StratumJob &job, const std::string &extranonce1,
                 const ShareSubmission &sub, ChainstateManager &chainman,
                 const CChainParams &chainParams) {
    // Reconstruct the full coinbase
    std::string fullCoinbaseHex =
        job.coinbase1 + extranonce1 + sub.extranonce2 + job.coinbase2;
    std::vector<uint8_t> coinbaseBytes = ParseHex(fullCoinbaseHex);

    CDataStream ss(coinbaseBytes, SER_NETWORK, PROTOCOL_VERSION);
    CMutableTransaction coinbaseMtx;
    ss >> coinbaseMtx;

    // Build the full block from the template
    auto block = std::make_shared<CBlock>(*job.block);
    block->vtx[0] = MakeTransactionRef(std::move(coinbaseMtx));
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    // Set the nonce and ntime from submission
    std::vector<uint8_t> ntimeBytes = ParseHex(sub.ntime);
    block->nTime = ntimeBytes[0] | (ntimeBytes[1] << 8) |
                   (ntimeBytes[2] << 16) | (ntimeBytes[3] << 24);

    std::vector<uint8_t> nonceBytes = ParseHex(sub.nonce);
    block->nNonce = nonceBytes[0] | (nonceBytes[1] << 8) |
                    (nonceBytes[2] << 16) | (nonceBytes[3] << 24);

    bool newBlock = false;
    return chainman.ProcessNewBlock(block, /*force_processing=*/true,
                                    /*min_pow_checked=*/true, &newBlock);
}

std::string BuildAuxPowForChain(const StratumJob &job,
                                const std::string &extranonce1,
                                const ShareSubmission &sub,
                                const std::string &chainName) {
    // 1. Reconstruct the full coinbase transaction
    std::string fullCoinbaseHex =
        job.coinbase1 + extranonce1 + sub.extranonce2 + job.coinbase2;
    std::vector<uint8_t> coinbaseBytes = ParseHex(fullCoinbaseHex);
    CDataStream cbStream(coinbaseBytes, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction coinbaseTx(deserialize, cbStream);

    // 2. Build the parent block header from the share
    CBaseBlockHeader parentHeader = ReconstructHeader(job, extranonce1, sub);

    // 3. Compute the coinbase merkle branch (path from coinbase to merkle root)
    //    The coinbase is always at index 0 in the block.
    std::vector<uint256> coinbaseMerkleBranch;
    if (job.block && job.block->vtx.size() > 1) {
        // Reconstruct the block with the miner's coinbase
        CBlock parentBlock(*job.block);
        CMutableTransaction mtx;
        CDataStream mtxStream(coinbaseBytes, SER_NETWORK, PROTOCOL_VERSION);
        mtxStream >> mtx;
        parentBlock.vtx[0] = MakeTransactionRef(std::move(mtx));

        std::vector<uint256> leaves;
        for (const auto &tx : parentBlock.vtx) {
            leaves.push_back(tx->GetHash());
        }
        size_t index = 0;
        std::vector<uint256> level = leaves;
        while (level.size() > 1) {
            size_t siblingIdx = index ^ 1;
            if (siblingIdx < level.size()) {
                coinbaseMerkleBranch.push_back(level[siblingIdx]);
            } else {
                coinbaseMerkleBranch.push_back(level[index]);
            }
            std::vector<uint256> nextLevel;
            for (size_t i = 0; i < level.size(); i += 2) {
                uint256 left = level[i];
                uint256 right =
                    (i + 1 < level.size()) ? level[i + 1] : left;
                nextLevel.push_back(Hash(Span(left), Span(right)));
            }
            level = nextLevel;
            index /= 2;
        }
    }

    // 4. Compute the chain merkle branch for this specific chain
    //    from the job's commitment data.
    //    For DOGE's own commitment, we use the stored branch directly.
    //    For other chains, we need to find the chain's leaf position and
    //    compute its branch in the aux merkle tree.
    const auto &commit = job.mergeCommitment;

    // 5. Serialize the CAuxPow
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);

    // coinbaseTx
    ss << coinbaseTx;

    // hashBlock (parent block's hash — the DOGE parent block header hash)
    uint256 parentBlockHash = parentHeader.GetHash();
    ss << parentBlockHash;

    // vMerkleBranch (coinbase merkle branch in the parent block)
    ss << coinbaseMerkleBranch;

    // nIndex (coinbase is always at index 0)
    ss << (uint32_t)0;

    // vChainMerkleBranch
    ss << commit.chainMerkleBranch;

    // nChainIndex
    ss << commit.nChainIndex;

    // parentBlock header
    ss << parentHeader;

    return HexStr(ss);
}

} // namespace stratum
