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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#include "cachedimapjob.h"
#include "imapaccountbase.h"

#include "kmfoldermgr.h"
#include "kmfolder.h"
#include "kmfoldercachedimap.h"
#include "kmailicalifaceimpl.h"
#include "kmacctcachedimap.h"
#include "kmmsgdict.h"
#include "maildirjob.h"
#include "util.h"
#include "scalix.h"

#include <kio/scheduler.h>
#include <kio/job.h>
#include <klocale.h>
#include <kdebug.h>

#include <QList>
#include <QByteArray>


namespace KMail {

// Get messages
CachedImapJob::CachedImapJob( const QList<MsgForDownload>& msgs,
                              JobType type, KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mMsgsForDownload( msgs ),
    mTotalBytes(0), mMsg(0), mParentFolder( 0 )
{
  QList<MsgForDownload>::ConstIterator it = msgs.begin();
  for ( ; it != msgs.end() ; ++it )
    mTotalBytes += (*it).size;
}

// Put messages
CachedImapJob::CachedImapJob( const QList<KMMessage*>& msgs, JobType type,
                              KMFolderCachedImap* folder )
  : FolderJob( msgs, QString(), type, folder?folder->folder():0 ), mFolder( folder ),
    mTotalBytes( msgs.count() ), // we abuse it as "total number of messages"
    mMsg( 0 ), mParentFolder( 0 )
{
}

CachedImapJob::CachedImapJob( const QList<unsigned long>& msgs,
                              JobType type, KMFolderCachedImap* folder )
  : FolderJob( QList<KMMessage*>(), QString(), type, folder?folder->folder():0 ),
    mFolder( folder ), mSerNumMsgList( msgs ), mTotalBytes( msgs.count() ), mMsg( 0 ),
    mParentFolder ( 0 )
{
}

// Add sub folders
CachedImapJob::CachedImapJob( const QList<KMFolderCachedImap*>& fList,
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
  mAccount->mJobList.removeAll(this);
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
    kDebug() <<"mAccount->makeConnection() failed";
    mPassiveDestructor = true;
    delete this;
    return;
  } else
    mPassiveDestructor = false;

  // All necessary conditions have been met. Register this job
  mAccount->mJobList.append(this);

  /**
   * The Scalix server requires to send him a custom X-SCALIX-ID command
   * to switch it into a special mode.
   *
   * This should be done once after the login and before the first command.
   */
  if ( mAccount->groupwareType() == KMAcctCachedImap::GroupwareScalix ) {
    if ( !mAccount->sentCustomLoginCommand() ) {
      QByteArray packedArgs;
      QDataStream stream( &packedArgs, QIODevice::WriteOnly );

      const QString command = QString( "X-SCALIX-ID " );
      const QString argument = QString( "(\"name\" \"Evolution\" \"version\" \"2.10.0\")" );

      stream << (int) 'X' << 'N' << command << argument;

      const KUrl url = mAccount->getUrl();

      ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
      jd.items << mFolder->label(); // for the err msg
      KIO::SimpleJob *simpleJob = KIO::special( url.url(), packedArgs, false );
      KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
      mAccount->insertJob(simpleJob, jd);

      mAccount->setSentCustomLoginCommand( true );
    }
  }

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
  KUrl url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + ";UID=1:*;SECTION=FLAGS RFC822.SIZE");

  KIO::SimpleJob *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.cancellable = true;
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL( result(KJob *) ),
           this, SLOT( slotListMessagesResult( KJob* ) ) );
  // send the data directly for KMFolderCachedImap
  connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
           mFolder, SLOT( slotGetMessagesData( KIO::Job*, const QByteArray& ) ) );
}

void CachedImapJob::slotDeleteNextMessages( KJob* job )
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }

    if( job->error() ) {
      mAccount->handleJobError( static_cast<KIO::Job*>(job), i18n( "Error while deleting messages on the server: " ) + '\n' );
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

  KUrl url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() +
               QString::fromLatin1(";UID=%1").arg(uids) );

  KIO::SimpleJob *simpleJob = KIO::file_delete( url, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), simpleJob );
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  mAccount->insertJob( simpleJob, jd );
  connect( simpleJob, SIGNAL( result(KJob *) ),
           this, SLOT( slotDeleteNextMessages(KJob *) ) );
}

