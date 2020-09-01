/*
  SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef VALIDATESENDMAILSHORTCUT_H
#define VALIDATESENDMAILSHORTCUT_H

#include <QWidget>
class KActionCollection;
class ValidateSendMailShortcut
{
public:
    explicit ValidateSendMailShortcut(KActionCollection *actionCollection, QWidget *parent = nullptr);
    ~ValidateSendMailShortcut();

    Q_REQUIRED_RESULT bool validate();

private:
    QWidget *const mParent;
    KActionCollection *const mActionCollection;
};

#endif // VALIDATESENDMAILSHORTCUT_H
