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

#include "imapjob.h"
#include "kmfolderimap.h"
#include "kmmsgpart.h"

#include <kio/scheduler.h>
#include <kdebug.h>
#include <mimelib/body.h>
#include <mimelib/bodypart.h>
#include <mimelib/string.h>


namespace KMail {

//-----------------------------------------------------------------------------
ImapJob::ImapJob( KMMessage *msg, JobType jt, KMFolderImap* folder,
    QString partSpecifier, const AttachmentStrategy *as )
  : FolderJob( msg, jt, folder, partSpecifier ),
    mAttachmentStrategy( as )
{
}

//-----------------------------------------------------------------------------
ImapJob::ImapJob( QPtrList<KMMessage>& msgList, QString sets, JobType jt,
                  KMFolderImap* folder )
  : FolderJob( msgList, sets, jt, folder ),
    mAttachmentStrategy ( 0 )
{
}

void ImapJob::init( JobType jt, QString sets, KMFolderImap* folder,
                    QPtrList<KMMessage>& msgList )
{
  mJob = 0;
  assert(jt == tGetMessage || folder);
  KMMessage* msg = msgList.first();
  mType = jt;
  mDestFolder = folder;
  // refcount++
  if (folder) {
    folder->open();
  }
  KMFolder *msg_parent = msg->parent();
  if (msg_parent) {
    if (!folder || folder!= msg_parent) {
      msg_parent->open();
    }
  }
  mSrcFolder = msg_parent;
  // If there is a destination folder, this is a copy, move or put to an
  // imap folder, use its account for keeping track of the job. Otherwise,
  // this is a get job and the src folder is an imap one. Use its account
  // then.
  KMAcctImap *account;
  if (folder) {
    account = folder->account();
  } else { 
    account = static_cast<KMFolderImap*>(msg_parent)->account();
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
    KURL url = account->getUrl();
    QString flags = KMFolderImap::statusToFlags( msg->status() ); 
    url.setPath( folder->imapPath() + ";SECTION=" + flags );
    ImapAccountBase::jobData jd;
    jd.parent = 0; jd.offset = 0;
    jd.total = 1; jd.done = 0;
    jd.msgList.append(msg);
    QCString cstr( msg->asString() );
    int a = cstr.find( "\nX-UID: " );
    int b = cstr.find( "\n", a );
    if ( a != -1 && b != -1 && cstr.find( "\n\n" ) > a ) cstr.remove( a, b-a );
    mData.resize( cstr.length() + cstr.contains( "\n" ) - cstr.contains( "\r\n" ) );
    unsigned int i = 0;
    char prevChar = '\0';
    // according to RFC 2060 we need CRLF
    for ( char *ch = cstr.data(); *ch; ch++ )
    {
      if ( *ch == '\n' && (prevChar != '\r') ) {
        mData.at( i ) = '\r';
        i++;
      }
      mData.at( i ) = *ch;
      prevChar = *ch;
      i++;
    }
    jd.data = mData;
    KIO::SimpleJob *simpleJob = KIO::put( url, 0, FALSE, FALSE, FALSE );
    KIO::Scheduler::assignJobToSlave( account->slave(), simpleJob );
    mJob = simpleJob;
    account->insertJob( mJob, jd );
    connect( mJob, SIGNAL(result(KIO::Job *)),
             SLOT(slotPutMessageResult(KIO::Job *)) );
    connect( mJob, SIGNAL(dataReq(KIO::Job *, QByteArray &)),
             SLOT(slotPutMessageDataReq(KIO::Job *, QByteArray &)) );
    connect( mJob, SIGNAL(infoMessage(KIO::Job *, const QString &)),
             SLOT(slotPutMessageInfoData(KIO::Job *, const QString &)) );
  }
  else if ( jt == tCopyMessage || jt == tMoveMessage )
  {
    KURL url = account->getUrl();
    KURL destUrl = account->getUrl();
    destUrl.setPath(folder->imapPath());
    KMFolderImap *imapDestFolder = static_cast<KMFolderImap*>(msg_parent);
    url.setPath( imapDestFolder->imapPath() + ";UID=" + sets );
    ImapAccountBase::jobData jd;
    jd.parent = 0; mOffset = 0;
    jd.total = 1; jd.done = 0;
    jd.msgList = msgList;

    QByteArray packedArgs;
    QDataStream stream( packedArgs, IO_WriteOnly );

    stream << (int) 'C' << url << destUrl;

    KIO::SimpleJob *simpleJob = KIO::special( url, packedArgs, FALSE );
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
  } else {
    slotGetNextMessage();
  }
}


//-----------------------------------------------------------------------------
ImapJob::~ImapJob()
{

  if ( mDestFolder )
  {
    KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder)->account();
    if ( account ) // just to be sure this job is removed from the list
      account->mJobList.remove(this);
    if ( account && mJob )
    {
      ImapAccountBase::JobIterator it = account->findJob( mJob );
      if ( it != account->jobsEnd() && !(*it).msgList.isEmpty() )
      {
        for ( QPtrListIterator<KMMessage> mit( (*it).msgList ); mit.current(); ++mit )
          mit.current()->setTransferInProgress(false);
      }
    }
      
    mDestFolder->close();
  }

