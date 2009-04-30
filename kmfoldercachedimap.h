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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

#include "kmfoldermaildir.h"
#include "kmfolderimap.h"
#include "kmacctcachedimap.h"
#include "kmfoldertype.h"
#include "folderjob.h"
#include "cachedimapjob.h"
#include "quotajobs.h"

#include <kdialog.h>
#include <kstandarddirs.h>

#include <QDialog>
#include <QList>
#include <QTimerEvent>

using KMail::FolderJob;
using KMail::QuotaInfo;
class KComboBox;
class KMCommand;

class QRadioButton;

namespace KMail {
  class AttachmentStrategy;
  class ImapAccountBase;
  struct ACLListEntry;
}
using KMail::AttachmentStrategy;

class DImapTroubleShootDialog : public KDialog
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

    DImapTroubleShootDialog( QWidget *parent=0 );

    static int run();

  private slots:
    void slotDone();
    void slotChanged( int id );

  private:
    QRadioButton *mIndexButton, *mCacheButton;
    KComboBox *mIndexScope;
    QButtonGroup *mButtonGroup;
    int rc;
};

class KMFolderCachedImap : public KMFolderMaildir
{
  Q_OBJECT

  public:
    static QString cacheLocation() {
      return KStandardDirs::locateLocal("data", "kmail/dimap" );
    }

    /**
      Usually a parent is given. But in some cases there is no
      fitting parent object available. Then the name of the folder
      is used as the absolute path to the folder file.
    */
    explicit KMFolderCachedImap( KMFolder *folder, const char *name=0 );
    virtual ~KMFolderCachedImap();

    /**
      Initializes this storage from another. Used when creating a child folder.
    */
    void initializeFrom( KMFolderCachedImap *parent );

    /**  @reimpl */
    void reallyDoClose();

    virtual void readConfig();
    virtual void writeConfig();

    void writeConfigKeysWhichShouldNotGetOverwrittenByReadConfig();

    /**
      Returns the type of this folder.
    */
    virtual KMFolderType folderType() const { return KMFolderTypeCachedImap; }

    /** @reimpl */
    virtual int create();

    /**
      Removes this folder.
    */
    virtual void remove();

    /**
      Synchronizes this folder and it's subfolders with the server.
    */
    virtual void serverSync( bool recurse );

    /**
      Forces the sync state to be reset.
    */
    void resetSyncState();

    /** Block this folder from generating alarms, even if the annotations
     * on it say otherwise. Used to override alarms for read-only folders.
     * (Only useful for resource folders) */
    void setAlarmsBlocked( bool blocked );

    /** Should alarms for this folder be blocked?  (Only useful for resource folders) */
    bool alarmsBlocked() const;

    void checkUidValidity();

    enum imapState {
      imapNoInformation=0,
      imapInProgress=1,
      imapFinished=2
    };

    virtual imapState getContentState() { return mContentState; }
    virtual void setContentState( imapState state ) { mContentState = state; }

    virtual imapState getSubfolderState() { return mSubfolderState; }
    virtual void setSubfolderState( imapState state );

    /**
      Sets the path to the imap folder on the server.
    */
    void setImapPath( const QString &path );
    QString imapPath() const { return mImapPath; }

    /**
      Sets the highest UID in the folder.
    */
    void setLastUid( ulong uid );
    ulong lastUid();

    /**
      Finds the message by UID.
      @return NULL if the message does not exist.
    */
    KMMsgBase *findByUID( ulong uid );

    /**
      Sets the uidvalidity of the last update.
    */
    void setUidValidity( const QString &validity ) { mUidValidity = validity; }
    QString uidValidity() const { return mUidValidity; }

    /**
      Clears the map of which messages are considered present locally.
      Needed when uidvalidity changes.
    */
    void clearUidMap() { uidMap.clear(); }

    /**
      Sets the imap account associated with this folder.
    */
    void setAccount( KMAcctCachedImap *acct );

    /**
      Returns the account associated with this folder.
      If no account exists yet, one is created.
    */
    KMAcctCachedImap *account() const;

    /**
      Returns the filename of the uidcache file.
    */
    QString uidCacheLocation() const;

    /**
      Reads the uidValitidy and lastUid values from disk.
    */
    int readUidCache();

    /**
      Writes the uidValitidy and lastUid values to disk.
    */
    int writeUidCache();

