#ifndef kmfoldercachedimap_h
#define kmfoldercachedimap_h

#include <assert.h>
#include <qvaluelist.h>

#include <qdialog.h>

#include "kmfoldermaildir.h"
#include "kmfolderimap.h"
#include "kmacctimap.h"
#include "kmfoldertype.h"

class KMAcctCachedImap;
class KMFolderCachedImap;

#define KMFolderCachedImapInherited KMFolderMaildir

class KMCachedImapJob : public QObject {
  Q_OBJECT
public:
  enum JobType { tListDirectory, tGetFolders, tCreateFolder, tExpungeFolder,
		 tDeleteMessage, tGetMessage, tPutMessage, tAddSubfolders,
		 tDeleteFolders, tCheckUidValidity, tRenameFolder };

  KMCachedImapJob( const QValueList<KMMessage*>& msgs,
		   JobType type = tGetMessage, KMFolderCachedImap* folder=0 );
  KMCachedImapJob( const QValueList<KMFolderCachedImap*>& folders,
		   JobType type = tAddSubfolders,
		   KMFolderCachedImap* folder = 0 );
  KMCachedImapJob( const QValueList<ulong>& uids,
		   JobType type = tGetMessage, KMFolderCachedImap* folder = 0,
		   const QValueList<int>& flags = QValueList<int>() );
  KMCachedImapJob( const QString& string1, JobType type,
		   KMFolderCachedImap* folder );
  KMCachedImapJob( const QStringList& folders, JobType type, KMFolderCachedImap* folder = 0 );
  KMCachedImapJob( JobType type, KMFolderCachedImap* folder );

  ~KMCachedImapJob();

  void setPassiveDestructor( bool passive ) { mPassiveDestructor = passive; }
  bool passiveDestructor() { return mPassiveDestructor; }

signals:
  void messageRetrieved( KMMessage * );
  void messageStored( KMMessage * );
  void finished();

protected:
  virtual void deleteMessages( const QString& uids );
  virtual void expungeFolder();
  virtual void checkUidValidity();
  virtual void renameFolder( const QString &newName );

protected slots:
  virtual void slotGetNextMessage( KIO::Job *job = 0 );
  virtual void slotAddNextSubfolder( KIO::Job *job = 0 );
  virtual void slotPutNextMessage();
  virtual void slotPutMessageDataReq( KIO::Job *job, QByteArray &data );
  virtual void slotPutMessageResult( KIO::Job *job );
  virtual void slotDeleteResult( KIO::Job *job );
  virtual void slotDeleteNextFolder( KIO::Job *job = 0 );
  virtual void slotCheckUidValidityResult( KIO::Job *job );
  virtual void slotRenameFolderResult( KIO::Job *job );

private:
  void init( JobType type );

  JobType mType;
  KMFolderCachedImap *mFolder;
  KMAcctCachedImap   *mAccount;
  QValueList<KMMessage*> mMsgList;
  QValueList<KMFolderCachedImap*> mFolderList;
  QValueList<ulong> mUidList;
  QValueList<int> mFlags;
  QStringList mFolderPathList; // Used only for folder deletion
  ulong mUid;
  int mFlag;
  KMMessage* mMsg;
  QString mString; // Used as uids and as rename target
  KIO::Job *mJob;
  QByteArray mData;

  // If this is true, the destructor won't emit finished()
  bool mPassiveDestructor;
};

class KMFolderCachedImap : public KMFolderMaildir
{
  Q_OBJECT
public:
  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderCachedImap(KMFolderDir* parent=0, const QString& name=QString::null);
  virtual ~KMFolderCachedImap();

  /** Returns the type of this folder */
  virtual KMFolderType folderType() const { return KMFolderTypeCachedImap; }

  /** Remove this folder */
  virtual int remove();

  /** Synchronize this folder and it's subfolders with the server */
  virtual void serverSync();

  /** Force the sync state to be done. */
  void resetSyncState() { mSyncState = SYNC_STATE_INITIAL; }

  virtual void checkUidValidity();

  enum imapState { imapNoInformation=0, imapInProgress=1, imapFinished=2 };

  virtual imapState getContentState() { return mContentState; }
  virtual void setContentState(imapState state) { mContentState = state; }

  virtual imapState getSubfolderState() { return mSubfolderState; }
  virtual void setSubfolderState(imapState state) { mSubfolderState = state; }

  virtual QCString protocol() const { return "cachedimap"; }

  /** The path to the imap folder on the server */
  void setImapPath(const QString &path) { mImapPath = path; }
  QString imapPath() { return mImapPath; }

  /** The highest UID in the folder */
  void setLastUid( ulong uid );
  ulong lastUid();

  /** Find message by UID. Returns NULL if it doesn't exist */
  KMMsgBase* findByUID( ulong uid );

  /** The uidvalidity of the last update */
  void setUidValidity(const QString &validity) { mUidValidity = validity; }
  QString uidValidity() const { return mUidValidity; }

  /** The imap account associated with this folder */
  void setAccount(KMAcctCachedImap *acct);
  KMAcctCachedImap* account();

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the
      c-library fopen call otherwise. */
  virtual int create(bool imap = FALSE);

  /** Returns the filename of the uidcache file */
  QString uidCacheLocation() const;

  /** Read the uidValitidy and lastUid values from disk */
  int readUidCache();

  /** Write the uidValitidy and lastUid values to disk */
  int writeUidCache();


