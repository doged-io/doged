[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / ScriptEndpoint

# Class: ScriptEndpoint

Defined in: ChronikClient.ts:417

Allows fetching script history and UTXOs.

## Constructors

### Constructor

> **new ScriptEndpoint**(`proxyInterface`, `scriptType`, `scriptPayload`): `ScriptEndpoint`

Defined in: ChronikClient.ts:422

#### Parameters

##### proxyInterface

`FailoverProxy`

##### scriptType

`string`

##### scriptPayload

`string`

#### Returns

`ScriptEndpoint`

## Methods

### confirmedTxs()

> **confirmedTxs**(`page?`, `pageSize?`): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:460

Fetches the confirmed tx history of this script, in the order they appear on the blockchain.

#### Parameters

##### page?

`number` = `0`

Page index of the tx history.

##### pageSize?

`number` = `25`

Number of txs per page.

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

---

### history()

> **history**(`page?`, `pageSize?`): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:440

Fetches the tx history of this script, in anti-chronological order.
This means it's ordered by first-seen first, i.e. TxHistoryPage.txs[0]
will be the most recent tx. If the tx hasn't been seen
by the indexer before, it's ordered by the block timestamp.

#### Parameters

##### page?

`number` = `0`

Page index of the tx history.

##### pageSize?

`number` = `25`

Number of txs per page.

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

---

### unconfirmedTxs()

> **unconfirmedTxs**(): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:480

Fetches the unconfirmed tx history of this script, in chronological order.

NB this endpoint is NOT paginated, even though it does return the TxHistoryPage shape

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

---

### utxos()

> **utxos**(): `Promise`\<[`ScriptUtxos`](../interfaces/ScriptUtxos.md)\>

Defined in: ChronikClient.ts:497

Fetches the current UTXO set for this script.
It is grouped by output script, in case a script type can match multiple
different output scripts (e.g. Taproot on Lotus).

#### Returns

`Promise`\<[`ScriptUtxos`](../interfaces/ScriptUtxos.md)\>
