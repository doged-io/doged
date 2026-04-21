// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { docsNav } from '@/lib/docs-nav';
import Link from 'next/link';

type Props = {
    /** Called after navigating (e.g. close mobile drawer). */
    onNavigate?: () => void;
};

export function SidebarNav({ onNavigate }: Props) {
    return (
        <nav className="h-full space-y-6 overflow-y-auto p-4">
            {docsNav.map(section => (
                <div key={section.title}>
                    <div className="mb-2 text-xs font-semibold tracking-wide text-secondaryText uppercase">
                        {section.title}
                    </div>
                    <ul className="space-y-1">
                        {section.links.map(link => (
                            <li key={link.href}>
                                <Link
                                    href={link.href}
                                    onClick={onNavigate}
                                    className="block rounded-md px-2 py-1.5 text-sm font-medium text-secondaryText transition-colors hover:bg-white/5 hover:text-accentLight"
                                >
                                    {link.label}
                                </Link>
                            </li>
                        ))}
                    </ul>
                </div>
            ))}
        </nav>
    );
}
