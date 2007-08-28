/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2002-2003  Bo Thorsen <bo@sonofthor.dk>
 *                2002-2003  Steffen Hansen <hansen@kde.org>
 *                2002-2003  Zack Rusin <zack@kde.org>
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

#ifndef CACHEDIMAPJOB_H
#define CACHEDIMAPJOB_H

#include "folderjob.h"
#include <kio/job.h>
#include <kio/global.h>

#include <QList>


class KMFolderCachedImap;
class KMAcctCachedImap;
class KMMessage;

namespace KMail {

class CachedImapJob : public FolderJob {
  Q_OBJECT
public:
  /** Information about a message to be downloaded (from the 'IMAP envelope') */
  struct MsgForDownload {
    MsgForDownload() : uid(0),flags(0),size(0) {} // for QValueList only
    MsgForDownload( ulong _uid, int _flags, ulong _size )
      : uid(_uid), flags(_flags), size(_size) {}
    ulong uid;
    int flags;
    ulong size;
  };

  // Get messages
  explicit CachedImapJob( const QList<MsgForDownload>& msgs,
                 JobType type = tGetMessage, KMFolderCachedImap* folder = 0 );
  // Put messages
  CachedImapJob( const QList<KMMessage*>& msgs,
                 JobType type, KMFolderCachedImap* folder=0 );
  CachedImapJob( const QList<unsigned long>& msgs,
                 JobType type, KMFolderCachedImap* folder=0 );
  // Add sub folders
  explicit CachedImapJob( const QList<KMFolderCachedImap*>& folders,
                 JobType type = tAddSubfolders,
                 KMFolderCachedImap* folder = 0 );
  // Rename folder
  CachedImapJob( const QString& string1, JobType type,
                 KMFolderCachedImap* folder );
  // Delete folders or messages
  CachedImapJob( const QStringList& foldersOrMsgs, JobType type,
                 KMFolderCachedImap* folder );
  // Other jobs (list messages,expunge folder, check uid validity)
  CachedImapJob( JobType type, KMFolderCachedImap* folder );

  virtual ~CachedImapJob();

  void setParentFolder( const KMFolderCachedImap* parent );

signals:
  // only emitted for uid validity checking jobs with PERMANENTFLAGS responses
  void permanentFlags( int flags );

protected:
  virtual void execute();
  void expungeFolder();
  void checkUidValidity();
  void renameFolder( const QString &newName );
  void listMessages();

protected slots:
  virtual void slotGetNextMessage( KJob *job = 0 );
  virtual void slotAddNextSubfolder( KJob *job = 0 );
  virtual void slotPutNextMessage();
  virtual void slotPutMessageDataReq( KIO::Job *job, QByteArray &data );
  virtual void slotPutMessageResult( KJob *job );
  virtual void slotPutMessageInfoData(KJob *, const QString &data,const QString&);
  virtual void slotExpungeResult( KJob *job );
  virtual void slotDeleteNextFolder( KJob *job = 0 );
  virtual void slotCheckUidValidityResult( KJob *job );
  virtual void slotRenameFolderResult( KJob *job );
  virtual void slotListMessagesResult( KJob * job );
  void slotDeleteNextMessages( KJob* job = 0 );
  void slotProcessedSize( KJob *, qulonglong );

private:
  KMFolderCachedImap *mFolder;
  KMAcctCachedImap   *mAccount;
  QList<KMFolderCachedImap*> mFolderList;
  QList<MsgForDownload> mMsgsForDownload;
  QList<unsigned long> mSerNumMsgList;
  ulong mSentBytes; // previous messages
  ulong mTotalBytes;
  QStringList mFoldersOrMessages; // Folder deletion: path list. Message deletion: sets of uids
  KMMessage* mMsg;
  QString mString; // Used as uids and as rename target
  KMFolderCachedImap *mParentFolder;
};

}

#endif
