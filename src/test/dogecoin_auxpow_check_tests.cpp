// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/auxpow.h>
#include <span.h>
#include <streams.h>
#include <util/strencodings.h>

#include <test/lcg.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(dogecoin_auxpow_check_tests)

// Block eac853ae22d59a498386241a3de69a36739ccc9e0a6acfd617b64c5ea4a0f4b3
// chainindex=56
std::string hexHeader700000 =
    "020162000c1194ac4c5d3826887eb8d97dc4ae02a25af30d5b504720febe4e047627217ec9"
    "224c3a9a91e4018801d1da0b981e0445b3812413e32ef891e3d7772a5ef17ea43e4e5566d7"
    "051b0000000001000000010000000000000000000000000000000000000000000000000000"
    "000000000000ffffffff57034bdd0be4b883e5bda9e7a59ee4bb99e9b1bcfabe6d6d1f0e6e"
    "c774ba83111dfe17e85be8e292092395050340aee13909bb5169f949114000000000000000"
    "031f881c0000000f4d696e65642062792061327468726565ffffffff0100f2052a01000000"
    "1976a914aa3750aa18b8a0f3f0590731e1fab934856680cf88ac0000000010ba25eadde6eb"
    "39add569420d1fcb08df83df645f7f148a2d230500000000000346f64bde86fb0444949f57"
    "4b2752ec82bafd7aa3599365e810638f87b2032e8d4124586ac0c16abc93a866107624236c"
    "a8ec933991e0f1db424e1f5cac260e412515793c469236b0f308345ef47b28ae0c45d4f953"
    "02a0e3ba84a970cf8252630000000006eb27cdf762126701d420dae67f9e0117751b098bc7"
    "9f207d505edb385ec73ae46c4127661ba7d68c453b449868e6135a6ac0d9351a3e40d7dd58"
    "767531de67ee31284c19194806e9c00943e05a9d1a79e17fa0c9b79bef9027ad2ca0bdf99e"
    "abfe2d8f99be8b35640357d1af6ec9884840d0a9d91dbac1a8334df680016151ffc154ad6a"
    "9b6e61c74b83dedd12a907e63a9c425fa1b199388196cb078c44d0c3a9a43398b7a93b294a"
    "6c16b6e352b15a50cd7ce001de6d0cc82cb3e6c179f908380000000200000016e121811e18"
    "8728f2aa586df32525412d76f9777cb7395ba20b5c40b3210711365ad72ba0e3d75af58431"
    "d47bb16febd0366b775faf384128e7740a99edd898c73e4e558ab0011b19215530";

// Block 773fbb34e1bfe82467eb24cda8769dfdcd13a5b4dac4c8f9f6534c40301f7fbf
// chainindex=40
std::string hexHeader800000 =
    "020162001bcb8498f1f6e084b81754f04ebefe123dac7f908bda324b7a4d3aa10c4d889139"
    "c8fcf6f95419dfd3d16fd1256db0b9f0ffa653a3b275067e3560cd72622990bd19ae553ab4"
    "061b0000000001000000010000000000000000000000000000000000000000000000000000"
    "000000000000ffffffff640337810ce4b883e5bda9e7a59ee4bb99e9b1bcfabe6d6d5c10b6"
    "060a131b10b9b2bbc56580f8d134d7328244350cc4b445037a51e15b0840000000f09f909f"
    "00000000000000000000000000000000000000000000000000000000000000000000000000"
    "0000001101f0ac0a2a010000001976a914aa3750aa18b8a0f3f0590731e1fab934856680cf"
    "88ac2f08c93853c72fd016e289473fea98933fd2484885cd036aad1891138b990100000000"
    "000340bf1ca21e44fec1f0c4d51f1913bfec389dc42a4e50abeb90dc6a7b71a20693603bd3"
    "b9048e8d0b5a34b67b7142314233914975936c989850b44fb36a3623aadfbbcb57b6d3e00f"
    "9e0b38153c403775974f5cba60e179a4d19f82992e2bfa6900000000065d01f4d3c900798b"
    "5a0a9955c4a710bd18b5a8efe95275d52ed9dd383c7040f6463ceed131958d98aee29089d1"
    "cf38b9728b224512e51ca3a8b1189d5ed03d0709b68fd6e328528f2a29ec7fb077c834fbf0"
    "f14c371fafcfb27444017fbf5b26fdb884bed8ad6a4bded36fc89ed8b05a6c6c0ae1cfd5fe"
    "37eb3021b32a1e29042b7a2e142329e7d0d0bffcb5cc338621a576b49d4d32991000b8d4ac"
    "793bc1f522c5c55826f53583f4924086f9a6f3ef3adca07aafd832d3ed883b7fcad80e8d28"
    "00000003000000340b110b4bb8169d370bb5a37a64d0eeb7f01952d92f50115d866b688406"
    "b6204ff1897acd7d56c25b954db1379243723414ae007be32ec19685cbc848b0987bbe19ae"
    "550a7f011bdd117b68";

