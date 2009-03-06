/*
 *  kmfoldercachedimap.cpp
 *
 *  Copyright (c) 2002-2004 Bo Thorsen <bo@sonofthor.dk>
 *  Copyright (c) 2002-2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
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
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#ifndef kmfoldercachedimap_h
#define kmfoldercachedimap_h

#include <kdialogbase.h>
#include <kstandarddirs.h>
#include <qvaluelist.h>
#include <qvaluevector.h>
#include <qptrlist.h>
#include <qdialog.h>

#include "kmfoldermaildir.h"
#include "kmfolderimap.h"
#include "kmacctcachedimap.h"
#include "kmfoldertype.h"
#include "folderjob.h"
#include "cachedimapjob.h"
#include "quotajobs.h"

using KMail::FolderJob;
using KMail::QuotaInfo;
class KMCommand;

class QComboBox;
class QRadioButton;

namespace KMail {
  class AttachmentStrategy;
  class ImapAccountBase;
  struct ACLListEntry;
}
using KMail::AttachmentStrategy;

class DImapTroubleShootDialog : public KDialogBase
{
  Q_OBJECT
public:
  enum SelectedOperation {
    None = -1,
    ReindexCurrent = 0,
    ReindexRecursive = 1,
    ReindexAll = 2,
    RefreshCache
  };

  DImapTroubleShootDialog( QWidget* parent=0, const char* name=0 );

  static int run();

private slots:
  void slotDone();

private:
  QRadioButton *mIndexButton, *mCacheButton;
  QComboBox *mIndexScope;
  int rc;
};

class KMFolderCachedImap : public KMFolderMaildir
{
  Q_OBJECT

public:
  static QString cacheLocation() {
     return locateLocal("data", "kmail/dimap" );
  }

  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderCachedImap(KMFolder* folder, const char* name=0);
  virtual ~KMFolderCachedImap();

  /**  @reimpl */
  void reallyDoClose(const char* owner);

  /** Initialize this storage from another one. Used when creating a child folder */
  void initializeFrom( KMFolderCachedImap* parent );

  virtual void readConfig();
  virtual void writeConfig();

  void writeConfigKeysWhichShouldNotGetOverwrittenByReadConfig();

  /** Returns the type of this folder */
  virtual KMFolderType folderType() const { return KMFolderTypeCachedImap; }

  /** @reimpl */
  virtual int create();

  /** Remove this folder */
  virtual void remove();

  /** Synchronize this folder and it's subfolders with the server */
  virtual void serverSync( bool recurse );

  /**  Force the sync state to be done. */
  void resetSyncState( );

  /** Block this folder from generating alarms, even if the annotations
   * on it say otherwise. Used to override alarms for read-only folders.
   * (Only useful for resource folders) */
  void setAlarmsBlocked( bool blocked );
  /** Should alarms for this folder be blocked?  (Only useful for resource folders) */
  bool alarmsBlocked() const;

  void checkUidValidity();

  enum imapState { imapNoInformation=0, imapInProgress=1, imapFinished=2 };

  virtual imapState getContentState() { return mContentState; }
  virtual void setContentState(imapState state) { mContentState = state; }

  virtual imapState getSubfolderState() { return mSubfolderState; }
  virtual void setSubfolderState(imapState state);

  /** The path to the imap folder on the server */
  void setImapPath(const QString &path);
  QString imapPath() const { return mImapPath; }

  /** The highest UID in the folder */
  void setLastUid( ulong uid );
  ulong lastUid();

  /** Find message by UID. Returns NULL if it doesn't exist */
  KMMsgBase* findByUID( ulong uid );

  /** The uidvalidity of the last update */
  void setUidValidity(const QString &validity) { mUidValidity = validity; }
  QString uidValidity() const { return mUidValidity; }

  /** Forget which mails are considered locally present. Needed when uidvalidity
   * changes. */
  void clearUidMap() { uidMap.clear(); }

  /** The imap account associated with this folder */
  void setAccount(KMAcctCachedImap *acct);
  KMAcctCachedImap* account() const;

  /** Returns the filename of the uidcache file */
  QString uidCacheLocation() const;

  /** Read the uidValitidy and lastUid values from disk */
  int readUidCache();

  /** Write the uidValitidy and lastUid values to disk */
  int writeUidCache();

  /** Current progress status (between 0 and 100) */
  int progress() const { return mProgress; }

  /* Reimplemented from KMFolder. Moving is not supported, so aParent must be 0 */
  virtual int rename(const QString& aName, KMFolderDir *aParent=0);

  /* Reimplemented from KMFolderMaildir */
  virtual KMMessage* take(int idx);
  /* Reimplemented from KMFolderMaildir */
  virtual int addMsg(KMMessage* msg, int* index_return = 0);
  /* internal version that doesn't remove the X-UID header */
  virtual int addMsgInternal(KMMessage* msg, bool, int* index_return = 0);
  virtual int addMsgKeepUID(KMMessage* msg, int* index_return = 0) {
    return addMsgInternal(msg, false, index_return);
  }

  /* Reimplemented from KMFolderMaildir */
  virtual void removeMsg(int i, bool imapQuiet = false);
  virtual void removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet = false)
    { FolderStorage::removeMsg(msgList, imapQuiet); }

  /// Is the folder readonly?
  bool isReadOnly() const { return KMFolderMaildir::isReadOnly() || mReadOnly; }
  bool canDeleteMessages() const;


  /**
   * Emit the folderComplete signal
   */
  void sendFolderComplete(bool success)
  { emit folderComplete(this, success); }

  /**
   * The silentUpload can be set to remove the folder upload error dialog
   */
  void setSilentUpload( bool silent ) { mSilentUpload = silent; }
  bool silentUpload() { return mSilentUpload; }

  virtual int createIndexFromContents() {
    const int result = KMFolderMaildir::createIndexFromContents();
    reloadUidMap();
    return result;
  }

  int createIndexFromContentsRecursive();

  //virtual void holdSyncs( bool hold ) { mHoldSyncs = hold; }

  /**
   * List a directory and add the contents to kmfoldermgr
   * It uses a ListJob to get the folders
   * returns false if the connection failed
   */
  virtual bool listDirectory();

  virtual void listNamespaces();

  /** Return the trash folder. */
  KMFolder* trashFolder() const;

  /**
   * The user's rights on this folder - see bitfield in ACLJobs namespace.
   * @return 0 when not known yet, -1 if there was an error fetching them
   */
  int userRights() const { return mUserRights; }

  /// Set the user's rights on this folder - called by getUserRights
  void setUserRights( unsigned int userRights );

  /**
   * The quota information for this folder.
   * @return an invalid info if we haven't synced yet, or the server
   * doesn't support quota. The difference can be figured out by
   * asking the account whether it supports quota. If we have
   * synced, the account supports quota, but there is no quota
   * on the folder, the return info will be valid, but empty.
   * @see QuotaInfo::isEmpty(), QuotaInfo::isValid()
   */
  const QuotaInfo quotaInfo() const { return mQuotaInfo; }
  void setQuotaInfo( const QuotaInfo & );

  /// Return the list of ACL for this folder
  typedef QValueVector<KMail::ACLListEntry> ACLList;
  const ACLList& aclList() const { return mACLList; }

  /// Set the list of ACL for this folder (for FolderDiaACLTab)
  void setACLList( const ACLList& arr );

  // Reimplemented so the mStatusChangedLocally bool can be set
  virtual void setStatus( int id, KMMsgStatus status, bool toggle );
  virtual void setStatus( QValueList<int>& ids, KMMsgStatus status, bool toggle );

  QString annotationFolderType() const { return mAnnotationFolderType; }

  // For kmailicalifaceimpl only
  void updateAnnotationFolderType();

  /// Free-busy and alarms relevance of this folder, i.e. for whom should
  /// events in this calendar lead to "busy" periods in their freebusy lists,
  /// and who should get alarms for the incidences in this folder.
  /// Applies to Calendar and Task folders only.
  ///
  /// IncForNobody: not relevant for free-busy and alarms to anybody
  /// IncForAdmins: apply to persons with admin permissions on this calendar
  /// IncForReaders: apply to all readers of this calendar
  enum IncidencesFor { IncForNobody, IncForAdmins, IncForReaders };

  IncidencesFor incidencesFor() const { return mIncidencesFor; }
  /// For the folder properties dialog
  void setIncidencesFor( IncidencesFor incfor );

  /** Returns wether the seen flag is shared among all users or every users has her own seen flags (default). */
  bool sharedSeenFlags() const { return mSharedSeenFlags; }
  /** Enable shared seen flags (requires server support). */
  void setSharedSeenFlags( bool b );

  /** Returns true if this folder can be moved */
  virtual bool isMoveable() const;

  /**
   * List of namespaces that need to be queried
   * Is set by the account for the root folder when the listing starts
   */
  QStringList namespacesToList() { return mNamespacesToList; }
  void setNamespacesToList( QStringList list ) { mNamespacesToList = list; }

  /**
   * Specify an imap path that is used to create the folder on the server
   * Otherwise the parent folder is used to construct the path
   */
  const QString& imapPathForCreation() { return mImapPathCreation; }
  void setImapPathForCreation( const QString& path ) { mImapPathCreation = path; }

  /**  \reimp */
  bool isCloseToQuota() const;

  /** Flags that can be permanently stored on the server. */
  int permanentFlags() const { return mPermanentFlags; }


  QString folderAttributes() const { return mFolderAttributes; }

