// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Build a single JSON stub file for Marlin Cypress (e.g.
 * `<payload-without-ecash-prefix>.json`):
 * - Fetches `script/p2pkh/<hash>/history` for pages 0..N-1
 * - Fetches each `tx/<explorerTxid>` referenced on those pages
 * - Fetches `script/p2pkh/<hash>/utxos` (full `ScriptUtxos`, including token UTXOs)
 * - Fetches `blockchain-info` (`BlockchainInfo` for wallet sync / maturity)
 *
 * Usage (from apps/marlin-wallet/web):
 *   pnpm exec tsx scripts/export-chronik-stub.ts <out.json> <ecash:...> \\
 *     [--chronik https://chronik.e.cash] \\
 *     [--max-pages 2] [--page-size 25]
 */

import fs from 'fs';
import path from 'path';
import { Address } from '../../../../modules/ecash-lib';
import * as proto from '../../../../modules/chronik-client/proto/chronik';
import { toHexRev } from '../../../../modules/chronik-client/src/hex';
import type { ChronikStub } from '../cypress/fixture/chronik-json-protobuf';

const DEFAULT_CHRONIK_URL = 'https://chronik.e.cash';
const DEFAULT_MAX_PAGES = 2;
const DEFAULT_PAGE_SIZE = 25;

function printUsage(): void {
    console.error(`Usage:
  pnpm exec tsx scripts/export-chronik-stub.ts <out.json> <ecash:...> [options]

Options:
  --chronik <url>   Chronik URL (default: ${DEFAULT_CHRONIK_URL})
  --max-pages <n>   default: ${DEFAULT_MAX_PAGES}
  --page-size <n>   default: ${DEFAULT_PAGE_SIZE}`);
}

function parseArgs(): {
    out: string;
    address: string;
    chronikUrl: string;
    maxPages: number;
    pageSize: number;
} {
    const argv = process.argv.slice(2);
    if (argv.length < 2) {
        printUsage();
        process.exit(1);
    }
    const out = argv[0];
    if (out.startsWith('-')) {
        console.error('First argument must be the output JSON path.');
        process.exit(1);
    }
    const address = argv[1];
    if (address.startsWith('-')) {
        console.error('Second argument must be the ecash: address.');
        process.exit(1);
    }

    const options = argv.splice(2);

    let chronikUrl = DEFAULT_CHRONIK_URL;
    let maxPages = DEFAULT_MAX_PAGES;
    let pageSize = DEFAULT_PAGE_SIZE;

    for (let i = 0; i < options.length; i++) {
        const option = options[i];
        if (option === '--chronik') {
            chronikUrl = options[++i] ?? chronikUrl;
            // Remove trailing slash if present
            chronikUrl = chronikUrl.replace(/\/$/, '');
        } else if (option === '--max-pages') {
            maxPages = Number(options[++i]);
        } else if (option === '--page-size') {
            pageSize = Number(options[++i]);
        } else {
            console.error(`Unknown argument: ${option}`);
            printUsage();
            process.exit(1);
        }
    }
    return {
        out,
        address,
        chronikUrl,
        maxPages,
        pageSize,
    };
}

function explorerTxidLower(tx: proto.Tx): string {
    return toHexRev(tx.txid).toLowerCase();
}

/*
 * We can't use chronik-client here because we want to mock the protobuf
 * response from chronik in the e2e tests. In order to keep the stub files human
 * readable, it's transformed to/from json but it's still not exactly the same
 * format used by chronik-client (no decoding/encoding).
 */
async function fetchHistoryPage(
    chronikUrl: string,
    p2pkhHex: string,
    page: number,
    pageSize: number,
): Promise<proto.TxHistoryPage> {
    const url = `${chronikUrl}/script/p2pkh/${p2pkhHex}/history?page=${page}&page_size=${pageSize}`;
    const res = await fetch(url);
    if (!res.ok) {
        throw new Error(`GET ${url} -> ${res.status} ${res.statusText}`);
    }
    const buf = new Uint8Array(await res.arrayBuffer());
    return proto.TxHistoryPage.decode(buf);
}

async function fetchTx(chronikUrl: string, txidHex: string): Promise<proto.Tx> {
    const url = `${chronikUrl}/tx/${txidHex}`;
    const res = await fetch(url);
    if (!res.ok) {
        throw new Error(`GET ${url} -> ${res.status} ${res.statusText}`);
    }
    const buf = new Uint8Array(await res.arrayBuffer());
    return proto.Tx.decode(buf);
}

async function fetchScriptUtxos(
    chronikUrl: string,
    p2pkhHex: string,
): Promise<proto.ScriptUtxos> {
    const url = `${chronikUrl}/script/p2pkh/${p2pkhHex}/utxos`;
    const res = await fetch(url);
    if (!res.ok) {
        throw new Error(`GET ${url} -> ${res.status} ${res.statusText}`);
    }
    const buf = new Uint8Array(await res.arrayBuffer());
    return proto.ScriptUtxos.decode(buf);
}

async function fetchBlockchainInfo(
    chronikUrl: string,
): Promise<proto.BlockchainInfo> {
    const url = `${chronikUrl}/blockchain-info`;
    const res = await fetch(url);
    if (!res.ok) {
        throw new Error(`GET ${url} -> ${res.status} ${res.statusText}`);
    }
    const buf = new Uint8Array(await res.arrayBuffer());
    return proto.BlockchainInfo.decode(buf);
}

async function exportFromAddress(args: {
    address: string;
    chronikUrl: string;
    maxPages: number;
    pageSize: number;
    out: string;
}): Promise<void> {
    const parsed = Address.parse(args.address);
    if (parsed.type !== 'p2pkh') {
        throw new Error(`Expected p2pkh address, got ${parsed.type}`);
    }
    const p2pkhPayloadHex = parsed.hash.toLowerCase();

    const historyPages: Record<string, unknown> = {};
    const txIdSet = new Set<string>();
    let historyNumPages = 0;
    let historyNumTxs = 0;

    for (let p = 0; p < args.maxPages; p++) {
        const page = await fetchHistoryPage(
            args.chronikUrl,
            p2pkhPayloadHex,
            p,
            args.pageSize,
        );
        historyPages[String(p)] = proto.TxHistoryPage.toJSON(page);
        if (p === 0) {
            historyNumPages = page.numPages;
            historyNumTxs = page.numTxs;
        }
        for (const tx of page.txs) {
            txIdSet.add(explorerTxidLower(tx));
        }
        if (page.numPages > 0 && p >= page.numPages - 1) {
            break;
        }
    }

    const txs: Record<string, unknown> = {};
    for (const id of txIdSet) {
        const tx = await fetchTx(args.chronikUrl, id);
        txs[id] = proto.Tx.toJSON(tx);
    }

    const scriptUtxosMsg = await fetchScriptUtxos(
        args.chronikUrl,
        p2pkhPayloadHex,
    );
    const blockchainInfoMsg = await fetchBlockchainInfo(args.chronikUrl);

    const stub: ChronikStub = {
        meta: {
            address: args.address,
            p2pkhPayloadHex,
            exportedAt: new Date().toISOString(),
            pageSize: args.pageSize,
            historyNumPages,
            historyNumTxs,
        },
        historyPages,
        txs,
        scriptUtxos: proto.ScriptUtxos.toJSON(scriptUtxosMsg),
        blockchainInfo: proto.BlockchainInfo.toJSON(blockchainInfoMsg),
    };

    fs.mkdirSync(path.dirname(args.out), { recursive: true });
    fs.writeFileSync(args.out, `${JSON.stringify(stub, null, 2)}\n`);
    console.log('wrote', args.out);
}

async function main(): Promise<void> {
    const args = parseArgs();
    await exportFromAddress({
        address: args.address,
        chronikUrl: args.chronikUrl,
        maxPages: args.maxPages,
        pageSize: args.pageSize,
        out: args.out,
    });
}

main().catch(err => {
    console.error(err);
    process.exit(1);
});
