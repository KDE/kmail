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

#include "imapjob.h"
#include "kmfolderimap.h"
#include "kmfolder.h"
#include "kmmsgpart.h"
#include "messageproperty.h"
#include "progressmanager.h"
using KPIM::ProgressManager;
#include "util.h"

#include <kio/scheduler.h>
#include <kdebug.h>
#include <klocale.h>
#include <mimelib/body.h>
#include <mimelib/bodypart.h>
#include <mimelib/string.h>

#include <QList>
#include <QTextDocument>
#include <QByteArray>

namespace KMail {

//-----------------------------------------------------------------------------
ImapJob::ImapJob( KMMessage *msg, JobType jt, KMFolderImap* folder,
    const QString &partSpecifier, const AttachmentStrategy *as )
  : FolderJob( msg, jt, folder? folder->folder() : 0, partSpecifier ),
    mAttachmentStrategy( as ), mParentProgressItem(0)
{
}

//-----------------------------------------------------------------------------
ImapJob::ImapJob( QList<KMMessage*>& msgList, const QString &sets, JobType jt,
                  KMFolderImap* folder )
  : FolderJob( msgList, sets, jt, folder? folder->folder() : 0 ),
    mAttachmentStrategy ( 0 ), mParentProgressItem(0)
{
}

void ImapJob::init( JobType jt, const QString &sets, KMFolderImap *folder,
                    QList<KMMessage*> &msgList )
{
  mJob = 0;

  assert( jt == tGetMessage || folder );
  KMMessage* msg = msgList.first();
   // guard against empty list
  if ( !msg ) {
    deleteLater();
    return;
  }
  mType = jt;
  mDestFolder = folder? folder->folder() : 0;
  // refcount++
  if ( folder ) {
    folder->open( "imapjobdest" );
  }
  KMFolder *msg_parent = msg->parent();
  if ( msg_parent ) {
    msg_parent->open( "imapjobsrc" );
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
  account->registerJob( this );

  if ( jt == tPutMessage )
  {
    // transfers the complete message to the server
    QList<KMMessage*>::const_iterator it;
    KMMessage* curMsg = 0;
    for ( it = msgList.begin(); it != msgList.end(); ++it )
    {
      curMsg = (*it);
      if ( mSrcFolder && !curMsg->isMessage() )
      {
        int idx = mSrcFolder->find( curMsg );
        curMsg = mSrcFolder->getMsg( idx );
      }
      KUrl url = account->getUrl();
      QString flags = KMFolderImap::statusToFlags( curMsg->status(), folder->permanentFlags() );
      url.setPath( folder->imapPath() + ";SECTION=" + flags );
      ImapAccountBase::jobData jd;
      jd.parent = 0; jd.offset = 0; jd.done = 0;
      jd.total = ( curMsg->msgSizeServer() > 0 ) ?
        curMsg->msgSizeServer() : curMsg->msgSize();
      jd.msgList.append( curMsg );
      QByteArray cstr( curMsg->asString() );
      int a = cstr.indexOf("\nX-UID: ");
      int b = cstr.indexOf('\n', a);
      if (a != -1 && b != -1 && cstr.indexOf("\n\n") > a)
        cstr.remove(a, b-a);
      jd.data.resize( cstr.length() + cstr.count( "\n" ) - cstr.count( "\r\n" ) );
      unsigned int i = 0;
      char prevChar = '\0';
      // according to RFC 2060 we need CRLF
      for ( char *ch = cstr.data(); *ch; ch++ )
      {
        if ( *ch == '\n' && (prevChar != '\r') ) {
          jd.data[i] = '\r';
          i++;
        }
        jd.data[i] = *ch;
        prevChar = *ch;
        i++;
      }
      jd.progressItem = ProgressManager::createProgressItem(
          mParentProgressItem,
          "ImapJobUploading"+ProgressManager::getUniqueID(),
          i18n("Uploading message data"),
          curMsg->subject(),
          true,
          account->useSSL() || account->useTLS() );
      jd.progressItem->setTotalItems( jd.total );
      connect ( jd.progressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem*)),
          account, SLOT( slotAbortRequested( KPIM::ProgressItem* ) ) );
      KIO::SimpleJob *job = KIO::put( url, 0, KIO::HideProgressInfo );
      KIO::Scheduler::assignJobToSlave( account->slave(), job );
      account->insertJob( job, jd );
      connect( job, SIGNAL(result(KJob *)),
          SLOT(slotPutMessageResult(KJob *)) );
      connect( job, SIGNAL(dataReq(KIO::Job *, QByteArray &)),
          SLOT(slotPutMessageDataReq(KIO::Job *, QByteArray &)) );
      connect( job, SIGNAL(infoMessage(KJob *, const QString &, const QString &)),
          SLOT(slotPutMessageInfoData(KJob *, const QString &, const QString &)) );
      connect( job, SIGNAL( processedSize( KJob *, qulonglong ) ),
          SLOT( slotProcessedSize( KJob *, qulonglong ) ) );

      // When moving the message, we want to keep the serial number if the
      // message property is set (by the action scheduler).
      // To do this, create metadata, which is later read by
      // KMFolderImap::slotGetMessagesData().
      if ( MessageProperty::keepSerialNumber( msg->getMsgSerNum() ) ) {

        // Note that we can't rely on connecting infoMessage() to something like
        // slotCopyMessageInfoData(), since that only works when the server supports
        // UIDPLUS.
        folder->rememberSerialNumber( curMsg );

        // Forget that property again, so we don't end up keeping the serial number
        // the next time we do something with that message
        MessageProperty::setKeepSerialNumber( msg->getMsgSerNum(), false );
      }
    }
  }
  else if ( jt == tCopyMessage || jt == tMoveMessage )
  {
    KUrl url = account->getUrl();
    KUrl destUrl = account->getUrl();
    destUrl.setPath(folder->imapPath());
    KMFolderImap *imapDestFolder = static_cast<KMFolderImap*>(msg_parent->storage());
    url.setPath( imapDestFolder->imapPath() + ";UID=" + sets );
    ImapAccountBase::jobData jd;
    jd.parent = 0; jd.offset = 0;
    jd.total = 1; jd.done = 0;
    jd.msgList = msgList;

    QByteArray packedArgs;
    QDataStream stream( &packedArgs, QIODevice::WriteOnly );

    stream << (int) 'C' << url << destUrl;
    jd.progressItem = ProgressManager::createProgressItem(
                          mParentProgressItem,
                          "ImapJobCopyMove"+ProgressManager::getUniqueID(),
                          i18n("Server operation"),
                          i18n("Source folder: %1 - Destination folder: %2",
                               msg_parent->prettyUrl(),
                               mDestFolder->prettyUrl() ),
                          true,
                          account->useSSL() || account->useTLS() );
    jd.progressItem->setTotalItems( jd.total );
    connect ( jd.progressItem, SIGNAL(progressItemCanceled(KPIM::ProgressItem*)),
              account, SLOT( slotAbortRequested(KPIM::ProgressItem* ) ) );
    KIO::SimpleJob *simpleJob = KIO::special( url, packedArgs, KIO::HideProgressInfo );
    KIO::Scheduler::assignJobToSlave( account->slave(), simpleJob );
    mJob = simpleJob;
    account->insertJob( mJob, jd );
    connect( mJob, SIGNAL(result(KJob *)),
             SLOT(slotCopyMessageResult(KJob *)) );
    if ( jt == tMoveMessage )
    {
      connect( mJob, SIGNAL(infoMessage(KJob *, const QString &, const QString &)),
               SLOT(slotCopyMessageInfoData(KJob *, const QString &, const QString &)) );
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
            QList<KMMessage*>::const_iterator it2;
            for ( it2 = (*it).msgList.begin(); it2 != (*it).msgList.end(); ++it2 )
              (*it2)->setTransferInProgress( false );
          }
        }
        account->removeJob( mJob );
      }
      account->unregisterJob( this );
    }
    mDestFolder->close( "imapjobdest" );
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
              QList<KMMessage*>::const_iterator it2;
              for ( it2 = (*it).msgList.begin(); it2 != (*it).msgList.end(); ++it2 )
                (*it2)->setTransferInProgress( false );
            }
          }
          account->removeJob( mJob ); // remove the associated kio job
        }
        account->unregisterJob( this ); // remove the folderjob
      }
    }
    mSrcFolder->close( "imapjobsrc" );
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
  KUrl url = account->getUrl();
  QString path = msgParent->imapPath() + ";UID=" + QString::number(msg->UID());
  ImapAccountBase::jobData jd;
  jd.parent = 0; jd.offset = 0;
  jd.total = 1; jd.done = 0;
  jd.msgList.append( msg );
  if ( !mPartSpecifier.isEmpty() )
  {
    if ( mPartSpecifier.contains ("STRUCTURE", Qt::CaseInsensitive) ) {
      path += ";SECTION=STRUCTURE";
    } else if ( mPartSpecifier == "HEADER" ) {
      path += ";SECTION=HEADER";
    } else {
      path += ";SECTION=BODY.PEEK[" + mPartSpecifier + ']';
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
//  kDebug(5006) << "Retrieve" << url.path();
  // protect the message, otherwise we'll get crashes afterwards
  msg->setTransferInProgress( true );
  const QString escapedSubject = Qt::escape( msg->subject() );
  jd.progressItem = ProgressManager::createProgressItem(
                          mParentProgressItem,
                          "ImapJobDownloading"+ProgressManager::getUniqueID(),
                          i18n("Downloading message data"),
                          i18n("Message with subject: ") + escapedSubject,
                          true,
                          account->useSSL() || account->useTLS() );
  connect ( jd.progressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem*)),
            account, SLOT( slotAbortRequested( KPIM::ProgressItem* ) ) );
  jd.progressItem->setTotalItems( jd.total );

  KIO::SimpleJob *simpleJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
  KIO::Scheduler::assignJobToSlave( account->slave(), simpleJob );
  mJob = simpleJob;
  account->insertJob( mJob, jd );
  if ( mPartSpecifier.contains( "STRUCTURE", Qt::CaseSensitive ) )
  {
    connect( mJob, SIGNAL(result(KJob *)),
             this, SLOT(slotGetBodyStructureResult(KJob *)) );
  } else {
    connect( mJob, SIGNAL(result(KJob *)),
             this, SLOT(slotGetMessageResult(KJob *)) );
  }
  connect( mJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
           msgParent, SLOT(slotSimpleData(KIO::Job *, const QByteArray &)) );
  if ( jd.total > 1 )
  {
    connect(mJob, SIGNAL(processedSize(KJob *, qulonglong)),
        this, SLOT(slotProcessedSize(KJob *, qulonglong)));
  }
}


