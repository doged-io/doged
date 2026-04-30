// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

'use client';

import { useCallback, useEffect, useRef, useState } from 'react';

type Props = {
    title: string;
    description: string;
    url: string;
};

function ClipboardIcon({ className }: { className?: string }) {
    return (
        <svg
            className={className}
            width="20"
            height="20"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
            aria-hidden
        >
            <rect x="9" y="9" width="13" height="13" rx="2" ry="2" />
            <path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1" />
        </svg>
    );
}

function ExternalLinkIcon({ className }: { className?: string }) {
    return (
        <svg
            className={className}
            width="20"
            height="20"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
            aria-hidden
        >
            <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6" />
            <polyline points="15 3 21 3 21 9" />
            <line x1="10" y1="14" x2="21" y2="3" />
        </svg>
    );
}

/**
 * Copy + open-in-new-tab actions for a sample pay.e.cash URL (used from MDX).
 */
export function PaySampleUrlRow({ title, description, url }: Props) {
    const [copied, setCopied] = useState(false);
    const copiedTimeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);

    useEffect(() => {
        return () => {
            if (copiedTimeoutRef.current !== null) {
                clearTimeout(copiedTimeoutRef.current);
            }
        };
    }, []);

    const copyUrl = useCallback(async () => {
        try {
            await navigator.clipboard.writeText(url);
            if (copiedTimeoutRef.current !== null) {
                clearTimeout(copiedTimeoutRef.current);
            }
            setCopied(true);
            copiedTimeoutRef.current = setTimeout(() => {
                setCopied(false);
                copiedTimeoutRef.current = null;
            }, 1000);
        } catch {
            setCopied(false);
        }
    }, [url]);

    return (
        <div className="not-prose my-4 flex flex-col gap-3 rounded-lg border border-borderLight bg-white/5 px-4 py-3 sm:flex-row sm:items-center sm:justify-between">
            <div className="min-w-0 space-y-1">
                <div className="text-sm font-semibold text-primaryText">
                    {title}
                </div>
                <div className="text-sm text-secondaryText">{description}</div>
            </div>
            <div className="flex shrink-0 items-center gap-2">
                <div className="relative">
                    {copied ? (
                        <span
                            role="status"
                            className="pointer-events-none absolute bottom-full left-1/2 z-10 mb-1 -translate-x-1/2 whitespace-nowrap rounded-md border border-borderLight bg-background px-2 py-1 text-xs font-medium text-accentLight shadow-lg"
                        >
                            Copied!
                        </span>
                    ) : null}
                    <button
                        type="button"
                        onClick={() => void copyUrl()}
                        className="inline-flex h-10 w-10 items-center justify-center rounded-md border border-borderLight bg-background text-secondaryText transition-colors hover:border-accentMedium hover:text-accentLight"
                        aria-label={`Copy payment URL: ${title}`}
                    >
                        <ClipboardIcon />
                    </button>
                </div>
                <a
                    href={url}
                    target="_blank"
                    rel="noopener noreferrer"
                    className="inline-flex h-10 w-10 items-center justify-center rounded-md border border-borderLight bg-background text-secondaryText transition-colors hover:border-accentMedium hover:text-accentLight"
                    aria-label={`Open payment URL in new tab: ${title}`}
                >
                    <ExternalLinkIcon />
                </a>
            </div>
        </div>
    );
}
