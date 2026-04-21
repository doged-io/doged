[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / Utxo

# Interface: Utxo

Defined in: ChronikClient.ts:2179

An unspent transaction output (aka. UTXO, aka. "Coin") with script attached
Useful when getting utxos by something other than script, e.g. tokenId

## Properties

### blockHeight

> **blockHeight**: `number`

Defined in: ChronikClient.ts:2183

Which block this UTXO is in, or -1 if in the mempool.

---

### isCoinbase

> **isCoinbase**: `boolean`

Defined in: ChronikClient.ts:2186

Whether this UTXO is a coinbase UTXO
(make sure it's buried 100 blocks before spending!)

---

### isFinal

> **isFinal**: `boolean`

Defined in: ChronikClient.ts:2192

Is this utxo avalanche finalized

---

### outpoint

> **outpoint**: [`OutPoint`](OutPoint.md)

Defined in: ChronikClient.ts:2181

Outpoint of the UTXO.

---

### plugins?

> `optional` **plugins?**: [`PluginEntries`](../type-aliases/PluginEntries.md)

Defined in: ChronikClient.ts:2196

Plugin data attached to this output

---

### sats

> **sats**: `bigint`

Defined in: ChronikClient.ts:2188

Value of the UTXO in satoshis.

---

### script

> **script**: `string`

Defined in: ChronikClient.ts:2190

Bytecode of the script of the output

---

### token?

> `optional` **token?**: [`Token`](Token.md)

Defined in: ChronikClient.ts:2194

Token value attached to this utxo
