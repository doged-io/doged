[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / TokenInfo

# Interface: TokenInfo

Defined in: ChronikClient.ts:2372

Info about a token

## Properties

### block?

> `optional` **block?**: [`BlockMetadata`](BlockMetadata.md)

Defined in: ChronikClient.ts:2384

Block of the GENESIS tx, if it's mined already

---

### genesisInfo

> **genesisInfo**: [`GenesisInfo`](GenesisInfo.md)

Defined in: ChronikClient.ts:2382

Info found in the token's GENESIS tx

---

### timeFirstSeen

> **timeFirstSeen**: `number`

Defined in: ChronikClient.ts:2386

Time the GENESIS tx has first been seen by the indexer

---

### tokenId

> **tokenId**: `string`

Defined in: ChronikClient.ts:2378

Hex token_id (in big-endian, like usually displayed to users) of the token.
This is not `bytes` because SLP and ALP use different endiannnes,
so to avoid this we use hex, which conventionally implies big-endian in a bitcoin context.

---

### tokenType

> **tokenType**: [`TokenType`](../type-aliases/TokenType.md)

Defined in: ChronikClient.ts:2380

Token type of the token
