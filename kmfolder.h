/* -*- mode: C++ -*-
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
#ifndef kmfolder_h
#define kmfolder_h

// for large file support
#include <config.h>

#include "kmfoldernode.h"
#include "kmfoldertype.h"
#include "kmmsginfo.h"
#include "kmglobal.h"
#include "kmkernel.h"
#include "folderjob.h"
using KMail::FolderJob;
#include "mailinglist-magic.h"
using KMail::MailingList;
#include "kmaccount.h" // for AccountList

#include "mimelib/string.h"

#include <qptrvector.h>
#include <sys/types.h>
#include <stdio.h>
#include <kshortcut.h>

class KMMessage;
class KMFolderDir;
class QTimer;
class FolderStorage;
class KMFolderTreeItem;
class KMFolderJob;

namespace KMail {
   class AttachmentStrategy;
}
using KMail::AttachmentStrategy;

typedef QValueList<Q_UINT32> SerNumList;

/** Mail folder.
 * (description will be here).
 *
 * @section Accounts
 *   The accounts (of KMail) that are fed into the folder are
 *   represented as the children of the folder. They are only stored here
 *   during runtime to have a reference for which accounts point to a
 *   specific folder.
 */

class KMFolder: public KMFolderNode
{
  Q_OBJECT
  friend class ::KMFolderJob;
public:

  /**
   * Constructs a new Folder object.
   * @param parent The directory in the folder storage hierarchy under which
   * the folder's storage will be found or created.
   * @param name If name of the folder. In case there is no parent directory, because
   * the folder is free-standing (/var/spool/mail/foo), this is used for the full path to
   * the folder's storage location.
   * @param aFolderType The type of folder to create.
   * @param withIndex Wether this folder has an index. No-index folders are
   * those used by KMail internally, the Outbox, and those of local spool accounts,
   * for example.
   * @param exportedSernums whether this folder exports its serial numbers to
   * the global MsgDict for lookup.
   * @return A new folder instance.
   */
  KMFolder( KMFolderDir* parent, const QString& name,
                KMFolderType aFolderType, bool withIndex = true,
                bool exportedSernums = true );
  ~KMFolder();

  /** Returns true if this folder is the inbox on the local disk */
  bool isMainInbox() {
    return this == KMKernel::self()->inboxFolder();
  }
  /** Returns true only if this is the outbox for outgoing mail */
  bool isOutbox() {
    return this == KMKernel::self()->outboxFolder();
  }
  /** Returns true if this folder is the sent-mail box of the local account,
    or is configured to be the sent mail box of any of the users identities */
  bool isSent() {
    return KMKernel::self()->folderIsSentMailFolder( this );
  }
  /** Returns true if this folder is configured as a trash folder, locally or
    for one of the accounts. */
  bool isTrash() {
    return KMKernel::self()->folderIsTrash( this );
  }
  /** Returns true if this folder is the drafts box of the local account,
    or is configured to be the drafts box of any of the users identities */
  bool isDrafts() {
    return KMKernel::self()->folderIsDrafts( this );
  }
  /** Returns true if this folder is the templates folder of the local account,
    or is configured to be the templates folder of any of the users identities */
  bool isTemplates() {
    return KMKernel::self()->folderIsTemplates( this );
  }

  void setAcctList( AccountList* list ) { mAcctList = list; }
  AccountList* acctList() { return mAcctList; }

