[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / TxOutput

# Interface: TxOutput

Defined in: ChronikClient.ts:1953

Output of a tx, creates new UTXOs.

## Properties

### outputScript

> **outputScript**: `string`

Defined in: ChronikClient.ts:1960

Script of this output, locking the coins.
Aka. `scriptPubKey` in bitcoind parlance.

---

### plugins?

> `optional` **plugins?**: [`PluginEntries`](../type-aliases/PluginEntries.md)

Defined in: ChronikClient.ts:1969

Plugin data attached to this output

---

### sats

> **sats**: `bigint`

Defined in: ChronikClient.ts:1955

Value of the output, in satoshis.

---

### spentBy?

> `optional` **spentBy?**: [`OutPoint`](OutPoint.md)

Defined in: ChronikClient.ts:1965

Transaction & input index spending this output, if
spent.

---

### token?

> `optional` **token?**: [`Token`](Token.md)

Defined in: ChronikClient.ts:1967

Token value attached to this output