protected slots:
  void slotGetMessagesData(KIO::Job * job, const QByteArray & data);
  void getMessagesResult(KMail::FolderJob *, bool lastSet);
  void slotGetLastMessagesResult(KMail::FolderJob *);
  void slotProgress(unsigned long done, unsigned long total);
  void slotPutProgress( unsigned long, unsigned long );

  //virtual void slotCheckValidityResult(KIO::Job * job);
  void slotSubFolderComplete(KMFolderCachedImap*, bool);

  // Connected to the imap account
  void slotConnectionResult( int errorCode, const QString& errorMsg );

  void slotCheckUidValidityResult( KMail::FolderJob* job );
  void slotPermanentFlags( int flags );
  void slotTestAnnotationResult(KIO::Job *job);
  void slotGetAnnotationResult( KIO::Job* );
  void slotMultiUrlGetAnnotationResult( KIO::Job* );
  void slotSetAnnotationResult(KIO::Job *job);
  void slotReceivedUserRights( KMFolder* );
  void slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& );

  void slotMultiSetACLResult(KIO::Job *);
  void slotACLChanged( const QString&, int );
  void slotAnnotationResult(const QString& entry, const QString& value, bool found);
  void slotAnnotationChanged( const QString& entry, const QString& attribute, const QString& value );
  void slotDeleteMessagesResult(KMail::FolderJob *);
  void slotImapStatusChanged(KMFolder* folder, const QString&, bool);
  void slotStorageQuotaResult( const QuotaInfo& );
  void slotQuotaResult( KIO::Job* job );

