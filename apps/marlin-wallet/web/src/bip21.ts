// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { config } from './config';
import { isValidECashAddress } from './address';
import { atomsToUnit, unitToAtoms } from './amount';
import { SUPPORTED_ASSETS, XEC_ASSET } from './supported-assets';
import { t } from './i18n';

/** User-visible parse failure reasons (string values for display / i18n mapping). */
export enum Bip21Error {
    UriMalformed = 'bip21.malformed',
    TokenNotSupported = 'bip21.unsupportedToken',
}

/**
 * Result of parsing a BIP21 URI
 */
export interface Bip21ParseResult {
    /** The URI string that was parsed */
    uri: string;
    address: string;
    /**
     * Amount in base units when an amount was parsed: satoshis for XEC
     * (`tokenAssetKey === 'xec'`), token atoms for a token send.
     */
    atoms?: number;
    // OP_RETURN data in hex (without 6a OP_RETURN opcode)
    opReturnRaw?: string;
    /** Marlin asset key (`xec` or a built-in token from `token_id`). */
    tokenAssetKey: string;
    /**
     * When set, parsing failed; ignore the other fields (except `uri`).
     * Localized user message.
     */
    error?: string;
}

function bip21ParseError(kind: Bip21Error, uri: string): Bip21ParseResult {
    return {
        uri,
        address: '',
        tokenAssetKey: XEC_ASSET.key,
        error: t(kind),
    };
}

/**
 * Create a BIP21 URI from an address and optional amount
 *
 * @param address - The eCash address (may include network prefix like "ectest:" or "ecash:")
 * @param amountSats - Optional amount in satoshis (will be converted to XEC in the URI)
 * @returns A BIP21 URI string (e.g., "ecash:address" or "ecash:address?amount=100.00")
 */
export function createBip21Uri(address: string, amountSats?: number): string {
    // Strip any existing prefix to get the raw address
    const rawAddress = address.includes(':') ? address.split(':')[1] : address;

    // Build BIP21 URI with config prefix
    let bip21Uri = config.bip21Prefix + rawAddress;

    // Add amount parameter if provided and positive
    if (amountSats && amountSats > 0) {
        // Convert satoshis to XEC using the standard conversion function
        const amountXec = atomsToUnit(amountSats, XEC_ASSET.decimals);
        bip21Uri += `?amount=${amountXec.toFixed(XEC_ASSET.decimals)}`;
    }

    return bip21Uri;
}

/**
 * Parse an amount (decimal) string in base unit and return the corresponding
 * number of atoms (aka sats for XEC). Returns null if the amount is invalid.
 *
 * The specification for amount in BIP21 is very relaxed:
 *  "amount=" *digit [ "." *digit ]
 * This means that any number of leading/trailing zeros are allowed, and numbers
 * can even be missing before and/or after the decimal point. Note that we treat
 * the empty string (which would be valid per spec) as invalid.
 */
export function parseAmountAsAtoms(
    amountString: string,
    maxDecimals: number,
): number | null {
    // Bail early if the amount string is empty.
    if (amountString.length < 1) {
        return null;
    }

    // Amount can only contain numbers and '.'. Note it can't contain + or -
    // signs so the amount can't be negative.
    const re = /^[0-9]*([.][0-9]*)?$/;
    if (!re.test(amountString)) {
        return null;
    }

    let [intPart, decPart] = amountString.split('.');
    intPart = intPart || '0';
    decPart = decPart || '';

    if (decPart.length > maxDecimals) {
        return null;
    }

    try {
        return unitToAtoms(parseFloat(`${intPart}.${decPart}`), maxDecimals);
    } catch {
        // Note this might throw if the atoms amount is too large to fit into a
        // number.
        return null;
    }
}

/**
 * Parse a 32-byte unsigned integer encoded as 64 hex digits (e.g. BIP21
 * `token_id`). Returns lowercase hex or null if invalid.
 */
export function parseUint256Hex(raw: string): string | null {
    const hex = raw.trim().toLowerCase();
    return /^[0-9a-f]{64}$/.test(hex) ? hex : null;
}

/**
 * Parse a BIP21 URI string
 *
 * Supports simplified BIP21 format for eCash:
 * - always starts with ecash: even for other prefixed addresses
 * - Optional amount parameter (e.g., ?amount=100.42)
 * - Optional single-recipient token send: `token_id` must match a Marlin
 *   built-in asset; `token_decimalized_qty` is optional and parsed with
 *   {@link parseAmountAsAtoms}.
 * - Other query parameters are ignored
 *
 * @param uri - The URI string to parse (e.g., "ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07?amount=100.42")
 * @returns Parsed result, or a result with {@link Bip21ParseResult.error} when
 * the URI is malformed or the token is unsupported.
 */
export function parseBip21Uri(uri: string): Bip21ParseResult {
    try {
        // Parse the URI using URL API
        const url = new URL(uri);

        // Validate that the protocol matches the expected BIP21 prefix
        if (url.protocol !== config.bip21Prefix) {
            return bip21ParseError(Bip21Error.UriMalformed, uri);
        }

        // Check if the pathname already has the expected prefix (e.g., "ectest:address")
        // If not, add the configured prefix
        let addressPart = url.pathname;
        if (!addressPart.startsWith(config.addressPrefix + ':')) {
            addressPart = config.addressPrefix + ':' + addressPart;
        }

        // Validate the address (this will catch invalid formats like ecash://address with leading slash)
        if (!isValidECashAddress(addressPart)) {
            return bip21ParseError(Bip21Error.UriMalformed, uri);
        }

        const result: Bip21ParseResult = {
            uri,
            address: addressPart,
            // This can be overridden if token parameters are present.
            tokenAssetKey: XEC_ASSET.key,
        };

        // Parse the amount parameter if present.
        // Amount in BIP21 is specified in XEC, we convert to satoshis (1 XEC = 100 sats)
        const amountParam = url.searchParams.get('amount');
        if (amountParam) {
            const atoms = parseAmountAsAtoms(amountParam, XEC_ASSET.decimals);
            if (atoms !== null) {
                result.atoms = atoms;
            }
        }

        // Parse op_return_raw parameter if present
        const opReturnRaw = url.searchParams.get('op_return_raw');
        if (opReturnRaw) {
            // Validate it's a valid hex string with even number of characters
            const cleanHex = opReturnRaw.trim().toUpperCase();
            if (/^[0-9A-F]+$/.test(cleanHex) && cleanHex.length % 2 === 0) {
                result.opReturnRaw = cleanHex;
            }
        }

        // If amount is present, don't attempt to parse token parameters. Using
        // both is undefined behavior and unsupported by Marlin.
        if (result.atoms === undefined && url.searchParams.has('token_id')) {
            const tokenId = parseUint256Hex(
                url.searchParams.get('token_id') ?? '',
            );
            if (!tokenId) {
                return bip21ParseError(Bip21Error.UriMalformed, uri);
            }

            // Only supported tokens are allowed.
            const matched = SUPPORTED_ASSETS.find(
                a => a.tokenId?.toLowerCase() === tokenId,
            );
            if (!matched?.key) {
                return bip21ParseError(Bip21Error.TokenNotSupported, uri);
            }
            result.tokenAssetKey = matched.key;

            const atoms = parseAmountAsAtoms(
                url.searchParams.get('token_decimalized_qty') ?? '',
                matched.decimals,
            );
            if (atoms !== null) {
                result.atoms = atoms;
            }

            return result;
        }

        return result;
    } catch {
        return bip21ParseError(Bip21Error.UriMalformed, uri);
    }
}
