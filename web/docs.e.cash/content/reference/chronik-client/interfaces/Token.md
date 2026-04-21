[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / Token

# Interface: Token

Defined in: ChronikClient.ts:2200

Token coloring an input or output

## Properties

### atoms

> **atoms**: `bigint`

Defined in: ChronikClient.ts:2212

Amount in atoms (aka base tokens) of the input/output

---

### entryIdx?

> `optional` **entryIdx?**: `number`

Defined in: ChronikClient.ts:2210

Index into `token_entries` for `Tx`
chronik returns -1 for UTXOs, chronik-client
passes no entryIdx key for UTXOS

---

### isMintBaton

> **isMintBaton**: `boolean`

Defined in: ChronikClient.ts:2214

Whether the token is a mint baton

---

### tokenId

> **tokenId**: `string`

Defined in: ChronikClient.ts:2202

Hex token_id of the token, see `TokenInfo` for details

---

### tokenType

> **tokenType**: [`TokenType`](../type-aliases/TokenType.md)

Defined in: ChronikClient.ts:2204

Token type of the token
