// Copyright (c) 2026 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Default editor source for each demo (react-live `noInline` + `render(...)`).
 * Default demos for react-live (ported from the former Docusaurus `jsx live` examples).
 */
export const liveDemoEditorCode = {
    'connect-blockchainInfo': `function DemoConnect() {
    return (
        <Json
            fn={async () => {
                const chronik = new ChronikClient([
                    'https://chronik.e.cash',
                ]);
                return await chronik.blockchainInfo();
            }}
        />
    );
}
render(<DemoConnect />);`,

    'blockchain-blockchainInfo': `function DemoBlockchainInfo() {
    return (
        <Json
            fn={async () => {
                const chronik = new ChronikClient([
                    'https://chronik.e.cash',
                ]);
                return await chronik.blockchainInfo();
            }}
        />
    );
}
render(<DemoBlockchainInfo />);`,

    'blockchain-chronikInfo': `function DemoChronikInfo() {
    return (
        <Json
            fn={async () => {
                const chronik = new ChronikClient([
                    'https://chronik.e.cash',
                ]);
                return await chronik.chronikInfo();
            }}
        />
    );
}
render(<DemoChronikInfo />);`,

    'blocks-block': `function DemoBlock() {
    return (
        <Json
            fn={async () => {
                return await chronik.block(
                    '000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f',
                );
            }}
        />
    );
}
render(<DemoBlock />);`,

    'blocks-blockTxs': `function DemoBlockTxs() {
    return (
        <Json
            fn={async () => {
                return await chronik.blockTxs(0);
            }}
        />
    );
}
render(<DemoBlockTxs />);`,

    'blocks-range': `function DemoBlocks() {
    return (
        <Json
            fn={async () => {
                return await chronik.blocks(50, 52);
            }}
        />
    );
}
render(<DemoBlocks />);`,

    'txs-tx': `function DemoTx() {
    return (
        <Json
            fn={async () => {
                return await chronik.tx(
                    'cdcdcdcdcdc9dda4c92bb1145aa84945c024346ea66fd4b699e344e45df2e145',
                );
            }}
        />
    );
}
render(<DemoTx />);`,

    'txs-rawTx': `function DemoRawTx() {
    return (
        <Json
            fn={async () => {
                return await chronik.rawTx(
                    '4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b',
                );
            }}
        />
    );
}
render(<DemoRawTx />);`,

    'addr-address': `function DemoAddress() {
    return (
        <Json
            fn={async () => {
                return await chronik
                    .address('ecash:qr3ax9kpca8qkgtwk0cdqdhnprsgfsq3xu2dx56fmw')
                    .history(0);
            }}
        />
    );
}
render(<DemoAddress />);`,

    'addr-script': `function DemoScript() {
    return (
        <Json
            fn={async () => {
                return await chronik
                    .script('p2pkh', 'e3d316c1c74e0b216eb3f0d036f308e084c01137')
                    .history(0);
            }}
        />
    );
}
render(<DemoScript />);`,

    'addr-history': `function DemoAddressHistory() {
    return (
        <Json
            fn={async () => {
                return await chronik
                    .address('ecash:prfhcnyqnl5cgrnmlfmms675w93ld7mvvqd0y8lz07')
                    .history(300, 2);
            }}
        />
    );
}
render(<DemoAddressHistory />);`,

    'addr-utxos': `function DemoScriptUtxos() {
    return (
        <Json
            fn={async () => {
                return await chronik
                    .script(
                        'p2pk',
                        '04678afdb0fe5548271967f1a67130b7105cd6a828e03909a6' +
                            '7962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c' +
                            '384df7ba0b8d578a4c702b6bf11d5f',
                    )
                    .utxos();
            }}
        />
    );
}
render(<DemoScriptUtxos />);`,

    'tokens-token': `function DemoToken() {
    return (
        <Json
            fn={async () => {
                return await chronik.token(
                    'cdcdcdcdcdc9dda4c92bb1145aa84945c024346ea66fd4b699e344e45df2e145',
                );
            }}
        />
    );
}
render(<DemoToken />);`,

    'tokens-tokenId': `function DemoTokenId() {
    return (
        <Json
            fn={async () => {
                return await chronik
                    .tokenId(
                        'cdcdcdcdcdc9dda4c92bb1145aa84945c024346ea66fd4b699e344e45df2e145',
                    )
                    .confirmedTxs(0, 1);
            }}
        />
    );
}
render(<DemoTokenId />);`,

    'tokens-history': `function DemoTokenIdHistory() {
    return (
        <Json
            fn={async () => {
                return await chronik
                    .tokenId(
                        'cdcdcdcdcdc9dda4c92bb1145aa84945c024346ea66fd4b699e344e45df2e145',
                    )
                    .history(0, 1);
            }}
        />
    );
}
render(<DemoTokenIdHistory />);`,

    'tokens-confirmedTxs': `function DemoTokenIdConfirmedTxs() {
    return (
        <Json
            fn={async () => {
                return await chronik
                    .tokenId(
                        'cdcdcdcdcdc9dda4c92bb1145aa84945c024346ea66fd4b699e344e45df2e145',
                    )
                    .confirmedTxs(0, 1);
            }}
        />
    );
}
render(<DemoTokenIdConfirmedTxs />);`,

    'tokens-unconfirmedTxs': `function DemoTokenIdUnconfirmedTxs() {
    return (
        <Json
            fn={async () => {
                return await chronik
                    .tokenId(
                        'fb4233e8a568993976ed38a81c2671587c5ad09552dedefa78760deed6ff87aa',
                    )
                    .unconfirmedTxs();
            }}
        />
    );
}
render(<DemoTokenIdUnconfirmedTxs />);`,

    'tokens-utxos': `function DemoTokenIdUtxos() {
    return (
        <Json
            fn={async () => {
                return (
                    await chronik
                        .tokenId(
                            'cdcdcdcdcdc9dda4c92bb1145aa84945c024346ea66fd4b699e344e45df2e145',
                        )
                        .utxos()
                ).utxos[0];
            }}
        />
    );
}
render(<DemoTokenIdUtxos />);`,

    'broadcast-tx': `function DemoBroadcastTx() {
    return (
        <Json
            fn={async () => {
                return await chronik.broadcastTx(
                    '000000000145e1f25de444e399b6d46fa66e3424c04549a85a14b12bc9a4ddc9' +
                        'cdcdcdcdcd0100000000000000000000000000',
                );
            }}
        />
    );
}
render(<DemoBroadcastTx />);`,

    'broadcast-txs': `function DemoBroadcastTxs() {
    return (
        <Json
            fn={async () => {
                return await chronik.broadcastTxs([
                    '000000000145e1f25de444e399b6d46fa66e3424c04549a85a14b12bc9a4ddc9' +
                        'cdcdcdcdcd0100000000000000000000000000',
                    '010000000145e1f25de444e399b6d46fa66e3424c04549a85a14b12bc9a4ddc9' +
                        'cdcdcdcdcd0100000000000000000000000000',
                ]);
            }}
        />
    );
}
render(<DemoBroadcastTxs />);`,

    'websocket-demo': `function DemoWebSocket() {
    const [logs, setLogs] = useState([]);
    useEffect(() => {
        const addToLog = (msg) => {
            const copy = { ...msg, timestamp: new Date().toISOString() };
            console.log(copy);
            setLogs((prev) => [...prev, JSON.stringify(copy)]);
        };
        const ws = chronik.ws({
            onMessage: addToLog,
            onConnect: addToLog,
            onReconnect: addToLog,
            onError: addToLog,
            onEnd: addToLog,
        });
        ws.subscribeToBlocks();
        ws.subscribeToAddress(
            'ecash:qryzw7gteszy8jgsejjchlwjg7lctxpwjgllx92x9j',
        );
        void ws.waitForOpen();
        return () => {
            ws.close();
        };
    }, []);
    return (
        <div
            style={{
                maxHeight: 384,
                overflowY: 'auto',
                fontFamily: 'var(--font-fira-code), ui-monospace, monospace',
                fontSize: 12,
            }}
        >
            {logs.map((log, idx) => (
                <div
                    key={idx}
                    style={{
                        borderBottom: '1px solid rgba(255,255,255,0.08)',
                        padding: '4px 0',
                    }}
                >
                    {log}
                </div>
            ))}
        </div>
    );
}
render(<DemoWebSocket />);`,
} as const;

export type ChronikLiveDemoId = keyof typeof liveDemoEditorCode;
