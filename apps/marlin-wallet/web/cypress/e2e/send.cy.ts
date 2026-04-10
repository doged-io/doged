// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import {
    normalizeBalanceText,
    visitWithWalletMnemonic,
    waitForMainLoaded,
} from '../fixture/common';
import {
    runWithChronik,
    stubChronikBroadcastsSuccess,
    stubCoingeckoXecFiatPrices,
} from '../fixture/stubs';

/** Matches `cypress/fixture/chronik/qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2.json`. */
const TEST_MNEMONIC =
    'load quality private purchase cream pony powder stairs edit fashion until earn';

const WALLET_ADDRESS = 'ecash:qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2';

const CHRONIK_STUB = 'qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2.json';

const STUB_EXPECTED_TOTAL_XEC = 100_000.0;

/**
 * Parse the leading number from a primary `formatPrice(..., XEC)` cell (e.g. `1,234.56 XEC`).
 */
function parsePrimaryXecDisplay(text: string): number {
    const normalized = normalizeBalanceText(text).replace(/,/g, '');
    const match = normalized.match(/^(-?[\d.]+)/);
    if (match === null) {
        throw new Error(
            `expected numeric prefix in fee display: ${JSON.stringify(text)}`,
        );
    }
    return parseFloat(match[1]);
}

describe('Send', () => {
    beforeEach(() => {
        stubCoingeckoXecFiatPrices();
    });

    it('sends max to self', () => {
        runWithChronik(CHRONIK_STUB, () => {
            stubChronikBroadcastsSuccess();
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();

            cy.get('#scan-btn').click();
            cy.get('#manual-entry-btn').click();

            cy.get('#send-screen').should('not.have.class', 'hidden');
            cy.get('#recipient-address').clear().type(WALLET_ADDRESS).blur();

            cy.get('#amount-slider').then($slider => {
                const max = $slider.attr('max');
                expect(max, 'slider max').to.be.a('string').and.not.eq('');
                cy.wrap($slider)
                    .invoke('val', max)
                    .trigger('input', { force: true });
            });

            cy.get('#fee-display')
                .should('be.visible')
                .and('not.have.class', 'error');
            cy.get('#fee-display .fee-item.title').should(
                'contain',
                'Transaction Details',
            );
            cy.get('#fee-display .fee-item.title').should(
                'not.have.class',
                'error',
            );
            cy.get('#fee-display').within(() => {
                cy.contains('.fee-label', 'Amount:');
                cy.contains('.fee-label', 'Network Fee:');
                cy.contains('.fee-label', 'Total:');
            });
            cy.get('#fee-display .fee-value-primary')
                .should('have.length', 3)
                .then($cells => {
                    const primaryValues = Array.from($cells).map(cell =>
                        parsePrimaryXecDisplay(cell.textContent ?? ''),
                    );
                    const [sendAmount, networkFee, totalOut] = primaryValues;
                    expect(
                        sendAmount,
                        'transaction details amount',
                    ).to.be.greaterThan(0);
                    expect(
                        networkFee,
                        'transaction details fee',
                    ).to.be.at.least(0);
                    expect(
                        totalOut,
                        'transaction details total',
                    ).to.be.greaterThan(0);
                    expect(
                        sendAmount + networkFee,
                        'amount + fee must equal total',
                    ).to.equal(totalOut);
                    expect(
                        totalOut,
                        'total XEC must match stub spendable (amount+fee sweeps 100k XEC)',
                    ).to.equal(STUB_EXPECTED_TOTAL_XEC);
                });

            cy.get('#confirm-send').should('not.be.disabled').click();

            cy.wait('@chronikBroadcastsSuccess').then(interception => {
                expect(interception.request.method).to.eq('POST');
                expect(interception.request.url).to.match(
                    /\/broadcast-txs(\?|$)/,
                );
            });

            // After sending, the app jumps to the main screen.
            cy.get('#main-screen').should('be.visible');
        });
    });
});
