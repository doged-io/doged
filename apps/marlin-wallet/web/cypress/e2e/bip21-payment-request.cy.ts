// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { visitWithWalletMnemonic, waitForMainLoaded } from '../fixture/common';
import { runWithChronik, stubCoingeckoXecFiatPrices } from '../fixture/stubs';

/** Matches `cypress/fixture/chronik/qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2.json`. */
const TEST_MNEMONIC =
    'load quality private purchase cream pony powder stairs edit fashion until earn';

const WALLET_ADDRESS = 'ecash:qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2';

const CHRONIK_STUB = 'qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2.json';

const UNSUPPORTED_TOKEN_ID =
    '1111111111111111111111111111111111111111111111111111111111111111';

/**
 * Simulates React Native WebView posting a PayButton / payment deep link
 * payload.
 * `index.ts` listens on `window` + `document` for `message` and parses JSON
 * `event.data`.
 */
function postPaymentRequest(bip21OrDeepLink: string): void {
    cy.window().then(win => {
        win.postMessage(
            JSON.stringify({
                type: 'PAYMENT_REQUEST',
                data: bip21OrDeepLink,
            }),
            '*',
        );
    });
}

describe('BIP21 PAYMENT_REQUEST (native bridge)', () => {
    beforeEach(() => {
        stubCoingeckoXecFiatPrices();
    });

    it('shows error modal for unsupported token_id', () => {
        const uri = `${WALLET_ADDRESS}?token_id=${UNSUPPORTED_TOKEN_ID}`;
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();

            postPaymentRequest(uri);

            cy.get('#error-modal-overlay').should('be.visible');
            cy.get('.error-modal-title').should(
                'contain',
                "Can't open this payment link",
            );
            cy.get('.error-modal-message').should(
                'contain',
                'The token is not supported',
            );
            cy.get('#error-modal-details-section').should('be.visible');
            cy.get('#error-modal-details-toggle').click();
            cy.get('#error-modal-details-text').should('contain', uri);
            cy.get('#error-modal-details-toggle').click();
            cy.get('#error-modal-details-quote').should('have.class', 'hidden');
            cy.get('#error-modal-details-toggle').should(
                'have.attr',
                'aria-expanded',
                'false',
            );
            cy.get('#error-modal-close').click();
            cy.get('#error-modal-overlay').should('not.be.visible');
        });
    });

    it('shows error modal for malformed ecash BIP21', () => {
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();

            postPaymentRequest('ecash:invalid-address');

            cy.get('#error-modal-overlay').should('be.visible');
            cy.get('.error-modal-message').should(
                'contain',
                'The link is malformed',
            );
            cy.get('#error-modal-details-toggle').click();
            cy.get('#error-modal-details-text').should(
                'contain',
                'ecash:invalid-address',
            );
            cy.get('#error-modal-close').click();
            cy.get('#error-modal-overlay').should('not.be.visible');
        });
    });

    it('shows error modal for PayButton deep link with unsupported token_id', () => {
        const deepLink =
            `https://paybutton.org/app?address=${encodeURIComponent(WALLET_ADDRESS)}` +
            `&token_id=${UNSUPPORTED_TOKEN_ID}`;
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();

            postPaymentRequest(deepLink);

            cy.get('#error-modal-overlay').should('be.visible');
            cy.get('.error-modal-title').should(
                'contain',
                "Can't open this payment link",
            );
            cy.get('.error-modal-message').should(
                'contain',
                'The token is not supported',
            );
            cy.get('#error-modal-details-toggle').click();
            cy.get('#error-modal-details-text').should('contain', deepLink);
            cy.get('#error-modal-close').click();
            cy.get('#error-modal-overlay').should('not.be.visible');
        });
    });

    it('shows error modal for PayButton deep link with malformed address', () => {
        const deepLink = `https://paybutton.org/app?address=${encodeURIComponent('ecash:invalid-address')}`;
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();

            postPaymentRequest(deepLink);

            cy.get('#error-modal-overlay').should('be.visible');
            cy.get('.error-modal-title').should(
                'contain',
                "Can't open this payment link",
            );
            cy.get('.error-modal-message').should(
                'contain',
                'The link is malformed',
            );
            cy.get('#error-modal-details-toggle').click();
            cy.get('#error-modal-details-text').should('contain', deepLink);
            cy.get('#error-modal-close').click();
            cy.get('#error-modal-overlay').should('not.be.visible');
        });
    });
});
