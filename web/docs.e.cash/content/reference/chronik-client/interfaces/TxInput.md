[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / TxInput

# Interface: TxInput

Defined in: ChronikClient.ts:1928

Input of a tx, spends an output of a previous tx.

## Properties

### inputScript

> **inputScript**: `string`

Defined in: ChronikClient.ts:1935

Script unlocking the output, in hex encoding.
Aka. `scriptSig` in bitcoind parlance.

---

### outputScript?

> `optional` **outputScript?**: `string`

Defined in: ChronikClient.ts:1941

Script of the output, in hex encoding.
Aka. `scriptPubKey` in bitcoind parlance.
Not present for coinbase txs

---

### plugins?

> `optional` **plugins?**: [`PluginEntries`](../type-aliases/PluginEntries.md)

Defined in: ChronikClient.ts:1949

Plugin data attached to this input

---

### prevOut

> **prevOut**: [`OutPoint`](OutPoint.md)

Defined in: ChronikClient.ts:1930

Points to an output spent by this input.

---

### sats

> **sats**: `bigint`

Defined in: ChronikClient.ts:1943

Value of the output spent by this input, in satoshis.

---

### sequenceNo

> **sequenceNo**: `number`

Defined in: ChronikClient.ts:1945

`sequence` field of the input; can be used for relative time locking.

---

### token?

> `optional` **token?**: [`Token`](Token.md)

Defined in: ChronikClient.ts:1947

Token value attached to this input
