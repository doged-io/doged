[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / WsEndpoint

# Class: WsEndpoint

Defined in: ChronikClient.ts:801

WebSocket connection to Chronik.

## Constructors

### Constructor

> **new WsEndpoint**(`proxyInterface`, `config`): `WsEndpoint`

Defined in: ChronikClient.ts:832

#### Parameters

##### proxyInterface

`FailoverProxy`

##### config

[`WsConfig`](../interfaces/WsConfig.md)

#### Returns

`WsEndpoint`

## Properties

### autoReconnect

> **autoReconnect**: `boolean`

Defined in: ChronikClient.ts:825

Whether to automatically reconnect on disconnect, default true.

---

### connected

> **connected**: `Promise`\<`Event`\> \| `undefined`

Defined in: ChronikClient.ts:828

---

### manuallyClosed

> **manuallyClosed**: `boolean`

Defined in: ChronikClient.ts:829

---

### onConnect?

> `optional` **onConnect?**: (`e`) => `void`

Defined in: ChronikClient.ts:807

Fired when a connection has been (re)established.

#### Parameters

##### e

`Event`

#### Returns

`void`

---

### onEnd?

> `optional` **onEnd?**: (`e`) => `void`

Defined in: ChronikClient.ts:822

Fired after a connection has been manually closed, or if `autoReconnect`
is false, if the WebSocket disconnects for any reason.

#### Parameters

##### e

`Event`

#### Returns

`void`

---

### onError?

> `optional` **onError?**: (`e`) => `void`

Defined in: ChronikClient.ts:816

Fired when an error with the WebSocket occurs.

#### Parameters

##### e

`ErrorEvent`

#### Returns

`void`

---

### onMessage?

> `optional` **onMessage?**: (`msg`) => `void`

Defined in: ChronikClient.ts:804

Fired when a message is sent from the WebSocket.

#### Parameters

##### msg

[`WsMsgClient`](../type-aliases/WsMsgClient.md)

#### Returns

`void`

---

### onReconnect?

> `optional` **onReconnect?**: (`e`) => `void`

Defined in: ChronikClient.ts:813

Fired after a connection has been unexpectedly closed, and before a
reconnection attempt is made. Only fired if `autoReconnect` is true.

#### Parameters

##### e

`Event`

#### Returns

`void`

---

### subs

> **subs**: `WsSubscriptions`

Defined in: ChronikClient.ts:830

---

### ws

> **ws**: `WebSocket` \| `undefined`

Defined in: ChronikClient.ts:827

## Methods

### close()

> **close**(): `void`

Defined in: ChronikClient.ts:1122

Close the WebSocket connection and prevent any future reconnection
attempts.

#### Returns

`void`

---

### handleMsg()

> **handleMsg**(`wsMsg`): `Promise`\<`void`\>

Defined in: ChronikClient.ts:1276

#### Parameters

##### wsMsg

`MessageEvent`

#### Returns

`Promise`\<`void`\>

---

### pause()

> **pause**(): `void`

Defined in: ChronikClient.ts:1136

Pause the WebSocket connection by disabling auto-reconnect and closing
the connection. Useful when the app is backgrounded to save resources.

Because we cannot predict the behavior of mobile operating systems handling
websocket connections, it is better for the app developer to manually handle.

We provide standard methods to accomplish this.

#### Returns

`void`

---

### resume()

> **resume**(): `Promise`\<`void`\>

Defined in: ChronikClient.ts:1151

Resume the WebSocket connection by re-enabling auto-reconnect and
reconnecting if the connection is closed. Useful when the app comes
to foreground.

#### Returns

`Promise`\<`void`\>

---

### subscribeToAddress()

> **subscribeToAddress**(`address`): `void`

Defined in: ChronikClient.ts:954

Subscribe to an address
Method can be used for p2pkh or p2sh addresses

#### Parameters

##### address

`string`

#### Returns

`void`

---

### subscribeToBlocks()

> **subscribeToBlocks**(): `void`

Defined in: ChronikClient.ts:861

Subscribe to block messages

#### Returns

`void`

---

### subscribeToLokadId()

> **subscribeToLokadId**(`lokadId`): `void`

Defined in: ChronikClient.ts:972

Subscribe to a lokadId

#### Parameters

##### lokadId

`string`

#### Returns

`void`

---

### subscribeToPlugin()

> **subscribeToPlugin**(`pluginName`, `group`): `void`

Defined in: ChronikClient.ts:1040

Subscribe to a plugin

#### Parameters

##### pluginName

`string`

##### group

`string`

#### Returns

`void`

---

### subscribeToScript()

> **subscribeToScript**(`type`, `payload`): `void`

Defined in: ChronikClient.ts:902

Subscribe to the given script type and payload.
For "p2pkh", `scriptPayload` is the 20 byte public key hash.

#### Parameters

##### type

[`ScriptType`](../type-aliases/ScriptType.md)

##### payload

`string`

#### Returns

`void`

---

### subscribeToTokenId()

> **subscribeToTokenId**(`tokenId`): `void`

Defined in: ChronikClient.ts:1006

Subscribe to a tokenId

#### Parameters

##### tokenId

`string`

#### Returns

`void`

---

### subscribeToTxid()

> **subscribeToTxid**(`txid`): `void`

Defined in: ChronikClient.ts:1085

Subscribe to a txid

#### Parameters

##### txid

`string`

#### Returns

`void`

---

### subscribeToTxs()

> **subscribeToTxs**(): `void`

Defined in: ChronikClient.ts:881

Subscribe to all tx messages

#### Returns

`void`

---

### unsubscribeFromAddress()

> **unsubscribeFromAddress**(`address`): `void`

Defined in: ChronikClient.ts:963

Unsubscribe from the given address

#### Parameters

##### address

`string`

#### Returns

`void`

---

### unsubscribeFromBlocks()

> **unsubscribeFromBlocks**(): `void`

Defined in: ChronikClient.ts:871

Unsubscribe from block messages

#### Returns

`void`

---

### unsubscribeFromLokadId()

> **unsubscribeFromLokadId**(`lokadId`): `void`

Defined in: ChronikClient.ts:985

Unsubscribe from the given lokadId

#### Parameters

##### lokadId

`string`

#### Returns

`void`

---

### unsubscribeFromPlugin()

> **unsubscribeFromPlugin**(`pluginName`, `group`): `void`

Defined in: ChronikClient.ts:1059

Unsubscribe from the given plugin

#### Parameters

##### pluginName

`string`

##### group

`string`

#### Returns

`void`

---

### unsubscribeFromScript()

> **unsubscribeFromScript**(`type`, `payload`): `void`

Defined in: ChronikClient.ts:925

Unsubscribe from the given script type and payload.

#### Parameters

##### type

[`ScriptType`](../type-aliases/ScriptType.md)

##### payload

`string`

#### Returns

`void`

---

### unsubscribeFromTokenId()

> **unsubscribeFromTokenId**(`tokenId`): `void`

Defined in: ChronikClient.ts:1019

Unsubscribe from the given tokenId

#### Parameters

##### tokenId

`string`

#### Returns

`void`

---

### unsubscribeFromTxid()

> **unsubscribeFromTxid**(`txid`): `void`

Defined in: ChronikClient.ts:1098

Unsubscribe from the given txid

#### Parameters

##### txid

`string`

#### Returns

`void`

---

### unsubscribeFromTxs()

> **unsubscribeFromTxs**(): `void`

Defined in: ChronikClient.ts:891

Unsubscribe from all tx messages

#### Returns

`void`

---

### waitForOpen()

> **waitForOpen**(): `Promise`\<`void`\>

Defined in: ChronikClient.ts:853

Wait for the WebSocket to be connected.

#### Returns

`Promise`\<`void`\>
