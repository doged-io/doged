// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <coins.h>
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

BOOST_AUTO_TEST_CASE(CountScriptSigOpsP2SH_test) {
    BOOST_CHECK_EQUAL(CountScriptSigOpsP2SH(CScript()), 0);
    BOOST_CHECK_EQUAL(CountScriptSigOpsP2SH(CScript() << OP_CHECKSIG), 0);

    std::vector<uint8_t> dummy(20);

    {
        CScript script;
        script << OP_CHECKSIG;
        std::vector<uint8_t> scriptBytes(script.begin(), script.end());
        BOOST_CHECK_EQUAL(CountScriptSigOpsP2SH(CScript() << scriptBytes), 1);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << dummy << scriptBytes), 1);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << scriptBytes << dummy), 0);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << OP_CHECKSIG << scriptBytes), 0);
    }

    {
        CScript script;
        script << OP_CHECKSIG << OP_CHECKSIGVERIFY;
        std::vector<uint8_t> scriptBytes(script.begin(), script.end());
        BOOST_CHECK_EQUAL(CountScriptSigOpsP2SH(CScript() << scriptBytes), 2);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << dummy << scriptBytes), 2);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << scriptBytes << dummy), 0);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << OP_CHECKSIG << scriptBytes), 0);
    }

    {
        CScript script;
        script << OP_1 << dummy << dummy << OP_4 << OP_CHECKMULTISIG;
        std::vector<uint8_t> scriptBytes(script.begin(), script.end());
        BOOST_CHECK_EQUAL(CountScriptSigOpsP2SH(CScript() << scriptBytes), 4);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << dummy << scriptBytes), 4);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << scriptBytes << dummy), 0);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << OP_CHECKSIG << scriptBytes), 0);
    }

    {
        CScript script;
        script << OP_CHECKMULTISIG;
        std::vector<uint8_t> scriptBytes(script.begin(), script.end());
        BOOST_CHECK_EQUAL(CountScriptSigOpsP2SH(CScript() << scriptBytes), 20);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << dummy << scriptBytes), 20);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << scriptBytes << dummy), 0);
        BOOST_CHECK_EQUAL(
            CountScriptSigOpsP2SH(CScript() << OP_CHECKSIG << scriptBytes), 0);
    }
}

/**
 * Builds a creationTx from scriptPubKey and a spendingTx from scriptSig
 * such that spendingTx spends output zero of creationTx. Also inserts
 * creationTx's output into the coins view.
 */
static void BuildTxs(CMutableTransaction &spendingTx, CCoinsViewCache &coins,
                     CMutableTransaction &creationTx,
                     const CScript &scriptPubKey, const CScript &scriptSig) {
    creationTx.nVersion = 1;
    creationTx.vin.resize(1);
    creationTx.vin[0].prevout = COutPoint();
    creationTx.vin[0].scriptSig = CScript();
    creationTx.vout.resize(1);
    creationTx.vout[0].nValue = SATOSHI;
    creationTx.vout[0].scriptPubKey = scriptPubKey;

    spendingTx.nVersion = 1;
    spendingTx.vin.resize(1);
    spendingTx.vin[0].prevout = COutPoint(creationTx.GetId(), 0);
    spendingTx.vin[0].scriptSig = scriptSig;
    spendingTx.vout.resize(1);
    spendingTx.vout[0].nValue = SATOSHI;
    spendingTx.vout[0].scriptPubKey = CScript();

    AddCoins(coins, CTransaction(creationTx), 0);
}

