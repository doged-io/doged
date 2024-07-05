// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/baseheader.h>

#include <chainparams.h>
#include <crypto/scrypt.h>
#include <hash.h>
#include <serialize.h>

BlockHash CBaseBlockHeader::GetHash() const {
    return BlockHash(SerializeHash(*this));
}

BlockHash CBaseBlockHeader::GetPowHash() const {
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *this;
    uint256 hash;
    scrypt_1024_1_1_256((const uint8_t *)ss.data(), hash.data());
    return BlockHash(hash);
}

void CBaseBlockHeader::SetVersionBits(int32_t nLowVersionBits,
                                      int32_t nChainId) {
    assert(nLowVersionBits >= 1 && nLowVersionBits < VERSION_AUXPOW_FLAG);
    assert(!HasAuxPowVersion());
    nVersion = nLowVersionBits | (nChainId << VERSION_CHAIN_ID_FIRST_BIT);
}
