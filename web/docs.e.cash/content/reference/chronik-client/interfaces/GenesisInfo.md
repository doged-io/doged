[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / GenesisInfo

# Interface: GenesisInfo

Defined in: ChronikClient.ts:2390

Genesis info found in GENESIS txs of tokens

## Properties

### authPubkey?

> `optional` **authPubkey?**: `string`

Defined in: ChronikClient.ts:2404

auth_pubkey of the token (only on ALP)

---

### data?

> `optional` **data?**: `string`

Defined in: ChronikClient.ts:2402

Arbitray payload data of the token (only on ALP)

---

### decimals

> **decimals**: `number`

Defined in: ChronikClient.ts:2406

decimals of the token, i.e. how many decimal places the token should be displayed with.

---

### hash?

> `optional` **hash?**: `string`

Defined in: ChronikClient.ts:2398

token_document_hash of the token (only on SLP)

---

### mintVaultScripthash?

> `optional` **mintVaultScripthash?**: `string`

Defined in: ChronikClient.ts:2400

mint_vault_scripthash (only on SLP V2 Mint Vault)

---

### tokenName

> **tokenName**: `string`

Defined in: ChronikClient.ts:2394

token_name of the token

---

### tokenTicker

> **tokenTicker**: `string`

Defined in: ChronikClient.ts:2392

token_ticker of the token

---

### url

> **url**: `string`

Defined in: ChronikClient.ts:2396

URL of the token
