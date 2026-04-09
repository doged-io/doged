// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/// <reference types="cypress" />

/**
 * Visit the app root with cleared localStorage so each test starts from defaults.
 */
export function visitFresh(): void {
    cy.visit('/', {
        onBeforeLoad(win) {
            win.localStorage.clear();
        },
    });
}

/**
 * Visit the app with a fixed mnemonic pre-seeded (web storage key
 * ecash_wallet_mnemonic). Use with Chronik stubs that target the same wallet.
 */
export function visitWithWalletMnemonic(mnemonic: string): void {
    cy.visit('/', {
        onBeforeLoad(win) {
            win.localStorage.clear();
            win.localStorage.setItem('ecash_wallet_mnemonic', mnemonic);
        },
    });
}

/**
 * Assert the startup loading overlay is gone and the main screen is visible.
 */
export function waitForMainLoaded(): void {
    cy.get('#loading').should('not.be.visible');
    cy.get('#main-screen').should('be.visible');
}

/**
 * Normalize balance / fiat strings from the DOM for assertions (NBSP → space,
 * trim).
 */
export function normalizeBalanceText(s: string): string {
    return s.replace(/\u00a0/g, ' ').trim();
}

/**
 * Open the settings screen from the main wallet view.
 */
export function openSettingsFromMain(): void {
    cy.get('#main-screen').should('be.visible');
    cy.get('.settings-button').click();
    cy.get('#settings-screen').should('be.visible');
}
