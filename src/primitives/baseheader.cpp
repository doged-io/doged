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
    uint256 thash;
    scrypt_1024_1_1_256((const uint8_t *)ss.data(), thash.data());
    return BlockHash(thash);
}
