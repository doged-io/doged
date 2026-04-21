// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

'use client';

import { chronikLiveScope } from '@/components/chronik-live/chronikLiveScope';
import {
    liveDemoEditorCode,
    type ChronikLiveDemoId,
} from '@/components/chronik-live/liveDemoEditorCode';
import { themes } from 'prism-react-renderer';
import { LiveEditor, LiveError, LivePreview, LiveProvider } from 'react-live';

type Props = {
    id: ChronikLiveDemoId;
};

export function ChronikLivePlayground({ id }: Props) {
    const code = liveDemoEditorCode[id];
    if (!code) {
        return null;
    }

    return (
        <div className="docs-live-breakout not-prose my-6">
            <div className="overflow-hidden rounded-lg border border-borderLight bg-black/20">
                <LiveProvider
                    code={code}
                    scope={chronikLiveScope}
                    noInline={true}
                    theme={themes.nightOwl}
                    language="tsx"
                >
                    <div className="grid min-w-0 grid-cols-1 gap-0 md:grid-cols-2 md:divide-x md:divide-borderLight">
                        <div className="flex min-h-0 min-w-0 flex-col border-b border-borderLight md:border-b-0">
                            <div className="shrink-0 border-b border-borderLight bg-background px-3 py-2 text-xs font-semibold tracking-wide text-secondaryText uppercase">
                                Live editor
                            </div>
                            <div className="docs-live-editor-wrap min-w-0">
                                <LiveEditor
                                    className="min-h-[220px] min-w-0 overflow-auto !bg-[#0d1117] p-3 font-mono text-sm leading-relaxed text-[#e6edf3]"
                                    style={{
                                        fontFamily:
                                            'var(--font-fira-code), monospace',
                                    }}
                                />
                            </div>
                        </div>
                        <div className="flex min-h-0 min-w-0 flex-col">
                            <div className="shrink-0 border-b border-borderLight bg-background px-3 py-2 text-xs font-semibold tracking-wide text-secondaryText uppercase">
                                Result
                            </div>
                            <div className="min-h-[220px] min-w-0 overflow-auto break-words p-4 [overflow-wrap:anywhere]">
                                <LivePreview className="live-preview min-w-0" />
                                <LiveError className="mt-2 rounded border border-red-500/40 bg-red-950/60 p-3 font-mono text-xs whitespace-pre-wrap text-red-200" />
                            </div>
                        </div>
                    </div>
                </LiveProvider>
            </div>
        </div>
    );
}
