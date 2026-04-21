[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / LokadIdEndpoint

# Class: LokadIdEndpoint

Defined in: ChronikClient.ts:592

Allows fetching lokadId confirmedTxs, unconfirmedTxs, and history.

## Constructors

### Constructor

> **new LokadIdEndpoint**(`proxyInterface`, `lokadId`): `LokadIdEndpoint`

Defined in: ChronikClient.ts:596

#### Parameters

##### proxyInterface

`FailoverProxy`

##### lokadId

`string`

#### Returns

`LokadIdEndpoint`

## Methods

### confirmedTxs()

> **confirmedTxs**(`page?`, `pageSize?`): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:626

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

Defined in: ChronikClient.ts:606

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

Defined in: ChronikClient.ts:646

Fetches the unconfirmed tx history of this tokenId, in chronological order.

NB this endpoint is NOT paginated, even though it does return the TxHistoryPage shape

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>
