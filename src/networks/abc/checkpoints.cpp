// Copyright (c) 2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/system.h>

static CCheckpointData mainNetCheckpointData = {
    .mapCheckpoints = {
        {0, BlockHash::fromHex("0x1a91e3dace36e2be3bf030a65679fe821aa1d6ef92e7c"
                               "9902eb318182c355691")},
        {104679, BlockHash::fromHex("0x35eb87ae90d44b98898fec8c39577b76cb1eb08e"
                                    "1261cfc10706c8ce9a1d01cf")},
        {145000, BlockHash::fromHex("0xcc47cae70d7c5c92828d3214a266331dde59087d"
                                    "4a39071fa76ddfff9b7bde72")},
        {371337, BlockHash::fromHex("0x60323982f9c5ff1b5a954eac9dc1269352835f47"
                                    "c2c5222691d80f0d50dcf053")},
        {450000, BlockHash::fromHex("0xd279277f8f846a224d776450aa04da3cf978991a"
                                    "182c6f3075db4c48b173bbd7")},
        {771275, BlockHash::fromHex("0x1b7d789ed82cbdc640952e7e7a54966c6488a32e"
                                    "aad54fc39dff83f310dbaaed")},
        {1000000, BlockHash::fromHex("0x6aae55bea74235f0c80bd066349d4440c31f2d0"
                                     "f27d54265ecd484d8c1d11b47")},
        {1250000, BlockHash::fromHex("0x00c7a442055c1a990e11eea5371ca5c1c02a067"
                                     "7b33cc88ec728c45edc4ec060")},
        {1500000, BlockHash::fromHex("0xf1d32d6920de7b617d51e74bdf4e58adccaa582"
                                     "ffdc8657464454f16a952fca6")},
        {1750000, BlockHash::fromHex("0x5c8e7327984f0d6f59447d89d143e5f6eafc524"
                                     "c82ad95d176c5cec082ae2001")},
        {2000000, BlockHash::fromHex("0x9914f0e82e39bbf21950792e8816620d71b9965"
                                     "bdbbc14e72a95e3ab9618fea8")},
        {2031142, BlockHash::fromHex("0x893297d89afb7599a3c571ca31a3b80e8353f4c"
                                     "f39872400ad0f57d26c4c5d42")},
        {2250000, BlockHash::fromHex("0x0a87a8d4e40dca52763f93812a288741806380c"
                                     "d569537039ee927045c6bc338")},
        {2510150, BlockHash::fromHex("0x77e3f4a4bcb4a2c15e8015525e3d15b466f6c02"
                                     "2f6ca82698f329edef7d9777e")},
        {2750000, BlockHash::fromHex("0xd4f8abb835930d3c4f92ca718aaa09bef545076"
                                     "bd872354e0b2b85deefacf2e3")},
        {3000000, BlockHash::fromHex("0x195a83b091fb3ee7ecb56f2e63d01709293f57f"
                                     "971ccf373d93890c8dc1033db")},
        {3250000, BlockHash::fromHex("0x7f3e28bf9e309c4b57a4b70aa64d3b2ea5250ae"
                                     "797af84976ddc420d49684034")},
        {3500000, BlockHash::fromHex("0xeaa303b93c1c64d2b3a2cdcf6ccf21b10cc3662"
                                     "6965cc2619661e8e1879abdfb")},
        {3606083, BlockHash::fromHex("0x954c7c66dee51f0a3fb1edb26200b735f5275fe"
                                     "54d9505c76ebd2bcabac36f1e")},
        {3854173, BlockHash::fromHex("0xe4b4ecda4c022406c502a247c0525480268ce7a"
                                     "bbbef632796e8ca1646425e75")},
        {3963597, BlockHash::fromHex("0x2b6927cfaa5e82353d45f02be8aadd3bfd165ec"
                                     "e5ce24b9bfa4db20432befb5d")},
        {4303965, BlockHash::fromHex("0xed7d266dcbd8bb8af80f9ccb8deb3e18f9cc3f6"
                                     "972912680feeb37b090f8cee0")},
        {5050000, BlockHash::fromHex("0xe7d4577405223918491477db725a393bcfc349d"
                                     "8ee63b0a4fde23cbfbfd81dea")},
    }};

