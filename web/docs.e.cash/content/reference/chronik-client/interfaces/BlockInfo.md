[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / BlockInfo

# Interface: BlockInfo

Defined in: ChronikClient.ts:1814

BlockInfo interface for in-node chronik

## Properties

### blockSize

> **blockSize**: `number`

Defined in: ChronikClient.ts:1831

Block size of this block in bytes (including headers etc.).

---

### hash

> **hash**: `string`

Defined in: ChronikClient.ts:1816

Block hash of the block, in 'human-readable' (big-endian) hex encoding.

---

### height

> **height**: `number`

Defined in: ChronikClient.ts:1820

Height of the block; Genesis block has height 0.

---

### isFinal

> **isFinal**: `boolean`

Defined in: ChronikClient.ts:1829

Is this block avalanche finalized?

---

### nBits

> **nBits**: `number`

Defined in: ChronikClient.ts:1822

nBits field of the block, encodes the target compactly.

---

### numInputs

> **numInputs**: `number`

Defined in: ChronikClient.ts:1835

Total number of tx inputs in block (including coinbase).

---

### numOutputs

> **numOutputs**: `number`

Defined in: ChronikClient.ts:1837

Total number of tx output in block (including coinbase).

---

### numTxs

> **numTxs**: `number`

Defined in: ChronikClient.ts:1833

Number of txs in this block.

---

### prevHash

> **prevHash**: `string`

Defined in: ChronikClient.ts:1818

Block hash of the prev block, in 'human-readable' (big-endian) hex encoding.

---

### sumBurnedSats

> **sumBurnedSats**: `bigint`

Defined in: ChronikClient.ts:1845

Total number of satoshis burned using OP_RETURN.

---

### sumCoinbaseOutputSats

> **sumCoinbaseOutputSats**: `bigint`

Defined in: ChronikClient.ts:1841

Total block reward for this block.

---

### sumInputSats

> **sumInputSats**: `bigint`

Defined in: ChronikClient.ts:1839

Total number of satoshis spent by tx inputs.

---

### sumNormalOutputSats

> **sumNormalOutputSats**: `bigint`

Defined in: ChronikClient.ts:1843

Total number of satoshis in non-coinbase tx outputs.

---

### timestamp

> **timestamp**: `number`

Defined in: ChronikClient.ts:1827

Timestamp of the block. Filled in by the miner,
so might not be 100 % precise.
