// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/// <reference types="cypress" />

import * as proto from '../../../../../modules/chronik-client/proto/chronik';

import {
    applyChronikStubIntercepts,
    CHRONIK_PROTOBUF_HEADERS,
    chronikAsProtobufBody,
    chronikStubPath,
    cypressRequestBodyToUint8Array,
    randomTxidBase64ForProtobufJson,
    type ChronikStub,
    type ChronikStubRunContext,
} from './chronik-json-protobuf';

/**
 * Stub `POST …/broadcast-txs` with a successful `BroadcastTxsResponse`. Decodes
 * the request and returns one random txid per `rawTx` (same shape ecash-wallet
 * expects from {@link WalletAction.broadcast}).
 *
 * Register before `cy.visit` (e.g. inside `runWithChronik` after GET stubs).
 * Route alias: `chronikBroadcastsSuccess` (use `cy.wait('@chronikBroadcastsSuccess')`).
 *
 * Chronik’s `POST /broadcast-tx` (singular) is not stubbed; only the batched
 * endpoint used by ecash-wallet `broadcastTxs` is matched.
 */
export function stubChronikBroadcastsSuccess(): void {
    cy.intercept({ method: 'POST', url: /\/broadcast-txs(\?|$)/ }, req => {
        let rawTxCount: number;
        try {
            const decoded = proto.BroadcastTxsRequest.decode(
                cypressRequestBodyToUint8Array(req.body),
            );
            rawTxCount = decoded.rawTxs.length;
        } catch {
            req.continue();
            return;
        }
        const txidsJson = Array.from({ length: rawTxCount }, () =>
            randomTxidBase64ForProtobufJson(),
        );
        const out = proto.BroadcastTxsResponse.encode(
            proto.BroadcastTxsResponse.fromJSON({ txids: txidsJson }),
        ).finish();
        req.reply({
            statusCode: 200,
            headers: CHRONIK_PROTOBUF_HEADERS,
            body: chronikAsProtobufBody(out),
        });
    }).as('chronikBroadcastsSuccess');
}

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