protected:
  /* returns true if there were messages to delete
     on the server */
  bool deleteMessages();
  void listMessages();
  void uploadNewMessages();
  void uploadFlags();
  void uploadSeenFlags();
  void createNewFolders();

  void listDirectory2();
  void createFoldersNewOnServerAndFinishListing( const QValueVector<int> foldersNewOnServer );


  /** Utility methods for syncing. Finds new messages
      in the local cache that must be uploaded */
  virtual QValueList<unsigned long> findNewMessages();
  /** Utility methods for syncing. Finds new subfolders
      in the local cache that must be created in the server */
  virtual QValueList<KMFolderCachedImap*> findNewFolders();

  /** This returns false if we have subfolders. Otherwise it returns ::canRemoveFolder() */
  virtual bool canRemoveFolder() const;

    /** Reimplemented from KMFolder */
  virtual FolderJob* doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder,
                                  QString partSpecifier, const AttachmentStrategy *as ) const;
  virtual FolderJob* doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                  FolderJob::JobType jt, KMFolder *folder ) const;

  virtual void timerEvent( QTimerEvent* );

  /* update progress status */
  void newState( int progress, const QString& syncStatus );

  /** See if there is a better parent then this folder */
  KMFolderCachedImap* findParent( const QString& path, const QString& name );



public slots:
  /**
   * Add the data a KIO::Job retrieves to the buffer
   */
  void slotSimpleData(KIO::Job * job, const QByteArray & data);

  /**
   * Troubleshoot the IMAP cache
   */
  void slotTroubleshoot();

  /**
   * Connected to ListJob::receivedFolders
   * creates/removes folders
   */
  void slotListResult( const QStringList&, const QStringList&,
      const QStringList&, const QStringList&, const ImapAccountBase::jobData& );

  /**
   * Connected to ListJob::receivedFolders
   * creates namespace folders
   */
  void slotCheckNamespace( const QStringList&, const QStringList&,
      const QStringList&, const QStringList&, const ImapAccountBase::jobData& );

private slots:
  void serverSyncInternal();
  void slotIncreaseProgress();
  void slotUpdateLastUid();
  void slotFolderDeletionOnServerFinished();
  void slotRescueDone( KMCommand* command );

signals:
  void folderComplete(KMFolderCachedImap *folder, bool success);
  void listComplete( KMFolderCachedImap* );

  /** emitted when we enter the state "state" and
     have to process "number" items (for example messages
  */
  void syncState( int state, int number );

