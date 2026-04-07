// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import {
    normalizeBalanceText,
    selectFirmaAsset,
    visitWithWalletMnemonic,
    waitForMainLoaded,
} from '../fixture/common';
import { DEFAULT_DUST_SATS } from 'ecash-lib';
import { atomsToUnit } from '../../src/amount';
import { FIRMA_TOKEN, XEC_ASSET } from '../../src/supported-assets';
import { createBip21Uri } from '../../src/bip21';
import {
    runWithChronik,
    stubChronikBroadcastsSuccess,
    stubCoingeckoXecFiatPrices,
} from '../fixture/stubs';

/** Matches `cypress/fixture/chronik/qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2.json`. */
const TEST_MNEMONIC =
    'load quality private purchase cream pony powder stairs edit fashion until earn';

const WALLET_ADDRESS = 'ecash:qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2';

/** Valid payout address, not the Firma stub wallet (`WALLET_ADDRESS`). */
const OTHER_RECIPIENT_ADDRESS =
    'ecash:qqa9lv3kjd8vq7952p7rq0f6lkpqvlu0cydvxtd70g';

const CHRONIK_STUB = 'qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2.json';

/**
 * Same wallet as `CHRONIK_STUB`; history is only the Firma receive tx; `scriptUtxos`
 * has no native XEC UTXO.
 */
const CHRONIK_STUB_NO_XEC =
    'qzrfaekxtl75jsgf30evmnzh9esjmx5wzu0vnzdxy2-no-xec.json';

const STUB_EXPECTED_TOTAL_XEC = 100_000.0;

const DUST_AMOUNT_XEC = atomsToUnit(
    Number(DEFAULT_DUST_SATS),
    XEC_ASSET.decimals,
);

/** Smallest Firma primary step on the send slider (`1 / 10^decimals`). */
const MIN_FIRMA_SLIDER_PRIMARY = 1 / Math.pow(10, FIRMA_TOKEN.decimals);

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

function primaryForSliderFraction(
    min: number,
    max: number,
    frac: number,
    decimals: number = 2,
): string {
    if (frac <= 0) {
        return min.toFixed(decimals);
    }
    if (frac >= 1) {
        return max.toFixed(decimals);
    }
    return (min + (max - min) * frac).toFixed(decimals);
}