void CachedImapJob::expungeFolder()
{
  KUrl url = mAccount->getUrl();
  // Special URL that means EXPUNGE
  url.setPath( mFolder->imapPath() + QString::fromLatin1(";UID=*") );

  KIO::SimpleJob *job = KIO::file_delete( url, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL( result(KJob *) ),
           this, SLOT( slotExpungeResult(KJob *) ) );
}

void CachedImapJob::slotExpungeResult( KJob * job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }

  if (job->error()) {
    mErrorCode = job->error();
    mAccount->handleJobError( static_cast<KIO::Job*>(job), i18n( "Error while deleting messages on the server: " ) + '\n' );
  }
  else
    mAccount->removeJob(it);

  delete this;
}

void CachedImapJob::slotGetNextMessage(KJob * job)
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }

    if (job->error()) {
      mErrorCode = job->error();
      mAccount->handleJobError( static_cast<KIO::Job*>(job), i18n( "Error while retrieving message on the server: " ) + '\n' );
      delete this;
      return;
    }

    ulong size = 0;
    if ((*it).data.size() > 0) {
      ulong uid = mMsg->UID();
      size = mMsg->msgSizeServer();

      // Convert CR/LF to LF.
      size_t dataSize = (*it).data.size();
      dataSize = Util::crlf2lf( (*it).data.data(), dataSize ); // always <=
      (*it).data.resize( dataSize );

      mMsg->setComplete( true );
      mMsg->fromString( (*it).data );
      mMsg->setUID(uid);
      mMsg->setMsgSizeServer(size);
      mMsg->setTransferInProgress( false );
      int index = 0;
      mFolder->open( "KMFolderCachedImap::slotGetNextMessage" );
      mFolder->addMsgInternal( mMsg, true, &index );

      if ( kmkernel->iCalIface().isResourceFolder( mFolder->folder() ) ) {
        mFolder->setStatus( index, MessageStatus::statusRead(), false );
      }
      mFolder->close( "KMFolderCachedImap::slotGetNextMessage" );

      emit messageRetrieved( mMsg );
      if ( index > 0 ) mFolder->unGetMsg( index );
    } else {
      emit messageRetrieved( 0 );
    }
    mMsg = 0;

    mSentBytes += size;
    emit progress( mSentBytes, mTotalBytes );
    mAccount->removeJob(it);
  } else
    mFolder->quiet( true );

  if( mMsgsForDownload.isEmpty() ) {
    mFolder->quiet( false );
    delete this;
    return;
  }

  MsgForDownload mfd = mMsgsForDownload.front(); mMsgsForDownload.pop_front();

  mMsg = new KMMessage;
  mMsg->setUID(mfd.uid);
  mMsg->setMsgSizeServer(mfd.size);
  if( mfd.flags > 0 )
    KMFolderImap::flagsToStatus(mMsg, mfd.flags, true, GlobalSettings::allowLocalFlags() ? mFolder->permanentFlags() : INT_MAX);
  KUrl url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + QString(";UID=%1;SECTION=BODY.PEEK[]").arg(mfd.uid));

  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.cancellable = true;
  mMsg->setTransferInProgress(true);
  KIO::SimpleJob *simpleJob = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect(simpleJob, SIGNAL(processedSize(KJob *, qulonglong)),
          this, SLOT(slotProcessedSize(KJob *, qulonglong)));
  connect(simpleJob, SIGNAL(result(KJob *)),
          this, SLOT(slotGetNextMessage(KJob *)));
  connect(simpleJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
          mFolder, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
}

