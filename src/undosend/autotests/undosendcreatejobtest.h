/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class UndoSendCreateJobTest : public QObject
{
    Q_OBJECT
public:
    explicit UndoSendCreateJobTest(QObject *parent = nullptr);
    ~UndoSendCreateJobTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
};

