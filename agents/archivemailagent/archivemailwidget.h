/*
   Copyright (C) 2015-2019 Montel Laurent <montel@kde.org>

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

#include <AkonadiCore/AgentConfigurationBase>

class ArchiveMailItem : public QTreeWidgetItem
{
public:
    explicit ArchiveMailItem(QTreeWidget *parent = nullptr);
    ~ArchiveMailItem();

    void setInfo(ArchiveMailInfo *info);
    ArchiveMailInfo *info() const;

private:
    ArchiveMailInfo *mInfo = nullptr;
};

class ArchiveMailWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit ArchiveMailWidget(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args);
    ~ArchiveMailWidget() override;

    enum ArchiveMailColumn {
        Name = 0,
        LastArchiveDate,
        NextArchive,
        StorageDirectory
    };

    Q_REQUIRED_RESULT bool save() const override;
    void load() override;

    void needReloadConfig();

    QSize restoreDialogSize() const override;
    void saveDialogSize(const QSize &size) override;

private:
    void createOrUpdateItem(ArchiveMailInfo *info, ArchiveMailItem *item = nullptr);
    bool verifyExistingArchive(ArchiveMailInfo *info) const;
    void updateDiffDate(ArchiveMailItem *item, ArchiveMailInfo *info);

    void slotRemoveItem();
    void slotModifyItem();
    void slotAddItem();
    void updateButtons();
    void slotOpenFolder();
    void slotCustomContextMenuRequested(const QPoint &);
    void slotItemChanged(QTreeWidgetItem *item, int);

    bool mChanged = false;
    Ui::ArchiveMailWidget mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(ArchiveMailAgentConfigFactory, "archivemailagentconfig.json", ArchiveMailWidget)

#endif // ARCHIVEMAILWIDGET_H
