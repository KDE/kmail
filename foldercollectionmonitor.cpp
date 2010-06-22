/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009, 2010 Montel Laurent <montel@kde.org>

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

#include "foldercollectionmonitor.h"
#include "kmkernel.h"
#include "util.h"
#include "foldercollection.h"

#include <akonadi/changerecorder.h>
#include <akonadi/collection.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/CollectionModel>
#include <akonadi/item.h>
#include <akonadi/kmime/messageparts.h>
#include <kmime/kmime_message.h>


FolderCollectionMonitor::FolderCollectionMonitor(QObject *parent)
  :QObject( parent )
{
  // monitor collection changes
  mMonitor = new Akonadi::ChangeRecorder( this );
  mMonitor->setCollectionMonitored( Akonadi::Collection::root() );
  mMonitor->fetchCollection( true );
  mMonitor->setAllMonitored( true );
  mMonitor->setMimeTypeMonitored( KMime::Message::mimeType() );
  mMonitor->setResourceMonitored( "akonadi_search_resource" ,  true );
  mMonitor->setResourceMonitored( "akonadi_nepomuktag_resource" ,  true );
  // TODO: Only fetch the envelope etc if possible.
  mMonitor->itemFetchScope().fetchPayloadPart( Akonadi::MessagePart::Header );
}

FolderCollectionMonitor::~FolderCollectionMonitor()
{
}

Akonadi::ChangeRecorder *FolderCollectionMonitor::monitor() const
{
  return mMonitor;
}

void FolderCollectionMonitor::expireAllFolders(bool immediate )
{
  expireAllCollection( KMKernel::self()->entityTreeModel(), immediate );
}

void FolderCollectionMonitor::expireAllCollection( const QAbstractItemModel *model, bool immediate, const QModelIndex& parentIndex )
{
  const int rowCount = model->rowCount( parentIndex );
  for ( int row = 0; row < rowCount; ++row ) {
    const QModelIndex index = model->index( row, 0, parentIndex );
    const Akonadi::Collection collection = model->data( index, Akonadi::CollectionModel::CollectionRole ).value<Akonadi::Collection>();

    if ( !collection.isValid() || KMail::Util::isVirtualCollection( collection ) )
      continue;

    QSharedPointer<FolderCollection> col = FolderCollection::forCollection( collection );
    if ( col && col->isAutoExpire() ) {
      col->expireOldMessages( immediate );
    }

    if ( model->rowCount( index ) > 0 ) {
      expireAllCollection( model, immediate, index );
    }
  }
}

void FolderCollectionMonitor::expunge( const Akonadi::Collection & col )
{
  if ( col.isValid() ) {
    Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( col,this );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotExpungeJob( KJob* ) ) );
  } else {
    kDebug()<<" Try to expunge an invalid collection :"<<col;
  }
}

void FolderCollectionMonitor::slotExpungeJob( KJob *job )
{
  if ( job->error() ) {
    if ( static_cast<KIO::Job*>( job )->ui() )
      static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    kDebug()<<" job->errorString() :"<<job->errorString();
    return;
  }
  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  if ( !fjob )
    return;
  Akonadi::Item::List lstItem = fjob->items();
  if ( lstItem.isEmpty() )
    return;
  Akonadi::ItemDeleteJob *jobDelete = new Akonadi::ItemDeleteJob(lstItem,this );
  connect( jobDelete, SIGNAL( result( KJob* ) ), this, SLOT( slotDeleteJob( KJob* ) ) );

}

void FolderCollectionMonitor::slotDeleteJob( KJob *job )
{
  if ( job->error() ) {
    if ( static_cast<KIO::Job*>( job )->ui() )
      static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    kDebug()<<" job->errorString() :"<<job->errorString();
    return;
  }
}

