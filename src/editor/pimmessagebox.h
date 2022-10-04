/*
    SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

    SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include <KMessageBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QString>
class PIMMessageBox
{
public:
    static QDialogButtonBox::StandardButton fourBtnMsgBox(QWidget *parent,
                                                          QMessageBox::Icon type,
                                                          const QString &text,
                                                          const QString &caption = QString(),
                                                          const QString &button1Text = QString(),
                                                          const QString &button2Text = QString(),
                                                          const QString &button3Text = QString(),
                                                          KMessageBox::Options options = KMessageBox::Notify);
};
