// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/chainparams.h>

#include <chainparamsbase.h>
#include <chainparamsconstants.h>
#include <chainparamsseeds.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>

static CBlock CreateGenesisBlock(const char *pszTimestamp,
                                 const CScript &genesisOutputScript,
                                 uint32_t nTime, uint32_t nNonce,
                                 uint32_t nBits, int32_t nVersion,
                                 const Amount genesisReward) {
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig =
        CScript() << 486604799 << CScriptNum(4)
                  << std::vector<uint8_t>((const uint8_t *)pszTimestamp,
                                          (const uint8_t *)pszTimestamp +
                                              strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation transaction
 * cannot be spent since it did not originally exist in the database.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce,
                                 uint32_t nBits, int32_t nVersion,
                                 const Amount genesisReward) {
    const char *pszTimestamp = "Nintondo";
    const CScript genesisOutputScript =
        CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857f"
                              "bcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216"
                              "fe1b51850b4acf21b179c45070ac7b03a9")
                  << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce,
                              nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    explicit CMainParams(const ChainOptions &opts) {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 100000;
        // 00000000000000ce80a7e057163a4db1d5ad7b20fb6f598c9597b9665c8fb0d4 -
        // April 1, 2012
        consensus.BIP16Height = 0;
        consensus.BIP34Height = 1034383;
        consensus.BIP34Hash = BlockHash::fromHex(
            "80d1364201e5df97e696c03bdd24dc885e8617b9de51e453c10a4f629b1e797a");
        // 34cd2cbba4ba366f47e5aa0db5f02c19eba2adf679ceb6653ac003bdc9a0ef1f -
        // first v4 block after the last v3 block
        consensus.BIP65Height = 3464751;
        // 80d1364201e5df97e696c03bdd24dc885e8617b9de51e453c10a4f629b1e797a -
        // this is the last block that could be v2, 1900 blocks past the last v2
        // block
        consensus.BIP66Height = 1034383;
        // TODO: CSV not activated yet
        consensus.CSVHeight = 0x7fffffff;
        consensus.powLimit = uint256S("0x00000fffffffffffffffffffffffffffffffff"
                                      "ffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetSpacing = 60; // one minute
        consensus.fPowNoRetargeting = false;

        // two days
        consensus.nDAAHalfLife = 2 * 24 * 60 * 60;

        // Disable min difficulty rules on mainnet
        consensus.enableTestnetMinDifficulty = false;

        // Enforce strict chain ID on mainnet
        consensus.enforceStrictAuxPowChainId = true;

        // The miner fund is disabled by default on Dogecoin mainnet.
        consensus.enableMinerFund = false;

        // The staking rewards are disabled by default on Dogecoin mainnet.
        consensus.enableStakingRewards = false;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork =
            ChainParamsConstants::MAINNET_MINIMUM_CHAIN_WORK;

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid =
            ChainParamsConstants::MAINNET_DEFAULT_ASSUME_VALID;

        // BCH/XEC upgrades, disabled on Dogecoin
        // Avoid using 0x7fffffff since some tests would overflow
        consensus.uahfHeight = 0x7ffffffe;
        consensus.daaHeight = 0x7ffffffe;
        consensus.magneticAnomalyHeight = 0x7ffffffe;
        consensus.gravitonHeight = 0x7ffffffe;
        consensus.phononHeight = 0x7ffffffe;
        consensus.axionHeight = 0x7ffffffe;
        consensus.wellingtonHeight = 800000;    // keep alive for tests
        consensus.cowperthwaiteHeight = 900000; // keep alive for tests
        consensus.augustoActivationTime = 0x7ffffffe;

        // Dogecoin: Digishield activation height
        consensus.digishieldHeight = 145000;
        consensus.initialCoinbaseMaturity = 30;
        // Dogecoin: Enforce legacy script rules on mainnet
        consensus.enforceLegacyScriptRules = true;

        /**
         * The message start string is designed to be unlikely to occur in
         * normal data. The characters are rarely used upper ASCII, not valid as
         * UTF-8, and produce a large 32-bit integer with any alignment.
         */
        diskMagic[0] = 0xcb;
        diskMagic[1] = 0x98;
        diskMagic[2] = 0xa6;
        diskMagic[3] = 0xb0;
        netMagic[0] = 0xc0;
        netMagic[1] = 0xc0;
        netMagic[2] = 0xc0;
        netMagic[3] = 0xc0;
        nDefaultPort = 22556;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size =
            ChainParamsConstants::MAINNET_ASSUMED_BLOCKCHAIN_SIZE;
        m_assumed_chain_state_size =
            ChainParamsConstants::MAINNET_ASSUMED_CHAINSTATE_SIZE;

        genesis =
            CreateGenesisBlock(1386325540, 99943, 0x1e0ffff0, 1, 88 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256S("0x1a91e3dace36e2be3bf030a65679fe821aa1d6ef92e7c9902eb3"
                        "18182c355691"));
        assert(genesis.hashMerkleRoot ==
               uint256S("0x5b2a3f53f605d62c53e62932dac6925e3d74afa5a4b459745c36"
                        "d42d0ed26a69"));

        // Note that of those which support the service bits prefix, most only
        // support a subset of possible options. This is fine at runtime as
        // we'll fall back to using them as an addrfetch if they don't support
        // the service bits we want, but we should get them updated to support
        // all service bits wanted by any release ASAP to avoid it where
        // possible.
        vSeeds.emplace_back("seed.multidoge.org");
        vSeeds.emplace_back("seed2.multidoge.org");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 30);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 22);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 158);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x02, 0xfa, 0xca, 0xfd};
        base58Prefixes[EXT_SECRET_KEY] = {0x02, 0xfa, 0xc3, 0x98};
        cashaddrPrefix = opts.ecash ? "ecash" : "bitcoincash";

        vFixedSeeds = std::vector<SeedSpec6>(std::begin(pnSeed6_main),
                                             std::end(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = CheckpointData(CBaseChainParams::MAIN);

        m_assumeutxo_data = MapAssumeutxo{
            // TODO to be specified in a future patch.
        };

        // Data as of block
        // 000000000000000001d2ce557406b017a928be25ee98906397d339c3f68eec5d
        // (height 523992).
        chainTxData = ChainTxData{
            // UNIX timestamp of last known number of transactions.
            1522608016,
            // Total number of transactions between genesis and that timestamp
            // (the tx=... number in the ChainStateFlushed debug.log lines)
            248589038,
            // Estimated number of transactions per second after that timestamp.
            3.2,
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    explicit CTestNetParams(const ChainOptions &opts) {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 100000;
        // 00000000040b4e986385315e14bee30ad876d8b47f748025b26683116d21aa65
        consensus.BIP16Height = 0;
        consensus.BIP34Height = 708658;
        consensus.BIP34Hash = BlockHash::fromHex(
            "21b8b97dcdb94caa67c7f8f6dbf22e61e0cfe0e46e1fff3528b22864659e9b38");
        // 955bd496d23790aba1ecfacb722b089a6ae7ddabaedf7d8fb0878f48308a71f9
        consensus.BIP65Height = 1854705;
        // 21b8b97dcdb94caa67c7f8f6dbf22e61e0cfe0e46e1fff3528b22864659e9b38
        consensus.BIP66Height = 708658;
        // TODO: CSV not activated yet
        consensus.CSVHeight = 0x7fffffff;
        consensus.powLimit = uint256S("0x00000fffffffffffffffffffffffffffffffff"
                                      "ffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowNoRetargeting = false;

        // two days
        consensus.nDAAHalfLife = 2 * 24 * 60 * 60;

        // Enable min difficulty rules on testnet
        consensus.enableTestnetMinDifficulty = true;

        // Testnet has no strict chain ID
        consensus.enforceStrictAuxPowChainId = false;

        // The miner fund is disabled by default on testnet.
        consensus.enableMinerFund = false;

        // The staking rewards are disabled by default on testnet.
        consensus.enableStakingRewards = false;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork =
            ChainParamsConstants::TESTNET_MINIMUM_CHAIN_WORK;

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid =
            ChainParamsConstants::TESTNET_DEFAULT_ASSUME_VALID;

        // BCH/XEC upgrades, disabled on Dogecoin
        consensus.uahfHeight = 0x7ffffffe;
        consensus.daaHeight = 0x7ffffffe;
        consensus.magneticAnomalyHeight = 0x7ffffffe;
        consensus.gravitonHeight = 0x7ffffffe;
        consensus.phononHeight = 0x7ffffffe;
        consensus.axionHeight = 0x7ffffffe;
        consensus.wellingtonHeight = 0x7ffffffe;
        consensus.cowperthwaiteHeight = 0x7ffffffe;
        consensus.augustoActivationTime = 0x7ffffffe;

        // Dogecoin: Digishield activation height
        consensus.digishieldHeight = 145000;
        consensus.initialCoinbaseMaturity = 30;
        // Dogecoin: Enforce legacy script rules on testnet
        consensus.enforceLegacyScriptRules = true;

        diskMagic[0] = 0xfb;
        diskMagic[1] = 0x87;
        diskMagic[2] = 0xb5;
        diskMagic[3] = 0xbf;
        netMagic[0] = 0xfc;
        netMagic[1] = 0xc1;
        netMagic[2] = 0xb7;
        netMagic[3] = 0xdc;
        nDefaultPort = 44556;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size =
            ChainParamsConstants::TESTNET_ASSUMED_BLOCKCHAIN_SIZE;
        m_assumed_chain_state_size =
            ChainParamsConstants::TESTNET_ASSUMED_CHAINSTATE_SIZE;

        genesis =
            CreateGenesisBlock(1391503289, 997879, 0x1e0ffff0, 1, 88 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256S("0xbb0a78264637406b6360aad926284d544d7049f45189db5664f3"
                        "c4d07350559e"));
        assert(genesis.hashMerkleRoot ==
               uint256S("0x5b2a3f53f605d62c53e62932dac6925e3d74afa5a4b459745c36"
                        "d42d0ed26a69"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testseed.jrn.me.uk");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 113);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 241);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        cashaddrPrefix = opts.ecash ? "ectest" : "bchtest";

        vFixedSeeds = std::vector<SeedSpec6>(std::begin(pnSeed6_test),
                                             std::end(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = CheckpointData(CBaseChainParams::TESTNET);

        m_assumeutxo_data = MapAssumeutxo{
            // TODO to be specified in a future patch.
        };

        // Data as of block
        // 000000000005b07ecf85563034d13efd81c1a29e47e22b20f4fc6919d5b09cd6
        // (height 1223263)
        chainTxData = ChainTxData{1522608381, 15052068, 0.15};
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ChainOptions &opts) {
        strNetworkID = CBaseChainParams::REGTEST;
        consensus.nSubsidyHalvingInterval = 150;
        // always enforce P2SH BIP16 on regtest
        consensus.BIP16Height = 0;
        // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Height = 500;
        consensus.BIP34Hash = BlockHash();
        // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP65Height = 1351;
        // BIP66 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251;
        // CSV activated on regtest (Used in functional tests)
        consensus.CSVHeight = 576;
        consensus.powLimit = uint256S(
            "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowNoRetargeting = true;

        // two days
        consensus.nDAAHalfLife = 2 * 24 * 60 * 60;

        // Disable min difficulty rules on regtest
        consensus.enableTestnetMinDifficulty = false;

        // Enforce strict chain ID on regtest
        consensus.enforceStrictAuxPowChainId = true;

        // The miner fund is disabled by default on regtest.
        consensus.enableMinerFund = false;

        // The staking rewards are disabled by default on regtest.
        consensus.enableStakingRewards = false;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are
        // valid.
        consensus.defaultAssumeValid = BlockHash();

        // UAHF is always enabled on regtest.
        consensus.uahfHeight = 0;

        // November 13, 2017 hard fork is always on on regtest.
        consensus.daaHeight = 0;

        // November 15, 2018 hard fork is always on on regtest.
        consensus.magneticAnomalyHeight = 0;

        // November 15, 2019 protocol upgrade
        consensus.gravitonHeight = 0;

        // May 15, 2020 12:00:00 UTC protocol upgrade
        consensus.phononHeight = 0;

        // Nov 15, 2020 12:00:00 UTC protocol upgrade
        consensus.axionHeight = 0;

        // May 15, 2023 12:00:00 UTC protocol upgrade
        consensus.wellingtonHeight = 0;

        // Nov 15, 2023 12:00:00 UTC protocol upgrade
        consensus.cowperthwaiteHeight = 0;

        // Nov 15, 2024 12:00:00 UTC protocol upgrade
        consensus.augustoActivationTime = 1731672000;

        // Digishield activation height
        consensus.digishieldHeight = 1450;
        // keep maturity same as Bitcoin for tests
        consensus.initialCoinbaseMaturity = REGTEST_COINBASE_MATURITY;
        // legacy rules disabled for regtest so we don't refactor the universe
        consensus.enforceLegacyScriptRules = false;

        diskMagic[0] = 0x94;
        diskMagic[1] = 0xb1;
        diskMagic[2] = 0xca;
        diskMagic[3] = 0xd2;
        netMagic[0] = 0xda;
        netMagic[1] = 0xb5;
        netMagic[2] = 0xbf;
        netMagic[3] = 0xfa;
        nDefaultPort = 18444;
        nPruneAfterHeight = opts.fastprune ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 88 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256S("0x3d2160a3b5dc4a9d62e7e66a295f70313ac808440ef7400d6c07"
                        "72171ce973a5"));
        assert(genesis.hashMerkleRoot ==
               uint256S("0x5b2a3f53f605d62c53e62932dac6925e3d74afa5a4b459745c36"
                        "d42d0ed26a69"));

        //! Regtest mode doesn't have any fixed seeds.
        vFixedSeeds.clear();
        //! Regtest mode doesn't have any DNS seeds.
        vSeeds.clear();

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = CheckpointData(CBaseChainParams::REGTEST);

        m_assumeutxo_data = MapAssumeutxo{
            {
                110,
                {AssumeutxoHash{uint256S("0xfcfa07adecbe5f753b9f062b5e5621dcdd9"
                                         "f998a45968876cb98d350667d745e")},
                 110},
            },
            {
                210,
                {AssumeutxoHash{uint256S("0x6fa0d0be104a5990d6f743820b8a5e9eb7d"
                                         "525cc55e2bdb595d49e0cde33e0b5")},
                 210},
            },
        };

        chainTxData = ChainTxData{0, 0, 0};

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<uint8_t>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<uint8_t>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<uint8_t>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        cashaddrPrefix = opts.ecash ? "ecregtest" : "bchreg";
    }
};

std::unique_ptr<const CChainParams>
CChainParams::RegTest(const ChainOptions &options) {
    return std::make_unique<const CRegTestParams>(options);
}

std::unique_ptr<const CChainParams>
CChainParams::Main(const ChainOptions &options) {
    return std::make_unique<const CMainParams>(options);
}

std::unique_ptr<const CChainParams>
CChainParams::TestNet(const ChainOptions &options) {
    return std::make_unique<const CTestNetParams>(options);
}
