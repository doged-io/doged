import React from 'react';
import styled from 'styled-components';
import PropTypes from 'prop-types';
import { WalletContext } from 'utils/context';
import OnBoarding from 'components/OnBoarding/OnBoarding';
import { QRCode } from 'components/Common/QRCode';
import { currency } from 'components/Common/Ticker.js';
import { LoadingCtn } from 'components/Common/Atoms';
import BalanceHeader from 'components/Common/BalanceHeader';
import BalanceHeaderFiat from 'components/Common/BalanceHeaderFiat';
import { WalletInfoCtn, ZeroBalanceHeader } from 'components/Common/Atoms';
import WalletLabel from 'components/Common/WalletLabel';
import { getWalletState } from 'utils/cashMethods';

export const ReceiveCtn = styled.div`
    width: 100%;
    h2 {
        color: ${props => props.theme.contrast};
        margin: 0 0 20px;
        margin-top: 10px;
    }
`;

const ReceiveWithWalletPresent = ({
    wallet,
    cashtabSettings,
    balances,
    fiatPrice,
    changeCashtabSettings,
}) => {
    return (
        <ReceiveCtn>
            <WalletInfoCtn>
                <WalletLabel
                    name={wallet.name}
                    cashtabSettings={cashtabSettings}
                    changeCashtabSettings={changeCashtabSettings}
                ></WalletLabel>
                {!balances.totalBalance ? (
                    <ZeroBalanceHeader>
                        You currently have 0 {currency.ticker}
                        <br />
                        Deposit some funds to use this feature
                    </ZeroBalanceHeader>
                ) : (
                    <>
                        <BalanceHeader
                            balance={balances.totalBalance}
                            ticker={currency.ticker}
                            cashtabSettings={cashtabSettings}
                        />

                        <BalanceHeaderFiat
                            balance={balances.totalBalance}
                            settings={cashtabSettings}
                            fiatPrice={fiatPrice}
                        />
                    </>
                )}
            </WalletInfoCtn>
            {wallet && ((wallet.Path245 && wallet.Path145) || wallet.Path1899) && (
                <>
                    {wallet.Path1899 ? (
                        <>
                            <QRCode
                                id="borderedQRCode"
                                address={wallet.Path1899.cashAddress}
                            />
                        </>
                    ) : (
                        <>
                            <QRCode
                                id="borderedQRCode"
                                address={wallet.Path245.cashAddress}
                            />
                        </>
                    )}
                </>
            )}
        </ReceiveCtn>
    );
};

const Receive = () => {
    const ContextValue = React.useContext(WalletContext);
    const {
        wallet,
        previousWallet,
        loading,
        cashtabSettings,
        changeCashtabSettings,
        fiatPrice,
    } = ContextValue;
    const walletState = getWalletState(wallet);
    const { balances } = walletState;
    return (
        <>
            {loading ? (
                <LoadingCtn />
            ) : (
                <>
                    {(wallet && wallet.Path1899) ||
                    (previousWallet && previousWallet.path1899) ? (
                        <ReceiveWithWalletPresent
                            wallet={wallet}
                            cashtabSettings={cashtabSettings}
                            balances={balances}
                            fiatPrice={fiatPrice}
                            changeCashtabSettings={changeCashtabSettings}
                        />
                    ) : (
                        <OnBoarding />
                    )}
                </>
            )}
        </>
    );
};

ReceiveWithWalletPresent.propTypes = {
    balances: PropTypes.oneOfType([PropTypes.string, PropTypes.object]),
    fiatPrice: PropTypes.number,
    wallet: PropTypes.object,
    cashtabSettings: PropTypes.oneOfType([
        PropTypes.shape({
            fiatCurrency: PropTypes.string,
            sendModal: PropTypes.bool,
            autoCameraOn: PropTypes.bool,
            hideMessagesFromUnknownSender: PropTypes.bool,
            toggleShowHideBalance: PropTypes.bool,
        }),
        PropTypes.bool,
    ]),
    changeCashtabSettings: PropTypes.func,
};

export default Receive;
