/* -*- mode: C++ -*-
 * Virtual base class for mail folder
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 *
 */
#ifndef kmfolder_h
#define kmfolder_h

// for large file support
#include <config.h>

#include "kmfoldernode.h"
#include "kmfoldertype.h"
#include "kmmsginfo.h"
#include "kmglobal.h"
#include "folderjob.h"
using KMail::FolderJob;
#include "mailinglist-magic.h"
using KMail::MailingList;

#include "mimelib/string.h"

#include <qptrvector.h>
#include <sys/types.h>
#include <stdio.h>
#include <kshortcut.h>

class KMMessage;
class KMFolderDir;
class KMAcctList;
class KMMsgDict;
class KMMsgDictREntry;
class QTimer;
class FolderStorage;
class KMFolderTreeItem;

namespace KMail {
   class AttachmentStrategy;
}
using KMail::AttachmentStrategy;

typedef QValueList<Q_UINT32> SerNumList;

/** Mail folder.
 * (description will be here).
 *
 * @section Accounts:
 *   The accounts (of KMail) that are fed into the folder are
 *   represented as the children of the folder. They are only stored here
 *   during runtime to have a reference for which accounts point to a
 *   specific folder.
 */

class KMFolder: public KMFolderNode
{
  Q_OBJECT
  friend class KMFolderJob;
public:


  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolder( KMFolderDir* parent, const QString& name,
            KMFolderType aFolderType );
  ~KMFolder();

  /** This is used by the storage to read the folder specific configuration */
  void readConfig( KConfig* config );

  /** This is used by the storage to save the folder specific configuration */
  void writeConfig( KConfig* config ) const;

  FolderStorage* storage() { return mStorage; }
  /** if the folder is const, the storage should be as well */
  const FolderStorage* storage() const { return mStorage; }

  /** Returns the type of this folder */
  KMFolderType folderType() const;

  /** Returns the filename of the folder (reimplemented in KMFolderImap) */
  QString fileName() const;

  /** Returns full path to folder file */
  QString location() const;

  /** Returns full path to index file */
  QString indexLocation() const;

  /** Returns full path to sub directory file */
  QString subdirLocation() const;

  /** Returns the folder directory associated with this node or
      0 if no such directory exists */
  KMFolderDir* child() const
    { return mChild; }

  /** Create a child folder directory and associates it with this folder */
  KMFolderDir* createChildFolder();

  /** Set the folder directory associated with this node */
  void setChild( KMFolderDir* aChild );

  /** Returns, if the folder can't contain mails, but only subfolder */
  bool noContent() const;

  /** Specify, that the folder can't contain mails. */
  void setNoContent(bool aNoContent);

  /** Returns, if the folder can't have children */
  bool noChildren() const;

  /** Specify, that the folder can't have children */
  void setNoChildren(bool aNoChildren);

  /** Read message at given index. Indexing starts at zero */
  KMMessage* getMsg(int idx);

  /** Replace KMMessage with KMMsgInfo and delete KMMessage  */
  KMMsgInfo* unGetMsg(int idx);

  /** Checks if the message is already "gotten" with getMsg */
  bool isMessage(int idx);

  /** Read a message and return a referece to a string */
  QCString& getMsgString(int idx, QCString& mDest);

  /** Read a message and returns a DwString */
  DwString getDwString(int idx);

  /**
   * Removes and deletes all jobs associated with the particular message
   */
  void ignoreJobsForMessage( KMMessage* );

  /**
   * These methods create respective FolderJob (You should derive FolderJob
   * for each derived KMFolder).
   */
  FolderJob* createJob( KMMessage *msg, FolderJob::JobType jt = FolderJob::tGetMessage,
                        KMFolder *folder = 0, QString partSpecifier = QString::null,
                        const AttachmentStrategy *as = 0 ) const;
  FolderJob* createJob( QPtrList<KMMessage>& msgList, const QString& sets,
                        FolderJob::JobType jt = FolderJob::tGetMessage,
                        KMFolder *folder = 0 ) const;

  /** Provides access to the basic message fields that are also stored
    in the index. Whenever you only need subject, from, date, status
    you should use this method instead of getMsg() because getMsg()
    will load the message if necessary and this method does not. */
  const KMMsgBase* getMsgBase(int idx) const;
  KMMsgBase* getMsgBase(int idx);

  /** Same as getMsgBase(int). */
  const KMMsgBase* operator[](int idx) const;

  /** Same as getMsgBase(int). This time non-const. */
  KMMsgBase* operator[](int idx);

  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  KMMessage* take(int idx);
  void take(QPtrList<KMMessage> msgList);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  int addMsg(KMMessage* msg, int* index_return = 0);

