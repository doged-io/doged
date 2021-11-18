/**
 * @generated by contrib/devtools/chainparams/generate_chainparams_constants.py
 */

#include <chainparamsconstants.h>

namespace ChainParamsConstants {
    const BlockHash MAINNET_DEFAULT_ASSUME_VALID = BlockHash::fromHex("0000000000000000043b6fd1461f15b7372283ebdc60b43684ba73060ad98ed2");
    const uint256 MAINNET_MINIMUM_CHAIN_WORK = uint256S("00000000000000000000000000000000000000000158146fcdbf945efb23c2c2");
    const uint64_t MAINNET_ASSUMED_BLOCKCHAIN_SIZE = 210;
    const uint64_t MAINNET_ASSUMED_CHAINSTATE_SIZE = 3;

    const BlockHash TESTNET_DEFAULT_ASSUME_VALID = BlockHash::fromHex("00000000000d4f9b905976e424f4f3606c3e122fbb6b33f24a7e09b622cf3690");
    const uint256 TESTNET_MINIMUM_CHAIN_WORK = uint256S("00000000000000000000000000000000000000000000006e85e4f789382786d4");
    const uint64_t TESTNET_ASSUMED_BLOCKCHAIN_SIZE = 55;
    const uint64_t TESTNET_ASSUMED_CHAINSTATE_SIZE = 2;
} // namespace ChainParamsConstants

