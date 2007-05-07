/*
 * kmfolderimap.h
 *
 * Copyright (c) 2001 Kurt Granroth <granroth@kde.org>
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 *
 * This file is based on kmacctimap.h by Michael Haeckel which is
 * based on popaccount.h by Don Sanders
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef kmfolderimap_h
#define kmfolderimap_h

#include "kmacctimap.h"
#include "kmfoldermbox.h"
#include "kmmsgbase.h"

#include "kio/job.h"
#include "kio/global.h"

#include <kstandarddirs.h>

#include <qintdict.h>
#include <qdict.h>
template< typename T> class QPtrList;
template< typename T> class QValueList;

class KMFolderTreeItem;
class KMFolderImap;
class KMSearchPattern;
class KMMessage;
namespace KMail {
  class FolderJob;
  class ImapJob;
  class AttachmentStrategy;
  class ImapAccountBase;
}
namespace KPIM {
  class ProgressItem;
}
using KMail::FolderJob;
using KMail::ImapJob;
using KMail::AttachmentStrategy;
using KMail::ImapAccountBase;
using KPIM::ProgressItem;

class KMMsgMetaData
{
public:
  KMMsgMetaData(KMMsgStatus aStatus)
    :mStatus(aStatus), mSerNum(0) {}
  KMMsgMetaData(KMMsgStatus aStatus, Q_UINT32 aSerNum)
    :mStatus(aStatus), mSerNum(aSerNum) {}
  ~KMMsgMetaData() {};
  const KMMsgStatus status() const { return mStatus; }
  const Q_UINT32 serNum() const { return mSerNum; }
private:
  KMMsgStatus mStatus;
  Q_UINT32 mSerNum;
};



class KMFolderImap : public KMFolderMbox
{
  Q_OBJECT
  friend class ::KMail::ImapJob;
public:

  static QString cacheLocation() {
     return locateLocal("data", "kmail/imap" );
  }

  enum imapState {
    imapNoInformation = 0,
    imapListingInProgress = 1,
    imapDownloadInProgress = 2,
    imapFinished = 3
  };

  virtual imapState getContentState() { return mContentState; }
  virtual void setContentState(imapState state) { mContentState = state; }

  virtual imapState getSubfolderState() { return mSubfolderState; }
  virtual void setSubfolderState(imapState state);

  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderImap(KMFolder* folder, const char* name=0);
  virtual ~KMFolderImap();

  /** Returns the type of this folder */
  virtual KMFolderType folderType() const { return KMFolderTypeImap; }

  virtual KMMessage* getMsg(int idx);
  /** The path to the imap folder on the server */
  void setImapPath( const QString &path );
  QString imapPath() const { return mImapPath; }

  /** The highest UID in the folder */
  ulong lastUid();

  /** The uidvalidity of the last update */
  void setUidValidity(const QString &validity) { mUidValidity = validity; }
  QString uidValidity() { return mUidValidity; }

  /** The imap account associated with this folder */
  void setAccount(KMAcctImap *acct);
  KMAcctImap* account() const;

  /** Remove (first occurrence of) given message from the folder. */
  virtual void removeMsg(int i, bool quiet = FALSE);
  virtual void removeMsg(const QPtrList<KMMessage>& msgList, bool quiet = FALSE);

  virtual int rename( const QString& newName, KMFolderDir *aParent = 0 );

  /** Remove the IMAP folder on the server and if successful also locally */
  virtual void remove();

  /** Automatically expunge deleted messages when leaving the folder */
  bool autoExpunge();

  /** Write the config file */
  virtual void writeConfig();

  /** Read the config file */
  virtual void readConfig();

  /**
   * List a directory and add the contents to kmfoldermgr
   * It uses a ListJob to get the folders
   * returns false if the connection failed
   */
  virtual bool listDirectory();

  /**
   * Retrieve all mails in a folder
   */
  void getFolder(bool force = FALSE);

  /**
   * same as above but also checks for new mails
   */
  void getAndCheckFolder(bool force = FALSE);

  /**
   * Get the whole message
   */
  void getMessage(KMFolder * folder, KMMessage * msg);

  /**
   * Create a new subfolder
   * You may specify the root imap path or this folder will be used
   * If you set askUser to false and the server can only handle folders
   * that contain messages _or_ folders the new folder is set to "contains messages"
   * by default
   */
  void createFolder(const QString &name,
      const QString& imapPath = QString::null, bool askUser = true);

  /**
   * Delete a message
   */
  void deleteMessage(KMMessage * msg);
  void deleteMessage(const QPtrList<KMMessage>& msgList);

  /**
   * Change the status of the message indicated by @p index
   * Overloaded function for the following one
   */
  virtual void setStatus(int idx, KMMsgStatus status, bool toggle);

  /**
   * Change the status of several messages indicated by @p ids
   */
  virtual void setStatus(QValueList<int>& ids, KMMsgStatus status, bool toggle);

  /** generates sets of uids */
  static QStringList makeSets( QValueList<ulong>&, bool sort = true);
  static QStringList makeSets(const QStringList&, bool sort = true);

  /** splits the message list according to sets. Modifies the @msgList. */
  static QPtrList<KMMessage> splitMessageList(const QString& set,
                                              QPtrList<KMMessage>& msgList);

  /** gets the uids of the given ids */
  void getUids(QValueList<int>& ids, QValueList<ulong>& uids);

  /** same as above but accepts a Message-List */
  void getUids(const QPtrList<KMMessage>& msgList, QValueList<ulong>& uids);

  /**
   * Expunge deleted messages from the folder
   */
  void expungeFolder(KMFolderImap * aFolder, bool quiet);

  virtual int compact( bool ) { expungeFolder(this, false); return 0; };

  /**
   * Emit the folderComplete signal
   */
  void sendFolderComplete(bool success)
  { emit folderComplete(this, success); }

  /**
   * Refresh the number of unseen mails
   * Returns false in an error condition
   */
  bool processNewMail(bool interactive);

  /**
   * Tell the folder, this it is selected and shall also display new mails,
   * not only their number, when checking for mail.
   */
  void setSelected(bool selected) { mIsSelected = selected; }
  bool isSelected() { return mIsSelected; }

  /**
   * Encode the given string in a filename save 7 bit string
   */
  static QString encodeFileName(const QString &);
  static QString decodeFileName(const QString &);
  static QTextCodec * utf7Codec();

  /**
   * Convert message status to a list of IMAP flags
   */
  static QString statusToFlags(KMMsgStatus status);

  /**
   * Return the filename of the folder (reimplemented from KFolder)
   */
  virtual QString fileName() const {
    return encodeFileName( KMFolderMbox::fileName() ); }

  /**
   * Get the serial number for the given UID (if available)
   */
   const ulong serNumForUID( ulong uid );

  /**
   * Save the metadata for the UID
   * If the UID is not supplied the one from the message is taken
   */
  void saveMsgMetaData( KMMessage* msg, ulong uid = 0 );

  /**
   * Splits a uid-set into single uids
   */
  static QValueList<ulong> splitSets(const QString);

  virtual void ignoreJobsForMessage( KMMessage* );

  /**
   * If this folder should be included in new-mail-check
   */
  bool includeInMailCheck() { return mCheckMail; }
  void setIncludeInMailCheck( bool check );

  /** Inherited */
  virtual int create();

  /** imap folders cannot expire */
  virtual bool isAutoExpire() const { return false; }

  /** Closes and cancels all pending jobs. */
  virtual void reallyDoClose();

  void setCheckingValidity( bool val ) { mCheckingValidity = val; }

  /** Return the trash folder. */
  KMFolder* trashFolder() const;

  /**
   * Mark the folder as already removed from the server
   * If set to true the folder will only be deleted locally
   * This will recursively be applied to all children
   */
  void setAlreadyRemoved(bool removed);

  /// Is the folder readonly?
  bool isReadOnly() const { return KMFolderMbox::isReadOnly() || mReadOnly; }

  /**
   * The user's rights on this folder - see bitfield in ACLJobs namespace.
   * @return 0 when not known yet
   */
  unsigned int userRights() const { return mUserRights; }

  /** Set the user's rights on this folder - called by getUserRights */
  void setUserRights( unsigned int userRights );

  /**
    * Search for messages
    * The actual search is done in slotSearch and the end
    * is signaled with searchDone()
    */
  virtual void search( const KMSearchPattern* );
  virtual void search( const KMSearchPattern*, Q_UINT32 serNum );

  /** Returns true if this folder can be moved */
  virtual bool isMoveable() const;

  /** Initialize this storage from another one. Used when creating a child folder */
  void initializeFrom( KMFolderImap* parent, QString path, QString mimeType );

