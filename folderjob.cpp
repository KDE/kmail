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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "folderjob.h"

#include "kmfolder.h"
#include "globalsettings.h"
#include "folderstorage.h"

#include <kdebug.h>

namespace KMail {

//----------------------------------------------------------------------------
FolderJob::FolderJob( KMMessage *msg, JobType jt, KMFolder* folder,
                          QString partSpecifier )
  : mType( jt ), mSrcFolder( 0 ), mDestFolder( folder ), mPartSpecifier( partSpecifier ),
    mErrorCode( 0 ),
    mPassiveDestructor( false ), mStarted( false )
{
  if ( msg ) {
    mMsgList.append(msg);
    mSets = msg->headerField("X-UID");
  }
  init();
}

//----------------------------------------------------------------------------
FolderJob::FolderJob( const QPtrList<KMMessage>& msgList, const QString& sets,
                          JobType jt, KMFolder *folder )
  : mMsgList( msgList ),mType( jt ),
    mSets( sets ), mSrcFolder( 0 ), mDestFolder( folder ),
    mErrorCode( 0 ),
    mPassiveDestructor( false ), mStarted( false )
{
  init();
}

//----------------------------------------------------------------------------
FolderJob::FolderJob( JobType jt )
  : mType( jt ),
    mErrorCode( 0 ),
    mPassiveDestructor( false ), mStarted( false )
{
  init();
}

//----------------------------------------------------------------------------
void FolderJob::init()
{
  switch ( mType ) {
  case tListMessages:
  case tGetFolder:
  case tGetMessage:
  case tCheckUidValidity:
  case tExpireMessages:
    mCancellable = true;
    break;
  default:
    mCancellable = false;
  }
}

//----------------------------------------------------------------------------
FolderJob::~FolderJob()
{
  if( !mPassiveDestructor ) {
    emit result( this );
    emit finished();
  }
}

//----------------------------------------------------------------------------
void
FolderJob::start()
{
  if (!mStarted)
  {
    mStarted = true;
    execute();
  }
}

//----------------------------------------------------------------------------
QPtrList<KMMessage>
FolderJob::msgList() const
{
  return mMsgList;
}

//----------------------------------------------------------------------------
void
FolderJob::expireMessages()
{
  int              maxUnreadTime    = 0;
  int              maxReadTime      = 0;
  const KMMsgBase *mb               = 0;
  int              i                = 0;
  time_t           msgTime, maxTime = 0;
  FolderStorage*   storage          = mDestFolder->storage();

  int unreadDays, readDays;
  mDestFolder->daysToExpire( unreadDays, readDays );
  if (unreadDays > 0) {
    kdDebug(5006) << "deleting unread older than "<< unreadDays << " days" << endl;
    maxUnreadTime = time(0) - unreadDays * 3600 * 24;
  }
  if (readDays > 0) {
    kdDebug(5006) << "deleting read older than "<< readDays << " days" << endl;
    maxReadTime = time(0) - readDays * 3600 * 24;
  }

  if ((maxUnreadTime == 0) && (maxReadTime == 0)) {
    return;
  }

  storage->open();
  QPtrList<KMMessage> list;
  for( i=storage->count()-1; i>=0; i-- ) {
    mb = storage->getMsgBase(i);
    if (mb == 0) {
      continue;
    }
    if ( mb->isImportant()
      && GlobalSettings::excludeImportantMailFromExpiry() )
       continue;

    msgTime = mb->date();

    if (mb->isUnread()) {
      maxTime = maxUnreadTime;
    } else {
      maxTime = maxReadTime;
    }

    if (msgTime < maxTime) {
      list.append( storage->getMsg( i ) );
    }
  }
  storage->removeMsg( list );
  storage->close();

  deleteLater();
}

}

#include "folderjob.moc"