void CachedImapJob::slotProcessedSize(KJob *, qulonglong processed)
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
    KMMsgDict::instance()->getLocation( serNum, &aFolder, &i );
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

  KUrl url = mAccount->getUrl();
  QString flags = KMFolderImap::statusToFlags( mMsg->status(), mFolder->permanentFlags() );
  url.setPath( mFolder->imapPath() + ";SECTION=" + flags );

  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );

  mMsg->setUID( 0 ); // for the index
  QByteArray cstr(mMsg->asString());
  int a = cstr.indexOf("\nX-UID: ");
  int b = cstr.indexOf('\n', a);
  if (a != -1 && b != -1 && cstr.indexOf("\n\n") > a) cstr.remove(a, b-a);
  QByteArray mData;
  mData.resize( cstr.length() + cstr.count('\n') );
  unsigned int i = 0;
  for( char *ch = cstr.data(); *ch; ch++ ) {
    if ( *ch == '\n' ) {
      mData[i] = '\r';
      i++;
    }
    mData[i] = *ch; i++;
  }
  jd.data = mData;
  jd.msgList.append( mMsg );

  mMsg->setTransferInProgress(true);
  KIO::SimpleJob *simpleJob = KIO::put(url, 0, KIO::HideProgressInfo);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect( simpleJob, SIGNAL( result(KJob *) ),
           SLOT( slotPutMessageResult(KJob *) ) );
  connect( simpleJob, SIGNAL( dataReq(KIO::Job *, QByteArray &) ),
           SLOT( slotPutMessageDataReq(KIO::Job *, QByteArray &) ) );
  connect( simpleJob, SIGNAL( data(KIO::Job *, const QByteArray &) ),
           mFolder, SLOT( slotSimpleData(KIO::Job *, const QByteArray &) ) );
  connect( simpleJob, SIGNAL(infoMessage(KJob *, const QString &, const QString &)),
             SLOT(slotPutMessageInfoData(KJob *, const QString &, const QString &)) );

}

//-----------------------------------------------------------------------------
// TODO: port to KIO::StoredTransferJob once it's ok to require kdelibs-3.3
void CachedImapJob::slotPutMessageDataReq(KIO::Job *job, QByteArray &data)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }
  if ((*it).data.size() - (*it).offset > 0x8000) {
    data = QByteArray((*it).data.data() + (*it).offset, 0x8000);
    (*it).offset += 0x8000;
  } else if ((*it).data.size() - (*it).offset > 0) {
    data = QByteArray((*it).data.data() + (*it).offset,
                   (*it).data.size() - (*it).offset);
    (*it).offset = (*it).data.size();
  } else
    data.resize(0);
}

//----------------------------------------------------------------------------
void CachedImapJob::slotPutMessageInfoData(KJob *job, const QString &data, const QString &)
{
  KMFolderCachedImap * imapFolder = static_cast<KMFolderCachedImap*>(mDestFolder->storage());
  KMAcctCachedImap *account = imapFolder->account();
  ImapAccountBase::JobIterator it = account->findJob( static_cast<KIO::Job*>(job) );
  if ( it == account->jobsEnd() ) return;

  if ( data.contains("UID") && mMsg )
  {
    ulong uid = (data.right(data.length()-4)).toLong();
    kDebug() << "Server told us uid is:" << uid;
    mMsg->setUID( uid );
    Q_ASSERT( mMsg->UID() == uid );
  }
}


