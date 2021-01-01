/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SENDLATERCONFIGTEST_H
#define SENDLATERCONFIGTEST_H

#include <QObject>
#include <KSharedConfig>
#include <QRegularExpression>

class SendLaterConfigTest : public QObject
{
    Q_OBJECT
public:
    explicit SendLaterConfigTest(QObject *parent = nullptr);
    ~SendLaterConfigTest();

private Q_SLOTS:
    void init();
    void cleanup();
    void cleanupTestCase();
    void shouldConfigBeEmpty();
    void shouldAddAnItem();
    void shouldNotAddInvalidItem();
private:
    KSharedConfig::Ptr mConfig;
    QRegularExpression mSendlaterRegExpFilter;
};

#endif // SENDLATERCONFIGTEST_H
