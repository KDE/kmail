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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/
#ifndef PIMMESSAGEBOX_H
#define PIMMESSAGEBOX_H

#include <KMessageBox>
#include <QMessageBox>
#include <QString>
#include <QDialogButtonBox>
class PIMMessageBox
{
public:
    static QDialogButtonBox::StandardButton fourBtnMsgBox(QWidget *parent, QMessageBox::Icon type, const QString &text, const QString &caption = QString(), const QString &button1Text = QString(), const QString &button2Text = QString(),
                                                          const QString &button3Text = QString(), KMessageBox::Options options = KMessageBox::Notify);
};
#endif
