/* Local Mail folder
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 *
 * Major changes:
 *
 * 23-10-97:  Stefan Taferner <taferner@kde.org>
 *   Source incompatible change! Index of messages now starts at zero
 *   instead of one.
 *   msgSubject(), msgFrom(), msgDate(), and msgStatus() are gone. Use
 *   getMsgBase()->subject() etc. instead.
 *   Use find() instead of indexOfMsg().
 *   Use count() instead of numMsgs().
 *   Use take(int) instead of detachMsg(int).
 *   Use take(find(KMMessage*)) instead of detachMsg(KMMessage*).
 */
#ifndef kmfolder_h
#define kmfolder_h

#include "kmfoldernode.h"
#include "kmmsginfo.h"
#include "kmmsglist.h"
#include "kmglobal.h"

#include <stdio.h>
#include <qvector.h>

class KMMessage;
class KMFolderDir;
class KMAcctList;

#define KMFolderInherited KMFolderNode

/** Mail folder.
 * (description will be here).
 *
 * @sect Accounts:
 *   The accounts (of KMail) that are fed into the folder are
 *   represented as the children of the folder. They are only stored here
 *   during runtime to have a reference for which accounts point to a
 *   specific folder.
 */

class KMFolder: public KMFolderNode
{
  Q_OBJECT
    friend class KMMsgBase;
  friend class KMMessage;

public:


  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolder(KMFolderDir* parent=NULL, const QString& name=NULL);
  virtual ~KMFolder();

  /** Returns full path to folder file */
  QString location() const;

  /** Returns full path to index file */
  QString indexLocation() const;

  /** Returns full path to sub directory file */
  QString subdirLocation() const;

  /** Returns the folder directory associated with this node or
      0 if no such directory exists */
  virtual KMFolderDir* child() const
    { return mChild; }

  /** Create a child folder directory and associates it with this folder */
  virtual KMFolderDir* createChildFolder();

  /** Set the folder directory associated with this node */
  virtual void setChild( KMFolderDir* aChild )
    { mChild = aChild; }

  /** Returns, if the folder can't contain mails, but only subfolder */
  virtual bool noContent() const
    { return mNoContent; }

  /** Specify, that the folder can't contain mails. */
  virtual void setNoContent(bool aNoContent)
    { mNoContent = aNoContent; }

  /** Read message at given index. Indexing starts at one */
  virtual KMMessage* getMsg(int idx);

  /** Replace KMMessage with KMMsgInfo and delete KMMessage  */
  virtual KMMsgInfo* unGetMsg(int idx);

  /** Checks if the message is already "gotten" with getMsg */
  virtual bool isMessage(int idx);

  /** Read a message and return a referece to a string */
  virtual QCString& getMsgString(int idx, QCString& mDest) = 0;

  /** Provides access to the basic message fields that are also stored
    in the index. Whenever you only need subject, from, date, status
    you should use this method instead of getMsg() because getMsg()
    will load the message if necessary and this method does not. */
  virtual KMMsgBase* getMsgBase(int idx) const { return mMsgList[idx]; }

  /** Same as getMsgBase(int). */
  const KMMsgBase* operator[](int idx) const { return mMsgList[idx]; }

