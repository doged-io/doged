// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import Link from 'next/link';

export default function NotFound() {
    return (
        <div className="text-center">
            <h1 className="text-2xl font-semibold text-primaryText">
                Page not found
            </h1>
            <p className="mt-2 text-secondaryText">
                <Link
                    href="/"
                    className="text-accentMedium underline decoration-transparent transition-colors hover:text-accentLight hover:decoration-accentLight"
                >
                    Back to home
                </Link>
            </p>
        </div>
    );
}
