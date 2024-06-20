// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_AUXPOW_H
#define BITCOIN_POW_AUXPOW_H

class CBlockHeader;

namespace Consensus {
struct Params;
} // namespace Consensus

bool CheckAuxProofOfWork(const CBlockHeader &block,
                         const Consensus::Params &params);

#endif // BITCOIN_POW_AUXPOW_H
