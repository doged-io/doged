// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { Bytes } from './io/bytes.js';
import { readOp, isPushOp } from './op.js';
import { shaRmd160 } from './hash.js';
import { Address } from './address/address.js';
import { OP_CHECKSIGVERIFY, OP_EQUAL, OP_EQUALVERIFY } from './opcode.js';

/**
 * Result of decoding a P2SH input that uses the Agora-style input-data format.
 *
 * Format follows ecash-agora ad script (ad.ts parseAdScriptSig, partial.ts adScript):
 * - scriptSig = <lokad> <data> <sig> <pubkey> <redeemScript>
 * - lokad first for chronik lokadId indexing
 * - Redeem script: OP_CHECKSIGVERIFY pushBytes(data) OP_EQUALVERIFY pushBytes(lokad) OP_EQUAL
 */
export interface DecodedInputData {
    /** 4-byte lokad ID (for chronik indexing) */
    lokadId: Uint8Array;
    /** Arbitrary data bytes */
    data: Uint8Array;
    /** Sender's P2PKH address (from pubkey in scriptSig) */
    address: string;
}

/**
 * Decode scriptSig from a P2SH input that uses the Agora-style input-data format.
 *
 * Built: scriptSig = [lokad, data, sig, pubkey, redeemScript]
 * Redeem script: OP_CHECKSIG pushBytes(data) OP_EQUALVERIFY pushBytes(lokad) OP_EQUAL
 *
 * @param scriptSig - The scriptSig bytes from the spending input
 * @param prefix - Address prefix (default 'ecash')
 * @returns Decoded lokadId, data, and address, or undefined if the format doesn't match
 */
export function decodeInputData(
    scriptSig: Uint8Array,
    prefix = 'ecash',
): DecodedInputData | undefined {
    const pushes: Uint8Array[] = [];
    const bytes = new Bytes(scriptSig);

    while (bytes.data.length - bytes.idx > 0) {
        try {
            const op = readOp(bytes);
            if (isPushOp(op)) {
                pushes.push(op.data);
            }
        } catch {
            return undefined;
        }
    }

    // scriptSig = <lokad> <data> <sig> <pubkey> <redeemScript>
    if (pushes.length < 5) {
        return undefined;
    }
    const lokadId = pushes[0];
    if (lokadId.length !== 4) {
        return undefined;
    }
    const data = pushes[1];
    const pubkey = pushes[pushes.length - 2];
    const redeemScript = pushes[pushes.length - 1];

    if (pubkey.length !== 33 && pubkey.length !== 65) {
        return undefined;
    }

    // Redeem script: OP_CHECKSIGVERIFY pushBytes(data) OP_EQUALVERIFY pushBytes(lokad) OP_EQUAL
    const redeemBytes = new Bytes(redeemScript);
    try {
        if (readOp(redeemBytes) !== OP_CHECKSIGVERIFY) {
            return undefined;
        }
        const dataOp = readOp(redeemBytes);
        if (!isPushOp(dataOp)) {
            return undefined;
        }
        if (
            dataOp.data.length !== data.length ||
            dataOp.data.some((b, i) => b !== data[i])
        ) {
            return undefined;
        }
        if (readOp(redeemBytes) !== OP_EQUALVERIFY) {
            return undefined;
        }
        const lokadOp = readOp(redeemBytes);
        if (!isPushOp(lokadOp) || lokadOp.data.length !== 4) {
            return undefined;
        }
        if (lokadOp.data.some((b, i) => b !== lokadId[i])) {
            return undefined;
        }
        if (readOp(redeemBytes) !== OP_EQUAL) {
            return undefined;
        }
    } catch {
        return undefined;
    }

    const pkh = shaRmd160(pubkey);
    const address = Address.p2pkh(pkh, prefix).toString();

    return { lokadId, data, address };
}
