[![Dogecash Logo](share/pixmaps/bitcoinabclogo.png "Dogecash")](https://www.doged.io)

**NOTE: We're rebranding `Dogecash` to `doged`!**

doged is a high performance full node implementation for Dogecoin with a
built-in Stratum mining server.

doged is not a new blockchain. It is an alternative node implementation that
is 100% compatible with Dogecoin.

The goal of doged is to create sound money that is usable by everyone in
the world. This is a civilization-changing technology which will dramatically
increase human flourishing, freedom, and prosperity. The project aims to
achieve this goal by implementing a series of optimizations and protocol
upgrades that will enable peer-to-peer digital cash to succeed at mankind scale.

Built-in Stratum Mining Server
------------------------------

doged includes a native Stratum v1 mining server that lets miners connect
directly to the node — no external pool software needed. It plugs into the
node's block assembly pipeline and validates shares using Dogecoin's Scrypt
proof-of-work.

### Quick start

```bash
# Enable stratum when launching the node
./doged -stratum -stratumport=3333 -stratumcoinbase=<your-doge-address>

# Point any Scrypt stratum miner at it
./doged-miner -o stratum+tcp://127.0.0.1:3333 -u worker -p x --cpu 4
```

### Features

- **Stratum v1 protocol** — subscribe, authorize, notify, submit
- **Scrypt PoW** — share validation uses Dogecoin's native Scrypt(1024,1,1)
- **Merge-mining** — `createauxblock` / `submitauxblock` RPCs for AuxPoW
- **Variable difficulty** — automatic per-worker difficulty retargeting
- **Tiered routing** — local node → upstream proxy failover → solo warning
- **Stats endpoint** — live JSON stats at `/stratum/status`

### Configuration flags

| Flag | Description | Default |
|------|-------------|---------|
| `-stratum` | Enable the Stratum server | off |
| `-stratumport=<port>` | Listen port | 3333 |
| `-stratumbind=<addr>` | Bind address | 0.0.0.0 |
| `-stratumcoinbase=<addr>` | Payout address for coinbase | (required) |
| `-stratumdifficulty=<n>` | Initial share difficulty | 1.0 |
| `-stratumvardiff` | Enable variable difficulty | on |
| `-stratumproxy=<spec>` | Upstream pool (`host:port:user[:pass[:priority]]`) | none |
| `-stratumpreferlocal` | Prefer local node over proxy when synced | on |

### Tiered routing

When upstream pools are configured with `-stratumproxy`, the server
automatically picks the best work source:

1. **Local node** (when synced) — assembles blocks directly, no latency
2. **Upstream proxy** — relays jobs from a pool, ordered by priority
3. **Failover** — switches between healthy upstreams if one goes down
4. **Solo warning** — logs a warning when falling back to solo mining

Tier switches are transparent to connected miners; they receive a clean job
broadcast and keep hashing without reconnecting.

doged-miner
-----------

A basic Scrypt GPU/CPU miner ships in `tools/miner/`. It supports OpenCL
for GPU mining and includes a pure-C++ CPU fallback.

```bash
# Build (included in the default cmake build)
ninja doged-miner

# List GPUs
./doged-miner --list-devices

# Mine with GPU
./doged-miner -o stratum+tcp://pool:3333 -u worker -p x --gpu 0

# Mine with CPU (4 threads)
./doged-miner -o stratum+tcp://pool:3333 -u worker -p x --cpu 4
```

What is Dogecoin?
---------------------

[Dogecoin](https://dogecoin.com/) is a digital currency that enables instant payments to
anyone, anywhere in the world. It uses peer-to-peer technology to operate with
no central authority: managing transactions and issuing money are carried out
collectively by the network.

What is Bitcoin ABC?
--------------------

Bitcoin ABC is the name of open-source software which enables the use of
eCash. It is a fork of the [Bitcoin Core](https://bitcoincore.org)
software project. doged is a fork of Bitcoin ABC.

License
-------

doged is released under the terms of the MIT license. See
[COPYING](COPYING) for more information or see
<https://opensource.org/licenses/MIT>.

Disclosure Policy
-----------------

See [DISCLOSURE_POLICY](DISCLOSURE_POLICY.md)
