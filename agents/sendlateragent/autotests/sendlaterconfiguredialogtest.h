/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SENDLATERCONFIGUREDIALOGTEST_H
#define SENDLATERCONFIGUREDIALOGTEST_H

#include <QObject>

class SendLaterConfigureDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit SendLaterConfigureDialogTest(QObject *parent = nullptr);
    ~SendLaterConfigureDialogTest();
private Q_SLOTS:
    void shouldHaveDefaultValue();
    void initTestCase();
};

#endif // SENDLATERCONFIGUREDIALOGTEST_H
