// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

'use client';

import { SidebarNav } from '@/components/SidebarNav';
import Link from 'next/link';
import { useEffect, useState } from 'react';

function HamburgerIcon({ open }: { open: boolean }) {
    return (
        <svg
            className="h-6 w-6 text-primaryText"
            fill="none"
            stroke="currentColor"
            viewBox="0 0 24 24"
            aria-hidden
        >
            {open ? (
                <path
                    strokeLinecap="round"
                    strokeLinejoin="round"
                    strokeWidth={2}
                    d="M6 18L18 6M6 6l12 12"
                />
            ) : (
                <path
                    strokeLinecap="round"
                    strokeLinejoin="round"
                    strokeWidth={2}
                    d="M4 6h16M4 12h16M4 18h16"
                />
            )}
        </svg>
    );
}

export function DocsChrome({ children }: { children: React.ReactNode }) {
    const [mobileNavOpen, setMobileNavOpen] = useState(false);

    useEffect(() => {
        if (!mobileNavOpen) {
            return;
        }
        const onKey = (e: KeyboardEvent) => {
            if (e.key === 'Escape') {
                setMobileNavOpen(false);
            }
        };
        window.addEventListener('keydown', onKey);
        return () => window.removeEventListener('keydown', onKey);
    }, [mobileNavOpen]);

    useEffect(() => {
        if (mobileNavOpen) {
            document.body.style.overflow = 'hidden';
        } else {
            document.body.style.overflow = '';
        }
        return () => {
            document.body.style.overflow = '';
        };
    }, [mobileNavOpen]);

    return (
        <div className="flex min-h-screen flex-col bg-background lg:flex-row">
            {/* Mobile overlay */}
            {mobileNavOpen ? (
                <button
                    type="button"
                    className="fixed inset-0 z-40 bg-black/60 lg:hidden"
                    aria-label="Close menu"
                    onClick={() => setMobileNavOpen(false)}
                />
            ) : null}

            {/* Sidebar: drawer on small screens, static on lg */}
            <aside
                className={`fixed inset-y-0 left-0 z-50 w-[min(18rem,85vw)] border-r border-borderLight bg-background transition-transform duration-200 ease-out lg:static lg:z-auto lg:flex lg:w-64 lg:max-w-none lg:translate-x-0 lg:border-b-0 ${
                    mobileNavOpen
                        ? 'translate-x-0'
                        : '-translate-x-full lg:translate-x-0'
                }`}
            >
                <div className="flex h-14 shrink-0 items-center justify-between border-b border-borderLight px-4 lg:hidden">
                    <span className="text-sm font-semibold text-primaryText">
                        Menu
                    </span>
                    <button
                        type="button"
                        className="rounded-md p-1.5 hover:bg-white/10"
                        onClick={() => setMobileNavOpen(false)}
                        aria-label="Close menu"
                    >
                        <HamburgerIcon open />
                    </button>
                </div>
                <SidebarNav onNavigate={() => setMobileNavOpen(false)} />
            </aside>

            <div className="flex min-w-0 flex-1 flex-col">
                <header className="flex items-center gap-3 border-b border-borderLight px-4 py-3 lg:px-10">
                    <button
                        type="button"
                        className="rounded-md p-1.5 hover:bg-white/10 lg:hidden"
                        onClick={() => setMobileNavOpen(true)}
                        aria-label="Open menu"
                        aria-expanded={mobileNavOpen}
                    >
                        <HamburgerIcon open={false} />
                    </button>
                    <Link
                        href="/"
                        className="inline-block text-lg font-semibold tracking-tight text-primaryText"
                        onClick={() => setMobileNavOpen(false)}
                    >
                        <span className="docs-gradient-text">eCash</span>{' '}
                        <span className="text-secondaryText">docs</span>
                    </Link>
                </header>
                <main className="w-full min-w-0 overflow-x-hidden px-4 py-10 sm:px-6 lg:px-10">
                    {children}
                </main>
                <footer className="border-t border-borderLight px-4 py-6 text-center text-sm text-secondaryText sm:px-6 lg:px-10">
                    <a
                        href="https://github.com/Bitcoin-ABC/bitcoin-abc"
                        className="text-accentMedium underline decoration-transparent transition-colors hover:text-accentLight hover:decoration-accentLight"
                    >
                        Bitcoin ABC on GitHub
                    </a>
                </footer>
            </div>
        </div>
    );
}
