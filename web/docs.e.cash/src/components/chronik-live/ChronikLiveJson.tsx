// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

'use client';

import { useEffect, useMemo, useState } from 'react';

function stringifyWithBigInt(value: unknown): string {
    return JSON.stringify(
        value,
        (_, v) => {
            if (typeof v === 'bigint') {
                if (v > BigInt(Number.MAX_SAFE_INTEGER)) {
                    return v.toString();
                }
                return Number(v);
            }
            return v;
        },
        2,
    );
}

/** JSON result panel for react-live scope (same role as Docusaurus ReactLiveScope `Json`). */
export function ChronikLiveJson({ fn }: { fn: () => Promise<unknown> }) {
    const [result, setResult] = useState<unknown>(undefined);
    const [error, setError] = useState<unknown>(undefined);

    useEffect(() => {
        fn()
            .then(r => {
                setResult(r);
                setError(undefined);
            })
            .catch(e => {
                setResult(undefined);
                setError(e);
            });
    }, [fn]);

    const text = useMemo(
        () => (result !== undefined ? stringifyWithBigInt(result) : ''),
        [result],
    );

    if (error !== undefined) {
        return (
            <pre className="overflow-x-auto whitespace-pre-wrap rounded border border-red-500/40 bg-red-950/50 p-3 text-xs text-red-200">
                {String(error)}
            </pre>
        );
    }

    if (result === undefined) {
        return (
            <p className="rounded border border-borderLight bg-white/5 p-3 text-sm text-secondaryText">
                Loading…
            </p>
        );
    }

    return (
        <pre className="max-h-96 overflow-auto whitespace-pre-wrap rounded border border-borderLight bg-black/40 p-3 font-mono text-xs text-secondaryText">
            {text}
        </pre>
    );
}
