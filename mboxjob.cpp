// mboxjob.cpp
// Zack Rusin <zack@kde.org>
// This file is part of KMail. Licensed under GNU GPL.
#include "mboxjob.h"

#include "kmmessage.h"
#include "kmfoldermbox.h"

#include <kapplication.h>
#include <kdebug.h>
#include <qobject.h>
#include <qtimer.h>
#include <qdatetime.h>

namespace KMail {


//-----------------------------------------------------------------------------
MboxJob::MboxJob( KMMessage *msg, JobType jt , KMFolder *folder  )
  : FolderJob( msg, jt, folder )
{
}

//-----------------------------------------------------------------------------
MboxJob::MboxJob( QPtrList<KMMessage>& msgList, const QString& sets,
                  JobType jt, KMFolder *folder  )
  : FolderJob( msgList, sets, jt, folder )
{
}

//-----------------------------------------------------------------------------
MboxJob::~MboxJob()
{
}

//-----------------------------------------------------------------------------
void
MboxJob::execute()
{
  QTimer::singleShot( 0, this, SLOT(startJob()) );
}


//-----------------------------------------------------------------------------
void
MboxJob::expireMessages()
{
  int              days             = 0;
  int              maxUnreadTime    = 0;
  int              maxReadTime      = 0;
  const KMMsgBase *mb               = 0;
  QValueList<int>  rmvMsgList;
  int              i                = 0;
  time_t           msgTime, maxTime = 0;
  QTime            t;

  days = mParent->daysToExpire( mParent->getUnreadExpireAge(),
                                mParent->getUnreadExpireUnits() );
  if (days > 0) {
    kdDebug(5006) << "deleting unread older than "<< days << " days" << endl;
    maxUnreadTime = time(0) - days * 3600 * 24;
  }

  days = mParent->daysToExpire( mParent->getReadExpireAge(),
                                mParent->getReadExpireUnits() );
  if (days > 0) {
    kdDebug(5006) << "deleting read older than "<< days << " days" << endl;
    maxReadTime = time(0) - days * 3600 * 24;
  }

  if ((maxUnreadTime == 0) && (maxReadTime == 0)) {
    return;
  }

  t.start();
  mParent->open();
  for( i=mParent->count()-1; i>=0; i-- ) {
    mb = mParent->getMsgBase(i);
    if (mb == 0) {
      continue;
    }
    msgTime = mb->date();

    if (mb->isUnread()) {
      maxTime = maxUnreadTime;
    } else {
      maxTime = maxReadTime;
    }

    if (msgTime < maxTime) {
      mParent->removeMsg( i );
    }
    if ( t.elapsed() >= 150 ) {
      kapp->processEvents();
      t.restart();
    }
  }
  mParent->close();

  return;
}

//-----------------------------------------------------------------------------
void
MboxJob::setParent( const KMFolderMbox *parent )
{
  mParent = const_cast<KMFolderMbox*>( parent );
}

//-----------------------------------------------------------------------------
void
MboxJob::startJob()
{
  KMMessage *msg = mMsgList.first();
  assert( (msg && ( mParent || msg->parent() )) || ( mParent && mType == tExpireMessages) );
  switch( mType ) {
  case tGetMessage:
    {
      kdDebug(5006)<<msg<<endl;
      kdDebug(5006)<<this<<endl;
      kdDebug(5006)<<"Done"<<endl;
      //KMMessage* msg = mParent->getMsg( mParent->find( mMsgList.first() ) );
      msg->setComplete( true );
      emit messageRetrieved( msg );
    }
    break;
  case tDeleteMessage:
    {
      mParent->removeMsg( mMsgList );
    }
    break;
  case tPutMessage:
    {
      mParent->addMsg(  mMsgList.first() );
      emit messageStored( mMsgList.first() );
    }
    break;
  case tExpireMessages:
    {
      expireMessages();
    }
    break;
  case tCopyMessage:
  case tCreateFolder:
  case tGetFolder:
  case tListDirectory:
    kdDebug(5006)<<k_funcinfo<<"### Serious problem! "<<endl;
    break;
  default:
    break;
  }
  //OK, we're done
  //delete this;
  deleteLater();
}

}

#include "mboxjob.moc"
