// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <primitives/baseheader.h>
#include <streams.h>
#include <util/strencodings.h>

BOOST_AUTO_TEST_SUITE(baseheader_tests)

BOOST_AUTO_TEST_CASE(genesis) {
    CBaseBlockHeader genesis;
    genesis.nVersion = 1;
    genesis.hashPrevBlock = BlockHash();
    genesis.hashMerkleRoot = uint256S(
        "5b2a3f53f605d62c53e62932dac6925e3d74afa5a4b459745c36d42d0ed26a69");
    genesis.nTime = 1386325540;
    genesis.nBits = 0x1e0ffff0;
    genesis.nNonce = 99943;
    BOOST_CHECK_EQUAL(
        genesis.GetHash().ToString(),
        "1a91e3dace36e2be3bf030a65679fe821aa1d6ef92e7c9902eb318182c355691");
    BOOST_CHECK_EQUAL(
        genesis.GetPowHash().ToString(),
        "0000026f3f7874ca0c251314eaed2d2fcf83d7da3acfaacf59417d485310b448");

    // Check serialization
    CDataStream ss(0, 0);
    ss << genesis;
    BOOST_CHECK_EQUAL(HexStr(ss),
                      "01000000000000000000000000000000000000000000000000000000"
                      "0000000000000000696ad20e2dd4365c7459b4a4a5af743d5e92c6da"
                      "3229e6532cd605f6533f2a5b24a6a152f0ff0f1e67860100");
}

BOOST_AUTO_TEST_CASE(block_145000) {
    CBaseBlockHeader block;
    block.nVersion = 2;
    block.hashPrevBlock = BlockHash(uint256S(
        "919a380db4b45eb97abb131633d87ff690387ebe03ac76690da3f4d681400558"));
    block.hashMerkleRoot = uint256S(
        "316614dcd65aa75888cfe1ebb2190740bd8d1fc3e30a0c1952062740b1419c33");
    block.nTime = 1395094679;
    block.nBits = 0x1b499dfd;
    block.nNonce = 1200826624;
    BOOST_CHECK_EQUAL(
        block.GetHash().ToString(),
        "cc47cae70d7c5c92828d3214a266331dde59087d4a39071fa76ddfff9b7bde72");
    BOOST_CHECK_EQUAL(
        block.GetPowHash().ToString(),
        "00000000002dfb87dd0d1b359eac948f33e91f87d586d36c497df2b08db7eb8a");

    // Check serialization
    CDataStream ss(0, 0);
    ss << block;
    BOOST_CHECK_EQUAL(HexStr(ss),
                      "0200000058054081d6f4a30d6976ac03be7e3890f67fd8331613bb7a"
                      "b95eb4b40d389a91339c41b140270652190c0ae3c31f8dbd400719b2"
                      "ebe1cf8858a75ad6dc14663197742753fd9d491b00299347");
}

BOOST_AUTO_TEST_CASE(block_371336) {
    // Block before the first merge-mined block
    CBaseBlockHeader block;
    block.nVersion = 0x00620002;
    block.hashPrevBlock = BlockHash(uint256S(
        "8ad58fc406423207bdd82bed27c0c9a22f8241e3d3e8595191decb55a50b20c7"));
    block.hashMerkleRoot = uint256S(
        "a0503bb44fd98e79239cc8f7b896a81b9a5fb1deb74e165173d7a530db34d877");
    block.nTime = 1410464569;
    block.nBits = 0x1b2fdf75;
    block.nNonce = 3401887720;
    BOOST_CHECK_EQUAL(
        block.GetHash().ToString(),
        "46a8b109fb016fa41abd17a19186ca78d39c60c020c71fcd2690320d47036f0d");
    BOOST_CHECK_EQUAL(
        block.GetPowHash().ToString(),
        "00000000000d07e96b781d6336d1badd7fc64ade5fd5957c596475a893d9b763");
}

BOOST_AUTO_TEST_CASE(block_371337) {
    // First merge-mined block
    CBaseBlockHeader block;
    block.nVersion = 0x00620102;
    block.hashPrevBlock = BlockHash(uint256S(
        "46a8b109fb016fa41abd17a19186ca78d39c60c020c71fcd2690320d47036f0d"));
    block.hashMerkleRoot = uint256S(
        "ee27b8fb782a5bfb99c975f0d4686440b9af9e16846603e5f2830e0b6fbf158a");
    block.nTime = 1410464577;
    block.nBits = 0x1b364184;
    block.nNonce = 0;
    BOOST_CHECK_EQUAL(
        block.GetHash().ToString(),
        "60323982f9c5ff1b5a954eac9dc1269352835f47c2c5222691d80f0d50dcf053");
    // Merge-mined block has no PoW done on the header itself
    BOOST_CHECK_EQUAL(
        block.GetPowHash().ToString(),
        "2486dafe34a0258425fbf7dd0c63b70f10c5803db63e9a61a1af5d2a2fc39146");
}

BOOST_AUTO_TEST_SUITE_END()
