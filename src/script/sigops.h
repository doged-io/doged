// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

class CScript;
class CTransaction;
class CCoinsViewCache;

enum class SigOpCountMode {
    ACCURATE,
    ESTIMATED,
};

uint32_t CountScriptSigOps(const CScript &script, SigOpCountMode mode);

uint32_t CountScriptSigOpsP2SH(const CScript &scriptSig);

uint64_t CountTxNonP2SHSigOps(const CTransaction &tx);

uint64_t CountTxP2SHSigOps(const CTransaction &tx, const CCoinsViewCache &view);

uint64_t CountTxSigOps(const CTransaction &tx, const CCoinsViewCache &view);
