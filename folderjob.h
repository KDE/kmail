/*
 * folderjob.h
 *
 * Copyright (C)  2002  Zack Rusin <zack@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef FOLDERJOB_H
#define FOLDERJOB_H

#include <qobject.h>
#include <qobjectlist.h>
#include <qstring.h>

#include "kmmessage.h"

class KMFolder;

namespace KMail {

class FolderJob : public QObject
{
  Q_OBJECT

public:
  enum JobType { tListDirectory, tGetFolder, tCreateFolder, tExpungeFolder,
		 tDeleteMessage, tGetMessage, tPutMessage, tAddSubfolders,
		 tDeleteFolders, tCheckUidValidity, tRenameFolder,
                 tCopyMessage, tExpireMessages, tMoveMessage };
  /**
   * Constructs a new job, operating on the message msg, of type
   * @p jt and with a parent folder @p folder.
   */
  FolderJob( KMMessage *msg, JobType jt = tGetMessage, KMFolder *folder = 0  );

  /**
   * Constructs a new job, operating on a message list @p msgList,
   * set @sets, JobType @p jt and with the parent folder @p folder.
   *
   */
  FolderJob( const QPtrList<KMMessage>& msgList, const QString& sets,
	     JobType jt = tGetMessage, KMFolder *folder = 0 );
  /**
   * This one should ONLY be used in derived folders, when a job
   * does something internal and needs to construct an empty parent
   * FolderJob
   */
  FolderJob( JobType jt );
  virtual ~FolderJob();

  QPtrList<KMMessage> msgList() const;
  void start();

signals:
  void messageRetrieved(KMMessage *);
  void messageStored(KMMessage *);
  void messageCopied(KMMessage *);
  void messageCopied(QPtrList<KMMessage>);
  void finished();
protected:
  virtual void execute()=0;
  virtual void expireMessages()=0;

  QPtrList<KMMessage> mMsgList;
  JobType             mType;
  QString             mSets;
  KMFolder*           mDestFolder;
  //finished() won't be emitted when this is set
  bool                mPassiveDestructor;
};

};

#endif
