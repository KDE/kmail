/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2002-2004  Bo Thorsen <bo@sonofthor.dk>
 *                2002-2003  Steffen Hansen <hansen@kde.org>
 *                2002-2003  Zack Rusin <zack@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cachedimapjob.h"
#include "imapaccountbase.h"

#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmfoldercachedimap.h"
#include "kmailicalifaceimpl.h"
#include "kmacctcachedimap.h"
#include "kmmsgdict.h"
#include "maildirjob.h"

#include <kio/scheduler.h>
#include <kio/job.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>


namespace KMail {

// Get messages
CachedImapJob::CachedImapJob( const QValueList<MsgForDownload>& msgs,
                              JobType type, KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mMsgsForDownload( msgs ),
    mTotalBytes(0), mMsg(0), mParentFolder( 0 )
{
  QValueList<MsgForDownload>::ConstIterator it = msgs.begin();
  for ( ; it != msgs.end() ; ++it )
    mTotalBytes += (*it).size;
}

// Put messages
CachedImapJob::CachedImapJob( const QPtrList<KMMessage>& msgs, JobType type,
                              KMFolderCachedImap* folder )
  : FolderJob( msgs, QString::null, type, folder?folder->folder():0 ), mFolder( folder ),
    mTotalBytes( msgs.count() ), // we abuse it as "total number of messages"
    mMsg( 0 ), mParentFolder( 0 )
{
}

CachedImapJob::CachedImapJob( const QValueList<unsigned long>& msgs,
			      JobType type, KMFolderCachedImap* folder )
  : FolderJob( QPtrList<KMMessage>(), QString::null, type, folder?folder->folder():0 ),
    mFolder( folder ), mSerNumMsgList( msgs ), mTotalBytes( msgs.count() ), mMsg( 0 ),
    mParentFolder ( 0 )
{
}

// Add sub folders
CachedImapJob::CachedImapJob( const QValueList<KMFolderCachedImap*>& fList,
                              JobType type, KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mFolderList( fList ), mMsg( 0 ),
    mParentFolder ( 0 )
{
}

// Rename folder
CachedImapJob::CachedImapJob( const QString& string1, JobType type,
                              KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder(folder), mMsg( 0 ), mString( string1 ),
    mParentFolder ( 0 )
{
  assert( folder );
  assert( type != tDeleteMessage ); // moved to another ctor
}

// Delete folders or messages
CachedImapJob::CachedImapJob( const QStringList& foldersOrMsgs, JobType type,
                              KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mFoldersOrMessages( foldersOrMsgs ),
    mMsg( 0 ), mParentFolder( 0 )
{
  assert( folder );
}

// Other jobs (list messages,expunge folder, check uid validity)
CachedImapJob::CachedImapJob( JobType type, KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mMsg( 0 ), mParentFolder ( 0 )
{
  assert( folder );
}

CachedImapJob::~CachedImapJob()
{
  mAccount->mJobList.remove(this);
}

void CachedImapJob::execute()
{
  mSentBytes = 0;

  if( !mFolder ) {
    if( !mMsgList.isEmpty() ) {
      mFolder = static_cast<KMFolderCachedImap*>(mMsgList.first()->storage());
    }
  }
  assert( mFolder );
  mAccount = mFolder->account();
  assert( mAccount != 0 );
  if( mAccount->makeConnection() != ImapAccountBase::Connected ) {
    // No connection to the IMAP server
    kdDebug(5006) << "mAccount->makeConnection() failed" << endl;
    mPassiveDestructor = true;
    delete this;
    return;
  } else
    mPassiveDestructor = false;

  // All necessary conditions have been met. Register this job
  mAccount->mJobList.append(this);

  switch( mType ) {
  case tGetMessage:       slotGetNextMessage();     break;
  case tPutMessage:       slotPutNextMessage();     break;
  case tDeleteMessage:    slotDeleteNextMessages(); break;
  case tExpungeFolder:    expungeFolder();          break;
  case tAddSubfolders:    slotAddNextSubfolder();   break;
  case tDeleteFolders:    slotDeleteNextFolder();   break;
  case tCheckUidValidity: checkUidValidity();       break;
  case tRenameFolder:     renameFolder(mString);    break;
  case tListMessages:     listMessages();           break;
  default:
    assert( 0 );
  }
}

void CachedImapJob::listMessages()
{
  KURL url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + ";UID=1:*;SECTION=FLAGS RFC822.SIZE");

  KIO::SimpleJob *job = KIO::get(url, false, false);
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.cancellable = true;
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL( result(KIO::Job *) ),
           this, SLOT( slotListMessagesResult( KIO::Job* ) ) );
  // send the data directly for KMFolderCachedImap
  connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
           mFolder, SLOT( slotGetMessagesData( KIO::Job* , const QByteArray& ) ) );
}

void CachedImapJob::slotDeleteNextMessages( KIO::Job* job )
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }

    if( job->error() ) {
      mAccount->handleJobError( job, i18n( "Error while deleting messages on the server: " ) + '\n' );
      delete this;
      return;
    }
    mAccount->removeJob(it);
  }

