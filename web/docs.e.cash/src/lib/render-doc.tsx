// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import fs from 'fs/promises';
import path from 'path';
import { compileMDX } from 'next-mdx-remote/rsc';
import remarkGfm from 'remark-gfm';
import { getMdxComponents } from '@/lib/mdx-components';

export type DocFrontmatter = {
    title?: string;
    description?: string;
};

async function resolveMarkdownPath(slug: string): Promise<string | null> {
    const contentRoot = path.join(process.cwd(), 'content');
    const candidates = [
        path.join(contentRoot, `${slug}.mdx`),
        path.join(contentRoot, `${slug}.md`),
        path.join(contentRoot, slug, 'index.mdx'),
        path.join(contentRoot, slug, 'index.md'),
    ];
    for (const p of candidates) {
        try {
            await fs.access(p);
            return p;
        } catch {
            /* try next */
        }
    }
    return null;
}

export async function renderMdxSlug(slugParts: string[]) {
    const slug = slugParts.join('/');
    const filePath = await resolveMarkdownPath(slug);
    if (!filePath) {
        return null;
    }
    const source = await fs.readFile(filePath, 'utf8');
    const { content, frontmatter } = await compileMDX<
        DocFrontmatter | Record<string, unknown>
    >({
        source,
        components: getMdxComponents(),
        options: {
            parseFrontmatter: true,
            mdxOptions: {
                remarkPlugins: [remarkGfm],
            },
        },
    });
    return {
        content,
        frontmatter: frontmatter as DocFrontmatter,
        slug,
    };
}
