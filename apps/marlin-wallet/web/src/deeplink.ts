// Copyright (c) 2025-present The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Deep link protocols detection and decoding
 *
 * PayButton spec: https://github.com/Bitcoin-ABC/bitcoin-abc/blob/master/doc/standards/paybutton.md
 *
 * The PayButton protocol identifier is: 0450415900
 * - 04: Pushdata opcode for 4 bytes
 * - 50415900: "PAY" + null byte (0x00) in ASCII
 *
 * pay.e.cash: a simple BIP21 wrapper
 */

/**
 * PayButton protocol identifier in hex (without OP_RETURN opcode)
 * This is what appears in the op_return_raw field of a BIP21 URI
 */
const PAYBUTTON_PROTOCOL_ID = '0450415900';

/**
 * Check if an op_return_raw hex string is a PayButton transaction
 *
 * @param opReturnRaw - Hex string of the OP_RETURN data (without 6a OP_RETURN opcode)
 * @returns true if it's a PayButton transaction
 */
export function isPayButtonTransaction(opReturnRaw: string): boolean {
    return opReturnRaw.startsWith(PAYBUTTON_PROTOCOL_ID);
}

export interface DeepLinkToBip21UriResult {
    bip21Uri: string | null;
    returnToBrowser: boolean;
}

export function paybuttonDeepLinkToBip21Uri(
    deepLink: string,
): DeepLinkToBip21UriResult {
    try {
        const url = new URL(deepLink);

        if (
            url.protocol !== 'https:' ||
            (url.hostname !== 'paybutton.org' &&
                url.hostname !== 'api.paybutton.org') ||
            url.pathname !== '/app'
        ) {
            return { bip21Uri: null, returnToBrowser: false };
        }

        const address = url.searchParams.get('address');
        if (!address) {
            return { bip21Uri: null, returnToBrowser: false };
        }
        url.searchParams.delete('address');

        // b=1 means return to browser
        const b = url.searchParams.get('b');
        if (b !== null) {
            url.searchParams.delete('b');
        }

        let bip21Params = url.searchParams.toString();
        if (bip21Params.length > 0) {
            bip21Params = '?' + bip21Params;
        }

        return {
            bip21Uri: address + bip21Params,
            returnToBrowser: b === '1',
        };
    } catch {
        return { bip21Uri: null, returnToBrowser: false };
    }
}

export function payecashDeepLinkToBip21Uri(
    deepLink: string,
): DeepLinkToBip21UriResult {
    const bip21QueryParam = '?bip21=';
    try {
        const url = new URL(deepLink);

        // The pay.e.cash link is a a simple wrapper around a BIP21 URI, passed
        // as a param via bip21 query parameter. We can use a simple string
        // matching to extract the BIP21 URI. If any other param is present
        // before the bip21 query parameter, it is invalid. If any other param
        // is present after the bip21 query parameter, it is considered part of
        // the BIP21 URI.
        if (
            url.protocol !== 'https:' ||
            url.hostname !== 'pay.e.cash' ||
            (url.pathname !== '' && url.pathname !== '/') ||
            !url.search.startsWith(bip21QueryParam)
        ) {
            return { bip21Uri: null, returnToBrowser: false };
        }

        const bip21Uri = new URL(url.search.slice(bip21QueryParam.length));

        // b=1 means return to browser
        // FIXME: add support for return to browser to pay.e.cash deep links
        // const b = bip21Uri.searchParams.get('b');
        // if (b !== null) {
        //     bip21Uri.searchParams.delete('b');
        // }
        // Then update the returnToBrowser flag based like so:
        // returnToBrowser = b === '1';

        let bip21Params = bip21Uri.searchParams.toString();
        if (bip21Params.length > 0) {
            bip21Params = '?' + bip21Params;
        }

        return {
            bip21Uri: bip21Uri.protocol + bip21Uri.pathname + bip21Params,
            returnToBrowser: false,
        };
    } catch {
        return { bip21Uri: null, returnToBrowser: false };
    }
}