  if( mFoldersOrMessages.isEmpty() ) {
    // No more messages to delete
    delete this;
    return;
  }

  QString uids = mFoldersOrMessages.front(); mFoldersOrMessages.pop_front();

  KURL url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() +
               QString::fromLatin1(";UID=%1").arg(uids) );

  KIO::SimpleJob *simpleJob = KIO::file_delete( url, false );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), simpleJob );
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  mAccount->insertJob( simpleJob, jd );
  connect( simpleJob, SIGNAL( result(KIO::Job *) ),
           this, SLOT( slotDeleteNextMessages(KIO::Job *) ) );
}

void CachedImapJob::expungeFolder()
{
  KURL url = mAccount->getUrl();
  // Special URL that means EXPUNGE
  url.setPath( mFolder->imapPath() + QString::fromLatin1(";UID=*") );

  KIO::SimpleJob *job = KIO::file_delete( url, false );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL( result(KIO::Job *) ),
           this, SLOT( slotExpungeResult(KIO::Job *) ) );
}

void CachedImapJob::slotExpungeResult( KIO::Job * job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }

  if (job->error()) {
    mErrorCode = job->error();
    mAccount->handleJobError( job, i18n( "Error while deleting messages on the server: " ) + '\n' );
  }
  else
    mAccount->removeJob(it);

  delete this;
}

void CachedImapJob::slotGetNextMessage(KIO::Job * job)
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }

    if (job->error()) {
      mErrorCode = job->error();
      mAccount->handleJobError( job, i18n( "Error while retrieving message on the server: " ) + '\n' );
      delete this;
      return;
    }

    ulong size = 0;
    if ((*it).data.size() > 0) {
      ulong uid = mMsg->UID();
      size = mMsg->msgSizeServer();

      // Convert CR/LF to LF.
      size_t dataSize = (*it).data.size();
      dataSize = FolderStorage::crlf2lf( (*it).data.data(), dataSize ); // always <=
      (*it).data.resize( dataSize );

      mMsg->setComplete( true );
      mMsg->fromByteArray( (*it).data );
      mMsg->setUID(uid);
      mMsg->setMsgSizeServer(size);
      mMsg->setTransferInProgress( false );
      int index = 0;
      mFolder->addMsgInternal( mMsg, true, &index );
      emit messageRetrieved( mMsg );
      if ( index > 0 ) mFolder->unGetMsg( index );
    } else {
      emit messageRetrieved( 0 );
    }
    mMsg = 0;

    mSentBytes += size;
    emit progress( mSentBytes, mTotalBytes );
    mAccount->removeJob(it);
  }

  if( mMsgsForDownload.isEmpty() ) {
    delete this;
    return;
  }

  MsgForDownload mfd = mMsgsForDownload.front(); mMsgsForDownload.pop_front();

  mMsg = new KMMessage;
  mMsg->setUID(mfd.uid);
  mMsg->setMsgSizeServer(mfd.size);
  if( mfd.flags > 0 )
    KMFolderImap::flagsToStatus(mMsg, mfd.flags);
  KURL url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + QString(";UID=%1;SECTION=BODY.PEEK[]").arg(mfd.uid));

  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.cancellable = true;
  mMsg->setTransferInProgress(true);
  KIO::SimpleJob *simpleJob = KIO::get(url, false, false);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect(simpleJob, SIGNAL(processedSize(KIO::Job *, KIO::filesize_t)),
          this, SLOT(slotProcessedSize(KIO::Job *, KIO::filesize_t)));
  connect(simpleJob, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotGetNextMessage(KIO::Job *)));
  connect(simpleJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
          mFolder, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
}

void CachedImapJob::slotProcessedSize(KIO::Job *, KIO::filesize_t processed)
{
  emit progress( mSentBytes + processed, mTotalBytes );
}

