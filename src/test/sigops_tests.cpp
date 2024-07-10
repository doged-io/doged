// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <script/script.h>
#include <script/sigops.h>
#include <script/standard.h>

BOOST_AUTO_TEST_SUITE(sigops_tests)

BOOST_AUTO_TEST_CASE(CountScriptSigOps_test) {
    CScript s1;
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ESTIMATED), 0);
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ACCURATE), 0);

    std::vector<uint8_t> dummy(20);

    s1 << OP_1 << dummy << dummy << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ACCURATE), 2);
    s1 << OP_IF << OP_CHECKSIG << OP_ENDIF;
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ACCURATE), 3);
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ESTIMATED), 21);
    s1 << OP_CHECKSIGVERIFY;
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ACCURATE), 4);
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ESTIMATED), 22);

    // OP_CHECKDATASIG and OP_CHECKDATASIGVERIFY don't count as SigOps
    s1 << OP_CHECKDATASIG;
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ACCURATE), 4);
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ESTIMATED), 22);
    s1 << OP_CHECKDATASIGVERIFY;
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ACCURATE), 4);
    BOOST_CHECK_EQUAL(CountScriptSigOps(s1, SigOpCountMode::ESTIMATED), 22);

    {
        // bare OP_CHECKMULTISIG has an "accurate" count of 20
        CScript s;
        s << OP_CHECKMULTISIG;
        BOOST_CHECK_EQUAL(CountScriptSigOps(s, SigOpCountMode::ACCURATE), 20);
        BOOST_CHECK_EQUAL(CountScriptSigOps(s, SigOpCountMode::ESTIMATED), 20);
    }

    {
        // 1-of-0 multisig has an "accurate" count of 20
        CScript s = GetScriptForMultisig(1, {});
        BOOST_CHECK_EQUAL(CountScriptSigOps(s, SigOpCountMode::ACCURATE), 20);
        BOOST_CHECK_EQUAL(CountScriptSigOps(s, SigOpCountMode::ESTIMATED), 20);
    }

    for (size_t i = 1; i <= 16; ++i) {
        std::vector<CPubKey> keys(i);
        CScript s = GetScriptForMultisig(1, keys);
        BOOST_CHECK_EQUAL(CountScriptSigOps(s, SigOpCountMode::ACCURATE), i);
        BOOST_CHECK_EQUAL(CountScriptSigOps(s, SigOpCountMode::ESTIMATED), 20);
    }
}

BOOST_AUTO_TEST_SUITE_END()
