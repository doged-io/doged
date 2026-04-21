// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import type { MDXComponents } from 'mdx/types';
import Link from 'next/link';
import { CongratsTryUrl } from '@/components/CongratsTryUrl';
import { ChronikLivePlayground } from '@/components/chronik-live/ChronikLivePlayground';

export function getMdxComponents(): MDXComponents {
    return {
        CongratsTryUrl,
        ChronikLivePlayground,
        a: ({ href, children, ...props }) => {
            const h = href ?? '';
            if (
                !h ||
                h.startsWith('#') ||
                /^https?:\/\//.test(h) ||
                h.startsWith('mailto:')
            ) {
                return (
                    <a href={href} {...props}>
                        {children}
                    </a>
                );
            }
            if (h.startsWith('/')) {
                return (
                    <Link href={h} {...props}>
                        {children}
                    </Link>
                );
            }
            return (
                <a href={href} {...props}>
                    {children}
                </a>
            );
        },
    };
}
