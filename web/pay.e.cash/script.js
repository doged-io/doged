// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

const CASHTAB_WEB_SEND_BASE = 'https://cashtab.com/#/send';
const homeViewEl = document.getElementById('home-view');
const fallbackViewEl = document.getElementById('fallback-view');
const paymentSummaryEl = document.getElementById('payment-summary');
const fullBip21BoxEl = document.getElementById('full-bip21-box');
const openWebLinkEl = document.getElementById('open-web-link');
const copyBip21ButtonEl = document.getElementById('copy-bip21-btn');

let currentBip21 = null;
let copyBip21TooltipTimeoutId = null;
let fullBip21CopiedTimeoutId = null;

const parseBip21FromQuery = () => {
    const params = new URLSearchParams(window.location.search);
    const bip21Param = params.get('bip21');
    if (!bip21Param) {
        return null;
    }

    if (!bip21Param.toLowerCase().startsWith('ecash:')) {
        return null;
    }

    // Support raw query style links where params after the first '&' are parsed
    // as outer URL params, e.g. ?bip21=ecash:addr?token_id=...&token_decimalized_qty=...
    const trailingParams = [];
    for (const [key, value] of params.entries()) {
        if (key === 'bip21') {
            continue;
        }
        trailingParams.push(`${key}=${value}`);
    }

    if (trailingParams.length === 0) {
        return bip21Param;
    }

    const joiner = bip21Param.includes('?') ? '&' : '?';
    return `${bip21Param}${joiner}${trailingParams.join('&')}`;
};

/** Matches Cashtab web hash-route format (`#/send?bip21=ecash:...?...`), not %-encoded params */
const buildCashtabWebSendUrl = bip21 =>
    `${CASHTAB_WEB_SEND_BASE}?bip21=${bip21}`;

const showFallbackView = bip21 => {
    currentBip21 = bip21;
    const cashtabWebUrl = buildCashtabWebSendUrl(bip21);
    if (homeViewEl) {
        homeViewEl.classList.add('hidden');
    }
    if (fallbackViewEl) {
        fallbackViewEl.classList.remove('hidden');
    }
    if (paymentSummaryEl) {
        paymentSummaryEl.textContent = 'BIP21 payment detected:';
    }
    if (fullBip21BoxEl) {
        fullBip21BoxEl.textContent = bip21;
    }
    if (openWebLinkEl) {
        openWebLinkEl.href = cashtabWebUrl;
    }
};

const copyBip21 = async () => {
    if (!currentBip21 || !copyBip21ButtonEl) {
        return;
    }
    try {
        await window.copyTextToClipboard(currentBip21);
        copyBip21ButtonEl.classList.add('copied');
        if (copyBip21TooltipTimeoutId !== null) {
            clearTimeout(copyBip21TooltipTimeoutId);
        }
        copyBip21TooltipTimeoutId = setTimeout(() => {
            copyBip21ButtonEl.classList.remove('copied');
            copyBip21TooltipTimeoutId = null;
        }, 1000);
    } catch {
        // No visual change on failure to avoid replacing button label text.
    }
};

const run = () => {
    const bip21 = parseBip21FromQuery();
    if (!bip21) {
        return;
    }
    showFallbackView(bip21);
};

run();

copyBip21ButtonEl?.addEventListener('click', () => {
    copyBip21();
});

fullBip21BoxEl?.addEventListener('click', async () => {
    if (!currentBip21) {
        return;
    }
    try {
        await window.copyTextToClipboard(currentBip21);
        fullBip21BoxEl.classList.add('copied');
        if (fullBip21CopiedTimeoutId !== null) {
            clearTimeout(fullBip21CopiedTimeoutId);
        }
        fullBip21CopiedTimeoutId = setTimeout(() => {
            fullBip21BoxEl.classList.remove('copied');
            fullBip21CopiedTimeoutId = null;
        }, 1000);
    } catch {
        // Ignore box tooltip on copy failure; primary copy button still indicates errors.
    }
});
