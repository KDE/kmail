/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2002-2003  Bo Thorsen <bo@sonofthor.dk>
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

#include "cachedimapjob.h"

#include <kio/scheduler.h>

#include "kmfoldermgr.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>


namespace KMail {

// Get messages
CachedImapJob::CachedImapJob( const QValueList<MsgForDownload>& msgs, JobType type,
				  KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mMsgsForDownload( msgs ),
    mTotalBytes(0), mMsg(0)
{
  QValueList<MsgForDownload>::ConstIterator it = msgs.begin();
  for ( ; it != msgs.end() ; ++it )
      mTotalBytes += (*it).size;
}

// Put messages
CachedImapJob::CachedImapJob( const QPtrList<KMMessage>& msgs, JobType type,
                              KMFolderCachedImap* folder )
  : FolderJob( msgs, QString::null, type, folder ), mFolder( folder ),
    mTotalBytes(0), mMsg(0)
{
}

CachedImapJob::CachedImapJob( const QValueList<KMFolderCachedImap*>& fList,
				  JobType type, KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mFolderList( fList ), mMsg( 0 )
{
}

CachedImapJob::CachedImapJob( const QString& uids, JobType type,
                              KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder(folder), mMsg( 0 ), mString( uids )
{
  assert( folder );
}

CachedImapJob::CachedImapJob( const QStringList& folderpaths, JobType type,
                              KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mFolderPathList( folderpaths ), mMsg( 0 )
{
  assert( folder );
}

CachedImapJob::CachedImapJob( JobType type, KMFolderCachedImap* folder )
  : FolderJob( type ), mFolder( folder ), mMsg( 0 )
{
  assert( folder );
}

CachedImapJob::~CachedImapJob()
{
  /* // TODO(steffen): Handle transferinpro...
     if ( !(*it).msgList.isEmpty() ) {
     for ( KMMessage* msg = (*it).msgList.first(); msg; msg = (*it).msgList.next() )
     msg->setTransferInProgress(false);
     }
  */
  //if( mMsg ) mMsg->setTransferInProgress(false);
  mAccount->displayProgress();

  // kdDebug(5006) << "~CachedImapJob(): Removing this from joblist" << endl;
  mAccount->mJobList.remove(this);
}

void CachedImapJob::init()
{
  mSentBytes = 0;

  if( !mFolder ) {
    if( !mMsgList.isEmpty() ) {
      mFolder = static_cast<KMFolderCachedImap*>(mMsgList.first()->parent());
    }
  }
  assert( mFolder );
  mAccount = mFolder->account();
  assert( mAccount != 0 );
  if( !mAccount->makeConnection() ) {
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
  case tDeleteMessage:    deleteMessages(mString);  break;
  case tExpungeFolder:    expungeFolder();          break;
  case tAddSubfolders:    slotAddNextSubfolder();   break;
  case tDeleteFolders:    slotDeleteNextFolder();   break;
  case tCheckUidValidity: checkUidValidity();       break;
  case tRenameFolder:     renameFolder(mString);    break;
  default:
    assert( 0 );
  }
}

void CachedImapJob::deleteMessages( const QString& uids )
{
  KURL url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + QString::fromLatin1(";UID=%1").arg(uids) );

  KIO::SimpleJob *job = KIO::file_delete( url, false );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  ImapAccountBase::jobData jd( url.url(), mFolder );
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL( result(KIO::Job *) ), this, SLOT( slotDeleteResult(KIO::Job *) ) );
}

void CachedImapJob::expungeFolder()
{
  KURL url = mAccount->getUrl();
  // Special URL that means EXPUNGE
  url.setPath( mFolder->imapPath() + QString::fromLatin1(";UID=*") );

  KIO::SimpleJob *job = KIO::file_delete( url, false );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  ImapAccountBase::jobData jd( url.url(), mFolder );
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL( result(KIO::Job *) ), this, SLOT( slotDeleteResult(KIO::Job *) ) );
}

void CachedImapJob::slotDeleteResult( KIO::Job * job )
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    delete this;
    return;
  }
  mAccount->removeJob(it);

