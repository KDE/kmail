/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2002-2003 Zack Rusin <zack@kde.org>
 *                2000-2002 Michael Haeckel <haeckel@kde.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "imapjob.h"
#include "kmfolderimap.h"
#include "kmfolder.h"
#include "kmmsgpart.h"
#include "progressmanager.h"
using KPIM::ProgressManager;
#include "util.h"

#include <qstylesheet.h>
#include <kio/scheduler.h>
#include <kdebug.h>
#include <klocale.h>
#include <mimelib/body.h>
#include <mimelib/bodypart.h>
#include <mimelib/string.h>


namespace KMail {

//-----------------------------------------------------------------------------
ImapJob::ImapJob( KMMessage *msg, JobType jt, KMFolderImap* folder,
    QString partSpecifier, const AttachmentStrategy *as )
  : FolderJob( msg, jt, folder? folder->folder() : 0, partSpecifier ),
    mAttachmentStrategy( as ), mParentProgressItem(0)
{
}

//-----------------------------------------------------------------------------
ImapJob::ImapJob( QPtrList<KMMessage>& msgList, QString sets, JobType jt,
                  KMFolderImap* folder )
  : FolderJob( msgList, sets, jt, folder? folder->folder() : 0 ),
    mAttachmentStrategy ( 0 ), mParentProgressItem(0)
{
}

void ImapJob::init( JobType jt, QString sets, KMFolderImap* folder,
                    QPtrList<KMMessage>& msgList )
{
  mJob = 0;

  assert(jt == tGetMessage || folder);
  KMMessage* msg = msgList.first();
   // guard against empty list
  if ( !msg ) {
    deleteLater();
    return;
  }
  mType = jt;
  mDestFolder = folder? folder->folder() : 0;
  // refcount++
  if (folder) {
    folder->open("imapjobdest");
  }
  KMFolder *msg_parent = msg->parent();
  if (msg_parent) {
    msg_parent->open("imapjobsrc");
  }
  mSrcFolder = msg_parent;
  // If there is a destination folder, this is a copy, move or put to an
  // imap folder, use its account for keeping track of the job. Otherwise,
  // this is a get job and the src folder is an imap one. Use its account
  // then.
  KMAcctImap *account = 0;
  if (folder) {
    account = folder->account();
  } else {
    if ( msg_parent && msg_parent->storage() )
      account = static_cast<KMFolderImap*>(msg_parent->storage())->account();
  }
  if ( !account ||
       account->makeConnection() == ImapAccountBase::Error ) {
    deleteLater();
    return;
  }
  account->mJobList.append( this );
  if ( jt == tPutMessage )
  {
    // transfers the complete message to the server
    QPtrListIterator<KMMessage> it( msgList );
    KMMessage* curMsg;
    while ( ( curMsg = it.current() ) != 0 )
    {
      ++it;
      if ( mSrcFolder && !curMsg->isMessage() )
      {
        int idx = mSrcFolder->find( curMsg );
        curMsg = mSrcFolder->getMsg( idx );
      }
      KURL url = account->getUrl();
      QString flags = KMFolderImap::statusToFlags( curMsg->status(), folder->permanentFlags() );
      url.setPath( folder->imapPath() + ";SECTION=" + flags );
      ImapAccountBase::jobData jd;
      jd.parent = 0; jd.offset = 0; jd.done = 0;
      jd.total = ( curMsg->msgSizeServer() > 0 ) ?
        curMsg->msgSizeServer() : curMsg->msgSize();
      jd.msgList.append( curMsg );
      QCString cstr( curMsg->asString() );
      int a = cstr.find("\nX-UID: ");
      int b = cstr.find('\n', a);
      if (a != -1 && b != -1 && cstr.find("\n\n") > a) cstr.remove(a, b-a);
      jd.data.resize( cstr.length() + cstr.contains( "\n" ) - cstr.contains( "\r\n" ) );
      unsigned int i = 0;
      char prevChar = '\0';
      // according to RFC 2060 we need CRLF
      for ( char *ch = cstr.data(); *ch; ch++ )
      {
        if ( *ch == '\n' && (prevChar != '\r') ) {
          jd.data.at( i ) = '\r';
          i++;
        }
        jd.data.at( i ) = *ch;
        prevChar = *ch;
        i++;
      }
      jd.progressItem = ProgressManager::createProgressItem(
          mParentProgressItem,
          "ImapJobUploading"+ProgressManager::getUniqueID(),
          i18n("Uploading message data"),
          QStyleSheet::escape( curMsg->subject() ),
          true,
          account->useSSL() || account->useTLS() );
      jd.progressItem->setTotalItems( jd.total );
      connect ( jd.progressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem*)),
          account, SLOT( slotAbortRequested( KPIM::ProgressItem* ) ) );
      KIO::SimpleJob *job = KIO::put( url, 0, false, false, false );
      KIO::Scheduler::assignJobToSlave( account->slave(), job );
      account->insertJob( job, jd );
      connect( job, SIGNAL(result(KIO::Job *)),
          SLOT(slotPutMessageResult(KIO::Job *)) );
      connect( job, SIGNAL(dataReq(KIO::Job *, QByteArray &)),
          SLOT(slotPutMessageDataReq(KIO::Job *, QByteArray &)) );
      connect( job, SIGNAL(infoMessage(KIO::Job *, const QString &)),
          SLOT(slotPutMessageInfoData(KIO::Job *, const QString &)) );
      connect( job, SIGNAL(processedSize(KIO::Job *, KIO::filesize_t)),
          SLOT(slotProcessedSize(KIO::Job *, KIO::filesize_t)));
    }
  }
  else if ( jt == tCopyMessage || jt == tMoveMessage )
  {
    KURL url = account->getUrl();
    KURL destUrl = account->getUrl();
    destUrl.setPath(folder->imapPath());
    KMFolderImap *imapDestFolder = static_cast<KMFolderImap*>(msg_parent->storage());
    url.setPath( imapDestFolder->imapPath() + ";UID=" + sets );
    ImapAccountBase::jobData jd;
    jd.parent = 0; jd.offset = 0;
    jd.total = 1; jd.done = 0;
    jd.msgList = msgList;

    QByteArray packedArgs;
    QDataStream stream( packedArgs, IO_WriteOnly );

    stream << (int) 'C' << url << destUrl;
    jd.progressItem = ProgressManager::createProgressItem(
                          mParentProgressItem,
                          "ImapJobCopyMove"+ProgressManager::getUniqueID(),
                          i18n("Server operation"),
                          i18n("Source folder: %1 - Destination folder: %2")
                            .arg( QStyleSheet::escape( msg_parent->prettyURL() ),
                                  QStyleSheet::escape( mDestFolder->prettyURL() ) ),
                          true,
                          account->useSSL() || account->useTLS() );
    jd.progressItem->setTotalItems( jd.total );
    connect ( jd.progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
              account, SLOT( slotAbortRequested(KPIM::ProgressItem* ) ) );
    KIO::SimpleJob *simpleJob = KIO::special( url, packedArgs, false );
    KIO::Scheduler::assignJobToSlave( account->slave(), simpleJob );
    mJob = simpleJob;
    account->insertJob( mJob, jd );
    connect( mJob, SIGNAL(result(KIO::Job *)),
             SLOT(slotCopyMessageResult(KIO::Job *)) );
    if ( jt == tMoveMessage )
    {
      connect( mJob, SIGNAL(infoMessage(KIO::Job *, const QString &)),
               SLOT(slotCopyMessageInfoData(KIO::Job *, const QString &)) );
    }
  }
  else {
    slotGetNextMessage();
  }
}


