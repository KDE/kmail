/*
   Copyright (C) 2010-2017 Montel Laurent <montel@kde.org>

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

#include "collectionpane.h"
#include "kmkernel.h"
#include "mailcommon/mailkernel.h"

#include <MailCommon/FolderSettings>
#include <PimCommonAkonadi/MailUtil>
#include <KIdentityManagement/kidentitymanagement/identitymanager.h>
#include <KIdentityManagement/kidentitymanagement/identity.h>
#include <Akonadi/KMime/MessageFolderAttribute>

using namespace MailCommon;

CollectionPane::CollectionPane(bool restoreSession, QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent)
    : MessageList::Pane(restoreSession, model, selectionModel, parent)
{
}

CollectionPane::~CollectionPane()
{
}

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

CollectionStorageModel::~CollectionStorageModel()
{
}

bool CollectionStorageModel::isOutBoundFolder(const Akonadi::Collection &c) const
{
    if (c.hasAttribute<Akonadi::MessageFolderAttribute>()
            && c.attribute<Akonadi::MessageFolderAttribute>()->isOutboundFolder()) {
        return true;
    }
    QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(c, false);
    if (fd) {
        const QString folderId(QString::number(c.id()));
        // default setting
        const KIdentityManagement::Identity &identity =
            kmkernel->identityManager()->identityForUoidOrDefault(fd->identity());

        bool isOnline = false;
        if (CommonKernel->isSystemFolderCollection(c) &&
                !PimCommon::MailUtil::isImapFolder(c, isOnline)) {
            // local system folders
            if (c == CommonKernel->inboxCollectionFolder() ||
                    c == CommonKernel->trashCollectionFolder()) {
                return false;
            }
            if (c == CommonKernel->outboxCollectionFolder() ||
                    c == CommonKernel->sentCollectionFolder() ||
                    c == CommonKernel->templatesCollectionFolder() ||
                    c == CommonKernel->draftsCollectionFolder()) {
                return true;
            }
        } else if (identity.drafts() == folderId ||
                   identity.templates() == folderId ||
                   identity.fcc() == folderId)
            // drafts, templates or sent of the identity
        {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

