// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

class CScript;

enum class SigOpCountMode {
    ACCURATE,
    ESTIMATED,
};

uint32_t CountScriptSigOps(const CScript &script, SigOpCountMode mode);

uint32_t CountScriptSigOpsP2SH(const CScript &scriptSig);
