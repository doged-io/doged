[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / TokenFailedParsing

# Interface: TokenFailedParsing

Defined in: ChronikClient.ts:2126

TokenFailedParsing
A report of a failed parsing attempt of SLP/ALP.
This should always indicate something went wrong when building the tx.

## Properties

### bytes

> **bytes**: `string`

Defined in: ChronikClient.ts:2133

The bytes that failed parsing, useful for debugging

---

### error

> **error**: `string`

Defined in: ChronikClient.ts:2135

Human-readable message of what went wrong

---

### pushdataIdx

> **pushdataIdx**: `number`

Defined in: ChronikClient.ts:2131

For ALP, the index of the pushdata in the OP_RETURN that failed parsing.
-1 if the whole OP_RETURN failed, e.g. for SLP or eMPP
