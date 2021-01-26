/*
   SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailkernel.h"

#include <AkonadiCore/changerecorder.h>
#include <AkonadiCore/entitymimetypefiltermodel.h>
#include <AkonadiCore/entitytreemodel.h>
#include <AkonadiCore/session.h>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <KSharedConfig>
#include <MailCommon/FolderCollectionMonitor>
#include <MailCommon/JobScheduler>

ArchiveMailKernel::ArchiveMailKernel(QObject *parent)
    : QObject(parent)
{
    mIdentityManager = new KIdentityManagement::IdentityManager(true, this);
    auto session = new Akonadi::Session("Archive Mail Kernel ETM", this);
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

ArchiveMailKernel *ArchiveMailKernel::self()
{
    static ArchiveMailKernel s_self;
    return &s_self;
}

KIdentityManagement::IdentityManager *ArchiveMailKernel::identityManager()
{
    return mIdentityManager;
}

MessageComposer::MessageSender *ArchiveMailKernel::msgSender()
{
    return nullptr;
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
    Q_UNUSED(col)
}

void ArchiveMailKernel::expunge(Akonadi::Collection::Id col, bool sync)
{
    Q_UNUSED(col)
    Q_UNUSED(sync)
}
