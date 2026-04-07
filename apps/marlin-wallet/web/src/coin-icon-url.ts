// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

type CoinIconContext = ((id: string) => string) & {
    keys(): string[];
};

const coinIconContext = (
    require as unknown as {
        context: (
            directory: string,
            useSubdirectories: boolean,
            regExp: RegExp,
        ) => CoinIconContext;
    }
).context('./assets/coins', false, /\.png$/) as CoinIconContext;

/**
 * Webpack-resolved URL for `assets/coins/{assetKey}.png` when that file exists.
 */
export function coinIconUrlForAssetKey(assetKey: string): string | undefined {
    const id = `./${assetKey}.png`;
    return coinIconContext.keys().includes(id)
        ? coinIconContext(id)
        : undefined;
}
