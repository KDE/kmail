/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef kmfoldermbox_h
#define kmfoldermbox_h

#include "kmfolderindex.h"
#include "mboxjob.h"

#include <sys/types.h> // for size_t

namespace KMail {
  class FolderJob;
  class MboxJob;
  class AttachmentStrategy;
}
using KMail::FolderJob;
using KMail::MboxJob;
using KMail::AttachmentStrategy;

/* Mail folder.
 * (description will be here).
 *
 * Accounts:
 *   The accounts (of KMail) that are fed into the folder are
 *   represented as the children of the folder. They are only stored here
 *   during runtime to have a reference for which accounts point to a
 *   specific folder.
 */

class KMFolderMbox : public KMFolderIndex
{
  Q_OBJECT
  friend class ::KMail::MboxJob;
public:


  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderMbox(KMFolder* folder, const char* name=0);
  virtual ~KMFolderMbox();

  /** Returns the type of this folder */
  virtual KMFolderType folderType() const { return KMFolderTypeMbox; }

  /** Read a message and return it as a string */
  DwString getDwString(int idx);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg( KMMessage* msg, int* index_return = 0 );

  /** Open folder for access.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise (errno). */
  virtual int open(const char *owner);

  /** @reimpl */
  virtual void reallyDoClose(const char *owner);

  virtual int canAccess();

  /** fsync buffers to disk */
  virtual void sync();

  /** @reimpl */
  virtual int create();

  /** Remove deleted messages from the folder. Returns zero on success
    and an errno on failure. */
  virtual int compact( bool silent );

  /** Remove some deleted messages from the folder. Returns zero on success
    and an errno on failure. This is only for use from MboxCompactionJob. */
  int compact( unsigned int startIndex, int nbMessages, FILE* tmpFile, off_t& offs, bool& done );

  /** Is the folder read-only? */
  virtual bool isReadOnly() const { return mReadOnly; }

  /** Is the folder locked? */
  bool isLocked() const { return mFilesLocked; }

  void setLockType( LockType ltype=FCNTL );

  void setProcmailLockFileName( const QString& );

  static QByteArray escapeFrom( const DwString & str );

  virtual IndexStatus indexStatus();

  /**  reimp */
  virtual Q_INT64 doFolderSize() const;

protected:
  virtual FolderJob* doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder,
                                  QString partSpecifier, const AttachmentStrategy *as ) const;
  virtual FolderJob* doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                  FolderJob::JobType jt, KMFolder *folder ) const;
  /** Load message from file and store it at given index. Returns 0
    on failure. */
  virtual KMMessage* readMsg(int idx);

  /** Create index file from messages file and fill the message-info list
      mMsgList. Returns 0 on success and an errno value (see fopen) on
      failure. */
  virtual int createIndexFromContents();

  /** Lock mail folder files. Called by ::open(). Returns 0 on success and
    an errno error code on failure. */
  virtual int lock();

  /** Unlock mail folder files. Called by ::close().  Returns 0 on success
    and an errno error code on failure. */
  virtual int unlock();

  /** Called by KMFolder::remove() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int removeContents();

  /** Called by KMFolder::expunge() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int expungeContents();

private:
  FILE *mStream;
  bool mFilesLocked; // TRUE if the files of the folder are locked (writable)
  bool mReadOnly; // true if locking failed
  LockType mLockType;
  QString mProcmailLockFileName;
};

#endif // kmfoldermbox_h
