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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef kmfolderimap_h
#define kmfolderimap_h

#include "kmacctimap.h"
#include "kmfoldermbox.h"
#include "kmmsgbase.h"

#include "kio/job.h"
#include "kio/global.h"

class KMFolderTreeItem;
class KMFolderImap;

class KMImapJob : public QObject
{
  Q_OBJECT

public:
  enum JobType { tListDirectory, tGetFolder, tCreateFolder, tDeleteMessage,
    tGetMessage, tPutMessage, tCopyMessage };
  KMImapJob(KMMessage *msg, JobType jt = tGetMessage, KMFolderImap *folder = NULL);
  KMImapJob(QPtrList<KMMessage>& msgList, QString sets, JobType jt = tGetMessage, KMFolderImap *folder = NULL );
  ~KMImapJob();
  static void ignoreJobsForMessage(KMMessage *msg);
signals:
  void messageRetrieved(KMMessage *);
  void messageStored(KMMessage *);
  void messageCopied(KMMessage *);
  void messageCopied(QPtrList<KMMessage>);
private slots:
  void slotGetMessageResult(KIO::Job * job);
  void slotGetNextMessage();
  /** Feeds the message in pieces to the server */
  void slotPutMessageDataReq(KIO::Job *job, QByteArray &data);
  void slotPutMessageResult(KIO::Job *job);
  void slotCopyMessageResult(KIO::Job *job);
private:
  void init(JobType jt, QString sets, KMFolderImap *folder, QPtrList<KMMessage>& msgList);
  JobType mType;
  KMMessage *mMsg;
  KMFolderImap *mDestFolder;
  KIO::Job *mJob;
  QByteArray mData;
  QCString mStrData;
  KMFolderTreeItem *mFti;
  int mTotal, mDone, mOffset;
};

#define KMFolderImapInherited KMFolderMbox

class KMFolderImap : public KMFolderMbox
{
  Q_OBJECT
  friend class KMImapJob;

public:
  enum imapState { imapNoInformation=0, imapInProgress=1, imapFinished=2 };

  virtual imapState getContentState() { return mContentState; }
  virtual void setContentState(imapState state) { mContentState = state; }

  virtual imapState getSubfolderState() { return mSubfolderState; }
  virtual void setSubfolderState(imapState state) { mSubfolderState = state; }

  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderImap(KMFolderDir* parent=0, const QString& name=QString::null);
  virtual ~KMFolderImap();

  virtual QCString protocol() const { return "imap"; }

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
  KMAcctImap* account() { return mAccount; }
  
  /** Remove (first occurance of) given message from the folder. */
  virtual void removeMsg(int i, bool quiet = FALSE);
  virtual void removeMsg(QPtrList<KMMessage> msgList, bool quiet = FALSE);

  /** Remove the IMAP folder on the server and if successful also locally */
  virtual void removeOnServer();

  /** Automatically expunge deleted messages when leaving the folder */
  bool autoExpunge() { return mAccount->autoExpunge(); }

  /** Write the config file */
  virtual void writeConfig();

  /** Read the config file */
  virtual void readConfig();

  /**
   * List a directory and add the contents to a KMFolderTreeItem
   */
  void listDirectory(KMFolderTreeItem * fti, bool secondStep = FALSE);

  /**
   * Retrieve all mails in a folder
   */
  void getFolder();

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
  void deleteMessage(QPtrList<KMMessage> msgList);

  /**
   * Change the status of the message indicated by @p index
   * Overloaded function for the following one
   */
  virtual void setStatus(int idx, KMMsgStatus status);

  /**
   * Change the status of several messages indicated by @p ids
   */
  virtual void setStatus(QValueList<int>& ids, KMMsgStatus status);

  /** Helper method to set the status on the server */
  void setImapStatus(QString path, QCString flags);

  /** generates sets of uids */
  QStringList makeSets(QValueList<int>&, bool sort = true);
  QStringList makeSets(QStringList&, bool sort = true);

  /** gets the uids of the given ids */ 
  void getUids(QValueList<int>& ids, QValueList<int>& uids, bool unget = false);
 
  /** same as above but accepts a Message-List
   * if unget is true the messages are unget'ted */ 
  void getUids(QPtrList<KMMessage>& msgList, QValueList<int>& uids, bool unget = false, KMFolder* msgParent = NULL);

  /**
   * Expunge deleted messages from the folder
   */
  void expungeFolder(KMFolderImap * aFolder, bool quiet);

  /**
   * Emit the folderComplete signal
   */
  void sendFolderComplete(bool success)
  { emit folderComplete(this, success); }

  /**
   * Refresh the number of unseen mails
   */
  void processNewMail(bool interactive);

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
  static QCString statusToFlags(KMMsgStatus status);

  /**
   * Return the filename of the folder (reimplemented from KFolder)
   */
  virtual QString fileName() const { return encodeFileName(name()); }

signals:
  void folderComplete(KMFolderImap *folder, bool success);

  /**
   * Emitted, when the account is deleted
   */
  void deleted(KMFolderImap*);

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
  virtual int addMsg(KMMessage* msg, int* index_return = NULL);
  int addMsg(QPtrList<KMMessage>&, int* index_return = NULL);

  /** Copy the messages to this folder */
  void copyMsg(QPtrList<KMMessage>& msgList/*, KMFolder* parent*/);

  QPtrList<KMMessage> splitMessageList(QString set, QPtrList<KMMessage>& msgList);

  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  virtual KMMessage* take(int idx);
  virtual void take(QPtrList<KMMessage>);

  /**
   * Add the data a KIO::Job retrieves to the buffer
   */
  void slotSimpleData(KIO::Job * job, const QByteArray & data);

protected:
  /**
   * Convert IMAP flags to a message status
   * @param newMsg specifies whether unseen messages are new or unread
   */
  KMMsgStatus flagsToStatus(int flags, bool newMsg = TRUE);

  void getMessagesResult(KIO::Job * job, bool lastSet);

  bool    gotMsgs;

  QString mUidValidity;

protected slots:

  /**
   * Add the imap folders to the folder tree
   */
  void slotListEntries(KIO::Job * job, const KIO::UDSEntryList & uds);

  /**
   * Free the resources
   */
  void slotListResult(KIO::Job * job);

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
   * Only delete information about the job and ignore write errors
   */
  void slotSetStatusResult(KIO::Job * job);

  /**
   * Remove the folder also locally, if removing on the server succeeded
   */
  void slotRemoveFolderResult(KIO::Job *job);

  /**
   * Update the number of unseen messages
   */
  void slotStatResult(KIO::Job *job);

protected:
  QString     mImapPath;
  ulong       mLastUid;
  imapState   mContentState, mSubfolderState;
  QStringList mSubfolderNames, mSubfolderPaths, mSubfolderMimeTypes;
  bool        mHasInbox;
  bool        mIsSelected;
  bool        mCheckFlags;
  bool        mReadOnly;
  QGuardedPtr<KMAcctImap> mAccount;
};

#endif // kmfolderimap_h
