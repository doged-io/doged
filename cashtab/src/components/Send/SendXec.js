// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

import React, { useState, useEffect } from 'react';
import { useLocation } from 'react-router-dom';
import { WalletContext } from 'wallet/context';
import {
    ThemedMailOutlined,
    CashReceivedNotificationIcon,
} from 'components/Common/CustomIcons';
import { CustomCollapseCtn } from 'components/Common/StyledCollapse';
import { Alert, Switch } from 'antd';
import Modal from 'components/Common/Modal';
import PrimaryButton from 'components/Common/PrimaryButton';
import { toSatoshis, toXec } from 'wallet';
import { getMaxSendAmountSatoshis } from 'ecash-coinselect';
import { sumOneToManyXec } from 'utils/cashMethods';
import { Event } from 'components/Common/GoogleAnalytics';
import {
    isValidMultiSendUserInput,
    shouldSendXecBeDisabled,
    parseAddressInput,
    isValidXecSendAmount,
} from 'validation';
import {
    ConvertAmount,
    AlertMsg,
    SidePaddingCtn,
    FormLabel,
    TxLink,
    MsgBytesizeError,
} from 'components/Common/Atoms';
import { getWalletState } from 'utils/cashMethods';
import {
    sendXec,
    getMultisendTargetOutputs,
    ignoreUnspendableUtxos,
} from 'transactions';
import {
    getCashtabMsgTargetOutput,
    getAirdropTargetOutput,
    getCashtabMsgByteCount,
    getOpreturnParamTargetOutput,
} from 'opreturn';
import ApiError from 'components/Common/ApiError';
import { formatFiatBalance, formatBalance } from 'utils/formatting';
import styled from 'styled-components';
import { opReturn as opreturnConfig } from 'config/opreturn';
import { explorer } from 'config/explorer';
import { queryAliasServer } from 'alias';
import { supportedFiatCurrencies } from 'config/cashtabSettings';
import appConfig from 'config/app';
import aliasSettings from 'config/alias';
import { isMobile, getUserLocale } from 'helpers';
import { hasEnoughToken, fiatToSatoshis } from 'wallet';
import { toast } from 'react-toastify';
import {
    InputWithScanner,
    SendXecInput,
    TextArea,
} from 'components/Common/Inputs';

const SwitchContainer = styled.div`
    display: flex;
    align-items: center;
    justify-content: flex-end;
    color: ${props => props.theme.forms.text};
    white-space: nowrap;
    margin: 12px;
`;

const SentLink = styled.a`
    color: ${props => props.theme.walletBackground};
    text-decoration: none;
`;

const AppCreatedTxSummary = styled.div`
    font-size: 24px;
    margin-top: -33px;
    padding: 0;
    color: ${props => props.theme.eCashPurple};
`;

const AliasAddressPreviewLabel = styled.div`
    text-align: center;
    color: ${props => props.theme.forms.text};
    padding-left: 1px;
    white-space: nowrap;
`;

const AmountPreviewCtn = styled.div`
    margin: 12px;
    display: flex;
    flex-direction: column;
    justify-content: center;
`;

const SendInputCtn = styled.div`
    .ant-form-item-explain-error {
        @media (max-width: 300px) {
            font-size: 12px;
        }
    }
`;

const LocaleFormattedValue = styled.div`
    color: ${props => props.theme.contrast};
    font-weight: bold;
    font-size: 1.17em;
    margin-bottom: 0;
`;

const DestinationAddressSingleCtn = styled.div``;
const DestinationAddressMultiCtn = styled.div``;

