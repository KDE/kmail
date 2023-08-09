/*
   SPDX-FileCopyrightText: 2012-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QWidget>
class KTimeComboBox;
class ArchiveMailRangeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ArchiveMailRangeWidget(QWidget *parent = nullptr);
    ~ArchiveMailRangeWidget() override;

private:
    KTimeComboBox *const mStartRange;
    KTimeComboBox *const mEndRange;
};
