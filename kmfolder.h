/* Virtual base class for mail folder
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 *
 */
#ifndef kmfolder_h
#define kmfolder_h

// for large file support
#include <config.h>
#include <sys/types.h>
#include "kmfoldernode.h"
#include "kmmsginfo.h"
#include "kmmsglist.h"
#include "kmglobal.h"
#include "mimelib/string.h"

#include <stdio.h>
#include <qptrvector.h>

class KMMessage;
class KMFolderDir;
class KMAcctList;
class KMMsgDict;
class KMMsgDictREntry;

#define KMFolderInherited KMFolderNode


class KMFolderJob : public QObject
{
  Q_OBJECT

public:
  enum JobType { tListDirectory, tGetFolder, tCreateFolder, tDeleteMessage,
                 tGetMessage, tPutMessage, tCopyMessage, tExpireMessages };
  KMFolderJob( KMMessage *msg, JobType jt = tGetMessage, KMFolder *folder = 0  );
  KMFolderJob( QPtrList<KMMessage>& msgList, const QString& sets,
               JobType jt = tGetMessage, KMFolder *folder = 0 );
  virtual ~KMFolderJob();

  QPtrList<KMMessage> msgList() const;
  void start();
  //void KMFolder* srcFolder() const;
  //void KMFolder* destFolder() const;

signals:
  void messageRetrieved(KMMessage *);
  void messageStored(KMMessage *);
  void messageCopied(KMMessage *);
  void messageCopied(QPtrList<KMMessage>);
  void finished();
protected:
  virtual void execute()=0;
  virtual void expireMessages()=0;