const ExpandingAddressInputCtn = styled.div`
    min-height: 14rem;
    ${DestinationAddressSingleCtn} {
        overflow: hidden;
        transition: ${props =>
            props.open
                ? 'max-height 200ms ease-in, opacity 200ms ease-out'
                : 'max-height 200ms cubic-bezier(0, 1, 0, 1), opacity 200ms ease-in'};
        max-height: ${props => (props.open ? '0rem' : '12rem')};
        opacity: ${props => (props.open ? 0 : 1)};
    }
    ${DestinationAddressMultiCtn} {
        overflow: hidden;
        transition: ${props =>
            props.open
                ? 'max-height 200ms ease-in, transform 200ms ease-out, opacity 200ms ease-in'
                : 'max-height 200ms cubic-bezier(0, 1, 0, 1), transform 200ms ease-out'};
        max-height: ${props => (props.open ? '13rem' : '0rem')};
        transform: ${props =>
            props.open ? 'translateY(0%)' : 'translateY(100%)'};
        opacity: ${props => (props.open ? 1 : 0)};
    }
`;

const PanelHeaderCtn = styled.div`
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 1rem;
`;

const AmountSetByBip21Alert = styled.span`
    color: ${props => props.theme.eCashPurple};
`;
const OpReturnRawSetByBip21Alert = styled.div`
    color: ${props => props.theme.eCashPurple};
    padding: 12px;
    overflow-wrap: break-word;
`;