//-----------------------------------------------------------------------------
void CachedImapJob::slotPutMessageResult(KJob *job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }

  if ( job->error() ) {
    bool cont = mAccount->handlePutError( static_cast<KIO::Job*>(job), *it, mFolder->folder() );
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
      * If we have acquired a uid during upload the server supports the uidnext
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


void CachedImapJob::slotAddNextSubfolder( KJob * job )
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }

    // make copy of setting, to reset it before potentially destroying 'it'
    bool silentUpload = static_cast<KMFolderCachedImap*>((*it).parent->storage())->silentUpload();
    static_cast<KMFolderCachedImap*>((*it).parent->storage())->setSilentUpload( false );

    if ( job->error() && !silentUpload ) {
      QString myError = "<p><b>" + i18n("Error while uploading folder")
        + "</b></p><p>" + i18n("Could not make the folder <b>%1</b> on the server.", (*it).items[0])
        + "</p><p>" + i18n("This could be because you do not have permission to do this, or because the folder is already present on the server; the error message from the server communication is here:") + "</p>";
      mAccount->handleJobError( static_cast<KIO::Job*>(job), myError );
    }

    if( job->error() ) {
      delete this;
      return;
    } else {
      KMFolderCachedImap* storage = static_cast<KMFolderCachedImap*>( (*it).current->storage() );
      KMFolderCachedImap* parentStorage = static_cast<KMFolderCachedImap*>( (*it).parent->storage() );
      Q_ASSERT( storage );
      Q_ASSERT( parentStorage );
      if ( storage->imapPath().isEmpty() ) {
        QString path = mAccount->createImapPath( parentStorage->imapPath(), storage->folder()->name() );
        if ( !storage->imapPathForCreation().isEmpty() )
          path = storage->imapPathForCreation();
        storage->setImapPath( path );
        storage->writeConfig();
      }
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
  KUrl url = mAccount->getUrl();
  QString path = mAccount->createImapPath( mFolder->imapPath(),
      folder->folder()->name() );
  if ( !folder->imapPathForCreation().isEmpty() ) {
    // the folder knows it's namespace
    path = folder->imapPathForCreation();
  }
  url.setPath( path );

  if ( mAccount->groupwareType() != KMAcctCachedImap::GroupwareScalix ) {
    // Associate the jobData with the parent folder, not with the child
    // This is necessary in case of an error while creating the subfolder,
    // so that folderComplete is called on the parent (and the sync is reset).
    ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
    jd.items << folder->label(); // for the err msg
    jd.current = folder->folder();
    KIO::SimpleJob *simpleJob = KIO::mkdir(url);
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
    mAccount->insertJob(simpleJob, jd);
    connect( simpleJob, SIGNAL(result(KJob *)),
             this, SLOT(slotAddNextSubfolder(KJob *)) );
  } else {
    QByteArray packedArgs;
    QDataStream stream( &packedArgs, QIODevice::WriteOnly );

    const QString command = QString( "X-CREATE-SPECIAL" );
    const QString argument = QString( "%1 %2" ).arg( Scalix::Utils::contentsTypeToScalixId( folder->contentsType() ) )
                                               .arg( path );

    stream << (int) 'X' << 'N' << command << argument;

    ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
    jd.items << folder->label(); // for the err msg
    jd.current = folder->folder();
    KIO::SimpleJob *simpleJob = KIO::special( url.url(), packedArgs, false );
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
    mAccount->insertJob(simpleJob, jd);
    connect( simpleJob, SIGNAL(result(KJob *)),
             this, SLOT(slotAddNextSubfolder(KJob *)) );
  }
}


void CachedImapJob::slotDeleteNextFolder( KJob *job )
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }

    mAccount->removeDeletedFolder( (*it).path );

    if( job->error() ) {
      mAccount->handleJobError( static_cast<KIO::Job*>(job), i18n( "Error while deleting folder %1 on the server: ", (*it).path ) + '\n' );
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

  QString folderPath = mFoldersOrMessages.front();
  mFoldersOrMessages.pop_front();
  KUrl url = mAccount->getUrl();
  url.setPath(folderPath);
  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.path = url.path();
  KIO::SimpleJob *simpleJob = KIO::file_delete(url, KIO::HideProgressInfo);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect( simpleJob, SIGNAL( result(KJob *) ),
           SLOT( slotDeleteNextFolder(KJob *) ) );
}

void CachedImapJob::checkUidValidity()
{
  KUrl url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + ";UID=0:0" );

  ImapAccountBase::jobData jd( url.url(), mFolder->folder() );
  jd.cancellable = true;

  KIO::SimpleJob *job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL(result(KJob *)),
           SLOT(slotCheckUidValidityResult(KJob *)) );
  connect( job, SIGNAL(data(KIO::Job *, const QByteArray &)),
           mFolder, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
}

