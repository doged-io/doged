// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import * as chai from 'chai';
import {
    createBip21Uri,
    parseAmountAsAtoms,
    parseBip21Uri,
} from '../src/bip21';

const expect = chai.expect;

describe('bip21.ts', function () {
    describe('parseAmountAsAtoms', function () {
        const xecDecimals = 2;

        it('Should parse integer base units to atoms (XEC-style decimals)', function () {
            expect(parseAmountAsAtoms('100', xecDecimals)).to.equal(10000);
            expect(parseAmountAsAtoms('0', xecDecimals)).to.equal(0);
            expect(parseAmountAsAtoms('1', xecDecimals)).to.equal(100);
            expect(parseAmountAsAtoms('0100', xecDecimals)).to.equal(10000);
            expect(parseAmountAsAtoms('000', xecDecimals)).to.equal(0);
            expect(parseAmountAsAtoms('00001', xecDecimals)).to.equal(100);
        });

        it('Should parse fractional amounts with exact decimal places', function () {
            expect(parseAmountAsAtoms('100.42', xecDecimals)).to.equal(10042);
            expect(parseAmountAsAtoms('0.01', xecDecimals)).to.equal(1);
            expect(parseAmountAsAtoms('.5', xecDecimals)).to.equal(50);
            expect(parseAmountAsAtoms('5.', xecDecimals)).to.equal(500);
            expect(parseAmountAsAtoms('0100.42', xecDecimals)).to.equal(10042);
            expect(parseAmountAsAtoms('00.01', xecDecimals)).to.equal(1);
            expect(parseAmountAsAtoms('000.5', xecDecimals)).to.equal(50);
            expect(parseAmountAsAtoms('00005.', xecDecimals)).to.equal(500);
            expect(parseAmountAsAtoms('5.0', xecDecimals)).to.equal(500);
            expect(parseAmountAsAtoms('5.00', xecDecimals)).to.equal(500);
            expect(parseAmountAsAtoms('05.0', xecDecimals)).to.equal(500);
            // Weird but allowed by the spec, equivalent to 0.0
            expect(parseAmountAsAtoms('.', xecDecimals)).to.equal(0);
        });

        it('Should respect maxDecimals for token-style amounts', function () {
            expect(parseAmountAsAtoms('1.2345', 4)).to.equal(12345);
            expect(parseAmountAsAtoms('100.1200', 4)).to.equal(1001200);
            expect(parseAmountAsAtoms('123.456', 4)).to.equal(1234560);
        });

        it('Should reject more fractional digits than maxDecimals', function () {
            expect(parseAmountAsAtoms('100.0', 0)).to.be.equal(null);
            expect(parseAmountAsAtoms('100.', 0)).to.be.equal(100);
            expect(parseAmountAsAtoms('1.000', 2)).to.be.equal(null);

            expect(parseAmountAsAtoms('100.421', xecDecimals)).to.be.equal(
                null,
            );
            expect(parseAmountAsAtoms('1.23456', 4)).to.be.equal(null);
        });

        it('Should reject strings with invalid characters', function () {
            expect(parseAmountAsAtoms('0x00', xecDecimals)).to.be.equal(null);
            expect(parseAmountAsAtoms('10ab', xecDecimals)).to.be.equal(null);
            expect(parseAmountAsAtoms('1e2', xecDecimals)).to.be.equal(null);
            expect(parseAmountAsAtoms('-5', xecDecimals)).to.be.equal(null);
            expect(parseAmountAsAtoms('+5', xecDecimals)).to.be.equal(null);
            expect(parseAmountAsAtoms(' 5', xecDecimals)).to.be.equal(null);
            expect(parseAmountAsAtoms('5 ', xecDecimals)).to.be.equal(null);
        });

        it('Should reject ambiguous or non-normalized decimal forms', function () {
            expect(parseAmountAsAtoms('1.2.3', xecDecimals)).to.be.equal(null);
            expect(parseAmountAsAtoms('..', xecDecimals)).to.be.equal(null);
            expect(parseAmountAsAtoms('', xecDecimals)).to.be.equal(null);
        });

        it('Should return null when atoms overflow safe integer', function () {
            expect(parseAmountAsAtoms('9007199254740991', 0)).to.be.equal(
                9007199254740991,
            );
            expect(parseAmountAsAtoms('90071992547409.91', 2)).to.be.equal(
                9007199254740991,
            );
            expect(parseAmountAsAtoms('900719925474.0991', 4)).to.be.equal(
                9007199254740991,
            );
            expect(parseAmountAsAtoms('9007199.254740991', 9)).to.be.equal(
                9007199254740991,
            );
            expect(parseAmountAsAtoms('9007199254740992', 0)).to.be.equal(null);
            expect(parseAmountAsAtoms('90071992547409.92', 2)).to.be.equal(
                null,
            );
            expect(parseAmountAsAtoms('900719925474.0992', 4)).to.be.equal(
                null,
            );
            expect(parseAmountAsAtoms('9007199.254740992', 9)).to.be.equal(
                null,
            );
        });
    });

    describe('createBip21Uri', function () {
        it('Should create BIP21 URI with address only', function () {
            const address = 'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07';
            const uri = createBip21Uri(address);
            expect(uri).to.equal(
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07',
            );
        });

        it('Should create BIP21 URI with address and amount', function () {
            const address = 'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07';
            const amountSats = 12345; // 100 XEC
            const uri = createBip21Uri(address, amountSats);
            expect(uri).to.equal(
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?amount=123.45',
            );
        });

        it('Should handle address without prefix', function () {
            const address = 'prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07';
            const uri = createBip21Uri(address);
            expect(uri).to.equal(
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07',
            );
        });

        it('Should not include amount if zero or negative', function () {
            const address = 'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07';
            expect(createBip21Uri(address, 0)).to.not.include('amount');
            expect(createBip21Uri(address, -100)).to.not.include('amount');
        });
    });

    describe('parseBip21Uri', function () {
        it('Should parse BIP21 URI with address only', function () {
            const uri = 'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07';
            const result = parseBip21Uri(uri);

            expect(result).to.not.be.equal(null);
            expect(result!.address).to.equal(
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07',
            );
            expect(result!.sats).to.be.equal(undefined);
        });

        it('Should parse BIP21 URI with amount', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?amount=100.42';
            const result = parseBip21Uri(uri);

            expect(result).to.not.be.equal(null);
            expect(result!.address).to.equal(
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07',
            );
            expect(result!.sats).to.equal(10042); // 100.42 XEC = 10042 sats
        });

        it('Should parse BIP21 URI with OP_RETURN data', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?op_return_raw=0450415900';
            const result = parseBip21Uri(uri);

            expect(result).to.not.be.equal(null);
            expect(result!.address).to.equal(
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07',
            );
            expect(result!.opReturnRaw).to.equal('0450415900');
        });

        it('Should parse BIP21 URI with both amount and OP_RETURN', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?amount=50.00&op_return_raw=045041590A';
            const result = parseBip21Uri(uri);

            expect(result).to.not.be.equal(null);
            expect(result!.address).to.equal(
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07',
            );
            expect(result!.sats).to.equal(5000);
            expect(result!.opReturnRaw).to.equal('045041590A');
        });

        it('Should reject invalid protocol', function () {
            const uri = 'bitcoin:1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa';
            const result = parseBip21Uri(uri);
            expect(result).to.be.equal(null);
        });

        it('Should reject invalid address format', function () {
            const uri = 'ecash:invalid-address';
            const result = parseBip21Uri(uri);
            expect(result).to.be.equal(null);
        });

        it('Should reject invalid amount', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?amount=invalid';
            const result = parseBip21Uri(uri);
            expect(result).to.not.be.equal(null);
            expect(result!.sats).to.be.equal(undefined); // Invalid amount should be ignored
        });

        it('Should reject negative amount', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?amount=-10.00';
            const result = parseBip21Uri(uri);
            expect(result).to.not.be.equal(null);
            expect(result!.sats).to.be.equal(undefined); // Negative amount should be ignored
        });

        it('Should reject invalid OP_RETURN hex', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?op_return_raw=invalid';
            const result = parseBip21Uri(uri);
            expect(result).to.not.be.equal(null);
            expect(result!.opReturnRaw).to.be.equal(undefined); // Invalid hex should be ignored
        });

        it('Should reject OP_RETURN with odd number of hex characters', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?op_return_raw=045041590';
            const result = parseBip21Uri(uri);
            expect(result).to.not.be.equal(null);
            expect(result!.opReturnRaw).to.be.equal(undefined); // Odd length should be ignored
        });

        it('Should handle lowercase hex in OP_RETURN', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?op_return_raw=045041590a';
            const result = parseBip21Uri(uri);
            expect(result).to.not.be.equal(null);
            expect(result!.opReturnRaw).to.equal('045041590A');
        });

        it('Should ignore unknown query parameters', function () {
            const uri =
                'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?amount=100.00&unknown=param';
            const result = parseBip21Uri(uri);
            expect(result).to.not.be.equal(null);
            expect(result!.sats).to.equal(10000);
        });

        it('Should handle malformed URI gracefully', function () {
            const uri = 'not-a-valid-uri';
            const result = parseBip21Uri(uri);
            expect(result).to.be.equal(null);
        });
    });
});
