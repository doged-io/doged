// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import styled from 'styled-components';

export const WelcomeCtn = styled.div`
    padding: 30px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    margin: auto;
    width: 100%;
    color: ${props => props.theme.primaryText};
    h2 {
        color: ${props => props.theme.primaryText};
    }
    img {
        width: 80%;
        max-width: 300px;
        margin-bottom: 28px;
        @media (min-width: 769px) {
            margin-bottom: 40px;
        }
    }
    > div {
        width: 100%;
        max-width: 300px;
    }
`;

export const WelcomeText = styled.p`
    width: 100%;
    font-size: var(--text-base);
    line-height: var(--text-base--line-height);
    text-align: center;
    margin-bottom: 20px;
`;

export const WelcomeLink = styled.a`
    text-decoration: underline;
    color: ${props => props.theme.accent};
    :hover {
        color: ${props => props.theme.secondaryAccent} !important;
        text-decoration: underline !important;
    }
`;
