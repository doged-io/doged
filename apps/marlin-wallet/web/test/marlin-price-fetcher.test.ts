// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import * as chai from 'chai';
import * as sinon from 'sinon';
import { CryptoTicker, Fiat, PriceFetcher } from 'ecash-price';
import { MarlinPriceFetcher } from '../src/price';
import { setActiveAsset } from '../src/active-asset';
import { FIRMA_TOKEN, XEC_ASSET } from '../src/supported-assets';

const expect = chai.expect;

const firmaQuoteCurrency = FIRMA_TOKEN.quoteCurrency;
if (firmaQuoteCurrency === undefined) {
    throw new Error(
        'FIRMA_TOKEN.quoteCurrency must be set (see supported-assets)',
    );
}

/** Minimal stand-in for PriceFetcher with stubbed async methods. */
function createInnerFetcherStub() {
    return {
        currentPairs: sinon.stub().resolves(undefined),
        current: sinon.stub().resolves(null as number | null),
    };
}

describe('MarlinPriceFetcher', function () {
    afterEach(function () {
        setActiveAsset({ def: XEC_ASSET });
    });

    it('prefetches currentPairs for all source currencies then returns inner current()', async function () {
        setActiveAsset({ def: XEC_ASSET });
        const inner = createInnerFetcherStub();
        inner.current.resolves(42);

        const sources = [CryptoTicker.XEC, CryptoTicker.USDT];
        const marlin = new MarlinPriceFetcher(
            inner as unknown as PriceFetcher,
            sources,
        );

        const pair = {
            source: CryptoTicker.XEC,
            quote: Fiat.EUR,
        };
        const result = await marlin.current(pair);

        expect(result).to.equal(42);
        expect(inner.currentPairs.calledOnce).to.equal(true);
        expect(inner.currentPairs.firstCall.args[0]).to.deep.equal([
            { source: CryptoTicker.XEC, quote: Fiat.EUR },
            { source: CryptoTicker.USDT, quote: Fiat.EUR },
        ]);
        expect(inner.current.calledOnce).to.equal(true);
        expect(inner.current.firstCall.args[0]).to.deep.equal(pair);
    });

    it('returns 1.0 for Firma stable quote/USD without calling the inner fetcher', async function () {
        setActiveAsset({ def: FIRMA_TOKEN });
        const inner = createInnerFetcherStub();

        const marlin = new MarlinPriceFetcher(
            inner as unknown as PriceFetcher,
            [firmaQuoteCurrency],
        );

        const pair = {
            source: firmaQuoteCurrency,
            quote: Fiat.USD,
        };
        const result = await marlin.current(pair);

        expect(result).to.equal(1.0);
        expect(inner.currentPairs.called).to.equal(false);
        expect(inner.current.called).to.equal(false);
    });

    it('does not short-circuit Firma when quote is not USD', async function () {
        setActiveAsset({ def: FIRMA_TOKEN });
        const inner = createInnerFetcherStub();
        inner.current.resolves(0.92);

        const marlin = new MarlinPriceFetcher(
            inner as unknown as PriceFetcher,
            [firmaQuoteCurrency],
        );

        const pair = {
            source: firmaQuoteCurrency,
            quote: Fiat.EUR,
        };
        const result = await marlin.current(pair);

        expect(result).to.equal(0.92);
        expect(inner.currentPairs.calledOnce).to.equal(true);
        expect(inner.current.calledOnce).to.equal(true);
    });

    it('does not short-circuit when active asset is XEC', async function () {
        setActiveAsset({ def: XEC_ASSET });
        const inner = createInnerFetcherStub();
        inner.current.resolves(1);

        const marlin = new MarlinPriceFetcher(
            inner as unknown as PriceFetcher,
            [firmaQuoteCurrency],
        );

        const pair = {
            source: firmaQuoteCurrency,
            quote: Fiat.USD,
        };
        await marlin.current(pair);

        expect(inner.currentPairs.calledOnce).to.equal(true);
        expect(inner.current.calledOnce).to.equal(true);
    });

    it('does not short-circuit Firma when pair source is not active quote currency', async function () {
        setActiveAsset({ def: FIRMA_TOKEN });
        const inner = createInnerFetcherStub();
        inner.current.resolves(5);

        const marlin = new MarlinPriceFetcher(
            inner as unknown as PriceFetcher,
            [CryptoTicker.XEC, firmaQuoteCurrency],
        );

        const pair = {
            source: CryptoTicker.XEC,
            quote: Fiat.USD,
        };
        const result = await marlin.current(pair);

        expect(result).to.equal(5);
        expect(inner.currentPairs.calledOnce).to.equal(true);
        expect(inner.current.calledOnce).to.equal(true);
    });
});
