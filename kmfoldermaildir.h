#ifndef kmfoldermaildir_h
#define kmfoldermaildir_h

#include "kmfolderindex.h"


class KMFolderMaildir;
namespace KMail {
  class FolderJob;
  class MaildirJob;
  class AttachmentStrategy;
}
using KMail::FolderJob;
using KMail::MaildirJob;
using KMail::AttachmentStrategy;

class KMFolderMaildir : public KMFolderIndex
{
  Q_OBJECT
  friend class MaildirJob;
public:
  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderMaildir(KMFolder* folder, const char* name=0);
  virtual ~KMFolderMaildir();

  /** Returns the type of this folder */
  virtual KMFolderType folderType() const { return KMFolderTypeMaildir; }

  /** Read a message and return a referece to a string */
  virtual QCString& getMsgString(int idx, QCString& mDest);
  virtual DwString getDwString(int idx);

  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  virtual KMMessage* take(int idx);
  virtual void take(QPtrList<KMMessage> msgList)
  { return KMFolderIndex::take(msgList); }

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index_return = 0);

  /** Remove (first occurrence of) given message from the folder. */
  virtual void removeMsg(int i, bool imapQuiet = FALSE);
  virtual void removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet = FALSE)
  { return KMFolderIndex::removeMsg(msgList, imapQuiet); }

  // Called by KMMsgBase::setStatus when status of a message has changed
  // required to keep the number unread messages variable current.
  virtual void msgStatusChanged( const KMMsgStatus oldStatus,
                                 const KMMsgStatus newStatus,
				 int idx);

  /** Open folder for access.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise (errno). */
  virtual int open();

  virtual int canAccess();

  /** fsync buffers to disk */
  virtual void sync();

  /** Close folder. If force is TRUE the files are closed even if
    others still use it (e.g. other mail reader windows). */
  virtual void close(bool force=FALSE);

  /** Create the necessary folders for a maildir folder. Usually you will
      want to use create() instead.

      @param folderPath the full path of the folder as returned by location()
      @return 0 on success and an error code (cf. man 3 errno) otherwise
   */
  static int createMaildirFolders( const QString & folderPath );

  static QString constructValidFileName( const QString & filename = QString(),
                                         KMMsgStatus status = KMMsgStatusNew );

  static bool removeFile( const QString & folderPath,
                          const QString & filename );

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the
      c-library fopen call otherwise. */
  virtual int create(bool imap = FALSE);

  /** Remove some deleted messages from the folder. Returns zero on success
    and an errno on failure. This is only for use from MaildirCompactionJob. */
  int compact( unsigned int startIndex, int nbMessages, const QStringList& entryList, bool& done );

  /** Remove deleted messages from the folder. Returns zero on success
    and an errno on failure. */
  virtual int compact( bool silent );

  /** Is the folder read-only? */
  virtual bool isReadOnly() const { return false; }

protected:
  virtual FolderJob* doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder,
                                  QString partSpecifier, const AttachmentStrategy *as ) const;
  virtual FolderJob* doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                  FolderJob::JobType jt, KMFolder *folder ) const;
  /** Load message from file and store it at given index. Returns 0
    on failure. */
  virtual KMMessage* readMsg(int idx);

  /** Called by KMFolder::remove() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int removeContents();

  /** Called by KMFolder::expunge() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int expungeContents();

  /** Create index file from messages file and fill the message-info list
      mMsgList. Returns 0 on success and an errno value (see fopen) on
      failure. */
  virtual int createIndexFromContents();

  /**
   * Internal helper called by addMsg. If stripUid is true it will remove any
   * uid headers and uid index setting before writing. KMFolderCachedImap needs this
   * but can't do it itself, since the final take() which removes the original mail
   * from the source folder, in moves, needs to happen after the adding, for safety
   * reasons, but needs the uid, in case the source folder was an imap folder, to
   * delete the original.
   * TODO: Avoid this by moving the take() out of the addMsg() methods and moving it
   * into the KMMoveCommand, where it can safely happen at a much higher level. */
  int addMsgInternal( KMMessage* msg, int* index_return = 0, bool stripUid=false );

private:
  void readFileHeaderIntern(const QString& dir, const QString& file, KMMsgStatus status);
  QString moveInternal(const QString& oldLoc, const QString& newLoc, KMMsgInfo* mi);
  QString moveInternal(const QString& oldLoc, const QString& newLoc, QString& aFileName, KMMsgStatus status);
  bool removeFile(const QString& filename);

  /** Tests whether the contents of this folder is newer than the index.
      Returns IndexTooOld if the index is older than the contents.
      Returns IndexMissing if there is no index.
      Returns IndexOk if the index is not older than the contents.
  */
  virtual IndexStatus indexStatus();

  QStrList mIdxToFileList;
  int mIdxCount;
};
#endif /*kmfoldermaildir_h*/