  if (job->error())
    mAccount->slotSlaveError( mAccount->slave(), job->error(), job->errorText() );

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
      mAccount->removeJob(it);
      mAccount->slotSlaveError( mAccount->slave(), job->error(), job->errorText() );
      delete this;
      return;
    }

    ulong size = 0;
    if ((*it).data.size() > 0) {
      QString uid = mMsg->headerField("X-UID");
      size = mMsg->headerField("X-Length").toULong();
      (*it).data.resize((*it).data.size() + 1);
      (*it).data[(*it).data.size() - 1] = '\0';
      mMsg->fromString(QCString((*it).data));
      //int idx = mFolder->find(mMsg);
      //if( idx >= 0 ) mFolder->take(idx);
      //else kdDebug(5006) << "weird, message not in folder!?!" << endl;
      mMsg->setHeaderField("X-UID",uid);

      mMsg->setTransferInProgress( false );
      mMsg->setComplete( true );
      mFolder->addMsgInternal(mMsg);
      emit messageRetrieved(mMsg);
      /*mFolder->unGetMsg(idx);*/ // Is this OK? /steffen
    } else {
      emit messageRetrieved(NULL);
    }
    mMsg = NULL;

    mSentBytes += size;
    emit progress( mSentBytes, mTotalBytes );
    mAccount->removeJob(it);
  }

  if( mMsgsForDownload.isEmpty() ) {
    //emit messageRetrieved(mMsg);
    delete this;
    return;
  }

  MsgForDownload mfd = mMsgsForDownload.front(); mMsgsForDownload.pop_front();
  //kdDebug() << "slotGetNextMessage downloading: uid=" << mfd.uid << " size=" << mfd.size << endl;

  mMsg = new KMMessage;
  mMsg->setHeaderField("X-UID",QString::number(mfd.uid));
  mMsg->setHeaderField("X-Length",QString::number(mfd.size));
  if( mfd.flags > 0 ) mMsg->setStatus( KMFolderCachedImap::flagsToStatus(mfd.flags) );
  KURL url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + QString(";UID=%1").arg(mfd.uid));

  ImapAccountBase::jobData jd( url.url(), mFolder );
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
  if( mMsgList.isEmpty() ) {
    delete this;
    return;
  }

  mMsg = mMsgList.first(); mMsgList.removeFirst();
  assert( mMsg );

  KURL url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + ";SECTION="
	      + QString::fromLatin1(KMFolderCachedImap::statusToFlags(mMsg->status())));
  ImapAccountBase::jobData jd( url.url(), mFolder );

  QCString cstr(mMsg->asString());
  int a = cstr.find("\nX-UID: ");
  int b = cstr.find('\n', a);
  if (a != -1 && b != -1 && cstr.find("\n\n") > a) cstr.remove(a, b-a);
  mData.resize(cstr.length() + cstr.contains('\n'));
  unsigned int i = 0;
  for( char *ch = cstr.data(); *ch; ch++ ) {
    if ( *ch == '\n' ) {
      mData.at(i) = '\r';
      i++;
    }
    mData.at(i) = *ch; i++;
  }
  jd.data = mData;

  mMsg->setTransferInProgress(true);
  KIO::SimpleJob *simpleJob = KIO::put(url, 0, false, false, false);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect( simpleJob, SIGNAL( result(KIO::Job *) ), SLOT( slotPutMessageResult(KIO::Job *) ) );
  connect( simpleJob, SIGNAL( dataReq(KIO::Job *, QByteArray &) ),
	   SLOT( slotPutMessageDataReq(KIO::Job *, QByteArray &) ) );
  connect( simpleJob, SIGNAL( data(KIO::Job *, const QByteArray &) ),
	   mFolder, SLOT( slotSimpleData(KIO::Job *, const QByteArray &) ) );
}

