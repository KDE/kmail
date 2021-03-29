/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>

class MailMergeConfigureWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MailMergeConfigureWidget(QWidget *parent = nullptr);
    ~MailMergeConfigureWidget() override;
};
