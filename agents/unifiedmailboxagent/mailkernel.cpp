/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

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

#include "mailkernel.h"

#include <MailCommon/MailKernel>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <MessageComposer/AkonadiSender>
#include <MailCommon/FolderCollectionMonitor>
#include <AkonadiCore/session.h>
#include <AkonadiCore/entitytreemodel.h>
#include <AkonadiCore/entitymimetypefiltermodel.h>
#include <AkonadiCore/changerecorder.h>
#include <KSharedConfig>

MailKernel::MailKernel(KSharedConfigPtr config, QObject *parent)
    : QObject(parent)
    , mConfig(config)
{
    mMessageSender = new MessageComposer::AkonadiSender(this);
    mIdentityManager = new KIdentityManagement::IdentityManager(true, this);
    Akonadi::Session *session = new Akonadi::Session("UnifiedMailbox Kernel ETM", this);

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
    Q_UNUSED(col);
}

void MailKernel::expunge(Akonadi::Collection::Id id, bool sync)
{
    Akonadi::Collection col(id);
    if (col.isValid()) {
        mFolderCollectionMonitor->expunge(Akonadi::Collection(col), sync);
    }
}

