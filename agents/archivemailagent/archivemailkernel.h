/*
   Copyright (C) 2012-2020 Laurent Montel <montel@kde.org>

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

#ifndef ARCHIVEMAILKERNEL_H
#define ARCHIVEMAILKERNEL_H

#include <MailCommon/MailInterfaces>

namespace Akonadi {
class EntityTreeModel;
class EntityMimeTypeFilterModel;
}

namespace MailCommon {
class FolderCollectionMonitor;
class JobScheduler;
}

class ArchiveMailKernel : public QObject, public MailCommon::IKernel, public MailCommon::ISettings
{
    Q_OBJECT
public:
    explicit ArchiveMailKernel(QObject *parent = nullptr);

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

#endif
