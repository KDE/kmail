/*
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "pimmessagebox.h"

#include <QDialog>
#include <QPushButton>

QDialogButtonBox::StandardButton PIMMessageBox::fourBtnMsgBox(QWidget *parent,
                                                              QMessageBox::Icon type,
                                                              const QString &text,
                                                              const QString &caption,
                                                              const QString &button1Text,
                                                              const QString &button2Text,
                                                              const QString &button3Text,
                                                              KMessageBox::Options options)
{
    auto dialog = new QDialog(parent);
    dialog->setWindowTitle(caption);
    auto box = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel | QDialogButtonBox::Ok, parent);
    dialog->setObjectName(QStringLiteral("PIMMessageBox"));
    box->button(QDialogButtonBox::Ok)->setText(button3Text);
    box->button(QDialogButtonBox::Yes)->setText(button1Text);
    box->button(QDialogButtonBox::No)->setText(button2Text);
    box->button(QDialogButtonBox::Yes)->setDefault(true);

    bool checkboxResult = false;
    const QDialogButtonBox::StandardButton result = KMessageBox::createKMessageBox(dialog, box, type, text, QStringList(), QString(), &checkboxResult, options);
    return result;
}
