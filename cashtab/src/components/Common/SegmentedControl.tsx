// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import React from 'react';
import styled from 'styled-components';

const Wrapper = styled.div`
    display: inline-flex;
    border-radius: 8px;
    overflow: hidden;
`;

const Segment = styled.button<{ $active: boolean }>`
    padding: 8px 20px;
    min-width: 56px;
    border: none;
    font-size: var(--text-sm);
    color: ${props => props.theme.primaryText};
    background: ${props =>
        props.$active ? props.theme.accent : props.theme.inputBackground};
    cursor: ${props => (props.disabled ? 'not-allowed' : 'pointer')};
    opacity: ${props => (props.disabled ? 0.6 : 1)};
    transition: background-color 0.2s ease;
    :hover:not(:disabled) {
        background: ${props =>
            props.$active ? props.theme.accent : 'rgba(255, 255, 255, 0.15)'};
    }
    :focus-visible {
        outline: 2px solid ${props => props.theme.accent};
        outline-offset: 2px;
    }
`;

export interface SegmentedControlOption {
    value: string;
    label: string;
}

interface SegmentedControlProps {
    'options': SegmentedControlOption[];
    'value': string;
    'onChange': (value: string) => void;
    'disabled'?: boolean;
    'name'?: string;
    'aria-label'?: string;
}

const SegmentedControl: React.FC<SegmentedControlProps> = ({
    options,
    value,
    onChange,
    disabled = false,
    name,
    'aria-label': ariaLabel,
}) => {
    return (
        <Wrapper role="group" aria-label={ariaLabel ?? name}>
            {options.map(opt => (
                <Segment
                    key={opt.value}
                    type="button"
                    $active={value === opt.value}
                    disabled={disabled}
                    onClick={() => onChange(opt.value)}
                    aria-pressed={value === opt.value}
                    aria-label={opt.label}
                >
                    {opt.label}
                </Segment>
            ))}
        </Wrapper>
    );
};

export default SegmentedControl;
