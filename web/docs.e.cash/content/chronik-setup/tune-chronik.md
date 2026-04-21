---
title: Tune Chronik
---

# Tune Chronik

You can tune Chronik to speed up indexing with the parameters below.

> **Warning**
>
> This is advanced usage — defaults are usually best. Incorrect settings can **slow down** indexing.

## TxNumCache

When resolving tx nums of spent outputs, Chronik uses a `TxNumCache`. By default it uses about 40MB; you can increase usage if you have spare RAM.

- `-chroniktxnumcachebuckets` — number of buckets (default `10`). Too high can slow indexing because buckets are scanned linearly.
- `-chroniktxnumcachebucketsize` — txs per bucket (default `100000`).