  /** (Note(bo): This needs to be fixed better at a later point.)
      This is overridden by dIMAP because addMsg strips the X-UID
      header from the mail. */
  int addMsgKeepUID(KMMessage* msg, int* index_return = 0);

  /** 
   * Adds the given messages to the folder. Behaviour is identical 
   * to addMsg(msg) 
   */
  int addMsg(QPtrList<KMMessage>&, QValueList<int>& index_return);

  /** Called by derived classes implementation of addMsg.
      Emits msgAdded signals */
  void emitMsgAddedSignals(int idx);

  /** Remove (first occurrence of) given message from the folder. */
  void removeMsg(int i, bool imapQuiet = FALSE);
  void removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet = FALSE);

  /** Delete messages in the folder that are older than days. Return the
   * number of deleted messages. */
  int expungeOldMsg(int days);

  /** Detaches the given message from it's current folder and
    adds it to this folder. Returns zero on success and an errno error
    code on failure. The index of the new message is stored in index_return
    if given. */
  int moveMsg(KMMessage* msg, int* index_return = 0);
  int moveMsg(QPtrList<KMMessage>, int* index_return = 0);

  /** Returns the index of the given message or -1 if not found. */
  int find(const KMMsgBase* msg) const;
  int find( const KMMessage * msg ) const;

  /** Number of messages in this folder. */
  int count(bool cache = false) const;

  /** Number of new or unread messages in this folder. */
  int countUnread();

  /** Number of new or unread messages in this folder and all folders
      contained by this folder */
  int countUnreadRecursive();

  /** Called by KMMsgBase::setStatus when status of a message has changed
      required to keep the number unread messages variable current. */
  void msgStatusChanged( const KMMsgStatus oldStatus,
                         const KMMsgStatus newStatus,
                         int idx);

  /** Open folder for access.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise (errno). */
  int open();

  /** Check folder for permissions
    Returns zero if readable and writable. */
  int canAccess();

  /** Close folder. If force is TRUE the files are closed even if
    others still use it (e.g. other mail reader windows). */
  void close(bool force=FALSE);

  /** fsync buffers to disk */
  void sync();

  /** Test if folder is opened. */
  bool isOpened() const;

  /** Mark all new messages as unread. */
  void markNewAsUnread();

  /** Mark all new and unread messages as read. */
  void markUnreadAsRead();

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the
      c-library fopen call otherwise. */
  int create(bool imap = FALSE);

  /** Removes the folder physically from disk and empties the contents
    of the folder in memory. Note that the folder is closed during this
    process, whether there are others using it or not.
    @see KMFolder::removeContents */
  void remove();

  /** Delete entire folder. Forces a close *but* opens the
    folder again afterwards. Returns errno(3) error code or zero on
    success.  @see KMFolder::expungeContents */
  int expunge();

  enum CompactOptions { CompactLater, CompactNow, CompactSilentlyNow };
  /**
   * Compact this folder. Options:
   * CompactLater: schedule it as a background task
   * CompactNow: do it now, and inform the user of the result (manual compaction)
   * CompactSilentlyNow: do it now, and keep silent about it (e.g. for outbox)
   */
  void compact( CompactOptions options );

  /** Physically rename the folder. Returns zero on success and an errno
    on failure. */
  int rename(const QString& newName, KMFolderDir *aParent = 0);

  /** Returns TRUE if a table of contents file is automatically created. */
  bool autoCreateIndex() const;

  /** Allow/disallow automatic creation of a table of contents file.
    Default is TRUE. */
  void setAutoCreateIndex(bool);

  /** Returns TRUE if the table of contents is dirty. This happens when
    a message is deleted from the folder. The toc will then be re-created
    when the folder is closed. */
  bool dirty() const;

  /** Change the dirty flag. */
  void setDirty(bool f);

  /** Returns TRUE if the folder contains deleted messages */
  bool needsCompacting() const;
  void setNeedsCompacting(bool f);

  /** If set to quiet the folder will not emit msgAdded(idx) signal.
    This is necessary because adding the messages to the listview
    one by one as they come in ( as happens on msgAdded(idx) ) is
    very slow for large ( >10000 ) folders. For pop, where whole
    bodies are downloaded this is not an issue, but for imap, where
    we only download headers it becomes a bottleneck. We therefore
    set the folder quiet() and rebuild the listview completely once
    the complete folder has been checked. */
  void quiet(bool beQuiet);

  /** Is the folder read-only? */
  bool isReadOnly() const;

  /** Returns TRUE if the folder is a kmail system folder. These are
    the folders 'outbox', 'sent', 'trash'. The name of these
    folders is nationalized in the folder display and they cannot have
    accounts associated. Deletion is also forbidden. Etc. */
  bool isSystemFolder() const { return mIsSystemFolder; }
  void setSystemFolder(bool itIs) { mIsSystemFolder=itIs; }

  /** Returns the label of the folder for visualization. */
  virtual QString label() const;
  void setLabel( const QString& l ) { mLabel = l; }

  /** Set the label that is used as a system default */
  virtual QString systemLabel() const { return mSystemLabel; }
  void setSystemLabel( const QString& l ) { mSystemLabel = l; }

  /** URL of the node for visualization purposes. */
  virtual QString prettyURL() const;

  /** Type of the folder. Inherited. */
  const char* type() const;

  /** Returns TRUE if accounts are associated with this folder. */
  bool hasAccounts() const;

  /** Returns TRUE if this folder is associated with a mailing-list. */
  void setMailingListEnabled( bool enabled );
  bool isMailingListEnabled() const { return mMailingListEnabled; }

  void setMailingList( const MailingList& mlist );
  MailingList mailingList() const
  { return mMailingList; }
  QString mailingListPostAddress() const;

  void setIdentity(uint identity);
  uint identity() const { return mIdentity; }

  /** Get / set the name of the field that is used for the Sender/Receiver column in the headers (From/To) */
  QString whoField() const { return mWhoField; }
  void setWhoField(const QString& aWhoField);

  /** Get / set the user-settings for the WhoField (From/To/Empty) */
  QString userWhoField(void) { return mUserWhoField; }
  void setUserWhoField(const QString &whoField,bool writeConfig=true);

  /** A cludge to help make sure the count of unread messges is kept in sync */
  void correctUnreadMsgsCount();

  /** Returns a string that can be used to identify this folder */
  QString idString() const;

  /**
   * Set whether this folder automatically expires messages.
   */
  void setAutoExpire(bool enabled);

  /**
   * Does this folder automatically expire old messages?
   */
  bool isAutoExpire() const { return mExpireMessages; }

  /**
   * Set the maximum age for unread messages in this folder.
   * Age should not be negative. Units are set using
   * setUnreadExpireUnits().
   */
  void setUnreadExpireAge(int age);

  /**
   * Set units to use for expiry of unread messages.
   * Values are 1 = days, 2 = weeks, 3 = months.
   */
  void setUnreadExpireUnits(ExpireUnits units);

  /**
   * Set the maximum age for read messages in this folder.
   * Age should not be negative. Units are set using
   * setReadExpireUnits().
   */
  void setReadExpireAge(int age);

  /**
   * Set units to use for expiry of read messages.
   * Values are 1 = days, 2 = weeks, 3 = months.
   */
  void setReadExpireUnits(ExpireUnits units);

  /**
   * Get the age at which unread messages are expired.
   * Units are determined by getUnreadExpireUnits().
   */
  int getUnreadExpireAge() const { return mUnreadExpireAge; }

  /**
   * Get the age at which read messages are expired.
   * Units are determined by getReadExpireUnits().
   */
  int getReadExpireAge() const { return mReadExpireAge; }

  /**
   * Units getUnreadExpireAge() is returned in.
   * 1 = days, 2 = weeks, 3 = months.
   */
  ExpireUnits getUnreadExpireUnits() const { return mUnreadExpireUnits; }

  /**
   * Units getReadExpireAge() is returned in.
   * 1 = days, 2 = weeks, 3 = months.
   */
  ExpireUnits getReadExpireUnits() const { return mReadExpireUnits; }

  enum ExpireAction { ExpireDelete, ExpireMove };
  /**
   * What should expiry do? Delete or move to another folder?
   */
  ExpireAction expireAction() const { return mExpireAction; }
  void setExpireAction( ExpireAction a );

  /**
   * If expiry should move to folder, return the ID of that folder
   */
  QString expireToFolderId() const { return mExpireToFolderId; }
  void setExpireToFolderId( const QString& id );

  /**
   * Expire old messages in this folder.
   * If immediate is true, do it immediately; otherwise schedule it for later
   */
  void expireOldMessages( bool immediate );

  /** Write index to index-file. Returns 0 on success and errno error on
    failure. */
  int writeIndex( bool createEmptyIndex = false );

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
  KMMsgDictREntry *rDict() const;

  /** Set the status of the message at index @p idx to @p status. */
  void setStatus(int idx, KMMsgStatus status, bool toggle=false);

  /** Set the status of the message(s) in the QValueList @p ids to @p status. */
  void setStatus(QValueList<int>& ids, KMMsgStatus status, bool toggle=false);

  /** Icon related methods */
  bool useCustomIcons() const { return mUseCustomIcons; }
  void setUseCustomIcons(bool useCustomIcons) { mUseCustomIcons = useCustomIcons; }
  QString normalIconPath() const { return mNormalIconPath; }
  QString unreadIconPath() const { return mUnreadIconPath; }
  void setIconPaths(const QString &normalPath, const QString &unreadPath);

  void removeJobs();

  /** Convert "\r\n" line endings in "\n" line endings. The conversion
      happens in place. Returns the length of the resulting string.
  */
  static size_t crlf2lf( char* str, const size_t strLen );

  void daysToExpire( int& unreadDays, int& readDays );

  /**
   * If this folder has a special trash folder set, return it. Otherwise
   * return 0.
   */
  KMFolder* trashFolder() const;

  /**
   * Returns true if the replies to mails from this folder should be
   * put in the same folder.
   */
  bool putRepliesInSameFolder() const { return mPutRepliesInSameFolder; }
  void setPutRepliesInSameFolder( bool b ) { mPutRepliesInSameFolder = b; }

  /**
   * Returns true if the user doesn't want to get notified about new mail
   * in this folder.
   */
  bool ignoreNewMail() const { return mIgnoreNewMail; }
  void setIgnoreNewMail( bool b ) { mIgnoreNewMail = b; }

  const KShortcut &shortcut() const { return mShortcut; }
  void setShortcut( const KShortcut& );

