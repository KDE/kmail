/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

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

    KIdentityManagement::IdentityManager *identityManager() override;
    MessageComposer::MessageSender *msgSender() override;

    Akonadi::EntityMimeTypeFilterModel *collectionModel() const override;
    KSharedConfig::Ptr config() override;
    void syncConfig() override;
    MailCommon::JobScheduler *jobScheduler() const override;
    Akonadi::ChangeRecorder *folderCollectionMonitor() const override;
    void updateSystemTray() override;

    Q_REQUIRED_RESULT qreal closeToQuotaThreshold() override;
    Q_REQUIRED_RESULT bool excludeImportantMailFromExpiry() override;
    Q_REQUIRED_RESULT QStringList customTemplates() override;
    Q_REQUIRED_RESULT Akonadi::Collection::Id lastSelectedFolder() override;
    void setLastSelectedFolder(Akonadi::Collection::Id col) override;
    Q_REQUIRED_RESULT bool showPopupAfterDnD() override;
    void expunge(Akonadi::Collection::Id col, bool sync) override;

private:
    Q_DISABLE_COPY(ArchiveMailKernel)
    KIdentityManagement::IdentityManager *mIdentityManager = nullptr;
    MailCommon::FolderCollectionMonitor *mFolderCollectionMonitor = nullptr;
    Akonadi::EntityTreeModel *mEntityTreeModel = nullptr;
    Akonadi::EntityMimeTypeFilterModel *mCollectionModel = nullptr;
    MailCommon::JobScheduler *mJobScheduler = nullptr;
};