static CCheckpointData testNetCheckpointData = {
    .mapCheckpoints = {
        {0, BlockHash::fromHex("0xbb0a78264637406b6360aad926284d544d7049f45189db5664f3c4d07350559e")},
        {483173, BlockHash::fromHex("0xa804201ca0aceb7e937ef7a3c613a9b7589245b10cc095148c4ce4965b0b73b5")},
        {591117, BlockHash::fromHex("0x5f6b93b2c28cedf32467d900369b8be6700f0649388a7dbfd3ebd4a01b1ffad8")},
        {658924, BlockHash::fromHex("0xed6c8324d9a77195ee080f225a0fca6346495e08ded99bcda47a8eea5a8a620b")},
        {703635, BlockHash::fromHex("0x839fa54617adcd582d53030a37455c14a87a806f6615aa8213f13e196230ff7f")},
        {1000000, BlockHash::fromHex("0x1fe4d44ea4d1edb031f52f0d7c635db8190dc871a190654c41d2450086b8ef0e")},
        {1202214, BlockHash::fromHex("0xa2179767a87ee4e95944703976fee63578ec04fa3ac2fc1c9c2c83587d096977")},
        {1250000, BlockHash::fromHex("0xb46affb421872ca8efa30366b09694e2f9bf077f7258213be14adb05a9f41883")},
        {1500000, BlockHash::fromHex("0x0caa041b47b4d18a4f44bdc05cef1a96d5196ce7b2e32ad3e4eb9ba505144917")},
        {1750000, BlockHash::fromHex("0x8042462366d854ad39b8b95ed2ca12e89a526ceee5a90042d55ebb24d5aab7e9")},
        {2000000, BlockHash::fromHex("0xd6acde73e1b42fc17f29dcc76f63946d378ae1bd4eafab44d801a25be784103c")},
        {2250000, BlockHash::fromHex("0xc4342ae6d9a522a02e5607411df1b00e9329563ef844a758d762d601d42c86dc")},
        {2500000, BlockHash::fromHex("0x3a66ec4933fbb348c9b1889aaf2f732fe429fd9a8f74fee6895eae061ac897e2")},
        {2750000, BlockHash::fromHex("0x473ea9f625d59f534ffcc9738ffc58f7b7b1e0e993078614f5484a9505885563")},
        {3062910, BlockHash::fromHex("0x113c41c00934f940a41f99d18b2ad9aefd183a4b7fe80527e1e6c12779bd0246")},
        {3286675, BlockHash::fromHex("0x07fef07a255d510297c9189dc96da5f4e41a8184bc979df8294487f07fee1cf3")},
        {3445426, BlockHash::fromHex("0x70574db7856bd685abe7b0a8a3e79b29882620645bd763b01459176bceb58cd1")},
        {3976284, BlockHash::fromHex("0xaf23c3e750bb4f2ce091235f006e7e4e2af453d4c866282e7870471dcfeb4382")},
        {5900000, BlockHash::fromHex("0x199bea6a442310589cbb50a193a30b097c228bd5a0f21af21e4e53dd57c382d3")},
    }};

static CCheckpointData regTestCheckpointData = {
    .mapCheckpoints = {
        {0, BlockHash::fromHex("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb4"
                               "36012afca590b1a11466e2206")},
    }};

const CCheckpointData &CheckpointData(const std::string &chain) {
    if (chain == CBaseChainParams::MAIN) {
        return mainNetCheckpointData;
    }
    if (chain == CBaseChainParams::TESTNET) {
        return testNetCheckpointData;
    }
    if (chain == CBaseChainParams::REGTEST) {
        return regTestCheckpointData;
    }

    throw std::runtime_error(
        strprintf("%s: Unknown chain %s.", __func__, chain));
}