  QPtrList<KMMessage> mMsgList;
  JobType             mType;
  QString             mSets;
  KMFolder*           mDestFolder;
};

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

  /** This enum indicates the status of the index file. It's returned by
      indexStatus().
   */
  enum IndexStatus { IndexOk,
                     IndexMissing,
                     IndexTooOld
  };

  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolder(KMFolderDir* parent=0, const QString& name=QString::null);
  virtual ~KMFolder();

  /** Returns the filename of the folder (reimplemented in KMFolderImap) */
  virtual QString fileName() const { return name(); }

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
  virtual const KMMsgBase* getMsgBase(int idx) const { return mMsgList[idx]; }
  virtual KMMsgBase* getMsgBase(int idx) { return mMsgList[idx]; }

  /** Same as getMsgBase(int). */
  const KMMsgBase* operator[](int idx) const { return mMsgList[idx]; }

  /** Same as getMsgBase(int). This time non-const. */
  KMMsgBase* operator[](int idx) { return mMsgList[idx]; }

  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  virtual KMMessage* take(int idx);
  virtual void take(QPtrList<KMMessage> msgList);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index_return = 0) = 0;

  /** Returns FALSE, if the message has to be retrieved from an IMAP account
   * first. In this case this function does this and cares for the rest */
  virtual bool canAddMsgNow(KMMessage* aMsg, int* aIndex_ret);

  /** Remove (first occurance of) given message from the folder. */
  virtual void removeMsg(int i, bool imapQuiet = FALSE);
  virtual void removeMsg(const KMMsgBase* msg);
  virtual void removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet = FALSE);

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
  virtual int moveMsg(KMMessage* msg, int* index_return = 0);
  virtual int moveMsg(QPtrList<KMMessage>, int* index_return = 0);

  /** Returns the index of the given message or -1 if not found. */
  virtual int find(const KMMsgBase* msg) const { return mMsgList.find((KMMsgBase*)msg); }

  /** Returns the index of the given message or -1 if not found. */
  virtual int find(const QString& msgIdMD5) const;

  /** Number of messages in this folder. */
  virtual int count(bool cache = false) const;

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

  /** Check folder for permissions
    Returns zero if readable and writable. */
  virtual int canAccess() = 0;

  /** Close folder. If force is TRUE the files are closed even if
    others still use it (e.g. other mail reader windows). */
  virtual void close(bool force=FALSE) = 0;

  /** fsync buffers to disk */
  virtual void sync() = 0;

  /** Test if folder is opened. */
  bool isOpened() const { return (mOpenCount>0); }

  /** Mark all new messages as unread. */
  virtual void markNewAsUnread();

  /** Mark all new and unread messages as read. */
  virtual void markUnreadAsRead();

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the
      c-library fopen call otherwise. */
  virtual int create(bool imap = FALSE) = 0;

  /** Removes the folder physically from disk and empties the contents
    of the folder in memory. Note that the folder is closed during this
    process, whether there are others using it or not.
    @see KMFolder::removeContents */
  virtual int remove();

  /** Delete entire folder. Forces a close *but* opens the
    folder again afterwards. Returns errno(3) error code or zero on
    success.  @see KMFolder::expungeContents */
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
  bool hasAccounts() const { return (mAcctList != 0); }

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

  void setIdentity(uint identity);
  uint identity() const { return mIdentity; }

  bool useCustomIcons() const { return mUseCustomIcons; }
  void setUseCustomIcons( bool yes ) { mUseCustomIcons = yes; }
  void setIconPaths(const QString &normal, const QString &unread)
  { mNormalIconPath = normal; mUnreadIconPath = unread; iconsFromPath(); 
    writeConfig(); if (mUseCustomIcons) mNeedsRepainting = true; }
  QString normalIconPath() const 
  { return mNormalIconPath; }
  QString unreadIconPath() const
  { return mUnreadIconPath; }
  QPixmap* normalIcon() const 
  { if ( mUseCustomIcons ) return mNormalIcon; else return 0; }
  QPixmap* unreadIcon() const
  { if ( mUseCustomIcons ) return mUnreadIcon; else return 0; }
  
  /** Tell the folder tree if repainting is required */
  bool needsRepainting() const 
  { return mNeedsRepainting; }
  /** repaint has been scheduled so stop demanding it */
  void repaintScheduled() 
  { mNeedsRepainting = false; }

  /** Tell the folder that a header field that is usually used for
    the index (subject, from, ...) has changed of given message.
    This method is usually called from within KMMessage::setSubject/set... */
  virtual void headerOfMsgChanged(const KMMsgBase*, int idx = -1);

  /** Get / set the name of the field that is used for the Sender/Receiver column in the headers (From/To) */
  QString whoField() const { return mWhoField; }
  void setWhoField(const QString& aWhoField) { mWhoField = aWhoField; /*writeConfig();*/ }

  /** Get / set the user-settings for the WhoField (From/To/Empty) */
  QString userWhoField(void) { return mUserWhoField; }
  void setUserWhoField(const QString &whoField);

  /** A cludge to help make sure the count of unread messges is kept in sync */
  virtual void correctUnreadMsgsCount();

  /** Returns a string that can be used to identify this folder */
  virtual QString idString() const;

  uchar *indexStreamBasePtr() { return mIndexStreamPtr; }
  
  bool indexSwapByteOrder() { return mIndexSwapByteOrder; }
  int  indexSizeOfLong() { return mIndexSizeOfLong; }

  /**
   * Set whether this folder automatically expires messages.
   */
  void
  setAutoExpire(bool enabled) {
    expireMessages = enabled;
    writeConfig();
  }

  /**
   * Does this folder automatically expire old messages?
   */
  bool    isAutoExpire() { return expireMessages; }

  /**
   * Set the maximum age for unread messages in this folder.
   * Age should not be negative. Units are set using
   * setUnreadExpireUnits().
   */
  void
  setUnreadExpireAge(int age) {
    if (age >= 0) {
      unreadExpireAge = age;
      writeConfig();
    }
  }

  /**
   * Set units to use for expiry of unread messages.
   * Values are 1 = days, 2 = weeks, 3 = months.
   */
  void
  setUnreadExpireUnits(ExpireUnits units) {
    if (units >= expireNever && units < expireMaxUnits) {
      unreadExpireUnits = units;
    }
  }

  /**
   * Set the maximum age for read messages in this folder.
   * Age should not be negative. Units are set using
   * setReadExpireUnits().
   */
  void
  setReadExpireAge(int age) {
    if (age >= 0) {
      readExpireAge = age;
      writeConfig();
    }
  }

  /**
   * Set units to use for expiry of read messages.
   * Values are 1 = days, 2 = weeks, 3 = months.
   */
  void
  setReadExpireUnits(ExpireUnits units) {
    if (units >= expireNever && units <= expireMaxUnits) {
      readExpireUnits = units;
    }
  }

  /**
   * Get the age at which unread messages are expired.
   * Units are determined by getUnreadExpireUnits().
   */
  int getUnreadExpireAge() { return unreadExpireAge; }

  /**
   * Get the age at which read messages are expired.
   * Units are determined by getReadExpireUnits().
   */
  int getReadExpireAge() { return readExpireAge; }

  /**
   * Units getUnreadExpireAge() is returned in.
   * 1 = days, 2 = weeks, 3 = months.
   */
  ExpireUnits getUnreadExpireUnits() { return unreadExpireUnits; }

  /**
   * Units getReadExpireAge() is returned in.
   * 1 = days, 2 = weeks, 3 = months.
   */
  ExpireUnits getReadExpireUnits() { return readExpireUnits; }

  void expireOldMessages();

  /** Write index to index-file. Returns 0 on success and errno error on
    failure. */
  virtual int writeIndex();

  /** Inserts messages into the message dictionary.  Might be called
    during kernel initialization. */
  void fillMsgDict(KMMsgDict *dict);
  
  /** Writes the message serial number file. */
  int writeMsgDict(KMMsgDict *dict = 0);
  
  /** Touches the message serial number file. */
  int touchMsgDict();
  
  /** Append message to end of message serial number file. */
  int appendtoMsgDict(int idx = -1);
  
  /** Sets the reverse-dictionary for this folder. */
  void setRDict(KMMsgDictREntry *rentry);
  
  /** Returns the reverse-dictionary for this folder. */
  KMMsgDictREntry *rDict() const { return mRDict; }
  
  /** Set the status of the message at index @p idx to @p status. */
  virtual void setStatus(int idx, KMMsgStatus status);
  
  /** Set the status of the message @p msg to @p status.  The message
   * should be in the current folder. */
  void setStatus(KMMsgBase *msg, KMMsgStatus status);

  /** Set the status of the message(s) in the QValueList @p ids to @p status. */
  virtual void setStatus(QValueList<int>& ids, KMMsgStatus status);

