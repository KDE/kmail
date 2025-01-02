/*
   SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <MailCommon/MailInterfaces>

namespace Akonadi
{
class EntityTreeModel;
class EntityMimeTypeFilterModel;
}

namespace MailCommon
{
class FolderCollectionMonitor;
class JobScheduler;
}

class ArchiveMailKernel : public QObject, public MailCommon::IKernel, public MailCommon::ISettings
{
    Q_OBJECT
public:
    explicit ArchiveMailKernel(QObject *parent = nullptr);

    static ArchiveMailKernel *self();

    [[nodiscard]] KIdentityManagementCore::IdentityManager *identityManager() override;
    [[nodiscard]] MessageComposer::MessageSender *msgSender() override;

    [[nodiscard]] Akonadi::EntityMimeTypeFilterModel *collectionModel() const override;
    [[nodiscard]] KSharedConfig::Ptr config() override;
    void syncConfig() override;
    [[nodiscard]] MailCommon::JobScheduler *jobScheduler() const override;
    [[nodiscard]] Akonadi::ChangeRecorder *folderCollectionMonitor() const override;
    void updateSystemTray() override;

    [[nodiscard]] qreal closeToQuotaThreshold() override;
    [[nodiscard]] bool excludeImportantMailFromExpiry() override;
    [[nodiscard]] QStringList customTemplates() override;
    [[nodiscard]] Akonadi::Collection::Id lastSelectedFolder() override;
    void setLastSelectedFolder(Akonadi::Collection::Id col) override;
    [[nodiscard]] bool showPopupAfterDnD() override;
    void expunge(Akonadi::Collection::Id col, bool sync) override;

private:
    Q_DISABLE_COPY(ArchiveMailKernel)
    KIdentityManagementCore::IdentityManager *mIdentityManager = nullptr;
    MailCommon::FolderCollectionMonitor *mFolderCollectionMonitor = nullptr;
    Akonadi::EntityTreeModel *mEntityTreeModel = nullptr;
    Akonadi::EntityMimeTypeFilterModel *mCollectionModel = nullptr;
    MailCommon::JobScheduler *mJobScheduler = nullptr;
};