void CachedImapJob::slotPutNextMessage()
{
  mMsg = 0;

  // First try the message list
  if( !mMsgList.isEmpty() ) {
    mMsg = mMsgList.first();
    mMsgList.removeFirst();
  }

  // Now try the serial number list
  while( mMsg == 0 && !mSerNumMsgList.isEmpty() ) {
    unsigned long serNum = mSerNumMsgList.first();
    mSerNumMsgList.pop_front();

    // Find the message with this serial number
    int i = 0;
    KMFolder* aFolder = 0;
    kmkernel->msgDict()->getLocation( serNum, &aFolder, &i );
    if( mFolder->folder() != aFolder )
      // This message was moved or something
      continue;
    mMsg = mFolder->getMsg( i );
  }

  if( !mMsg ) {
    // No message found for upload
    delete this;
    return;
  }

  KURL url = mAccount->getUrl();
  QString flags = KMFolderImap::statusToFlags( mMsg->status() );
  url.setPath( mFolder->imapPath() + ";SECTION=" + flags );

  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );

  mMsg->setUID( 0 ); // for the index
  QCString cstr(mMsg->asString());
  int a = cstr.find("\nX-UID: ");
  int b = cstr.find('\n', a);
  if (a != -1 && b != -1 && cstr.find("\n\n") > a) cstr.remove(a, b-a);
  QCString mData(cstr.length() + cstr.contains('\n'));
  unsigned int i = 0;
  for( char *ch = cstr.data(); *ch; ch++ ) {
    if ( *ch == '\n' ) {
      mData.at(i) = '\r';
      i++;
    }
    mData.at(i) = *ch; i++;
  }
  jd.data = mData;
  jd.msgList.append( mMsg );

  mMsg->setTransferInProgress(true);
  KIO::SimpleJob *simpleJob = KIO::put(url, 0, false, false, false);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect( simpleJob, SIGNAL( result(KIO::Job *) ),
           SLOT( slotPutMessageResult(KIO::Job *) ) );
  connect( simpleJob, SIGNAL( dataReq(KIO::Job *, QByteArray &) ),
           SLOT( slotPutMessageDataReq(KIO::Job *, QByteArray &) ) );
  connect( simpleJob, SIGNAL( data(KIO::Job *, const QByteArray &) ),
           mFolder, SLOT( slotSimpleData(KIO::Job *, const QByteArray &) ) );
  connect( simpleJob, SIGNAL(infoMessage(KIO::Job *, const QString &)),
             SLOT(slotPutMessageInfoData(KIO::Job *, const QString &)) );

}

//-----------------------------------------------------------------------------
// TODO: port to KIO::StoredTransferJob once it's ok to require kdelibs-3.3
void CachedImapJob::slotPutMessageDataReq(KIO::Job *job, QByteArray &data)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }
  if ((*it).data.size() - (*it).offset > 0x8000) {
    data.duplicate((*it).data.data() + (*it).offset, 0x8000);
    (*it).offset += 0x8000;
  } else if ((*it).data.size() - (*it).offset > 0) {
    data.duplicate((*it).data.data() + (*it).offset,
                   (*it).data.size() - (*it).offset);
    (*it).offset = (*it).data.size();
  } else
    data.resize(0);
}

//----------------------------------------------------------------------------
void CachedImapJob::slotPutMessageInfoData(KIO::Job *job, const QString &data)
{
  KMFolderCachedImap * imapFolder = static_cast<KMFolderCachedImap*>(mDestFolder->storage());
  KMAcctCachedImap *account = imapFolder->account();
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if ( data.find("UID") != -1 && mMsg )
  {
    int uid = (data.right(data.length()-4)).toInt();
    kdDebug( 5006 ) << k_funcinfo << "Server told us uid is: " << uid << endl;
    mMsg->setUID( uid );
  }
}


