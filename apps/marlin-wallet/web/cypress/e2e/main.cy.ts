// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * CoinGecko response shape for ecash-price CoinGeckoProvider (see
 * modules/ecash-price). Intercepting avoids flaky secondary balance when the
 * API is down or rate-limited.
 */
function stubCoingeckoXecFiatPrices() {
    cy.intercept(
        { method: 'GET', url: /api\.coingecko\.com\/api\/v3\/simple\/price/ },
        req => {
            const url = new URL(req.url);
            const vsRaw = url.searchParams.get('vs_currencies') || '';
            const parts = vsRaw
                .split(',')
                .map(s => s.trim().toLowerCase())
                .filter(Boolean);
            const lastUpdated = Math.floor(Date.now() / 1000);
            const ecash: Record<string, number> = {
                last_updated_at: lastUpdated,
            };
            if (parts.includes('usd')) {
                ecash.usd = 0.00005;
            }
            if (parts.includes('eur')) {
                ecash.eur = 0.000046;
            }
            req.reply({ statusCode: 200, body: { ecash } });
        },
    );
}

/**
 * Collapse Unicode spaces so currency strings match across browsers.
 * This avoid hard to debug failures when the locale uses non-ASCII spaces.
 * For example, the German locale uses a non-breaking space (U+00A0) in the
 * currency strings.
 */
function normalizeBalanceText(s: string): string {
    return s.replace(/\u00a0/g, ' ').trim();
}

describe('Main screen', () => {
    const visitFresh = () => {
        cy.visit('/', {
            onBeforeLoad(win) {
                win.localStorage.clear();
            },
        });
    };

    const waitForMainLoaded = () => {
        cy.get('#loading').should('not.be.visible');
        cy.get('#main-screen').should('be.visible');
    };

    const openSettingsFromMain = () => {
        cy.get('#main-screen').should('be.visible');
        cy.get('.settings-button').click();
        cy.get('#settings-screen').should('be.visible');
    };

    const returnToMainFromSettings = () => {
        cy.get('#settings-back-btn').click();
        cy.get('#main-screen').should('be.visible');
    };

    beforeEach(() => {
        stubCoingeckoXecFiatPrices();
    });

    it('shows then hides the loading overlay during startup', () => {
        visitFresh();

        // Overlay is shown after i18n init with loading.authenticationRequired
        // We can safely assume the default locale is used in this test
        // (en.json) since the local storage is cleared before the test.
        cy.get('#loading').should('be.visible');
        cy.get('#loading .loading-text').should(
            'contain',
            'Authentication required',
        );
        cy.get('#loading .loading-spinner').should('exist');

        cy.get('#loading').should('not.be.visible');
        cy.get('#main-screen').should('be.visible');
    });

    it('displays zero XEC primary balance and USD secondary balance', () => {
        visitFresh();

        waitForMainLoaded();

        // Default settings: primary XEC, fiat USD (en locale)
        cy.get('#primary-balance')
            .should('be.visible')
            .invoke('text')
            .then(t => t.trim())
            .should('eq', '0.00 XEC');

        cy.get('#secondary-balance')
            .should('be.visible')
            .invoke('text')
            .then(t => t.trim())
            .should('eq', '$0.00');
    });

    it('inverts balance ordering when fiat is the primary balance', () => {
        visitFresh();
        waitForMainLoaded();

        cy.get('#primary-balance')
            .invoke('text')
            .then(t => t.trim())
            .should('eq', '0.00 XEC');
        cy.get('#secondary-balance')
            .invoke('text')
            .then(t => t.trim())
            .should('eq', '$0.00');

        openSettingsFromMain();
        cy.get('#primary-balance-toggle').should('not.be.checked');
        // Input is purposely visually hidden (0×0); the slider is the visible
        // control (see src/main.css .toggle-switch input).
        // We force the check to avoid cypress from failing due to the input
        // being hidden.
        cy.get('#primary-balance-toggle').check({ force: true });
        cy.get('#primary-balance-toggle').should('be.checked');

        returnToMainFromSettings();

        // primaryBalanceType === 'Fiat': primary is fiat, secondary is XEC
        cy.get('#primary-balance')
            .should('be.visible')
            .invoke('text')
            .then(t => normalizeBalanceText(t))
            .should('eq', '$0.00');
        cy.get('#secondary-balance')
            .should('be.visible')
            .invoke('text')
            .then(t => normalizeBalanceText(t))
            .should('eq', '0.00 XEC');

        openSettingsFromMain();
        // Input is purposely visually hidden (0×0); the slider is the visible
        // control (see src/main.css .toggle-switch input).
        // We force the check to avoid cypress from failing due to the input
        // being hidden.
        cy.get('#primary-balance-toggle').uncheck({ force: true });
        cy.get('#primary-balance-toggle').should('not.be.checked');

        returnToMainFromSettings();

        cy.get('#primary-balance')
            .invoke('text')
            .then(t => normalizeBalanceText(t))
            .should('eq', '0.00 XEC');
        cy.get('#secondary-balance')
            .invoke('text')
            .then(t => normalizeBalanceText(t))
            .should('eq', '$0.00');
    });

    it('updates balance display when locale then fiat currency change', () => {
        visitFresh();
        waitForMainLoaded();

        cy.get('#primary-balance')
            .invoke('text')
            .then(t => t.trim())
            .should('eq', '0.00 XEC');
        cy.get('#secondary-balance')
            .invoke('text')
            .then(t => t.trim())
            .should('eq', '$0.00');

        openSettingsFromMain();
        cy.get('#language-select').select('de');
        cy.get('#settings-screen h2').should('contain', 'Einstellungen');

        returnToMainFromSettings();

        cy.get('#primary-balance')
            .invoke('text')
            .then(t => normalizeBalanceText(t))
            .should('eq', '0,00 XEC');
        cy.get('#secondary-balance')
            .invoke('text')
            .then(t => normalizeBalanceText(t))
            .should('eq', '0,00 $');

        openSettingsFromMain();
        cy.get('#fiat-currency-select').select('eur');
        cy.get('#fiat-currency-select').should('have.value', 'eur');

        returnToMainFromSettings();

        cy.get('#primary-balance')
            .invoke('text')
            .then(t => normalizeBalanceText(t))
            .should('eq', '0,00 XEC');
        cy.get('#secondary-balance')
            .invoke('text')
            .then(t => normalizeBalanceText(t))
            .should('eq', '0,00 €');
    });

    it('displays wallet receiving address and QR code', () => {
        visitFresh();

        cy.get('#loading').should('not.be.visible');
        cy.get('#address')
            .should('be.visible')
            .invoke('text')
            .then(text => text.trim())
            .should('match', /^ecash:[a-z0-9]+$/i);

        cy.get('#qr-code canvas').should($canvasList => {
            expect($canvasList).to.have.length(1);
            const el = $canvasList.get(0) as HTMLCanvasElement;
            expect(el.width).to.be.greaterThan(0);
            expect(el.height).to.be.greaterThan(0);
        });

        cy.get('#qr-code').should('not.contain', 'QR Code generation failed');
    });
});
