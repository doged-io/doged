// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { Capacitor } from '@capacitor/core';
import {
    BiometricAuth,
    BiometryError,
    BiometryErrorType,
} from '@aparajita/capacitor-biometric-auth';
import { toast } from 'react-toastify';
import type CashtabSettings from 'config/CashtabSettings';

interface BiometricAuthResult {
    ok: true;
}

interface BiometricAuthErrorResult {
    ok: false;
    cancelled: boolean;
    message?: string;
}

type MaybeBiometricAuthResult = BiometricAuthResult | BiometricAuthErrorResult;

/** Neutral Android BiometricPrompt title; plugin default labels "fingerprint" even with face unlock. */
const authenticateOptions = {
    allowDeviceCredential: true,
    androidTitle: 'Biometric' as const,
} as const;

export const isNativeMobilePlatform = (): boolean => {
    if (!Capacitor.isNativePlatform()) {
        return false;
    }
    const platform = Capacitor.getPlatform();
    return platform === 'android' || platform === 'ios';
};

const parseBiometricError = (error: unknown): BiometricAuthErrorResult => {
    if (error instanceof BiometryError) {
        const biometryError = error as unknown as {
            code?: BiometryErrorType | string;
            message?: string;
        };
        const cancelled =
            biometryError.code === BiometryErrorType.userCancel ||
            biometryError.code === BiometryErrorType.appCancel;
        return { ok: false, cancelled, message: biometryError.message };
    }
    const code =
        error && typeof error === 'object' && 'code' in error
            ? String((error as { code: string }).code)
            : '';
    const cancelled = code === 'userCancel' || code === 'appCancel';
    return {
        ok: false,
        cancelled,
        message: error instanceof Error ? error.message : String(error),
    };
};

export const authenticateIfBiometricLockEnabled = async (
    biometricLockEnabled: boolean,
    reason: string,
): Promise<MaybeBiometricAuthResult> => {
    if (!biometricLockEnabled || !isNativeMobilePlatform()) {
        return { ok: true };
    }
    try {
        await BiometricAuth.checkBiometry();
        await BiometricAuth.authenticate({
            ...authenticateOptions,
            reason,
        });
        return { ok: true };
    } catch (error) {
        return parseBiometricError(error);
    }
};

/** Result for gating broadcasts: proceed only when `{ allow: true }`. */
export type BroadcastBiometricGateResult =
    | { allow: true }
    | { allow: false; cancelled: boolean; message?: string };

/**
 * Prompt for biometric / device credential before broadcasting a tx (native mobile only).
 */
export const biometricBroadcastGate = async (
    biometricLockEnabled: boolean,
    reason: string,
): Promise<BroadcastBiometricGateResult> => {
    const r = await authenticateIfBiometricLockEnabled(
        biometricLockEnabled,
        reason,
    );
    if (r.ok) {
        return { allow: true };
    }
    return { allow: false, cancelled: r.cancelled, message: r.message };
};

/**
 * Runs {@link biometricBroadcastGate}; on failure shows a toast for non-cancel errors.
 * @returns whether the caller may proceed with broadcast
 */
export async function confirmBiometricBroadcast(
    settings: CashtabSettings,
    reason: string,
): Promise<boolean> {
    const gate = await biometricBroadcastGate(
        settings.biometricLockEnabled,
        reason,
    );
    if (gate.allow) {
        return true;
    }
    if (!gate.cancelled && gate.message) {
        toast.error(gate.message);
    }
    return false;
}

export const authenticateToEnableBiometricLock =
    async (): Promise<MaybeBiometricAuthResult> => {
        if (!isNativeMobilePlatform()) {
            return { ok: false, cancelled: false, message: 'Not available' };
        }
        try {
            await BiometricAuth.checkBiometry();
            await BiometricAuth.authenticate({
                ...authenticateOptions,
                reason: 'Confirm to protect Cashtab with device authentication',
            });
            return { ok: true };
        } catch (error) {
            return parseBiometricError(error);
        }
    };
