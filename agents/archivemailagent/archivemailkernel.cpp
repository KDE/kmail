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

#include "archivemailkernel.h"

#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <MailCommon/FolderCollectionMonitor>
#include <MailCommon/JobScheduler>
#include <AkonadiCore/session.h>
#include <AkonadiCore/entitytreemodel.h>
#include <AkonadiCore/entitymimetypefiltermodel.h>
#include <AkonadiCore/changerecorder.h>
#include <KSharedConfig>

ArchiveMailKernel::ArchiveMailKernel(QObject *parent)
    : QObject(parent)
{
    mIdentityManager = new KIdentityManagement::IdentityManager(true, this);
    Akonadi::Session *session = new Akonadi::Session("Archive Mail Kernel ETM", this);
    mFolderCollectionMonitor = new MailCommon::FolderCollectionMonitor(session, this);

    mFolderCollectionMonitor->monitor()->setChangeRecordingEnabled(false);

    mEntityTreeModel = new Akonadi::EntityTreeModel(folderCollectionMonitor(), this);
    mEntityTreeModel->setListFilter(Akonadi::CollectionFetchScope::Enabled);
    mEntityTreeModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    mCollectionModel = new Akonadi::EntityMimeTypeFilterModel(this);
    mCollectionModel->setSourceModel(mEntityTreeModel);
    mCollectionModel->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());
    mCollectionModel->setHeaderGroup(Akonadi::EntityTreeModel::CollectionTreeHeaders);
    mCollectionModel->setDynamicSortFilter(true);
    mCollectionModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    mJobScheduler = new MailCommon::JobScheduler(this);
}

KIdentityManagement::IdentityManager *ArchiveMailKernel::identityManager()
{
    return mIdentityManager;
}

MessageComposer::MessageSender *ArchiveMailKernel::msgSender()
{
    return Q_NULLPTR;
}

Akonadi::EntityMimeTypeFilterModel *ArchiveMailKernel::collectionModel() const
{
    return mCollectionModel;
}

KSharedConfig::Ptr ArchiveMailKernel::config()
{
    return KSharedConfig::openConfig();
}

void ArchiveMailKernel::syncConfig()
{
    Q_ASSERT(false);
}

MailCommon::JobScheduler *ArchiveMailKernel::jobScheduler() const
{
    return mJobScheduler;
}

Akonadi::ChangeRecorder *ArchiveMailKernel::folderCollectionMonitor() const
{
    return mFolderCollectionMonitor->monitor();
}

void ArchiveMailKernel::updateSystemTray()
{
    Q_ASSERT(false);
}

bool ArchiveMailKernel::showPopupAfterDnD()
{
    return false;
}

qreal ArchiveMailKernel::closeToQuotaThreshold()
{
    return 80;
}

QStringList ArchiveMailKernel::customTemplates()
{
    Q_ASSERT(false);
    return QStringList();
}

bool ArchiveMailKernel::excludeImportantMailFromExpiry()
{
    Q_ASSERT(false);
    return true;
}

Akonadi::Collection::Id ArchiveMailKernel::lastSelectedFolder()
{
    return Akonadi::Collection::Id();
}

void ArchiveMailKernel::setLastSelectedFolder(Akonadi::Collection::Id col)
{
    Q_UNUSED(col);
}

void ArchiveMailKernel::expunge(Akonadi::Collection::Id col, bool sync)
{
    Q_UNUSED(col);
    Q_UNUSED(sync);
}