//-----------------------------------------------------------------------------
ImapJob::~ImapJob()
{
  if ( mDestFolder )
  {
    KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder->storage())->account();
    if ( account ) {
      if ( mJob ) {
        ImapAccountBase::JobIterator it = account->findJob( mJob );
        if ( it != account->jobsEnd() ) {
          if( (*it).progressItem ) {
            (*it).progressItem->setComplete();
            (*it).progressItem = 0;
          }
          if ( !(*it).msgList.isEmpty() ) {
            for ( QPtrListIterator<KMMessage> mit( (*it).msgList ); mit.current(); ++mit )
              mit.current()->setTransferInProgress( false );
          }
        }
        account->removeJob( mJob );
      }
      account->mJobList.remove( this );
    }
    mDestFolder->close("imapjobdest");
  }

  if ( mSrcFolder ) {
    if (!mDestFolder || mDestFolder != mSrcFolder) {
      if (! (mSrcFolder->folderType() == KMFolderTypeImap) ) return;
      KMAcctImap *account = static_cast<KMFolderImap*>(mSrcFolder->storage())->account();
      if ( account ) {
        if ( mJob ) {
          ImapAccountBase::JobIterator it = account->findJob( mJob );
          if ( it != account->jobsEnd() ) {
            if( (*it).progressItem ) {
              (*it).progressItem->setComplete();
              (*it).progressItem = 0;
            }
            if ( !(*it).msgList.isEmpty() ) {
              for ( QPtrListIterator<KMMessage> mit( (*it).msgList ); mit.current(); ++mit )
                mit.current()->setTransferInProgress( false );
            }
          }
          account->removeJob( mJob ); // remove the associated kio job
        }
        account->mJobList.remove( this ); // remove the folderjob
      }
    }
    mSrcFolder->close("imapjobsrc");
  }
}


