// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import type { Dirent } from 'fs';
import fs from 'fs/promises';
import path from 'path';

const CONTENT = path.join(process.cwd(), 'content');

export async function walkMarkdownFiles(
    dir: string,
    base: string = '',
): Promise<string[]> {
    const out: string[] = [];
    let entries: Dirent[];
    try {
        entries = await fs.readdir(path.join(dir, base), {
            withFileTypes: true,
        });
    } catch {
        return out;
    }
    for (const e of entries) {
        const rel = base ? `${base}/${e.name}` : e.name;
        if (e.isDirectory()) {
            out.push(...(await walkMarkdownFiles(dir, rel)));
        } else if (/\.(md|mdx)$/i.test(e.name)) {
            out.push(rel);
        }
    }
    return out;
}

function toUrlSlug(rel: string): string {
    const noExt = rel.replace(/\.(md|mdx)$/i, '');
    if (noExt.endsWith('/index')) {
        return noExt.slice(0, -'/index'.length);
    }
    return noExt;
}

export async function getAllDocRelPaths(): Promise<string[]> {
    const all = await walkMarkdownFiles(CONTENT);
    return all.map(toUrlSlug).filter(slug => slug.length > 0);
}
