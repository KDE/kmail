/* Local Mail folder
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 *
*/
#ifndef kmfoldermbox_h
#define kmfoldermbox_h

#include "kmfolder.h"
#include "mboxjob.h"
using KMail::MboxJob;

#define KMFolderMboxInherited KMFolder

/* Mail folder.
 * (description will be here).
 *
 * Accounts:
 *   The accounts (of KMail) that are fed into the folder are
 *   represented as the children of the folder. They are only stored here
 *   during runtime to have a reference for which accounts point to a
 *   specific folder.
 */

class KMFolderMbox : public KMFolder
{
  Q_OBJECT
  friend class MboxJob;
public:


  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderMbox(KMFolderDir* parent=0, const QString& name=QString::null);
  virtual ~KMFolderMbox();

  /** Read a message and return a referece to a string */
  virtual QCString& getMsgString(int idx, QCString& mDest);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index_return = 0);

  /** Open folder for access. Does not work if the parent is not set.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise (errno). */
  virtual int open();

  /** Close folder. If force is TRUE the files are closed even if
    others still use it (e.g. other mail reader windows). */
  virtual void close(bool force=FALSE);

  virtual int canAccess();

  /** fsync buffers to disk */
  virtual void sync();

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the
      c-library fopen call otherwise. */
  virtual int create(bool imap = FALSE);

  /** Remove deleted messages from the folder. Returns zero on success
    and an errno on failure. */
  virtual int compact();

  /** Is the folder read-only? */
  virtual bool isReadOnly() const { return !mFilesLocked; }

  void setLockType( LockType ltype=FCNTL );

  void setProcmailLockFileName( const QString& );

  virtual QCString protocol() const { return "mbox"; }

protected:
  virtual FolderJob* doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder ) const;
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

  /** Tests whether the contents of this folder is newer than the index.
      Returns IndexTooOld if the index file is older than the mbox file.
      Returns IndexMissing if there is an mbox file but no index file.
      Returns IndexOk if there is no mbox file or if the index file
      is not older than the mbox file.
  */
  virtual IndexStatus indexStatus();

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
  LockType mLockType;
  QString mProcmailLockFileName;
};

#endif // kmfoldermbox_h
