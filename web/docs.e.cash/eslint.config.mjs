// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { createRequire } from 'module';

const require = createRequire(import.meta.url);

/** @type {import('eslint').Linter.Config[]} */
export default require('eslint-config-next');
