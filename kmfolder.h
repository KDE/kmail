/* Local Mail folder
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmfolder_h
#define kmfolder_h

#include "kmfoldernode.h"
#include "kmmsginfo.h"
#include <stdio.h>
#include <qarray.h>

typedef QArray<KMMessage> KMMessageList;

class KMMessage;
class KMFolderDir;

#define KMFolderInherited KMFolderNode

class KMFolder: public KMFolderNode
{
  Q_OBJECT

public:
  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolder(KMFolderDir* parent=NULL, const char* name=NULL);
  virtual ~KMFolder();

  /** Type of the folder: "plain" or "account" (maybe others later). */
  virtual const char* type(void) const;

  /** Returns full path to folder file */
  const QString location(void) const;

  /** Returns full path to table of contents file */
  const QString tocLocation(void) const;

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

  /** Try to lock the folder. The folder has to be open. Returns 0 
    on success or an errno error code on failure. The toc file is
    not locked. */
  virtual int lock(bool sharedLock=FALSE);

  /** Unlock a previously locked folder. */
  virtual void unlock(void);

  /** Read the toc header only such that possible header information 
    (e.g. information about the associated accounts for KMAcctFolder)
    is set. The folder is *not* open afterwards. This method does nothing
    if the folder is already open and fails if the path was not set
    with setPath. */
  virtual int readHeader(void);

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
  bool autoCreateToc(void) const { return mAutoCreateToc; }

  /** Allow/disallow automatic creation of a table of contents file.
    Default is TRUE. */
  virtual void setAutoCreateToc(bool);

  /** Returns TRUE if the table of contents is dirty. This happens when
    a message is deleted from the folder. The toc will then be re-created
    when the folder is closed. */
  bool tocDirty(void) const { return mTocDirty; }

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

protected:
  friend class KMMessage;
  friend int msgSortCompFunc(const void* a, const void* b);

  /** Called from KMMessage::setStatus(). Do not use directly. */
  virtual void setMsgStatus(KMMessage*, KMMessage::Status);

  /** Read message from file. */
  virtual void readMsg(int msgNo);

  /** Read table of contents file from messages file and fill the
     message-info list mMsgList. */
  virtual void readToc(void);

  /** This method can be inherited to read a custom toc header. */
  virtual void readTocHeader(void);

  /** Create table of contents file from messages file and fill the
     message-info list mMsgList. Returns 0 on success and an errno 
     value (see fopen) on failure. */
  virtual int createTocFromContents(void);

  /** Completely (re)write table of contents file. */
  virtual int writeToc(void);

  /** This method can be inherited to create a custom toc header.
     Returns zero on success and an errno value (see fopen) on failure.*/
  virtual int createTocHeader(void);

  /** Remove msg-info from mMsgInfo array. */
  void removeMsgInfo(int id);

  /** Change the Toc-dirty flag. */
  void setTocDirty(bool f) { mTocDirty=f; }

  FILE* mStream;	// file with the messages
  FILE* mTocStream;	// table of contents file
  int mMsgs, mUnreadMsgs, mActiveMsgs;
  KMMsgInfoList mMsgInfo; // list of toc entries, one per message
  int mOpenCount, mQuiet;
  unsigned long mHeaderOffset; // offset of header of toc file
  bool mAutoCreateToc;  // is the automatic creation of a toc file allowed ?
  bool mTocDirty; // if the Toc is dirty it will be recreated upon close()
};

#endif /*kmfolder_h*/
