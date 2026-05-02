# Copyright (c) 2026 The Bitcoin ABC developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Next.js documentation hub at web/docs.e.cash (same pattern as e.cash.Dockerfile).

FROM node:22-trixie-slim AS builder
WORKDIR /app

RUN npm install -g pnpm

COPY pnpm-workspace.yaml .
COPY pnpm-lock.yaml .
COPY package.json .

COPY modules/ecashaddrjs/package.json ./modules/ecashaddrjs/
COPY modules/chronik-client/package.json ./modules/chronik-client/
COPY web/docs.e.cash/package.json ./web/docs.e.cash/

RUN pnpm fetch --frozen-lockfile

COPY modules/ecashaddrjs/ ./modules/ecashaddrjs/
COPY modules/chronik-client/ ./modules/chronik-client/

RUN pnpm install --frozen-lockfile --offline --filter ecashaddrjs...
RUN pnpm install --frozen-lockfile --offline --filter chronik-client...

RUN pnpm --filter ecashaddrjs run build
RUN pnpm --filter chronik-client run build

RUN pnpm install --frozen-lockfile --offline --filter docs.e.cash...

COPY CMakeLists.txt .
COPY web/docs.e.cash/ ./web/docs.e.cash/

ARG PREVIEW_BUILD=next.config.ts
COPY web/docs.e.cash/$PREVIEW_BUILD web/docs.e.cash/next.config.ts

RUN pnpm --filter docs.e.cash run build

FROM node:22-trixie-slim AS runner
WORKDIR /app

RUN npm install -g pnpm

COPY pnpm-workspace.yaml .
COPY pnpm-lock.yaml .
COPY package.json .

COPY --from=builder /app/web/docs.e.cash/.next ./web/docs.e.cash/.next
# next start loads next.config (e.g. trailingSlash).
COPY --from=builder /app/web/docs.e.cash/next.config.ts ./web/docs.e.cash/next.config.ts
COPY --from=builder /app/web/docs.e.cash/package.json ./web/docs.e.cash/package.json
# RSC / request-time rendering reads Markdown from disk (see src/lib/render-doc.tsx).
COPY --from=builder /app/web/docs.e.cash/content ./web/docs.e.cash/content
COPY --from=builder /app/node_modules ./node_modules
# pnpm keeps package symlinks under the workspace (e.g. next-mdx-remote); root node_modules alone is not enough.
COPY --from=builder /app/web/docs.e.cash/node_modules ./web/docs.e.cash/node_modules

WORKDIR /app/web/docs.e.cash
EXPOSE 3000
CMD ["pnpm", "start"]
