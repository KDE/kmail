/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2010 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "collectionpane.h"
#include "kmkernel.h"
#include "kernel/mailkernel.h"

#include "foldercollection.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include <akonadi/kmime/messagefolderattribute.h>

using namespace MailCommon;

CollectionPane::CollectionPane( bool restoreSession, QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent )
    :MessageList::Pane( restoreSession, model, selectionModel, parent )
{
}

CollectionPane::~CollectionPane()
{
}

void CollectionPane::writeConfig(bool /*restoreSession*/)
{
    MessageList::Pane::writeConfig(!GlobalSettings::self()->startSpecificFolderAtStartup());
}

MessageList::StorageModel *CollectionPane::createStorageModel( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent )
{
    return new CollectionStorageModel( model, selectionModel, parent );
}

CollectionStorageModel::CollectionStorageModel( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QObject *parent )
    : MessageList::StorageModel( model, selectionModel, parent )
{
}

CollectionStorageModel::~CollectionStorageModel()
{
}

bool CollectionStorageModel::isOutBoundFolder( const Akonadi::Collection& c ) const
{
    if ( c.hasAttribute<Akonadi::MessageFolderAttribute>()
         && c.attribute<Akonadi::MessageFolderAttribute>()->isOutboundFolder() ) {
        return true;
    }
    QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( c, false );
    if ( fd ) {
        const Akonadi::Item::Id folderId( c.id() );
        // default setting
        const KPIMIdentities::Identity & identity =
                kmkernel->identityManager()->identityForUoidOrDefault( fd->identity() );

        bool isOnline = false;
        if ( CommonKernel->isSystemFolderCollection(c) &&
             !kmkernel->isImapFolder( c, isOnline ) ) {
            // local system folders
            if ( c == CommonKernel->inboxCollectionFolder() ||
                 c == CommonKernel->trashCollectionFolder() )
                return false;
            if ( c == CommonKernel->outboxCollectionFolder() ||
                 c == CommonKernel->sentCollectionFolder() ||
                 c == CommonKernel->templatesCollectionFolder() ||
                 c == CommonKernel->draftsCollectionFolder() )
                return true;
        } else if ( identity.drafts() == folderId ||
                    identity.templates() == folderId ||
                    identity.fcc() == folderId )
            // drafts, templates or sent of the identity
            return true;
        else
            return false;
    }

    return false;
}

