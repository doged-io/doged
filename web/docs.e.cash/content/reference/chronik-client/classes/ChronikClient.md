[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / ChronikClient

# Class: ChronikClient

Defined in: ChronikClient.ts:101

Client to access an in-node Chronik instance.
Plain object, without any connections.

## Constructors

### Constructor

> **new ChronikClient**(`urls`): `ChronikClient`

Defined in: ChronikClient.ts:142

Create a new client. This just creates an object, without any connections.

#### Parameters

##### urls

`string`[]

Array of valid urls. A valid url comes with schema and without a trailing slash.
e.g. `['https://chronik.e.cash', 'https://someotherchronikinstance.e.cash']`
The approach of accepting an array of urls as input is to ensure redundancy if the
first url encounters downtime.

#### Returns

`ChronikClient`

#### Throws

throws error on invalid constructor inputs

## Methods

### address()

> **address**(`address`): [`ScriptEndpoint`](ScriptEndpoint.md)

Defined in: ChronikClient.ts:404

Create object that allows fetching script history or UTXOs by p2pkh or p2sh address

#### Parameters

##### address

`string`

#### Returns

[`ScriptEndpoint`](ScriptEndpoint.md)

---

### block()

> **block**(`hashOrHeight`): `Promise`\<[`Block`](../interfaces/Block.md)\>

Defined in: ChronikClient.ts:310

Fetch the block given hash or height.

#### Parameters

##### hashOrHeight

`string` \| `number`

#### Returns

`Promise`\<[`Block`](../interfaces/Block.md)\>

---

### blockchainInfo()

> **blockchainInfo**(): `Promise`\<[`BlockchainInfo`](../interfaces/BlockchainInfo.md)\>

Defined in: ChronikClient.ts:296

Fetch current info of the blockchain, such as tip hash and height.

#### Returns

`Promise`\<[`BlockchainInfo`](../interfaces/BlockchainInfo.md)\>

---

### blocks()

> **blocks**(`startHeight`, `endHeight`): `Promise`\<[`BlockInfo`](../interfaces/BlockInfo.md)[]\>

Defined in: ChronikClient.ts:344

Fetch block info of a range of blocks.
`startHeight` and `endHeight` are inclusive ranges.

#### Parameters

##### startHeight

`number`

##### endHeight

`number`

#### Returns

`Promise`\<[`BlockInfo`](../interfaces/BlockInfo.md)[]\>

---

### blockTxs()

> **blockTxs**(`hashOrHeight`, `page?`, `pageSize?`): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:317

Fetch the tx history of a block given hash or height.

#### Parameters

##### hashOrHeight

`string` \| `number`

##### page?

`number` = `0`

##### pageSize?

`number` = `25`

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

---

### broadcastAndFinalizeTx()

> **broadcastAndFinalizeTx**(`rawTx`, `finalizationTimeoutSecs?`, `skipTokenChecks?`): `Promise`\<\{ `txid`: `string`; \}\>

Defined in: ChronikClient.ts:190

Broadcasts the `rawTx` on the network.

Wait for the tx to finalize by default. Allow user customization of finalizationTimeoutSecs.

If `skipTokenChecks` is false, it will be checked that the tx doesn't burn
any tokens before broadcasting.

#### Parameters

##### rawTx

`string` \| `Uint8Array`\<`ArrayBufferLike`\>

##### finalizationTimeoutSecs?

`number` = `120`

##### skipTokenChecks?

`boolean` = `false`

#### Returns

`Promise`\<\{ `txid`: `string`; \}\>

---

### broadcastAndFinalizeTxs()

> **broadcastAndFinalizeTxs**(`rawTxs`, `finalizationTimeoutSecs?`, `skipTokenChecks?`): `Promise`\<`BroadcastTxsResponse`\>

Defined in: ChronikClient.ts:216

Broadcasts the `rawTxs` on the network, only if all of them are valid.

Wait for the txs to finalize by default. Allow user customization of finalizationTimeoutSecs.

If `skipTokenChecks` is false, it will be checked that the txs don't burn
any tokens before broadcasting.

#### Parameters

##### rawTxs

(`string` \| `Uint8Array`\<`ArrayBufferLike`\>)[]

##### finalizationTimeoutSecs?

`number` = `120`

##### skipTokenChecks?

`boolean` = `false`

#### Returns

`Promise`\<`BroadcastTxsResponse`\>

---

### broadcastTx()

> **broadcastTx**(`rawTx`, `skipTokenChecks?`): `Promise`\<`BroadcastTxResponse`\>

Defined in: ChronikClient.ts:160

Broadcasts the `rawTx` on the network.

If `skipTokenChecks` is false, it will be checked that the tx doesn't burn
any tokens before broadcasting.

NB this method DOES NOT wait for finalization

#### Parameters

##### rawTx

`string` \| `Uint8Array`\<`ArrayBufferLike`\>

##### skipTokenChecks?

`boolean` = `false`

#### Returns

`Promise`\<`BroadcastTxResponse`\>

---

### broadcastTxs()

> **broadcastTxs**(`rawTxs`, `skipTokenChecks?`): `Promise`\<`BroadcastTxsResponse`\>

Defined in: ChronikClient.ts:175

Broadcasts the `rawTxs` on the network, only if all of them are valid.

If `skipTokenChecks` is false, it will be checked that the txs don't burn
any tokens before broadcasting.

NB this method DOES NOT wait for finalization

#### Parameters

##### rawTxs

(`string` \| `Uint8Array`\<`ArrayBufferLike`\>)[]

##### skipTokenChecks?

`boolean` = `false`

#### Returns

`Promise`\<`BroadcastTxsResponse`\>

---

### chronikInfo()

> **chronikInfo**(): `Promise`\<[`ChronikInfo`](../interfaces/ChronikInfo.md)\>

Defined in: ChronikClient.ts:303

Fetch info about the current running chronik server

#### Returns

`Promise`\<[`ChronikInfo`](../interfaces/ChronikInfo.md)\>

---

### lokadId()

> **lokadId**(`lokadId`): [`LokadIdEndpoint`](LokadIdEndpoint.md)

Defined in: ChronikClient.ts:382

Create object that allows fetching info about a given lokadId

#### Parameters

##### lokadId

`string`

#### Returns

[`LokadIdEndpoint`](LokadIdEndpoint.md)

---

### plugin()

> **plugin**(`pluginName`): [`PluginEndpoint`](PluginEndpoint.md)

Defined in: ChronikClient.ts:387

Create object that allows fetching info about a given plugin

#### Parameters

##### pluginName

`string`

#### Returns

[`PluginEndpoint`](PluginEndpoint.md)

---

### proxyInterface()

> **proxyInterface**(): `FailoverProxy`

Defined in: ChronikClient.ts:148

#### Returns

`FailoverProxy`

---

### rawTx()

> **rawTx**(`txid`): `Promise`\<[`RawTx`](../interfaces/RawTx.md)\>

Defined in: ChronikClient.ts:370

Fetch tx details given the txid.

#### Parameters

##### txid

`string`

#### Returns

`Promise`\<[`RawTx`](../interfaces/RawTx.md)\>

---

### script()

> **script**(`scriptType`, `scriptPayload`): [`ScriptEndpoint`](ScriptEndpoint.md)

Defined in: ChronikClient.ts:392

Create object that allows fetching script history or UTXOs.

#### Parameters

##### scriptType

[`ScriptType`](../type-aliases/ScriptType.md)

##### scriptPayload

`string`

#### Returns

[`ScriptEndpoint`](ScriptEndpoint.md)

---

### token()

> **token**(`tokenId`): `Promise`\<[`TokenInfo`](../interfaces/TokenInfo.md)\>

Defined in: ChronikClient.ts:356

Fetch token info and stats given the tokenId.

#### Parameters

##### tokenId

`string`

#### Returns

`Promise`\<[`TokenInfo`](../interfaces/TokenInfo.md)\>

---

### tokenId()

> **tokenId**(`tokenId`): [`TokenIdEndpoint`](TokenIdEndpoint.md)

Defined in: ChronikClient.ts:377

Create object that allows fetching info about a given token

#### Parameters

##### tokenId

`string`

#### Returns

[`TokenIdEndpoint`](TokenIdEndpoint.md)

---

### tx()

> **tx**(`txid`): `Promise`\<[`Tx`](../interfaces/Tx.md)\>

Defined in: ChronikClient.ts:363

Fetch tx details given the txid.

#### Parameters

##### txid

`string`

#### Returns

`Promise`\<[`Tx`](../interfaces/Tx.md)\>

---

### unconfirmedTxs()

> **unconfirmedTxs**(): `Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

Defined in: ChronikClient.ts:334

Fetch unconfirmed transactions from the mempool.

NB this endpoint is NOT paginated, even though it does return the TxHistoryPage shape

#### Returns

`Promise`\<[`TxHistoryPage`](../interfaces/TxHistoryPage.md)\>

---

### validateRawTx()

> **validateRawTx**(`rawTx`): `Promise`\<[`Tx`](../interfaces/Tx.md)\>

Defined in: ChronikClient.ts:280

Validate a tx by rawtx
This is a sort of preflight check before broadcasting a tx
Allows us to

- check before broadcast if a tx unintentionally burns tokens

#### Parameters

##### rawTx

`string` \| `Uint8Array`\<`ArrayBufferLike`\>

#### Returns

`Promise`\<[`Tx`](../interfaces/Tx.md)\>

---

### ws()

> **ws**(`config`): [`WsEndpoint`](WsEndpoint.md)

Defined in: ChronikClient.ts:411

Open a WebSocket connection to listen for updates.

#### Parameters

##### config

[`WsConfig`](../interfaces/WsConfig.md)

#### Returns

[`WsEndpoint`](WsEndpoint.md)

---

### useStrategy()

> `static` **useStrategy**(`strategy`, `urls`): `Promise`\<`ChronikClient`\>

Defined in: ChronikClient.ts:111

Create Chronik client instance with specified strategy

#### Parameters

##### strategy

[`ConnectionStrategy`](../enumerations/ConnectionStrategy.md)

Connection strategy

##### urls

`string`[]

Array of Chronik URLs

#### Returns

`Promise`\<`ChronikClient`\>

Client instance created with sorted URLs
