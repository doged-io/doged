// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import 'dotenv/config';
import { Bot, Context } from 'grammy';
import { ChronikClient, ConnectionStrategy } from 'chronik-client';
import { HdNode, mnemonicToSeed } from 'ecash-lib';
import { Wallet } from 'ecash-wallet';
import { initDb } from '../src/db';
import { respawn } from '../src/bot';

/**
 * Run the same respawn flow as /respawn for a given Telegram user ID (admin / ops).
 * Usage: npx tsx scripts/respawnByUserId.ts <telegramUserId>
 *
 * Env (same as the main bot): TELEGRAM_BOT_TOKEN, MOD_MNEMONIC, DATABASE_URL,
 * ADMIN_GROUP_CHAT_ID
 */
const main = async (): Promise<void> => {
    const rawUserId = process.argv[2];
    if (!rawUserId) {
        console.error(
            'Usage: npx tsx scripts/respawnByUserId.ts <telegramUserId>',
        );
        process.exit(1);
    }

    if (!/^\d+$/.test(rawUserId)) {
        throw new Error('telegramUserId must be a non-negative integer');
    }

    const telegramUserId = Number(rawUserId);
    if (!Number.isSafeInteger(telegramUserId)) {
        throw new Error('telegramUserId is outside safe integer range');
    }

    const telegramBotToken = process.env.TELEGRAM_BOT_TOKEN;
    if (!telegramBotToken) {
        throw new Error('TELEGRAM_BOT_TOKEN environment variable is required');
    }

    const modMnemonic = process.env.MOD_MNEMONIC;
    if (!modMnemonic) {
        throw new Error('MOD_MNEMONIC environment variable is required');
    }

    const databaseUrl = process.env.DATABASE_URL;
    if (!databaseUrl) {
        throw new Error('DATABASE_URL environment variable is required');
    }

    const adminGroupChatId = process.env.ADMIN_GROUP_CHAT_ID;
    if (!adminGroupChatId) {
        throw new Error('ADMIN_GROUP_CHAT_ID environment variable is required');
    }

    console.info(
        `Attempting respawn for Telegram user ID ${telegramUserId}...`,
    );

    const pool = await initDb(databaseUrl);
    console.info('Database connected');

    const chronikUrls = [
        'https://chronik-native3.fabien.cash',
        'https://chronik-native2.fabien.cash',
        'https://chronik-native1.fabien.cash',
    ];
    const chronik = await ChronikClient.useStrategy(
        ConnectionStrategy.ClosestFirst,
        chronikUrls,
    );
    console.info('Chronik client initialized');

    const seed = mnemonicToSeed(modMnemonic);
    const master = HdNode.fromSeed(seed);
    const overmindNode = master.derivePath("m/44'/1899'/0'/0/0");
    const modSk = overmindNode.seckey();
    if (!modSk) {
        throw new Error('Failed to derive secret key from mnemonic');
    }
    const wallet = Wallet.fromSk(modSk, chronik);
    console.info(`Wallet initialized: ${wallet.address}`);

    const bot = new Bot(telegramBotToken);

    const scriptCtx = {
        from: { id: telegramUserId },
        reply: async (text: string) => {
            console.info(text);
        },
    } as unknown as Context;

    try {
        await respawn(scriptCtx, pool, chronik, wallet, bot, adminGroupChatId);
    } finally {
        await pool.end();
        console.info('Database connection closed');
    }
};

main().catch(err => {
    console.error('Fatal error:', err);
    process.exit(1);
});
