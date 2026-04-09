// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import * as bip39 from 'bip39';

import {
    openSettingsFromMain,
    visitFresh,
    waitForMainLoaded,
} from '../fixture/common';
import { stubCoingeckoXecFiatPrices } from '../fixture/stubs';

/**
 * Same mnemonic as cashtab/src/wallet/fixtures/vectors.js (createCashtabWallet).
 * ecash-wallet derives the same receiving address as Cashtab for this phrase.
 * This ensures the address is also the same as the one displayed in Cashtab,
 * and so that the wallets are compatible and can be moved by setting the
 * mnemonic from one to the other.
 */
const CASHTAB_TEST_MNEMONIC =
    'beauty shoe decline spend still weird slot snack coach flee between paper';

const CASHTAB_TEST_ADDRESS = 'ecash:qqa9lv3kjd8vq7952p7rq0f6lkpqvlu0cydvxtd70g';

/**
 * True when text is exactly twelve whitespace-separated English BIP39 words and
 * the phrase passes bip39 checksum validation.
 */
function isValidMnemonic(text: string): boolean {
    const trimmed = text.trim();
    if (!trimmed) {
        return false;
    }
    const words = trimmed.split(/\s+/);
    if (words.length !== 12) {
        return false;
    }
    return bip39.validateMnemonic(trimmed, bip39.wordlists['english']);
}

