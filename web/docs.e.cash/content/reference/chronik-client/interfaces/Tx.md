[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / Tx

# Interface: Tx

Defined in: ChronikClient.ts:1892

A transaction on the blockchain or in the mempool.

## Properties

### block?

> `optional` **block?**: [`BlockMetadata`](BlockMetadata.md)

Defined in: ChronikClient.ts:1904

Block data for this tx, if it is in a block.

---

### inputs

> **inputs**: [`TxInput`](TxInput.md)[]

Defined in: ChronikClient.ts:1898

Inputs of this transaction.

---

### isCoinbase

> **isCoinbase**: `boolean`

Defined in: ChronikClient.ts:1913

Whether this tx is a coinbase tx.

---

### isFinal

> **isFinal**: `boolean`

Defined in: ChronikClient.ts:1924

Whether or not the tx is finalized

---

### lockTime

> **lockTime**: `number`

Defined in: ChronikClient.ts:1902

`locktime` field of the transaction, tx is not valid before this time.

---

### outputs

> **outputs**: [`TxOutput`](TxOutput.md)[]

Defined in: ChronikClient.ts:1900

Outputs of this transaction.

---

### size

> **size**: `number`

Defined in: ChronikClient.ts:1911

Serialized size of the tx.

---

### timeFirstSeen

> **timeFirstSeen**: `number`

Defined in: ChronikClient.ts:1909

UNIX timestamp when this tx has first been seen in the mempool.
0 if unknown -> make sure to check.

---

### tokenEntries

> **tokenEntries**: [`TokenEntry`](TokenEntry.md)[]

Defined in: ChronikClient.ts:1915

Tokens involved in this txs

---

### tokenFailedParsings

> **tokenFailedParsings**: [`TokenFailedParsing`](TokenFailedParsing.md)[]

Defined in: ChronikClient.ts:1917

Failed parsing attempts of this tx

---

### tokenStatus

> **tokenStatus**: [`TokenStatus`](../type-aliases/TokenStatus.md)

Defined in: ChronikClient.ts:1922

Token status, i.e. whether this tx has any tokens or unintentional token burns
or something unexpected, like failed parsings etc.

---

### txid

> **txid**: `string`

Defined in: ChronikClient.ts:1894

Transaction ID.

---

### version

> **version**: `number`

Defined in: ChronikClient.ts:1896

`version` field of the transaction.
