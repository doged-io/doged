[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / TokenIdEndpoint

# Class: TokenIdEndpoint

Defined in: ChronikClient.ts:510

Allows fetching tokenId confirmedTxs, unconfirmedTxs, history, and UTXOs.

## Constructors

### Constructor

> **new TokenIdEndpoint**(`proxyInterface`, `tokenId`): `TokenIdEndpoint`

Defined in: ChronikClient.ts:514

#### Parameters

##### proxyInterface

`FailoverProxy`

##### tokenId

`string`

#### Returns

`TokenIdEndpoint`

## Methods

### confirmedTxs()

> **confirmedTxs**(`page?`, `pageSize?`): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:544

Fetches the confirmed tx history of this tokenId, in the order they appear on the blockchain.

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

Defined in: ChronikClient.ts:524

Fetches the tx history of this tokenId, in anti-chronological order.

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

Defined in: ChronikClient.ts:564

Fetches the unconfirmed tx history of this tokenId, in chronological order.

NB this endpoint is NOT paginated, even though it does return the TxHistoryPage shape

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

---

### utxos()

> **utxos**(): `Promise`\<[`TokenIdUtxos`](../interfaces/TokenIdUtxos.md)\>

Defined in: ChronikClient.ts:579

Fetches the current UTXO set for this tokenId.

#### Returns

`Promise`\<[`TokenIdUtxos`](../interfaces/TokenIdUtxos.md)\>
