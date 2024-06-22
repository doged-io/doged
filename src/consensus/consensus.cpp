// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/activation.h>
#include <consensus/consensus.h>

#include <consensus/params.h>

int32_t CoinbaseMaturity(const Consensus::Params &params, int32_t nHeight) {
    if (IsDigishieldEnabled(params, nHeight)) {
        return DIGISHIELD_COINBASE_MATURITY;
    }

    return params.initialCoinbaseMaturity;
}
