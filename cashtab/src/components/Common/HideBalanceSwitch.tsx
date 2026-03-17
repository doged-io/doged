// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import React from 'react';
import styled from 'styled-components';
import CashtabSettings from 'config/CashtabSettings';
import { ReactComponent as EyeIcon } from 'assets/visible.svg';
import { ReactComponent as EyeInvisibleIcon } from 'assets/hidden.svg';

interface HideBalanceSwitchProps {
    settings: CashtabSettings;
    updateCashtabState: (updates: {
        [key: string]: CashtabSettings;
    }) => Promise<boolean>;
}

const HideBalanceSwitch: React.FC<HideBalanceSwitchProps> = ({
    settings,
    updateCashtabState,
}) => {
    const handleShowHideBalance = (
        e: React.ChangeEvent<HTMLInputElement>,
    ): void => {
        // Update settings in state and localforage
        updateCashtabState({
            settings: {
                ...settings,
                balanceVisible: e.target.checked,
            },
        });
    };

    const SwitchInputWrapper = styled.label`
        height: 100%;
        flex-shrink: 0;
        display: inline-block;
        position: relative;
        cursor: pointer;
    `;

    const HiddenCheckbox = styled.input.attrs({ type: 'checkbox' })`
        display: none;
    `;

    const CustomCheckbox = styled.div<{ checked: boolean }>`
        width: 100%;
        height: 100%;
        color: ${({ checked }) =>
            checked
                ? props => props.theme.primaryText
                : props => props.theme.accent};

        display: flex;
        align-items: center;
        justify-content: center;
        transition: color 0.2s;

        svg {
            width: 25px;
            height: 25px;
        }
        :hover {
            color: ${props => props.theme.accent};
        }
    `;

    return (
        <SwitchInputWrapper>
            <HiddenCheckbox
                checked={settings.balanceVisible}
                onChange={handleShowHideBalance}
            />
            <CustomCheckbox checked={settings.balanceVisible}>
                {settings.balanceVisible ? (
                    <EyeIcon title="Visible" fill="currentColor" />
                ) : (
                    <EyeInvisibleIcon title="Hidden" fill="currentColor" />
                )}
            </CustomCheckbox>
        </SwitchInputWrapper>
    );
};

export default HideBalanceSwitch;
