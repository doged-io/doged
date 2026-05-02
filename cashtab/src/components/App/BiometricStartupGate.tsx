// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import React, { useCallback, useEffect, useRef, useState } from 'react';
import styled from 'styled-components';
import { App as CapacitorApp } from '@capacitor/app';
import {
    authenticateIfBiometricLockEnabled,
    isNativeMobilePlatform,
} from 'services/biometricLockService';
import Spinner from 'components/Common/Spinner';

const GateOverlay = styled.div`
    position: fixed;
    inset: 0;
    z-index: 100000;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
    background: linear-gradient(
        -165deg,
        #1e0b3b 0%,
        ${props => props.theme.primaryBackground} 50%,
        ${props => props.theme.primaryBackground} 100%
    );
`;

const GateContent = styled.div`
    max-width: 360px;
`;

const GateTitle = styled.h1`
    margin: 0 0 8px;
    color: ${props => props.theme.primaryText};
    font-size: var(--text-2xl);
`;

const GateText = styled.p`
    margin: 0 0 20px;
    color: ${props => props.theme.secondaryText};
`;

const ErrorText = styled.p`
    margin: 0 0 16px;
    color: #ff8f8f;
`;

const RetryButton = styled.button`
    width: 100%;
    border: none;
    border-radius: 8px;
    padding: 12px 16px;
    color: ${props => props.theme.buttons.primary.color};
    background: ${props => props.theme.buttons.primary.background};
    cursor: pointer;
    :disabled {
        color: ${props => props.theme.buttons.disabled.color};
        background: ${props => props.theme.buttons.disabled.background};
        cursor: not-allowed;
    }
`;

interface BiometricStartupGateProps {
    children: React.ReactNode;
    isCashtabLoaded: boolean;
    biometricLockEnabled: boolean;
}

const BiometricStartupGate: React.FC<BiometricStartupGateProps> = ({
    children,
    isCashtabLoaded,
    biometricLockEnabled,
}) => {
    const [sessionUnlocked, setSessionUnlocked] = useState(false);
    const [isAuthenticating, setIsAuthenticating] = useState(false);
    const [authError, setAuthError] = useState<string | null>(null);
    const [isAppActive, setIsAppActive] = useState(true);

    const biometricLockRef = useRef(biometricLockEnabled);
    const cashtabLoadedRef = useRef(isCashtabLoaded);
    biometricLockRef.current = biometricLockEnabled;
    cashtabLoadedRef.current = isCashtabLoaded;

    const gateEnabled =
        isCashtabLoaded && biometricLockEnabled && isNativeMobilePlatform();

    useEffect(() => {
        if (!isNativeMobilePlatform()) {
            return;
        }

        let removeListener: undefined | (() => void);

        const setupListener = async () => {
            const handle = await CapacitorApp.addListener(
                'appStateChange',
                ({ isActive }) => {
                    setIsAppActive(isActive);
                    if (
                        !isActive &&
                        cashtabLoadedRef.current &&
                        biometricLockRef.current
                    ) {
                        setSessionUnlocked(false);
                    }
                },
            );
            removeListener = () => handle.remove();
        };

        void setupListener();

        return () => {
            removeListener?.();
        };
    }, []);

    const runUnlock = useCallback(async () => {
        setAuthError(null);
        setIsAuthenticating(true);
        try {
            const result = await authenticateIfBiometricLockEnabled(
                biometricLockEnabled,
                'Unlock Cashtab to continue',
            );
            if (result.ok) {
                setSessionUnlocked(true);
            } else if (!result.cancelled && result.message) {
                setAuthError(result.message);
            }
        } finally {
            setIsAuthenticating(false);
        }
    }, [biometricLockEnabled]);

    useEffect(() => {
        if (!gateEnabled) {
            // While Cashtab is still loading, `gateEnabled` is false even if biometric
            // lock will apply once loaded. Do not set sessionUnlocked here — that would
            // skip the cold-start prompt after load (unbackgrounding still worked because
            // we reset sessionUnlocked on background).
            if (isCashtabLoaded) {
                setSessionUnlocked(true);
            }
            return;
        }
        if (!isAppActive) {
            return;
        }
        if (sessionUnlocked || isAuthenticating) {
            return;
        }
        void runUnlock();
    }, [
        gateEnabled,
        isCashtabLoaded,
        sessionUnlocked,
        isAuthenticating,
        isAppActive,
        runUnlock,
    ]);

    if (!gateEnabled || sessionUnlocked) {
        return <>{children}</>;
    }

    return (
        <>
            {children}
            <GateOverlay>
                <GateContent>
                    <GateTitle>Authentication required</GateTitle>
                    <GateText>
                        Use your device biometrics to open Cashtab.
                    </GateText>
                    {isAuthenticating && !authError && <Spinner />}
                    {authError && <ErrorText>{authError}</ErrorText>}
                    <RetryButton
                        disabled={isAuthenticating}
                        onClick={() => {
                            void runUnlock();
                        }}
                    >
                        {isAuthenticating ? 'Checking...' : 'Try again'}
                    </RetryButton>
                </GateContent>
            </GateOverlay>
        </>
    );
};

export default BiometricStartupGate;
