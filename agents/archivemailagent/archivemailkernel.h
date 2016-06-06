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

#ifndef ARCHIVEMAILKERNEL_H
#define ARCHIVEMAILKERNEL_H

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
public:
    explicit  ArchiveMailKernel(QObject *parent = Q_NULLPTR);

    KIdentityManagement::IdentityManager *identityManager() Q_DECL_OVERRIDE;
    MessageComposer::MessageSender *msgSender() Q_DECL_OVERRIDE;

    Akonadi::EntityMimeTypeFilterModel *collectionModel() const Q_DECL_OVERRIDE;
    KSharedConfig::Ptr config() Q_DECL_OVERRIDE;
    void syncConfig() Q_DECL_OVERRIDE;
    MailCommon::JobScheduler *jobScheduler() const Q_DECL_OVERRIDE;
    Akonadi::ChangeRecorder *folderCollectionMonitor() const Q_DECL_OVERRIDE;
    void updateSystemTray() Q_DECL_OVERRIDE;

    qreal closeToQuotaThreshold() Q_DECL_OVERRIDE;
    bool excludeImportantMailFromExpiry() Q_DECL_OVERRIDE;
    QStringList customTemplates() Q_DECL_OVERRIDE;
    Akonadi::Collection::Id lastSelectedFolder() Q_DECL_OVERRIDE;
    void setLastSelectedFolder(Akonadi::Collection::Id col) Q_DECL_OVERRIDE;
    bool showPopupAfterDnD() Q_DECL_OVERRIDE;

private:
    KIdentityManagement::IdentityManager *mIdentityManager;
    MailCommon::FolderCollectionMonitor *mFolderCollectionMonitor;
    Akonadi::EntityTreeModel *mEntityTreeModel;
    Akonadi::EntityMimeTypeFilterModel *mCollectionModel;
    MailCommon::JobScheduler *mJobScheduler;
};

#endif
