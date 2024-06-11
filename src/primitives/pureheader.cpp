// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/pureheader.h"

#include "chainparams.h"
#include "crypto/scrypt.h"
#include "hash.h"
//#include "utilstrencodings.h"

void CPureBlockHeader::SetBaseVersion(int32_t nBaseVersion, int32_t nChainId)
{
    assert(nBaseVersion >= 1 && nBaseVersion < VERSION_AUXPOW);
    assert(!IsAuxpow());
    nVersion = nBaseVersion | (nChainId * VERSION_CHAIN_START);
}

uint256 CPureBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

uint256 CPureBlockHeader::GetPoWHash() const
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << nVersion;
    ss << hashPrevBlock;
    ss << hashMerkleRoot;
    ss << nTime;
    ss << nBits;
    ss << nNonce;
    uint256 thash;
    scrypt_1024_1_1_256((const char *)ss.data(), (char *)(&thash));
    return thash;
}
