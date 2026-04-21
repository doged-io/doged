---
title: Build Chronik
---

# Build Chronik

Build Chronik yourself if we do not ship a binary for your platform or you want a custom build.

## UNIX / Linux

### Rust

Chronik is written in Rust. Install it from [rustup.rs](https://rustup.rs/).

### Build tools & libraries

```bash
sudo apt update
sudo apt install bsdmainutils build-essential cmake libssl-dev libevent-dev lld ninja-build python3 libjemalloc-dev libboost-dev libprotobuf-dev protobuf-compiler
```

### Full build

See additional dependencies in [Bitcoin ABC build-unix.md](https://github.com/Bitcoin-ABC/bitcoin-abc/blob/master/doc/build-unix.md), then:

```bash
mkdir build
cd build
cmake -GNinja .. -DBUILD_CHRONIK=on
ninja
```

### Minimal build

Only what is required to run Chronik:

```bash
mkdir build
cd build
cmake -GNinja .. -DBUILD_CHRONIK=ON -DBUILD_WALLET=OFF -DBUILD_QT=OFF -DBUILD_ZMQ=OFF
ninja
```

## macOS

### Rust

Install from [rustup.rs](https://rustup.rs/).

### Preparation

1. Install Xcode from the App Store.
2. Install command-line tools: `xcode-select --install`
3. Install [Homebrew](https://brew.sh).

### Libraries

```bash
brew install ninja cmake jemalloc boost openssl protobuf
```

### Minimal build

```bash
mkdir build
cd build
cmake -GNinja .. -DBUILD_CHRONIK=ON -DBUILD_WALLET=OFF -DBUILD_QT=OFF -DBUILD_ZMQ=OFF
ninja
```

> **Note**
>
> On some macOS setups RocksDB can cause linker issues. You can try `default-features = true` on the `rocksdb` dependency in `chronik/chronik-db/Cargo.toml`:
>
> ```toml
> rocksdb = { version = "0.21", default-features = true }
> ```