BOOST_AUTO_TEST_CASE(CountTxSigOps_test) {
    // Transaction creates outputs
    CMutableTransaction creationTx;
    // Transaction that spends outputs and whose sig op count is going to be
    // tested
    CMutableTransaction spendingTx;

    std::vector<uint8_t> dummy(20);

    // Coinbase
    {
        CCoinsView coinsDummy;
        CCoinsViewCache coins(&coinsDummy);

        // Coinbase with a OP_CHECKSIG in the sig script is counted
        creationTx.nVersion = 1;
        creationTx.vin.resize(1);
        creationTx.vin[0].prevout = COutPoint();
        creationTx.vin[0].scriptSig = CScript() << OP_CHECKSIG;
        CTransaction createTx(creationTx);
        BOOST_CHECK_EQUAL(CountTxSigOps(createTx, coins), 1);
        BOOST_CHECK_EQUAL(CountTxNonP2SHSigOps(createTx), 1);
        BOOST_CHECK_EQUAL(CountTxP2SHSigOps(createTx, coins), 0);

        // Coinbase with something looking like a P2SH redeemScript not counted
        creationTx.vin[0].scriptSig = CScript()
                                      << ToByteVector(CScript() << OP_CHECKSIG);
        CTransaction createTx2(creationTx);
        BOOST_CHECK_EQUAL(CountTxSigOps(createTx2, coins), 0);
        BOOST_CHECK_EQUAL(CountTxNonP2SHSigOps(createTx2), 0);
        BOOST_CHECK_EQUAL(CountTxP2SHSigOps(createTx2, coins), 0);
    }

    // Multisig script (legacy counting)
    {
        CCoinsView coinsDummy;
        CCoinsViewCache coins(&coinsDummy);

        CScript scriptPubKey = CScript() << 1 << dummy << dummy << 2
                                         << OP_CHECKMULTISIGVERIFY;
        // Do not use a valid signature to avoid using wallet operations.
        CScript scriptSig = CScript() << OP_0 << OP_0;

        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig);
        CTransaction createTx(creationTx);
        CTransaction spendTx(spendingTx);

        // Legacy counting only includes signature operations in scriptSigs and
        // scriptPubKeys of a transaction and does not take the actual executed
        // sig operations into account. spendingTx in itself does not contain a
        // signature operation.
        BOOST_CHECK_EQUAL(CountTxSigOps(spendTx, coins), 0);
        BOOST_CHECK_EQUAL(CountTxP2SHSigOps(spendTx, coins), 0);
        BOOST_CHECK_EQUAL(CountTxNonP2SHSigOps(spendTx), 0);
        // creationTx contains two signature operations in its scriptPubKey, but
        // legacy counting is not accurate.
        BOOST_CHECK_EQUAL(CountTxSigOps(createTx, coins),
                          MAX_PUBKEYS_PER_MULTISIG);
        BOOST_CHECK_EQUAL(CountTxP2SHSigOps(createTx, coins), 0);
        BOOST_CHECK_EQUAL(CountTxNonP2SHSigOps(createTx),
                          MAX_PUBKEYS_PER_MULTISIG);
    }

    // Multisig nested in P2SH
    {
        CCoinsView coinsDummy;
        CCoinsViewCache coins(&coinsDummy);

        CScript redeemScript = CScript() << 1 << dummy << dummy << 2
                                         << OP_CHECKMULTISIGVERIFY;
        CScript scriptPubKey =
            GetScriptForDestination(ScriptHash(redeemScript));
        CScript scriptSig = CScript()
                            << OP_0 << OP_0 << ToByteVector(redeemScript);

        BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig);
        CTransaction createTx(creationTx);
        CTransaction spendTx(spendingTx);

        BOOST_CHECK_EQUAL(CountTxSigOps(spendTx, coins), 2);
        BOOST_CHECK_EQUAL(CountTxP2SHSigOps(spendTx, coins), 2);
        BOOST_CHECK_EQUAL(CountTxNonP2SHSigOps(spendTx), 0);

        BOOST_CHECK_EQUAL(CountTxSigOps(createTx, coins), 0);
        BOOST_CHECK_EQUAL(CountTxP2SHSigOps(createTx, coins), 0);
        BOOST_CHECK_EQUAL(CountTxNonP2SHSigOps(createTx), 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
