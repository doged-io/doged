#!/usr/bin/env bash
# Copyright (c) 2026 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8
set -euo pipefail

TOPLEVEL=$(git rev-parse --show-toplevel)

pushd "${TOPLEVEL}/modules/ecash-lib-wasm"
./build-wasm.sh
popd

pushd "${TOPLEVEL}"

pnpm install --frozen-lockfile --offline --filter b58-ts...
pnpm install --frozen-lockfile --offline --filter ecashaddrjs...
pnpm install --frozen-lockfile --offline --filter chronik-client...
pnpm install --frozen-lockfile --offline --filter mock-chronik-client...
pnpm install --frozen-lockfile --offline --filter ecash-lib...
pnpm install --frozen-lockfile --offline --filter ecash-wallet...
pnpm install --frozen-lockfile --offline --filter ecash-price...

pnpm --filter b58-ts run build
pnpm --filter ecashaddrjs run build
pnpm --filter chronik-client run build
pnpm --filter mock-chronik-client run build
pnpm --filter ecash-lib run build
pnpm --filter ecash-wallet run build
pnpm --filter ecash-price run build

popd
