// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import styled from 'styled-components';

export const ContactsContainer = styled.div`
    width: 100%;
    @media (max-width: 768px) {
        padding: 0 10px;
    }
`;

export const ContactList = styled.div`
    margin-top: 10px;
    display: flex;
    flex-direction: column;
    width: 100%;
    align-items: center;
    gap: 12px;
    color: ${props => props.theme.primaryText};
    box-sizing: border-box;
    *,
    *:before,
    *:after {
        box-sizing: inherit;
    }
`;

export const ContactsPanel = styled.div`
    display: flex;
    flex-direction: column;
    width: 100%;
    margin-bottom: 12px;
    background-color: ${props => props.theme.inputBackground};
    border-radius: 8px;
    padding: 0 10px;
`;

export const Row = styled.div`
    display: flex;
    width: 100%;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    flex-wrap: wrap;
    padding: 16px;
    border-bottom: 1px solid ${props => props.theme.primaryBackground};
    &:last-child {
        border-bottom: none;
    }
`;
export const ButtonRow = styled.div`
    display: flex;
    width: 100%;
    align-items: center;
    gap: 12px;
`;

export const ContactListName = styled.div`
    display: flex;
    text-align: left;
    word-break: break-word;
`;

export const ButtonPanel = styled.div`
    display: flex;
    gap: 9px;
    align-items: baseline;
`;
