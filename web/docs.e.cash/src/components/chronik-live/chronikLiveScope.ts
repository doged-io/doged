// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { ChronikLiveJson } from '@/components/chronik-live/ChronikLiveJson';
import { ChronikClient } from 'chronik-client';
import React from 'react';

/** Pre-connected client for snippets that use `chronik.block(...)` etc. */
export const chronikSingleton = new ChronikClient(['https://chronik.e.cash']);

/** Passed to react-live `LiveProvider` `scope` (like Docusaurus ReactLiveScope). */
export const chronikLiveScope: Record<string, unknown> = {
    React,
    ChronikClient,
    Json: ChronikLiveJson,
    chronik: chronikSingleton,
    useState: React.useState,
    useEffect: React.useEffect,
    useRef: React.useRef,
    useCallback: React.useCallback,
    Fragment: React.Fragment,
};
