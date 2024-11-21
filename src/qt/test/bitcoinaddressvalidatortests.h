// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DOGECASH_QT_TEST_BITCOINADDRESSVALIDATORTESTS_H
#define DOGECASH_QT_TEST_BITCOINADDRESSVALIDATORTESTS_H

#include <QObject>
#include <QTest>

class BitcoinAddressValidatorTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void inputTests();
};

#endif // DOGECASH_QT_TEST_BITCOINADDRESSVALIDATORTESTS_H