const SendXec = () => {
    const ContextValue = React.useContext(WalletContext);
    const location = useLocation();
    const { chaintipBlockheight, fiatPrice, apiError, cashtabState, chronik } =
        ContextValue;
    const { settings, wallets } = cashtabState;
    const wallet = wallets.length > 0 ? wallets[0] : false;
    const walletState = getWalletState(wallet);
    const { balanceSats, nonSlpUtxos, tokens } = walletState;
    // Use spendable utxos instead of all nonSlpUtxos for onMax function
    const spendableUtxos = ignoreUnspendableUtxos(
        nonSlpUtxos,
        chaintipBlockheight,
    );
    const [isOneToManyXECSend, setIsOneToManyXECSend] = useState(false);
    const [opReturnMsg, setOpReturnMsg] = useState(false);

    // Load with QR code open if device is mobile
    const openWithScanner =
        settings && settings.autoCameraOn === true && isMobile(navigator);

    const [formData, setFormData] = useState({
        amount: '',
        address: '',
        multiAddressInput: '',
        airdropTokenId: '',
    });
    const [sendAddressError, setSendAddressError] = useState(false);
    const [multiSendAddressError, setMultiSendAddressError] = useState(false);
    const [sendAmountError, setSendAmountError] = useState(false);
    const [isMsgError, setIsMsgError] = useState(false);
    const [aliasInputAddress, setAliasInputAddress] = useState(false);
    const [selectedCurrency, setSelectedCurrency] = useState(appConfig.ticker);
    const [parsedAddressInput, setParsedAddressInput] = useState(
        parseAddressInput(''),
    );

    // Support cashtab button from web pages
    const [txInfoFromUrl, setTxInfoFromUrl] = useState(false);

    // Show a confirmation modal on transactions created by populating form from web page button
    const [isModalVisible, setIsModalVisible] = useState(false);

    // Airdrop transactions embed the additional tokenId (32 bytes), along with prefix (4 bytes) and two pushdata (2 bytes)
    // hence setting airdrop tx message limit to 38 bytes less than opreturnConfig.cashtabMsgByteLimit
    const pushDataByteCount = 1;
    const prefixByteCount = 4;
    const tokenIdByteCount = 32;
    const localAirdropTxAddedBytes =
        pushDataByteCount +
        tokenIdByteCount +
        pushDataByteCount +
        prefixByteCount; // 38

    const [airdropFlag, setAirdropFlag] = useState(false);

    const userLocale = getUserLocale(navigator);
    const clearInputForms = () => {
        setFormData({
            amount: '',
            address: '',
            multiAddressInput: '',
            airdropTokenId: '',
        });
        setOpReturnMsg(''); // OP_RETURN message has its own state field
        setAliasInputAddress(false); // clear alias address preview
        setParsedAddressInput(parseAddressInput(''));
        // Reset to XEC
        // Note, this ensures we never are in fiat send mode for multi-send
        setSelectedCurrency(appConfig.ticker);
    };

    const checkForConfirmationBeforeSendXec = () => {
        if (settings.sendModal) {
            setIsModalVisible(settings.sendModal);
        } else {
            // if the user does not have the send confirmation enabled in settings then send directly
            send();
        }
    };

    const handleOk = () => {
        setIsModalVisible(false);
        send();
    };

    const handleCancel = () => {
        setIsModalVisible(false);
    };

    useEffect(() => {
        // Manually parse for txInfo object on page load when Send.js is loaded with a query string

        // if this was routed from Wallet screen's Reply to message link then prepopulate the address and value field
        if (location && location.state && location.state.replyAddress) {
            setFormData({
                address: location.state.replyAddress,
                amount: `${toXec(appConfig.dustSats)}`,
            });
        }

        // if this was routed from the Contact List
        if (location && location.state && location.state.contactSend) {
            setFormData({
                address: location.state.contactSend,
            });

            // explicitly trigger the address validation upon navigation from contact list
            handleAddressChange({
                target: {
                    name: 'address',
                    value: location.state.contactSend,
                },
            });
        }

        // if this was routed from the Airdrop screen's Airdrop Calculator then
        // switch to multiple recipient mode and prepopulate the recipients field
        if (
            location &&
            location.state &&
            location.state.airdropRecipients &&
            location.state.airdropTokenId
        ) {
            setIsOneToManyXECSend(true);
            setFormData({
                multiAddressInput: location.state.airdropRecipients,
                airdropTokenId: location.state.airdropTokenId,
            });

            // validate the airdrop outputs from the calculator
            handleMultiAddressChange({
                target: {
                    value: location.state.airdropRecipients,
                },
            });

            setAirdropFlag(true);
        }

        // Do not set txInfo in state if query strings are not present
        if (
            !window.location ||
            !window.location.hash ||
            window.location.hash === '#/send'
        ) {
            return;
        }

        // Get everything after the first ? mark
        const hashRoute = window.location.hash;
        // The "+1" is because we want to also omit the first question mark
        // So we need to slice at 1 character past it
        const txInfoStr = hashRoute.slice(hashRoute.indexOf('?') + 1);
        const txInfo = {};

        // If bip21 is the first param, parse the whole string as a bip21 param string
        const parseAllAsBip21 = txInfoStr.startsWith('bip21');

        if (parseAllAsBip21) {
            // Cashtab requires param string to start with bip21 if this is requesting bip21 validation
            txInfo.bip21 = txInfoStr.slice('bip21='.length);
        } else {
            // Parse for legacy amount and value params
            const legacyParams = new URLSearchParams(txInfoStr);
            // Check for duplicated params
            const duplicatedParams =
                new Set(legacyParams.keys()).size !==
                Array.from(legacyParams.keys()).length;
            if (!duplicatedParams) {
                const supportedLegacyParams = ['address', 'value'];
                // Iterate over
                for (const paramKeyValue of legacyParams) {
                    const paramKey = paramKeyValue[0];
                    if (!supportedLegacyParams.includes(paramKey)) {
                        // ignore unsupported params
                        continue;
                    }
                    txInfo[paramKey] = paramKeyValue[1];
                }
            }
        }
        // Only set txInfoFromUrl if you have valid legacy params or bip21
        let validUrlParams =
            (parseAllAsBip21 && 'bip21' in txInfo) ||
            // Good if we have both address and value
            ('address' in txInfo && 'value' in txInfo) ||
            // Good if we have address and no value
            ('address' in txInfo && !('value' in txInfo));
        // If we 'value' key with no address, no good
        // Note: because only the address and value keys are handled below,
        // it's not an issue if we get all kinds of other garbage params

        if (validUrlParams) {
            // This is a tx request from the URL

            // Save this flag in state var so it can be parsed in useEffect
            txInfo.parseAllAsBip21 = parseAllAsBip21;
            setTxInfoFromUrl(txInfo);
        }
    }, []);

    useEffect(() => {
        if (txInfoFromUrl === false) {
            return;
        }
        if (txInfoFromUrl.parseAllAsBip21) {
            handleAddressChange({
                target: {
                    name: 'address',
                    value: txInfoFromUrl.bip21,
                },
            });
        } else {
            // Enter address into input field and trigger handleAddressChange for validation
            handleAddressChange({
                target: {
                    name: 'address',
                    value: txInfoFromUrl.address,
                },
            });
            if (
                'value' in txInfoFromUrl &&
                !Number.isNaN(parseFloat(txInfoFromUrl.value))
            ) {
                // Only update the amount field if txInfo.value is a good input
                // Sometimes we want this field to be adjusted by the user, e.g. a donation amount

                // Do not populate the field if the value param is not parseable as a number
                // the strings 'undefined' and 'null', which PayButton passes to signify 'no amount', fail this test

                // TODO deprecate this support once PayButton and cashtab-components do not require it
                handleAmountChange({
                    target: {
                        name: 'amount',
                        value: txInfoFromUrl.value,
                    },
                });
            }
        }
        // We re-run this when balanceSats changes because validation of send amounts depends on balanceSats
    }, [txInfoFromUrl, balanceSats]);

    function handleSendXecError(errorObj) {
        let message;
        if (
            errorObj.error &&
            errorObj.error.includes(
                'too-long-mempool-chain, too many unconfirmed ancestors [limit: 50] (code 64)',
            )
        ) {
            message = `The ${appConfig.ticker} you are trying to send has too many unconfirmed ancestors to send (limit 50). Sending will be possible after a block confirmation. Try again in about 10 minutes.`;
        } else {
            message =
                errorObj.message || errorObj.error || JSON.stringify(errorObj);
        }

        toast.error(`${message}`);
    }

    async function send() {
        setFormData({
            ...formData,
        });

        // Initialize targetOutputs for this tx
        let targetOutputs = [];

        // If you have an OP_RETURN output, add it at index 0
        // Aesthetic choice, easier to see when checking on block explorer

        if (airdropFlag) {
            // Airdrop txs require special OP_RETURN handling
            targetOutputs.push(
                getAirdropTargetOutput(
                    formData.airdropTokenId,
                    opReturnMsg ? opReturnMsg : '',
                ),
            );
        } else if (opReturnMsg !== false && opReturnMsg !== '') {
            // If not an airdrop msg, add opReturnMsg as a Cashtab Msg, if it exists
            targetOutputs.push(getCashtabMsgTargetOutput(opReturnMsg));
        } else if (
            'op_return_raw' in parsedAddressInput &&
            parsedAddressInput.op_return_raw.error === false
        ) {
            targetOutputs.push(
                getOpreturnParamTargetOutput(
                    parsedAddressInput.op_return_raw.value,
                ),
            );
        }

        if (isOneToManyXECSend) {
            // Handle XEC send to multiple addresses
            targetOutputs = targetOutputs.concat(
                getMultisendTargetOutputs(formData.multiAddressInput),
            );

            Event('Send.js', 'SendToMany', selectedCurrency);
        } else {
            // Handle XEC send to one address
            let cleanAddress;
            // check state on whether this is an alias or ecash address
            if (aliasInputAddress) {
                cleanAddress = aliasInputAddress;
            } else {
                // Get the non-alias param-free address
                cleanAddress = formData.address.split('?')[0];
            }
            const satoshisToSend =
                selectedCurrency === 'XEC'
                    ? toSatoshis(formData.amount)
                    : fiatToSatoshis(formData.amount, fiatPrice);

            targetOutputs.push({
                address: cleanAddress,
                value: satoshisToSend,
            });

            Event('Send.js', 'Send', selectedCurrency);
        }

        // Send and notify
        try {
            const txObj = await sendXec(
                chronik,
                wallet,
                targetOutputs,
                settings.minFeeSends &&
                    hasEnoughToken(
                        tokens,
                        appConfig.vipSettingsTokenId,
                        appConfig.vipSettingsTokenQty,
                    )
                    ? appConfig.minFee
                    : appConfig.defaultFee,
                chaintipBlockheight,
            );

            toast(
                <SentLink
                    href={`${explorer.blockExplorerUrl}/tx/${txObj.response.txid}`}
                    target="_blank"
                    rel="noopener noreferrer"
                >
                    eCash sent
                </SentLink>,
                {
                    icon: CashReceivedNotificationIcon,
                },
            );

            clearInputForms();
            setAirdropFlag(false);
            if (txInfoFromUrl) {
                // Close window after successful tx
                window.close();
            }
        } catch (err) {
            handleSendXecError(err);
        }
    }

    const handleAddressChange = async e => {
        setAliasInputAddress(false); // clear alias address preview
        const { value, name } = e.target;
        const parsedAddressInput = parseAddressInput(
            value,
            balanceSats,
            userLocale,
        );

        // Set in state as various param outputs determine app rendering
        // For example, a valid amount param should disable user amount input
        setParsedAddressInput(parsedAddressInput);

        const address = parsedAddressInput.address.value;
        let renderedSendToError = parsedAddressInput.address.error;
        if (
            'queryString' in parsedAddressInput &&
            typeof parsedAddressInput.queryString.error === 'string'
        ) {
            // If you have a bad queryString, this should be the rendered error
            renderedSendToError = parsedAddressInput.queryString.error;
        } else if (
            parsedAddressInput.address.isAlias &&
            parsedAddressInput.address.error === false
        ) {
            // If we have a valid alias input, check the server for full validation
            // extract alias without the `.xec`
            const aliasName = address.slice(0, address.length - 4);

            // retrieve the alias details for `aliasName` from alias-server
            let aliasDetails;
            try {
                aliasDetails = await queryAliasServer('alias', aliasName);
                if (!aliasDetails.address) {
                    renderedSendToError =
                        'eCash Alias does not exist or yet to receive 1 confirmation';
                    setAliasInputAddress(false);
                } else {
                    // Valid address response returned
                    setAliasInputAddress(aliasDetails.address);
                }
            } catch (err) {
                setAliasInputAddress(false);
                renderedSendToError =
                    'Error resolving alias at indexer, contact admin.';
            }
        }

        // Handle errors in op_return_raw as an address error if no other error is set
        if (
            renderedSendToError === false &&
            'op_return_raw' in parsedAddressInput &&
            typeof parsedAddressInput.op_return_raw.error === 'string'
        ) {
            renderedSendToError = parsedAddressInput.op_return_raw.error;
        }

        setSendAddressError(renderedSendToError);

        // Set amount if it's in the query string
        if ('amount' in parsedAddressInput) {
            // Set currency to non-fiat
            setSelectedCurrency(appConfig.ticker);

            // Use this object to mimic user input and get validation for the value
            let amountObj = {
                target: {
                    name: 'amount',
                    value: parsedAddressInput.amount.value,
                },
            };
            handleAmountChange(amountObj);
        }

        // Set address field to user input
        setFormData(p => ({
            ...p,
            [name]: value,
        }));
    };

    const handleMultiAddressChange = e => {
        const { value, name } = e.target;
        let errorOrIsValid = isValidMultiSendUserInput(
            value,
            balanceSats,
            userLocale,
        );

        // If you get an error msg, set it. If validation is good, clear error msg.
        setMultiSendAddressError(
            typeof errorOrIsValid === 'string' ? errorOrIsValid : false,
        );

        // Set address field to user input
        setFormData(p => ({
            ...p,
            [name]: value,
        }));
    };

    const handleSelectedCurrencyChange = e => {
        setSelectedCurrency(e.target.value);
        // Clear input field to prevent accidentally sending 1 XEC instead of 1 USD
        setFormData(p => ({
            ...p,
            amount: '',
        }));
    };

    const handleAmountChange = e => {
        const { value, name } = e.target;

        // Validate user input send amount
        const isValidAmountOrErrorMsg = isValidXecSendAmount(
            value,
            balanceSats,
            userLocale,
            selectedCurrency,
            fiatPrice,
        );

        setSendAmountError(
            isValidAmountOrErrorMsg !== true ? isValidAmountOrErrorMsg : false,
        );

        setFormData(p => ({
            ...p,
            [name]: value,
        }));
    };

    const handleMsgChange = e => {
        const { value } = e.target;
        let msgError = false;
        const msgByteSize = getCashtabMsgByteCount(value);

        const maxSize =
            location && location.state && location.state.airdropTokenId
                ? opreturnConfig.cashtabMsgByteLimit - localAirdropTxAddedBytes
                : opreturnConfig.cashtabMsgByteLimit;
        if (msgByteSize > maxSize) {
            msgError = `Message can not exceed ${maxSize} bytes`;
        }
        setIsMsgError(msgError);
        setOpReturnMsg(e.target.value);
    };

    const onMax = () => {
        // Clear amt error
        setSendAmountError(false);

        // Set currency to XEC
        setSelectedCurrency(appConfig.ticker);

        // Account for CashtabMsg if it is set
        const intendedTargetOutputs =
            opReturnMsg !== false && opReturnMsg !== ''
                ? getCashtabMsgTargetOutput(opReturnMsg)
                : [];

        // Get max send amount in satoshis
        let maxSendSatoshis;
        try {
            // An error will be thrown if the wallet has insufficient funds to send more than dust
            maxSendSatoshis = getMaxSendAmountSatoshis(
                spendableUtxos,
                settings.minFeeSends &&
                    hasEnoughToken(
                        tokens,
                        appConfig.vipSettingsTokenId,
                        appConfig.vipSettingsTokenQty,
                    )
                    ? appConfig.minFee
                    : appConfig.defaultFee,
                intendedTargetOutputs,
            );
        } catch (err) {
            // Set to zero. In this case, 0 is the max amount we can send, and we know
            // this will trigger the expected dust validation error
            maxSendSatoshis = 0;
        }

        // Convert to XEC to set in form
        const maxSendXec = toXec(maxSendSatoshis);

        // Update value in the send field
        // Note, if we are updating it to 0, we will get a 'dust' error
        handleAmountChange({
            target: {
                name: 'amount',
                value: maxSendXec,
            },
        });
    };
    // Display price in USD below input field for send amount, if it can be calculated
    let fiatPriceString = '';
    let multiSendTotal =
        typeof formData.multiAddressInput === 'string'
            ? sumOneToManyXec(formData.multiAddressInput.split('\n'))
            : 0;
    if (isNaN(multiSendTotal)) {
        multiSendTotal = 0;
    }
    if (fiatPrice !== null && !isNaN(formData.amount)) {
        if (selectedCurrency === appConfig.ticker) {
            // insert symbol and currency before/after the locale formatted fiat balance
            fiatPriceString = isOneToManyXECSend
                ? `${
                      settings
                          ? `${
                                supportedFiatCurrencies[settings.fiatCurrency]
                                    .symbol
                            } `
                          : '$ '
                  } ${(fiatPrice * multiSendTotal).toLocaleString(userLocale, {
                      minimumFractionDigits: appConfig.cashDecimals,
                      maximumFractionDigits: appConfig.cashDecimals,
                  })} ${
                      settings && settings.fiatCurrency
                          ? settings.fiatCurrency.toUpperCase()
                          : 'USD'
                  }`
                : `${
                      settings
                          ? `${
                                supportedFiatCurrencies[settings.fiatCurrency]
                                    .symbol
                            } `
                          : '$ '
                  } ${(fiatPrice * formData.amount).toLocaleString(userLocale, {
                      minimumFractionDigits: appConfig.cashDecimals,
                      maximumFractionDigits: appConfig.cashDecimals,
                  })} ${
                      settings && settings.fiatCurrency
                          ? settings.fiatCurrency.toUpperCase()
                          : 'USD'
                  }`;
        } else {
            fiatPriceString = `${
                formData.amount !== 0
                    ? formatFiatBalance(
                          toXec(fiatToSatoshis(formData.amount, fiatPrice)),
                          userLocale,
                      )
                    : formatFiatBalance(0, userLocale)
            } ${appConfig.ticker}`;
        }
    }

    const priceApiError = fiatPrice === null && selectedCurrency !== 'XEC';

    const disableSendButton = shouldSendXecBeDisabled(
        formData,
        balanceSats,
        apiError,
        sendAmountError,
        sendAddressError,
        isMsgError,
        priceApiError,
        isOneToManyXECSend,
    );
    return (
        <>
            {isModalVisible && (
                <Modal
                    title="Confirm Send"
                    description={
                        isOneToManyXECSend
                            ? `Send
                                ${multiSendTotal.toLocaleString(userLocale, {
                                    maximumFractionDigits: 2,
                                })} 
                                XEC to multiple recipients?`
                            : `Send ${formData.amount}${' '}
                  ${selectedCurrency} to ${parsedAddressInput.address.value}`
                    }
                    handleOk={handleOk}
                    handleCancel={handleCancel}
                    showCancelButton
                />
            )}
            <SidePaddingCtn data-testid="send-xec-ctn">
                {txInfoFromUrl && (
                    <AppCreatedTxSummary data-testid="app-created-tx">
                        Webapp Tx Request
                    </AppCreatedTxSummary>
                )}

                {!txInfoFromUrl && !('queryString' in parsedAddressInput) && (
                    <SwitchContainer>
                        Multiple Recipients:&nbsp;&nbsp;
                        <Switch
                            data-testid="multiple-recipients-switch"
                            defaultunchecked="true"
                            checked={isOneToManyXECSend}
                            onChange={() => {
                                setIsOneToManyXECSend(!isOneToManyXECSend);
                                // Do not persist multisend input to single send and vice versa
                                clearInputForms();
                            }}
                        />
                    </SwitchContainer>
                )}
                <ExpandingAddressInputCtn open={isOneToManyXECSend}>
                    <SendInputCtn>
                        <DestinationAddressSingleCtn>
                            <InputWithScanner
                                placeholder={
                                    aliasSettings.aliasEnabled
                                        ? `Address or Alias`
                                        : `Address`
                                }
                                name="address"
                                value={formData.address}
                                disabled={txInfoFromUrl !== false}
                                handleInput={handleAddressChange}
                                error={sendAddressError}
                                loadWithScannerOpen={openWithScanner}
                            />
                            <AliasAddressPreviewLabel>
                                <TxLink
                                    key={aliasInputAddress}
                                    data-testid="alias-address-preview"
                                    href={`${explorer.blockExplorerUrl}/address/${aliasInputAddress}`}
                                    target="_blank"
                                    rel="noreferrer"
                                >
                                    {aliasInputAddress &&
                                        `${aliasInputAddress.slice(
                                            0,
                                            10,
                                        )}...${aliasInputAddress.slice(-5)}`}
                                </TxLink>
                            </AliasAddressPreviewLabel>
                            <FormLabel>
                                {'amount' in parsedAddressInput &&
                                    parsedAddressInput.amount.value !==
                                        null && (
                                        <AmountSetByBip21Alert data-testid="bip-alert">
                                            {' '}
                                            (set by BIP21 query string)
                                        </AmountSetByBip21Alert>
                                    )}
                            </FormLabel>
                            <SendXecInput
                                name="amount"
                                value={formData.amount}
                                selectValue={selectedCurrency}
                                selectDisabled={
                                    'amount' in parsedAddressInput ||
                                    txInfoFromUrl
                                }
                                inputDisabled={
                                    priceApiError ||
                                    (txInfoFromUrl !== false &&
                                        'value' in txInfoFromUrl &&
                                        txInfoFromUrl.value !== 'null' &&
                                        txInfoFromUrl.value !== 'undefined') ||
                                    'amount' in parsedAddressInput
                                }
                                fiatCode={settings.fiatCurrency.toUpperCase()}
                                error={sendAmountError}
                                handleInput={handleAmountChange}
                                handleSelect={handleSelectedCurrencyChange}
                                handleOnMax={onMax}
                            />
                        </DestinationAddressSingleCtn>
                        {priceApiError && (
                            <AlertMsg>
                                Error fetching fiat price. Setting send by{' '}
                                {supportedFiatCurrencies[
                                    settings.fiatCurrency
                                ].slug.toUpperCase()}{' '}
                                disabled
                            </AlertMsg>
                        )}
                    </SendInputCtn>

                    <>
                        <DestinationAddressMultiCtn>
                            <TextArea
                                placeholder={`One address & value per line, separated by comma \ne.g. \necash:qpatql05s9jfavnu0tv6lkjjk25n6tmj9gkpyrlwu8,500 \necash:qzvydd4n3lm3xv62cx078nu9rg0e3srmqq0knykfed,700`}
                                name="multiAddressInput"
                                handleInput={e => handleMultiAddressChange(e)}
                                value={formData.multiAddressInput}
                                error={multiSendAddressError}
                            />
                        </DestinationAddressMultiCtn>
                    </>
                    <AmountPreviewCtn>
                        {!priceApiError && (
                            <>
                                {isOneToManyXECSend ? (
                                    <LocaleFormattedValue>
                                        {formatBalance(
                                            multiSendTotal,
                                            userLocale,
                                        ) +
                                            ' ' +
                                            selectedCurrency}
                                    </LocaleFormattedValue>
                                ) : (
                                    <LocaleFormattedValue>
                                        {!isNaN(formData.amount)
                                            ? formatBalance(
                                                  formData.amount,
                                                  userLocale,
                                              ) +
                                              ' ' +
                                              selectedCurrency
                                            : ''}
                                    </LocaleFormattedValue>
                                )}
                                <ConvertAmount>
                                    {fiatPriceString !== '' && '='}{' '}
                                    {fiatPriceString}
                                </ConvertAmount>
                            </>
                        )}
                    </AmountPreviewCtn>
                </ExpandingAddressInputCtn>
                {'op_return_raw' in parsedAddressInput && (
                    <OpReturnRawSetByBip21Alert data-testid="op-return-raw-set-alert">
                        Hex OP_RETURN &quot;
                        {parsedAddressInput.op_return_raw.value}
                        &quot; set by BIP21
                    </OpReturnRawSetByBip21Alert>
                )}
                <PrimaryButton
                    style={{ marginTop: '12px' }}
                    data-testid="send-it"
                    disabled={disableSendButton}
                    onClick={() => {
                        checkForConfirmationBeforeSendXec();
                    }}
                >
                    Send
                </PrimaryButton>
                {!('op_return_raw' in parsedAddressInput) && (
                    <CustomCollapseCtn
                        data-testid="cashtab-msg-collapse"
                        panelHeader={
                            <PanelHeaderCtn>
                                <ThemedMailOutlined /> Message
                            </PanelHeaderCtn>
                        }
                        optionalDefaultActiveKey={
                            location &&
                            location.state &&
                            location.state.replyAddress
                                ? ['1']
                                : ['0']
                        }
                        optionalKey="1"
                    >
                        <Alert
                            style={{
                                marginBottom: '10px',
                            }}
                            description="Please note this message will be public."
                            type="warning"
                            showIcon
                        />
                        <TextArea
                            name="opReturnMsg"
                            placeholder={
                                location &&
                                location.state &&
                                location.state.airdropTokenId
                                    ? `(max ${
                                          opreturnConfig.cashtabMsgByteLimit -
                                          localAirdropTxAddedBytes
                                      } bytes)`
                                    : `(max ${opreturnConfig.cashtabMsgByteLimit} bytes)`
                            }
                            value={opReturnMsg ? opReturnMsg : ''}
                            handleInput={e => handleMsgChange(e)}
                            onKeyDown={e =>
                                e.keyCode == 13 ? e.preventDefault() : ''
                            }
                        />
                        <MsgBytesizeError>
                            {isMsgError ? isMsgError : ''}
                        </MsgBytesizeError>
                    </CustomCollapseCtn>
                )}
                {apiError && <ApiError />}
            </SidePaddingCtn>
        </>
    );
};

export default SendXec;
