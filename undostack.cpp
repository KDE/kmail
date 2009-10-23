/*
    This file is part of KMail

    Copyright (C) 1999 Waldo Bastian (bastian@kde.org)
    Copyright (c) 2003 Zack Rusin <zack@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library; see the file COPYING. If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "undostack.h"

#include "kmmainwin.h"
#include "kmmsgdict.h"

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

#include <QList>

namespace KMail {

UndoStack::UndoStack(int size)
  : QObject(0), mSize(size), mLastId(0),
    mCachedInfo(0)
{
  setObjectName( "undostack" );
}

UndoStack::~UndoStack()
{
  qDeleteAll( mStack );
}

void UndoStack::clear()
{
  qDeleteAll( mStack );
  mStack.clear();
}

int UndoStack::newUndoAction( const Akonadi::Collection &srcFolder, const Akonadi::Collection &destFolder )
{
  UndoInfo *info = new UndoInfo;
  info->id         = ++mLastId;
  info->srcFolder  = srcFolder;
  info->destFolder = destFolder;
  if ((int) mStack.count() == mSize) {
    delete mStack.last();
    mStack.removeLast();
  }
  mStack.prepend( info );
  emit undoStackChanged();
  return info->id;
}

void UndoStack::addMsgToAction( int undoId, ulong serNum )
{
  if ( !mCachedInfo || mCachedInfo->id != undoId ) {
    QList<UndoInfo*>::const_iterator itr = mStack.constBegin();
    while ( itr != mStack.constEnd() ) {
      if ( (*itr)->id == undoId ) {
        mCachedInfo = (*itr);
        break;
      }
      ++itr;
    }
  }

  Q_ASSERT( mCachedInfo );
  mCachedInfo->serNums.append( serNum );
}

void UndoStack::undo()
{
#if 0 //TODO port to akonadi
  KMMessage *msg;
  ulong serNum;
  int idx = -1;
  KMFolder *curFolder;
  if ( mStack.count() > 0 )
  {
    UndoInfo *info = mStack.takeFirst();
    emit undoStackChanged();
    QList<ulong>::iterator itr;
    KMFolderOpener openDestFolder( info->destFolder, "undodest" );
    for( itr = info->serNums.begin(); itr != info->serNums.end(); ++itr ) {
      serNum = *itr;
      KMMsgDict::instance()->getLocation(serNum, &curFolder, &idx);
      if ( idx == -1 || curFolder != info->destFolder ) {
        kDebug()<<"Serious undo error!";
        delete info;
        return;
      }
      msg = curFolder->getMsg( idx );
      info->srcFolder->moveMsg( msg );
      if ( info->srcFolder->count() > 1 )
        info->srcFolder->unGetMsg( info->srcFolder->count() - 1 );
    }
    delete info;
  }
  else
  {
    // Sorry.. stack is empty..
    KMessageBox::sorry( kmkernel->mainWin(), i18n("There is nothing to undo."));
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void
UndoStack::pushSingleAction(ulong serNum, const Akonadi::Collection &folder, const Akonadi::Collection &destFolder)
{
  int id = newUndoAction( folder, destFolder );
  addMsgToAction( id, serNum );
}

void
UndoStack::msgDestroyed( const Akonadi::Item & /*msg*/)
{
  /*
   for(UndoInfo *info = mStack.first(); info; )
   {
      if (info->msgIdMD5 == msg->msgIdMD5())
      {
         mStack.removeRef( info );
         info = mStack.current();
      }
      else
         info = mStack.next();
   }
  */
}

void
UndoStack::folderDestroyed( const Akonadi::Collection &folder)
{
  QList<UndoInfo*>::iterator it = mStack.begin();
  while ( it != mStack.end() ) {
    UndoInfo *info = *it;
    if ( info &&
          ( (info->srcFolder == folder) ||
            (info->destFolder == folder) ) ) {
      delete info;
      it = mStack.erase( it );
    }
    else
      it++;
  }
  emit undoStackChanged();
}

}

#include "undostack.moc"
