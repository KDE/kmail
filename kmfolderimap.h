/**
 * kmfolder.h
 *
 * Copyright (c) 2001 Kurt Granroth <granroth@kde.org>
 * Copyright (c) 2000 Michael Haeckel <Michael@Haeckel.Net>
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
#include "kmfoldermaildir.h"
#include "kmmsgbase.h"

#include "kio/job.h"
#include "kio/global.h"

#include <qdialog.h>

class KMFolderTreeItem;
class KMFolderImap;
class KMFolderMaildir;

class QLineEdit;
class QPushButton;

class KMImapJob : public QObject
{
  Q_OBJECT

public:
  enum JobType { tListDirectory, tGetFolder, tCreateFolder, tDeleteMessage,
    tGetMessage, tPutMessage, tCopyMessage };
  KMImapJob(KMMessage *msg, JobType jt = tGetMessage, KMFolderImap *folder = NULL);
  ~KMImapJob();
  static void ignoreJobsForMessage(KMMessage *msg);
signals:
  void messageRetrieved(KMMessage *);
  void messageStored(KMMessage *);
  void messageCopied(KMMessage *);
private slots:
  void slotGetMessageResult(KIO::Job * job);
  void slotGetNextMessage();
  void slotPutMessageDataReq(KIO::Job *job, QByteArray &data);
  void slotPutMessageResult(KIO::Job *job);
  void slotCopyMessageResult(KIO::Job *job);
private:
  JobType mType;
  KMMessage *mMsg;
  KMFolderImap *mDestFolder;
  KIO::Job *mJob;
  QByteArray mData;
  QCString mStrData;
  KMFolderTreeItem *mFti;
  int mTotal, mDone, mOffset;
};

#define KMFolderImapInherited KMFolderMaildir

class KMFolderImap : public KMFolderMaildir
{
  Q_OBJECT
  friend class KMImapJob;

public:
  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolderImap(KMFolderDir* parent=NULL, const QString& name=NULL);
  virtual ~KMFolderImap();

  virtual QCString protocol() const { return "imap"; }

  /** The path to the imap folder on the server */
  void setImapPath(const QString &path) { mImapPath = path; }
  QString imapPath() { return mImapPath; }

  /** The next predicted UID of the folder */
  void setUidNext(const QString &uidNext) { mUidNext = uidNext; }
  QString uidNext() { return mUidNext; }

  /** The uidvalidity of the last update */
  void setUidValidity(const QString &validity) { mUidValidity = validity; }
  QString uidValidity() { return mUidValidity; }

  /** The imap account associated with this folder */
  void setAccount(KMAcctImap *acct) { mAccount = acct; }
  KMAcctImap* account() { return mAccount; }
  
  /** Remove (first occurance of) given message from the folder. */
  virtual void removeMsg(int i, bool quiet = FALSE);

  /** Automatically expunge deleted messages when leaving the folder */
  bool autoExpunge() { return mAccount->autoExpunge(); }

  /** Write the config file */
  virtual void writeConfig();

  /** Read the config file */
  virtual void readConfig();

  /**
   * Initialize the slave configuration
   */
  virtual void initSlaveConfig();

  /**
   * List a directory and add the contents to a KMFolderTreeItem
   */
  void listDirectory(KMFolderTreeItem * fti, bool secondStep = FALSE);

  /**
   * Retrieve all mails in a folder
   */
  void getFolder(KMFolderTreeItem * fti);

  /**
   * Get the whole message
   */
  void getMessage(KMFolder * folder, KMMessage * msg);

  /**
   * Create a new subfolder
   */
  void createFolder(KMFolderTreeItem * fti, const QString &name);

  /**
   * Kill all jobs related the the specified folder
   */
  void killJobsForItem(KMFolderTreeItem * fti);

  /**
   * Kill the slave if any jobs are active
   */
  void killAllJobs();

  /**
   * Set the account idle or busy
   */
  void setIdle(bool aIdle) { mIdle = aIdle; }

  /**
   * Delete a message
   */
  void deleteMessage(KMMessage * msg);

  /**
   * Change the status of a message
   */
  void setStatus(KMMessage * msg, KMMsgStatus status);

  /**
   * Expunge deleted messages from the folder
   */
  void expungeFolder(KMFolderImap * aFolder);

  struct jobData
  {
    QByteArray data;
    QCString cdata;
    QStringList items;
    KMFolderTreeItem *parent;
    int total, done;
    bool inboxOnly;
  };
  QMap<KIO::Job *, jobData> mapJobData;

  /**
   * Get the URL for the account
    */
  KURL getUrl();

  /**
   * Update the progress bar
   */
  void displayProgress();

  /**
   * Get the Slave used for the account
   */
  KIO::Slave * slave() { return mSlave; }
  void slaveDied() { mSlave = NULL; }

