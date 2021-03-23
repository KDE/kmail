/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class SendLaterUtilTest : public QObject
{
    Q_OBJECT
public:
    explicit SendLaterUtilTest(QObject *parent = nullptr);

private Q_SLOTS:
    void shouldRestoreFromSettings();
};