//-----------------------------------------------------------------------------
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
    data.duplicate((*it).data.data() + (*it).offset, (*it).data.size() - (*it).offset);
    (*it).offset = (*it).data.size();
  } else
    data.resize(0);
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
    QStringList errors = job->detailedErrorStrings();
    QString myError = "<qt><p><b>" + i18n("Error while uploading message")
      + "</b></p><p>" + i18n("Could not upload the message %1 on the server from folder %2 with URL %3.").arg((*it).items[0]).arg(mFolder->name()).arg((*it).htmlURL())
      + "</p><p>" + i18n("This could be because you don't have permission to do this. The error message from the server communication is here:") + "</p>";
    KMessageBox::error( 0, myError + errors[1] + '\n' + errors[2], errors[0] );
    mAccount->removeJob(it);
    delete this;
    return;
  }

  // kdDebug(5006) << "resulting data \"" << QCString((*it).data) << "\"" << endl;
  emit messageStored( mMsg );
  int i;
  if( ( i = mFolder->find(mMsg) ) != -1 ) {
    mFolder->quiet( true );
    mFolder->removeMsg(i);
    mFolder->quiet( false );
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

    if ( job->error() && ! static_cast<KMFolderCachedImap*>((*it).parent)->silentUpload() ) {
      QStringList errors = job->detailedErrorStrings();
      QString myError = "<qt><p><b>" + i18n("Error while uploading folder")
	+ "</b></p><p>" + i18n("Could not make the folder %1 on the server.").arg((*it).items[0])
	+ "</p><p>" + i18n("This could be because you don't have permission to do this or because the directory is already present on the server. The error message from the server communication is here:") + "</p>";
      // kdDebug(5006) << "Error messages:\n 0: " << errors[0].latin1() << "\n 1: " << errors[1].latin1() << "\n 2: " << errors[2].latin1() << endl;
      KMessageBox::error( 0, myError + errors[1] + '\n' + errors[2], errors[0] );
    }
    static_cast<KMFolderCachedImap*>((*it).parent)->setSilentUpload( false );
    mAccount->removeJob( it );

    if( job->error() ) {
      delete this;
      return;
    }
  }

  if (mFolderList.isEmpty()) {
    // No more folders to add
    delete this;
    return;
  }

  KMFolderCachedImap *folder = mFolderList.front();
  mFolderList.pop_front();
  KURL url = mAccount->getUrl();
  url.setPath(mFolder->imapPath() + folder->name());

  ImapAccountBase::jobData jd( url.url(), folder );
  KIO::SimpleJob *simpleJob = KIO::mkdir(url);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect( simpleJob, SIGNAL(result(KIO::Job *)), this, SLOT(slotAddNextSubfolder(KIO::Job *)) );
}


void CachedImapJob::slotDeleteNextFolder( KIO::Job *job )
{
  if (job) {
    KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
    if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
      delete this;
      return;
    }
    mAccount->removeJob(it);

    if( job->error() ) {
      job->showErrorDialog( 0L  ); // why not mAccount->slotSlaveError?
      delete this;
      return;
    }
  }

  if( mFolderPathList.isEmpty() ) {
    // No more folders to delete
    delete this;
    return;
  }

  QString folderPath = mFolderPathList.front(); mFolderPathList.pop_front();
  KURL url = mAccount->getUrl();
  url.setPath(folderPath);
  ImapAccountBase::jobData jd( url.url(), mFolder );
  KIO::SimpleJob *simpleJob = KIO::file_delete(url, false);
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), simpleJob);
  mAccount->insertJob(simpleJob, jd);
  connect( simpleJob, SIGNAL( result(KIO::Job *) ), SLOT( slotDeleteNextFolder(KIO::Job *) ) );
}

