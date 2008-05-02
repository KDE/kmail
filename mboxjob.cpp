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

#include "mboxjob.h"

#include "kmfoldermbox.h"
#include "kmfolder.h"

#include <kdebug.h>

#include <QDateTime>
#include <QTimer>

namespace KMail {


//-----------------------------------------------------------------------------
MboxJob::MboxJob( KMMessage *msg, JobType jt, KMFolder *folder  )
  : FolderJob( msg, jt, folder )
{
}

//-----------------------------------------------------------------------------
MboxJob::MboxJob( QList<KMMessage*>& msgList, const QString& sets,
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
MboxJob::setParent( const KMFolderMbox *parent )
{
  mParent = const_cast<KMFolderMbox*>( parent );
}

//-----------------------------------------------------------------------------
void
MboxJob::startJob()
{
  KMMessage *msg = mMsgList.first();
  assert( (msg && ( mParent || msg->parent() )) );
  switch( mType ) {
  case tGetMessage:
    {
      kDebug(5006)<<msg;
      kDebug(5006)<<this;
      kDebug(5006)<<"Done";
      //KMMessage* msg = mParent->getMsg( mParent->find( mMsgList.first() ) );
      msg->setComplete( true );
      emit messageRetrieved( msg );
    }
    break;
  case tDeleteMessage:
    {
      mParent->removeMessages( mMsgList );
    }
    break;
  case tPutMessage:
    {
      mParent->addMsg(  mMsgList.first() );
      emit messageStored( mMsgList.first() );
    }
    break;
  case tCopyMessage:
  case tCreateFolder:
  case tGetFolder:
  case tListMessages:
    kDebug(5006)<<"### Serious problem!";
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
