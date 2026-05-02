// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { DocsHome } from '@/components/DocsHome';
import { getAllDocRelPaths } from '@/lib/paths';
import { renderMdxSlug } from '@/lib/render-doc';
import type { Metadata } from 'next';
import { notFound } from 'next/navigation';

type Props = {
    params: Promise<{ slug?: string[] }>;
};

export async function generateStaticParams() {
    const paths = await getAllDocRelPaths();
    const fromFiles = paths.map(p => ({
        slug: p.split('/').filter(Boolean),
    }));
    return [{ slug: [] }, ...fromFiles];
}

export async function generateMetadata({ params }: Props): Promise<Metadata> {
    const { slug } = await params;
    const slugParts = slug ?? [];
    if (slugParts.length === 0) {
        return { title: 'Documentation hub' };
    }
    const doc = await renderMdxSlug(slugParts);
    if (!doc?.frontmatter.title) {
        return {};
    }
    return { title: doc.frontmatter.title };
}

export default async function DocPage({ params }: Props) {
    const { slug } = await params;
    const slugParts = slug ?? [];

    if (slugParts.length === 0) {
        return <DocsHome />;
    }

    const doc = await renderMdxSlug(slugParts);
    if (!doc) {
        notFound();
    }
    return <article className="prose mx-auto max-w-3xl">{doc.content}</article>;
}