  /** Same as getMsgBase(int). This time non-const. */
  KMMsgBase* operator[](int idx) { return mMsgList[idx]; }

  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  virtual KMMessage* take(int idx);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index_return = NULL) = 0;

  /** Remove (first occurance of) given message from the folder. */
  virtual void removeMsg(int i, bool imapQuiet = FALSE);
  virtual void removeMsg(KMMsgBasePtr msg);

  /** Delete messages in the folder that are older than days. Return the
   * number of deleted messages. */
  virtual int expungeOldMsg(int days);

  /** Delete messages until the size of the folder goes below size Mo.
   * Returns the number of deleted messages. */
  virtual int reduceSize( int size );

  /** Detaches the given message from it's current folder and
    adds it to this folder. Returns zero on success and an errno error
    code on failure. The index of the new message is stored in index_return
    if given. */
  virtual int moveMsg(KMMessage* msg, int* index_return = NULL);

  /** Returns the index of the given message or -1 if not found. */
  virtual int find(const KMMsgBasePtr msg) const { return mMsgList.find(msg); }

  /** Returns the index of the given message or -1 if not found. */
  virtual int find(const QString& msgIdMD5) const;

  /** Number of messages in this folder. */
  virtual int count() const { return mMsgList.count(); }

  /** Number of new or unread messages in this folder. */
  virtual int countUnread();

  /** Number of new or unread messages in this folder and all folders
      contained by this folder */
  virtual int countUnreadRecursive();

  /** Called by KMMsgBase::setStatus when status of a message has changed
      required to keep the number unread messages variable current. */
  virtual void msgStatusChanged( const KMMsgStatus oldStatus,
                                 const KMMsgStatus newStatus);

  /** Open folder for access. Does not work if the parent is not set.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise (errno). */
  virtual int open() = 0;

  /** Close folder. If force is TRUE the files are closed even if
    others still use it (e.g. other mail reader windows). */
  virtual void close(bool force=FALSE) = 0;

  /** fsync buffers to disk */
  virtual void sync() = 0;

  /** Test if folder is opened. */
  bool isOpened() const { return (mOpenCount>0); }

  /** Mark all new messages as unread. */
  virtual void markNewAsUnread();

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the
      c-library fopen call otherwise. */
  virtual int create(bool imap = FALSE) = 0;

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
  virtual int compact() = 0;

  /** Physically rename the folder. Returns zero on success and an errno
    on failure. */
  virtual int rename(const QString& newName, KMFolderDir *aParent = 0);

  /** Returns TRUE if a table of contents file is automatically created. */
  bool autoCreateIndex() const { return mAutoCreateIndex; }

  /** Allow/disallow automatic creation of a table of contents file.
    Default is TRUE. */
  virtual void setAutoCreateIndex(bool);

  /** Returns TRUE if the table of contents is dirty. This happens when
    a message is deleted from the folder. The toc will then be re-created
    when the folder is closed. */
  bool dirty() const { return mDirty; }

  /** Change the dirty flag. */
  void setDirty(bool f) { mDirty=f; }

  /** Returns TRUE if the folder contains deleted messages */
  bool needsCompacting() const { return needsCompact; }
  virtual void setNeedsCompacting(bool f) { needsCompact = f; }

  /** Registered unique serial number for the index file */
  int serialIndexId() const { return mIndexId; }

  /** If set to quiet the folder will not emit signals. */
  virtual void quiet(bool beQuiet);

  /** Is the folder read-only? */
  virtual bool isReadOnly() const = 0;

  /** Returns TRUE if the folder is a kmail system folder. These are
    the folders 'outbox', 'sent', 'trash'. The name of these
    folders is nationalized in the folder display and they cannot have
    accounts associated. Deletion is also forbidden. Etc. */
  bool isSystemFolder() const { return mIsSystemFolder; }
  void setSystemFolder(bool itIs) { mIsSystemFolder=itIs; }

  /** Returns the label of the folder for visualization. */
  virtual QString label() const;
  void setLabel(const QString& lbl) { mLabel = lbl; }

  /** Type of the folder. Inherited. */
  virtual const char* type() const;

  virtual QCString protocol() const = 0;

  /** Returns TRUE if accounts are associated with this folder. */
  bool hasAccounts() const { return (mAcctList != NULL); }

  /** Returns TRUE if this folder is associated with a mailing-list. */
  void setMailingList(bool enabled)
  { mMailingListEnabled = enabled; writeConfig(); }
  bool isMailingList() const { return mMailingListEnabled; }

  void setMailingListPostAddress(const QString &address)
  { mMailingListPostingAddress = address; writeConfig(); }
  QString mailingListPostAddress() const
  { return mMailingListPostingAddress; }

  void setMailingListAdminAddress(const QString &address)
  { mMailingListAdminAddress = address; writeConfig(); }
  QString mailingListAdminAddress() const
  { return mMailingListAdminAddress; }

  void setIdentity(const QString &identity)
  { mIdentity = identity; writeConfig(); }
  QString identity() const
  { return mIdentity; }

  /** Tell the folder that a header field that is usually used for
    the index (subject, from, ...) has changed of given message.
    This method is usually called from within KMMessage::setSubject/set... */
  virtual void headerOfMsgChanged(const KMMsgBase*);

  /** Name of the field that is used for the "From" column in index
    and listbox. */
  const char* whoField() const;

  /** Set contents of whoField. */
  void setWhoField(const QString&);

  /** A cludge to help make sure the count of unread messges is kept in sync */
  virtual void correctUnreadMsgsCount();

  /** Returns a string that can be used to identify this folder */
  virtual QString idString();

    uchar *indexStreamBasePtr() { return mIndexStreamPtr; }