void CachedImapJob::checkUidValidity()
{
  KURL url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + ";UID=0:0" );

  ImapAccountBase::jobData jd( url.url(), mFolder );

  KIO::SimpleJob *job = KIO::get( url, false, false );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
  mAccount->insertJob( job, jd );
  connect( job, SIGNAL(result(KIO::Job *)), SLOT(slotCheckUidValidityResult(KIO::Job *)) );
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
    mAccount->removeJob(it);
    job->showErrorDialog( 0 ); // why not mAccount->slotSlaveError?
    delete this;
    return;
  }

  // Check the uidValidity
  QCString cstr((*it).data.data(), (*it).data.size() + 1);
  int a = cstr.find("X-uidValidity: ");
  if (a < 0) {
    // Something is seriously rotten here! TODO: Tell the user that he has a problem
    kdDebug(5006) << "No uidvalidity available for folder " << mFolder->name() << endl;
    return;
  }
  int b = cstr.find("\r\n", a);
  if ( (b - a - 15) >= 0 ) {
    QString uidv = cstr.mid(a + 15, b - a - 15);
    // kdDebug(5006) << "New uidv = " << uidv << ", old uidv = " << mFolder->uidValidity()
    // << endl;
    if( !mFolder->uidValidity().isEmpty() && mFolder->uidValidity() != uidv ) {
      // kdDebug(5006) << "Expunging the mailbox " << mFolder->name() << "!" << endl;
      mFolder->expunge();
      mFolder->setLastUid( 0 );
    }
  } else
    kdDebug(5006) << "No uidvalidity available for folder " << mFolder->name() << endl;

#if 0
  // Set access control on the folder
  a = cstr.find("X-Access: ");
  if (a >= 0) {
    b = cstr.find("\r\n", a);
    QString access;
    if ( (b - a - 10) >= 0 ) access = cstr.mid(a + 10, b - a - 10);
    mReadOnly = access == "Read only";
  }
#endif

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
  imapPath.truncate( imapPath.length() - mFolder->name().length() - 1);
  imapPath += newName + '/';
  urlDst.setPath( imapPath );

  ImapAccountBase::jobData jd( newName, mFolder );
  jd.path = imapPath;

  KIO::SimpleJob *simpleJob = KIO::rename( urlSrc, urlDst, false );
  KIO::Scheduler::assignJobToSlave( mAccount->slave(), simpleJob );
  mAccount->insertJob( simpleJob, jd );
  connect( simpleJob, SIGNAL(result(KIO::Job *)), SLOT(slotRenameFolderResult(KIO::Job *)) );
}

static void renameChildFolders( KMFolderDir* dir, const QString& oldPath, const QString& newPath )
{
  if( dir ) {
    KMFolderNode *node = dir->first();
    while( node ) {
      if( !node->isDir() ) {
	KMFolderCachedImap* imapFolder = static_cast<KMFolderCachedImap*>(node);
	if ( !imapFolder->imapPath().isEmpty() )
	  // Only rename folders that have been accepted by the server
	  if( imapFolder->imapPath().find( oldPath ) == 0 ) {
	    QString p = imapFolder->imapPath();
	    p = p.mid( oldPath.length() );
	    p.prepend( newPath );
	    imapFolder->setImapPath( p );
	    renameChildFolders( imapFolder->child(), oldPath, newPath );
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
    job->showErrorDialog( 0 ); // why not mAccount->slotSlaveError?
  } else {
    // Okay, the folder seems to be renamed on the folder. Now rename it on disk
    QString oldName = mFolder->name();
    QString oldPath = mFolder->imapPath();
    mFolder->setImapPath( (*it).path );
    mFolder->KMFolder::rename( (*it).url );

    if( oldPath.endsWith( "/" ) ) oldPath = oldPath.left( oldPath.length() -1 );
    QString newPath = mFolder->imapPath();
    if( newPath.endsWith( "/" ) ) newPath = newPath.left( newPath.length() -1 );
    renameChildFolders( mFolder->child(), oldPath, newPath );
    kernel->imapFolderMgr()->contentsChanged();
  }

  mAccount->removeJob(it);
  delete this;
}

void CachedImapJob::execute()
{
  init();
}

void CachedImapJob::expireMessages()
{
  //FIXME: not implemented yet
}

}

#include "cachedimapjob.moc"
