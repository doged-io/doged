// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { expect, use, assert } from 'chai';
import chaiAsPromised from 'chai-as-promised';
import { ChronikClient } from 'chronik-client';
import {
    ALL_BIP143,
    ALP_STANDARD,
    DEFAULT_DUST_LIMIT,
    Ecc,
    P2PKHSignatory,
    Script,
    TxBuilderInput,
    alpSend,
    emppScript,
    fromHex,
    initWasm,
    shaRmd160,
    toHex,
} from 'ecash-lib';
import { TestRunner } from 'ecash-lib/dist/test/testRunner.js';

import { AgoraPartial } from '../src/partial.js';
import { makeAlpOffer, takeAlpOffer } from './partial-helper-alp.js';
import { Agora, TakenInfo } from '../src/agora.js';

use(chaiAsPromised);

// This test needs a lot of sats
const NUM_COINS = 500;
const COIN_VALUE = 1100000000;

const BASE_PARAMS_ALP = {
    tokenId: '00'.repeat(32), // filled in later
    tokenType: ALP_STANDARD,
    tokenProtocol: 'ALP' as const,
    dustAmount: DEFAULT_DUST_LIMIT,
};

describe('AgoraPartial ALP', () => {
    let runner: TestRunner;
    let chronik: ChronikClient;
    let ecc: Ecc;

    let makerSk: Uint8Array;
    let makerPk: Uint8Array;
    let makerPkh: Uint8Array;
    let makerScript: Script;
    let makerScriptHex: string;
    let takerSk: Uint8Array;
    let takerPk: Uint8Array;
    let takerPkh: Uint8Array;
    let takerScript: Script;
    let takerScriptHex: string;

    async function makeBuilderInputs(
        values: number[],
    ): Promise<TxBuilderInput[]> {
        const txid = await runner.sendToScript(values, makerScript);
        return values.map((value, outIdx) => ({
            input: {
                prevOut: {
                    txid,
                    outIdx,
                },
                signData: {
                    value,
                    outputScript: makerScript,
                },
            },
            signatory: P2PKHSignatory(makerSk, makerPk, ALL_BIP143),
        }));
    }

    before(async () => {
        await initWasm();
        runner = await TestRunner.setup('setup_scripts/ecash-agora_base');
        chronik = runner.chronik;
        ecc = runner.ecc;
        await runner.setupCoins(NUM_COINS, COIN_VALUE);

        makerSk = fromHex('33'.repeat(32));
        makerPk = ecc.derivePubkey(makerSk);
        makerPkh = shaRmd160(makerPk);
        makerScript = Script.p2pkh(makerPkh);
        makerScriptHex = toHex(makerScript.bytecode);
        takerSk = fromHex('44'.repeat(32));
        takerPk = ecc.derivePubkey(takerSk);
        takerPkh = shaRmd160(takerPk);
        takerScript = Script.p2pkh(takerPkh);
        takerScriptHex = toHex(takerScript.bytecode);
    });

    after(() => {
        runner.stop();
    });

    interface TestCase {
        offeredTokens: bigint;
        info: string;
        priceNanoSatsPerToken: bigint;
        acceptedTokens: bigint;
        askedSats: number;
        allowUnspendable?: boolean;
    }
    const TEST_CASES: TestCase[] = [
        {
            offeredTokens: 1000n,
            info: '1sat/token, full accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 1000n,
            askedSats: 1000,
        },
        {
            offeredTokens: 1000n,
            info: '1sat/token, dust accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 546n,
            askedSats: 546,
            allowUnspendable: true,
        },
        {
            offeredTokens: 1000n,
            info: '1000sat/token, full accept',
            priceNanoSatsPerToken: 1000n * 1000000000n,
            acceptedTokens: 1000n,
            askedSats: 1000225,
        },
        {
            offeredTokens: 1000n,
            info: '1000sat/token, half accept',
            priceNanoSatsPerToken: 1000n * 1000000000n,
            acceptedTokens: 500n,
            askedSats: 500113,
        },
        {
            offeredTokens: 1000n,
            info: '1000sat/token, 1 accept',
            priceNanoSatsPerToken: 1000n * 1000000000n,
            acceptedTokens: 1n,
            askedSats: 1001,
        },
        {
            offeredTokens: 1000n,
            info: '1000000sat/token, full accept',
            priceNanoSatsPerToken: 1000000n * 1000000000n,
            acceptedTokens: 1000n,
            askedSats: 1000013824,
        },
        {
            offeredTokens: 1000n,
            info: '1000000sat/token, half accept',
            priceNanoSatsPerToken: 1000000n * 1000000000n,
            acceptedTokens: 500n,
            askedSats: 500039680,
        },
        {
            offeredTokens: 1000n,
            info: '1000000sat/token, 1 accept',
            priceNanoSatsPerToken: 1000000n * 1000000000n,
            acceptedTokens: 1n,
            askedSats: 1048576,
        },
        {
            offeredTokens: 1000n,
            info: '1000000000sat/token, 1 accept',
            priceNanoSatsPerToken: 1000000000n * 1000000000n,
            acceptedTokens: 1n,
            askedSats: 1006632960,
        },
        {
            offeredTokens: 1000000n,
            info: '0.001sat/token, full accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 1000000n,
            askedSats: 1000,
        },
        {
            offeredTokens: 1000000n,
            info: '1sat/token, full accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 1000000n,
            askedSats: 1000000,
        },
        {
            offeredTokens: 1000000n,
            info: '1sat/token, half accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 500000n,
            askedSats: 500000,
        },
        {
            offeredTokens: 1000000n,
            info: '1sat/token, dust accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 546n,
            askedSats: 546,
        },
        {
            offeredTokens: 1000000n,
            info: '1000sat/token, full accept',
            priceNanoSatsPerToken: 1000n * 1000000000n,
            acceptedTokens: 999936n,
            askedSats: 999948288,
        },
        {
            offeredTokens: 1000000n,
            info: '1000sat/token, half accept',
            priceNanoSatsPerToken: 1000n * 1000000000n,
            acceptedTokens: 499968n,
            askedSats: 499974144,
        },
        {
            offeredTokens: 1000000n,
            info: '1000sat/token, 256 accept',
            priceNanoSatsPerToken: 1000n * 1000000000n,
            acceptedTokens: 256n,
            askedSats: 262144,
        },
        {
            offeredTokens: 1000000n,
            info: '1000000sat/token, 1024 accept',
            priceNanoSatsPerToken: 1000000n * 1000000000n,
            acceptedTokens: 1024n,
            askedSats: 1040187392,
        },
        {
            offeredTokens: 1000000n,
            info: '1000000sat/token, 256 accept',
            priceNanoSatsPerToken: 1000000n * 1000000000n,
            acceptedTokens: 256n,
            askedSats: 268435456,
        },
        {
            offeredTokens: 1000000000n,
            info: '0.001sat/token, full accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 1000000000n,
            askedSats: 1000000,
        },
        {
            offeredTokens: 1000000000n,
            info: '0.001sat/token, half accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 500000000n,
            askedSats: 500000,
        },
        {
            offeredTokens: 1000000000n,
            info: '0.001sat/token, dust accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 546000n,
            askedSats: 546,
        },
        {
            offeredTokens: 1000000000n,
            info: '1sat/token, full accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 1000000000n,
            askedSats: 1000000000,
        },
        {
            offeredTokens: 1000000000n,
            info: '1sat/token, half accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 500000000n,
            askedSats: 500000000,
        },
        {
            offeredTokens: 1000000000n,
            info: '1sat/token, dust accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 546n,
            askedSats: 546,
        },
        {
            offeredTokens: 1000000000n,
            info: '1000sat/token, 983040 accept',
            priceNanoSatsPerToken: 1000n * 1000000000n,
            acceptedTokens: 983040n,
            askedSats: 989855744,
        },
        {
            offeredTokens: 1000000000n,
            info: '1000sat/token, 65536 accept',
            priceNanoSatsPerToken: 1000n * 1000000000n,
            acceptedTokens: 65536n,
            askedSats: 67108864,
        },
        {
            offeredTokens: 1000000000000n,
            info: '0.000001sat/token, full accept',
            priceNanoSatsPerToken: 1000n,
            acceptedTokens: 999999995904n,
            askedSats: 1000108,
        },
        {
            offeredTokens: 1000000000000n,
            info: '0.000001sat/token, half accept',
            priceNanoSatsPerToken: 1000n,
            acceptedTokens: 546045952n,
            askedSats: 547,
        },
        {
            offeredTokens: 1000000000000n,
            info: '0.001sat/token, full accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 999999995904n,
            askedSats: 1068115230,
        },
        {
            offeredTokens: 1000000000000n,
            info: '0.001sat/token, dust accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 589824n,
            askedSats: 630,
        },
        {
            offeredTokens: 0x7fffffffffffn,
            info: '0.000000001sat/token, full accept',
            priceNanoSatsPerToken: 1n,
            acceptedTokens: 0x7fffc4660000n,
            askedSats: 140744,
        },
        {
            offeredTokens: 0x7fffffffffffn,
            info: '0.000000001sat/token, dust accept',
            priceNanoSatsPerToken: 1n,
            acceptedTokens: 0x7f1e660000n,
            askedSats: 546,
        },
        {
            offeredTokens: 0x7fffffffffffn,
            info: '0.000001sat/token, full accept',
            priceNanoSatsPerToken: 1000n,
            acceptedTokens: 0x7ffffff10000n,
            askedSats: 143165576,
        },
        {
            offeredTokens: 0x7fffffffffffn,
            info: '0.000001sat/token, dust accept',
            priceNanoSatsPerToken: 1000n,
            acceptedTokens: 0x1ffd0000n,
            askedSats: 546,
        },
        {
            offeredTokens: 0x7fffffffffffn,
            info: '0.001sat/token, max sats accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 799999983616n,
            askedSats: 1041666816,
        },
        {
            offeredTokens: 0x7fffffffffffn,
            info: '0.001sat/token, dust accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 0x70000n,
            askedSats: 768,
        },
        {
            offeredTokens: 0x7fffffffffffn,
            info: '1sat/token, max sats accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 999948288n,
            askedSats: 999948288,
        },
        {
            offeredTokens: 0x7fffffffffffn,
            info: '1sat/token, min accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 0x10000n,
            askedSats: 0x10000,
        },
        {
            offeredTokens: 0xffffffffffffn,
            info: '0.000000001sat/token, full accept',
            priceNanoSatsPerToken: 1n,
            acceptedTokens: 0xffffff000000n,
            askedSats: 281505,
        },
        {
            offeredTokens: 0xffffffffffffn,
            info: '0.000000001sat/token, dust accept',
            priceNanoSatsPerToken: 1n,
            acceptedTokens: 0x7f1c000000n,
            askedSats: 546,
        },
        {
            offeredTokens: 0xffffffffffffn,
            info: '0.000001sat/token, full accept',
            priceNanoSatsPerToken: 1000n,
            acceptedTokens: 0xffffff000000n,
            askedSats: 306783360,
        },
        {
            offeredTokens: 0xffffffffffffn,
            info: '0.000001sat/token, dust accept',
            priceNanoSatsPerToken: 1000n,
            acceptedTokens: 0x1e000000n,
            askedSats: 549,
        },
        {
            offeredTokens: 0xffffffffffffn,
            info: '0.001sat/token, max sats accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 0x8000000000n,
            askedSats: 1073741824,
        },
        {
            offeredTokens: 0xffffffffffffn,
            info: '0.001sat/token, min accept',
            priceNanoSatsPerToken: 1000000n,
            acceptedTokens: 0x1000000n,
            askedSats: 32768,
        },
        {
            offeredTokens: 0xffffffffffffn,
            info: '1sat/token, max sats accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 989855744n,
            askedSats: 989855744,
        },
        {
            offeredTokens: 0xffffffffffffn,
            info: '1sat/token, min accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 0x1000000n,
            askedSats: 0x1000000,
        },
    ];

    for (const testCase of TEST_CASES) {
        it(`AgoraPartial ALP ${testCase.offeredTokens} for ${testCase.info}`, async () => {
            const agora = new Agora(chronik);
            const agoraPartial = await agora.selectParams({
                offeredTokens: testCase.offeredTokens,
                priceNanoSatsPerToken: testCase.priceNanoSatsPerToken,
                minAcceptedTokens: testCase.acceptedTokens,
                makerPk,
                ...BASE_PARAMS_ALP,
            });
            const askedSats = agoraPartial.askedSats(testCase.acceptedTokens);
            const requiredSats = askedSats + 2000n;
            const [fuelInput, takerInput] = await makeBuilderInputs([
                4000,
                Number(requiredSats),
            ]);

            const offer = await makeAlpOffer({
                chronik,
                ecc,
                agoraPartial,
                makerSk,
                fuelInput,
            });
            const acceptTxid = await takeAlpOffer({
                chronik,
                ecc,
                takerSk,
                offer,
                takerInput,
                acceptedTokens: testCase.acceptedTokens,
                allowUnspendable: testCase.allowUnspendable,
            });
            const acceptTx = await chronik.tx(acceptTxid);
            const offeredTokens = agoraPartial.offeredTokens();
            const isFullAccept = testCase.acceptedTokens == offeredTokens;
            if (isFullAccept) {
                // FULL ACCEPT
                // 0th output is OP_RETURN eMPP AGR0 ad + ALP SEND
                expect(acceptTx.outputs[0].outputScript).to.equal(
                    toHex(
                        emppScript([
                            agoraPartial.adPushdata(),
                            alpSend(
                                agoraPartial.tokenId,
                                agoraPartial.tokenType,
                                [0, agoraPartial.offeredTokens()],
                            ),
                        ]).bytecode,
                    ),
                );
                expect(acceptTx.outputs[0].value).to.equal(0);
                expect(acceptTx.outputs[0].token).to.equal(undefined);
                // 1st output is sats to maker
                expect(acceptTx.outputs[1].token).to.equal(undefined);
                expect(acceptTx.outputs[1].value).to.equal(testCase.askedSats);
                expect(acceptTx.outputs[1].outputScript).to.equal(
                    makerScriptHex,
                );
                // 2nd output is tokens to taker
                expect(acceptTx.outputs[2].token?.amount).to.equal(
                    offeredTokens.toString(),
                );
                expect(acceptTx.outputs[2].value).to.equal(DEFAULT_DUST_LIMIT);
                expect(acceptTx.outputs[2].outputScript).to.equal(
                    takerScriptHex,
                );
                // Offer is now gone
                const newOffers = await agora.activeOffersByTokenId(
                    offer.token.tokenId,
                );
                expect(newOffers).to.deep.equal([]);
                return;
            }

            // PARTIAL ACCEPT
            const leftoverTokens = offeredTokens - testCase.acceptedTokens;
            const leftoverTruncTokens =
                leftoverTokens >> BigInt(8 * agoraPartial.numTokenTruncBytes);
            // 0th output is OP_RETURN eMPP AGR0 ad + ALP SEND
            expect(acceptTx.outputs[0].outputScript).to.equal(
                toHex(
                    emppScript([
                        agoraPartial.adPushdata(),
                        alpSend(agoraPartial.tokenId, agoraPartial.tokenType, [
                            0,
                            leftoverTokens,
                            testCase.acceptedTokens,
                        ]),
                    ]).bytecode,
                ),
            );
            expect(acceptTx.outputs[0].value).to.equal(0);
            expect(acceptTx.outputs[0].token).to.equal(undefined);
            // 1st output is sats to maker
            expect(acceptTx.outputs[1].token).to.equal(undefined);
            expect(acceptTx.outputs[1].value).to.equal(testCase.askedSats);
            expect(acceptTx.outputs[1].outputScript).to.equal(makerScriptHex);
            // 2nd output is back to the P2SH Script
            expect(acceptTx.outputs[2].token?.amount).to.equal(
                leftoverTokens.toString(),
            );
            expect(acceptTx.outputs[2].value).to.equal(DEFAULT_DUST_LIMIT);
            expect(acceptTx.outputs[2].outputScript.slice(0, 4)).to.equal(
                'a914',
            );
            // 3rd output is tokens to taker
            expect(acceptTx.outputs[3].token?.amount).to.equal(
                testCase.acceptedTokens.toString(),
            );
            expect(acceptTx.outputs[3].value).to.equal(DEFAULT_DUST_LIMIT);
            expect(acceptTx.outputs[3].outputScript).to.equal(takerScriptHex);
            // Offer is now modified
            const newOffers = await agora.activeOffersByTokenId(
                offer.token.tokenId,
            );
            expect(newOffers.length).to.equal(1);
            const newOffer = newOffers[0];
            expect(newOffer.variant).to.deep.equal({
                type: 'PARTIAL',
                params: new AgoraPartial({
                    ...agoraPartial,
                    truncTokens: leftoverTruncTokens,
                }),
            });

            // Cancel leftover offer
            const cancelFeeSats = newOffer.cancelFeeSats({
                recipientScript: makerScript,
                extraInputs: [fuelInput], // dummy input for measuring
            });
            const cancelTxSer = newOffer
                .cancelTx({
                    ecc,
                    cancelSk: makerSk,
                    fuelInputs: await makeBuilderInputs([
                        Number(cancelFeeSats),
                    ]),
                    recipientScript: makerScript,
                })
                .ser();
            const cancelTxid = (await chronik.broadcastTx(cancelTxSer)).txid;
            const cancelTx = await chronik.tx(cancelTxid);
            expect(cancelTx.outputs[1].token?.amount).to.equal(
                leftoverTokens.toString(),
            );
            expect(cancelTx.outputs[1].outputScript).to.equal(makerScriptHex);

            // takerIndex is 2 for full accept, 3 for partial accept
            const takerIndex = isFullAccept ? 2 : 3;

            // Get takenInfo from offer creation params
            const takenInfo: TakenInfo = {
                satoshisPaid: testCase.askedSats,
                takerScriptHex: acceptTx.outputs[takerIndex].outputScript,
                baseTokens: testCase.acceptedTokens.toString(),
            };

            // Tx history by token ID
            const offers = [
                {
                    ...offer,
                    status: 'TAKEN',
                    takenInfo,
                },
                {
                    ...newOffer,
                    status: 'CANCELED',
                },
            ];
            const actualOffers = await agora.historicOffers({
                type: 'TOKEN_ID',
                tokenId: offer.token.tokenId,
                table: 'UNCONFIRMED',
            });
            if (offers[0].status !== actualOffers.offers[0].status) {
                offers.reverse();
            }
            expect(actualOffers).to.deep.equal({
                offers,
                numTxs: 3,
                numPages: 1,
            });
        });
    }
    it('Without manually setting an over-ride, we are unable to accept an agora partial if the remaining offer would be unacceptable due to the terms of the contract', async () => {
        const thisTestCase: TestCase = {
            offeredTokens: 1000n,
            info: '1sat/token, dust accept',
            priceNanoSatsPerToken: 1000000000n,
            acceptedTokens: 546n,
            askedSats: 546,
            allowUnspendable: true,
        };
        const agora = new Agora(chronik);
        const agoraPartial = await agora.selectParams({
            offeredTokens: thisTestCase.offeredTokens,
            priceNanoSatsPerToken: thisTestCase.priceNanoSatsPerToken,
            minAcceptedTokens: thisTestCase.acceptedTokens,
            makerPk,
            ...BASE_PARAMS_ALP,
        });
        const askedSats = agoraPartial.askedSats(thisTestCase.acceptedTokens);
        const requiredSats = askedSats + 2000n;
        const [fuelInput, takerInput] = await makeBuilderInputs([
            4000,
            Number(requiredSats),
        ]);

        const offer = await makeAlpOffer({
            chronik,
            ecc,
            agoraPartial,
            makerSk,
            fuelInput,
        });

        const expectedError = `Accepting ${thisTestCase.acceptedTokens} token satoshis would leave an amount lower than the min acceptable by the terms of this contract, and hence unacceptable. Accept fewer tokens or the full offer.`;

        // We can get the error from the isolated method
        expect(() =>
            agoraPartial.preventUnacceptableRemainder(
                thisTestCase.acceptedTokens,
            ),
        ).to.throw(Error, expectedError);

        // We get an error for test cases that would result in unspendable amounts
        // if we do not pass allowUnspendable to agoraOffer.acceptTx
        await assert.isRejected(
            takeAlpOffer({
                chronik,
                ecc,
                takerSk,
                offer,
                takerInput,
                acceptedTokens: thisTestCase.acceptedTokens,
                allowUnspendable: false,
            }),
            expectedError,
        );
    });
    it('Without manually setting an over-ride, we are unable to accept an agora partial if the remaining offer would be unacceptable due to a price less than dust', async () => {
        // ecash-agora does not support creating an agora partial with min accept amount priced less than dust
        // from the approximateParams method
        // However we can still do this if we manually create a new AgoraPartial
        // I think it is okay to preserve this, as the protocol does technically allow it,
        // and perhaps a power user wants to do this for some reason

        // Manually build an offer equivalent to previous test but accepting 500 tokens
        const agoraPartial = new AgoraPartial({
            truncTokens: 1000n,
            numTokenTruncBytes: 0,
            tokenScaleFactor: 2145336n,
            scaledTruncTokensPerTruncSat: 2145336n,
            numSatsTruncBytes: 0,
            minAcceptedScaledTruncTokens: 1072668000n,
            scriptLen: 209,
            enforcedLockTime: 1333546081,
            makerPk,
            ...BASE_PARAMS_ALP,
        });
        const acceptedTokens = 500n;
        const askedSats = agoraPartial.askedSats(acceptedTokens);
        const requiredSats = askedSats + 2000n;
        const [fuelInput, takerInput] = await makeBuilderInputs([
            4000,
            Number(requiredSats),
        ]);

        const offer = await makeAlpOffer({
            chronik,
            ecc,
            agoraPartial,
            makerSk,
            fuelInput,
        });

        const expectedError = `Accepting 500 token satoshis would leave an amount priced lower than dust. Accept fewer tokens or the full offer.`;

        // We can get the error from the isolated method
        expect(() =>
            agoraPartial.preventUnacceptableRemainder(acceptedTokens),
        ).to.throw(Error, expectedError);

        // And from attempting to accept
        await assert.isRejected(
            takeAlpOffer({
                chronik,
                ecc,
                takerSk,
                offer,
                takerInput,
                acceptedTokens: acceptedTokens,
                allowUnspendable: false,
            }),
            expectedError,
        );
    });
});
