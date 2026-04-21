[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / WsConfig

# Interface: WsConfig

Defined in: ChronikClient.ts:774

Config for a WebSocket connection to Chronik.

## Properties

### autoReconnect?

> `optional` **autoReconnect?**: `boolean`

Defined in: ChronikClient.ts:797

Whether to automatically reconnect on disconnect, default true.

---

### onConnect?

> `optional` **onConnect?**: (`e`) => `void`

Defined in: ChronikClient.ts:779

Fired when a connection has been (re)established.

#### Parameters

##### e

`Event`

#### Returns

`void`

---

### onEnd?

> `optional` **onEnd?**: (`e`) => `void`

Defined in: ChronikClient.ts:794

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

Defined in: ChronikClient.ts:788

Fired when an error with the WebSocket occurs.

#### Parameters

##### e

`ErrorEvent`

#### Returns

`void`

---

### onMessage?

> `optional` **onMessage?**: (`msg`) => `void`

Defined in: ChronikClient.ts:776

Fired when a message is sent from the WebSocket.

#### Parameters

##### msg

[`WsMsgClient`](../type-aliases/WsMsgClient.md)

#### Returns

`void`

---

### onReconnect?

> `optional` **onReconnect?**: (`e`) => `void`

Defined in: ChronikClient.ts:785

Fired after a connection has been unexpectedly closed, and before a
reconnection attempt is made. Only fired if `autoReconnect` is true.

#### Parameters

##### e

`Event`

#### Returns

`void`
