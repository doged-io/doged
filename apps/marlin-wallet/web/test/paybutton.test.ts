// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import * as chai from 'chai';
import {
    isPayButtonTransaction,
    paybuttonDeepLinkToBip21Uri,
} from '../src/deeplink';

const expect = chai.expect;

describe('paybutton.ts', function () {
    describe('isPayButtonTransaction', function () {
        it('Should identify PayButton transactions', function () {
            expect(isPayButtonTransaction('0450415900')).to.be.equal(true);
        });

        it('Should identify PayButton transactions with additional data', function () {
            expect(
                isPayButtonTransaction('04504159001234567890abcdef'),
            ).to.be.equal(true);
            expect(
                isPayButtonTransaction('04504159001234567890ABCDEF'),
            ).to.be.equal(true);
        });

        it('Should reject non-PayButton transactions', function () {
            expect(isPayButtonTransaction('0450415901')).to.be.equal(false); // Different last byte
        });

        it('Should reject empty OP_RETURN', function () {
            expect(isPayButtonTransaction('')).to.be.equal(false);
        });
    });

    describe('paybuttonDeepLinkToBip21Uri', function () {
        it('Should unwrap paybutton.org /app link into the address BIP21 URI', function () {
            const deepLink =
                'https://paybutton.org/app?address=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5&amount=1';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri:
                    'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5?amount=1',
                returnToBrowser: false,
            });
        });

        it('Should accept api.paybutton.org host', function () {
            const deepLink =
                'https://api.paybutton.org/app?address=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: 'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5',
                returnToBrowser: false,
            });
        });

        it('Should set returnToBrowser when b=1 and strip b from output', function () {
            const deepLink =
                'https://paybutton.org/app?address=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5&b=1';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: 'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5',
                returnToBrowser: true,
            });
        });

        it('Should not set returnToBrowser for b other than 1 but still strip b', function () {
            const deepLink =
                'https://paybutton.org/app?address=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5&b=0';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: 'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5',
                returnToBrowser: false,
            });
        });

        it('Should leave other params and strip only b when both present', function () {
            const deepLink =
                'https://paybutton.org/app?address=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5&amount=2&b=1';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri:
                    'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5?amount=2',
                returnToBrowser: true,
            });
        });

        it('Should return null when address param is missing', function () {
            const deepLink = 'https://paybutton.org/app?amount=1';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should return null for wrong host', function () {
            const deepLink =
                'https://example.com/app?address=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should return null when path is not /app', function () {
            const deepLink =
                'https://paybutton.org/other?address=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should require HTTPS', function () {
            const deepLink =
                'http://paybutton.org/app?address=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should return null for malformed URL string', function () {
            const deepLink = 'not-a-url';
            expect(paybuttonDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });
    });
});
