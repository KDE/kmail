#ifndef kmfoldermaildir_h
#define kmfoldermaildir_h

#include "kmfolder.h"

#define KMFolderMaildirInherited KMFolder

class KMFolderMaildir : public KMFolder
{
  Q_OBJECT
public:
  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderMaildir(KMFolderDir* parent=NULL, const QString& name=NULL);
  virtual ~KMFolderMaildir();

  /** Read a message and return a referece to a string */
  virtual QCString& getMsgString(int idx, QCString& mDest);

  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  virtual KMMessage* take(int idx);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index_return = NULL);

  // Called by KMMsgBase::setStatus when status of a message has changed
  // required to keep the number unread messages variable current.
  virtual void msgStatusChanged( const KMMsgStatus oldStatus,
                                 const KMMsgStatus newStatus);

  /** Open folder for access. Does not work if the parent is not set.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise (errno). */
  virtual int open();

  /** fsync buffers to disk */
  virtual void sync();

  /** Close folder. If force is TRUE the files are closed even if
    others still use it (e.g. other mail reader windows). */
  virtual void close(bool force=FALSE);

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the
      c-library fopen call otherwise. */
  virtual int create(bool imap = FALSE);

  /** Removes the folder physically from disk and empties the contents
    of the folder in memory. Note that the folder is closed during this
    process, whether there are others using it or not. */
  virtual int remove();

  /** Delete contents of folder. Forces a close *but* opens the
    folder again afterwards. Returns errno(3) error code or zero on
    success. */
  virtual int expunge();

  /** Remove deleted messages from the folder. Returns zero on success
    and an errno on failure. */
  virtual int compact();

  /** Is the folder read-only? */
  virtual bool isReadOnly() const { return false; }

  virtual QCString protocol() const { return "maildir"; }

protected:
  /** Load message from file and store it at given index. Returns NULL
    on failure. */
  virtual KMMessage* readMsg(int idx);

private:
  void readFileHeaderIntern(const QString& dir, const QString& file, KMMsgStatus status);
  QString constructValidFileName(QString& file, KMMsgStatus status);
  QString moveInternal(const QString& oldLoc, const QString& newLoc, KMMsgInfo* mi);
  QString moveInternal(const QString& oldLoc, const QString& newLoc, QString& aFileName, KMMsgStatus status);

  /** Create index file from messages file and fill the message-info list
      mMsgList. Returns 0 on success and an errno value (see fopen) on
      failure. */
  virtual int createIndexFromContents();

  /** Tests whether the contents (file) is newer than the index. Returns
    TRUE if the contents has changed (and the index should be recreated),
    and FALSE otherwise. Returns TRUE if there is no index file, and
    TRUE if there is no contents (file). */
  virtual bool isIndexOutdated();

  QStrList mIdxToFileList;
  int mIdxCount;
};
#endif /*kmfoldermaildir_h*/
