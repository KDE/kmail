/*
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "pimmessagebox.h"

#include <QDialog>
#include <QPushButton>

QDialogButtonBox::StandardButton PIMMessageBox::fourBtnMsgBox(QWidget *parent, QMessageBox::Icon type, const QString &text, const QString &caption, const QString &button1Text, const QString &button2Text, const QString &button3Text, KMessageBox::Options options)
{
    QDialog *dialog = new QDialog(parent);
    dialog->setWindowTitle(caption);
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel | QDialogButtonBox::Ok, parent);
    dialog->setObjectName(QStringLiteral("PIMMessageBox"));
    box->button(QDialogButtonBox::Ok)->setText(button3Text);
    box->button(QDialogButtonBox::Yes)->setText(button1Text);
    box->button(QDialogButtonBox::No)->setText(button2Text);
    box->button(QDialogButtonBox::Yes)->setDefault(true);

    bool checkboxResult = false;
    const QDialogButtonBox::StandardButton result = KMessageBox::createKMessageBox(
        dialog, box, type, text, QStringList(), QString(), &checkboxResult, options);
    return result;
}
