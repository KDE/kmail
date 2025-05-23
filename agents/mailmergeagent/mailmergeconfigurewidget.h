/*
   SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QWidget>
#include <memory>
namespace Ui
{
class MailMergeConfigureWidget;
}

class MailMergeConfigureWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MailMergeConfigureWidget(QWidget *parent = nullptr);
    ~MailMergeConfigureWidget() override;

private:
    std::unique_ptr<Ui::MailMergeConfigureWidget> mWidget;
};
