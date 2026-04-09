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
 * Assert the startup loading overlay is gone and the main screen is visible.
 */
export function waitForMainLoaded(): void {
    cy.get('#loading').should('not.be.visible');
    cy.get('#main-screen').should('be.visible');
}

/**
 * Open the settings screen from the main wallet view.
 */
export function openSettingsFromMain(): void {
    cy.get('#main-screen').should('be.visible');
    cy.get('.settings-button').click();
    cy.get('#settings-screen').should('be.visible');
}