  if (mSrcFolder) {
    if (!mDestFolder || mDestFolder != mSrcFolder) {
      if (! (mSrcFolder->folderType() == KMFolderTypeImap) ) return;
      KMAcctImap *account = static_cast<KMFolderImap*>(mSrcFolder)->account();
      if ( account ) // just to be sure this job is removed from the list
        account->mJobList.remove(this);
      if ( account && mJob )
      {
        ImapAccountBase::JobIterator it = account->findJob( mJob );
        if ( it != account->jobsEnd() && !(*it).msgList.isEmpty() )
        {
          for ( QPtrListIterator<KMMessage> mit( (*it).msgList ); mit.current(); ++mit )
            mit.current()->setTransferInProgress(false);
        }
      }
    }
      
    mSrcFolder->close();
  }
}


//-----------------------------------------------------------------------------
void ImapJob::slotGetNextMessage()
{
  KMMessage *msg = mMsgList.first();
  KMFolderImap *msgParent = static_cast<KMFolderImap*>(msg->parent());
  KMAcctImap *account = msgParent->account();
  if ( msg->headerField("X-UID").isEmpty() )
  {
    emit messageRetrieved( msg );
    account->mJobList.remove( this );
    deleteLater();
    return;
  }
  KURL url = account->getUrl();
  QString path = msgParent->imapPath() + ";UID=" + msg->headerField("X-UID");
  if ( !mPartSpecifier.isEmpty() )
  {
    if ( mPartSpecifier.find ("STRUCTURE", 0, false) != -1 ) {
      path += ";SECTION=STRUCTURE";
    } else if ( mPartSpecifier == "HEADER" ) {
      path += ";SECTION=HEADER";
    } else {
      path += ";SECTION=" + mPartSpecifier;
    }
  }
  url.setPath( path );
//  kdDebug(5006) << "ImapJob::slotGetNextMessage - retrieve " << url.path() << endl;
  ImapAccountBase::jobData jd;
  jd.parent = 0;
  jd.total = 1; jd.done = 0;
  // protect the message, otherwise we'll get crashes afterwards
  msg->setTransferInProgress( true );
  KIO::SimpleJob *simpleJob = KIO::get( url, FALSE, FALSE );
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
}


//-----------------------------------------------------------------------------
void ImapJob::slotGetMessageResult( KIO::Job * job )
{
  KMMessage *msg = mMsgList.first();
  if (!msg || !msg->parent() || !job) {
    deleteLater();
    return;
  }
  KMFolderImap* parent = static_cast<KMFolderImap*>(msg->parent());
  if (msg->transferInProgress())
    msg->setTransferInProgress( false );
  KMAcctImap *account = parent->account();
  if ( !account ) {
    deleteLater();
    return;
  }
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  bool gotData = true;
  if (job->error())
  {
    account->slotSlaveError( account->slave(), job->error(), job->errorText() );
    return;
  } else {
    if ((*it).data.size() > 0)
    {
      kdDebug(5006) << "ImapJob::slotGetMessageResult - retrieved part " << mPartSpecifier << endl;
      if ( mPartSpecifier.isEmpty() ||
           mPartSpecifier == "HEADER" )
      {
        QString uid = msg->headerField("X-UID");
        msg->fromByteArray( (*it).data );
        msg->setHeaderField("X-UID",uid);
        if ( mPartSpecifier.isEmpty() ) 
          msg->setComplete( TRUE );
      } else {
        // Update the body of the retrieved part (the message notifies all observers)
        msg->updateBodyPart( mPartSpecifier, (*it).data );
        msg->setComplete( TRUE );
      }
    } else {
      kdWarning(5006) << "ImapJob::slotGetMessageResult - got no data for " << mPartSpecifier << endl;
      gotData = false;
      msg->setComplete( TRUE );
      // in case we got an empty message with attachment we have to update anyway
      if ( mPartSpecifier == "1" )
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
      parent->ignoreJobsForMessage( msg );
      int idx = parent->find( msg );
      if (idx != -1) parent->removeMsg( idx, true );
      emit messageRetrieved( 0 );
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
  KMFolderImap* parent = static_cast<KMFolderImap*>(msg->parent());
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
    account->slotSlaveError( account->slave(), job->error(),
        job->errorText() );
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
  KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder)->account();
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
  KMMessage *msg = mMsgList.first();
  KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder)->account();
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if (job->error())
  {
    account->slotSlaveError( account->slave(), job->error(),
        job->errorText() );
    return;
  } else {
    if ( !(*it).msgList.isEmpty() )
    {
      emit messageStored((*it).msgList.last());
      (*it).msgList.removeLast();
    } else if (msg)
    {
      emit messageStored(msg);
    }
    msg = 0;
  }
  if (account->slave()) {
    account->removeJob(it);
    account->mJobList.remove(this);
  }
  deleteLater();
}

