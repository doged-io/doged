// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import Link from 'next/link';

export function DocsHome() {
    return (
        <article className="prose mx-auto max-w-3xl">
            <h1>Documentation hub</h1>
            <p>
                The official reference developer libraries on eCash: Chronik,
                JavaScript APIs, and related topics.
            </p>
            <h2>pay.e.cash</h2>
            <ul>
                <li>
                    <Link href="/pay/">Universal payments</Link> —{' '}
                    <code>pay.e.cash</code> links, BIP21 payloads, and how to
                    use them in apps.
                </li>
            </ul>
            <h2>Chronik</h2>
            <h3>Overview</h3>
            <ul>
                <li>
                    <Link href="/chronik/">Chronik documentation</Link> —
                    indexer built into Bitcoin ABC, what it does, and where to
                    go next.
                </li>
            </ul>
            <h3>Run Chronik</h3>
            <ul>
                <li>
                    <Link href="/chronik/setup/download/">
                        Download &amp; install Chronik
                    </Link>{' '}
                    for your platform, then follow the setup guides in the
                    sidebar.
                </li>
            </ul>
            <h3>JavaScript &amp; TypeScript</h3>
            <ul>
                <li>
                    <Link href="/chronik/chronik-client/install/">
                        chronik-client guides
                    </Link>{' '}
                    — install, connect, blocks, txs, tokens, WebSocket, and
                    more.
                </li>
                <li>
                    <Link href="/reference/chronik-client/">
                        chronik-client API (Typedoc)
                    </Link>{' '}
                    — generated reference for the npm package.
                </li>
            </ul>
        </article>
    );
}