// Block 195a83b091fb3ee7ecb56f2e63d01709293f57f971ccf373d93890c8dc1033db
// chainindex=8
std::string hexHeader3000000 =
    "03016200f9a4d3ecc3ae92f6287c7b45e9c5d749200863231005fceb377fa020038c9b2301"
    "05bb642ed4d23ce7b6b537e9da8b866ca610bab77021d80b721464d7e0736bcd11e25d10da"
    "071a0000000001000000010000000000000000000000000000000000000000000000000000"
    "000000000000ffffffff5503a4a01a41d778847a32eef241d77884793136cd2f4c54432e54"
    "4f502ffabe6d6d9bfb6d1004d9a7f7a21288c39dac1f23a67b4f4c939b52431699b926bdd9"
    "7d7010000000000000006813cdc80300000000000000ffffffff023894854a000000001976"
    "a9140c617fdb2ea42aed30a509595ee21ba3f6688db088ac0000000000000000266a24aa21"
    "a9ed6e2181c6f858307ffb827d72ad45382cce763facfc251b59998fe92892d6f400000000"
    "0064a056193968d8a9d04e089bcc8350f8fc83248dd3df35d842685ce0942b03ce04f7b5cc"
    "f83492a71e6b52dcbec6bec951e7101d729c006c5f69b9474a02f9b29c99a8c120342cf5c1"
    "1c5d99e4f7929f76ed965cedefd2190184b3feb930af5dbeb373539bc2bfd7c9119d64218c"
    "7eae5f8413a291f745c954b4b3c9e1a717c29bd75cb5d3a89cc40a8f968ad11e5b59daae02"
    "8ee4e448700428de5b4ec3222f9d00000000045899bf3c423b15f8dbb5dda1490b3668c563"
    "6e45191d6ad9a19fc4e56633d91bdbc0d080019eed31e838bb40a413dfe25708411c8e11c5"
    "f2849a6ec032c1d4d1c3e38c249a5c68bbfb9a281ebc99bdacce51bedb38de6d7df0371912"
    "65fee0bd822688e534f559de234e7a798cac3b6ea5538488b731bac3693df77c092118e008"
    "00000000000020979d5d65c1e40bab403720505b8fe9130aaa7e647c1449f331bfc52054a6"
    "ad314ddcb6a03b62930a4d05b60ddd0221122a31b8e7cb9f5b3ca45baccf76d0840de411e2"
    "5d09e3021ab13d0e90";

BOOST_AUTO_TEST_CASE(auxpow_block700000_test) {
    CDataStream ss{ParseHex(hexHeader700000), SER_NETWORK, PROTOCOL_VERSION};
    CBlockHeader header;
    ss >> header;
    const CAuxPow &auxpow = *header.auxpow;

    uint256 hashRoot = ComputeMerkleRootForBranch(
        header.GetHash(), auxpow.vChainMerkleBranch, auxpow.nChainIndex);

    ParsedAuxPowCoinbase parsed = *ParsedAuxPowCoinbase::Parse(
        auxpow.coinbaseTx->vin[0].scriptSig, hashRoot);

    BOOST_CHECK_EQUAL(parsed.nTreeSize, 64);
    BOOST_CHECK_EQUAL(parsed.nMergeMineNonce, 0);
    BOOST_CHECK_EQUAL(
        CalcExpectedMerkleTreeIndex(parsed.nMergeMineNonce,
                                    VersionChainId(header.nVersion),
                                    auxpow.vChainMerkleBranch.size()),
        56);

    Consensus::Params params = CChainParams::Main({})->GetConsensus();
    BOOST_CHECK(auxpow.CheckAuxBlockHash(
        header.GetHash(), VersionChainId(header.nVersion), params));
}