  /** Returns TRUE if accounts are associated with this folder. */
  bool hasAccounts() const { return (mAcctList != 0); }

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
  void removeMsg(int i, bool imapQuiet = false);
  void removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet = false);

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
  int open(const char *owner);

  /** Check folder for permissions
    Returns zero if readable and writable. */
  int canAccess();

  /** Close folder. If force is true the files are closed even if
    others still use it (e.g. other mail reader windows). */
  void close(const char *owner, bool force=false);

  /** fsync buffers to disk */
  void sync();

  /** Test if folder is opened. */
  bool isOpened() const;

  /** Mark all new messages as unread. */
  void markNewAsUnread();

  /** Mark all new and unread messages as read. */
  void markUnreadAsRead();

  /** Removes the folder physically from disk and empties the contents
    of the folder in memory. Note that the folder is closed during this
    process, whether there are others using it or not.
    see KMFolder::removeContents */
  void remove();

  /** Delete entire folder. Forces a close *but* opens the
    folder again afterwards. Returns errno(3) error code or zero on
    success.  see KMFolder::expungeContents */
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

  /** Returns true if the table of contents is dirty. This happens when
    a message is deleted from the folder. The toc will then be re-created
    when the folder is closed. */
  bool dirty() const;

  /** Change the dirty flag. */
  void setDirty(bool f);

  /** Returns true if the folder contains deleted messages */
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

  /** Can messages in this folder be deleted? */
  bool canDeleteMessages() const;

  /** Returns true if the folder is a kmail system folder. These are
    the folders 'inbox', 'outbox', 'sent', 'trash', 'drafts', 'templates'.
    The name of these folders is nationalized in the folder display and
    they cannot have accounts associated. Deletion is also forbidden. Etc. */
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

  /** Returns true if this folder is associated with a mailing-list. */
  void setMailingListEnabled( bool enabled );
  bool isMailingListEnabled() const { return mMailingListEnabled; }

  void setMailingList( const MailingList& mlist );
  MailingList mailingList() const
  { return mMailingList; }
  QString mailingListPostAddress() const;

  void setIdentity(uint identity);
  uint identity() const;

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

  /** Returns true if this folder can be moved */
  bool isMoveable() const;

  /** Returns true if there is currently a move or copy operation going
      on with this folder as target.
  */
  bool moveInProgress() const { return mMoveInProgress; }

  /** Sets the move-in-progress flag. */
  void setMoveInProgress( bool b ) { mMoveInProgress = b; }

signals:
  /** Emitted when the status, name, or associated accounts of this
    folder changed. */
  void changed();

  /** Emitted when the folder is closed for real - ticket holders should
   * discard any messages */
  void closed();

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
  void msgRemoved( int idx, QString msgIdMD5 );
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

  /** Emitted when the variables for the config of the view have changed */
  void viewConfigChanged();

  /** Emitted when the folder's size changes. */
  void folderSizeChanged( KMFolder * );

  /** Emitted when the no content state changed. */
  void noContentChanged();

public slots:
  /** Incrementally update the index if possible else call writeIndex */
  int updateIndex();

  /** Add the message to the folder after it has been retrieved from an IMAP
      server */
  void reallyAddMsg(KMMessage* aMsg);

  /** Add a copy of the message to the folder after it has been retrieved
      from an IMAP server */
  void reallyAddCopyOfMsg(KMMessage* aMsg);

private slots:
  /** The type of contents of this folder changed. Do what is needed. */
  void slotContentsTypeChanged( KMail::FolderContentsType type );
  /** Triggered by the storage when its size changed. */
  void slotFolderSizeChanged();

private:
  FolderStorage* mStorage;
  KMFolderDir* mChild;
  bool mIsSystemFolder;
  bool mHasIndex :1;
  bool mExportsSernums :1;
  bool mMoveInProgress :1;

  /** nationalized label or QString::null (then name() should be used) */
  QString mLabel;
  QString mSystemLabel;

  /** Support for automatic expiry of old messages */
  bool         mExpireMessages;          // true if old messages are expired
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

  AccountList* mAcctList;

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


/**
   RAII for KMFolder::open() / close().

   Usage: const KMFolderOpener opener( folder );
*/
class KMFolderOpener {
  KMFolder* mFolder;
  const char* const mOwner;
  int mOpenRc;

public:
  KMFolderOpener( KMFolder* folder, const char* const owner )
   : mFolder( folder )
   , mOwner( owner )
  {
    assert( folder ); //change if that's not what you want
    mOpenRc = folder->open( owner );
  }

  ~KMFolderOpener()
  {
    if ( !mOpenRc )
      mFolder->close( mOwner );
  }

  KMFolder* folder() const { return mFolder; }

  int openResult() const { return mOpenRc; }

private:
  //we forbid construction on the heap as good as we can
  void* operator new( size_t size );
};


#endif /*kmfolder_h*/
