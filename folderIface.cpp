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

#include "folderIface.h"

#include "kmmainwin.h"
#include "kmfoldertree.h"
#include "kmfoldermgr.h"
#include "kmfolder.h"

#include <kapplication.h>
#include <kdebug.h>

#include <stdlib.h>

namespace KMail {

FolderIface::FolderIface( const QString& vpath )
  : DCOPObject( "FolderIface" ), mPath( vpath )
{
  //kdDebug(5006)<<"FolderIface folder = "<< mPath <<endl;
  mFolder = kmkernel->folderMgr()->getFolderByURL( mPath );
  if ( !mFolder )
    mFolder = kmkernel->imapFolderMgr()->getFolderByURL( mPath );
  if ( !mFolder )
    mFolder = kmkernel->dimapFolderMgr()->getFolderByURL( mPath );
  Q_ASSERT( mFolder );
}

QString
FolderIface::path() const
{
  return mPath;
}

QString
FolderIface::displayName() const
{
  return mFolder->label();
}

QString
FolderIface::displayPath() const
{
  return mFolder->prettyURL();
}

bool
FolderIface::usesCustomIcons() const
{
  return mFolder->useCustomIcons();
}

QString
FolderIface::normalIconPath() const
{
  return mFolder->normalIconPath();
}

QString
FolderIface::unreadIconPath() const
{
  return mFolder->unreadIconPath();
}

int
FolderIface::messages()
{
  // if the folder isn't open then return the cached count
  return mFolder->count( !mFolder->isOpened() );
}

int
FolderIface::unreadMessages()
{
    return mFolder->countUnread();
}

int
FolderIface::unreadRecursiveMessages()
{
    return mFolder->countUnreadRecursive();
}

//The reason why this function is disabled is that we loose
//the message as soon as we get it (after we switch folders,
//it's being unget. We need a reference count on message to make it work
/*
QValueList<DCOPRef>
FolderIface::messageRefs()
{
    QValueList<DCOPRef> refList;
    KMMsgList messageCache;
    KMFolderIndex *indexFolder = static_cast<KMFolderIndex*>( mFolder);

    if ( indexFolder ) {
      mFolder->open();
      indexFolder->readIndex();
      messageCache = indexFolder->mMsgList;
      mFolder->close();
    } else
      return refList;

    kdDebug(5006)<<"refList == "<<messageCache.count()<<endl;

    for( int i = 0; i < messageCache.size(); ++i ) {
      KMMsgBase *msg = messageCache[i];
      if ( msg ) {
        KMMessage *fmsg = msg->parent()->getMsg( msg->parent()->find( msg ) );
        refList.append( DCOPRef( new MessageIface( fmsg ) ) );
      }
    }

    kdDebug(5006)<<"Reflist size = "<<refList.count()<<endl;
    return refList;
}*/

}

#include "folderIface.moc"
