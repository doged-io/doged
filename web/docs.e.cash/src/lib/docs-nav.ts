// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/** Paths are relative to the deployed app root (Next adds `basePath`). */
export const docsNav: {
    title: string;
    links: { label: string; href: string }[];
}[] = [
    {
        title: 'Chronik',
        links: [{ label: 'Overview', href: '/' }],
    },
    {
        title: 'Run Chronik',
        links: [
            { label: 'Download & install', href: '/chronik-setup/download/' },
            { label: 'Setup Chronik', href: '/chronik-setup/setup-chronik/' },
            { label: 'Host Chronik', href: '/chronik-setup/host-chronik/' },
            {
                label: 'Congratulations',
                href: '/chronik-setup/congratulations/',
            },
            { label: 'Tune Chronik', href: '/chronik-setup/tune-chronik/' },
            { label: 'Build Chronik', href: '/chronik-setup/build-chronik/' },
        ],
    },
    {
        title: 'JavaScript / TypeScript',
        links: [
            { label: 'Install', href: '/chronik-js/install/' },
            { label: 'Connect', href: '/chronik-js/connect/' },
            { label: 'Blockchain', href: '/chronik-js/blockchain/' },
            { label: 'Blocks', href: '/chronik-js/blocks/' },
            { label: 'Transactions', href: '/chronik-js/txs/' },
            { label: 'Scripts & addresses', href: '/chronik-js/addresses/' },
            { label: 'Tokens', href: '/chronik-js/tokens/' },
            {
                label: 'Broadcast transactions',
                href: '/chronik-js/broadcast-tx/',
            },
            { label: 'WebSocket', href: '/chronik-js/websocket/' },
        ],
    },
    {
        title: 'Reference',
        links: [
            {
                label: 'chronik-client (Typedoc)',
                href: '/reference/chronik-client/',
            },
        ],
    },
];
