// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

describe('Main screen', () => {
    const visitFresh = () => {
        cy.visit('/', {
            onBeforeLoad(win) {
                win.localStorage.clear();
            },
        });
    };

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
});
