/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UNDOSENDCOMBOBOX_H
#define UNDOSENDCOMBOBOX_H

#include <QComboBox>
#include "kmail_private_export.h"

class KMAILTESTS_TESTS_EXPORT UndoSendCombobox : public QComboBox
{
    Q_OBJECT
public:
    explicit UndoSendCombobox(QWidget *parent = nullptr);
    ~UndoSendCombobox();

    Q_REQUIRED_RESULT int delay() const;
    void setDelay(int val);
private:
    void initialize();
};

#endif // UNDOSENDCOMBOBOX_H
