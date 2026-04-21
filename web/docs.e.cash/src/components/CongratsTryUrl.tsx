// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

'use client';

import { ChronikClient } from 'chronik-client';
import { useCallback, useState } from 'react';

export function CongratsTryUrl() {
    const [chronikUrl, setChronikUrl] = useState('https://chronik.e.cash');
    const [blockchainInfo, setBlockchainInfo] = useState<{
        tipHash: string;
        tipHeight: number;
    } | null>(null);
    const [error, setError] = useState<string | undefined>(undefined);

    const getBlockchainInfo = useCallback(async () => {
        const client = new ChronikClient([chronikUrl]);
        try {
            const info = await client.blockchainInfo();
            setBlockchainInfo(info);
            setError(undefined);
        } catch (ex) {
            setError(String(ex));
            setBlockchainInfo(null);
        }
    }, [chronikUrl]);

    return (
        <div className="not-prose my-6 rounded-xl border border-borderLight bg-white/5 p-4 shadow-[inset_0_1px_0_0_rgba(255,255,255,0.06)]">
            <label className="mb-2 block text-sm font-medium text-primaryText">
                Chronik URL
            </label>
            <input
                className="mb-3 w-full rounded-md border border-borderLight bg-background px-3 py-2 font-mono text-sm text-primaryText placeholder:text-secondaryText"
                value={chronikUrl}
                onChange={e => setChronikUrl(e.target.value)}
            />
            <button
                type="button"
                className="rounded-md bg-accentMedium px-4 py-2 text-sm font-medium text-primaryText transition-colors hover:bg-accentLight"
                onClick={() => void getBlockchainInfo()}
            >
                Try it out!
            </button>
            <div className="mt-4 space-y-2 text-sm text-secondaryText">
                {error && (
                    <p className="text-red-300">
                        <strong className="text-primaryText">Error:</strong>{' '}
                        {error}
                    </p>
                )}
                {blockchainInfo && (
                    <>
                        <p>
                            <strong className="text-primaryText">
                                Tip block hash:
                            </strong>{' '}
                            <span className="font-mono text-accentLight">
                                {blockchainInfo.tipHash}
                            </span>
                        </p>
                        <p>
                            <strong className="text-primaryText">
                                Tip block height:
                            </strong>{' '}
                            <span>{blockchainInfo.tipHeight}</span>
                        </p>
                    </>
                )}
            </div>
        </div>
    );
}