//-----------------------------------------------------------------------------
void CachedImapJob::slotPutMessageResult(KIO::Job *job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }

  if ( job->error() ) {
    bool cont = mAccount->handlePutError( job, *it, mFolder->folder() );
    if ( !cont ) {
      delete this;
    } else {
      mMsg = 0;
      slotPutNextMessage();
    }
    return;
  }

  emit messageStored( mMsg );

  // we abuse those fields, the unit is the number of messages, here
  ++mSentBytes;
  emit progress( mSentBytes, mTotalBytes );

  int i;
  if( ( i = mFolder->find(mMsg) ) != -1 ) {
     /*
      * If we have aquired a uid during upload the server supports the uidnext
      * extension and there is no need to redownload this mail, we already have
      * it. Otherwise remove it, it will be redownloaded.
      */
     if ( mMsg->UID() == 0 ) {
        mFolder->removeMsg(i);
     } else {
        // When removing+readding, no point in telling the imap resources about it
        bool b = kmkernel->iCalIface().isResourceQuiet();
        kmkernel->iCalIface().setResourceQuiet( true );

        mFolder->take( i );
        mFolder->addMsgKeepUID( mMsg );
        mMsg->setTransferInProgress( false );

        kmkernel->iCalIface().setResourceQuiet( b );
     }
  }
  mMsg = NULL;
  mAccount->removeJob( it );
  slotPutNextMessage();
}


void CachedImapJob::slotAddNextSubfolder( KIO::Job * job )
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }

    // make copy of setting, to reset it before potentially destroying 'it'
    bool silentUpload = static_cast<KMFolderCachedImap*>((*it).parent->storage())->silentUpload();
    static_cast<KMFolderCachedImap*>((*it).parent->storage())->setSilentUpload( false );

    if ( job->error() && !silentUpload ) {
      QString myError = "<p><b>" + i18n("Error while uploading folder")
        + "</b></p><p>" + i18n("Could not make the folder <b>%1</b> on the server.").arg((*it).items[0])
        + "</p><p>" + i18n("This could be because you do not have permission to do this, or because the folder is already present on the server; the error message from the server communication is here:") + "</p>";
      mAccount->handleJobError( job, myError );
    }

    if( job->error() ) {
      delete this;
      return;
    }
    mAccount->removeJob( it );
  }

  if (mFolderList.isEmpty()) {
    // No more folders to add
    delete this;
    return;
  }

  KMFolderCachedImap *folder = mFolderList.front();
  mFolderList.pop_front();
  KURL url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + folder->folder()->name());

  // Associate the jobData with the parent folder, not with the child
  // This is necessary in case of an error while creating the subfolder,
  // so that folderComplete is called on the parent (and the sync resetted).
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.items << folder->label(); // for the err msg
  KIO::SimpleJob *simpleJob = KIO::mkdir(url);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect( simpleJob, SIGNAL(result(KIO::Job *)),
           this, SLOT(slotAddNextSubfolder(KIO::Job *)) );
}


void CachedImapJob::slotDeleteNextFolder( KIO::Job *job )
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }

    mAccount->removeDeletedFolder( (*it).path );

    if( job->error() ) {
      mAccount->handleJobError( job, i18n( "Error while deleting folder %1 on the server: " ).arg( (*it).path ) + '\n' );
      delete this;
      return;
    }
    mAccount->removeJob(it);
  }

  if( mFoldersOrMessages.isEmpty() ) {
    // No more folders to delete
    delete this;
    return;
  }

  QString folderPath = mFoldersOrMessages.front(); mFoldersOrMessages.pop_front();
  KURL url = mAccount->getUrl();
  url.setPath(folderPath);
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.path = url.path();
  KIO::SimpleJob *simpleJob = KIO::file_delete(url, false);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect( simpleJob, SIGNAL( result(KIO::Job *) ),
           SLOT( slotDeleteNextFolder(KIO::Job *) ) );
}

void CachedImapJob::checkUidValidity()
{
  KURL url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + ";UID=0:0" );

  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.cancellable = true;

  KIO::SimpleJob *job = KIO::get( url, false, false );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL(result(KIO::Job *)),
           SLOT(slotCheckUidValidityResult(KIO::Job *)) );
  connect( job, SIGNAL(data(KIO::Job *, const QByteArray &)),
           mFolder, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
}

void CachedImapJob::slotCheckUidValidityResult(KIO::Job * job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }

  if( job->error() ) {
    mErrorCode = job->error();
    mAccount->handleJobError( job, i18n( "Error while reading folder %1 on the server: " ).arg( (*it).parent->label() ) + '\n' );
    delete this;
    return;
  }

  // Check the uidValidity
  QCString cstr((*it).data.data(), (*it).data.size() + 1);
  int a = cstr.find("X-uidValidity: ");
  if (a < 0) {
    // Something is seriously rotten here!
    // TODO: Tell the user that he has a problem
    kdDebug(5006) << "No uidvalidity available for folder "
                  << mFolder->name() << endl;
  }
  else {
    int b = cstr.find("\r\n", a);
    if ( (b - a - 15) >= 0 ) {
      QString uidv = cstr.mid(a + 15, b - a - 15);
      // kdDebug(5006) << "New uidv = " << uidv << ", old uidv = "
      //               << mFolder->uidValidity() << endl;
      if( !mFolder->uidValidity().isEmpty() && mFolder->uidValidity() != uidv ) {
        // kdDebug(5006) << "Expunging the mailbox " << mFolder->name()
        //               << "!" << endl;
        mFolder->expunge();
        mFolder->setLastUid( 0 );
        mFolder->clearUidMap();
      }
    } else
      kdDebug(5006) << "No uidvalidity available for folder "
                    << mFolder->name() << endl;
  }

  mAccount->removeJob(it);
  delete this;
}


