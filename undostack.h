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

#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <QList>
#include <QObject>
#include <akonadi/collection.h>
#include <akonadi/item.h>
namespace KMail {

/** A class for storing Undo information. */
class UndoInfo
{
public:
  int          id;
  Akonadi::Item::List items;
  Akonadi::Collection srcFolder;
  Akonadi::Collection destFolder;
};

class UndoStack : public QObject
{
  Q_OBJECT

public:
  UndoStack(int size);
  ~UndoStack();

  void clear();
  int  size() const { return mStack.count(); }
  int  newUndoAction( const Akonadi::Collection& srcFolder, const Akonadi::Collection & destFolder );
  void addMsgToAction( int undoId, const Akonadi::Item &item );
  void undo();

  void pushSingleAction(const Akonadi::Item &item, const Akonadi::Collection&, const Akonadi::Collection& destFolder);
  void msgDestroyed( const Akonadi::Item &msg);
  void folderDestroyed( const Akonadi::Collection &folder);
protected:
  QList<UndoInfo*> mStack;
  int mSize;
  int mLastId;
  UndoInfo *mCachedInfo;

signals:
   void undoStackChanged();
};

}

#endif
