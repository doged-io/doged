[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / MsgBlockClient

# Interface: MsgBlockClient

Defined in: ChronikClient.ts:2235

Block got connected, disconnected, finalized, etc.

## Properties

### blockHash

> **blockHash**: `string`

Defined in: ChronikClient.ts:2240

Hash of the block (human-readable big-endian)

---

### blockHeight

> **blockHeight**: `number`

Defined in: ChronikClient.ts:2242

Height of the block

---

### blockTimestamp

> **blockTimestamp**: `number`

Defined in: ChronikClient.ts:2244

Timestamp of the block

---

### coinbaseData?

> `optional` **coinbaseData?**: [`CoinbaseData`](CoinbaseData.md)

Defined in: ChronikClient.ts:2246

Coinbase data of the block

---

### msgType

> **msgType**: [`BlockMsgType`](../type-aliases/BlockMsgType.md)

Defined in: ChronikClient.ts:2238

What happened to the block

---

### type

> **type**: `"Block"`

Defined in: ChronikClient.ts:2236
