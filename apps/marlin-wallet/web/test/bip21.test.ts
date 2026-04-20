// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import * as chai from 'chai';
import {
    createBip21Uri,
    parseAmountAsAtoms,
    parseBip21Uri,
    parseUint256Hex,
} from '../src/bip21';
import { FIRMA_TOKEN, XEC_ASSET } from '../src/supported-assets';

const expect = chai.expect;

describe('bip21.ts', function () {
    describe('parseUint256Hex', function () {
        const validLower = FIRMA_TOKEN.tokenId!;

        it('Should accept 64 hex digits and return lowercase', function () {
            expect(parseUint256Hex(validLower)).to.equal(validLower);
        });

        it('Should normalize uppercase hex to lowercase', function () {
            const upper = validLower.toUpperCase();
            expect(parseUint256Hex(upper)).to.equal(validLower);
        });

        it('Should trim surrounding whitespace', function () {
            expect(parseUint256Hex(`  ${validLower}  `)).to.equal(validLower);
        });

        it('Should reject wrong length', function () {
            expect(parseUint256Hex(validLower.slice(0, 63))).to.be.equal(null);
            expect(parseUint256Hex(`${validLower}0`)).to.be.equal(null);
        });

        it('Should reject non-hex characters', function () {
            expect(parseUint256Hex(`${validLower.slice(0, -1)}g`)).to.be.equal(
                null,
            );
            expect(parseUint256Hex(`${validLower.slice(0, -1)}z`)).to.be.equal(
                null,
            );
        });

        it('Should reject empty and whitespace-only input', function () {
            expect(parseUint256Hex('')).to.be.equal(null);
            expect(parseUint256Hex('   ')).to.be.equal(null);
        });
    });

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
        const sampleAddress =
            'ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07';

        it('Should parse BIP21 URI with address only', function () {
            const uri = sampleAddress;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: XEC_ASSET.key,
            });
        });

        it('Should parse BIP21 URI with amount', function () {
            const uri = `${sampleAddress}?amount=100.42`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                atoms: 10042,
                tokenAssetKey: XEC_ASSET.key,
            });
        });

        it('Should parse BIP21 URI with OP_RETURN data', function () {
            const uri = `${sampleAddress}?op_return_raw=0450415900`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: XEC_ASSET.key,
                opReturnRaw: '0450415900',
            });
        });

        it('Should parse BIP21 URI with both amount and OP_RETURN', function () {
            const uri = `${sampleAddress}?amount=50.00&op_return_raw=045041590A`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                atoms: 5000,
                tokenAssetKey: XEC_ASSET.key,
                opReturnRaw: '045041590A',
            });
        });

        it('Should reject invalid protocol', function () {
            const uri = 'bitcoin:1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa';
            expect(parseBip21Uri(uri)).to.equal(null);
        });

        it('Should reject invalid address format', function () {
            const uri = 'ecash:invalid-address';
            expect(parseBip21Uri(uri)).to.equal(null);
        });

        it('Should reject invalid amount', function () {
            const uri = `${sampleAddress}?amount=invalid`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: XEC_ASSET.key,
            });
        });

        it('Should reject negative amount', function () {
            const uri = `${sampleAddress}?amount=-10.00`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: XEC_ASSET.key,
            });
        });

        it('Should reject invalid OP_RETURN hex', function () {
            const uri = `${sampleAddress}?op_return_raw=invalid`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: XEC_ASSET.key,
            });
        });

        it('Should reject OP_RETURN with odd number of hex characters', function () {
            const uri = `${sampleAddress}?op_return_raw=045041590`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: XEC_ASSET.key,
            });
        });

        it('Should handle lowercase hex in OP_RETURN', function () {
            const uri = `${sampleAddress}?op_return_raw=045041590a`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: XEC_ASSET.key,
                opReturnRaw: '045041590A',
            });
        });

        it('Should ignore unknown query parameters', function () {
            const uri = `${sampleAddress}?amount=100.00&unknown=param`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                atoms: 10000,
                tokenAssetKey: XEC_ASSET.key,
            });
        });

        it('Should handle malformed URI gracefully', function () {
            const uri = 'not-a-valid-uri';
            expect(parseBip21Uri(uri)).to.equal(null);
        });

        it('Should parse single-recipient token BIP21 for a supported token', function () {
            const uri =
                `${sampleAddress}?token_id=${FIRMA_TOKEN.tokenId}` +
                '&token_decimalized_qty=100.12';
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: FIRMA_TOKEN.key,
                atoms: 1001200,
            });
        });

        it('Should accept uppercase token_id hex for a supported token', function () {
            const upperId = FIRMA_TOKEN.tokenId!.toUpperCase();
            const uri = `${sampleAddress}?token_id=${upperId}&token_decimalized_qty=1`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: FIRMA_TOKEN.key,
                atoms: 10000,
            });
        });

        it('Should parse supported token BIP21 without token_decimalized_qty', function () {
            const uri = `${sampleAddress}?token_id=${FIRMA_TOKEN.tokenId}`;
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: FIRMA_TOKEN.key,
            });
        });

        it('Should ignore token params when XEC amount is present', function () {
            const uri =
                `${sampleAddress}?token_id=${FIRMA_TOKEN.tokenId}` +
                '&token_decimalized_qty=2&amount=50';
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                atoms: 5000,
                tokenAssetKey: XEC_ASSET.key,
            });
        });

        it('Should reject token_id that does not match any supported asset', function () {
            const uri =
                `${sampleAddress}?token_id=` +
                '1111111111111111111111111111111111111111111111111111111111111111' +
                '&token_decimalized_qty=1';
            expect(parseBip21Uri(uri)).to.equal(null);
        });

        it('Should reject token_decimalized_qty with too many decimals', function () {
            const uri =
                `${sampleAddress}?token_id=${FIRMA_TOKEN.tokenId}` +
                '&token_decimalized_qty=1.23456';
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: FIRMA_TOKEN.key,
            });
        });

        it('Should reject empty token_id', function () {
            const uri = `${sampleAddress}?token_id=&token_decimalized_qty=1`;
            expect(parseBip21Uri(uri)).to.equal(null);
        });

        it('Should reject non-positive token_decimalized_qty when present', function () {
            const uri =
                `${sampleAddress}?token_id=${FIRMA_TOKEN.tokenId}` +
                '&token_decimalized_qty=-1';
            expect(parseBip21Uri(uri)).to.deep.equal({
                address: sampleAddress,
                tokenAssetKey: FIRMA_TOKEN.key,
            });
        });
    });
});
