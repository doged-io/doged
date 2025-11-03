// Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRATUM_STRATUMSUBMIT_H
#define BITCOIN_STRATUM_STRATUMSUBMIT_H

#include <arith_uint256.h>
#include <consensus/params.h>
#include <primitives/baseheader.h>
#include <primitives/blockhash.h>
#include <stratum/stratumjob.h>
#include <univalue.h>
#include <util/result.h>

#include <cstdint>
#include <set>
#include <string>

class ChainstateManager;

namespace stratum {

enum class ShareResult {
    ACCEPTED,
    ACCEPTED_BLOCK,
    REJECTED_LOW_DIFF,
    REJECTED_STALE,
    REJECTED_DUPLICATE,
    REJECTED_INVALID,
};

struct ShareSubmission {
    std::string workerName;
    uint64_t jobId;
    std::string extranonce2; // hex
    std::string ntime;       // hex, 4 bytes
    std::string nonce;       // hex, 4 bytes
};

/** Parse mining.submit params into ShareSubmission. */
util::Result<ShareSubmission> ParseSubmitParams(const UniValue &params);

/**
 * Reconstruct the complete coinbase transaction from coinbase1 + extranonce1 +
 * extranonce2 + coinbase2, recompute the merkle root, and build the full
 * 80-byte block header.
 */
CBaseBlockHeader ReconstructHeader(const StratumJob &job,
                                   const std::string &extranonce1,
                                   const ShareSubmission &sub);

/** Compute Scrypt PoW hash of a block header. */
BlockHash ScryptPowHash(const CBaseBlockHeader &header);

/**
 * Convert a Stratum difficulty value to a 256-bit target.
 * Stratum difficulty 1 corresponds to target 0x00000000ffff... (pdiff).
 */
arith_uint256 DifficultyToTarget(double difficulty);

/**
 * Validate a submitted share:
 * 1. Reconstruct header from job + extranonce1 + submission
 * 2. Scrypt hash the header
 * 3. Compare against worker difficulty target (share) and network target (block)
 */
ShareResult ValidateShare(const StratumJob &job,
                          const std::string &extranonce1,
                          const ShareSubmission &sub,
                          double workerDifficulty,
                          const Consensus::Params &params,
                          std::set<std::string> &submittedNonces);

/**
 * Assemble the full CBlock from job + submission and call ProcessNewBlock.
 */
bool SubmitBlock(const StratumJob &job, const std::string &extranonce1,
                 const ShareSubmission &sub, ChainstateManager &chainman,
                 const CChainParams &chainParams);

/** Stratum error codes per specification. */
namespace StratumError {
static constexpr int JOB_NOT_FOUND = 21;
static constexpr int DUPLICATE_SHARE = 22;
static constexpr int LOW_DIFFICULTY = 23;
static constexpr int UNAUTHORIZED = 24;
static constexpr int NOT_SUBSCRIBED = 25;
} // namespace StratumError

} // namespace stratum

#endif // BITCOIN_STRATUM_STRATUMSUBMIT_H
