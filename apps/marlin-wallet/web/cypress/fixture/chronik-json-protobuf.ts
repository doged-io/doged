// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/// <reference types="cypress" />

import * as proto from '../../../../../modules/chronik-client/proto/chronik';
import {
    fromHexRev,
    toHexRev,
} from '../../../../../modules/chronik-client/src/hex';

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
    /** `ScriptUtxos` protobuf-JSON for `GET …/script/p2pkh/<hex>/utxos`. */
    scriptUtxos: unknown;
    /** `BlockchainInfo` protobuf-JSON for `GET /blockchain-info`. */
    blockchainInfo: unknown;
}

/** Context passed to {@link runWithChronik} in `stubs.ts`. */
export type ChronikStubRunContext = {
    stub: ChronikStub;
};

const CHRONIK_STUB_DIR = 'cypress/fixture/chronik';

export const CHRONIK_PROTOBUF_HEADERS = {
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

function encodeScriptUtxosFromFixtureJson(parsed: unknown): Uint8Array {
    const msg = proto.ScriptUtxos.fromJSON(parsed);
    return proto.ScriptUtxos.encode(msg).finish();
}

function encodeBlockchainInfoFromFixtureJson(parsed: unknown): Uint8Array {
    const msg = proto.BlockchainInfo.fromJSON(parsed);
    return proto.BlockchainInfo.encode(msg).finish();
}

/**
 * Txid (hex, reversed byte order) for a `Tx` JSON fixture, lowercase.
 */
export function lowerCaseTxidFromTxJson(tx: unknown): string {
    const msg = proto.Tx.fromJSON(tx);
    return toHexRev(msg.txid).toLowerCase();
}

/**
 * Encode a Chronik WebSocket `WsMsg` with a `tx` payload (same wire format as
 * Chronik). For `TX_FINALIZED`, pass `finalizationReasonType` so the client
 * receives a valid finalization reason (Chronik `MsgTx.finalization_reason`).
 */
export function encodeChronikWsMsgTx(
    txid: string,
    msgType: proto.TxMsgType,
    finalizationReasonType?: proto.TxFinalizationReasonType,
): Uint8Array {
    const txidBytes = fromHexRev(txid.toLowerCase());
    const msgTx: proto.MsgTx = {
        msgType,
        txid: txidBytes,
        finalizationReason: undefined,
    };
    if (finalizationReasonType !== undefined) {
        msgTx.finalizationReason = {
            finalizationType: finalizationReasonType,
        };
    }
    return proto.WsMsg.encode({ tx: msgTx }).finish();
}

/**
 * Stub `GET …/tx/<txid>` with a protobuf `Tx` built from fixture JSON.
 * Register after {@link applyChronikStubIntercepts} so this route wins over the
 * generic `/tx/*` handler for the same id.
 */
export function stubChronikGetTxFromFixtureJson(txJson: unknown): void {
    const body = encodeTxFromFixtureJson(txJson);
    const txid = lowerCaseTxidFromTxJson(txJson);
    cy.intercept(
        { method: 'GET', url: new RegExp(`/tx/${txid}(\\?|$)`, 'i') },
        req => {
            req.reply({
                statusCode: 200,
                headers: CHRONIK_PROTOBUF_HEADERS,
                body: chronikAsProtobufBody(body),
            });
        },
    );
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

export function chronikAsProtobufBody(
    data: Cypress.Buffer | Uint8Array,
): ArrayBuffer {
    const u8 =
        data instanceof Uint8Array
            ? data
            : new Uint8Array(data as unknown as Iterable<number>);
    return u8.buffer.slice(u8.byteOffset, u8.byteOffset + u8.byteLength);
}

/**
 * Normalize a Cypress intercept `req.body` (binary protobuf POST) to
 * `Uint8Array`.
 */
export function cypressRequestBodyToUint8Array(body: unknown): Uint8Array {
    if (body instanceof Uint8Array) {
        return body;
    }
    if (body instanceof ArrayBuffer) {
        return new Uint8Array(body);
    }
    return new Uint8Array(body as unknown as Iterable<number>);
}

/** 32 random bytes as protobuf-JSON base64 for a `BroadcastTxsResponse` txid. */
export function randomTxidBase64ForProtobufJson(): string {
    const bytes = new Uint8Array(32);
    crypto.getRandomValues(bytes);
    let binary = '';
    for (let i = 0; i < bytes.length; i++) {
        binary += String.fromCharCode(bytes[i]!);
    }
    return btoa(binary);
}

function chronikHistoryUrlRegex(stub: ChronikStub): RegExp {
    const hex = stub.meta.p2pkhPayloadHex.toLowerCase();
    return new RegExp(`/script/p2pkh/${hex}/history(\\?|$)`);
}

function chronikScriptUtxosUrlRegex(stub: ChronikStub): RegExp {
    const hex = stub.meta.p2pkhPayloadHex.toLowerCase();
    return new RegExp(`/script/p2pkh/${hex}/utxos(\\?|$)`);
}

function chronikEncodeTxStubMap(stub: ChronikStub): Record<string, Uint8Array> {
    const out: Record<string, Uint8Array> = {};
    for (const [id, txJson] of Object.entries(stub.txs)) {
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
export function applyChronikStubIntercepts(stub: ChronikStub): void {
    const historyUrlRe = chronikHistoryUrlRegex(stub);

    const pages = stub.historyPages;
    if (pages['0'] === undefined) {
        throw new Error('Chronik stub missing historyPages["0"]');
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
    const allTxBodies = chronikEncodeTxStubMap(stub);
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

    const scriptUtxosBody = encodeScriptUtxosFromFixtureJson(stub.scriptUtxos);
    cy.intercept(
        { method: 'GET', url: chronikScriptUtxosUrlRegex(stub) },
        req => {
            req.reply({
                statusCode: 200,
                headers: CHRONIK_PROTOBUF_HEADERS,
                body: chronikAsProtobufBody(scriptUtxosBody),
            });
        },
    );

    const blockchainInfoBody = encodeBlockchainInfoFromFixtureJson(
        stub.blockchainInfo,
    );
    cy.intercept({ method: 'GET', url: /\/blockchain-info(\?|$)/ }, req => {
        req.reply({
            statusCode: 200,
            headers: CHRONIK_PROTOBUF_HEADERS,
            body: chronikAsProtobufBody(blockchainInfoBody),
        });
    });
}
