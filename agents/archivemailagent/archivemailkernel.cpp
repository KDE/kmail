/*
   SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivemailkernel.h"

#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/Session>
#include <KIdentityManagementCore/IdentityManager>
#include <KSharedConfig>
#include <MailCommon/FolderCollectionMonitor>
#include <MailCommon/JobScheduler>

ArchiveMailKernel::ArchiveMailKernel(QObject *parent)
    : QObject(parent)
{
    mIdentityManager = new KIdentityManagementCore::IdentityManager(true, this);
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
    mCollectionModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    mJobScheduler = new MailCommon::JobScheduler(this);
}

ArchiveMailKernel *ArchiveMailKernel::self()
{
    static ArchiveMailKernel s_self;
    return &s_self;
}

KIdentityManagementCore::IdentityManager *ArchiveMailKernel::identityManager()
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
    return {};
}

bool ArchiveMailKernel::excludeImportantMailFromExpiry()
{
    return false;
}

Akonadi::Collection::Id ArchiveMailKernel::lastSelectedFolder()
{
    return Akonadi::Collection::Id();
}

void ArchiveMailKernel::setLastSelectedFolder([[maybe_unused]] Akonadi::Collection::Id col)
{
}

void ArchiveMailKernel::expunge([[maybe_unused]] Akonadi::Collection::Id col, [[maybe_unused]] bool sync)
{
}

#include "moc_archivemailkernel.cpp"
