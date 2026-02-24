// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { expect } from 'chai';
import { Script } from './script.js';
import { pushBytesOp } from './op.js';
import { OP_CHECKSIGVERIFY, OP_EQUAL, OP_EQUALVERIFY } from './opcode.js';
import { Address } from './address/address.js';
import { shaRmd160 } from './hash.js';
import { fromHex } from './io/hex.js';
import { strToBytes } from './io/str.js';
import { decodeInputData } from './inputData.js';

describe('inputData', () => {
    it('decodeInputData returns lokadId, data, and address for valid scriptSig', () => {
        const lokad = strToBytes('utf8');
        const data = strToBytes('thisisatest');
        const redeemScript = Script.fromOps([
            OP_CHECKSIGVERIFY,
            pushBytesOp(data),
            OP_EQUALVERIFY,
            pushBytesOp(lokad),
            OP_EQUAL,
        ]);

        const pk = fromHex(
            '02' +
                '0123456789012345678901234567890123456789012345678901234567890123',
        );
        const expectedAddr = Address.p2pkh(shaRmd160(pk)).toString();

        const sig = new Uint8Array(65);
        sig[64] = 0x41;

        const scriptSig = Script.fromOps([
            pushBytesOp(lokad),
            pushBytesOp(data),
            pushBytesOp(sig),
            pushBytesOp(pk),
            pushBytesOp(redeemScript.bytecode),
        ]);

        const decoded = decodeInputData(scriptSig.bytecode);
        expect(decoded).to.not.equal(undefined);
        expect(decoded!.lokadId).to.deep.equal(lokad);
        expect(decoded!.data).to.deep.equal(data);
        expect(decoded!.address).to.equal(expectedAddr);
    });

    it('decodeInputData returns undefined for scriptSig with too few pushes', () => {
        const scriptSig = Script.fromOps([
            pushBytesOp(new Uint8Array(65)),
        ]).bytecode;
        expect(decodeInputData(scriptSig)).to.equal(undefined);
    });

    it('decodeInputData returns undefined for invalid lokad length', () => {
        const lokad = new Uint8Array(3); // Wrong: must be 4 bytes
        const data = new Uint8Array([1, 2, 3]);
        const redeemScript = Script.fromOps([
            OP_CHECKSIGVERIFY,
            pushBytesOp(data),
            OP_EQUALVERIFY,
            pushBytesOp(lokad),
            OP_EQUAL,
        ]);
        const pk = fromHex(
            '02' +
                '0123456789012345678901234567890123456789012345678901234567890123',
        );
        const scriptSig = Script.fromOps([
            pushBytesOp(lokad),
            pushBytesOp(data),
            pushBytesOp(new Uint8Array(65)),
            pushBytesOp(pk),
            pushBytesOp(redeemScript.bytecode),
        ]).bytecode;
        expect(decodeInputData(scriptSig)).to.equal(undefined);
    });

    it('decodeInputData returns undefined for invalid pubkey length', () => {
        const lokad = new Uint8Array([1, 2, 3, 4]);
        const data = new Uint8Array([1, 2, 3]);
        const redeemScript = Script.fromOps([
            OP_CHECKSIGVERIFY,
            pushBytesOp(data),
            OP_EQUALVERIFY,
            pushBytesOp(lokad),
            OP_EQUAL,
        ]);
        const invalidPk = new Uint8Array(32);
        const scriptSig = Script.fromOps([
            pushBytesOp(lokad),
            pushBytesOp(data),
            pushBytesOp(new Uint8Array(65)),
            pushBytesOp(invalidPk),
            pushBytesOp(redeemScript.bytecode),
        ]).bytecode;
        expect(decodeInputData(scriptSig)).to.equal(undefined);
    });

    it('decodeInputData accepts custom prefix', () => {
        const lokad = new Uint8Array([1, 2, 3, 4]);
        const data = new Uint8Array([1, 2, 3]);
        const redeemScript = Script.fromOps([
            OP_CHECKSIGVERIFY,
            pushBytesOp(data),
            OP_EQUALVERIFY,
            pushBytesOp(lokad),
            OP_EQUAL,
        ]);
        const pk = fromHex(
            '02' +
                '0123456789012345678901234567890123456789012345678901234567890123',
        );
        const expectedAddr = Address.p2pkh(shaRmd160(pk), 'ectest').toString();

        const scriptSig = Script.fromOps([
            pushBytesOp(lokad),
            pushBytesOp(data),
            pushBytesOp(new Uint8Array(65)),
            pushBytesOp(pk),
            pushBytesOp(redeemScript.bytecode),
        ]).bytecode;

        const decoded = decodeInputData(scriptSig, 'ectest');
        expect(decoded).to.not.equal(undefined);
        expect(decoded!.lokadId).to.deep.equal(lokad);
        expect(decoded!.data).to.deep.equal(data);
        expect(decoded!.address).to.equal(expectedAddr);
    });
});
