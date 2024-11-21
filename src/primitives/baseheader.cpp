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
    uint8_t bytes[80];
    size_t idx = 0;
    uint32_t version = this->nVersion;
    for (size_t i = 0; i < 4; ++i) {
        bytes[idx++] = version & 0xff;
        version >>= 8;
    }
    for (uint8_t byte : this->hashPrevBlock) {
        bytes[idx++] = byte;
    }
    for (uint8_t byte : this->hashMerkleRoot) {
        bytes[idx++] = byte;
    }
    uint32_t time = this->nTime;
    for (size_t i = 0; i < 4; ++i) {
        bytes[idx++] = time & 0xff;
        time >>= 8;
    }
    uint32_t bits = this->nBits;
    for (size_t i = 0; i < 4; ++i) {
        bytes[idx++] = bits & 0xff;
        bits >>= 8;
    }
    uint32_t nonce = this->nNonce;
    for (size_t i = 0; i < 4; ++i) {
        bytes[idx++] = nonce & 0xff;
        nonce >>= 8;
    }
    uint256 hash;
    scrypt_1024_1_1_256(bytes, hash.data());
    return BlockHash(hash);
}
