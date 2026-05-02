// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/** Paths are relative to the deployed site root (`docs.e.cash`). */
export const docsNav: {
    title: string;
    links: { label: string; href: string }[];
}[] = [
    {
        title: 'Chronik',
        links: [{ label: 'Overview', href: '/chronik/' }],
    },
    {
        title: 'Payments',
        links: [{ label: 'pay.e.cash', href: '/pay/' }],
    },
    {
        title: 'Run Chronik',
        links: [
            { label: 'Download & install', href: '/chronik/setup/download/' },
            { label: 'Setup Chronik', href: '/chronik/setup/setup-chronik/' },
            { label: 'Host Chronik', href: '/chronik/setup/host-chronik/' },
            {
                label: 'Congratulations',
                href: '/chronik/setup/congratulations/',
            },
            { label: 'Tune Chronik', href: '/chronik/setup/tune-chronik/' },
            { label: 'Build Chronik', href: '/chronik/setup/build-chronik/' },
        ],
    },
    {
        title: 'JavaScript / TypeScript',
        links: [
            { label: 'Install', href: '/chronik/chronik-client/install/' },
            { label: 'Connect', href: '/chronik/chronik-client/connect/' },
            {
                label: 'Blockchain',
                href: '/chronik/chronik-client/blockchain/',
            },
            { label: 'Blocks', href: '/chronik/chronik-client/blocks/' },
            { label: 'Transactions', href: '/chronik/chronik-client/txs/' },
            {
                label: 'Scripts & addresses',
                href: '/chronik/chronik-client/addresses/',
            },
            { label: 'Tokens', href: '/chronik/chronik-client/tokens/' },
            {
                label: 'Broadcast transactions',
                href: '/chronik/chronik-client/broadcast-tx/',
            },
            { label: 'WebSocket', href: '/chronik/chronik-client/websocket/' },
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
