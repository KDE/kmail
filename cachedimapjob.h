/*
 * cachedimapjob.h
 *
 * Copyright (C) 2002  Bo Thorsen <bo@sonofthor.dk>
 *               2002  Steffen Hansen <hansen@kde.org>
 *               2002  Zack Rusin <zack@kde.org>
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
#ifndef CACHEDIMAPJOB_H
#define CACHEDIMAPJOB_H

#include "folderjob.h"
#include <kio/job.h>
#include <kio/global.h>

#include <qptrlist.h>
#include <qvaluelist.h>
#include <qcstring.h>

class KMFolderCachedImap;
class KMAcctCachedImap;
class KMMessage;

namespace KMail {

class CachedImapJob : public FolderJob {
  Q_OBJECT
public:
  // Put messages
  CachedImapJob( QPtrList<KMMessage>& msgs,
                 JobType type, KMFolderCachedImap* folder=0 );
  // Add sub folders
  CachedImapJob( const QValueList<KMFolderCachedImap*>& folders,
                 JobType type = tAddSubfolders,
                 KMFolderCachedImap* folder = 0 );
  // Get messages
  CachedImapJob( const QValueList<ulong>& uids,
                 JobType type = tGetMessage, KMFolderCachedImap* folder = 0,
                 const QValueList<int>& flags = QValueList<int>() );
  // Delete message ; Rename folder
  CachedImapJob( const QString& string1, JobType type,
                 KMFolderCachedImap* folder );
  // Delete folders
  CachedImapJob( const QStringList& folders, JobType type, KMFolderCachedImap* folder = 0 );
  // Other jobs (expunge folder, check uid validity)
  CachedImapJob( JobType type, KMFolderCachedImap* folder );

  virtual ~CachedImapJob();

  void setPassiveDestructor( bool passive ) { mPassiveDestructor = passive; }
  bool passiveDestructor() { return mPassiveDestructor; }

protected:
  virtual void execute();
  virtual void expireMessages();
  virtual void deleteMessages( const QString& uids );
  virtual void expungeFolder();
  virtual void checkUidValidity();
  virtual void renameFolder( const QString &newName );

protected slots:
  virtual void slotGetNextMessage( KIO::Job *job = 0 );
  virtual void slotAddNextSubfolder( KIO::Job *job = 0 );
  virtual void slotPutNextMessage();
  virtual void slotPutMessageDataReq( KIO::Job *job, QByteArray &data );
  virtual void slotPutMessageResult( KIO::Job *job );
  virtual void slotDeleteResult( KIO::Job *job );
  virtual void slotDeleteNextFolder( KIO::Job *job = 0 );
  virtual void slotCheckUidValidityResult( KIO::Job *job );
  virtual void slotRenameFolderResult( KIO::Job *job );

private:
  void init();

  KMFolderCachedImap *mFolder;
  KMAcctCachedImap   *mAccount;
  QValueList<KMFolderCachedImap*> mFolderList;
  QValueList<ulong> mUidList;
  QValueList<int> mFlags;
  QStringList mFolderPathList; // Used only for folder deletion
  ulong mUid;
  int mFlag;
  KMMessage* mMsg;
  QString mString; // Used as uids and as rename target
  KIO::Job *mJob;
  QByteArray mData;
};

}

#endif
