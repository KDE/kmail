/**
 * kmfolderimap.h
 *
 * Copyright (c) 2001 Kurt Granroth <granroth@kde.org>
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 *
 * This file is based on kmacctimap.h by Michael Haeckel which is
 * based on kmacctexppop.h by Don Sanders
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
 */

#ifndef kmfolderimap_h
#define kmfolderimap_h

#include "kmacctimap.h"
#include "kmfoldermbox.h"
#include "kmmsgbase.h"
#include "imapaccountbase.h"
using KMail::ImapAccountBase;

#include "kio/job.h"
#include "kio/global.h"

#include <qintdict.h>
#include <qdict.h>
#include <qvaluelist.h>
#include <kstandarddirs.h>

class KMFolderTreeItem;
class KMFolderImap;
class KMSearchPattern;
namespace KMail {
  class FolderJob;
  class ImapJob;
  class AttachmentStrategy;
}
namespace KPIM {
  class ProgressItem;
}
using KMail::FolderJob;
using KMail::ImapJob;
using KMail::AttachmentStrategy;
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
  friend class ImapJob;
public:

  static QString cacheLocation() {
     return locateLocal("data", "kmail/imap" );
  }

  enum imapState { imapNoInformation=0, imapInProgress=1, imapFinished=2 };

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
  void setImapPath(const QString &path) { mImapPath = path; }
  QString imapPath() { return mImapPath; }

  /** The highest UID in the folder */
  ulong lastUid();

  /** The uidvalidity of the last update */
  void setUidValidity(const QString &validity) { mUidValidity = validity; }
  QString uidValidity() { return mUidValidity; }

  /** The imap account associated with this folder */
  void setAccount(KMAcctImap *acct);
  KMAcctImap* account() const { return mAccount; }

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
  virtual bool listDirectory(bool secondStep = false);

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
   */
  void createFolder(const QString &name);

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
  void getUids(const QPtrList<KMMessage>& msgList, QValueList<ulong>& uids, KMFolder* msgParent = 0);

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
   * Insert a new entry into the uid <=> sernum cache
   */
  void insertUidSerNumEntry(ulong uid, const ulong * sernum) {
    uidmap.insert(uid, sernum); }

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
  virtual int create(bool imap = FALSE);

  /** imap folders cannot expire */
  virtual bool isAutoExpire() const { return false; }

  /** Close folder. If force is TRUE the files are closed even if
    others still use it (e.g. other mail reader windows). This also
    cancels all pending jobs. */
  virtual void close(bool force=FALSE);

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
  virtual void search( KMSearchPattern* );
  virtual void search( KMSearchPattern*, Q_UINT32 serNum );

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
  int addMsg(QPtrList<KMMessage>&, int* index_return = 0);

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

  /** Generates an IMAP search command from the pattern */
  QString searchStringFromPattern( KMSearchPattern* );

protected slots:

  /**
   * Connected to ListJob::receivedFolders
   * creates/removes folders
   */
  void slotListResult(const QStringList&, const QStringList&,
      const QStringList&, const QStringList&, const ImapAccountBase::jobData& );

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
   * For renaming folders
   */
  void slotRenameResult( KIO::Job *job );

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
  void slotCreatePendingFolders();

  /**
   * Do an IMAP search
   * It uses the mLastSearchPattern for this
   */
  void slotSearch();

  /**
   * Is called when the search is done
   */ 
  void slotSearchData( KIO::Job * job, const QString& data );
  void slotSearchDataSingleMessage( KIO::Job * job, const QString& data );
  void slotSearchResult( KIO::Job * job );

  /**
   * Called when a msg was downloaded for local search
   */
  void slotSearchMessageArrived( KMMessage* msg );
  void slotSearchSingleMessage( KMMessage* msg );
  
protected:
  QString     mImapPath;
  ulong       mLastUid;
  imapState   mContentState, mSubfolderState;
  bool        mIsSelected;
  bool        mCheckFlags;
  bool        mReadOnly;
  bool        mCheckMail;
  QGuardedPtr<KMAcctImap> mAccount;
  QIntDict<ulong> uidmap;
  QString mUidValidity;
  unsigned int mUserRights;

private:
  bool        mCheckingValidity;
  QDict<KMMsgMetaData> mMetaDataMap;
  bool        mAlreadyRemoved;
  QGuardedPtr<ProgressItem> mMailCheckProgressItem;
  ProgressItem *mListDirProgressItem;
  QStringList mFoldersPendingCreation;
  // remember the SearchPattern
  KMSearchPattern* mLastSearchPattern;
  // saves the patterns that are used for local search
  KMSearchPattern* mLocalSearchPattern;
  // saves the results of the imap search
  QString mImapSearchData;
  // collects the serial numbers from imap and local search
  QValueList<Q_UINT32> mSearchSerNums;
  // the remaining messages that have to be downloaded for local search
  uint mRemainingMsgs;
};

#endif // kmfolderimap_h
