/*
    This file is part of KMail

    Copyright (C) 1999 Waldo Bastian (bastian@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License 
    version 2 as published by the Free Software Foundation. 

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library; see the file COPYING. If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "kmundostack.h"

KMUndoStack::KMUndoStack(int size)
 : mSize(size)
{
   mStack.setAutoDelete(true);
}

void KMUndoStack::clear()
{
   mStack.clear();
}

void 
KMUndoStack::pushAction(QString msgIdMD5, KMFolder *folder, KMFolder *destFolder)
{
   KMUndoInfo *info = new KMUndoInfo;
   info->msgIdMD5 = msgIdMD5;
   info->folder = folder;
   info->destFolder = destFolder;
   if ((int) mStack.count() == mSize)
      mStack.removeLast();
   mStack.prepend(info);
}

void 
KMUndoStack::msgDestroyed( KMMsgBase* /*msg*/)
{
  /*
   for(KMUndoInfo *info = mStack.first(); info; )
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
KMUndoStack::folderDestroyed( KMFolder *folder)
{
   for(KMUndoInfo *info = mStack.first(); info; )
   {
      if ((info->folder == folder) ||
	  (info->destFolder == folder))
      {
         mStack.removeRef( info );
         info = mStack.current();
      }
      else
         info = mStack.next();     
   }
}

bool 
KMUndoStack::popAction(QString &msgIdMD5, KMFolder *&folder, KMFolder *&destFolder)
{
   if (mStack.count() == 0) return false;
   KMUndoInfo *info = mStack.take(0);
   msgIdMD5 = info->msgIdMD5;
   folder = info->folder;
   destFolder = info->destFolder;
   delete info;
   return true;
}