//-----------------------------------------------------------------------------
void ImapJob::slotGetMessageResult( KJob * job )
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
  ImapAccountBase::JobIterator it = account->findJob( static_cast<KIO::Job*>(job) );
  if ( it == account->jobsEnd() ) return;

  bool gotData = true;
  if (job->error())
  {
    QString errorStr = i18n( "Error while retrieving messages from the server." );
    if ( (*it).progressItem )
      (*it).progressItem->setStatus( errorStr );
    account->handleJobError( static_cast<KIO::Job*>(job), errorStr );
    return;
  } else {
    if ((*it).data.size() > 0)
    {
      kDebug(5006) << "Retrieved part" << mPartSpecifier;
      if ( mPartSpecifier.isEmpty() ||
           mPartSpecifier == "HEADER" )
      {
        uint size = msg->msgSizeServer();
        if ( size > 0 && mPartSpecifier.isEmpty() )
          (*it).done = size;
        ulong uid = msg->UID();
        // must set this first so that msg->fromString sets the attachment status
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
        msg->fromString( (*it).data );
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
      kDebug(5006) << "Got no data for" << mPartSpecifier;
      gotData = false;
      msg->setReadyToShow( true );
      // nevertheless give visual feedback
      msg->notify();
    }
  }
  if (account->slave()) {
      account->removeJob(it);
      account->unregisterJob(this);
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
      if ( idx != -1 ) {
        parent->removeMsg( idx, true );
        // the removeMsg will unGet the message, which will delete all
        // jobs, including this one
        return;
      }
    }
  } else {
    emit messageUpdated(msg, mPartSpecifier);
  }
  deleteLater();
}

