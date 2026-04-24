// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Writes src/data/bitcoin-abc-version.json at build time.
 *
 * 1. Latest version from GitHub Releases API (matches published assets on
 *    download.bitcoinabc.org). Optionally set GITHUB_TOKEN for higher rate limits.
 * 2. Fallback: monorepo root CMakeLists.txt `project(bitcoin-abc VERSION …)`.
 * 3. Fallback: copy bitcoin-abc-version.sample.json if no version yet and no existing file.
 *
 * The generated JSON is gitignored; the sample seeds offline installs.
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const packageRoot = path.join(__dirname, '..');
const repoRoot = path.join(packageRoot, '..', '..');
const cmakePath = path.join(repoRoot, 'CMakeLists.txt');
const outPath = path.join(packageRoot, 'src', 'data', 'bitcoin-abc-version.json');
const samplePath = path.join(
    packageRoot,
    'src',
    'data',
    'bitcoin-abc-version.sample.json',
);

const RELEASES_URL =
    'https://api.github.com/repos/Bitcoin-ABC/bitcoin-abc/releases?per_page=3';

function parseProjectVersion(content) {
    const m = content.match(
        /project\s*\(\s*bitcoin-abc[\s\S]*?VERSION\s+([\d.]+)/i,
    );
    if (m?.[1]) {
        return m[1].trim();
    }
    return null;
}

/** Extract a release version string suitable for download.bitcoinabc.org paths. */
function versionFromRelease(rel) {
    const name = rel?.name != null ? String(rel.name).trim() : '';
    const tag = rel?.tag_name != null ? String(rel.tag_name).trim() : '';
    for (const raw of [name, tag]) {
        if (!raw) {
            continue;
        }
        const stripped = raw.replace(/^v/i, '');
        const m = stripped.match(/(\d+\.\d+\.\d+(?:\.\d+)?)/);
        if (m) {
            return m[1];
        }
    }
    return null;
}

async function fetchVersionFromGitHub() {
    const headers = {
        Accept: 'application/vnd.github.v3+json',
        'User-Agent': 'bitcoin-abc-docs-sync',
    };
    const token = process.env.GITHUB_TOKEN;
    if (token) {
        headers.Authorization = `Bearer ${token}`;
    }

    const res = await fetch(RELEASES_URL, { headers });
    if (!res.ok) {
        console.warn(
            `sync-abc-version: GitHub API ${res.status} ${res.statusText}`,
        );
        return null;
    }

    const releases = await res.json();
    if (!Array.isArray(releases)) {
        console.warn('sync-abc-version: GitHub API did not return a JSON array');
        return null;
    }

    for (const rel of releases) {
        if (rel.draft) {
            continue;
        }
        const v = versionFromRelease(rel);
        if (v) {
            return v;
        }
    }

    console.warn(
        'sync-abc-version: no parseable non-draft release in first page of results',
    );
    return null;
}

function tryCMakeVersion() {
    try {
        const content = fs.readFileSync(cmakePath, 'utf8');
        return parseProjectVersion(content);
    } catch {
        return null;
    }
}

function copySample() {
    if (!fs.existsSync(samplePath)) {
        console.error(`sync-abc-version: missing ${samplePath}`);
        process.exit(1);
    }
    fs.mkdirSync(path.dirname(outPath), { recursive: true });
    fs.copyFileSync(samplePath, outPath);
    console.warn(`sync-abc-version: copied ${samplePath} -> ${outPath}`);
}

function writeOut(version, source) {
    fs.mkdirSync(path.dirname(outPath), { recursive: true });
    fs.writeFileSync(
        outPath,
        `${JSON.stringify({ version, source }, null, 2)}\n`,
    );
    console.log(`sync-abc-version: wrote ${version} (${source}) -> ${outPath}`);
}

async function main() {
    let version = null;
    let source = null;

    try {
        version = await fetchVersionFromGitHub();
        if (version) {
            source = 'github-releases';
        }
    } catch (err) {
        console.warn(
            `sync-abc-version: GitHub fetch failed: ${err instanceof Error ? err.message : err}`,
        );
    }

    if (!version) {
        version = tryCMakeVersion();
        if (version) {
            source = 'CMakeLists.txt';
        }
    }

    if (!version) {
        console.warn(
            'sync-abc-version: no version from GitHub API or CMakeLists.txt',
        );
        if (!fs.existsSync(outPath)) {
            copySample();
        }
        process.exit(0);
    }

    writeOut(version, source);
}

await main();
