[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / MsgTxClient

# Interface: MsgTxClient

Defined in: ChronikClient.ts:2273

Tx got added to/removed from mempool, or confirmed in a block, etc.

## Properties

### finalizationReasonType?

> `optional` **finalizationReasonType?**: [`TxFinalizationReasonType`](../type-aliases/TxFinalizationReasonType.md)

Defined in: ChronikClient.ts:2280

If the tx is finalized, why it was finalized

---

### msgType

> **msgType**: [`TxMsgType`](../type-aliases/TxMsgType.md)

Defined in: ChronikClient.ts:2276

What happened to the tx

---

### txid

> **txid**: `string`

Defined in: ChronikClient.ts:2278

Txid of the tx (human-readable big-endian)

---

### type

> **type**: `"Tx"`

Defined in: ChronikClient.ts:2274
