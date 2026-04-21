[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / ScriptUtxo

# Interface: ScriptUtxo

Defined in: ChronikClient.ts:2157

An unspent transaction output (aka. UTXO, aka. "Coin") of a script.

## Properties

### blockHeight

> **blockHeight**: `number`

Defined in: ChronikClient.ts:2161

Which block this UTXO is in, or -1 if in the mempool.

---

### isCoinbase

> **isCoinbase**: `boolean`

Defined in: ChronikClient.ts:2164

Whether this UTXO is a coinbase UTXO
(make sure it's buried 100 blocks before spending!)

---

### isFinal

> **isFinal**: `boolean`

Defined in: ChronikClient.ts:2168

Is this utxo avalanche finalized

---

### outpoint

> **outpoint**: [`OutPoint`](OutPoint.md)

Defined in: ChronikClient.ts:2159

Outpoint of the UTXO.

---

### plugins?

> `optional` **plugins?**: [`PluginEntries`](../type-aliases/PluginEntries.md)

Defined in: ChronikClient.ts:2172

Plugin data attached to this output

---

### sats

> **sats**: `bigint`

Defined in: ChronikClient.ts:2166

Value of the UTXO in satoshis.

---

### token?

> `optional` **token?**: [`Token`](Token.md)

Defined in: ChronikClient.ts:2170

Token value attached to this utxo
