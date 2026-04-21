// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { webViewError } from '../common';
import { t } from '../i18n';

/**
 * Global error overlay (title, message, optional expandable URI/details).
 */
export class ErrorModal {
    private ui: {
        overlay: HTMLElement;
        title: HTMLElement;
        message: HTMLElement;
        closeBtn: HTMLButtonElement;
        detailsSection: HTMLElement;
        detailsToggle: HTMLButtonElement;
        detailsQuote: HTMLElement;
        detailsPre: HTMLPreElement;
    };

    constructor() {
        this.assertUIElements();
    }

    private assertUIElements(): void {
        this.ui = {
            overlay: document.getElementById(
                'error-modal-overlay',
            ) as HTMLElement,
            title: document.getElementById('error-modal-title') as HTMLElement,
            message: document.getElementById(
                'error-modal-message',
            ) as HTMLElement,
            closeBtn: document.getElementById(
                'error-modal-close',
            ) as HTMLButtonElement,
            detailsSection: document.getElementById(
                'error-modal-details-section',
            ) as HTMLElement,
            detailsToggle: document.getElementById(
                'error-modal-details-toggle',
            ) as HTMLButtonElement,
            detailsQuote: document.getElementById(
                'error-modal-details-quote',
            ) as HTMLElement,
            detailsPre: document.getElementById(
                'error-modal-details-text',
            ) as HTMLPreElement,
        };

        if (
            !this.ui.overlay ||
            !this.ui.title ||
            !this.ui.message ||
            !this.ui.closeBtn ||
            !this.ui.detailsSection ||
            !this.ui.detailsToggle ||
            !this.ui.detailsQuote ||
            !this.ui.detailsPre
        ) {
            webViewError('Missing required UI elements for error modal');
            throw new Error('Missing required UI elements for error modal');
        }
    }

    private collapseDetails(): void {
        this.ui.detailsQuote.classList.add('hidden');
        this.ui.detailsToggle.textContent = t('app.showDetails');
        this.ui.detailsToggle.setAttribute('aria-expanded', 'false');
    }

    /**
     * Show the global error modal. When `details` is a non-empty string, a
     * "Show details" control reveals it in a quote-style block.
     */
    show(title: string, message: string, details?: string): void {
        const {
            overlay,
            title: titleEl,
            message: messageEl,
            closeBtn,
            detailsSection,
            detailsToggle,
            detailsQuote,
            detailsPre,
        } = this.ui;

        titleEl.textContent = title;
        messageEl.textContent = message;

        const rawDetails = details?.trim();
        if (rawDetails) {
            detailsSection.classList.remove('hidden');
            detailsPre.textContent = rawDetails;
            detailsQuote.classList.add('hidden');
            detailsToggle.textContent = t('app.showDetails');
            detailsToggle.setAttribute('aria-expanded', 'false');
            detailsToggle.onclick = () => {
                if (!detailsQuote.classList.contains('hidden')) {
                    this.collapseDetails();
                } else {
                    detailsQuote.classList.remove('hidden');
                    detailsToggle.textContent = t('app.hideDetails');
                    detailsToggle.setAttribute('aria-expanded', 'true');
                }
            };
        } else {
            detailsSection.classList.add('hidden');
            detailsPre.textContent = '';
            detailsQuote.classList.add('hidden');
            detailsToggle.onclick = null;
            this.collapseDetails();
        }

        closeBtn.onclick = () => {
            overlay.style.display = 'none';
            if (!detailsQuote.classList.contains('hidden')) {
                this.collapseDetails();
            }
        };

        overlay.style.display = 'flex';
    }

    /** BIP21-specific title with the same body/details behavior as {@link show}. */
    showBip21ParseError(message: string, uri: string): void {
        this.show(t('bip21.unavailableTitle'), message, uri);
    }
}