//-----------------------------------------------------------------------------
void ImapJob::slotGetNextMessage()
{
  KMMessage *msg = mMsgList.first();
  KMFolderImap *msgParent = msg ? static_cast<KMFolderImap*>(msg->storage()) : 0;
  if ( !msgParent || !msg || msg->UID() == 0 )
  {
    // broken message
    emit messageRetrieved( 0 );
    deleteLater();
    return;
  }
  KMAcctImap *account = msgParent->account();
  KURL url = account->getUrl();
  QString path = msgParent->imapPath() + ";UID=" + QString::number(msg->UID());
  ImapAccountBase::jobData jd;
  jd.parent = 0; jd.offset = 0;
  jd.total = 1; jd.done = 0;
  jd.msgList.append( msg );
  if ( !mPartSpecifier.isEmpty() )
  {
    if ( mPartSpecifier.find ("STRUCTURE", 0, false) != -1 ) {
      path += ";SECTION=STRUCTURE";
    } else if ( mPartSpecifier == "HEADER" ) {
      path += ";SECTION=HEADER";
    } else {
      path += ";SECTION=BODY.PEEK[" + mPartSpecifier + "]";
      DwBodyPart * part = msg->findDwBodyPart( msg->getFirstDwBodyPart(), mPartSpecifier );
      if (part)
        jd.total = part->BodySize();
    }
  } else {
      path += ";SECTION=BODY.PEEK[]";
      if (msg->msgSizeServer() > 0)
        jd.total = msg->msgSizeServer();
  }
  url.setPath( path );
//  kdDebug(5006) << "ImapJob::slotGetNextMessage - retrieve " << url.path() << endl;
  // protect the message, otherwise we'll get crashes afterwards
  msg->setTransferInProgress( true );
  jd.progressItem = ProgressManager::createProgressItem(
                          mParentProgressItem,
                          "ImapJobDownloading"+ProgressManager::getUniqueID(),
                          i18n("Downloading message data"),
                          i18n("Message with subject: ") +
                          QStyleSheet::escape( msg->subject() ),
                          true,
                          account->useSSL() || account->useTLS() );
  connect ( jd.progressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem*)),
            account, SLOT( slotAbortRequested( KPIM::ProgressItem* ) ) );
  jd.progressItem->setTotalItems( jd.total );

  KIO::SimpleJob *simpleJob = KIO::get( url, false, false );
  KIO::Scheduler::assignJobToSlave( account->slave(), simpleJob );
  mJob = simpleJob;
  account->insertJob( mJob, jd );
  if ( mPartSpecifier.find( "STRUCTURE", 0, false ) != -1 )
  {
    connect( mJob, SIGNAL(result(KIO::Job *)),
             this, SLOT(slotGetBodyStructureResult(KIO::Job *)) );
  } else {
    connect( mJob, SIGNAL(result(KIO::Job *)),
             this, SLOT(slotGetMessageResult(KIO::Job *)) );
  }
  connect( mJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
           msgParent, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)) );
  if ( jd.total > 1 )
  {
    connect(mJob, SIGNAL(processedSize(KIO::Job *, KIO::filesize_t)),
        this, SLOT(slotProcessedSize(KIO::Job *, KIO::filesize_t)));
  }
}


