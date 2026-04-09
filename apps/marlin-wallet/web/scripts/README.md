# Scripts

## `export-chronik-stub.ts`

Downloads Chronik history pages and txs, then writes one JSON file for Cypress (`ChronikStub` — see `cypress/fixture/chronik/chronik-json-protobuf.ts`).

Run from **`apps/marlin-wallet/web`** (needs network):

```bash
pnpm exec tsx scripts/export-chronik-stub.ts <out.json> <ecash:...> [options]
```

- **`<out.json>`** — output path (often under `cypress/fixture/chronik/`).
- **`<ecash:...>`** — cash address to export.

**Options:** `--chronik <url>` (default `https://chronik.e.cash`), `--max-pages <n>` (default `2`), `--page-size <n>` (default `25`).

Example:

```bash
pnpm exec tsx scripts/export-chronik-stub.ts \
  cypress/fixture/chronik/qqa9lv3kjd8vq7952p7rq0f6lkpqvlu0cydvxtd70g.json \
  'ecash:qqa9lv3kjd8vq7952p7rq0f6lkpqvlu0cydvxtd70g' \
  --max-pages 2 \
  --page-size 25
```

Pass `--help`-style usage by running with no args (the script prints usage and exits).
