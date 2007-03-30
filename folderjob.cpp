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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "folderjob.h"

#include "kmfolder.h"
#include "globalsettings.h"
#include "folderstorage.h"

#include <kdebug.h>
#include <kio/global.h>

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
void FolderJob::kill()
{
  mErrorCode = KIO::ERR_USER_CANCELED;
  delete this;
}

//----------------------------------------------------------------------------
QPtrList<KMMessage>
FolderJob::msgList() const
{
  return mMsgList;
}

}

#include "folderjob.moc"
