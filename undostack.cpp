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
#include "kmkernel.h"
#include <KJob>
#include <akonadi/itemmovejob.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

#include <QList>

namespace KMail {

UndoStack::UndoStack(int size)
    : QObject(0),
      mSize(size),
      mLastId(0),
      mCachedInfo(0)
{
}

UndoStack::~UndoStack()
{
    clear();
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

void UndoStack::addMsgToAction( int undoId, const Akonadi::Item &item )
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
    mCachedInfo->items.append( item );
}

void UndoStack::undo()
{
    if ( !mStack.isEmpty() )
    {
        UndoInfo *info = mStack.takeFirst();
        emit undoStackChanged();
        Akonadi::ItemMoveJob * job = new Akonadi::ItemMoveJob( info->items, info->srcFolder, this );
        connect( job, SIGNAL(result(KJob*)), this, SLOT(slotMoveResult(KJob*)) );
        delete info;
    }
    else
    {
        // Sorry.. stack is empty..
        KMessageBox::sorry( kmkernel->mainWin(), i18n("There is nothing to undo."));
    }
}

void UndoStack::slotMoveResult( KJob *job )
{
    if ( job->error() )
        KMessageBox::sorry( kmkernel->mainWin(), i18n("Cannot move message. %1", job->errorString() ) );
}

void UndoStack::pushSingleAction(const Akonadi::Item &item, const Akonadi::Collection &folder, const Akonadi::Collection &destFolder)
{
    int id = newUndoAction( folder, destFolder );
    addMsgToAction( id, item );
}

void UndoStack::msgDestroyed( const Akonadi::Item & /*msg*/)
{
    /*
   for (UndoInfo *info = mStack.first(); info; )
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
            ++it;
    }
    emit undoStackChanged();
}

}

#include "undostack.moc"
