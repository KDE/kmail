/**
 *  kmfoldercachedimap.cpp
 *
 *  Copyright (c) 2002-2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include "kmacctimap.h"
#include "kmfoldertype.h"
#include "folderjob.h"
#include "cachedimapjob.h"

using KMail::FolderJob;
class KMAcctCachedImap;

namespace KMail {
  class AttachmentStrategy;
  struct ACLListEntry;
}
using KMail::AttachmentStrategy;

class DImapTroubleShootDialog : public KDialogBase
{
  Q_OBJECT
public:
  DImapTroubleShootDialog( QWidget* parent=0, const char* name=0 );

  static int run();

private slots:
  void slotRebuildIndex();
  void slotRebuildCache();

private:
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

  /** Initialize this storage from another one. Used when creating a child folder */
  void initializeFrom( KMFolderCachedImap* parent );

  virtual void readConfig();

  /** Returns the type of this folder */
  virtual KMFolderType folderType() const { return KMFolderTypeCachedImap; }

  /** Remove this folder */
  virtual void remove();

  /** Synchronize this folder and it's subfolders with the server */
  virtual void serverSync( bool recurse );

  /** Force the sync state to be done. */
  void resetSyncState();

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
  virtual void removeMsg(int i, bool imapQuiet = FALSE);
  virtual void removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet = FALSE)
    { FolderStorage::removeMsg(msgList, imapQuiet); }

  /// Is the folder readonly?
  bool isReadOnly() const { return KMFolderMaildir::isReadOnly() || mReadOnly; }

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

  virtual int createIndexFromContents()
    { return KMFolderMaildir::createIndexFromContents(); }

  // Mark for resync
  void resync() { mResync = true; }

  //virtual void holdSyncs( bool hold ) { mHoldSyncs = hold; }

  /**
   * List a directory and add the contents to kmfoldermgr
   * It uses a ListJob to get the folders
   * returns false if the connection failed
   */
  virtual bool listDirectory(bool secondStep = false);

  /** Return the trash folder. */
  KMFolder* trashFolder() const;

  /**
   * The user's rights on this folder - see bitfield in ACLJobs namespace.
   * @return 0 when not known yet, -1 if there was an error fetching them
   */
  int userRights() const { return mUserRights; }

  /// Set the user's rights on this folder - called by getUserRights
  void setUserRights( unsigned int userRights );

  /// Return the list of ACL for this folder
  typedef QValueVector<KMail::ACLListEntry> ACLList;
  const ACLList& aclList() const { return mACLList; }

  /// Set the list of ACL for this folder (for FolderDiaACLTab)
  void setACLList( const ACLList& arr );

  /// Reimplemented from FolderStorage
  void setContentsType( KMail::FolderContentsType type );

  // Reimplemented so the mStatusChangedLocally bool can be set
  virtual void setStatus(QValueList<int>& ids, KMMsgStatus status, bool toggle);

protected slots:
  /**
   * Connected to ListJob::receivedFolders
   * creates/removes folders
   */
  void slotListResult(const QStringList&, const QStringList&,
      const QStringList&, const QStringList&, const ImapAccountBase::jobData& );

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
  void slotGetAnnotationResult( KIO::Job* );
  void slotSetAnnotationResult(KIO::Job *job);
  void slotReceivedUserRights( KMFolder* );
  void slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& );

  void slotMultiSetACLResult(KIO::Job *);
  void slotACLChanged( const QString&, int );
  void slotDeleteMessagesResult(KMail::FolderJob *);
  void slotImapStatusChanged(KMFolder* folder, const QString&, bool);

protected:
  /* returns true if there were messages to delete
     on the server */
  bool deleteMessages();
  void listMessages();
  void uploadNewMessages();
  void uploadFlags();
  void createNewFolders();

  void listDirectory2();


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

public slots:
  /**
   * Add the data a KIO::Job retrieves to the buffer
   */
  void slotSimpleData(KIO::Job * job, const QByteArray & data);

  /**
   * Troubleshoot the IMAP cache
   */
  void slotTroubleshoot();

private slots:
  void serverSyncInternal();
  void slotIncreaseProgress();
  void slotUpdateLastUid();

signals:
  void folderComplete(KMFolderCachedImap *folder, bool success);
  void listComplete( KMFolderCachedImap* );

  /** emitted when we enter the state "state" and
     have to process "number" items (for example messages
  */
  void syncState( int state, int number );

private:
  QString state2String( int state ) const;

  /** State variable for the synchronization mechanism */
  enum {
    SYNC_STATE_INITIAL,
    SYNC_STATE_PUT_MESSAGES,
    SYNC_STATE_UPLOAD_FLAGS,
    SYNC_STATE_CREATE_SUBFOLDERS,
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
  QString     mAnnotationFolderType;

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

  int mUserRights;
  ACLList mACLList;

  bool mSilentUpload;
  bool mFolderRemoved;
  bool mResync;
  //bool mHoldSyncs;
  bool mRecurse;
  bool mCreateInbox;
  bool mContentsTypeChanged;
  /** Set to true by setStatus. Indicates that the client has changed
      the status of at least one mail. The mail flags will therefore be
      uploaded to the server, overwriting the server's notion of the status
      of the mails in this folder. */
  bool mStatusChangedLocally;
};

#endif /*kmfoldercachedimap_h*/
