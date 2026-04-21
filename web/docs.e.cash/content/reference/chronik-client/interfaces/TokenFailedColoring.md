[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / TokenFailedColoring

# Interface: TokenFailedColoring

Defined in: ChronikClient.ts:2114

A report of a failed coloring attempt of SLP/ALP.
This should always indicate something went wrong when building the tx.

## Properties

### error

> **error**: `string`

Defined in: ChronikClient.ts:2118

Human-readable message of what went wrong

---

### pushdataIdx

> **pushdataIdx**: `number`

Defined in: ChronikClient.ts:2116

For ALP, the index of the pushdata in the OP_RETURN that failed parsing.
