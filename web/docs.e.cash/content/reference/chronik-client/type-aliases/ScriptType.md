[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / ScriptType

# Type Alias: ScriptType

> **ScriptType** = `"other"` \| `"p2pk"` \| `"p2pkh"` \| `"p2sh"`

Defined in: ChronikClient.ts:2229

Script type queried in the `script` method.

- `other`: Script type not covered by the standard script types; payload is
  the raw hex.
- `p2pk`: Pay-to-Public-Key (`<pk> OP_CHECKSIG`), payload is the hex of the
  pubkey (compressed (33 bytes) or uncompressed (65 bytes)).
- `p2pkh`: Pay-to-Public-Key-Hash
  (`OP_DUP OP_HASH160 <pkh> OP_EQUALVERIFY OP_CHECKSIG`).
  Payload is the 20 byte public key hash.
- `p2sh`: Pay-to-Script-Hash (`OP_HASH160 <sh> OP_EQUAL`).
  Payload is the 20 byte script hash.