    /**
      Returns the cCurrent progress status (between 0 and 100).
    */
    int progress() const { return mProgress; }

    /** Reimplemented from KMFolder. Moving is not supported, so aParent must be 0. */
    virtual int rename( const QString &aName, KMFolderDir *aParent = 0 );

    /** Reimplemented from KMFolderMaildir */
    virtual KMMessage *take( int idx );
    bool canDeleteMessages() const;

    /** Reimplemented from KMFolderMaildir */
    virtual int addMsg( KMMessage *msg, int *index_return = 0 );


    /**
      Adds a message without clearing it's X-UID field.
    */
    virtual int addMsgInternal( KMMessage *msg, bool, int *index_return = 0 );
    virtual int addMsgKeepUID( KMMessage *msg, int *index_return = 0 ) {
      return addMsgInternal( msg, false, index_return );
    }

    /** Reimplemented from KMFolderMaildir */
    virtual void removeMsg( int i, bool imapQuiet = false );

    /**
      Returns true if the folder is read-only; false otherwise.
    */
    bool isReadOnly() const {
      return KMFolderMaildir::isReadOnly() || mReadOnly;
    }

    /**
      Emits the folderComplete signal.
    */
    void sendFolderComplete( bool success ) {
      emit folderComplete( this, success );
    }

    /**
      Sets the silentUpload flag, which removes the folder upload error dialog.
    */
    void setSilentUpload( bool silent ) {
      mSilentUpload = silent;
    }
    bool silentUpload() { return mSilentUpload; }

    virtual int createIndexFromContents() {
      const int result = KMFolderMaildir::createIndexFromContents();
      reloadUidMap();
      return result;
    }

    int createIndexFromContentsRecursive();

    /**
      Lists a directory and add the contents to kmfoldermgr.
      It uses a ListJob to get the folders
      @return false if the connection failed.
    */
    virtual bool listDirectory();

    virtual void listNamespaces();

    /**
      Returns the trash folder.
    */
    KMFolder *trashFolder() const;

    /**
      The user's rights on this folder - see bitfield in ACLJobs namespace.
      @return 0 when not known yet, -1 if there was an error fetching them
    */
    int userRights() const { return mUserRights; }
  void setQuotaInfo( const QuotaInfo & );

    /**
      Sets the user's rights on this folder.
    */
    void setUserRights( unsigned int userRights );

    /**
      Returns the  quota information for this folder.
      @return an invalid info if we haven't synced yet, or the server
      doesn't support quota. The difference can be figured out by
      asking the account whether it supports quota. If we have
      synced, the account supports quota, but there is no quota
      on the folder, the return info will be valid, but empty.
      @see QuotaInfo::isEmpty(), QuotaInfo::isValid()
    */
    const QuotaInfo quotaInfo() const { return mQuotaInfo; }

    /**
      Returns the list of ACL for this folder.
    */
    typedef QVector<KMail::ACLListEntry> ACLList;
    const ACLList &aclList() const { return mACLList; }

    /**
      Sets the list of ACL for this folder (for FolderDialogACLTab)
    */
    void setACLList( const ACLList &arr );

    /** Reimplemented so the mStatusChangedLocally bool can be set */
    virtual void setStatus( int id, const MessageStatus &status, bool toggle );
    virtual void setStatus( QList<int> &ids, const MessageStatus &status, bool toggle );

    QString annotationFolderType() const { return mAnnotationFolderType; }

    /** For kmailicalifaceimpl only */
    void updateAnnotationFolderType();

    /**
      Free-busy and alarms relevance of this folder, i.e. for whom should
      events in this calendar lead to "busy" periods in their freebusy lists,
      and who should get alarms for the incidences in this folder.
      Applies to Calendar and Task folders only.
    */
    enum IncidencesFor {
      IncForNobody, /**< Not relevant for free-busy and alarms to anybody */
      IncForAdmins, /**< Persons with admin permissions on this calendar */
      IncForReaders /**< All readers of this calendar */
    };

    IncidencesFor incidencesFor() const { return mIncidencesFor; }
    void setIncidencesFor( IncidencesFor incfor );

    /**
      Returns true if this folder can be moved.
    */
    virtual bool isMoveable() const;

