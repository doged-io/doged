// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Generate Markdown API docs for chronik-client via TypeDoc.
 *
 * Usage:
 *   BITCOIN_ABC_ROOT=/path/to/bitcoin-abc node scripts/generate-chronik-client-api.mjs
 */

import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.join(__dirname, '..');

const abcRoot = process.env.BITCOIN_ABC_ROOT;
if (!abcRoot) {
    console.error(
        'Set BITCOIN_ABC_ROOT to your bitcoin-abc checkout, e.g.\n' +
            '  BITCOIN_ABC_ROOT=/path/to/bitcoin-abc node scripts/generate-chronik-client-api.mjs',
    );
    process.exit(1);
}

const entry = path.join(abcRoot, 'modules', 'chronik-client', 'index.ts');
if (!fs.existsSync(entry)) {
    console.error(`Missing chronik-client entry: ${entry}`);
    process.exit(1);
}

const outDir = path.join(repoRoot, 'content', 'reference', 'chronik-client');
const handWrittenIndex = path.join(outDir, 'index.md');
const savedIndex = fs.existsSync(handWrittenIndex)
    ? fs.readFileSync(handWrittenIndex, 'utf8')
    : null;

fs.rmSync(outDir, { recursive: true, force: true });
fs.mkdirSync(outDir, { recursive: true });

const cfgPath = path.join(repoRoot, 'typedoc.chronik-client.json');
const cfg = {
    $schema: 'https://typedoc.org/schema.json',
    entryPoints: [entry],
    tsconfig: path.join(abcRoot, 'modules', 'chronik-client', 'tsconfig.json'),
    plugin: ['typedoc-plugin-markdown'],
    out: outDir,
    readme: 'none',
    hideGenerator: true,
};
fs.writeFileSync(cfgPath, JSON.stringify(cfg, null, 2));

try {
    execSync(`npx typedoc --options "${cfgPath}"`, {
        stdio: 'inherit',
        cwd: repoRoot,
    });
} finally {
    fs.rmSync(cfgPath, { force: true });
}

if (savedIndex) {
    fs.writeFileSync(handWrittenIndex, savedIndex);
}

console.log(`Wrote TypeDoc Markdown to ${outDir}`);