private:
  void setReadOnly( bool readOnly );
  QString state2String( int state ) const;
  void rememberDeletion( int );
  /** Rescue not yet synced messages to a lost+found folder in case
    syncing is not possible because the folder has been deleted on the
    server or write access to this folder has been revoked.
  */
  KMCommand* rescueUnsyncedMessages();
  /** Recursive helper function calling the above method. */
  void rescueUnsyncedMessagesAndDeleteFolder( KMFolder *folder, bool root = true );

  /** State variable for the synchronization mechanism */
  enum {
    SYNC_STATE_INITIAL,
    SYNC_STATE_TEST_ANNOTATIONS,
    SYNC_STATE_PUT_MESSAGES,
    SYNC_STATE_UPLOAD_FLAGS,
    SYNC_STATE_CREATE_SUBFOLDERS,
    SYNC_STATE_LIST_NAMESPACES,
    SYNC_STATE_LIST_SUBFOLDERS,
    SYNC_STATE_LIST_SUBFOLDERS2,
    SYNC_STATE_DELETE_SUBFOLDERS,
    SYNC_STATE_LIST_MESSAGES,
    SYNC_STATE_DELETE_MESSAGES,
    SYNC_STATE_EXPUNGE_MESSAGES,
    SYNC_STATE_GET_MESSAGES,
    SYNC_STATE_HANDLE_INBOX,
    SYNC_STATE_GET_USERRIGHTS,
    SYNC_STATE_GET_ANNOTATIONS,
    SYNC_STATE_SET_ANNOTATIONS,
    SYNC_STATE_GET_ACLS,
    SYNC_STATE_SET_ACLS,
    SYNC_STATE_GET_QUOTA,
    SYNC_STATE_FIND_SUBFOLDERS,
    SYNC_STATE_SYNC_SUBFOLDERS,
    SYNC_STATE_CHECK_UIDVALIDITY,
    SYNC_STATE_RENAME_FOLDER
  } mSyncState;

  int mProgress;
  int mStatusFlagsJobs;

  QString mUidValidity;
  QString     mImapPath;
  imapState   mContentState, mSubfolderState;
  QStringList mSubfolderNames, mSubfolderPaths,
              mSubfolderMimeTypes, mSubfolderAttributes;
  QString     mFolderAttributes;
  QString     mAnnotationFolderType;
  IncidencesFor mIncidencesFor;
  bool mSharedSeenFlags;

  bool        mHasInbox;
  bool        mIsSelected;
  bool        mCheckFlags;
  bool        mReadOnly;
  mutable QGuardedPtr<KMAcctCachedImap> mAccount;

  QIntDict<int> uidsOnServer;
  QValueList<ulong> uidsForDeletionOnServer;
  QValueList<KMail::CachedImapJob::MsgForDownload> mMsgsForDownload;
  QValueList<ulong> mUidsForDownload;
  QStringList       foldersForDeletionOnServer;

  QValueList<KMFolderCachedImap*> mSubfoldersForSync;
  KMFolderCachedImap* mCurrentSubfolder;

  /** Mapping uid -> index
      Keep updated in addMsg, take and removeMsg. This is used to lookup
      whether a mail is present locally or not.  */
  QMap<ulong,int> uidMap;
  bool uidMapDirty;
  void reloadUidMap();
  int uidWriteTimer;

  /** This is the last uid that we have seen from the server on the last
      sync. It is crucially important that this is correct at all times
      and not bumped up permaturely, as it is the watermark which is used
      to discern message which are not present locally, because they were
      deleted locally and now need to be deleted from the server,
      from those which are new and need to be downloaded. Sucessfull
      downloading of all pending mail from the server sets this. Between
      invocations it is stored on disk in the uidcache file. It must not
      change during a sync. */
  ulong mLastUid;
  /** The highest id encountered while syncing. Once the sync process has
      successfully downloaded all pending mail and deleted on the server
      all messages that were removed locally, this will become the new
      mLastUid. See above for details. */
  ulong mTentativeHighestUid;

  /** Used to determine whether listing messages yielded a sensible result.
   * Only then is the deletion o messages (which relies on succesful
   * listing) attempted, during the sync.  */
  bool mFoundAnIMAPDigest;

  int mUserRights, mOldUserRights;
  ACLList mACLList;

  bool mSilentUpload;
  bool mFolderRemoved;
  //bool mHoldSyncs;
  bool mRecurse;

  /// Set to true when the foldertype annotation needs to be set on the next sync
  bool mAnnotationFolderTypeChanged;
  /// Set to true when the "incidences-for" annotation needs to be set on the next sync
  bool mIncidencesForChanged;
  /// Set to true when the "sharedseen" annotation needs to be set on the next sync
  bool mSharedSeenFlagsChanged;

 /**
  * UIDs added by setStatus. Indicates that the client has changed
  * the status of those mails. The mail flags for changed mails will be
  * uploaded to the server, overwriting the server's notion of the status
  * of the mails in this folder.
  */
  QValueList<ulong> mUIDsOfLocallyChangedStatuses;

 /**
  * Same as above, but uploads the flags of all mails, even if not all changed.
  * Only still here for config compatibility.
  */
  bool mStatusChangedLocally;

  QStringList mNamespacesToList;
  int mNamespacesToCheck;
  bool mPersonalNamespacesCheckDone;
  QString mImapPathCreation;

  QuotaInfo mQuotaInfo;
  QMap<ulong,void*> mDeletedUIDsSinceLastSync;
  bool mAlarmsBlocked;

  QValueList<KMFolder*> mToBeDeletedAfterRescue;
  int mRescueCommandCount;

  int mPermanentFlags;
};

#endif /*kmfoldercachedimap_h*/