signals:
  void folderComplete(KMFolderImap *folder, bool success);

  /**
   * Emitted, when the account is deleted
   */
  void deleted(KMFolderImap*);

  /**
   * Emitted at the end of the directory listing
   */
  void directoryListingFinished(KMFolderImap*);

  /**
   * Emitted when a folder creation has finished.
   * @param name The name of the folder that should have been created.
   * @param success True if the folder was created, false otherwise.
   */
  void folderCreationResult( const QString &name, bool success );

public slots:
  /** Add a message to a folder after is has been added on an IMAP server */
  virtual void addMsgQuiet(KMMessage *);
  virtual void addMsgQuiet(QPtrList<KMMessage>);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index_return = 0);
  virtual int addMsg(QPtrList<KMMessage>&, QValueList<int>& index_return);

  /** Copy the messages to this folder */
  void copyMsg(QPtrList<KMMessage>& msgList/*, KMFolder* parent*/);


  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  virtual KMMessage* take(int idx);
  virtual void take(QPtrList<KMMessage>);

  /**
   * Add the data a KIO::Job retrieves to the buffer
   */
  void slotSimpleData(KIO::Job * job, const QByteArray & data);

  /**
   * Convert IMAP flags to a message status
   * @param newMsg specifies whether unseen messages are new or unread
   */
  static void flagsToStatus(KMMsgBase *msg, int flags, bool newMsg = TRUE);

  /**
   * Connected to the result signal of the copy/move job
   */
  void slotCopyMsgResult( KMail::FolderJob* job );

  /**
   * Called from the SearchJob when the folder is done or messages where found
   */
  void slotSearchDone( QValueList<Q_UINT32> serNums,
                       const KMSearchPattern* pattern,
                       bool complete );

  /**
   * Called from the SearchJob when the message was searched
   */
  void slotSearchDone( Q_UINT32 serNum, const KMSearchPattern* pattern, bool matches );

  /**
   * Connected to ListJob::receivedFolders
   * creates/removes folders
   */
  void slotListResult( const QStringList&, const QStringList&,
      const QStringList&, const QStringList&, const ImapAccountBase::jobData& );

  /**
   * Connected to slotListNamespaces
   * creates/removes namespace folders
   */
  void slotCheckNamespace( const QStringList&, const QStringList&,
      const QStringList&, const QStringList&, const ImapAccountBase::jobData& );

