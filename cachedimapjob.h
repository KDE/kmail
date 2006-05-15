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
  CachedImapJob( const QValueList<MsgForDownload>& msgs,
                 JobType type = tGetMessage, KMFolderCachedImap* folder = 0 );
  // Put messages
  CachedImapJob( const QPtrList<KMMessage>& msgs,
                 JobType type, KMFolderCachedImap* folder=0 );
  CachedImapJob( const QValueList<unsigned long>& msgs,
                 JobType type, KMFolderCachedImap* folder=0 );
  // Add sub folders
  CachedImapJob( const QValueList<KMFolderCachedImap*>& folders,
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

protected:
  virtual void execute();
  void expungeFolder();
  void checkUidValidity();
  void renameFolder( const QString &newName );
  void listMessages();

protected slots:
  virtual void slotGetNextMessage( KIO::Job *job = 0 );
  virtual void slotAddNextSubfolder( KIO::Job *job = 0 );
  virtual void slotPutNextMessage();
  virtual void slotPutMessageDataReq( KIO::Job *job, QByteArray &data );
  virtual void slotPutMessageResult( KIO::Job *job );
  virtual void slotPutMessageInfoData(KIO::Job *, const QString &data);
  virtual void slotExpungeResult( KIO::Job *job );
  virtual void slotDeleteNextFolder( KIO::Job *job = 0 );
  virtual void slotCheckUidValidityResult( KIO::Job *job );
  virtual void slotRenameFolderResult( KIO::Job *job );
  virtual void slotListMessagesResult( KIO::Job * job );
  void slotDeleteNextMessages( KIO::Job* job = 0 );
  void slotProcessedSize( KIO::Job *, KIO::filesize_t processed );

private:
  KMFolderCachedImap *mFolder;
  KMAcctCachedImap   *mAccount;
  QValueList<KMFolderCachedImap*> mFolderList;
  QValueList<MsgForDownload> mMsgsForDownload;
  QValueList<unsigned long> mSerNumMsgList;
  ulong mSentBytes; // previous messages
  ulong mTotalBytes;
  QStringList mFoldersOrMessages; // Folder deletion: path list. Message deletion: sets of uids
  KMMessage* mMsg;
  QString mString; // Used as uids and as rename target
  KMFolderCachedImap *mParentFolder;
};

}

#endif