BOOST_AUTO_TEST_CASE(auxpow_block800000_test) {
    CDataStream ss{ParseHex(hexHeader800000), SER_NETWORK, PROTOCOL_VERSION};
    CBlockHeader header;
    ss >> header;
    const CAuxPow &auxpow = *header.auxpow;

    uint256 hashRoot = ComputeMerkleRootForBranch(
        header.GetHash(), auxpow.vChainMerkleBranch, auxpow.nChainIndex);

    ParsedAuxPowCoinbase parsed = *ParsedAuxPowCoinbase::Parse(
        auxpow.coinbaseTx->vin[0].scriptSig, hashRoot);

    BOOST_CHECK_EQUAL(parsed.nTreeSize, 64);
    BOOST_CHECK_EQUAL(parsed.nMergeMineNonce, 2677055472);
    BOOST_CHECK_EQUAL(
        CalcExpectedMerkleTreeIndex(parsed.nMergeMineNonce,
                                    VersionChainId(header.nVersion),
                                    auxpow.vChainMerkleBranch.size()),
        40);

    Consensus::Params params = CChainParams::Main({})->GetConsensus();
    BOOST_CHECK(auxpow.CheckAuxBlockHash(
        header.GetHash(), VersionChainId(header.nVersion), params));
}

BOOST_AUTO_TEST_CASE(auxpow_block3000000_test) {
    CDataStream ss{ParseHex(hexHeader3000000), SER_NETWORK, PROTOCOL_VERSION};
    CBlockHeader header;
    ss >> header;
    const CAuxPow &auxpow = *header.auxpow;

    uint256 hashRoot = ComputeMerkleRootForBranch(
        header.GetHash(), auxpow.vChainMerkleBranch, auxpow.nChainIndex);

    ParsedAuxPowCoinbase parsed = *ParsedAuxPowCoinbase::Parse(
        auxpow.coinbaseTx->vin[0].scriptSig, hashRoot);

    BOOST_CHECK_EQUAL(parsed.nTreeSize, 16);
    BOOST_CHECK_EQUAL(parsed.nMergeMineNonce, 0);
    BOOST_CHECK_EQUAL(
        CalcExpectedMerkleTreeIndex(parsed.nMergeMineNonce,
                                    VersionChainId(header.nVersion),
                                    auxpow.vChainMerkleBranch.size()),
        8);

    Consensus::Params params = CChainParams::Main({})->GetConsensus();
    BOOST_CHECK(auxpow.CheckAuxBlockHash(
        header.GetHash(), VersionChainId(header.nVersion), params));
}

