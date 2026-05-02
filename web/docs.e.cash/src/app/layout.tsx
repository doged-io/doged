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

const siteTitle = 'eCash Developer Documentation';

const siteDescription =
    'The official eCash Developer Documentation hub. Documentation for libraries and services published by BitcoinABC, the developer team behind eCash (XEC).';

const socialCardPath = '/social.jpg';

export const metadata: Metadata = {
    metadataBase: new URL('https://docs.e.cash'),
    title: {
        default: siteTitle,
        template: '%s · eCash docs',
    },
    description: siteDescription,
    openGraph: {
        title: siteTitle,
        description: siteDescription,
        siteName: siteTitle,
        locale: 'en_US',
        type: 'website',
        images: [
            {
                url: socialCardPath,
                width: 1200,
                height: 630,
                alt: siteTitle,
            },
        ],
    },
    twitter: {
        card: 'summary_large_image',
        title: siteTitle,
        description: siteDescription,
        images: [socialCardPath],
    },
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