describe('Send', () => {
    beforeEach(() => {
        stubCoingeckoXecFiatPrices();
    });

    /**
     * Open send form via QR flow → Manual Entry.
     * We can't test the camera flow in Cypress, so we just click the buttons.
     */
    function openManualSendScreen(): void {
        cy.get('#scan-btn').click();
        cy.get('#manual-entry-btn').click();
        cy.get('#send-screen').should('not.have.class', 'hidden');
    }

    function assertReturnedToMain(): void {
        cy.get('#main-screen').should('be.visible');
        cy.get('#send-screen').should('have.class', 'hidden');
    }

    function fillRecipientAndMaxSlider(): void {
        cy.get('#recipient-address').clear().type(WALLET_ADDRESS).blur();
        cy.get('#amount-slider').then($slider => {
            const max = $slider.attr('max');
            expect(max, 'slider max').to.be.a('string').and.not.eq('');
            cy.wrap($slider)
                .invoke('val', max)
                .trigger('input', { force: true });
        });
    }

    function pasteBip21UriIntoRecipient(uri: string): void {
        cy.get('#recipient-address')
            .invoke('val', uri)
            .trigger('input', { force: true });
    }

    describe('BIP21 URI pasted into recipient field', () => {
        it('fills address and amount from URI', () => {
            const uri = createBip21Uri(WALLET_ADDRESS, 10_000);
            runWithChronik(CHRONIK_STUB, () => {
                visitWithWalletMnemonic(TEST_MNEMONIC, {
                    requireHoldToSend: false,
                });
                waitForMainLoaded();
                openManualSendScreen();
                pasteBip21UriIntoRecipient(uri);
                cy.get('#recipient-address')
                    .should('have.value', WALLET_ADDRESS)
                    .and('have.attr', 'readonly');
                cy.get('#recipient-address').should('have.class', 'valid');
                cy.get('#send-amount')
                    .should('have.value', '100.00')
                    .and('have.attr', 'readonly');
                cy.get('#amount-slider').should('be.disabled');
                cy.get('#paybutton-logo-container').should(
                    'have.css',
                    'display',
                    'none',
                );
                cy.get('#fee-display').should('be.visible');
            });
        });

        it('fills fields, shows PayButton logo for PayButton op_return_raw', () => {
            const uri = `${createBip21Uri(WALLET_ADDRESS, 2500)}&op_return_raw=0450415900`;
            runWithChronik(CHRONIK_STUB, () => {
                visitWithWalletMnemonic(TEST_MNEMONIC, {
                    requireHoldToSend: false,
                });
                waitForMainLoaded();
                openManualSendScreen();
                pasteBip21UriIntoRecipient(uri);
                cy.get('#recipient-address')
                    .should('have.value', WALLET_ADDRESS)
                    .and('have.attr', 'readonly');
                cy.get('#send-amount')
                    .should('have.value', '25.00')
                    .and('have.attr', 'readonly');
                cy.get('#amount-slider').should('be.disabled');
                cy.get('#paybutton-logo-container').should(
                    'have.css',
                    'display',
                    'flex',
                );
                cy.get('#fee-display').should('be.visible');
            });
        });

        it('fills amount but hides PayButton logo for non-PayButton op_return_raw', () => {
            const uri = `${createBip21Uri(WALLET_ADDRESS, 2500)}&op_return_raw=deadbeef`;
            runWithChronik(CHRONIK_STUB, () => {
                visitWithWalletMnemonic(TEST_MNEMONIC, {
                    requireHoldToSend: false,
                });
                waitForMainLoaded();
                openManualSendScreen();
                pasteBip21UriIntoRecipient(uri);
                cy.get('#recipient-address')
                    .should('have.value', WALLET_ADDRESS)
                    .and('have.attr', 'readonly');
                cy.get('#send-amount')
                    .should('have.value', '25.00')
                    .and('have.attr', 'readonly');
                cy.get('#amount-slider').should('be.disabled');
                cy.get('#paybutton-logo-container').should(
                    'have.css',
                    'display',
                    'none',
                );
                cy.get('#fee-display').should('be.visible');
            });
        });
    });

    it('slider samples update amount input and fee rows in proportion', () => {
        const sampleFracs = [0, 0.25, 0.5, 0.75, 1];

        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();
            openManualSendScreen();
            cy.get('#recipient-address').clear().type(WALLET_ADDRESS).blur();

            cy.get('#amount-slider').then($slider => {
                const min = parseFloat(String($slider.attr('min')));
                const max = parseFloat(String($slider.attr('max')));
                if (!Number.isFinite(min) || !Number.isFinite(max)) {
                    throw new Error(
                        `invalid slider bounds min=${String($slider.attr('min'))} max=${String($slider.attr('max'))}`,
                    );
                }
                expect(
                    min,
                    'slider min = dust threshold (XEC primary)',
                ).to.equal(DUST_AMOUNT_XEC);
                expect(
                    max,
                    'slider max ≤ stub balance (XEC primary)',
                ).to.be.at.most(STUB_EXPECTED_TOTAL_XEC);
            });

            cy.wrap(sampleFracs).each(frac => {
                cy.get('#amount-slider').then($slider => {
                    const min = parseFloat(String($slider.attr('min')));
                    const max = parseFloat(String($slider.attr('max')));
                    if (!Number.isFinite(min) || !Number.isFinite(max)) {
                        throw new Error(
                            `invalid slider bounds min=${String($slider.attr('min'))} max=${String($slider.attr('max'))}`,
                        );
                    }
                    const valuePrimary = primaryForSliderFraction(
                        min,
                        max,
                        frac,
                    );
                    cy.wrap($slider)
                        .invoke('val', String(valuePrimary))
                        .trigger('input', { force: true });

                    // The amount input gets updated by the slider
                    cy.get('#send-amount').should('have.value', valuePrimary);

                    // So are the transaction details
                    cy.get('#fee-display')
                        .should('be.visible')
                        .and('not.have.class', 'error');
                    cy.get('#fee-display .fee-value-primary')
                        .should('have.length', 3)
                        .then($cells => {
                            const primaryValues = Array.from($cells).map(cell =>
                                parsePrimaryXecDisplay(
                                    cell.textContent ?? '',
                                ).toFixed(2),
                            );
                            const [sendAmount, networkFee, totalOut] =
                                primaryValues;
                            cy.log(
                                `[slider debug] frac=${frac} sendAmount=${sendAmount} networkFee=${networkFee} totalOut=${totalOut}`,
                            );
                            expect(
                                sendAmount,
                                `Amount row = slider primary at frac ${frac}`,
                            ).to.equal(valuePrimary);
                            expect(
                                (
                                    parseFloat(sendAmount) +
                                    parseFloat(networkFee)
                                ).toFixed(2),
                                `amount + fee = total at slider frac ${frac}`,
                            ).to.equal(totalOut);
                        });
                });
            });
        });
    });

    it('slider samples update amount input and fee rows when Firma is selected', () => {
        const sampleFracs = [0, 0.25, 0.5, 0.75, 1];
        const firmaDecimals = FIRMA_TOKEN.decimals;

        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();
            selectFirmaAsset();
            openManualSendScreen();
            cy.get('#recipient-address')
                .clear()
                .type(OTHER_RECIPIENT_ADDRESS)
                .blur();

            cy.get('#amount-slider').then($slider => {
                const min = parseFloat(String($slider.attr('min')));
                const max = parseFloat(String($slider.attr('max')));
                if (!Number.isFinite(min) || !Number.isFinite(max)) {
                    throw new Error(
                        `invalid slider bounds min=${String($slider.attr('min'))} max=${String($slider.attr('max'))}`,
                    );
                }
                expect(
                    min,
                    'slider min = smallest Firma primary step',
                ).to.equal(MIN_FIRMA_SLIDER_PRIMARY);
                expect(max, 'slider max ≤ stub Firma balance').to.be.at.most(
                    atomsToUnit(5_000, FIRMA_TOKEN.decimals),
                );
            });

            cy.wrap(sampleFracs).each(frac => {
                cy.get('#amount-slider').then($slider => {
                    const min = parseFloat(String($slider.attr('min')));
                    const max = parseFloat(String($slider.attr('max')));
                    if (!Number.isFinite(min) || !Number.isFinite(max)) {
                        throw new Error(
                            `invalid slider bounds min=${String($slider.attr('min'))} max=${String($slider.attr('max'))}`,
                        );
                    }
                    const valuePrimary = primaryForSliderFraction(
                        min,
                        max,
                        frac,
                        firmaDecimals,
                    );
                    cy.wrap($slider)
                        .invoke('val', String(valuePrimary))
                        .trigger('input', { force: true });

                    cy.get('#send-amount').should('have.value', valuePrimary);

                    cy.get('#fee-display')
                        .should('be.visible')
                        .and('not.have.class', 'error');
                    cy.get('#fee-display .fee-value-primary')
                        .should('have.length', 2)
                        .then($cells => {
                            const sendFirma = parsePrimaryXecDisplay(
                                $cells.get(0)?.textContent ?? '',
                            );
                            const networkFeeXec = parsePrimaryXecDisplay(
                                $cells.get(1)?.textContent ?? '',
                            );
                            cy.log(
                                `[Firma slider] frac=${frac} sendFirma=${sendFirma} feeXec=${networkFeeXec}`,
                            );
                            expect(
                                sendFirma,
                                `Amount row = slider primary at frac ${frac}`,
                            ).to.equal(parseFloat(valuePrimary));
                            expect(
                                networkFeeXec,
                                `network fee above dust at frac ${frac}`,
                            ).to.be.greaterThan(DUST_AMOUNT_XEC);
                        });
                });
            });
        });
    });

    it('amount input updates slider and transaction details', () => {
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();
            openManualSendScreen();
            cy.get('#recipient-address').clear().type(WALLET_ADDRESS).blur();

            const valuePrimary = (STUB_EXPECTED_TOTAL_XEC / 2).toFixed(2);
            cy.get('#send-amount')
                .clear()
                .type(valuePrimary)
                .should('have.value', valuePrimary);
            // The slider gets updated by the amount input
            cy.get('#amount-slider')
                .invoke('val')
                .then(val => {
                    expect(
                        parseFloat(String(val)),
                        'slider reflects amount input',
                    ).to.equal(parseFloat(valuePrimary));
                });
            // So are the transaction details
            cy.get('#fee-display')
                .should('be.visible')
                .and('not.have.class', 'error');
            cy.get('#fee-display .fee-value-primary')
                .should('have.length', 3)
                .then($cells => {
                    const primaryValues = Array.from($cells).map(cell =>
                        parsePrimaryXecDisplay(cell.textContent ?? '').toFixed(
                            2,
                        ),
                    );
                    const [sendAmount, networkFee, totalOut] = primaryValues;
                    expect(
                        sendAmount,
                        'Amount row matches typed primary',
                    ).to.equal(valuePrimary);
                    expect(
                        (
                            parseFloat(sendAmount) + parseFloat(networkFee)
                        ).toFixed(2),
                        'amount + fee = total after input change',
                    ).to.equal(totalOut);
                });
        });
    });

    it('amount input updates slider and transaction details when Firma is selected', () => {
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();
            selectFirmaAsset();
            openManualSendScreen();
            cy.get('#recipient-address')
                .clear()
                .type(OTHER_RECIPIENT_ADDRESS)
                .blur();

            cy.get('#send-amount')
                .invoke('attr', 'max')
                .then(maxStr => {
                    const max = parseFloat(String(maxStr));
                    if (!Number.isFinite(max)) {
                        throw new Error(
                            `invalid send-amount max: ${String(maxStr)}`,
                        );
                    }
                    const valuePrimary = (max / 2).toFixed(
                        FIRMA_TOKEN.decimals,
                    );
                    cy.get('#send-amount')
                        .clear()
                        .type(valuePrimary)
                        .should('have.value', valuePrimary);
                    cy.get('#amount-slider')
                        .invoke('val')
                        .then(val => {
                            expect(
                                parseFloat(String(val)),
                                'slider reflects amount input',
                            ).to.equal(parseFloat(valuePrimary));
                        });
                    cy.get('#fee-display')
                        .should('be.visible')
                        .and('not.have.class', 'error');
                    cy.get('#fee-display .fee-value-primary')
                        .should('have.length', 2)
                        .then($cells => {
                            const sendFirma = parsePrimaryXecDisplay(
                                $cells.get(0)?.textContent ?? '',
                            );
                            const networkFeeXec = parsePrimaryXecDisplay(
                                $cells.get(1)?.textContent ?? '',
                            );
                            expect(
                                sendFirma,
                                'Amount row matches typed primary',
                            ).to.equal(parseFloat(valuePrimary));
                            expect(
                                networkFeeXec,
                                'network fee above dust after input change',
                            ).to.be.greaterThan(DUST_AMOUNT_XEC);
                        });
                });
        });
    });

    describe('transaction details error heading', () => {
        it('shows dust error when amount is below minimum', () => {
            runWithChronik(CHRONIK_STUB, () => {
                visitWithWalletMnemonic(TEST_MNEMONIC, {
                    requireHoldToSend: false,
                });
                waitForMainLoaded();
                openManualSendScreen();
                cy.get('#recipient-address')
                    .clear()
                    .type(WALLET_ADDRESS)
                    .blur();
                cy.get('#send-amount').clear().type('0.01');
                cy.get('#fee-display').should('be.visible');
                cy.get('#fee-display').should('have.class', 'error');
                cy.get('#fee-display .fee-item.title').should(
                    'contain',
                    'Amount is too small',
                );
            });
        });

        it('shows insufficient funds when amount is above max spendable', () => {
            runWithChronik(CHRONIK_STUB, () => {
                visitWithWalletMnemonic(TEST_MNEMONIC, {
                    requireHoldToSend: false,
                });
                waitForMainLoaded();
                openManualSendScreen();
                cy.get('#recipient-address')
                    .clear()
                    .type(WALLET_ADDRESS)
                    .blur();
                cy.get('#send-amount')
                    .invoke('attr', 'max')
                    .then(maxStr => {
                        const max = parseFloat(String(maxStr));
                        if (!Number.isFinite(max)) {
                            throw new Error(
                                `invalid send-amount max: ${String(maxStr)}`,
                            );
                        }
                        const tooHigh = (max + 10_000).toFixed(2);
                        cy.get('#send-amount').clear().type(tooHigh);
                    });
                cy.get('#fee-display').should('be.visible');
                cy.get('#fee-display').should('have.class', 'error');
                cy.get('#fee-display .fee-item.title').should(
                    'contain',
                    'Insufficient funds',
                );
            });
        });

        it('shows error message when amount is below minimum with Firma selected', () => {
            runWithChronik(CHRONIK_STUB, () => {
                visitWithWalletMnemonic(TEST_MNEMONIC, {
                    requireHoldToSend: false,
                    /** 8 amount digits so a sub-min Firma value is not truncated to zero. */
                    primaryBalanceType: 'Fiat',
                });
                waitForMainLoaded();
                selectFirmaAsset();
                openManualSendScreen();
                cy.get('#recipient-address')
                    .clear()
                    .type(WALLET_ADDRESS)
                    .blur();
                cy.get('#send-amount')
                    .clear()
                    .type(
                        (MIN_FIRMA_SLIDER_PRIMARY / 10).toFixed(
                            FIRMA_TOKEN.decimals + 1,
                        ),
                    )
                    .blur();
                cy.get('#fee-display').should('be.visible');
                cy.get('#fee-display').should('have.class', 'error');
                cy.get('#fee-display .fee-item.title').should(
                    'contain',
                    'Amount is too small',
                );
            });
        });

        it('shows insufficient funds when amount is above max spendable with Firma selected', () => {
            runWithChronik(CHRONIK_STUB, () => {
                visitWithWalletMnemonic(TEST_MNEMONIC, {
                    requireHoldToSend: false,
                });
                waitForMainLoaded();
                selectFirmaAsset();
                openManualSendScreen();
                cy.get('#recipient-address')
                    .clear()
                    .type(WALLET_ADDRESS)
                    .blur();
                cy.get('#send-amount')
                    .invoke('attr', 'max')
                    .then(maxStr => {
                        const max = parseFloat(String(maxStr));
                        if (!Number.isFinite(max)) {
                            throw new Error(
                                `invalid send-amount max: ${String(maxStr)}`,
                            );
                        }
                        const tooHigh = (max + 10_000).toFixed(
                            FIRMA_TOKEN.decimals,
                        );
                        cy.get('#send-amount').clear().type(tooHigh);
                    });
                cy.get('#fee-display').should('be.visible');
                cy.get('#fee-display').should('have.class', 'error');
                cy.get('#fee-display .fee-item.title').should(
                    'contain',
                    'Insufficient funds',
                );
            });
        });

        it('shows insufficient XEC for network fee when Firma is selected and there is no native XEC UTXO', () => {
            runWithChronik(CHRONIK_STUB_NO_XEC, () => {
                visitWithWalletMnemonic(TEST_MNEMONIC, {
                    requireHoldToSend: false,
                });
                waitForMainLoaded();
                selectFirmaAsset();
                openManualSendScreen();
                cy.get('#recipient-address')
                    .clear()
                    .type(WALLET_ADDRESS)
                    .blur();
                cy.get('#send-amount')
                    .invoke('attr', 'max')
                    .then(maxStr => {
                        const max = parseFloat(String(maxStr));
                        if (!Number.isFinite(max)) {
                            throw new Error(
                                `invalid send-amount max: ${String(maxStr)}`,
                            );
                        }
                        const valuePrimary = (max / 2).toFixed(
                            FIRMA_TOKEN.decimals,
                        );
                        cy.get('#send-amount')
                            .clear()
                            .type(valuePrimary)
                            .should('have.value', valuePrimary);
                    });
                cy.get('#fee-display').should('be.visible');
                cy.get('#fee-display').should('have.class', 'error');
                cy.get('#fee-display .fee-item.title').should(
                    'contain',
                    'Insufficient XEC for network fee',
                );
            });
        });
    });

    it('header back button and cancel button return to main', () => {
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();
            openManualSendScreen();
            cy.get('#back-btn').click();
            assertReturnedToMain();
            openManualSendScreen();
            cy.get('#cancel-send').click();
            assertReturnedToMain();
        });
    });

    it('disabling require hold to send updates the send button after reopening send', () => {
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC);
            waitForMainLoaded();
            openManualSendScreen();
            fillRecipientAndMaxSlider();
            cy.get('#confirm-send span').should('have.text', 'Hold to Send');

            cy.get('#back-btn').click();
            assertReturnedToMain();
            cy.get('.settings-button').click();
            cy.get('#settings-screen').should('not.have.class', 'hidden');
            cy.get('#hold-to-send-toggle').should('be.checked');
            cy.get('#hold-to-send-toggle').uncheck({ force: true });
            cy.get('#settings-back-btn').click();
            cy.get('#main-screen').should('be.visible');

            openManualSendScreen();
            fillRecipientAndMaxSlider();
            cy.get('#confirm-send span').should('have.text', 'Send');
        });
    });

    it('fee details show dual currency; fiat primary swaps send + fee units', () => {
        runWithChronik(CHRONIK_STUB, () => {
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();
            openManualSendScreen();
            fillRecipientAndMaxSlider();

            // XEC primary: amount/slider in XEC; fee rows show XEC then USD.
            cy.get('#fee-display').should('be.visible');
            cy.get('#ticker-label').should('contain', 'XEC');
            cy.get('#send-amount').should('have.attr', 'step', '0.01');
            cy.get('#amount-slider').should('have.attr', 'step', '0.01');
            cy.get('#fee-display .fee-item .fee-value').should(
                'have.length',
                3,
            );
            cy.get('#fee-display .fee-item .fee-value').each($wrap => {
                cy.wrap($wrap)
                    .find('.fee-value-primary')
                    .invoke('text')
                    .should('match', /XEC/i);
                cy.wrap($wrap)
                    .find('.fee-value-secondary')
                    .invoke('text')
                    .should('match', /\$/);
            });

            cy.get('#back-btn').click();
            assertReturnedToMain();
            cy.get('.settings-button').click();
            cy.get('#settings-screen').should('not.have.class', 'hidden');
            cy.get('#primary-balance-toggle').should('not.be.checked');
            cy.get('#primary-balance-toggle').check({ force: true });
            cy.get('#primary-balance-toggle').should('be.checked');
            cy.get('#settings-back-btn').click();
            cy.get('#main-screen').should('be.visible');

            openManualSendScreen();
            fillRecipientAndMaxSlider();

            // Fiat primary: amount/slider in fiat; fee rows show USD then XEC.
            cy.get('#fee-display').should('be.visible');
            cy.get('#ticker-label').should('contain', 'USD');
            cy.get('#send-amount').should('have.attr', 'step', '0.00000001');
            cy.get('#amount-slider').should('have.attr', 'step', '0.00000001');
            cy.get('#fee-display .fee-item .fee-value').should(
                'have.length',
                3,
            );
            cy.get('#fee-display .fee-item .fee-value').each($wrap => {
                cy.wrap($wrap)
                    .find('.fee-value-primary')
                    .invoke('text')
                    .should('match', /\$/);
                cy.wrap($wrap)
                    .find('.fee-value-secondary')
                    .invoke('text')
                    .should('match', /XEC/i);
            });
        });
    });

    it('sends max to self', () => {
        runWithChronik(CHRONIK_STUB, () => {
            stubChronikBroadcastsSuccess();
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();

            openManualSendScreen();
            fillRecipientAndMaxSlider();

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

    it('sends max Firma to self', () => {
        runWithChronik(CHRONIK_STUB, () => {
            stubChronikBroadcastsSuccess();
            visitWithWalletMnemonic(TEST_MNEMONIC, {
                requireHoldToSend: false,
            });
            waitForMainLoaded();

            selectFirmaAsset();

            cy.get('#primary-balance').should($el => {
                expect(normalizeBalanceText($el.text())).to.equal(
                    '0.5000 FIRMA',
                );
            });

            openManualSendScreen();
            fillRecipientAndMaxSlider();

            cy.get('#ticker-label').should('contain', 'FIRMA');
            cy.get('#send-amount').should('have.attr', 'step', '0.0001');

            cy.get('#fee-display')
                .should('be.visible')
                .and('not.have.class', 'error');
            cy.get('#fee-display .fee-item.title').should(
                'contain',
                'Transaction Details',
            );
            cy.get('#fee-display').within(() => {
                cy.contains('.fee-label', 'Amount:');
                cy.contains('.fee-label', 'Network Fee');
            });
            cy.get('#send-amount')
                .invoke('val')
                .then(sendVal => {
                    const expectedFirma = parsePrimaryXecDisplay(
                        String(sendVal),
                    );
                    cy.get('#fee-display .fee-value-primary')
                        .should('have.length', 2)
                        .then($cells => {
                            const primaryValues = Array.from($cells).map(cell =>
                                parsePrimaryXecDisplay(cell.textContent ?? ''),
                            );
                            const [sendAmountFirma, networkFeeXec] =
                                primaryValues;
                            expect(
                                sendAmountFirma,
                                'fee amount row matches send field (max Firma)',
                            ).to.equal(expectedFirma);
                            // When sending to self, the gas amount is also sent
                            // to self so it is not accounted
                            expect(
                                networkFeeXec,
                                'transaction details network fee (XEC) above zero',
                            ).to.be.greaterThan(0);
                        });
                });

            cy.get('#confirm-send').should('not.be.disabled').click();

            cy.wait('@chronikBroadcastsSuccess').then(interception => {
                expect(interception.request.method).to.eq('POST');
                expect(interception.request.url).to.match(
                    /\/broadcast-txs(\?|$)/,
                );
            });

            cy.get('#main-screen').should('be.visible');
        });
    });
});
