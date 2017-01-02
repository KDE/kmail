/*
   Copyright (C) 2015-2017 Montel Laurent <montel@kde.org>

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

#ifndef ARCHIVEMAILWIDGET_H
#define ARCHIVEMAILWIDGET_H

#include "ui_archivemailwidget.h"
#include "archivemailinfo.h"
#include <QTreeWidgetItem>

class ArchiveMailItem : public QTreeWidgetItem
{
public:
    explicit ArchiveMailItem(QTreeWidget *parent = nullptr);
    ~ArchiveMailItem();

    void setInfo(ArchiveMailInfo *info);
    ArchiveMailInfo *info() const;

private:
    ArchiveMailInfo *mInfo;
};

class ArchiveMailWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ArchiveMailWidget(QWidget *parent = nullptr);
    ~ArchiveMailWidget();

    enum ArchiveMailColumn {
        Name = 0,
        LastArchiveDate,
        NextArchive,
        StorageDirectory
    };

    void save();
    void saveTreeWidgetHeader(KConfigGroup &group);
    void restoreTreeWidgetHeader(const QByteArray &group);
    void needReloadConfig();

Q_SIGNALS:
    void archiveNow(ArchiveMailInfo *info);

private:
    void load();
    void createOrUpdateItem(ArchiveMailInfo *info, ArchiveMailItem *item = nullptr);
    bool verifyExistingArchive(ArchiveMailInfo *info) const;
    void updateDiffDate(ArchiveMailItem *item, ArchiveMailInfo *info);

private:
    void slotRemoveItem();
    void slotModifyItem();
    void slotAddItem();
    void updateButtons();
    void slotOpenFolder();
    void customContextMenuRequested(const QPoint &);
    void slotArchiveNow();
    void slotItemChanged(QTreeWidgetItem *item, int);
    bool mChanged;
    Ui::ArchiveMailWidget *mWidget;
};

#endif // ARCHIVEMAILWIDGET_H