    /**
      Returns a list of namespaces that need to be queried
      Is set by the account for the root folder when the listing starts
    */
    QStringList namespacesToList() { return mNamespacesToList; }
    void setNamespacesToList( QStringList list ) { mNamespacesToList = list; }

    /**  \reimp */
    bool isCloseToQuota() const;

    /** Flags that can be permanently stored on the server. */
    int permanentFlags() const { return mPermanentFlags; }

    /**
      Specify an imap path that is used to create the folder on the server
      Otherwise the parent folder is used to construct the path.
    */
    const QString &imapPathForCreation() { return mImapPathCreation; }
    void setImapPathForCreation( const QString &path ) { mImapPathCreation = path; }

    QString folderAttributes() const { return mFolderAttributes; }

  protected slots:
    void slotGetMessagesData( KIO::Job *job, const QByteArray &data );
    void getMessagesResult( KMail::FolderJob *job, bool lastSet );
    void slotGetLastMessagesResult( KMail::FolderJob *job );
    void slotProgress( unsigned long done, unsigned long total );
    void slotPutProgress( unsigned long, unsigned long );

    void slotSubFolderComplete( KMFolderCachedImap*, bool );

    /**
      Connected to the imap account's connectionResult signal.
      Emitted when the slave connected or failed to connect.
    */
    void slotConnectionResult( int errorCode, const QString &errorMsg );

    void slotPermanentFlags( int flags );
    void slotCheckUidValidityResult( KMail::FolderJob *job );
    void slotTestAnnotationResult( KJob *job );
    void slotGetAnnotationResult( KJob *job );
    void slotMultiUrlGetAnnotationResult( KJob *job );
    void slotSetAnnotationResult( KJob *job );
    void slotReceivedUserRights( KMFolder *folder );
    void slotReceivedACL( KMFolder *folder, KIO::Job *job, const KMail::ACLList&arr );

    void slotMultiSetACLResult( KJob *job );
    void slotACLChanged( const QString&, int );
    void slotAnnotationResult( const QString &entry, const QString &value,
                               bool found );
    void slotAnnotationChanged( const QString &entry, const QString &attribute,
                                const QString &value );
    void slotDeleteMessagesResult( KMail::FolderJob *job );
    void slotImapStatusChanged( KMFolder *folder, const QString&, bool );
    void slotStorageQuotaResult( const QuotaInfo &info );
    void slotQuotaResult( KJob *job );

  protected:
    /**
      Returns true if there were messages to delete on the server.
    */
    bool deleteMessages();

    /**
      List the messages in a folder. No directory listing done.
    */
    void listMessages();

    void uploadNewMessages();
    void uploadFlags();
    void uploadSeenFlags();
    void createNewFolders();

    /**
      Synchronizes the local folders as needed (creation/deletion).
      No network communication here.
    */
    void listDirectory2();

    void createFoldersNewOnServerAndFinishListing( const QVector<int> foldersNewOnServer );

    /**
      Utility methods for syncing. Finds new messages
      in the local cache that must be uploaded.
    */
    virtual QList<unsigned long> findNewMessages();

    /**
      Utility methods for syncing. Finds new subfolders
      in the local cache that must be created in the server.
    */
    virtual QList<KMFolderCachedImap*> findNewFolders();

    /**
      Returns false if we have subfolders; else returns ::canRemoveFolder()
    */
    virtual bool canRemoveFolder() const;

    /** Reimplemented from KMFolder */
    virtual FolderJob *doCreateJob( KMMessage *msg, FolderJob::JobType jt,
                                    KMFolder *folder,
                                    const QString &partSpecifier,
                                    const AttachmentStrategy *as ) const;
    virtual FolderJob *doCreateJob( QList<KMMessage*> &msgList,
                                    const QString &sets, FolderJob::JobType jt,
                                    KMFolder *folder ) const;

    virtual void timerEvent( QTimerEvent *e );

    /**
      Updates the progress status.
    */
    void newState( int progress, const QString &syncStatus );

    /**
      Determines if there is a better parent then this folder.
    */
    KMFolderCachedImap *findParent( const QString &path, const QString &name );

public slots:
  /**
   * Add the data a KIO::Job retrieves to the buffer
   */
  void slotSimpleData(KIO::Job * job, const QByteArray & data);

    /**
      Troubleshoots the IMAP cache.
    */
    void slotTroubleshoot();

