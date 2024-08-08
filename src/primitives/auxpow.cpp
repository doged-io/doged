// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/auxpow.h>
#include <stdexcept>
#include <tinyformat.h>

int32_t MakeVersionWithChainId(uint32_t nChainId, uint32_t nLowVersionBits) {
    // Ensure nChainId and nLowVersionBits are in a valid range
    if (nLowVersionBits >= VERSION_AUXPOW_BIT) {
        throw std::invalid_argument(
            strprintf("nLowVersionBits out of range: 0x%x >= 0x%x",
                      nLowVersionBits, VERSION_AUXPOW_BIT));
    }
    if (nChainId > MAX_ALLOWED_CHAIN_ID) {
        throw std::invalid_argument(
            strprintf("nChainId out of range: 0x%x > 0x%x", nChainId,
                      MAX_ALLOWED_CHAIN_ID));
    }
    return (nChainId << VERSION_CHAIN_ID_BIT_POS) | nLowVersionBits;
}

int32_t VersionWithAuxPow(uint32_t nVersion, bool hasAuxPow) {
    if (hasAuxPow) {
        return nVersion | VERSION_AUXPOW_BIT;
    } else {
        return nVersion & ~VERSION_AUXPOW_BIT;
    }
}
