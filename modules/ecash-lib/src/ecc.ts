// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/** Interface to abstract over Elliptic Curve Cryptography */
export interface Ecc {
    /** Derive a public key from secret key. */
    derivePubkey(seckey: Uint8Array): Uint8Array;

    /** Sign an ECDSA signature. msg needs to be a 32-byte hash */
    ecdsaSign(seckey: Uint8Array, msg: Uint8Array): Uint8Array;

    /** Sign a Schnorr signature. msg needs to be a 32-byte hash */
    schnorrSign(seckey: Uint8Array, msg: Uint8Array): Uint8Array;

    /**
     * Return whether the given secret key is valid, i.e. whether is of correct
     * length (32 bytes) and is on the curve.
     */
    isValidSeckey(seckey: Uint8Array): boolean;

    /** Add a scalar to a secret key */
    seckeyAdd(a: Uint8Array, b: Uint8Array): Uint8Array;

    /** Add a scalar to a public key (adding G*b) */
    pubkeyAdd(a: Uint8Array, b: Uint8Array): Uint8Array;
}

/** Ecc implementation using WebAssembly */
export let Ecc: { new (): Ecc };

/** Dummy Ecc impl that always returns 0, useful for measuring tx size */
export class EccDummy implements Ecc {
    derivePubkey(_seckey: Uint8Array): Uint8Array {
        return new Uint8Array(33);
    }

    ecdsaSign(_seckey: Uint8Array, _msg: Uint8Array): Uint8Array {
        return new Uint8Array(73);
    }

    schnorrSign(_seckey: Uint8Array, _msg: Uint8Array): Uint8Array {
        return new Uint8Array(64);
    }

    isValidSeckey(_seckey: Uint8Array): boolean {
        return false;
    }

    seckeyAdd(_a: Uint8Array, _b: Uint8Array): Uint8Array {
        return new Uint8Array(32);
    }

    pubkeyAdd(_a: Uint8Array, _b: Uint8Array): Uint8Array {
        return new Uint8Array(32);
    }
}

export function __setEcc(ecc: { new (): Ecc }) {
    Ecc = ecc;
}
