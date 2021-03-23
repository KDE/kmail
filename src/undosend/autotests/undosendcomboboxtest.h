/*
   SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class UndoSendComboboxTest : public QObject
{
    Q_OBJECT
public:
    explicit UndoSendComboboxTest(QObject *parent = nullptr);
    ~UndoSendComboboxTest() override = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
};

