// Copyright (c) 2025 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { entropyToMnemonic, mnemonicToEntropy } from 'ecash-lib';
import englishWordlist from 'ecash-lib/wordlists/english.json';
import randomBytes from 'randombytes';
import {
    isReactNativeWebView,
    sendMessageToBackend,
    webViewLog,
    webViewError,
} from './common';
import { WalletData } from './wallet';

/**
 * Generate a 12-word BIP39 mnemonic using ecash-lib.
 */
export function generateMnemonic(): string {
    const entropy = randomBytes(16);
    return entropyToMnemonic(entropy, englishWordlist);
}

/*
 * Get the mnemonic from the wallet data object.
 * TODO: Switch to be a proper HD wallet. This means that the seed should be
 * stored in ecash-wallet and then we can retrieve the mnemonic from there.
 */
export function getMnemonic(walletData: WalletData): string | null {
    if (!walletData || !walletData.mnemonic) {
        return null;
    }
    return walletData.mnemonic;
}

/**
 * True if the phrase is valid BIP39 English (checksum and word list).
 * mnemonicToEntropy throws if invalid.
 */
export function validateMnemonic(mnemonic: string): boolean {
    try {
        const trimmed = mnemonic.trim();
        if (!trimmed) {
            return false;
        }
        const words = trimmed.split(/\s+/);
        if (words.length === 0) {
            return false;
        }
        const normalized = words.join(' ');
        mnemonicToEntropy(normalized, englishWordlist.words);
        return true;
    } catch {
        return false;
    }
}

/**
 * Get the BIP39 English wordlist (from ecash-lib).
 * @returns Array of all 2048 BIP39 English words
 */
export function getBIP39Wordlist(): string[] {
    return englishWordlist.words;
}

/**
 * Request to store mnemonic in secure storage, fallback to localStorage for
 * web.
 */
export function storeMnemonic(mnemonic: string): void {
    if (!sendMessageToBackend('STORE_MNEMONIC', mnemonic)) {
        // Fallback to localStorage for web
        localStorage.setItem('ecash_wallet_mnemonic', mnemonic);
    }
}

/**
 * Request to load mnemonic from secure storage, fallback to localStorage for
 * web.
 */
export function loadMnemonic(): Promise<string | null> {
    return new Promise((resolve, reject) => {
        if (isReactNativeWebView()) {
            webViewLog('Loading mnemonic from secure storage');
            const handleResponse = (event: MessageEvent) => {
                try {
                    const message = JSON.parse(event.data);
                    if (message.type === 'MNEMONIC_RESPONSE') {
                        document.removeEventListener('message', handleResponse);
                        window.removeEventListener('message', handleResponse);
                        resolve(message.data);
                    }
                } catch (error) {
                    webViewError('Error parsing mnemonic response:', error);
                }
            };

            document.addEventListener('message', handleResponse);
            window.addEventListener('message', handleResponse);

            sendMessageToBackend('LOAD_MNEMONIC', undefined);

            // Timeout after 30 seconds
            setTimeout(() => {
                document.removeEventListener('message', handleResponse);
                window.removeEventListener('message', handleResponse);
                reject(new Error('Timeout loading mnemonic'));
            }, 30000);
        } else {
            webViewLog('Loading mnemonic from local storage');
            // Fallback to localStorage for web
            const mnemonic = localStorage.getItem('ecash_wallet_mnemonic');
            resolve(mnemonic);
        }
    });
}
