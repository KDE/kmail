/*
 * Copyright (c) 2004 Carsten Burghardt <burghardt@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */


#include "listjob.h"
#include "kmessagebox.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmacctimap.h"
#include "kmacctcachedimap.h"
#include "folderstorage.h"
#include "kmfolder.h"
#include "progressmanager.h"
using KPIM::ProgressManager;

#include <kdebug.h>
#include <kurl.h>
#include <kio/scheduler.h>
#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>

#include <stdlib.h>

using namespace KMail;

ListJob::ListJob( ImapAccountBase* account, ImapAccountBase::ListType type,
    FolderStorage* storage, const QString& path, bool complete,
    KPIM::ProgressItem* item )
 : FolderJob( 0, tOther, (storage ? storage->folder() : 0) ),
   mStorage( storage ), mAccount( account ), mType( type ),
   mComplete( complete ),
   mHonorLocalSubscription( false ), mPath( path ),
   mParentProgressItem( item )
{
}

ListJob::~ListJob()
{
}

void ListJob::execute()
{
  if ( mAccount->makeConnection() == ImapAccountBase::Error )
  {
    kWarning() <<"ListJob - got no connection";
    deleteLater();
    return;
  } else if ( mAccount->makeConnection() == ImapAccountBase::Connecting )
  {
    // We'll wait for the connectionResult signal from the account.
    kDebug() <<"ListJob - waiting for connection";
    connect( mAccount, SIGNAL( connectionResult(int, const QString&) ),
        this, SLOT( slotConnectionResult(int, const QString&) ) );
    return;
  }
  // this is needed until we have a common base class for d(imap)
  if ( mPath.isEmpty() )
  {
    if ( mStorage && mStorage->folderType() == KMFolderTypeImap ) {
      mPath = static_cast<KMFolderImap*>(mStorage)->imapPath();
    } else if ( mStorage && mStorage->folderType() == KMFolderTypeCachedImap ) {
      mPath = static_cast<KMFolderCachedImap*>(mStorage)->imapPath();
    } else {
      kError() <<"ListJob - no valid path and no folder given";
      deleteLater();
      return;
    }
  }
  if ( mNamespace.isEmpty() && mStorage )
  {
    mNamespace = mAccount->namespaceForFolder( mStorage );
  }
  // create jobData
  ImapAccountBase::jobData jd;
  jd.total = 1; jd.done = 0;
  jd.cancellable = true;
  jd.parent = mDestFolder;
  jd.onlySubscribed = ( mType == ImapAccountBase::ListSubscribed ||
                        mType == ImapAccountBase::ListSubscribedNoCheck ||
                        mType == ImapAccountBase::ListFolderOnlySubscribed );
  jd.path = mPath;
  jd.curNamespace = mNamespace;
  if ( mParentProgressItem )
  {
    QString status = mDestFolder ? mDestFolder->prettyUrl() : QString();
    jd.progressItem = ProgressManager::createProgressItem(
        mParentProgressItem,
        "ListDir" + ProgressManager::getUniqueID(),
        status,
        i18n("retrieving folders"),
        false,
        mAccount->useSSL() || mAccount->useTLS() );
    mParentProgressItem->setStatus( status );
  }

  // make the URL
  QString ltype = "LIST";
  if ( mType == ImapAccountBase::ListSubscribed ||
       mType == ImapAccountBase::ListFolderOnlySubscribed )
    ltype = "LSUB";
  else if ( mType == ImapAccountBase::ListSubscribedNoCheck )
    ltype = "LSUBNOCHECK";

  QString section;
  if ( mComplete )
    section = ";SECTION=COMPLETE";
  else if ( mType == ImapAccountBase::ListFolderOnly ||
            mType == ImapAccountBase::ListFolderOnlySubscribed )
    section = ";SECTION=FOLDERONLY";

  KUrl url = mAccount->getUrl();
  url.setPath( mPath
      + ";TYPE=" + ltype
      + section );
  // go
  //kDebug() <<"start listjob for" << url.path();
  KIO::SimpleJob *job = KIO::listDir( url, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL(result(KJob *)),
      this, SLOT(slotListResult(KJob *)) );
  connect( job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
      this, SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)) );
}

void ListJob::slotConnectionResult( int errorCode, const QString& errorMsg )
{
  Q_UNUSED( errorMsg );
  if ( !errorCode )
    execute();
  else {
    if ( mParentProgressItem )
      mParentProgressItem->setComplete();
    deleteLater();
  }
}

void ListJob::slotListResult( KJob* job )
{
  ImapAccountBase::JobIterator it = mAccount->findJob( static_cast<KIO::Job *>(job) );
  if ( it == mAccount->jobsEnd() )
  {
    deleteLater();
    return;
  }
  if ( job->error() )
  {
    mAccount->handleJobError( static_cast<KIO::Job *>(job),
        i18n( "Error while listing folder %1: ", (*it).path),
        true );
  } else
  {
    // transport the information, include the jobData
    emit receivedFolders( mSubfolderNames, mSubfolderPaths,
        mSubfolderMimeTypes, mSubfolderAttributes, *it );
    mAccount->removeJob( it );
  }
  deleteLater();
}

void ListJob::slotListEntries( KIO::Job* job, const KIO::UDSEntryList& uds )
{
  ImapAccountBase::JobIterator it = mAccount->findJob( job );
  if ( it == mAccount->jobsEnd() )
  {
    deleteLater();
    return;
  }
  if( (*it).progressItem )
    (*it).progressItem->setProgress( 50 );
  for ( KIO::UDSEntryList::ConstIterator udsIt = uds.constBegin();
        udsIt != uds.constEnd(); ++udsIt )
  {
    // get the needed information
    const QString name = udsIt->stringValue( KIO::UDSEntry::UDS_NAME );
    const KUrl url = KUrl( udsIt->stringValue( KIO::UDSEntry::UDS_URL ) ); // utf-8
    const QString mimeType = udsIt->stringValue( KIO::UDSEntry::UDS_MIME_TYPE );
    const QString attributes = udsIt->stringValue( KIO::UDSEntry::UDS_EXTRA );
    if ( (mimeType == "inode/directory" || mimeType == "message/digest"
          || mimeType == "message/directory")
         && name != ".." && (mAccount->hiddenFolders() || name.at(0) != '.') )
    {
      if ( mHonorLocalSubscription && mAccount->onlyLocallySubscribedFolders()
        && !mAccount->locallySubscribedTo( url.path() ) ) {
          continue;
      }

      // Some servers send _lots_ of duplicates
      // This check is too slow for huge lists
      if ( mSubfolderPaths.count() > 100 ||
           !mSubfolderPaths.contains(url.path()) )
      {
        mSubfolderNames.append( name );
        mSubfolderPaths.append( url.path() );
        mSubfolderMimeTypes.append( mimeType );
        mSubfolderAttributes.append( attributes );
      }
    }
  }
}


void KMail::ListJob::setHonorLocalSubscription( bool value )
{
  mHonorLocalSubscription = value;
}

bool KMail::ListJob::honorLocalSubscription() const
{
  return mHonorLocalSubscription;
}

#include "listjob.moc"
