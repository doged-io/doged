// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <feerate.h>
#include <policy/fees.h>

#include <tinyformat.h>

CFeeRate::CFeeRate(const Amount nFeePaid, size_t nBytes_) {
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    if (nFeePaid > MAX_MONEY / 1000) {
        // This computation will not fit in Amount. Since it is not realistic,
        // just set the max fee rate.
        nSatoshisPerK = MAX_FEERATE;
    } else if (nSize > 0) {
        nSatoshisPerK = (1000 * nFeePaid) / nSize;
    } else {
        nSatoshisPerK = Amount::zero();
    }
}

template <bool ceil>
static Amount GetFee(size_t nBytes_, Amount nSatoshisPerK) {
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    // Ensure fee is rounded up when truncated if ceil is true.
    Amount nFee = Amount::zero();
    if (ceil) {
        nFee = Amount(nSize * nSatoshisPerK % 1000 > Amount::zero()
                          ? nSize * nSatoshisPerK / 1000 + SATOSHI
                          : nSize * nSatoshisPerK / 1000);
    } else {
        nFee = nSize * nSatoshisPerK / 1000;
    }

    if (nFee == Amount::zero() && nSize != 0) {
        if (nSatoshisPerK > Amount::zero()) {
            nFee = SATOSHI;
        }
        if (nSatoshisPerK < Amount::zero()) {
            nFee = -SATOSHI;
        }
    }

    return nFee;
}

Amount CFeeRate::GetFee(size_t nBytes) const {
    return ::GetFee<false>(nBytes, nSatoshisPerK);
}

Amount CFeeRate::GetFeeCeiling(size_t nBytes) const {
    return ::GetFee<true>(nBytes, nSatoshisPerK);
}

std::string CFeeRate::ToString() const {
    const auto currency = Currency::get();
    return strprintf("%d.%0*d %s/kB", nSatoshisPerK / currency.baseunit,
                     currency.decimals,
                     (nSatoshisPerK % currency.baseunit) / currency.subunit,
                     currency.ticker);
}