    /**
      Connected to ListJob::receivedFolders. creates/removes folders.
    */
    void slotListResult( const QStringList &folderNames,
                         const QStringList &folderPaths,
                         const QStringList &folderMimeTypes,
                         const QStringList &folderAttributes,
                         const ImapAccountBase::jobData &jobData );

    /**
      Connected to ListJob::receivedFolders. creates namespace folders.
    */
    void slotCheckNamespace( const QStringList &folderNames,
                             const QStringList &folderPaths,
                             const QStringList &folderMimeTypes,
                             const QStringList &folderAttributes,
                             const ImapAccountBase::jobData &jobData );

  private slots:
    void serverSyncInternal();
    void slotIncreaseProgress();
    void slotUpdateLastUid();
    void slotFolderDeletionOnServerFinished();
  void slotRescueDone( KMCommand* command );

  signals:
    void folderComplete( KMFolderCachedImap *folder, bool success );
    void listComplete( KMFolderCachedImap *folder );

    /**
      Emitted when we enter the state "state" and have to process @p "number
      items (for example messages)
    */
    void syncState( int state, int number );

  private:
    void setReadOnly( bool readOnly );
    QString state2String( int state ) const;
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
  void rememberDeletion( int );

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

    bool        mHasInbox;
    bool        mIsSelected;
    bool        mCheckFlags;
    bool        mReadOnly;
    mutable QPointer<KMAcctCachedImap> mAccount;

    QSet<ulong> uidsOnServer;

    QList<ulong> uidsForDeletionOnServer;
    QList<KMail::CachedImapJob::MsgForDownload> mMsgsForDownload;
    QList<ulong> mUidsForDownload;
    QStringList       foldersForDeletionOnServer;

    QList<KMFolderCachedImap*> mSubfoldersForSync;
    KMFolderCachedImap *mCurrentSubfolder;

    /**
      Mapping uid -> index
      Keep updated in addMsg, take and removeMsg. This is used to lookup
      whether a mail is present locally or not.
    */
    QMap<ulong,int> uidMap;
    bool uidMapDirty;
    void reloadUidMap();
    int uidWriteTimer;

    /**
      This is the last uid that we have seen from the server on the last
      sync. It is crucially important that this is correct at all times
      and not bumped up permaturely, as it is the watermark which is used
      to discern message which are not present locally, because they were
      deleted locally and now need to be deleted from the server,
      from those which are new and need to be downloaded. Successful
      downloading of all pending mail from the server sets this. Between
      invocations it is stored on disk in the uidcache file. It must not
      change during a sync.
    */
    ulong mLastUid;

    /**
      The highest id encountered while syncing. Once the sync process has
      successfully downloaded all pending mail and deleted on the server
      all messages that were removed locally, this will become the new
      mLastUid. See above for details.
    */
    ulong mTentativeHighestUid;

    /** Used to determine whether listing messages yielded a sensible result.
     * Only then is the deletion o messages (which relies on successful
     * listing) attempted, during the sync.  */
    bool mFoundAnIMAPDigest;

    int mUserRights, mOldUserRights;
    ACLList mACLList;

    bool mSilentUpload;
    bool mFolderRemoved;
    bool mRecurse;

   /**
    * UIDs added by setStatus. Indicates that the client has changed
    * the status of those mails. The mail flags for changed mails will be
    * uploaded to the server, overwriting the server's notion of the status
    * of the mails in this folder.
    */
    QList<ulong> mUIDsOfLocallyChangedStatuses;

    /**
     * Same as above, but uploads the flags of all mails, even if not all changed.
     * Only still here for config compatibility.
     */
    bool mStatusChangedLocally;

    /**
      Set to true when the foldertype annotation needs to be set
      on the next sync.
    */
    bool mAnnotationFolderTypeChanged;

    /**
      Set to true when the "incidences-for" annotation needs to be set
      on the next sync
    */
    bool mIncidencesForChanged;

    QStringList mNamespacesToList;
    int mNamespacesToCheck;
    bool mPersonalNamespacesCheckDone;
    QString mImapPathCreation;

    QuotaInfo mQuotaInfo;
    bool mAlarmsBlocked;

    QList<KMFolder*> mToBeDeletedAfterRescue;
    int mRescueCommandCount;

  int mPermanentFlags;
  QMap<ulong,void*> mDeletedUIDsSinceLastSync;
};

#endif /*kmfoldercachedimap_h*/
