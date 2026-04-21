[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / TokenEntry

# Interface: TokenEntry

Defined in: ChronikClient.ts:1986

Token involved in a transaction

## Properties

### actualBurnAtoms

> **actualBurnAtoms**: `bigint`

Defined in: ChronikClient.ts:2009

Number of actually burned tokens (in atoms, aka base tokens).

---

### burnsMintBatons

> **burnsMintBatons**: `boolean`

Defined in: ChronikClient.ts:2013

Whether any mint batons have been burned of this token

---

### burnSummary

> **burnSummary**: `string`

Defined in: ChronikClient.ts:2005

Human-readable error message of why this entry burned tokens

---

### failedColorings

> **failedColorings**: [`TokenFailedColoring`](TokenFailedColoring.md)[]

Defined in: ChronikClient.ts:2007

Human-readable error messages of why colorings failed

---

### groupTokenId?

> `optional` **groupTokenId?**: `string`

Defined in: ChronikClient.ts:2001

For NFT1 Child tokens: group ID
Unset if the token is not an NFT1 Child token

---

### intentionalBurnAtoms

> **intentionalBurnAtoms**: `bigint`

Defined in: ChronikClient.ts:2011

Burn amount the user explicitly opted into (in atoms, aka base tokens)

---

### isInvalid

> **isInvalid**: `boolean`

Defined in: ChronikClient.ts:2003

Whether the validation rules have been violated for this section

---

### tokenId

> **tokenId**: `string`

Defined in: ChronikClient.ts:1992

Hex token_id (in big-endian, like usually displayed to users) of the token.
This is not `bytes` because SLP and ALP use different endiannes, so to avoid
this we use hex, which conventionally implies big-endian in a bitcoin context.

---

### tokenType

> **tokenType**: [`TokenType`](../type-aliases/TokenType.md)

Defined in: ChronikClient.ts:1994

Token type of the token

---

### txType

> **txType**: [`TokenTxType`](../type-aliases/TokenTxType.md)

Defined in: ChronikClient.ts:1996

Tx type of the token; NONE if there's no section that introduced it (e.g. in an accidental burn)
