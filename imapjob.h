/*
 * imapjob.h
 *
 * Copyright (C)  2002  Zack Rusin <zack@kde.org>
 *                2000-2002 Michael Haeckel <haeckel@kde.org>
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
#ifndef IMAPJOB_H
#define IMAPJOB_H

#include <kio/job.h>
#include <kio/global.h>

#include "folderjob.h"

#include <qcstring.h>

class KMAcctImap;
class KMMessage;
class KMFolderTreeItem;
class KMFolderImap;

namespace KMail {

class ImapJob : public FolderJob
{
  Q_OBJECT
  friend class KMAcctImap;

public:
  ImapJob( KMMessage *msg, JobType jt = tGetMessage, KMFolderImap *folder = 0 );
  ImapJob( QPtrList<KMMessage>& msgList, QString sets, JobType jt = tGetMessage, KMFolderImap *folder = 0 );
  virtual ~ImapJob();

  static void ignoreJobsForMessage( KMMessage *msg );

private slots:
  void slotGetMessageResult( KIO::Job * job );
  void slotGetNextMessage();
  /** Feeds the message in pieces to the server */
  void slotPutMessageDataReq( KIO::Job *job, QByteArray &data );
  void slotPutMessageResult( KIO::Job *job );
  void slotPutMessageInfoData(KIO::Job *, const QString &data);
  /** result of a copy-operation */
  void slotCopyMessageResult( KIO::Job *job );
  void slotCopyMessageInfoData(KIO::Job *, const QString &data);
private:
  void execute();
  void expireMessages();
  void init( JobType jt, QString sets, KMFolderImap *folder, QPtrList<KMMessage>& msgList );
  KIO::Job *mJob;
  QByteArray mData;
  QCString mStrData;
  KMFolderTreeItem *mFti;
  int mTotal, mDone, mOffset;
};

}


#endif
