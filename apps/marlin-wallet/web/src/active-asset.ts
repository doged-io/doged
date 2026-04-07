// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { CryptoTicker } from 'ecash-price';
import { XEC_ASSET, type AssetDefinition } from './supported-assets';

type ActiveAsset = { def: AssetDefinition };

let current: ActiveAsset = { def: XEC_ASSET };

export function setActiveAsset(asset: ActiveAsset): void {
    current = asset;
}

export function activeAssetTicker(): string {
    return current.def.ticker;
}

export function activeCryptoTicker(): CryptoTicker {
    return new CryptoTicker(current.def.ticker.toLowerCase());
}

export function activeTokenId(): string | null {
    return current.def.tokenId ?? null;
}

export function activeAssetDefinition(): AssetDefinition {
    return current.def;
}

export function activeAssetDecimals(): number {
    return current.def.decimals;
}

export function activeQuoteCurrency(): CryptoTicker | undefined {
    return current.def.quoteCurrency ?? undefined;
}

export function allowFiatForActiveAsset(): boolean {
    return current.def.quoteCurrency !== undefined;
}
