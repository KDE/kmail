/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UNDOSENDCOMBOBOXTEST_H
#define UNDOSENDCOMBOBOXTEST_H

#include <QObject>

class UndoSendComboboxTest : public QObject
{
    Q_OBJECT
public:
    explicit UndoSendComboboxTest(QObject *parent = nullptr);
    ~UndoSendComboboxTest() = default;
private Q_SLOTS:
    void shouldHaveDefaultValues();
};

#endif // UNDOSENDCOMBOBOXTEST_H
