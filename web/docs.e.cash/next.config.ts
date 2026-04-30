// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import type { NextConfig } from 'next';

const nextConfig: NextConfig = {
    reactStrictMode: true,
    // Chronik docs and related MDX live under this prefix; add more base paths later.
    basePath: '/chronik',
    trailingSlash: true,
    async redirects() {
        return [
            {
                source: '/',
                destination: '/chronik/',
                permanent: false,
                basePath: false,
            },
            {
                source: '/pay',
                destination: '/chronik/pay/',
                permanent: false,
                basePath: false,
            },
            {
                source: '/pay/',
                destination: '/chronik/pay/',
                permanent: false,
                basePath: false,
            },
        ];
    },
};

export default nextConfig;
