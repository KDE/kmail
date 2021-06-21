/*
   SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "archivemailinfo.h"
#include "ui_archivemailwidget.h"
#include <QTreeWidgetItem>

#include <AkonadiCore/AgentConfigurationBase>

class ArchiveMailItem : public QTreeWidgetItem
{
public:
    explicit ArchiveMailItem(QTreeWidget *parent = nullptr);
    ~ArchiveMailItem() override;

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
        StorageDirectory,
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

