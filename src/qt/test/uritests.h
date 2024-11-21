// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DOGECASH_QT_TEST_URITESTS_H
#define DOGECASH_QT_TEST_URITESTS_H

#include <QObject>
#include <QTest>

class URITests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void uriTestsCashAddr();
    void uriTestFormatURI();
};

#endif // DOGECASH_QT_TEST_URITESTS_H
