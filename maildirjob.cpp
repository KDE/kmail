/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2003 Zack Rusin <zack@kde.org>
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
#include "maildirjob.h"

#include "kmfoldermaildir.h"

#include <kapplication.h>
#include <kdebug.h>

#include <qtimer.h>
#include <qdatetime.h>

namespace KMail {


//-----------------------------------------------------------------------------
MaildirJob::MaildirJob( KMMessage *msg, JobType jt , KMFolder *folder )
  : FolderJob( msg, jt, folder ), mParentFolder( 0 )
{
}

//-----------------------------------------------------------------------------
MaildirJob::MaildirJob( QPtrList<KMMessage>& msgList, const QString& sets,
                        JobType jt , KMFolder *folder )
  : FolderJob( msgList, sets, jt, folder ), mParentFolder( 0 )
{
}

//-----------------------------------------------------------------------------
MaildirJob::~MaildirJob()
{
}

//-----------------------------------------------------------------------------
void MaildirJob::setParentFolder( const KMFolderMaildir* parent )
{
  mParentFolder = const_cast<KMFolderMaildir*>( parent );
}



//-----------------------------------------------------------------------------
void MaildirJob::execute()
{
  QTimer::singleShot( 0, this, SLOT(startJob()) );
}

//-----------------------------------------------------------------------------
void MaildirJob::startJob()
{
  switch( mType ) {
  case tGetMessage:
    {
      KMMessage* msg = mMsgList.first();
      if ( msg ) {
        msg->setComplete( true );
        emit messageRetrieved( msg );
      }
    }
    break;
  case tDeleteMessage:
    {
      static_cast<KMFolder*>(mParentFolder)->removeMsg( mMsgList );
    }
    break;
  case tPutMessage:
    {
      mParentFolder->addMsg(  mMsgList.first() );
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

void
MaildirJob::expireMessages()
{
  int              days             = 0;
  int              maxUnreadTime    = 0;
  int              maxReadTime      = 0;
  const KMMsgBase *mb               = 0;
  QValueList<int>  rmvMsgList;
  int              i                = 0;
  time_t           msgTime, maxTime = 0;
  QTime            t;

  days = mParentFolder->daysToExpire( mParentFolder->getUnreadExpireAge(),
                                      mParentFolder->getUnreadExpireUnits() );
  if (days > 0) {
    kdDebug(5006) << "deleting unread older than "<< days << " days" << endl;
    maxUnreadTime = time(0) - days * 3600 * 24;
  }

  days = mParentFolder->daysToExpire( mParentFolder->getReadExpireAge(),
                                      mParentFolder->getReadExpireUnits() );
  if (days > 0) {
    kdDebug(5006) << "deleting read older than "<< days << " days" << endl;
    maxReadTime = time(0) - days * 3600 * 24;
  }

  if ((maxUnreadTime == 0) && (maxReadTime == 0)) {
    return;
  }

  t.start();
  mParentFolder->open();
  for( i=mParentFolder->count()-1; i>=0; i-- ) {
    mb = mParentFolder->getMsgBase(i);
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
      mParentFolder->removeMsg( i );
    }
    if ( t.elapsed() >= 150 ) {
      kapp->processEvents();
      t.restart();
    }
  }
  mParentFolder->close();

  return;
}

}

#include "maildirjob.moc"
