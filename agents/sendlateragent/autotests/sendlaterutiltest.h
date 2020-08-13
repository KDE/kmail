/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SENDLATERUTILTEST_H
#define SENDLATERUTILTEST_H

#include <QObject>

class SendLaterUtilTest : public QObject
{
    Q_OBJECT
public:
    explicit SendLaterUtilTest(QObject *parent = nullptr);

private Q_SLOTS:
    void shouldRestoreFromSettings();
};

#endif // SENDLATERUTILTEST_H
