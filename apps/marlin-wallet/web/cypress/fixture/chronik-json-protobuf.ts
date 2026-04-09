// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/// <reference types="cypress" />

import * as proto from '../../../../../modules/chronik-client/proto/chronik';
import { toHexRev } from '../../../../../modules/chronik-client/src/hex';

/**
 * Single reviewable JSON for Chronik Cypress stubs: history pages (protobuf
 * JSON) plus full `Tx` objects keyed by txid.
 */
export interface ChronikStub {
    meta: {
        address: string;
        p2pkhPayloadHex: string;
        exportedAt: string;
        pageSize: number;
        /** Chronik `TxHistoryPage.numPages` for this script history. */
        historyNumPages: number;
        /** Chronik `TxHistoryPage.numTxs` (total txs for the address). */
        historyNumTxs: number;
    };
    /** `TxHistoryPage` protobuf-JSON per page index (`"0"`, `"1"`, …). */
    historyPages: Record<string, unknown>;
    /** `Tx` protobuf-JSON keyed by txid */
    txs: Record<string, unknown>;
}

/** Context passed to {@link runWithChronik} in `stubs.ts`. */
export type ChronikStubRunContext = {
    stub: ChronikStub;
};

const CHRONIK_STUB_DIR = 'cypress/fixture/chronik';

const CHRONIK_PROTOBUF_HEADERS = {
    'content-type': 'application/x-protobuf',
} as const;

/**
 * Encode a `TxHistoryPage` from protobuf-JSON (ts-proto `fromJSON` shape) to
 * wire bytes for Chronik `application/x-protobuf` stubs.
 */
function encodeHistoryPageFromFixtureJson(parsed: unknown): Uint8Array {
    const msg = proto.TxHistoryPage.fromJSON(parsed);
    return proto.TxHistoryPage.encode(msg).finish();
}

/**
 * Encode a full `Tx` from protobuf-JSON for `GET /tx/<id>` stubs.
 */
function encodeTxFromFixtureJson(parsed: unknown): Uint8Array {
    const msg = proto.Tx.fromJSON(parsed);
    return proto.Tx.encode(msg).finish();
}

/**
 * Explorer / Chronik URL txid (hex, reversed byte order) for a `Tx` JSON
 * fixture, lowercase.
 */
function lowerCaseTxidFromTxJson(tx: unknown): string {
    const msg = proto.Tx.fromJSON(tx);
    return toHexRev(msg.txid).toLowerCase();
}

/**
 * Resolve a Chronik stub path: basename only is looked up under `cypress/fixture/chronik/`.
 */
export function chronikStubPath(fileNameOrPath: string): string {
    if (fileNameOrPath.includes('/')) {
        return fileNameOrPath;
    }
    return `${CHRONIK_STUB_DIR}/${fileNameOrPath}`;
}

function chronikAsProtobufBody(data: Cypress.Buffer | Uint8Array): ArrayBuffer {
    const u8 =
        data instanceof Uint8Array
            ? data
            : new Uint8Array(data as unknown as Iterable<number>);
    return u8.buffer.slice(u8.byteOffset, u8.byteOffset + u8.byteLength);
}

function chronikHistoryUrlRegex(bundle: ChronikStub): RegExp {
    const hex = bundle.meta.p2pkhPayloadHex.toLowerCase();
    return new RegExp(`/script/p2pkh/${hex}/history(\\?|$)`);
}

function chronikEncodeTxStubMap(
    bundle: ChronikStub,
): Record<string, Uint8Array> {
    const out: Record<string, Uint8Array> = {};
    for (const [id, txJson] of Object.entries(bundle.txs)) {
        out[id.toLowerCase()] = encodeTxFromFixtureJson(txJson);
    }
    return out;
}

function chronikTxIdsOnHistoryPage(pageJson: unknown): string[] {
    const page = pageJson as { txs?: unknown[] };
    if (!page.txs?.length) {
        return [];
    }
    return page.txs.map(tx => lowerCaseTxidFromTxJson(tx));
}

function chronikRegisterTxStubs(
    urlRe: RegExp,
    bodies: Record<string, Uint8Array>,
): void {
    cy.intercept({ method: 'GET', url: urlRe }, req => {
        const matches = req.url.match(/\/tx\/([0-9a-fA-F]+)/);
        if (!matches) {
            req.continue();
            return;
        }
        const id = matches[1].toLowerCase();
        const body = bodies[id];
        if (!body) {
            req.continue();
            return;
        }
        req.reply({
            statusCode: 200,
            headers: CHRONIK_PROTOBUF_HEADERS,
            body: chronikAsProtobufBody(body),
        });
    });
}

/**
 * Register Cypress intercepts for Chronik `history` and `/tx/*` from a parsed
 * stub.
 */
export function applyChronikStubIntercepts(bundle: ChronikStub): void {
    const historyUrlRe = chronikHistoryUrlRegex(bundle);

    const pages = bundle.historyPages;
    if (pages['0'] === undefined) {
        throw new Error('Chronik bundle missing historyPages["0"]');
    }
    const encodedByPage: Record<string, Uint8Array> = {};
    const need = new Set<string>();
    for (const key of Object.keys(pages).sort(
        (a, b) => Number(a) - Number(b),
    )) {
        const pageJson = pages[key];
        encodedByPage[key] = encodeHistoryPageFromFixtureJson(pageJson);
        for (const id of chronikTxIdsOnHistoryPage(pageJson)) {
            need.add(id);
        }
    }
    const allTxBodies = chronikEncodeTxStubMap(bundle);
    const txBodies: Record<string, Uint8Array> = {};
    for (const id of need) {
        const body = allTxBodies[id];
        if (body) {
            txBodies[id] = body;
        }
    }
    cy.intercept({ method: 'GET', url: historyUrlRe }, req => {
        const page = new URL(req.url).searchParams.get('page') || '0';
        const hist = encodedByPage[page];
        if (hist === undefined) {
            const keys = Object.keys(encodedByPage).sort(
                (a, b) => Number(a) - Number(b),
            );
            throw new Error(
                `Chronik stub: no historyPages["${page}"] (stub has pages: ${keys.join(', ') || 'none'}); URL ${req.url}`,
            );
        }
        req.reply({
            statusCode: 200,
            headers: CHRONIK_PROTOBUF_HEADERS,
            body: chronikAsProtobufBody(hist),
        });
    });
    chronikRegisterTxStubs(/\/tx\/[0-9a-fA-F]+(\?|$)/, txBodies);
}
