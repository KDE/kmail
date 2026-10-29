/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <QDateTime>
#include <QDialog>
class QDateTimeEdit;

class KMAILTESTS_TESTS_EXPORT SnoozeSelectDateTimeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SnoozeSelectDateTimeDialog(QWidget *parent = nullptr);
    ~SnoozeSelectDateTimeDialog() override;

    [[nodiscard]] QDateTime wakeUpDateTime() const;

private:
    QDateTimeEdit *const mDateTimeEdit;
};
