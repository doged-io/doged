[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / OutPoint

# Interface: OutPoint

Defined in: ChronikClient.ts:1881

Outpoint referencing an output on the blockchain (or input for field
`spentBy`).

## Properties

### outIdx

> **outIdx**: `number`

Defined in: ChronikClient.ts:1888

Index of the output in the tx referenced by this outpoint
(or input index if used in field `spentBy`).

---

### txid

> **txid**: `string`

Defined in: ChronikClient.ts:1883

Transaction referenced by this outpoint.