//-----------------------------------------------------------------------------
void ImapJob::slotGetBodyStructureResult( KJob * job )
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
  ImapAccountBase::JobIterator it = account->findJob( static_cast<KIO::Job*>(job) );
  if ( it == account->jobsEnd() ) return;


  if (job->error())
  {
    account->handleJobError( static_cast<KIO::Job*>(job), i18n( "Error while retrieving information on the structure of a message." ) );
    return;
  } else {
    if ((*it).data.size() > 0)
    {
      QDataStream stream( &(*it).data, QIODevice::ReadOnly );
      account->handleBodyStructure(stream, msg, mAttachmentStrategy);
    }
  }
  if (account->slave()) {
      account->removeJob(it);
      account->unregisterJob(this);
  }
  deleteLater();
}

//-----------------------------------------------------------------------------
void ImapJob::slotPutMessageDataReq( KIO::Job *job, QByteArray &data )
{
  KMAcctImap *account =
    static_cast<KMFolderImap*>(mDestFolder->storage())->account();
  if ( !account ) {
    emit finished();
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if ((*it).data.size() - (*it).offset > 0x8000)
  {
    data = QByteArray((*it).data.data() + (*it).offset, 0x8000);
    (*it).offset += 0x8000;
  }
  else if ((*it).data.size() - (*it).offset > 0)
  {
    data = QByteArray((*it).data.data() + (*it).offset, (*it).data.size() - (*it).offset);
    (*it).offset = (*it).data.size();
  } else data.resize(0);
}


//-----------------------------------------------------------------------------
void ImapJob::slotPutMessageResult( KJob *job )
{
  KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder->storage())->account();
  ImapAccountBase::JobIterator it = account->findJob( static_cast<KIO::Job*>(job) );
  if ( it == account->jobsEnd() ) return;
  bool deleteMe = false;
  if (job->error())
  {
    if ( (*it).progressItem )
      (*it).progressItem->setStatus( i18n("Uploading message data failed.") );
    account->handlePutError( static_cast<KIO::Job*>(job), *it, mDestFolder );
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
    if ( msg == mMsgList.last() )
    {
      emit messageCopied( mMsgList );
      if (account->slave()) {
        account->unregisterJob( this );
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
void ImapJob::slotCopyMessageInfoData(KJob * job, const QString & data, const QString &)
{
  KMFolderImap * imapFolder = static_cast<KMFolderImap*>(mDestFolder->storage());
  KMAcctImap *account = imapFolder->account();
  ImapAccountBase::JobIterator it = account->findJob( static_cast<KIO::Job*>(job) );
  if ( it == account->jobsEnd() ) return;

  if (data.contains("UID") )
  {
    // split
    QString oldUid = data.section(' ', 1, 1);
    QString newUid = data.section(' ', 2, 2);

    // get lists of uids
    QList<ulong> olduids = KMFolderImap::splitSets(oldUid);
    QList<ulong> newuids = KMFolderImap::splitSets(newUid);

    int index = -1;
    KMMessage * msg;
    foreach ( msg, (*it).msgList )
    {
      ulong uid = msg->UID();
      index = olduids.indexOf(uid);
      if (index > -1)
      {
        // found, get the new uid
        imapFolder->saveMsgMetaData( msg, newuids[index] );
      }
    }
  }
}

//----------------------------------------------------------------------------
void ImapJob::slotPutMessageInfoData(KJob *job, const QString &data, const QString &)
{
  KMFolderImap * imapFolder = static_cast<KMFolderImap*>(mDestFolder->storage());
  KMAcctImap *account = imapFolder->account();
  ImapAccountBase::JobIterator it = account->findJob( static_cast<KIO::Job*>(job) );
  if ( it == account->jobsEnd() ) return;

  if ( data.contains("UID") )
  {
    ulong uid = ( data.right(data.length()-4) ).toInt();
    if ( !(*it).msgList.isEmpty() )
    {
      imapFolder->saveMsgMetaData( (*it).msgList.first(), uid );
    }
  }
}


//-----------------------------------------------------------------------------
void ImapJob::slotCopyMessageResult( KJob *job )
{
  KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder->storage())->account();
  ImapAccountBase::JobIterator it = account->findJob( static_cast<KIO::Job*>(job) );
  if ( it == account->jobsEnd() ) return;

  if (job->error())
  {
    mErrorCode = job->error();
    QString errStr = i18n("Error while copying messages.");
    if ( (*it).progressItem )
      (*it).progressItem->setStatus( errStr );
    if ( account->handleJobError( static_cast<KIO::Job*>(job), errStr  ) )
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
    account->unregisterJob(this);
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
void ImapJob::slotProcessedSize(KJob * job, qulonglong processed)
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
