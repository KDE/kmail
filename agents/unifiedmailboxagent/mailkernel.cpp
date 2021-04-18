/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailkernel.h"

#include <AkonadiCore/changerecorder.h>
#include <AkonadiCore/entitymimetypefiltermodel.h>
#include <AkonadiCore/entitytreemodel.h>
#include <AkonadiCore/session.h>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <KSharedConfig>
#include <MailCommon/FolderCollectionMonitor>
#include <MailCommon/MailKernel>
#include <MessageComposer/AkonadiSender>

MailKernel::MailKernel(const KSharedConfigPtr &config, QObject *parent)
    : QObject(parent)
    , mConfig(config)
    , mIdentityManager(new KIdentityManagement::IdentityManager(true, this))
    , mMessageSender(new MessageComposer::AkonadiSender(this))
{
    auto session = new Akonadi::Session("UnifiedMailbox Kernel ETM", this);

    mFolderCollectionMonitor = new MailCommon::FolderCollectionMonitor(session, this);

    mEntityTreeModel = new Akonadi::EntityTreeModel(folderCollectionMonitor(), this);
    mEntityTreeModel->setListFilter(Akonadi::CollectionFetchScope::Enabled);
    mEntityTreeModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);

    mCollectionModel = new Akonadi::EntityMimeTypeFilterModel(this);
    mCollectionModel->setSourceModel(mEntityTreeModel);
    mCollectionModel->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());
    mCollectionModel->setHeaderGroup(Akonadi::EntityTreeModel::CollectionTreeHeaders);
    mCollectionModel->setDynamicSortFilter(true);
    mCollectionModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    CommonKernel->registerKernelIf(this);
    CommonKernel->registerSettingsIf(this);
}

MailKernel::~MailKernel()
{
    CommonKernel->registerKernelIf(nullptr);
    CommonKernel->registerSettingsIf(nullptr);
}

KIdentityManagement::IdentityManager *MailKernel::identityManager()
{
    return mIdentityManager;
}

MessageComposer::MessageSender *MailKernel::msgSender()
{
    return mMessageSender;
}

Akonadi::EntityMimeTypeFilterModel *MailKernel::collectionModel() const
{
    return mCollectionModel;
}

KSharedConfig::Ptr MailKernel::config()
{
    return mConfig;
}

void MailKernel::syncConfig()
{
    Q_ASSERT(false);
}

MailCommon::JobScheduler *MailKernel::jobScheduler() const
{
    Q_ASSERT(false);
    return nullptr;
}

Akonadi::ChangeRecorder *MailKernel::folderCollectionMonitor() const
{
    return mFolderCollectionMonitor->monitor();
}

void MailKernel::updateSystemTray()
{
    Q_ASSERT(false);
}

bool MailKernel::showPopupAfterDnD()
{
    return false;
}

qreal MailKernel::closeToQuotaThreshold()
{
    return 80;
}

QStringList MailKernel::customTemplates()
{
    Q_ASSERT(false);
    return QStringList();
}

bool MailKernel::excludeImportantMailFromExpiry()
{
    Q_ASSERT(false);
    return true;
}

Akonadi::Collection::Id MailKernel::lastSelectedFolder()
{
    Q_ASSERT(false);
    return Akonadi::Collection::Id();
}

void MailKernel::setLastSelectedFolder(Akonadi::Collection::Id col)
{
    Q_UNUSED(col)
}

void MailKernel::expunge(Akonadi::Collection::Id id, bool sync)
{
    Akonadi::Collection col(id);
    if (col.isValid()) {
        mFolderCollectionMonitor->expunge(Akonadi::Collection(col), sync);
    }
}