describe('Mnemonic', () => {
    beforeEach(() => {
        stubCoingeckoXecFiatPrices();
    });

    it('toggles blur on the mnemonic textarea when clicking the show button', () => {
        visitFresh();
        waitForMainLoaded();
        openSettingsFromMain();

        cy.get('#mnemonic-text').should($ta => {
            expect(isValidMnemonic(String($ta.val()))).to.equal(true);
        });

        // Initially blurred
        cy.get('#mnemonic-text').should('have.class', 'blurred');

        cy.get('#show-mnemonic-btn').click();
        cy.get('#mnemonic-text').should('not.have.class', 'blurred');

        cy.get('#show-mnemonic-btn').click();
        cy.get('#mnemonic-text').should('have.class', 'blurred');
    });

    it('opens the edit overlay with inputs prefilled from the current mnemonic', () => {
        visitFresh();
        waitForMainLoaded();
        openSettingsFromMain();

        cy.get('#mnemonic-text')
            .invoke('val')
            .then(raw => {
                const phrase = String(raw).trim();
                expect(isValidMnemonic(phrase)).to.equal(true);
                const words = phrase.split(/\s+/);

                // Modal is initially hidden
                cy.get('#mnemonic-edit-modal').should('have.class', 'hidden');

                cy.get('#edit-mnemonic-btn').click();
                cy.get('#mnemonic-edit-modal').should(
                    'not.have.class',
                    'hidden',
                );

                // The fields contain the correct words from the current
                // mnemonic
                cy.get(
                    '#mnemonic-words-container input.mnemonic-word-input',
                ).should($inputs => {
                    expect($inputs).to.have.length(12);
                    for (let i = 0; i < 12; i++) {
                        expect(
                            ($inputs.get(i) as HTMLInputElement).value,
                        ).to.equal(words[i]);
                    }
                });
            });
    });

    it('clears all word fields when Clear All is clicked', () => {
        visitFresh();
        waitForMainLoaded();
        openSettingsFromMain();

        cy.get('#edit-mnemonic-btn').click();
        cy.get('#mnemonic-edit-modal').should('not.have.class', 'hidden');

        cy.get('#clear-mnemonic-btn').click();
        cy.get('#mnemonic-words-container input.mnemonic-word-input').should(
            $inputs => {
                $inputs.each((_i, el) => {
                    expect((el as HTMLInputElement).value).to.equal('');
                });
            },
        );
    });

    it('shows a filtered BIP39 list and fills the word when a suggestion is chosen', () => {
        visitFresh();
        waitForMainLoaded();
        openSettingsFromMain();

        cy.get('#edit-mnemonic-btn').click();
        cy.get('#mnemonic-edit-modal').should('not.have.class', 'hidden');
        cy.get('#clear-mnemonic-btn').click();

        cy.get('#mnemonic-words-container input[data-word-index="0"]')
            .clear()
            .type('ab', { delay: 0 });

        cy.get('#mnemonic-edit-modal .mnemonic-word-dropdown')
            .filter(':visible')
            .should('have.length', 1)
            .find('.mnemonic-dropdown-option')
            .should($opts => {
                expect($opts.length).to.be.at.least(2);
                const texts = [...$opts].map(
                    el => (el as HTMLElement).textContent?.trim() ?? '',
                );
                expect(texts).to.include('abandon');
                expect(texts).to.include('ability');
            });

        cy.get('#mnemonic-edit-modal .mnemonic-word-dropdown')
            .filter(':visible')
            .contains('.mnemonic-dropdown-option', 'ability')
            .click();

        cy.get('#mnemonic-words-container input[data-word-index="0"]').should(
            'have.value',
            'ability',
        );
        cy.get('#mnemonic-words-container input[data-word-index="1"]').should(
            'have.focus',
        );
    });

    it('closes the overlay when Cancel is clicked', () => {
        visitFresh();
        waitForMainLoaded();
        openSettingsFromMain();

        cy.get('#edit-mnemonic-btn').click();
        cy.get('#mnemonic-edit-modal').should('not.have.class', 'hidden');

        cy.get('#cancel-mnemonic-edit').click();
        cy.get('#mnemonic-edit-modal').should('have.class', 'hidden');
    });

    it('shows an error and keeps the wallet unchanged when Save is clicked with empty or invalid mnemonic', () => {
        const invalidChecksumTwelveWords = Array(12).fill('zoo').join(' ');
        const invalidMnemonicMessageEn =
            'Invalid mnemonic. Please enter a valid 12-word recovery phrase.';

        expect(
            bip39.validateMnemonic(
                invalidChecksumTwelveWords,
                bip39.wordlists['english'],
            ),
        ).to.equal(false);

        visitFresh();
        waitForMainLoaded();
        openSettingsFromMain();

        cy.get('#mnemonic-text')
            .invoke('val')
            .then(original => {
                const originalPhrase = String(original).trim();
                expect(isValidMnemonic(originalPhrase)).to.equal(true);

                cy.get('#edit-mnemonic-btn').click();
                cy.get('#mnemonic-edit-modal').should(
                    'not.have.class',
                    'hidden',
                );
                cy.get('#clear-mnemonic-btn').click();

                cy.get('#save-mnemonic-edit').click();

                cy.get('#mnemonic-validation')
                    .should('be.visible')
                    .and('have.class', 'error')
                    .and('contain.text', invalidMnemonicMessageEn);

                cy.get('#mnemonic-edit-modal').should(
                    'not.have.class',
                    'hidden',
                );

                cy.get('#mnemonic-text')
                    .invoke('val')
                    .then(v => {
                        expect(String(v).trim()).to.equal(originalPhrase);
                    });

                cy.get('#cancel-mnemonic-edit').click();
                cy.get('#mnemonic-edit-modal').should('have.class', 'hidden');
                cy.get('#mnemonic-validation').should('not.be.visible');

                cy.get('#edit-mnemonic-btn').click();
                cy.get('#mnemonic-edit-modal').should(
                    'not.have.class',
                    'hidden',
                );
                cy.get('#mnemonic-validation').should('not.be.visible');

                const originalWords = originalPhrase.split(/\s+/);
                originalWords.forEach((word, i) => {
                    cy.get(
                        `#mnemonic-words-container input[data-word-index="${i}"]`,
                    ).should('have.value', word);
                });

                cy.get('#clear-mnemonic-btn').click();

                const words = invalidChecksumTwelveWords.split(' ');
                words.forEach((word, i) => {
                    cy.get(
                        `#mnemonic-words-container input[data-word-index="${i}"]`,
                    )
                        .clear()
                        .type(word, { delay: 0 });
                });

                cy.get('#save-mnemonic-edit').click();

                cy.get('#mnemonic-validation')
                    .should('be.visible')
                    .and('have.class', 'error')
                    .and('contain.text', invalidMnemonicMessageEn);

                cy.get('#mnemonic-edit-modal').should(
                    'not.have.class',
                    'hidden',
                );

                cy.get('#mnemonic-text')
                    .invoke('val')
                    .then(v => {
                        expect(String(v).trim()).to.equal(originalPhrase);
                    });
            });
    });

    it('saves a new mnemonic, returns to main with updated address, and settings show the new phrase', () => {
        visitFresh();
        waitForMainLoaded();
        openSettingsFromMain();

        cy.get('#edit-mnemonic-btn').click();
        cy.get('#mnemonic-edit-modal').should('not.have.class', 'hidden');
        cy.get('#clear-mnemonic-btn').click();

        const words = CASHTAB_TEST_MNEMONIC.split(' ');
        words.forEach((word, i) => {
            cy.get(`#mnemonic-words-container input[data-word-index="${i}"]`)
                .clear()
                .type(word, { delay: 0 });
        });

        cy.get('#save-mnemonic-edit').click();

        cy.get('#address').should($el => {
            expect($el.text().trim()).to.equal(CASHTAB_TEST_ADDRESS);
        });

        cy.get('#main-screen').should('be.visible');
        cy.get('#mnemonic-edit-modal').should('have.class', 'hidden');

        openSettingsFromMain();

        cy.get('#mnemonic-text')
            .invoke('val')
            .then(v => {
                const phrase = String(v).trim();
                expect(phrase).to.equal(CASHTAB_TEST_MNEMONIC);
            });
    });
});
