// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/// <reference types="cypress" />

import { DEFAULT_LOCALE } from '../../../i18n/locales';

/**
 * Partial overrides for `ecashwallet.settings.1` (stored JSON in
 * `src/settings.ts`). Omitted keys use the same defaults as `loadSettings()`
 * there when no saved settings exist.
 */
export type VisitWalletMnemonicSettings = {
    requireHoldToSend?: boolean;
    primaryBalanceType?: 'XEC' | 'Fiat';
    /** Fiat code string, e.g. `USD` (as persisted by `saveSettings`). */
    fiatCurrency?: string;
    locale?: string;
};

const DEFAULT_STORED_WALLET_SETTINGS = {
    requireHoldToSend: true,
    primaryBalanceType: 'XEC' as const,
    fiatCurrency: 'USD',
    locale: DEFAULT_LOCALE,
};

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
 * Visit the app with a fixed mnemonic pre-seeded (`ecash_wallet_mnemonic`).
 * Use with Chronik stubs that target the same wallet.
 *
 * If `settings` is omitted, `ecashwallet.settings.1` is not written and the app
 * uses `loadSettings()` defaults from `src/settings.ts`. If `settings` is
 * passed, each field falls back to those same defaults when not specified, then
 * the merged object is stored before load.
 */
export function visitWithWalletMnemonic(
    mnemonic: string,
    settings?: VisitWalletMnemonicSettings,
): void {
    cy.visit('/', {
        onBeforeLoad(win) {
            win.localStorage.clear();
            win.localStorage.setItem('ecash_wallet_mnemonic', mnemonic);
            if (settings !== undefined) {
                win.localStorage.setItem(
                    'ecashwallet.settings.1',
                    JSON.stringify({
                        ...DEFAULT_STORED_WALLET_SETTINGS,
                        ...settings,
                    }),
                );
            }
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
