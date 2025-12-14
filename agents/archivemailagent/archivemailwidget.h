/*
   SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "archivemailinfo.h"
#include "ui_archivemailwidget.h"
#include <QTreeWidgetItem>

#include <Akonadi/AgentConfigurationBase>

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

    [[nodiscard]] bool save() const override;
    void load() override;

    void needReloadConfig();

private:
    void createOrUpdateItem(ArchiveMailInfo *info, ArchiveMailItem *item = nullptr);
    [[nodiscard]] bool verifyExistingArchive(ArchiveMailInfo *info) const;
    void updateDiffDate(ArchiveMailItem *item, ArchiveMailInfo *info);

    void slotDeleteItem();
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
