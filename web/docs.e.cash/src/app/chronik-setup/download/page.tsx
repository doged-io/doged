// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { DownloadLinks } from '@/components/DownloadLinks';
import type { Metadata } from 'next';

export const metadata: Metadata = {
    title: 'Download & install',
};

export default function DownloadPage() {
    return (
        <article className="prose mx-auto max-w-3xl">
            <h1>Download &amp; install Chronik</h1>
            <p>
                Download Chronik for the right platform. The Bitcoin ABC version
                shown below is read from the monorepo root{' '}
                <code className="font-mono text-accentLight">
                    CMakeLists.txt
                </code>{' '}
                when you run{' '}
                <code className="font-mono text-accentLight">npm run dev</code>{' '}
                or{' '}
                <code className="font-mono text-accentLight">
                    npm run build
                </code>{' '}
                (see{' '}
                <code className="font-mono text-accentLight">
                    scripts/sync-abc-version.mjs
                </code>
                ). Override with{' '}
                <code className="font-mono text-accentLight">
                    NEXT_PUBLIC_BITCOIN_ABC_VERSION
                </code>{' '}
                if needed.
            </p>
            <DownloadLinks />
        </article>
    );
}
