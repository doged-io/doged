// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import versionFile from '@/data/bitcoin-abc-version.json';
import Link from 'next/link';

const downloadUrl = 'https://download.bitcoinabc.org';

function buildFiles(version: string) {
    return {
        linux: {
            x86_64: `bitcoin-abc-${version}-x86_64-linux-gnu.tar.gz`,
            aarch64: `bitcoin-abc-${version}-aarch64-linux-gnu.tar.gz`,
        },
        windows: {
            win64: `bitcoin-abc-${version}-win64-setup-unsigned.exe`,
        },
        macos: {
            x86_64: `bitcoin-abc-${version}-x86_64-apple-darwin.tar.gz`,
        },
    };
}

export function DownloadLinks() {
    const version =
        process.env.NEXT_PUBLIC_BITCOIN_ABC_VERSION ?? versionFile.version;
    const files = buildFiles(version);
    const downloadUrls = {
        linux: {
            x86_64: `${downloadUrl}/${version}/linux/${files.linux.x86_64}`,
            aarch64: `${downloadUrl}/${version}/linux/${files.linux.aarch64}`,
        },
        windows: {
            win64: `${downloadUrl}/${version}/win/${files.windows.win64}`,
        },
        macos: {
            x86_64: `${downloadUrl}/${version}/osx/${files.macos.x86_64}`,
        },
    };

    return (
        <div className="not-prose space-y-8">
            <p className="rounded-lg border border-amber-500/35 bg-amber-500/10 p-4 text-sm text-secondaryText">
                <strong className="text-primaryText">Heads up:</strong> this
                tutorial is about running{' '}
                <code className="text-accentLight">bitcoind</code> with Chronik
                enabled. That needs a capable machine and can take days to sync.
                For app development you can use the public indexer — see{' '}
                <Link
                    href="/chronik-js/install/"
                    className="font-medium text-accentMedium underline decoration-transparent transition-colors hover:text-accentLight hover:decoration-accentLight"
                >
                    chronik-client
                </Link>
                .
            </p>

            <p className="text-sm text-secondaryText">
                Showing Bitcoin ABC{' '}
                <strong className="text-primaryText">{version}</strong>{' '}
                (override with{' '}
                <code className="font-mono text-accentLight">
                    NEXT_PUBLIC_BITCOIN_ABC_VERSION
                </code>
                ).
            </p>

            <section>
                <h2 className="text-lg font-semibold text-primaryText">
                    Linux
                </h2>
                <h3 className="mt-4 font-medium text-primaryText">x86_64</h3>
                <p>
                    <a
                        className="text-accentMedium underline decoration-transparent transition-colors hover:text-accentLight hover:decoration-accentLight"
                        href={downloadUrls.linux.x86_64}
                    >
                        Download Bitcoin ABC Chronik for Linux x86_64
                    </a>
                </p>
                <pre className="mt-2 overflow-x-auto rounded-lg border border-borderLight bg-black/40 p-4 font-mono text-sm text-secondaryText">
                    {`curl -O ${downloadUrls.linux.x86_64}
tar xf ${files.linux.x86_64}
rm ${files.linux.x86_64}`}
                </pre>

                <h3 className="mt-6 font-medium text-primaryText">aarch64</h3>
                <p>
                    <a
                        className="text-accentMedium underline decoration-transparent transition-colors hover:text-accentLight hover:decoration-accentLight"
                        href={downloadUrls.linux.aarch64}
                    >
                        Download Bitcoin ABC Chronik for Linux aarch64
                    </a>
                </p>
                <pre className="mt-2 overflow-x-auto rounded-lg border border-borderLight bg-black/40 p-4 font-mono text-sm text-secondaryText">
                    {`curl -O ${downloadUrls.linux.aarch64}
tar xf ${files.linux.aarch64}
rm ${files.linux.aarch64}`}
                </pre>
            </section>

            <section>
                <h2 className="text-lg font-semibold text-primaryText">
                    Windows
                </h2>
                <p>
                    <a
                        className="text-accentMedium underline decoration-transparent transition-colors hover:text-accentLight hover:decoration-accentLight"
                        href={downloadUrls.windows.win64}
                    >
                        Download Bitcoin ABC Chronik for Windows
                    </a>
                </p>
                <pre className="mt-2 overflow-x-auto rounded-lg border border-borderLight bg-black/40 p-4 font-mono text-sm text-secondaryText">
                    {`curl -O ${downloadUrls.windows.win64}`}
                </pre>
            </section>

            <section>
                <h2 className="text-lg font-semibold text-primaryText">
                    macOS
                </h2>
                <p>
                    <a
                        className="text-accentMedium underline decoration-transparent transition-colors hover:text-accentLight hover:decoration-accentLight"
                        href={downloadUrls.macos.x86_64}
                    >
                        Download Bitcoin ABC Chronik for macOS x86_64
                    </a>
                </p>
                <pre className="mt-2 overflow-x-auto rounded-lg border border-borderLight bg-black/40 p-4 font-mono text-sm text-secondaryText">
                    {`curl -O ${downloadUrls.macos.x86_64}
tar xf ${files.macos.x86_64}
rm ${files.macos.x86_64}`}
                </pre>
            </section>
        </div>
    );
}
