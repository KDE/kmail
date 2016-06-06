/*
   Copyright (C) 2012-2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef ARCHIVEMAILDIALOG_H
#define ARCHIVEMAILDIALOG_H

#include "archivemailinfo.h"
#include <QDialog>
class ArchiveMailWidget;
class ArchiveMailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ArchiveMailDialog(QWidget *parent = Q_NULLPTR);
    ~ArchiveMailDialog();

Q_SIGNALS:
    void archiveNow(ArchiveMailInfo *info);

public Q_SLOTS:
    void slotNeedReloadConfig();

protected Q_SLOTS:
    void slotSave();

private:
    void writeConfig();
    void readConfig();
    ArchiveMailWidget *mWidget;
};

#endif /* ARCHIVEMAILWIDGET_H */