signals:
  void folderComplete(KMFolderTreeItem * fti, bool success);

  /**
   * Emitted, when the account is deleted
   */
  void deleted(KMFolderImap*);

public slots:
  /** Add the message to the folder after it has been retrieved from an IMAP
      server */
  virtual void reallyAddMsg(KMMessage *);

  /** Add a message to a folder after is has been added on an IMAP server */
  virtual void addMsgQuiet(KMMessage *);

  /** Add a copy of the message to the folder after it has been retrieved
      from an IMAP server */
  virtual void reallyAddCopyOfMsg(KMMessage *);

  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  virtual KMMessage* take(int idx);

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

  /**
   * Connect to the IMAP server, if no connection is active
   */
  bool makeConnection();

  bool    gotMsgs;
  bool    mProgressEnabled;
  int     mTotal;
  bool    mIdle;
  QTimer  mIdleTimer;

  KIO::Slave *mSlave;
  KIO::MetaData mSlaveConfig;

  QList<KMImapJob> mJobList;

  QString mUidValidity;

protected slots:
  /**
   * Send a NOOP command or log out when idle
   */
  void slotIdleTimeout();

  /**
   * Kills all jobs
   */
  void slotAbortRequested();

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
  void checkValidity(KMFolderTreeItem * fti);
  void slotCheckValidityResult(KIO::Job * job);

  /**
   * Get the folder now (internal)
   */
  void reallyGetFolder(KMFolderTreeItem * fti,
                       const QString &startUid = QString::null);

  /**
   * Retrieve the next message
   */
  void getNextMessage(jobData & jd);

  /**
   * For listing the contents of a folder
   */
  void slotListFolderResult(KIO::Job * job);
  void slotListFolderEntries(KIO::Job * job, const KIO::UDSEntryList & uds);

  /**
   * For retrieving a message digest
   */
  void slotGetMessagesResult(KIO::Job * job);
  void slotGetMessagesData(KIO::Job * job, const QByteArray & data);

  /**
   * For creating a new subfolder
   */
  void slotCreateFolderResult(KIO::Job * job);

  /**
   * Only delete information about the job
   */
  void slotSimpleResult(KIO::Job * job);

  /**
   * Only delete information about the job and ignore write errors
   */
  void slotSetStatusResult(KIO::Job * job);

  /**
   * Display an error message, that connecting failed
   */
  void slotSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

protected:
  QString    mImapPath;
  QString    mUidNext;
  KMAcctImap *mAccount;
};


//-----------------------------------------------------------------------------
class KMImapPasswdDialog : public QDialog
{
  Q_OBJECT

public:
  KMImapPasswdDialog(QWidget *parent = 0,const char *name= 0,
                     KMAcctImap *act=0, const QString &caption=QString::null,
                     const QString &login=QString::null,
                     const QString &passwd=QString::null);

private:
  QLineEdit *usernameLEdit;
  QLineEdit *passwdLEdit;
  QPushButton *ok;
  QPushButton *cancel;
  KMAcctImap *act;

private slots:
  void slotOkPressed();
  void slotCancelPressed();

};
#endif // kmfolderimap_h
