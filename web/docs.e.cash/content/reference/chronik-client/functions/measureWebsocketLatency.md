[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / measureWebsocketLatency

# Function: measureWebsocketLatency()

> **measureWebsocketLatency**(`wsUrl`): `Promise`\<`number`\>

Defined in: ChronikClient.ts:33

Measures connection latency to a given WebSocket URL

## Parameters

### wsUrl

`string`

WebSocket URL to test connection

## Returns

`Promise`\<`number`\>

Returns latency in milliseconds, or Infinity if connection times out or fails
