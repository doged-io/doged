[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / PluginEndpoint

# Class: PluginEndpoint

Defined in: ChronikClient.ts:660

Allows fetching plugin UTXOs.

## Constructors

### Constructor

> **new PluginEndpoint**(`proxyInterface`, `pluginName`): `PluginEndpoint`

Defined in: ChronikClient.ts:664

#### Parameters

##### proxyInterface

`FailoverProxy`

##### pluginName

`string`

#### Returns

`PluginEndpoint`

## Methods

### confirmedTxs()

> **confirmedTxs**(`groupHex`, `page?`, `pageSize?`): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:738

Fetches the confirmed tx history of this groupHex for this plugin, in the order they appear on the blockchain.

#### Parameters

##### groupHex

`string`

group as a lowercase hex string

##### page?

`number` = `0`

Page index of the tx history.

##### pageSize?

`number` = `25`

Number of txs per page.

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

---

### groups()

> **groups**(`prefixHex?`, `startHex?`, `pageSize?`): `Promise`\<[`PluginGroups`](../interfaces/PluginGroups.md)\>

Defined in: ChronikClient.ts:687

Fetches groups of this plugin.

#### Parameters

##### prefixHex?

`string`

##### startHex?

`string`

##### pageSize?

`number`

#### Returns

`Promise`\<[`PluginGroups`](../interfaces/PluginGroups.md)\>

---

### history()

> **history**(`groupHex`, `page?`, `pageSize?`): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:716

Fetches the tx history of this groupHex for this plugin, in anti-chronological order.

#### Parameters

##### groupHex

`string`

group as a lowercase hex string

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

> **unconfirmedTxs**(`groupHex`): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:760

Fetches the unconfirmed tx history of this groupHex for this plugin, in chronological order.

#### Parameters

##### groupHex

`string`

group as a lowercase hex string

NB this endpoint is NOT paginated, even though it does return the TxHistoryPage shape

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

---

### utxos()

> **utxos**(`groupHex`): `Promise`\<[`PluginUtxos`](../interfaces/PluginUtxos.md)\>

Defined in: ChronikClient.ts:672

Fetches the current UTXO set for this plugin group.

#### Parameters

##### groupHex

`string`

#### Returns

`Promise`\<[`PluginUtxos`](../interfaces/PluginUtxos.md)\>