signals:
  /** Emitted when the status, name, or associated accounts of this
    folder changed. */
  void changed();

  /** Emitted when a message is removed from the folder. */
  void msgRemoved(int,QString);
  void msgRemoved(KMFolder*);

  /** Emitted when a message is added from the folder. */
  void msgAdded(int);
  void msgAdded(KMFolder*);

  /** Emitted when a field of the header of a specific message changed. */
  void msgHeaderChanged(int);

  /** Emmited to display a message somewhere in a status line. */
  void statusMsg(const QString&);

  /** Emitted when number of unread messages has changed. */
  void numUnreadMsgsChanged( KMFolder* );

public slots:
  /** Add the message to the folder after it has been retrieved from an IMAP
      server */
  virtual void reallyAddMsg(KMMessage* aMsg);

  /** Add a copy of the message to the folder after it has been retrieved
      from an IMAP server */
  virtual void reallyAddCopyOfMsg(KMMessage* aMsg);

protected:
  /** Escape a leading dot */
  virtual QString dotEscape(const QString&) const;

  /** Load message from file and store it at given index. Returns 0
    on failure. */
  virtual KMMessage* readMsg(int idx) = 0;

  /** Read index file and fill the message-info list mMsgList. */
  virtual bool readIndex();

  /** Read index header. Called from within readIndex(). */
    virtual bool readIndexHeader(int *gv=0);

  /** Create index file from messages file and fill the message-info list
      mMsgList. Returns 0 on success and an errno value (see fopen) on
      failure. */
  virtual int createIndexFromContents() = 0;

  bool updateIndexStreamPtr(bool just_close=FALSE);

  /** Tests whether the contents of this folder is newer than the index.
      Should return IndexTooOld if the index is older than the contents.
      Should return IndexMissing if there is contents but no index.
      Should return IndexOk if the folder doesn't exist anymore "physically"
      or if the index is not older than the contents.
  */
  virtual IndexStatus indexStatus() = 0;

  /** Called by KMFolder::remove() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int removeContents() = 0;
  
  /** Called by KMFolder::expunge() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int expungeContents() = 0;
  
  /** Write the config file */
  virtual void writeConfig();

  /** Read the config file */
  virtual void readConfig();

  /** tries to create icons from paths */
  virtual void iconsFromPath();
  
  /** table of contents file */
  FILE* mIndexStream;
  /** list of index entries or messages */
  KMMsgList mMsgList;
  int mOpenCount, mQuiet;
  bool mChanged;
  /** offset of header of index file */
  off_t mHeaderOffset;
  /** is the automatic creation of a index file allowed ? */
  bool mAutoCreateIndex;
  /** if the index is dirty it will be recreated upon close() */
  bool mDirty;
  /** TRUE if the files of the folder are locked (writable) */
  bool mFilesLocked;
  /** nationalized label or QString::null (then name() should be used) */
  QString mLabel;
  /** name of the field that is used for "From" in listbox */
  QString mWhoField, mUserWhoField;
  bool mIsSystemFolder;
  KMAcctList* mAcctList;

  bool    mMailingListEnabled;
  QString mMailingListPostingAddress;
  QString mMailingListAdminAddress;
  uint    mIdentity;

  /** Custom pixmaps to display in the tree, none by default */
  QPixmap *mNormalIcon;
  QPixmap *mUnreadIcon;
  QString mNormalIconPath;
  QString mUnreadIconPath;
  bool    mUseCustomIcons;
  bool    mNeedsRepainting;
  
  /** number of unread messages, -1 if not yet set */
  int mUnreadMsgs, mGuessedUnreadMsgs;
  int mTotalMsgs;
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
  bool mIndexSwapByteOrder; // Index file was written with swapped byte order
  int mIndexSizeOfLong; // Index file was written with longs of this size

  /** Support for automatic expiry of old messages */
  bool         expireMessages;          // TRUE if old messages are expired
  int          unreadExpireAge;         // Given in unreadExpireUnits
  int          readExpireAge;           // Given in readExpireUnits
  ExpireUnits  unreadExpireUnits;
  ExpireUnits  readExpireUnits;

  int          daysToExpire(int num, ExpireUnits units);
  
  /** Points at the reverse dictionary for this folder. */
  KMMsgDictREntry *mRDict;
};

#endif /*kmfolder_h*/
