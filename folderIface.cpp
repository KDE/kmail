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
#include "folderIface.h"
#include "kmkernel.h"
#include "kmfoldermgr.h"
#include "kmfolderindex.h"
#include "kmfolder.h"
#include "kmmsgdict.h"

#include <stdlib.h>

#include <kdebug.h>

namespace KMail {

FolderIface::FolderIface( const QString& vpath )
  : DCOPObject( "FolderIface" ), mPath( vpath )
{
    kdDebug(5006)<<"FolderIface folder = "<< mPath <<endl;
    mFolder = kernel->folderMgr()->getFolderByURL( mPath );
    if ( !mFolder )
      mFolder = kernel->imapFolderMgr()->getFolderByURL( mPath );
}

QString
FolderIface::path() const
{
  return mPath;
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
    return mFolder->countUnreadRecursive();
}

int
FolderIface::unreadMessages()
{
    return mFolder->countUnread();
}

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

    kdDebug()<<"refList == "<<messageCache.count()<<endl;

    for( int i = 0; i < messageCache.size(); ++i ) {
      KMMsgBase *msg = messageCache[i];
      if ( msg ) {
        KMMessage *fmsg = msg->parent()->getMsg( msg->parent()->find( msg ) );
        refList.append( DCOPRef( new MessageIface( fmsg ) ) );
      }
    }

    kdDebug()<<"Reflist size = "<<refList.count()<<endl;
    return refList;
}*/

}

#include "folderIface.moc"
