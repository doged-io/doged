// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { DocsChrome } from '@/components/DocsChrome';
import type { Metadata } from 'next';
import { Fira_Code, Space_Grotesk } from 'next/font/google';
import './globals.css';

const spaceGrotesk = Space_Grotesk({
    subsets: ['latin'],
    variable: '--font-space-grotesk',
    weight: ['400', '500', '600', '700'],
});

const firaCode = Fira_Code({
    subsets: ['latin'],
    variable: '--font-fira-code',
    weight: ['400', '500', '600'],
});

export const metadata: Metadata = {
    title: {
        default: 'Bitcoin ABC Documentation',
        template: '%s · Bitcoin ABC Documentation',
    },
    description:
        'Documentation for Bitcoin ABC: Chronik, APIs, and the broader monorepo.',
};

export default function RootLayout({
    children,
}: Readonly<{
    children: React.ReactNode;
}>) {
    return (
        <html lang="en">
            <body
                className={`${spaceGrotesk.variable} ${firaCode.variable} font-sans antialiased`}
            >
                <DocsChrome>{children}</DocsChrome>
            </body>
        </html>
    );
}
