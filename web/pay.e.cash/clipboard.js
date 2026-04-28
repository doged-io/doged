// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Copy text: prefer Clipboard API; fall back to execCommand copy for insecure HTTP
 * (e.g. local dev without TLS) where `navigator.clipboard` is unavailable.
 *
 * Exposed on `window` for plain-script pages (`index.html`, `test.html`).
 *
 * @param {string} text
 * @returns {Promise<void>}
 */
window.copyTextToClipboard = async function copyTextToClipboard(text) {
    if (
        navigator.clipboard &&
        typeof navigator.clipboard.writeText === 'function'
    ) {
        try {
            await navigator.clipboard.writeText(text);
            return;
        } catch {
            /* fall through */
        }
    }
    const ta = document.createElement('textarea');
    ta.value = text;
    ta.setAttribute('readonly', '');
    ta.style.position = 'fixed';
    ta.style.left = '-9999px';
    document.body.appendChild(ta);
    ta.focus();
    ta.select();
    // execCommand copy is deprecated but still needed when Clipboard API cannot run.
    // @ts-expect-error execCommand is deprecated in DOM typings; fallback for non-secure origins
    const ok = document.execCommand('copy');
    document.body.removeChild(ta);
    if (!ok) {
        throw new Error('Clipboard unavailable');
    }
};
