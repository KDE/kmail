/*
  SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QWidget>
class KActionCollection;
class ValidateSendMailShortcut
{
public:
    explicit ValidateSendMailShortcut(KActionCollection *actionCollection, QWidget *parent = nullptr);
    ~ValidateSendMailShortcut();

    [[nodiscard]] bool validate();

private:
    QWidget *const mParent;
    KActionCollection *const mActionCollection;
};
