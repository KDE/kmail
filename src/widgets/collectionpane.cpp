/*
   SPDX-FileCopyrightText: 2010-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "collectionpane.h"
#include "kmkernel.h"
#include <MailCommon/MailKernel>

#include <Akonadi/KMime/MessageFolderAttribute>
#include <KIdentityManagement/kidentitymanagement/identity.h>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <MailCommon/FolderSettings>
#include <PimCommonAkonadi/MailUtil>

using namespace MailCommon;

CollectionPane::CollectionPane(bool restoreSession, QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent)
    : MessageList::Pane(restoreSession, model, selectionModel, parent)
{
}

CollectionPane::~CollectionPane() = default;

void CollectionPane::writeConfig(bool /*restoreSession*/)
{
    MessageList::Pane::writeConfig(!KMailSettings::self()->startSpecificFolderAtStartup());
}

MessageList::StorageModel *CollectionPane::createStorageModel(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent)
{
    return new CollectionStorageModel(model, selectionModel, parent);
}

CollectionStorageModel::CollectionStorageModel(QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent)
    : MessageList::StorageModel(model, selectionModel, parent)
{
}

CollectionStorageModel::~CollectionStorageModel() = default;

bool CollectionStorageModel::isOutBoundFolder(const Akonadi::Collection &c) const
{
    if (c.hasAttribute<Akonadi::MessageFolderAttribute>()) {
        return c.attribute<Akonadi::MessageFolderAttribute>()->isOutboundFolder();
    }
    QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(c, false);
    if (fd) {
        const QString folderId(QString::number(c.id()));
        // default setting
        const KIdentityManagement::Identity &identity = kmkernel->identityManager()->identityForUoidOrDefault(fd->identity());

        bool isOnline = false;
        if (CommonKernel->isSystemFolderCollection(c) && !PimCommon::MailUtil::isImapFolder(c, isOnline)) {
            // local system folders
            if (c == CommonKernel->inboxCollectionFolder() || c == CommonKernel->trashCollectionFolder()) {
                return false;
            }
            if (c == CommonKernel->outboxCollectionFolder() || c == CommonKernel->sentCollectionFolder() || c == CommonKernel->templatesCollectionFolder()
                || c == CommonKernel->draftsCollectionFolder()) {
                return true;
            }
        } else if (identity.drafts() == folderId || identity.templates() == folderId || identity.fcc() == folderId) {
            // drafts, templates or sent of the identity
            return true;
        } else {
            return false;
        }
    }

    return false;
}
