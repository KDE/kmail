/* Local Mail folder
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 * This code is under GPL
 */
#ifndef kmfolder_h
#define kmfolder_h

#include "kmfoldernode.h"
#include "kmmsginfo.h"

#include <stdio.h>
#include <qarray.h>
#include <qstring.h>

typedef QArray<KMMessage> KMMessageList;

class KMMessage;
class KMFolderDir;
class KMAcctList;

#define KMFolderInherited KMFolderNode

/* Mail folder.
 *
 * Accounts:
 *   The accounts (of KMail) that are fed into the folder are
 *   represented as the children of the folder. They are only stored here
 *   during runtime to have a reference for which accounts point to a
 *   specific folder.
 */

class KMFolder: public KMFolderNode
{
  Q_OBJECT

public:
  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolder(KMFolderDir* parent=NULL, const char* name=NULL);
  virtual ~KMFolder();

  /** Returns full path to folder file */
  const QString location(void) const;

  /** Returns full path to table of contents file */
  const QString indexLocation(void) const;

  /** Read message at given index. Indexing starts at one to stay
    compatible with imap-lib */
  virtual KMMessage* getMsg(int index);

  /** Detach message from this folder. Usable to call addMsg()
    with the message for another folder. */
  virtual void detachMsg(int index);
  virtual void detachMsg(KMMessage* msg);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index_return = NULL);

  /** Detaches the given message from it's current folder and
    adds it to this folder. Returns zero on success and an errno error
    code on failure. The index of the new message is stored in index_return
    if given. */
  virtual int moveMsg(KMMessage* msg, int* index_return = NULL);

  /** Returns the index of the given message or -1 if not found. */
  virtual int indexOfMsg(const KMMessage*) const;

  /** Total number of messages in this folder (may include already deleted
   messages) */
  virtual long numMsgs(void) const { return mMsgs; }

  /** Number of unread messages */
  virtual int numUnreadMsgs(void) const { return mUnreadMsgs; }

  /** Number of active (not deleted) messages in folder */
  virtual int numActiveMsgs(void) const { return mActiveMsgs; }

  virtual int isValid(unsigned long);

  /** Open folder for access. Does not work if the parent is not set.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise (errno). */
  virtual int open(void);

  /** Close folder. If force is TRUE the files are closed even if
   others still use it (e.g. other mail reader windows). */
  virtual void close(bool force=FALSE);

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the 
      c-library fopen call otherwise. */
  virtual int create(void);

  /** Removes the folder physically from disk and empties the contents
    of the folder in memory. Note that the folder is closed during this
    process, whether there are others using it or not. */
  virtual int remove(void);

  /** Delete contents of folder. Forces a close *but* opens the
    folder again afterwards. Returns errno(3) error code or zero on 
    success. */
  virtual int expunge(void);

  /** Sync all TOC-changes to file. Returns zero on success and an errno
    on failure. */
  virtual int sync(void);

  /** Returns TRUE if a table of contents file is automatically created. */
  bool autoCreateIndex(void) const { return mAutoCreateIndex; }

  /** Allow/disallow automatic creation of a table of contents file.
    Default is TRUE. */
  virtual void setAutoCreateIndex(bool);

  /** Returns TRUE if the table of contents is dirty. This happens when
    a message is deleted from the folder. The toc will then be re-created
    when the folder is closed. */
  bool dirty(void) const { return mDirty; }

  /** If set to quiet the folder will not emit signals. */
  virtual void quiet(bool beQuiet);

  /** Return "Subject:" of given message without reading the message.*/
  virtual const char* msgSubject(int msgId) const;

  /** Return "Date:" of given message without reading the message.*/
  virtual const char* msgDate(int msgId) const;

  /** Return "From:" of given message without reading the message.*/
  virtual const char* msgFrom(int msgId) const;

  /** Return "Status:" of given message without reading the message.*/
  virtual KMMessage::Status msgStatus(int msgId) const;

  /** Valid parameters for sort() */
  typedef enum { sfSubject=1, sfFrom=2, sfDate=3 } SortField;

  /** Sort folder by given field. Actually sorts the index. */
  virtual void sort(KMFolder::SortField field=KMFolder::sfSubject);

  /** Is the folder read-only? */
  virtual bool isReadOnly(void) const { return !mFilesLocked; }

  /** Returns TRUE if the folder is a kmail system folder. These are
    the folders 'outbox', 'sent', 'trash'. The name of these
    folders is nationalized in the folder display and they cannot have
    accounts associated. Deletion is also forbidden. Etc. */
  bool isSystemFolder(void) const { return mIsSystemFolder; }
  void setSystemFolder(bool itIs) { mIsSystemFolder=itIs; }

  /** Returns the label of the folder for visualization. */
  virtual const QString label(void) const;
  void setLabel(const QString lbl) { mLabel = lbl; }

  /** Type of the folder. Inherited. */
  virtual const char* type(void) const;

  /** Returns TRUE if accounts are associated with this folder. */
  bool hasAccounts(void) const { return (mAcctList != NULL); }

signals:
  /** Emitted when the status, name, or associated accounts of this
    folder changed. */
  void changed();

  /** Emitted when a message is removed from the folder. */
  void msgRemoved(int);

  /** Emitted when a message is added from the folder. */
  void msgAdded(int);

  /** Emitted when a field of the header of a specific message changed. */
  void msgHeaderChanged(int);

  /** Status messages. */
  void statusMsg(const char*);


protected:
  friend class KMMessage;
  friend int msgSortCompFunc(const void* a, const void* b);

  /** Called from KMMessage::setStatus(). Do not use directly. */
  virtual void setMsgStatus(KMMessage*, KMMessage::Status);

  /** Load message from file. */
  virtual void readMsg(int msgNo);

  /** Read index file and fill the message-info list mMsgList. */
  virtual void readIndex(void);

  /** Read index header. Called from within readIndex(). */
  virtual bool readIndexHeader(void);

  /** Create index file from messages file and fill the message-info list 
      mMsgList. Returns 0 on success and an errno value (see fopen) on 
      failure. */
  virtual int createIndexFromContents(void);

  /** Write index to index-file. Returns 0 on success and errno error on
    failure. */
  virtual int writeIndex(void);

  /** Remove msg-info from mMsgInfo array. */
  void removeMsgInfo(int id);

  /** Change the dirty flag. */
  void setDirty(bool f) { mDirty=f; }

  /** Lock mail folder files. Called by ::open(). Returns 0 on success and
    an errno error code on failure. */
  virtual int lock(void);

  /** Unlock mail folder files. Called by ::close().  Returns 0 on success
    and an errno error code on failure. */
  virtual int unlock(void);

  FILE* mStream;	// file with the messages
  FILE* mIndexStream;	// table of contents file
  int mMsgs, mUnreadMsgs, mActiveMsgs;
  KMMsgInfoList mMsgInfo; // list of toc entries, one per message
  int mOpenCount, mQuiet;
  unsigned long mHeaderOffset; // offset of header of toc file
  bool mAutoCreateIndex;  // is the automatic creation of a toc file allowed ?
  bool mDirty; // if the index is dirty it will be recreated upon close()
  bool mFilesLocked; // TRUE if the files of the folder are locked (writable)
  QString mLabel; // nationalized label or NULL (then name() should be used)
  bool mIsSystemFolder;
  KMAcctList* mAcctList;
};

#endif /*kmfolder_h*/
