// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/// <reference types="cypress" />

import {
    applyChronikStubIntercepts,
    chronikStubPath,
    type ChronikStub,
    type ChronikStubRunContext,
} from './chronik-json-protobuf';

/**
 * Load a Chronik stub JSON, intercept matching `history` and `/tx/*`, and run `body`.
 * `cy.readFile` parses `.json` into an object (Cypress default); see
 * `chronik-json-protobuf.ts`.
 */
export function runWithChronik(
    fileNameOrPath: string,
    body: (ctx: ChronikStubRunContext) => void,
): void {
    const path = chronikStubPath(fileNameOrPath);
    cy.readFile(path).then(data => {
        const stub = data as ChronikStub;
        applyChronikStubIntercepts(stub);
        body({ stub });
    });
}

/**
 * CoinGecko response shape for ecash-price CoinGeckoProvider (see
 * modules/ecash-price). Intercepting avoids flaky secondary balance when the
 * API is down or rate-limited. Replies include whichever vs_currencies the app
 * requested (usd and/or eur).
 */
export function stubCoingeckoXecFiatPrices(): void {
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