void CachedImapJob::slotCheckUidValidityResult(KJob * job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }

  if( job->error() ) {
    mErrorCode = job->error();
    mAccount->handleJobError( static_cast<KIO::Job*>(job), i18n( "Error while reading folder %1 on the server: ", (*it).parent->label() ) + '\n' );
    delete this;
    return;
  }

  // Check the uidValidity
  QByteArray cstr((*it).data.data(), (*it).data.size());
  int a = cstr.indexOf("X-uidValidity: ");
  if (a < 0) {
    // Something is seriously rotten here!
    // TODO: Tell the user that he has a problem
    kDebug() <<"No uidvalidity available for folder"
                  << mFolder->objectName();
  }
  else {
    int b = cstr.indexOf("\r\n", a);
    if ( (b - a - 15) >= 0 ) {
      QString uidv = cstr.mid(a + 15, b - a - 15);
      // kDebug() <<"New uidv =" << uidv <<", old uidv ="
      //               << mFolder->uidValidity();
      if( !mFolder->uidValidity().isEmpty() && mFolder->uidValidity() != uidv ) {
        // kDebug() <<"Expunging the mailbox" << mFolder->name()
        //               << "!";
        mFolder->expunge();
        mFolder->open( "cachedimap" ); // reopen after the forced close by expunge() for KMFolderCachedImap
        mFolder->setLastUid( 0 );
        mFolder->clearUidMap();
      }
    } else
      kDebug() <<"No uidvalidity available for folder"
                    << mFolder->objectName();
  }

  a = cstr.indexOf( "X-PermanentFlags: " );
  if ( a < 0 ) {
    kDebug() << "no PERMANENTFLAGS response? assumming custom flags are not available";
  } else {
    int b = cstr.indexOf( "\r\n", a );
    if ( (b - a - 18) >= 0 ) {
      int flags = cstr.mid( a + 18, b - a - 18 ).toInt();
      emit permanentFlags( flags );
    } else {
      kDebug() << "PERMANENTFLAGS response broken, assumming custom flags are not available";
    }
  }

  mAccount->removeJob(it);
  delete this;
}


void CachedImapJob::renameFolder( const QString &newName )
{
  // Set the source URL
  KUrl urlSrc = mAccount->getUrl();
  urlSrc.setPath( mFolder->imapPath() );

  // Set the destination URL - this is a bit trickier
  KUrl urlDst = mAccount->getUrl();
  QString imapPath( mFolder->imapPath() );
  // Destination url = old imappath - oldname + new name
  imapPath.truncate( imapPath.length() - mFolder->folder()->name().length() - 1);
  imapPath += newName + '/';
  urlDst.setPath( imapPath );

  ImapAccountBase::jobData jd( newName, mFolder->folder() );
  jd.path = imapPath;

  KIO::SimpleJob *simpleJob = KIO::rename( urlSrc, urlDst, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), simpleJob );
  mAccount->insertJob( simpleJob, jd );
  connect( simpleJob, SIGNAL(result(KJob *)),
           SLOT(slotRenameFolderResult(KJob *)) );
}

static void renameChildFolders( KMFolderDir* dir, const QString& oldPath,
                                const QString& newPath )
{
  if( dir ) {
    QList<KMFolderNode*>::const_iterator it;
    for ( it = dir->constBegin(); it != dir->constEnd(); ++it ) {
      KMFolderNode *node = *it;
      if( !node->isDir() ) {
        KMFolderCachedImap* imapFolder =
          static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
        if ( !imapFolder->imapPath().isEmpty() )
          // Only rename folders that have been accepted by the server
          if( imapFolder->imapPath().startsWith( oldPath ) ) {
            QString p = imapFolder->imapPath();
            p = p.mid( oldPath.length() );
            p.prepend( newPath );
            imapFolder->setImapPath( p );
            renameChildFolders( imapFolder->folder()->child(), oldPath, newPath );
          }
      }
    }
  }
}

void CachedImapJob::slotRenameFolderResult( KJob *job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
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
    mAccount->handleJobError( static_cast<KIO::Job*>(job), i18n( "Error while trying to rename folder %1", mFolder->label() ) + '\n' );
  } else {
    // Okay, the folder seems to be renamed on the server,
    // now rename it on disk
    QString oldName = mFolder->objectName();
    QString oldPath = mFolder->imapPath();
    mAccount->removeRenamedFolder( oldPath );
    mFolder->setImapPath( (*it).path );
    mFolder->FolderStorage::rename( (*it).url );

    if( oldPath.endsWith( '/' ) ) oldPath.truncate( oldPath.length() -1 );
    QString newPath = mFolder->imapPath();
    if( newPath.endsWith( '/' ) ) newPath.truncate( newPath.length() -1 );
    renameChildFolders( mFolder->folder()->child(), oldPath, newPath );
    kmkernel->dimapFolderMgr()->contentsChanged();

    mAccount->removeJob(it);
  }
  delete this;
}

void CachedImapJob::slotListMessagesResult( KJob * job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(static_cast<KIO::Job*>(job));
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }

  if (job->error()) {
    mErrorCode = job->error();
    mAccount->handleJobError( static_cast<KIO::Job*>(job), i18n( "Error while deleting messages on the server: " ) + '\n' );
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
