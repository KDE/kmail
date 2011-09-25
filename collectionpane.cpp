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
#include "mailkernel.h"

#include "foldercollection.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>
#include <akonadi/kmime/messagefolderattribute.h>

using namespace MailCommon;

CollectionPane::CollectionPane( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent )
  :MessageList::Pane( model, selectionModel, parent )
{
}

CollectionPane::~CollectionPane()
{
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
    const QString folderString( QString::number( c.id() ) );
    // default setting
    const KPIMIdentities::Identity & identity =
      kmkernel->identityManager()->identityForUoidOrDefault( fd->identity() );

    if ( CommonKernel->isSystemFolderCollection(c) &&
         !kmkernel->isImapFolder( c ) ) {
      // local system folders
      if ( c == CommonKernel->inboxCollectionFolder() ||
           c == CommonKernel->trashCollectionFolder() )
        return false;
      if ( c == CommonKernel->outboxCollectionFolder() ||
           c == CommonKernel->sentCollectionFolder() ||
           c == CommonKernel->templatesCollectionFolder() ||
           c == CommonKernel->draftsCollectionFolder() )
        return true;
    } else if ( identity.drafts() == folderString ||
                identity.templates() == folderString ||
                identity.fcc() == folderString )
      // drafts, templates or sent of the identity
      return true;
    else
      return false;
  }

  return false;
}



#include "collectionpane.moc"