BOOST_AUTO_TEST_CASE(auxpow_parse_coinbase_test) {
    BOOST_CHECK_EQUAL(
        ErrorString(ParsedAuxPowCoinbase::Parse(CScript(), uint256())).original,
        "AuxPow missing chain merkle root in parent coinbase");

    // Hash needs to be big-endian
    BOOST_CHECK_EQUAL(ErrorString(ParsedAuxPowCoinbase::Parse(
                                      CScript() << ToByteVector(uint256S("1")),
                                      uint256S("1")))
                          .original,
                      "AuxPow missing chain merkle root in parent coinbase");

    uint256 testHash = uint256S(
        "cdab907856341290785634129078563412907856341290785634129078563412");
    uint256 testHashBE = uint256S(
        "123456789012345678901234567890123456789012345678901234567890abcd");
    // Big-endian hash found
    {
        CScript coinbase = CScript() << ToByteVector(testHashBE);
        BOOST_CHECK_EQUAL(
            ErrorString(ParsedAuxPowCoinbase::Parse(coinbase, testHash))
                .original,
            "AuxPow missing chain merkle tree size and nonce in parent "
            "coinbase");
    }

    // Only one merge mine prefix allowed
    for (size_t numPrefixes : {2, 3, 4, 5, 10, 20}) {
        CScript coinbase = CScript() << ToByteVector(testHashBE);
        for (size_t i = 0; i < numPrefixes; ++i) {
            coinbase = coinbase << ToByteVector(MERGE_MINE_PREFIX);
        }
        BOOST_CHECK_EQUAL(
            ErrorString(ParsedAuxPowCoinbase::Parse(coinbase, testHash))
                .original,
            "Multiple merged mining prefixes in coinbase");
    }

    // Hash must be right after the prefix (not before)
    {
        CScript coinbase = CScript() << ToByteVector(testHashBE)
                                     << ToByteVector(MERGE_MINE_PREFIX);
        BOOST_CHECK_EQUAL(
            ErrorString(ParsedAuxPowCoinbase::Parse(coinbase, testHash))
                .original,
            "Merged mining prefix is not just before chain merkle root");
    }

    // Hash must be right after the prefix (not with any bytes in between)
    for (size_t sizePad = 1; sizePad < 100; ++sizePad) {
        std::vector<uint8_t> coinbaseBytes(MERGE_MINE_PREFIX.begin(),
                                           MERGE_MINE_PREFIX.end());
        std::vector<uint8_t> pad(sizePad);
        coinbaseBytes.insert(coinbaseBytes.end(), pad.begin(), pad.end());
        coinbaseBytes.insert(coinbaseBytes.end(), testHashBE.begin(),
                             testHashBE.end());
        CScript coinbase(coinbaseBytes.begin(), coinbaseBytes.end());
        BOOST_CHECK_EQUAL(
            ErrorString(ParsedAuxPowCoinbase::Parse(coinbase, testHash))
                .original,
            "Merged mining prefix is not just before chain merkle root");
    }

    // Found prefix + root hash (with 0 bytes in between)
    {
        std::vector<uint8_t> coinbaseBytes = ToByteVector(MERGE_MINE_PREFIX);
        coinbaseBytes.insert(coinbaseBytes.end(), testHashBE.begin(),
                             testHashBE.end());
        CScript coinbase(coinbaseBytes.begin(), coinbaseBytes.end());
        BOOST_CHECK_EQUAL(
            ErrorString(ParsedAuxPowCoinbase::Parse(coinbase, testHash))
                .original,
            "AuxPow missing chain merkle tree size and nonce in parent "
            "coinbase");
    }

    // Backwards compatibility: hash without prefix allowed
    BOOST_CHECK_EQUAL(
        ErrorString(
            ParsedAuxPowCoinbase::Parse(
                CScript(testHashBE.begin(), testHashBE.end()), testHash))
            .original,
        "AuxPow missing chain merkle tree size and nonce in parent coinbase");

    // Hash must be within the first 20 bytes
    for (size_t sizePad = 0; sizePad <= 100; ++sizePad) {
        std::vector<uint8_t> coinbaseBytes(sizePad);
        coinbaseBytes.insert(coinbaseBytes.end(), testHashBE.begin(),
                             testHashBE.end());
        CScript coinbase(coinbaseBytes.begin(), coinbaseBytes.end());
        BOOST_CHECK_EQUAL(
            ErrorString(ParsedAuxPowCoinbase::Parse(coinbase, testHash))
                .original,
            sizePad <= 20
                ? "AuxPow missing chain merkle tree size and nonce in parent "
                  "coinbase"
                : "AuxPow chain merkle root can have at most 20 preceding "
                  "bytes of the parent coinbase");
    }

    // One byte missing for tree size and nonce (must be 8 bytes)
    {
        std::vector<uint8_t> coinbaseBytes = ToByteVector(MERGE_MINE_PREFIX);
        coinbaseBytes.insert(coinbaseBytes.end(), testHashBE.begin(),
                             testHashBE.end());
        std::vector<uint8_t> data(7);
        coinbaseBytes.insert(coinbaseBytes.end(), data.begin(), data.end());
        CScript coinbase(coinbaseBytes.begin(), coinbaseBytes.end());
        BOOST_CHECK_EQUAL(
            ErrorString(ParsedAuxPowCoinbase::Parse(coinbase, testHash))
                .original,
            "AuxPow missing chain merkle tree size and nonce in parent "
            "coinbase");
    }

    // Successful parse (with prefix)
    {
        std::vector<uint8_t> coinbaseBytes = ToByteVector(MERGE_MINE_PREFIX);
        coinbaseBytes.insert(coinbaseBytes.end(), testHashBE.begin(),
                             testHashBE.end());
        std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
        coinbaseBytes.insert(coinbaseBytes.end(), data.begin(), data.end());
        CScript coinbase(coinbaseBytes.begin(), coinbaseBytes.end());
        ParsedAuxPowCoinbase parsed =
            *ParsedAuxPowCoinbase::Parse(coinbase, testHash);
        BOOST_CHECK_EQUAL(parsed.nTreeSize, 0x4030201);
        BOOST_CHECK_EQUAL(parsed.nMergeMineNonce, 0x08070605);
    }

    // Successful parse (without prefix)
    {
        std::vector<uint8_t> coinbaseBytes(testHashBE.begin(),
                                           testHashBE.end());
        std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
        coinbaseBytes.insert(coinbaseBytes.end(), data.begin(), data.end());
        CScript coinbase(coinbaseBytes.begin(), coinbaseBytes.end());
        ParsedAuxPowCoinbase parsed =
            *ParsedAuxPowCoinbase::Parse(coinbase, testHash);
        BOOST_CHECK_EQUAL(parsed.nTreeSize, 0x4030201);
        BOOST_CHECK_EQUAL(parsed.nMergeMineNonce, 0x08070605);
    }

    // Successful parse (with prefix and some extra padding)
    for (size_t sizePad = 0; sizePad <= 100; ++sizePad) {
        // If we have a prefix, any number of bytes before it are allowed
        std::vector<uint8_t> coinbaseBytes(sizePad);
        coinbaseBytes.insert(coinbaseBytes.end(), MERGE_MINE_PREFIX.begin(),
                             MERGE_MINE_PREFIX.end());
        coinbaseBytes.insert(coinbaseBytes.end(), testHashBE.begin(),
                             testHashBE.end());
        std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 0xff, 0xff, 0xff};
        coinbaseBytes.insert(coinbaseBytes.end(), data.begin(), data.end());
        CScript coinbase(coinbaseBytes.begin(), coinbaseBytes.end());
        ParsedAuxPowCoinbase parsed =
            *ParsedAuxPowCoinbase::Parse(coinbase, testHash);
        BOOST_CHECK_EQUAL(parsed.nTreeSize, 0x4030201);
        BOOST_CHECK_EQUAL(parsed.nMergeMineNonce, 0x08070605);
    }

    // Successful parse (without prefix and some extra padding)
    for (size_t sizePad = 0; sizePad <= 100; ++sizePad) {
        std::vector<uint8_t> coinbaseBytes(sizePad);
        coinbaseBytes.insert(coinbaseBytes.end(), testHashBE.begin(),
                             testHashBE.end());
        std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 0xff, 0xff, 0xff};
        coinbaseBytes.insert(coinbaseBytes.end(), data.begin(), data.end());
        CScript coinbase(coinbaseBytes.begin(), coinbaseBytes.end());
        util::Result<ParsedAuxPowCoinbase> parsed =
            ParsedAuxPowCoinbase::Parse(coinbase, testHash);
        if (sizePad <= 20) {
            BOOST_CHECK(parsed.has_value());
            BOOST_CHECK_EQUAL(parsed->nTreeSize, 0x4030201);
            BOOST_CHECK_EQUAL(parsed->nMergeMineNonce, 0x08070605);
        } else {
            BOOST_CHECK_EQUAL(ErrorString(parsed).original,
                              "AuxPow chain merkle root can have at most 20 "
                              "preceding bytes of the parent coinbase");
        }
    }
}

