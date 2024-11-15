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

BOOST_AUTO_TEST_SUITE(dogecoin_auxpow_serialize_tests)

BOOST_AUTO_TEST_CASE(auxpow_serialize_block_5462519_test) {
    // Merge-mined block at height 5462519
    std::string hexHeader =
        "040162005e840cdf3fec777d0d634c8e9c92f6a2ea8930fd8b48a0dbb5f2357a9f4f1d"
        "e657c1358bd1f89e699d7c07dcc44de34c7bf1a27ea28edd2d6cb884f7d15288e2dae7"
        "3667d901011a0000000001000000010000000000000000000000000000000000000000"
        "000000000000000000000000ffffffff6403f6992a2cfabe6d6dd81f3988043c358152"
        "86caf34112119f102a40f5254b38ccd8a26034e20a5b0208000000f09f909f092f4632"
        "506f6f6c2f610000000000000000000000000000000000000000000000000000000000"
        "0000000000000500db3784010000000002a62e8f25000000001976a914f2910ecaf7bb"
        "8d18ed71f0904e0e7456f29ce18288ac0000000000000000266a24aa21a9ed801e68db"
        "7a74fb3b4d488c0434637c970f57e7f163928f7f70379e8105cbc97e2d571a42601f1e"
        "481b6c99ed95f6fe403cd27fbbc50946e730b79e64e30000000000000008b464254251"
        "342ff2fde346bc0cd59c9b2408d80a445b939d88c38b04f68e6f156f898d3a5f1d5817"
        "64292bf9627aa94d3610bda497d085ce5e724d37218511f677afd5410d3d222bb16e3c"
        "9f6812bc99033358e80ab5e34c635c524af1b4c27136f4be10555a7994fc90e0c01484"
        "0e50dc26466ccff91ce2882ebe3fa0a6c307092a3b4cd9ffcef9f5c4d662a18b54ddad"
        "38a8384888ece87bc0244dadffbbcad0f02d09a1155ea29b3c93afadd8341a23452966"
        "d43af767876ace612015e7b1ef572ff80f874d6f1b8fe9debbbc9bff2f82b45336c1df"
        "916400cd08e6817e2612653f629a170b856a787a1dd4f4de70c44905760cf2287bee56"
        "fda72472118e00000000031fe0dfb97e716ec277ac600ff9c485831f2ef52a4478a797"
        "704b06dfb838192f4bd17ef3ab1e19336a217f754ef38b3e96b9da39c79bfce6b51e9d"
        "42ce34bbd1e8c100c21e0533acf748e8372ca702edea7ce12e4a5ec5067f4587b05963"
        "e3df0000000014000020914f47f57dbd460a348179763e195fbae41090f7e87a1836e6"
        "0b9f876e32a6ea4093045657cce77921dcb897ea71bbeff7fa6ace8e7c4020b9f3b7ca"
        "f3be35cb11e83667db555c195945a9ff";
    CDataStream ss{ParseHex(hexHeader), SER_NETWORK, PROTOCOL_VERSION};

    CMutableTransaction tx;
    tx.nVersion = 1;
    tx.nLockTime = 1109022509;
    tx.vin.resize(1);
    tx.vout.resize(2);
    std::vector<uint8_t> coinbase = ParseHex(
        "03f6992a2cfabe6d6dd81f3988043c35815286caf34112119f102a40f5254b38ccd8a2"
        "6034e20a5b0208000000f09f909f092f4632506f6f6c2f610000000000000000000000"
        "0000000000000000000000000000000000000000000000000500db378401");
    tx.vin[0].scriptSig = CScript(coinbase.begin(), coinbase.end());
    tx.vin[0].nSequence = 0;
    tx.vout[0].nValue = 630140582 * Amount::satoshi();
    tx.vout[0].scriptPubKey =
        CScript() << OP_DUP << OP_HASH160
                  << ParseHex("f2910ecaf7bb8d18ed71f0904e0e7456f29ce182")
                  << OP_EQUALVERIFY << OP_CHECKSIG;
    tx.vout[1].nValue = Amount::zero();
    tx.vout[1].scriptPubKey =
        CScript() << OP_RETURN
                  << ParseHex("aa21a9ed801e68db7a74fb3b4d488c0434637c970f57e7f1"
                              "63928f7f70379e8105cbc97e");

    CBlockHeader header;
    ss >> header;

    BOOST_CHECK_EQUAL(header.GetHash(),
                      uint256S("26e1a3ef7e6e34ece7bb9e09324dca34678af1bcfcdb7a8"
                               "10e53cb12249c0a6a"));
    BOOST_CHECK_EQUAL(header.nVersion, 0x00620104);
    BOOST_CHECK_EQUAL(header.hashPrevBlock,
                      BlockHash(uint256S("e61d4f9f7a35f2b5dba0488bfd3089eaa2f69"
                                         "29c8e4c630d7d77ec3fdf0c845e")));
    BOOST_CHECK_EQUAL(header.hashMerkleRoot,
                      uint256S("e28852d1f784b86c2ddd8ea27ea2f17b4ce34dc4dc077c9"
                               "d699ef8d18b35c157"));
    BOOST_CHECK_EQUAL(header.nTime, 1731651546);
    BOOST_CHECK_EQUAL(header.nBits, 0x1a0101d9);
    BOOST_CHECK_EQUAL(header.nNonce, 0);

    const CAuxPow &auxpow = *header.auxpow;

    BOOST_CHECK_MESSAGE(*auxpow.coinbaseTx == CTransaction(tx),
                        strprintf("%s != %s", auxpow.coinbaseTx->ToString(),
                                  CTransaction(tx).ToString()));
    const uint256 hashBlock = uint256S(
        "00000000000000e3649eb730e74609c5bb7fd23c40fef695ed996c1b481e1f60");
    BOOST_CHECK_EQUAL(auxpow.hashBlock, hashBlock);

    std::vector<std::string> merkleBranchHex{
        "156f8ef6048bc3889d935b440ad808249b9cd50cbc46e3fdf22f3451422564b4",
        "f6118521374d725ece85d097a4bd10364da97a62f92b296417581d5f3a8d896f",
        "71c2b4f14a525c634ce3b50ae858330399bc12689f3c6eb12b223d0d41d5af77",
        "07c3a6a03fbe2e88e21cf9cf6c4626dc500e8414c0e090fc94795a5510bef436",
        "cabbffad4d24c07be8ec884838a838addd548ba162d6c4f5f9ceffd94c3b2a09",
        "b1e7152061ce6a8767f73ad4662945231a34d8adaf933c9ba25e15a1092df0d0",
        "267e81e608cd006491dfc13653b4822fff9bbcbbdee98f1b6f4d870ff82f57ef",
        "8e117224a7fd56ee7b28f20c760549c470def4d41d7a786a850b179a623f6512"};
    std::vector<uint256> merkleBranch;
    for (const std::string &hex : merkleBranchHex) {
        merkleBranch.push_back(uint256S(hex));
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(auxpow.vMerkleBranch.begin(),
                                  auxpow.vMerkleBranch.end(),
                                  merkleBranch.begin(), merkleBranch.end());

    std::vector<std::string> chainMerkleBranchHex{
        "2f1938b8df064b7097a778442af52e1f8385c4f90f60ac77c26e717eb9dfe01f",
        "d1bb34ce429d1eb5e6fc9bc739dab9963e8bf34e757f216a33191eabf37ed14b",
        "dfe36359b087457f06c55e4a2ee17ceaed02a72c37e848f7ac33051ec200c1e8"};
    std::vector<uint256> chainMerkleBranch;
    for (const std::string &hex : chainMerkleBranchHex) {
        chainMerkleBranch.push_back(uint256S(hex));
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(
        auxpow.vChainMerkleBranch.begin(), auxpow.vChainMerkleBranch.end(),
        chainMerkleBranch.begin(), chainMerkleBranch.end());

    BOOST_CHECK_EQUAL(auxpow.nIndex, 0);
    BOOST_CHECK_EQUAL(auxpow.nChainIndex, 0);

    CBaseBlockHeader parentHeader;
    parentHeader.nVersion = 0x20000014;
    parentHeader.hashPrevBlock = BlockHash(uint256S(
        "eaa6326e879f0be636187ae8f79010e4ba5f193e767981340a46bd7df5474f91"));
    parentHeader.hashMerkleRoot = uint256S(
        "cb35bef3cab7f3b920407c8ece6afaf7efbb71ea97b8dc2179e7cc5756049340");
    parentHeader.nTime = 1731651601;
    parentHeader.nBits = 0x195c55db;
    parentHeader.nNonce = 4289283417;

    BOOST_CHECK_EQUAL(auxpow.parentBlock.GetHash(), parentHeader.GetHash());
    BOOST_CHECK_EQUAL(auxpow.parentBlock.GetPowHash(), hashBlock);

    CDataStream ss2{SER_NETWORK, PROTOCOL_VERSION};
    ss2 << header;
    BOOST_CHECK_EQUAL(HexStr(std::vector<std::byte>(ss2.begin(), ss2.end())),
                      hexHeader);
}

BOOST_AUTO_TEST_SUITE_END()
