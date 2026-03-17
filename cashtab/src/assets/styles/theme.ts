// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import { DefaultTheme } from 'styled-components';

export interface CashtabTheme extends DefaultTheme {
    primaryBackground: string;
    secondaryBackground: string;
    txRowBackground: string;
    inputBackground: string;
    accent: string;
    secondaryAccent: string;
    firmaAccent: string;
    primaryText: string;
    secondaryText: string;
    border: string;
    agoraDepthBar: string;
    agoraDepthBarOwnOffer: string;
    agoraDepthBarUnacceptable: string;
    agoraDepthBarUnaffordable: string;
    error: string;
    genesisGreen: string;
    formError: string;
    qrBackground: string;
    backgroundImage: string;
    menuGlow: string;
    buttons: {
        primary: {
            background: string;
            color: string;
            hoverShadow: string;
            borderTop: string;
            borderBottom: string;
        };
        secondary: {
            backgroundImage: string;
            color: string;
        };
        disabled: {
            background: string;
            color: string;
        };
    };
}

export const theme: CashtabTheme = {
    primaryBackground: '#090916',
    secondaryBackground: '#2B2B37',
    txRowBackground: 'rgba(255, 255, 255, 0.07)',
    inputBackground: 'rgba(255, 255, 255, 0.1)',
    accent: '#0074c2',
    secondaryAccent: '#5E0EAE',
    firmaAccent: '#64d6c5',
    primaryText: '#FFFFFF',
    secondaryText: 'rgba(255, 255, 255, 0.5)',
    border: 'rgba(255, 255, 255, 0.15)',
    agoraDepthBar: 'rgba(255, 255, 255, 0.2)',
    agoraDepthBarOwnOffer: 'rgba(179, 33, 144, 0.2)',
    agoraDepthBarUnacceptable: 'rgba(220, 20, 60, .7)',
    agoraDepthBarUnaffordable: 'rgba(255, 165, 0, 0.4)',
    error: '#DC143C',
    genesisGreen: '#00e781',
    formError: '#ff21d0',
    qrBackground: '#fff',
    backgroundImage: `url("/cashtab_bg.png")`,
    menuGlow: '#00ABE740',
    buttons: {
        primary: {
            background:
                'linear-gradient(90deg, #01a0e0 0%, #0671c0 50%, #224da8 100%)',
            color: '#fff',
            hoverShadow: '0 0 8px rgba(255, 255, 255, 0.3)',
            borderTop: 'rgba(255, 255, 255, 0.4)',
            borderBottom: 'rgba(0, 0, 0, 0.5)',
        },
        secondary: {
            backgroundImage:
                'linear-gradient(270deg, #ff21d0 0%, #273498 100%)',
            color: '#fff',
        },
        disabled: {
            background: 'rgba(255, 255, 255, 0.05)',
            color: 'rgba(255, 255, 255, 0.2)',
        },
    },
};