  /* Reimplemented from KMFolder. Moving is not supported, so aParent must be 0 */
  virtual int rename(const QString& aName, KMFolderDir *aParent=0);

  /* Reimplemented from KMFolderMaildir */
  virtual KMMessage* take(int idx);
  /* Reimplemented from KMFolderMaildir */
  virtual int addMsg(KMMessage* msg, int* index_return = NULL);
  /* internal version that doesn't remove the X-UID header */
  virtual int addMsgInternal(KMMessage* msg, int* index_return = NULL);

  /* Reimplemented from KMFolderMaildir */
  virtual void removeMsg(int i, bool imapQuiet = FALSE);

  /* Utility methods: */
  static QStringList makeSets(QStringList& uids, bool sort);
  static QStringList makeSets(QValueList<ulong>& uids, bool sort);

  /**
   * Emit the folderComplete signal
   */
  void sendFolderComplete(bool success)
  { emit folderComplete(this, success); }

  static KMMsgStatus flagsToStatus(int flags, bool newMsg = TRUE);
  static QCString statusToFlags(KMMsgStatus status);

  /**
   * The silentUpload can be set to remove the folder upload error dialog
   */
  void setSilentUpload( bool silent ) { mSilentUpload = silent; }
  bool silentUpload() { return mSilentUpload; }

  int prog;

protected slots:
  /**
   * Add the imap folders to the folder tree
   */
  void slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds);
  /**
   * Free the resources
   */
  void slotListResult(KIO::Job * job);

  void slotGetMessagesData(KIO::Job * job, const QByteArray & data);
  void getMessagesResult(KIO::Job * job, bool lastSet);
  void slotGetMessagesResult(KIO::Job * job);
  void slotGetLastMessagesResult(KIO::Job * job);

  //virtual void slotCheckValidityResult(KIO::Job * job);
  virtual void listMessages();
  virtual void uploadNewMessages();
  /* returns true if there were messages to delete
     on the server */
  virtual bool deleteMessages();
  virtual void createNewFolders();

  /**
   * List a directory and add the contents to kmfoldermgr
   * returns false if the connection failed
   */
  bool listDirectory();
  void listDirectory2();


  /** Utility methods for syncing. Finds new messages
      in the local cache that must be uploaded */
  virtual QValueList<KMMessage*> findNewMessages();
  /** Utility methods for syncing. Finds new subfolders
      in the local cache that must be created in the server */
  virtual QValueList<KMFolderCachedImap*> findNewFolders();

  /** This returns false if we have subfolders. Otherwise it returns ::canRemoveFolder() */
  virtual bool canRemoveFolder() const;

public slots:
  /**
   * Add the data a KIO::Job retrieves to the buffer
   */
  void slotSimpleData(KIO::Job * job, const QByteArray & data);

private slots:
  void serverSyncInternal();

signals:
  void folderComplete(KMFolderCachedImap *folder, bool success);
  void listComplete( KMFolderCachedImap* );

  void listMessagesComplete();

  /* emitted at each state */
  void newState( const QString& folderName, int progressLevel, const QString& syncStatus );
 
  /** emitted when we enter the state "state" and
     have to process "number" items (for example messages
  */
  void syncState( int state, int number );

private:

  /** State variable for the synchronization mechanism */
  enum {
    SYNC_STATE_INITIAL,
    SYNC_STATE_PUT_MESSAGES,
    SYNC_STATE_CREATE_SUBFOLDERS,
    SYNC_STATE_LIST_SUBFOLDERS,
    SYNC_STATE_LIST_SUBFOLDERS2,
    SYNC_STATE_DELETE_SUBFOLDERS,
    SYNC_STATE_LIST_MESSAGES,
    SYNC_STATE_DELETE_MESSAGES,
    SYNC_STATE_EXPUNGE_MESSAGES,
    SYNC_STATE_GET_MESSAGES,
    SYNC_STATE_HANDLE_INBOX,
    SYNC_STATE_FIND_SUBFOLDERS,
    SYNC_STATE_SYNC_SUBFOLDERS,
    SYNC_STATE_CHECK_UIDVALIDITY
  } mSyncState;

  QString mUidValidity;
  QString     mImapPath;
  imapState   mContentState, mSubfolderState;
  QStringList mSubfolderNames, mSubfolderPaths, mSubfolderMimeTypes;

  bool        mHasInbox;
  bool        mIsSelected;
  bool        mCheckFlags;
  bool        mReadOnly;
  QGuardedPtr<KMAcctCachedImap> mAccount;

  QValueList<ulong> uidsOnServer;
  QValueList<ulong> uidsForDeletionOnServer;
  QValueList<ulong> uidsForDownload;
  QValueList<int>   flagsForDownload;
  QStringList       foldersForDeletionOnServer;

  QValueList<KMFolderCachedImap*> mSubfoldersForSync;
  KMFolderCachedImap* mCurrentSubfolder;

  /* Mapping between index<->uid
     Keep updated in addMsg, take and removeMsg */
  QMap<ulong,int> uidMap; /* maps uid -> idx */
  QMap<int,ulong> uidRevMap; /* maps isx -> uid */
  ulong       mLastUid;
  void reloadUidMap();

  QString state2String( int state ) const;
  bool mIsConnected;

  bool mSilentUpload;
  bool mFolderRemoved;
};

#endif /*kmfoldercachedimap_h*/