//-----------------------------------------------------------------------------
void ImapJob::slotGetMessageResult( KIO::Job * job )
{
  KMMessage *msg = mMsgList.first();
  if (!msg || !msg->parent() || !job) {
    emit messageRetrieved( 0 );
    deleteLater();
    return;
  }
  KMFolderImap* parent = static_cast<KMFolderImap*>(msg->storage());
  if (msg->transferInProgress())
    msg->setTransferInProgress( false );
  KMAcctImap *account = parent->account();
  if ( !account ) {
    emit messageRetrieved( 0 );
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  bool gotData = true;
  if (job->error())
  {
    QString errorStr = i18n( "Error while retrieving messages from the server." );
    if ( (*it).progressItem )
      (*it).progressItem->setStatus( errorStr );
    account->handleJobError( job, errorStr );
    return;
  } else {
    if ((*it).data.size() > 0)
    {
      kdDebug(5006) << "ImapJob::slotGetMessageResult - retrieved part " << mPartSpecifier << endl;
      if ( mPartSpecifier.isEmpty() ||
           mPartSpecifier == "HEADER" )
      {
        uint size = msg->msgSizeServer();
        if ( size > 0 && mPartSpecifier.isEmpty() )
          (*it).done = size;
        ulong uid = msg->UID();
        // must set this first so that msg->fromByteArray sets the attachment status
        if ( mPartSpecifier.isEmpty() )
          msg->setComplete( true );
        else
          msg->setReadyToShow( false );

        // Convert CR/LF to LF.
        size_t dataSize = (*it).data.size();
        dataSize = Util::crlf2lf( (*it).data.data(), dataSize ); // always <=
        (*it).data.resize( dataSize );

        // During the construction of the message from the byteArray it does
        // not have a uid. Therefore we have to make sure that no connected
        // slots are called, since they would operate on uid == 0.
        msg->parent()->storage()->blockSignals( true );
        msg->fromByteArray( (*it).data );
        // now let others react
        msg->parent()->storage()->blockSignals( false );
        if ( size > 0 && msg->msgSizeServer() == 0 ) {
          msg->setMsgSizeServer(size);
        }
        // reconstruct the UID as it gets overwritten above
        msg->setUID(uid);

      } else {
        // Convert CR/LF to LF.
        size_t dataSize = (*it).data.size();
        dataSize = Util::crlf2lf( (*it).data.data(), dataSize ); // always <=
        (*it).data.resize( dataSize );

        // Update the body of the retrieved part (the message notifies all observers)
        msg->updateBodyPart( mPartSpecifier, (*it).data );
        msg->setReadyToShow( true );
        // Update the attachment state, we have to do this for every part as we actually
        // do not know if the message has no attachment or we simply did not load the header
        if (msg->attachmentState() != KMMsgHasAttachment)
          msg->updateAttachmentState();
      }
    } else {
      kdDebug(5006) << "ImapJob::slotGetMessageResult - got no data for " << mPartSpecifier << endl;
      gotData = false;
      msg->setReadyToShow( true );
      // nevertheless give visual feedback
      msg->notify();
    }
  }
  if (account->slave()) {
      account->removeJob(it);
      account->mJobList.remove(this);
  }
  /* This needs to be emitted last, so the slots that are hooked to it
   * don't unGetMsg the msg before we have finished. */
  if ( mPartSpecifier.isEmpty() ||
       mPartSpecifier == "HEADER" )
  {
    if ( gotData )
      emit messageRetrieved(msg);
    else
    {
      /* we got an answer but not data
       * this means that the msg is not on the server anymore so delete it */
      emit messageRetrieved( 0 );
      parent->ignoreJobsForMessage( msg );
      int idx = parent->find( msg );
      if (idx != -1) parent->removeMsg( idx, true );
      // the removeMsg will unGet the message, which will delete all
      // jobs, including this one
      return;
    }
  } else {
    emit messageUpdated(msg, mPartSpecifier);
  }
  deleteLater();
}

//-----------------------------------------------------------------------------
void ImapJob::slotGetBodyStructureResult( KIO::Job * job )
{
  KMMessage *msg = mMsgList.first();
  if (!msg || !msg->parent() || !job) {
    deleteLater();
    return;
  }
  KMFolderImap* parent = static_cast<KMFolderImap*>(msg->storage());
  if (msg->transferInProgress())
    msg->setTransferInProgress( false );
  KMAcctImap *account = parent->account();
  if ( !account ) {
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;


  if (job->error())
  {
    account->handleJobError( job, i18n( "Error while retrieving information on the structure of a message." ) );
    return;
  } else {
    if ((*it).data.size() > 0)
    {
      QDataStream stream( (*it).data, IO_ReadOnly );
      account->handleBodyStructure(stream, msg, mAttachmentStrategy);
    }
  }
  if (account->slave()) {
      account->removeJob(it);
      account->mJobList.remove(this);
  }
  deleteLater();
}

//-----------------------------------------------------------------------------
void ImapJob::slotPutMessageDataReq( KIO::Job *job, QByteArray &data )
{
  KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder->storage())->account();
  if ( !account )
  {
    emit finished();
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if ((*it).data.size() - (*it).offset > 0x8000)
  {
    data.duplicate((*it).data.data() + (*it).offset, 0x8000);
    (*it).offset += 0x8000;
  }
  else if ((*it).data.size() - (*it).offset > 0)
  {
    data.duplicate((*it).data.data() + (*it).offset, (*it).data.size() - (*it).offset);
    (*it).offset = (*it).data.size();
  } else data.resize(0);
}


//-----------------------------------------------------------------------------
void ImapJob::slotPutMessageResult( KIO::Job *job )
{
  KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder->storage())->account();
  if ( !account )
  {
    emit finished();
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;
  bool deleteMe = false;
  if (job->error())
  {
    if ( (*it).progressItem )
      (*it).progressItem->setStatus( i18n("Uploading message data failed.") );
    account->handlePutError( job, *it, mDestFolder );
    return;
  } else {
    if ( (*it).progressItem )
      (*it).progressItem->setStatus( i18n("Uploading message data completed.") );
    if ( mParentProgressItem )
    {
      mParentProgressItem->incCompletedItems();
      mParentProgressItem->updateProgress();
    }
    KMMessage *msg = (*it).msgList.first();
    emit messageStored( msg );
    if ( msg == mMsgList.getLast() )
    {
      emit messageCopied( mMsgList );
      if (account->slave()) {
        account->mJobList.remove( this );
      }
      deleteMe = true;
    }
  }
  if (account->slave()) {
    account->removeJob( it ); // also clears progressitem
  }
  if ( deleteMe )
    deleteLater();
}

//-----------------------------------------------------------------------------
void ImapJob::slotCopyMessageInfoData(KIO::Job * job, const QString & data)
{
  KMFolderImap * imapFolder = static_cast<KMFolderImap*>(mDestFolder->storage());
  KMAcctImap *account = imapFolder->account();
  if ( !account )
  {
    emit finished();
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if (data.find("UID") != -1)
  {
    // split
    QString oldUid = data.section(' ', 1, 1);
    QString newUid = data.section(' ', 2, 2);

    // get lists of uids
    QValueList<ulong> olduids = KMFolderImap::splitSets(oldUid);
    QValueList<ulong> newuids = KMFolderImap::splitSets(newUid);

    int index = -1;
    KMMessage * msg;
    for ( msg = (*it).msgList.first(); msg; msg = (*it).msgList.next() )
    {
      ulong uid = msg->UID();
      index = olduids.findIndex(uid);
      if (index > -1)
      {
        // found, get the new uid
        imapFolder->saveMsgMetaData( msg, newuids[index] );
      }
    }
  }
}

//----------------------------------------------------------------------------
void ImapJob::slotPutMessageInfoData(KIO::Job *job, const QString &data)
{
  KMFolderImap * imapFolder = static_cast<KMFolderImap*>(mDestFolder->storage());
  KMAcctImap *account = imapFolder->account();
  if ( !account )
  {
    emit finished();
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if ( data.find("UID") != -1 )
  {
    ulong uid = ( data.right(data.length()-4) ).toInt();
    if ( !(*it).msgList.isEmpty() )
    {
      imapFolder->saveMsgMetaData( (*it).msgList.first(), uid );
    }
  }
}


//-----------------------------------------------------------------------------
void ImapJob::slotCopyMessageResult( KIO::Job *job )
{
  KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder->storage())->account();
  if ( !account )
  {
    emit finished();
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if (job->error())
  {
    mErrorCode = job->error();
    QString errStr = i18n("Error while copying messages.");
    if ( (*it).progressItem )
      (*it).progressItem->setStatus( errStr );
    if ( account->handleJobError( job, errStr  ) )
      deleteLater();
    return;
  } else {
    if ( !(*it).msgList.isEmpty() )
    {
      emit messageCopied((*it).msgList);
    } else if (mMsgList.first()) {
      emit messageCopied(mMsgList.first());
    }
  }
  if (account->slave()) {
    account->removeJob(it);
    account->mJobList.remove(this);
  }
  deleteLater();
}

//-----------------------------------------------------------------------------
void ImapJob::execute()
{
  init( mType, mSets, mDestFolder?
        dynamic_cast<KMFolderImap*>( mDestFolder->storage() ):0, mMsgList );
}

//-----------------------------------------------------------------------------
void ImapJob::setParentFolder( const KMFolderImap* parent )
{
  mParentFolder = const_cast<KMFolderImap*>( parent );
}

//-----------------------------------------------------------------------------
void ImapJob::slotProcessedSize(KIO::Job * job, KIO::filesize_t processed)
{
  KMMessage *msg = mMsgList.first();
  if (!msg || !job) {
    return;
  }
  KMFolderImap* parent = 0;
  if ( msg->parent() && msg->parent()->folderType() == KMFolderTypeImap )
    parent = static_cast<KMFolderImap*>(msg->parent()->storage());
  else if (mDestFolder) // put
    parent = static_cast<KMFolderImap*>(mDestFolder->storage());
  if (!parent) return;
  KMAcctImap *account = parent->account();
  if ( !account ) return;
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;
  (*it).done = processed;
  if ( (*it).progressItem ) {
    (*it).progressItem->setCompletedItems( processed );
    (*it).progressItem->updateProgress();
  }
  emit progress( (*it).done, (*it).total );
}

}//namespace KMail

#include "imapjob.moc"