BOOST_AUTO_TEST_CASE(CalcExpectedMerkleTreeIndex_test) {
    // Block 564415
    BOOST_CHECK_EQUAL(CalcExpectedMerkleTreeIndex(0, AUXPOW_CHAIN_ID, 5), 24);
    // Block 750100
    BOOST_CHECK_EQUAL(
        CalcExpectedMerkleTreeIndex(0x77654e2f, AUXPOW_CHAIN_ID, 6), 63);
    // Block 805660
    BOOST_CHECK_EQUAL(
        CalcExpectedMerkleTreeIndex(0x9f909ff0, AUXPOW_CHAIN_ID, 6), 40);
    // Block 845783
    BOOST_CHECK_EQUAL(CalcExpectedMerkleTreeIndex(0, AUXPOW_CHAIN_ID, 11),
                      1080);
}

BOOST_AUTO_TEST_CASE(CheckAuxBlockHash_test) {
    Consensus::Params mainParams = CChainParams::Main({})->GetConsensus();
    Consensus::Params testParams = CChainParams::TestNet({})->GetConsensus();
    Consensus::Params regParams = CChainParams::RegTest({})->GetConsensus();

    const std::vector<Consensus::Params> allParams{mainParams, testParams,
                                                   regParams};

    // nIndex must be 0
    for (uint32_t nIndex : {1u, 2u, 100u, 0xffffffffu, 0x7fffffffu}) {
        CAuxPow auxpow{};
        auxpow.nIndex = nIndex;
        for (const Consensus::Params &params : allParams) {
            BOOST_CHECK_EQUAL(
                ErrorString(auxpow.CheckAuxBlockHash(uint256(), 0, params))
                    .original,
                "AuxPow nIndex must be 0");
        }
    }

    // vChainMerkleBranch can at most be 30
    for (size_t branchLen = 31; branchLen <= 100; ++branchLen) {
        CAuxPow auxpow{};
        auxpow.nIndex = 0;
        auxpow.vChainMerkleBranch.resize(branchLen);
        for (const Consensus::Params &params : allParams) {
            BOOST_CHECK_EQUAL(
                ErrorString(auxpow.CheckAuxBlockHash(uint256(), 1, params))
                    .original,
                "AuxPow chain merkle branch too long");
        }
    }

    // If a strict chain ID is enforced, we don't allow the parent to have the
    // same chain ID as our chain.
    for (uint32_t nChainId : {0u, 1u, 2u, 0xffffu, uint32_t(AUXPOW_CHAIN_ID)}) {
        CAuxPow auxpow{};
        auxpow.nIndex = 0;
        auxpow.parentBlock.nVersion = MakeVersionWithChainId(nChainId, 0);
        auxpow.vChainMerkleBranch.resize(31);
        // Enforced on mainnet
        BOOST_CHECK_EQUAL(ErrorString(auxpow.CheckAuxBlockHash(
                                          uint256(), nChainId, mainParams))
                              .original,
                          "AuxPow parent has our chain ID");
        // Enforced on regtest
        BOOST_CHECK_EQUAL(ErrorString(auxpow.CheckAuxBlockHash(
                                          uint256(), nChainId, regParams))
                              .original,
                          "AuxPow parent has our chain ID");
        // Not enforced on testnet (so we fail on the merkle branch check)
        BOOST_CHECK_EQUAL(ErrorString(auxpow.CheckAuxBlockHash(
                                          uint256(), nChainId, testParams))
                              .original,
                          "AuxPow chain merkle branch too long");
    }

    // coinbaseTx is not in the parentBlock (or merkle proof incorrect)
    for (size_t branchLen = 0; branchLen < 31; ++branchLen) {
        CAuxPow auxpow{};
        auxpow.nIndex = 0;
        auxpow.coinbaseTx = MakeTransactionRef();
        auxpow.vMerkleBranch.resize(branchLen);
        auxpow.parentBlock.hashMerkleRoot = uint256S(
            "123456789012345678901234567890123456789012345678901234567890abcd");
        for (const Consensus::Params &params : allParams) {
            BOOST_CHECK_EQUAL(
                ErrorString(auxpow.CheckAuxBlockHash(uint256(), 1, params))
                    .original,
                "AuxPow merkle root incorrect");
        }
    }

    // Coinbase can't have no inputs
    {
        CAuxPow auxpow{};
        auxpow.nIndex = 0;
        auxpow.coinbaseTx = MakeTransactionRef();
        auxpow.vMerkleBranch.resize(7);
        auxpow.parentBlock.hashMerkleRoot = ComputeMerkleRootForBranch(
            auxpow.coinbaseTx->GetHash(), auxpow.vMerkleBranch, 0);
        for (const Consensus::Params &params : allParams) {
            BOOST_CHECK_EQUAL(
                ErrorString(auxpow.CheckAuxBlockHash(uint256(), 1, params))
                    .original,
                "AuxPow coinbase transaction missing input");
        }
    }

    // Coinbase must have the required auxpow data in the scriptSig
    {
        CAuxPow auxpow{};
        auxpow.nIndex = 0;
        CMutableTransaction tx;
        tx.vin.resize(1);
        auxpow.coinbaseTx = MakeTransactionRef(tx);
        auxpow.vMerkleBranch.resize(7);
        auxpow.parentBlock.hashMerkleRoot = ComputeMerkleRootForBranch(
            auxpow.coinbaseTx->GetHash(), auxpow.vMerkleBranch, 0);
        for (const Consensus::Params &params : allParams) {
            BOOST_CHECK_EQUAL(
                ErrorString(auxpow.CheckAuxBlockHash(uint256(), 1, params))
                    .original,
                "AuxPow missing chain merkle root in parent coinbase");
        }
    }

    // Must set nTreeSize to 2^vChainMerkleBranch.size()
    for (uint32_t nChainIndex = 0; nChainIndex < 128; ++nChainIndex) {
        uint256 hashChildBlock = uint256S(
            "123456789012345678901234567890123456789012345678901234567890abcd");

        CAuxPow auxpow{};
        auxpow.nIndex = 0;
        auxpow.nChainIndex = nChainIndex;
        auxpow.vChainMerkleBranch.resize(7);
        uint256 hashChainRoot = ComputeMerkleRootForBranch(
            hashChildBlock, auxpow.vChainMerkleBranch, nChainIndex);
        std::reverse(hashChainRoot.begin(), hashChainRoot.end());

        std::vector<uint8_t> coinbaseBytes = ToByteVector(MERGE_MINE_PREFIX);
        coinbaseBytes.insert(coinbaseBytes.end(), hashChainRoot.begin(),
                             hashChainRoot.end());
        std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
        coinbaseBytes.insert(coinbaseBytes.end(), data.begin(), data.end());

        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].scriptSig =
            CScript(coinbaseBytes.begin(), coinbaseBytes.end());

        auxpow.coinbaseTx = MakeTransactionRef(tx);
        auxpow.vMerkleBranch.resize(5);
        auxpow.parentBlock.hashMerkleRoot = ComputeMerkleRootForBranch(
            auxpow.coinbaseTx->GetHash(), auxpow.vMerkleBranch, 0);

        for (const Consensus::Params &params : allParams) {
            BOOST_CHECK_EQUAL(
                ErrorString(auxpow.CheckAuxBlockHash(hashChildBlock, 1, params))
                    .original,
                "AuxPow merkle branch size does not match parent coinbase");
        }
    }

    // Test every chain index (for merkle height 7, this is 128).
    // Merge-mining requries us to have nChainIndex to be a specific value based
    // on the merge-mine nonce, chain ID and the merkle height, so for one of
    // these values, the check will succeed, and for the others, it will error
    // with "wrong chain index"
    for (uint32_t nChainIndex = 0; nChainIndex < 128; ++nChainIndex) {
        const uint32_t merkleHeight = 7;
        uint256 hashChildBlock = uint256S(
            "123456789012345678901234567890123456789012345678901234567890abcd");

        CAuxPow auxpow{};
        auxpow.nIndex = 0;
        auxpow.nChainIndex = nChainIndex;
        auxpow.vChainMerkleBranch.resize(7);

        uint256 hashChainRoot = ComputeMerkleRootForBranch(
            hashChildBlock, auxpow.vChainMerkleBranch, nChainIndex);
        std::reverse(hashChainRoot.begin(), hashChainRoot.end());

        std::vector<uint8_t> coinbaseBytes = ToByteVector(MERGE_MINE_PREFIX);
        coinbaseBytes.insert(coinbaseBytes.end(), hashChainRoot.begin(),
                             hashChainRoot.end());
        std::vector<uint8_t> data = {1 << merkleHeight, 0, 0, 0, 5, 6, 7, 8};
        coinbaseBytes.insert(coinbaseBytes.end(), data.begin(), data.end());

        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].scriptSig =
            CScript(coinbaseBytes.begin(), coinbaseBytes.end());

        auxpow.coinbaseTx = MakeTransactionRef(tx);
        auxpow.vMerkleBranch.resize(5);
        auxpow.parentBlock.hashMerkleRoot = ComputeMerkleRootForBranch(
            auxpow.coinbaseTx->GetHash(), auxpow.vMerkleBranch, 0);

        for (const Consensus::Params &params : allParams) {
            const uint32_t expectedChainIndex = CalcExpectedMerkleTreeIndex(
                0x08070605, AUXPOW_CHAIN_ID, merkleHeight);
            if (expectedChainIndex == nChainIndex) {
                BOOST_CHECK(auxpow.CheckAuxBlockHash(hashChildBlock,
                                                     AUXPOW_CHAIN_ID, params));
            } else {
                BOOST_CHECK_EQUAL(
                    ErrorString(auxpow.CheckAuxBlockHash(
                                    hashChildBlock, AUXPOW_CHAIN_ID, params))
                        .original,
                    "AuxPow wrong chain index");
            }
        }
    }
}

