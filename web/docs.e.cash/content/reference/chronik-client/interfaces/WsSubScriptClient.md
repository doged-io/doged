[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / WsSubScriptClient

# Interface: WsSubScriptClient

Defined in: ChronikClient.ts:2314

## Properties

### payload

> **payload**: `string`

Defined in: ChronikClient.ts:2323

Payload for the given script type:

- 20-byte hash for "p2pkh" and "p2sh"
- 33-byte or 65-byte pubkey for "p2pk"
- Serialized script for "other"

---

### scriptType

> **scriptType**: [`ScriptType`](../type-aliases/ScriptType.md)

Defined in: ChronikClient.ts:2316

Script type to subscribe to ("p2pkh", "p2sh", "p2pk", "other").