signals:
  /** Emitted when the status, name, or associated accounts of this
    folder changed. */
  void changed();

  /** Emitted when the contents of a folder have been cleared
     (new search in a search folder, for example) */
  void cleared();

  /** Emitted after an expunge. If not quiet, changed() will be
      emmitted first. */
  void expunged( KMFolder* );

  /** Emitted when the icon paths are set. */
  void iconsChanged();

  /** Emitted when the name of the folder changes. */
  void nameChanged();

  /** Emitted when the shortcut associated with this folder changes. */
  void shortcutChanged( KMFolder * );

  /** Emitted before a message is removed from the folder. */
  void msgRemoved(KMFolder*, Q_UINT32 sernum);

  /** Emitted after a message is removed from the folder. */
  void msgRemoved(int idx,QString msgIdMD5, QString strippedSubjMD5);
  void msgRemoved(KMFolder*);

  /** Emitted when a message is added from the folder. */
  void msgAdded(int idx);
  void msgAdded(KMFolder*, Q_UINT32 sernum);

  /** Emitted, when the status of a message is changed */
  void msgChanged(KMFolder*, Q_UINT32 sernum, int delta);

  /** Emitted when a field of the header of a specific message changed. */
  void msgHeaderChanged(KMFolder*, int);

  /** Emmited to display a message somewhere in a status line. */
  void statusMsg(const QString&);

  /** Emitted when number of unread messages has changed. */
  void numUnreadMsgsChanged( KMFolder* );

  /** Emitted when a folder was removed */
  void removed(KMFolder*, bool);