void CachedImapJob::renameFolder( const QString &newName )
{
  // Set the source URL
  KURL urlSrc = mAccount->getUrl();
  urlSrc.setPath( mFolder->imapPath() );

  // Set the destination URL - this is a bit trickier
  KURL urlDst = mAccount->getUrl();
  QString imapPath( mFolder->imapPath() );
  // Destination url = old imappath - oldname + new name
  imapPath.truncate( imapPath.length() - mFolder->folder()->name().length() - 1);
  imapPath += newName + '/';
  urlDst.setPath( imapPath );

  ImapAccountBase::jobData jd( newName, mFolder->folder() );
  jd.path = imapPath;

  KIO::SimpleJob *simpleJob = KIO::rename( urlSrc, urlDst, false );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), simpleJob );
  mAccount->insertJob( simpleJob, jd );
  connect( simpleJob, SIGNAL(result(KIO::Job *)),
           SLOT(slotRenameFolderResult(KIO::Job *)) );
}

static void renameChildFolders( KMFolderDir* dir, const QString& oldPath,
                                const QString& newPath )
{
  if( dir ) {
    KMFolderNode *node = dir->first();
    while( node ) {
      if( !node->isDir() ) {
        KMFolderCachedImap* imapFolder =
          static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
        if ( !imapFolder->imapPath().isEmpty() )
          // Only rename folders that have been accepted by the server
          if( imapFolder->imapPath().find( oldPath ) == 0 ) {
            QString p = imapFolder->imapPath();
            p = p.mid( oldPath.length() );
            p.prepend( newPath );
            imapFolder->setImapPath( p );
            renameChildFolders( imapFolder->folder()->child(), oldPath, newPath );
          }
      }
      node = dir->next();
    }
  }
}

void CachedImapJob::slotRenameFolderResult( KIO::Job *job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }


  if( job->error() ) {
    // Error, revert label change
    QMap<QString, KMAcctCachedImap::RenamedFolder>::ConstIterator renit = mAccount->renamedFolders().find( mFolder->imapPath() );
    Q_ASSERT( renit != mAccount->renamedFolders().end() );
    if ( renit != mAccount->renamedFolders().end() ) {
      mFolder->folder()->setLabel( (*renit).mOldLabel );
      mAccount->removeRenamedFolder( mFolder->imapPath() );
    }
    mAccount->handleJobError( job, i18n( "Error while trying to rename folder %1" ).arg( mFolder->label() ) + '\n' );
  } else {
    // Okay, the folder seems to be renamed on the server,
    // now rename it on disk
    QString oldName = mFolder->name();
    QString oldPath = mFolder->imapPath();
    mAccount->removeRenamedFolder( oldPath );
    mFolder->setImapPath( (*it).path );
    mFolder->FolderStorage::rename( (*it).url );

    if( oldPath.endsWith( "/" ) ) oldPath.truncate( oldPath.length() -1 );
    QString newPath = mFolder->imapPath();
    if( newPath.endsWith( "/" ) ) newPath.truncate( newPath.length() -1 );
    renameChildFolders( mFolder->folder()->child(), oldPath, newPath );
    kmkernel->dimapFolderMgr()->contentsChanged();

    mAccount->removeJob(it);
  }
  delete this;
}

void CachedImapJob::slotListMessagesResult( KIO::Job * job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }

  if (job->error()) {
    mErrorCode = job->error();
    mAccount->handleJobError( job, i18n( "Error while deleting messages on the server: " ) + '\n' );
  }
  else
    mAccount->removeJob(it);

  delete this;
}

//-----------------------------------------------------------------------------
void CachedImapJob::setParentFolder( const KMFolderCachedImap* parent )
{
  mParentFolder = const_cast<KMFolderCachedImap*>( parent );
}

}

#include "cachedimapjob.moc"
