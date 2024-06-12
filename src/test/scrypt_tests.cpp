// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <crypto/scrypt.h>
#include <uint256.h>
#include <util/strencodings.h>

BOOST_AUTO_TEST_SUITE(scrypt_tests)

BOOST_AUTO_TEST_CASE(scrypt_hashtest) {
    // Test Scrypt hash with known inputs against expected outputs

    struct TestCase {
        std::string input;
        std::string output;
    };

    const std::vector<TestCase> TEST_CASES = {
        {
            "020000004c1271c211717198227392b029a64a7971931d351b387bb80db027f270"
            "411e398a07046f7d4a08dd815412a8712f874a7ebf0507e3878bd24e20a3b73fd7"
            "50a667d2f451eac7471b00de6659",
            "00000000002bef4107f882f6115e0b01f348d21195dacd3582aa2dabd7985806",
        },
        {
            "0200000011503ee6a855e900c00cfdd98f5f55fffeaee9b6bf55bea9b852d9de2c"
            "e35828e204eef76acfd36949ae56d1fbe81c1ac9c0209e6331ad56414f9072506a"
            "77f8c6faf551eac7471b00389d01",
            "00000000003a0d11bdd5eb634e08b7feddcfbbf228ed35d250daf19f1c88fc94",
        },
        {
            "02000000a72c8a177f523946f42f22c3e86b8023221b4105e8007e59e81f6beb01"
            "3e29aaf635295cb9ac966213fb56e046dc71df5b3f7f67ceaeab24038e743f883a"
            "ff1aaafaf551eac7471b0166249b",
            "00000000000b40f895f288e13244728a6c2d9d59d8aff29c65f8dd5114a8ca81",
        },
        {
            "010000007824bc3a8a1b4628485eee3024abd8626721f7f870f8ad4d2f33a27155"
            "167f6a4009d1285049603888fe85a84b6c803a53305a8d497965a5e896e1a00568"
            "359589faf551eac7471b0065434e",
            "00000000003007005891cd4923031e99d8e8d72f6e8e7edc6a86181897e105fe",
        },
        {
            "0200000050bfd4e4a307a8cb6ef4aef69abc5c0f2d579648bd80d7733e1ccc3fbc"
            "90ed664a7f74006cb11bde87785f229ecd366c2d4e44432832580e0608c579e4cb"
            "76f383f7f551eac7471b00c36982",
            "000000000018f0b426a4afc7130ccb47fa02af730d345b4fe7c7724d3800ec8c",
        }};

#if defined(USE_SSE2)
    scrypt_detect_sse2();
#endif

    char scratchpad[SCRYPT_SCRATCHPAD_SIZE];
    for (const TestCase &test_case : TEST_CASES) {
        uint256 scrypthash;
        std::vector<uint8_t> input = ParseHex(test_case.input);

#if defined(USE_SSE2)
        // Test SSE2 scrypt
        scrypt_1024_1_1_256_sp_sse2((const char *)&input[0],
                                    (char *)scrypthash.data(), scratchpad);
        BOOST_CHECK_EQUAL(scrypthash.ToString().c_str(), test_case.output);
#endif

        // Test generic scrypt
        scrypt_1024_1_1_256_sp_generic((const char *)&input[0],
                                       (char *)scrypthash.data(), scratchpad);
        BOOST_CHECK_EQUAL(scrypthash.ToString().c_str(), test_case.output);
    }
}

BOOST_AUTO_TEST_SUITE_END()