public slots:
  /** Incrementally update the index if possible else call writeIndex */
  int updateIndex();

  /** Add the message to the folder after it has been retrieved from an IMAP
      server */
  void reallyAddMsg(KMMessage* aMsg);

  /** Add a copy of the message to the folder after it has been retrieved
      from an IMAP server */
  void reallyAddCopyOfMsg(KMMessage* aMsg);

private:
  FolderStorage* mStorage;
  KMFolderDir* mChild;
  bool mIsSystemFolder;

  /** nationalized label or QString::null (then name() should be used) */
  QString mLabel;
  QString mSystemLabel;

  /** Support for automatic expiry of old messages */
  bool         mExpireMessages;          // TRUE if old messages are expired
  int          mUnreadExpireAge;         // Given in unreadExpireUnits
  int          mReadExpireAge;           // Given in readExpireUnits
  ExpireUnits  mUnreadExpireUnits;
  ExpireUnits  mReadExpireUnits;
  ExpireAction mExpireAction;
  QString      mExpireToFolderId;

  /** Icon related variables */
  bool mUseCustomIcons;
  QString mNormalIconPath;
  QString mUnreadIconPath;

  /** Mailing list attributes */
  bool                mMailingListEnabled;
  MailingList         mMailingList;

  uint mIdentity;

  /** name of the field that is used for "From" in listbox */
  QString mWhoField, mUserWhoField;

  /** Should replies to messages in this folder be put in here? */
  bool mPutRepliesInSameFolder;

  /** Should new mail in this folder be ignored? */
  bool mIgnoreNewMail;

  /** shortcut associated with this folder or null, if none is configured. */
  KShortcut mShortcut;
};

#endif /*kmfolder_h*/
