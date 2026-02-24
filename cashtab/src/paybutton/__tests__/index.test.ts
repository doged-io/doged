// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { paybuttonDeepLinkToBip21Uri } from 'paybutton';

describe('paybuttonDeepLinkToBip21Uri', () => {
    describe('valid PayButton URLs - paybutton.org', () => {
        it('converts address-only URL to BIP21 URI', () => {
            const result = paybuttonDeepLinkToBip21Uri(
                'https://paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0',
            );
            expect(result).toEqual({
                bip21Uri: 'ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0',
                returnToBrowser: false,
            });
        });

        it('converts URL with amount to BIP21 URI', () => {
            const result = paybuttonDeepLinkToBip21Uri(
                'https://paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0&amount=0.001',
            );
            expect(result).toEqual({
                bip21Uri:
                    'ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0?amount=0.001',
                returnToBrowser: false,
            });
        });

        it('sets returnToBrowser true when b=1', () => {
            const result = paybuttonDeepLinkToBip21Uri(
                'https://paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0&amount=0.001&b=1',
            );
            expect(result).toEqual({
                bip21Uri:
                    'ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0?amount=0.001',
                returnToBrowser: true,
            });
        });

        it('sets returnToBrowser false when b=0', () => {
            const result = paybuttonDeepLinkToBip21Uri(
                'https://paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0&b=0',
            );
            expect(result).toEqual({
                bip21Uri: 'ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0',
                returnToBrowser: false,
            });
        });

        it('handles multiple BIP21 params (amount, op_return_raw)', () => {
            const result = paybuttonDeepLinkToBip21Uri(
                'https://paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0&amount=1&op_return_raw=0400746162',
            );
            expect(result).toEqual({
                bip21Uri:
                    'ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0?amount=1&op_return_raw=0400746162',
                returnToBrowser: false,
            });
        });

        it('handles b param with non-1 value as returnToBrowser false', () => {
            const result = paybuttonDeepLinkToBip21Uri(
                'https://paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0&b=2',
            );
            expect(result.returnToBrowser).toBe(false);
        });
    });

    describe('valid PayButton URLs - api.paybutton.org', () => {
        it('converts api.paybutton.org URL to BIP21 URI', () => {
            const result = paybuttonDeepLinkToBip21Uri(
                'https://api.paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0&amount=0.5&b=1',
            );
            expect(result).toEqual({
                bip21Uri:
                    'ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0?amount=0.5',
                returnToBrowser: true,
            });
        });
    });

    describe('pass-through for non-PayButton URLs', () => {
        it('passes through ecash: BIP21 URI unchanged', () => {
            const input =
                'ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0?amount=0.001';
            const result = paybuttonDeepLinkToBip21Uri(input);
            expect(result).toEqual({
                bip21Uri: input,
                returnToBrowser: false,
            });
        });

        it('passes through for wrong protocol (http)', () => {
            const input =
                'http://paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0';
            const result = paybuttonDeepLinkToBip21Uri(input);
            expect(result).toEqual({ bip21Uri: input, returnToBrowser: false });
        });

        it('passes through for wrong hostname', () => {
            const input =
                'https://other-domain.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0';
            const result = paybuttonDeepLinkToBip21Uri(input);
            expect(result).toEqual({ bip21Uri: input, returnToBrowser: false });
        });

        it('passes through for wrong pathname', () => {
            const input =
                'https://paybutton.org/other?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0';
            const result = paybuttonDeepLinkToBip21Uri(input);
            expect(result).toEqual({ bip21Uri: input, returnToBrowser: false });
        });

        it('passes through when address param is missing', () => {
            const input = 'https://paybutton.org/app?amount=0.001';
            const result = paybuttonDeepLinkToBip21Uri(input);
            expect(result).toEqual({ bip21Uri: input, returnToBrowser: false });
        });

        it('passes through when address param is empty', () => {
            const input = 'https://paybutton.org/app?address=';
            const result = paybuttonDeepLinkToBip21Uri(input);
            expect(result).toEqual({ bip21Uri: input, returnToBrowser: false });
        });

        it('passes through invalid URL (throws)', () => {
            const input = 'not-a-valid-url';
            const result = paybuttonDeepLinkToBip21Uri(input);
            expect(result).toEqual({ bip21Uri: input, returnToBrowser: false });
        });
    });

    describe('edge cases', () => {
        it('passes through when path has trailing slash (/app/)', () => {
            const input =
                'https://paybutton.org/app/?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0';
            const result = paybuttonDeepLinkToBip21Uri(input);
            expect(result).toEqual({ bip21Uri: input, returnToBrowser: false });
        });

        it('preserves additional BIP21 params in query string', () => {
            const result = paybuttonDeepLinkToBip21Uri(
                'https://paybutton.org/app?address=ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0&label=Hello%20World',
            );
            expect(result).toEqual({
                bip21Uri:
                    'ecash:qp3wj05au4l7q2m5ng4qg0vpeejl42lvl0nqj8q0q0?label=Hello+World',
                returnToBrowser: false,
            });
        });
    });
});