protected:
  virtual FolderJob* doCreateJob( KMMessage *msg, FolderJob::JobType jt,
                                  KMFolder *folder, QString partSpecifier,
                                  const AttachmentStrategy *as ) const;
  virtual FolderJob* doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                  FolderJob::JobType jt, KMFolder *folder ) const;

  void getMessagesResult(KIO::Job * job, bool lastSet);

  /** Called by KMFolder::expunge() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int expungeContents();

  void setChildrenState( QString attributes );

  /** Create or find the INBOX and initialize it */
  void initInbox();

  /** See if there is a better parent then this folder */
  KMFolderImap* findParent( const QString& path, const QString& name );

  /** See if all folders are still present on server, otherwise delete them */
  void checkFolders( const QStringList& folderNames, const QString& ns );

  void finishMailCheck( imapState state );

protected slots:

  /**
   * Retrieve the whole folder or only the changes
   */
  void checkValidity();
  void slotCheckValidityResult(KIO::Job * job);

  /**
   * Get the folder now (internal)
   */
  void reallyGetFolder(const QString &startUid = QString::null);

  /**
   * For listing the contents of a folder
   */
  void slotListFolderResult(KIO::Job * job);
  void slotListFolderEntries(KIO::Job * job, const KIO::UDSEntryList & uds);

  /**
   * For retrieving a message digest
   */
  void slotGetMessagesResult(KIO::Job * job);
  void slotGetLastMessagesResult(KIO::Job * job);
  void slotGetMessagesData(KIO::Job * job, const QByteArray & data);

  /**
   * For creating a new subfolder
   */
  void slotCreateFolderResult(KIO::Job * job);

  /**
   * Remove the folder also locally, if removing on the server succeeded
   */
  void slotRemoveFolderResult(KIO::Job *job);

  /**
   * Update the number of unseen messages
   */
  void slotStatResult(KIO::Job *job);

  /**
   * notify the progress item that the mail check for this folder is
   * done.
   */
  void slotCompleteMailCheckProgress();

  /**
   * Is called when the slave is connected and triggers a newmail check
   */
  void slotProcessNewMail( int errorCode, const QString& errorMsg );

  /**
   * Is connected when there are folders to be created on startup and the
   * account is still connecting. Once the account emits the connected
   * signal this slot is called and the folders created.
   */
  void slotCreatePendingFolders( int errorCode, const QString& errorMsg );

  /**
   * Starts a namespace listing
   */
  void slotListNamespaces();

protected:
  QString     mImapPath;
  ulong       mLastUid;
  imapState   mContentState, mSubfolderState;
  bool        mIsSelected;
  bool        mCheckFlags;
  bool        mReadOnly;
  bool        mCheckMail;
  mutable QGuardedPtr<KMAcctImap> mAccount;
  // the current uidvalidity
  QString mUidValidity;
  unsigned int mUserRights;

private:
  // if we're checking validity currently
  bool        mCheckingValidity;
  // uid - metadata cache
  QIntDict<KMMsgMetaData> mUidMetaDataMap;
  // msgidMD5 - status map
  QDict<KMMsgMetaData> mMetaDataMap;
  // if the folder should be deleted without server roundtrip
  bool        mAlreadyRemoved;
  // the progress for mailchecks
  QGuardedPtr<ProgressItem> mMailCheckProgressItem;
  // the progress for listings
  ProgressItem *mListDirProgressItem;
  // the progress for addMsg
  ProgressItem *mAddMessageProgressItem;
  // to-be-added folders
  QStringList mFoldersPendingCreation;
};

#endif // kmfolderimap_h
