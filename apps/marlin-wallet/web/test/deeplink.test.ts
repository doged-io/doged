// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import * as chai from 'chai';
import { payecashDeepLinkToBip21Uri } from '../src/deeplink';

const expect = chai.expect;

describe('deeplink.ts', function () {
    describe('payecashDeepLinkToBip21Uri', function () {
        it('Should unwrap a pay.e.cash HTTPS link into the inner BIP21 URI', function () {
            const deepLink =
                'https://pay.e.cash?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5?amount=1.5';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri:
                    'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5?amount=1.5',
                returnToBrowser: false,
            });
        });

        it('Should parse when the BIP21 URI has no query parameters', function () {
            const deepLink =
                'https://pay.e.cash?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: 'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5',
                returnToBrowser: false,
            });
        });

        it('Should accept pay.e.cash with a trailing slash before query', function () {
            const deepLink =
                'https://pay.e.cash/?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: 'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5',
                returnToBrowser: false,
            });
        });

        it('Should return null when pathname is not empty', function () {
            const deepLink =
                'https://pay.e.cash/pay?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should set returnToBrowser when inner URI has b=1 and strip b', function () {
            const deepLink =
                'https://pay.e.cash/?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5?b=1';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: 'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5',
                returnToBrowser: true,
            });
        });

        it('Should not set returnToBrowser for b other than 1 but still strip b', function () {
            const deepLink =
                'https://pay.e.cash/?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5?b=0';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: 'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5',
                returnToBrowser: false,
            });
        });

        it('Should leave amount and strip only b when both present', function () {
            const deepLink =
                'https://pay.e.cash/?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5?amount=2&b=1';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri:
                    'ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5?amount=2',
                returnToBrowser: true,
            });
        });

        it('Should not parse wrong host', function () {
            const deepLink =
                'https://example.com/?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should require HTTPS', function () {
            const deepLink =
                'http://pay.e.cash/?bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should require search to start with ?bip21=', function () {
            const deepLink =
                'https://pay.e.cash/?other=1&bip21=ecash:qp7g5uyxvun4r5afffs6pfy27eyhcqtj9cev06d8s5';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should return null when inner BIP21 is not a URI', function () {
            const deepLink = 'https://pay.e.cash/?bip21=%';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });

        it('Should return null for malformed deep link string', function () {
            const deepLink = 'not-a-url';
            expect(payecashDeepLinkToBip21Uri(deepLink)).to.deep.equal({
                bip21Uri: null,
                returnToBrowser: false,
            });
        });
    });
});