signals:
  /** Emitted when the status, name, or associated accounts of this
    folder changed. */
  void changed();

  /** Emitted when a message is removed from the folder. */
  void msgRemoved(int,QString);

  /** Emitted when a message is added from the folder. */
  void msgAdded(int);

  /** Emitted when a field of the header of a specific message changed. */
  void msgHeaderChanged(int);

  /** Emmited to display a message somewhere in a status line. */
  void statusMsg(const QString&);

  /** Emitted when number of unread messages has changed. */
  void numUnreadMsgsChanged( KMFolder* );

protected:
  /** Escape a leading dot */
  virtual QString dotEscape(const QString&) const;

  /** Load message from file and store it at given index. Returns NULL
    on failure. */
  virtual KMMessage* readMsg(int idx) = 0;

  /** Read index file and fill the message-info list mMsgList. */
  virtual bool readIndex();

  /** Read index header. Called from within readIndex(). */
    virtual bool readIndexHeader(int *gv=NULL);

  /** Create index file from messages file and fill the message-info list
      mMsgList. Returns 0 on success and an errno value (see fopen) on
      failure. */
  virtual int createIndexFromContents() = 0;

  /** Write index to index-file. Returns 0 on success and errno error on
    failure. */
  virtual int writeIndex();
    bool updateIndexStreamPtr(bool just_close=FALSE);

  /** Tests whether the contents (file) is newer than the index. Returns
    TRUE if the contents has changed (and the index should be recreated),
    and FALSE otherwise. Returns TRUE if there is no index file, and
    TRUE if there is no contents (file). */
  virtual bool isIndexOutdated() = 0;

  /** Write the config file */
  virtual void writeConfig();

  /** Read the config file */
  virtual void readConfig();

  /** table of contents file */
  FILE* mIndexStream;
  /** list of index entries or messages */
  KMMsgList mMsgList;
  int mOpenCount, mQuiet;
  bool mChanged;
  /** offset of header of index file */
  unsigned long mHeaderOffset;
  /** is the automatic creation of a index file allowed ? */
  bool mAutoCreateIndex;
  /** if the index is dirty it will be recreated upon close() */
  bool mDirty;
  /** TRUE if the files of the folder are locked (writable) */
  bool mFilesLocked;
  /** nationalized label or NULL (then name() should be used) */
  QString mLabel;
  /** name of the field that is used for "From" in listbox */
  QString mWhoField;
  bool mIsSystemFolder;
  KMAcctList* mAcctList;

  bool    mMailingListEnabled;
  QString mMailingListPostingAddress;
  QString mMailingListAdminAddress;
  QString mIdentity;

  /** number of unread messages, -1 if not yet set */
  int mUnreadMsgs;
  bool mWriteConfigEnabled;
  /** sven: true if on destruct folder needs to be compacted. */
  bool needsCompact;
  /** false if index file is out of sync with mbox file */
  bool mCompactable;
  bool mNoContent;
  KMFolderDir* mChild;
  bool mConvertToUtf8;
  uchar *mIndexStreamPtr;
  int mIndexStreamPtrLength, mIndexId;
};

#endif /*kmfolder_h*/
