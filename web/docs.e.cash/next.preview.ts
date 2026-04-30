// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Preview build (TeamCity): copied over next.config.ts by docs.e.cash.Dockerfile.
// Keep aligned with next.config.ts; extend here if preview-only settings are needed.

import type { NextConfig } from 'next';

const nextConfig: NextConfig = {
    reactStrictMode: true,
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