//-----------------------------------------------------------------------------
void ImapJob::slotCopyMessageInfoData(KIO::Job * job, const QString & data)
{
  KMFolderImap * imapFolder = static_cast<KMFolderImap*>(mDestFolder);
  KMAcctImap *account = imapFolder->account();
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if (data.find("UID") != -1)
  {
    // split
    QString oldUid = data.section(' ', 1, 1);
    QString newUid = data.section(' ', 2, 2);

    // get lists of uids
    QValueList<int> olduids = KMFolderImap::splitSets(oldUid);
    QValueList<int> newuids = KMFolderImap::splitSets(newUid);

    int index = -1;
    if ( !(*it).msgList.isEmpty() )
    {
      KMMessage * msg;
      for ( msg = (*it).msgList.first(); msg; msg = (*it).msgList.next() )
      {
        uint uid = msg->headerField("X-UID").toInt();
        index = olduids.findIndex(uid);
        if (index > -1)
        {
          // found, get the new uid
          const ulong * sernum = (ulong *)msg->getMsgSerNum();
          imapFolder->insertUidSerNumEntry(newuids[index], sernum);
        }
      }
    } else if (mMsgList.first()) {
      uint uid = mMsgList.first()->headerField("X-UID").toInt();
      index = olduids.findIndex(uid);
      if (index > -1)
      {
        // found, get the new uid
        const ulong * sernum = (ulong *)mMsgList.first()->getMsgSerNum();
        imapFolder->insertUidSerNumEntry(newuids[index], sernum);
      }
    }
  }
}

//----------------------------------------------------------------------------
void ImapJob::slotPutMessageInfoData(KIO::Job *job, const QString &data)
{
  KMFolderImap * imapFolder = static_cast<KMFolderImap*>(mDestFolder);
  KMAcctImap *account = imapFolder->account();
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if (data.find("UID") != -1)
  {
    int uid = (data.right(data.length()-4)).toInt();

    if ( !(*it).msgList.isEmpty() )
    {
      const ulong * sernum = (ulong *)(*it).msgList.last()->getMsgSerNum();
//      kdDebug(5006) << "insert sernum " << (*it).msgList.last()->getMsgSerNum() << " for " << uid << endl;
      imapFolder->insertUidSerNumEntry(uid, sernum);
    } else if (mMsgList.first())
    {
      const ulong * sernum = (ulong *)mMsgList.first()->getMsgSerNum();
//      kdDebug(5006) << "insert sernum " << mMsgList.first()->getMsgSerNum() << " for " << uid << endl;
      imapFolder->insertUidSerNumEntry(uid, sernum);
    }
  }
}


//-----------------------------------------------------------------------------
void ImapJob::slotCopyMessageResult( KIO::Job *job )
{
  KMAcctImap *account = static_cast<KMFolderImap*>(mDestFolder)->account();
  ImapAccountBase::JobIterator it = account->findJob( job );
  if ( it == account->jobsEnd() ) return;

  if (job->error())
  {
    account->slotSlaveError( account->slave(), job->error(),
        job->errorText() );
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
  init( mType, mSets, static_cast<KMFolderImap*>( mDestFolder ), mMsgList );
}

//-----------------------------------------------------------------------------
void ImapJob::expireMessages()
{
    return;
}

}//namespace KMail

#include "imapjob.moc"