uint256 GenHash(MMIXLinearCongruentialGenerator &lcg) {
    uint256 hash;
    for (uint8_t &byte : hash) {
        byte = lcg.next();
    }
    return hash;
}

BOOST_AUTO_TEST_CASE(CheckAuxBlockHash_rng_test) {
    const std::vector<Consensus::Params> allParams{
        CChainParams::Main({})->GetConsensus(),
        CChainParams::TestNet({})->GetConsensus(),
        CChainParams::RegTest({})->GetConsensus()};

    MMIXLinearCongruentialGenerator lcg;
    CDataStream ss{0, 0};
    // Randomly generate a lot of configurations and test for successes.
    for (size_t testCase = 0; testCase < 2048; testCase++) {
        // Generate random parameters
        const uint32_t chainId = lcg.next() % 0x10000;
        const uint32_t parentChainId = (chainId + 1) % 0x10000;
        const uint32_t chainMerkleHeight = lcg.next() % 31;
        const uint32_t chainTreeSize = 1 << chainMerkleHeight;
        const uint32_t nMergeMineNonce = lcg.next();
        const uint256 hashChildBlock = GenHash(lcg);
        const uint32_t blockMerkleHeight = lcg.next() % 32;
        const uint32_t versionLowBits = lcg.next() % 256;

        CAuxPow auxpow{};
        auxpow.vChainMerkleBranch.resize(chainMerkleHeight);
        for (uint256 &hash : auxpow.vChainMerkleBranch) {
            hash = GenHash(lcg);
        }
        auxpow.vMerkleBranch.resize(blockMerkleHeight);
        for (uint256 &hash : auxpow.vMerkleBranch) {
            hash = GenHash(lcg);
        }

        auxpow.nIndex = 0;
        auxpow.nChainIndex = CalcExpectedMerkleTreeIndex(
            nMergeMineNonce, chainId, chainMerkleHeight);

        uint256 hashChainRoot = ComputeMerkleRootForBranch(
            hashChildBlock, auxpow.vChainMerkleBranch, auxpow.nChainIndex);
        std::reverse(hashChainRoot.begin(), hashChainRoot.end());

        std::vector<uint8_t> coinbaseBytes = ToByteVector(MERGE_MINE_PREFIX);
        coinbaseBytes.insert(coinbaseBytes.end(), hashChainRoot.begin(),
                             hashChainRoot.end());
        ss.clear();
        ss << chainTreeSize;
        ss << nMergeMineNonce;
        Span<const uint8_t> span = MakeUCharSpan(ss);
        coinbaseBytes.insert(coinbaseBytes.end(), span.begin(), span.end());

        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].scriptSig =
            CScript(coinbaseBytes.begin(), coinbaseBytes.end());

        auxpow.coinbaseTx = MakeTransactionRef(tx);
        auxpow.parentBlock.nVersion =
            MakeVersionWithChainId(parentChainId, versionLowBits);
        auxpow.parentBlock.hashMerkleRoot = ComputeMerkleRootForBranch(
            auxpow.coinbaseTx->GetHash(), auxpow.vMerkleBranch, 0);

        for (const Consensus::Params &params : allParams) {
            util::Result<std::monostate> result =
                auxpow.CheckAuxBlockHash(hashChildBlock, chainId, params);
            BOOST_CHECK_MESSAGE(result, ErrorString(result).original);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
