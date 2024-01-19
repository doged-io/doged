export default {
    getBalanceSatsVectors: {
        expectedReturns: [
            {
                description: 'Kind of a normal balance calculation',
                nonSlpUtxos: [
                    { value: '546' },
                    { value: '150000000' },
                    { value: '62500000' },
                ],
                balanceSats: 212500546,
            },
            {
                description: 'Wallet balance of total XEC supply',
                nonSlpUtxos: [
                    { value: '700000000000000' },
                    { value: '700000000000000' },
                    { value: '700000000000000' },
                ],
                balanceSats: 2100000000000000,
            },
            {
                description: 'Empty array returns 0 balance',
                nonSlpUtxos: [],
                balanceSats: 0,
            },
            {
                description:
                    'Array containing valid and invalid chronik utxos returns NaN',
                nonSlpUtxos: [
                    { noValueKey: '546' },
                    { value: { thisKeyIsNotAString: 500 } },
                    { value: '62500000' },
                ],
                balanceSats: NaN,
            },
            {
                description:
                    'Array containing invalid chronik utxos returns NaN',
                nonSlpUtxos: [
                    { noValueKey: '546' },
                    { value: { thisKeyIsNotAString: 500 } },
                ],
                balanceSats: NaN,
            },
        ],
        expectedErrors: [
            {
                description: 'Call with non-Array',
                nonSlpUtxos: { somekey: 'an object instead of an array' },
                errorMsg: 'nonSlpUtxos.reduce is not a function',
            },
        ],
    },
    toXec: {
        expectedReturns: [
            {
                description: 'Total XEC supply',
                satoshis: 2100000000000000,
                xec: 21000000000000,
            },
            {
                description: 'Total XEC supply less 1 satoshi',
                satoshis: 2099999999999999,
                xec: 20999999999999.99,
            },
            {
                description: '0 is 0',
                satoshis: 0,
                xec: 0,
            },
        ],
        expectedErrors: [
            {
                description: 'Bad XEC amount, too many decimal places',
                satoshis: 100.123,
                errorMsg: 'Input param satoshis must be an integer',
            },
        ],
    },
    toSatoshis: {
        expectedErrors: [
            {
                description: 'Bad XEC amount, too many decimal places',
                xec: 100.123,
                errorMsg:
                    'Result not an integer. Check input for valid XEC amount.',
            },
        ],
    },
};