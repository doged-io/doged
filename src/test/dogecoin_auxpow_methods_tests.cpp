// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow/auxpow.h>
#include <pow/pow.h>
#include <primitives/auxpow.h>
#include <streams.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(dogecoin_auxpow_methods_tests)

constexpr size_t BASE_HEADER_SIZE = 80;
constexpr size_t NULL_TX_SIZE = 4 + 1 + 1 + 4;
constexpr size_t HASH_SIZE = 32;
constexpr size_t NULL_AUXPOW_HEADER_SIZE = BASE_HEADER_SIZE + NULL_TX_SIZE +
                                           HASH_SIZE + 1 + 4 + 1 + 4 +
                                           BASE_HEADER_SIZE;

BOOST_AUTO_TEST_CASE(auxpow_block_setnull_test) {
    // Zero'd merge-mined block
    std::vector<uint8_t> nullAuxPowBlockBytes(NULL_AUXPOW_HEADER_SIZE + 1);
    nullAuxPowBlockBytes[1] = 1; // set auxpow bit
    CDataStream ss{nullAuxPowBlockBytes, SER_NETWORK, PROTOCOL_VERSION};

    CBlock block;
    ss >> block;

    BOOST_CHECK(VersionHasAuxPow(block.nVersion));
    BOOST_CHECK(block.auxpow);
    BOOST_CHECK(block.GetBlockHeader().auxpow);

    // Test SetNull also resets the auxpow
    block.SetNull();

    BOOST_CHECK(!VersionHasAuxPow(block.nVersion));
    BOOST_CHECK(!block.auxpow);
    BOOST_CHECK(!block.GetBlockHeader().auxpow);
}

BOOST_AUTO_TEST_CASE(auxpow_blockheader_setnull_test) {
    // Zero'd merge-mined block header
    std::vector<uint8_t> nullAuxPowBlockHeaderBytes(NULL_AUXPOW_HEADER_SIZE);
    nullAuxPowBlockHeaderBytes[1] = 1; // set auxpow bit
    CDataStream ss{nullAuxPowBlockHeaderBytes, SER_NETWORK, PROTOCOL_VERSION};

    CBlockHeader header;
    ss >> header;

    BOOST_CHECK(VersionHasAuxPow(header.nVersion));
    BOOST_CHECK(header.auxpow);

    // Test SetNull also resets the auxpow
    header.SetNull();

    BOOST_CHECK(!VersionHasAuxPow(header.nVersion));
    BOOST_CHECK(!header.auxpow);
}

BOOST_AUTO_TEST_SUITE_END()
