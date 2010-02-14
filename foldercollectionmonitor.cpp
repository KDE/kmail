/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

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
#include <akonadi/changerecorder.h>
#include <akonadi/collection.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/item.h>
#include <akonadi/kmime/messageparts.h>

FolderCollectionMonitor::FolderCollectionMonitor(QObject *parent)
  :QObject( parent )
{
  // monitor collection changes
  mMonitor = new Akonadi::ChangeRecorder( this );
  mMonitor->setCollectionMonitored( Akonadi::Collection::root() );
  mMonitor->fetchCollection( true );
  mMonitor->setAllMonitored( true );
  mMonitor->setMimeTypeMonitored( FolderCollectionMonitor::mimetype() );
  mMonitor->setResourceMonitored( "akonadi_search_resource" ,  true );
  mMonitor->setResourceMonitored( "akonadi_nepomuktag_resource" ,  true );
  // TODO: Only fetch the envelope etc if possible.
  mMonitor->itemFetchScope().fetchPayloadPart( Akonadi::MessagePart::Header );
}

FolderCollectionMonitor::~FolderCollectionMonitor()
{
}

QString FolderCollectionMonitor::mimetype()
{
  return "message/rfc822";
}


Akonadi::ChangeRecorder *FolderCollectionMonitor::monitor()
{
  return mMonitor;
}

void FolderCollectionMonitor::expireAllFolders(bool immediate )
{
    kDebug() << "AKONADI PORT: Need to implement it  " << Q_FUNC_INFO;
}

void FolderCollectionMonitor::compactAllFolders( bool immediate )
{
    kDebug() << "AKONADI PORT: Need to implement it  " << Q_FUNC_INFO;

}

void FolderCollectionMonitor::expunge( const Akonadi::Collection & col )
{
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( col,this );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotExpungeJob( KJob* ) ) );

}

void FolderCollectionMonitor::slotExpungeJob( KJob *job )
{
  if ( job->error() ) {
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    return;
  }
  Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
  if ( !fjob )
    return;
  Akonadi::Item::List lstItem = fjob->items();
  Akonadi::ItemDeleteJob *jobDelete = new Akonadi::ItemDeleteJob(lstItem,this );

}

