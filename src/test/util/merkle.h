// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_MERKLE_H
#define BITCOIN_TEST_UTIL_MERKLE_H

/**
 * This implements a constant-space merkle root/path calculator, limited to 2^32
 * leaves.
 */
void MerkleComputation(const std::vector<uint256> &leaves, uint256 *proot,
                       bool *pmutated, uint32_t branchpos,
                       std::vector<uint256> *pbranch);

std::vector<uint256> ComputeMerkleBranch(const std::vector<uint256> &leaves,
                                         uint32_t position);

std::vector<uint256> BlockMerkleBranch(const CBlock &block, uint32_t position);

#endif // BITCOIN_TEST_UTIL_MERKLE_H
