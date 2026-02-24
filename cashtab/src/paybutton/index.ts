// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * PayButton deep link detection and conversion to BIP21 URI
 *
 * PayButton spec: https://github.com/Bitcoin-ABC/bitcoin-abc/blob/master/doc/standards/paybutton.md
 *
 * PayButton deep links: https://paybutton.org/app?address=...&amount=...&b=1
 * - address: BIP21 address (ecash:...)
 * - Other params (amount, op_return_raw, etc.) become BIP21 query params
 * - b=1: return to browser after send/reject
 */

export interface PaybuttonDeepLinkResult {
    bip21Uri: string;
    returnToBrowser: boolean;
}

/**
 * Convert a PayButton deep link URL to a BIP21 URI
 *
 * @param deepLink - URL like https://paybutton.org/app?address=ecash:...&amount=1&b=1
 * @returns BIP21 URI and returnToBrowser flag, or passes through if not a paybutton URL
 */
export function paybuttonDeepLinkToBip21Uri(
    deepLink: string,
): PaybuttonDeepLinkResult {
    try {
        const url = new URL(deepLink);

        if (
            url.protocol !== 'https:' ||
            (url.hostname !== 'paybutton.org' &&
                url.hostname !== 'api.paybutton.org') ||
            url.pathname !== '/app'
        ) {
            return { bip21Uri: deepLink, returnToBrowser: false };
        }

        const address = url.searchParams.get('address');
        if (!address) {
            return { bip21Uri: deepLink, returnToBrowser: false };
        }
        url.searchParams.delete('address');

        // b=1 means return to browser
        const b = url.searchParams.get('b');
        if (b !== null) {
            url.searchParams.delete('b');
        }

        const queryString = url.searchParams.toString();
        const bip21Uri = queryString ? `${address}?${queryString}` : address;

        return {
            bip21Uri,
            returnToBrowser: b === '1',
        };
    } catch {
        return { bip21Uri: deepLink, returnToBrowser: false };
    }
}
