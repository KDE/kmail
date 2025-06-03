/*
  SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
