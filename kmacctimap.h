/**
 * kmacctimap.h
 *
 * Copyright (c) 2000-2001 Michael Haeckel <Michael@Haeckel.Net>
 *
 * This file is based on kmacctexppop.h by Don Sanders
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

#ifndef KMAcctImap_h
#define KMAcctImap_h

#include "kmaccount.h"
#include <qdialog.h>
#include <kio/global.h>
#include <kio/job.h>

class QLineEdit;
class QPushButton;
class KMFolderImap;
class KMFolderTreeItem;
class KMImapJob;

#define KMAcctImapInherited KMAccount

//-----------------------------------------------------------------------------
class KMAcctImap: public KMAccount
{
  Q_OBJECT
  friend class KMImapJob;

public:
  virtual ~KMAcctImap();
  virtual void init(void);

  /**
   * Initialize the slave configuration
   */
  virtual void initSlaveConfig();

  /**
   * Imap user login name
   */
  const QString& login(void) const { return mLogin; }
  virtual void setLogin(const QString&);

  /**
   * Imap user password
   */
  QString passwd(void) const;
  virtual void setPasswd(const QString&, bool storeInConfig=FALSE);

  /**
   * Imap authentificaion method
   */
  QString auth(void) const { return mAuth; }
  virtual void setAuth(const QString&);

  /**
   * Will the password be stored in the config file ?
   */
  bool storePasswd(void) const { return mStorePasswd; }
  virtual void setStorePasswd(bool);

  /**
   * Imap host
   */
  const QString& host(void) const { return mHost; }
  virtual void setHost(const QString&);

  /**
   * Port on Imap host
   */
  unsigned short int port(void) { return mPort; }
  virtual void setPort(unsigned short int);

  /**
   * Prefix to the Imap folders
   */
  const QString& prefix(void) const { return mPrefix; }
  virtual void setPrefix(const QString&);

  /**
   * Automatically expunge deleted messages when leaving the folder
   */
  bool autoExpunge() { return mAutoExpunge; }
  virtual void setAutoExpunge(bool);

  /**
   * Show hidden files on the server
   */
  bool hiddenFolders() { return mHiddenFolders; }
  virtual void setHiddenFolders(bool);

  /**
   * Use SSL or not
   */
  bool useSSL() { return mUseSSL; }
  virtual void setUseSSL(bool);

  /**
   * Use TLS or not
   */
  bool useTLS() { return mUseTLS; }
  virtual void setUseTLS(bool);

  /**
   * Inherited methods.
   */
  virtual QString type(void) const;
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);
  virtual void processNewMail(bool) { emit finishedCheck(false); }
  virtual void pseudoAssign(KMAccount*);

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
   * Connect to the IMAP server, if no connection is active
   */
  bool makeConnection();

  /**
   * Update the progress bar
   */
  void displayProgress();
 
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
   * Get the Slave used for the account
   */
  KIO::Slave * slave() { return mSlave; }
  void slaveDied() { mSlave = NULL; }

  /**
   * Set the top level pseudo folder
   */
  virtual void setImapFolder(KMFolderImap *);

signals:
  /**
   * Emitted, when the account is deleted
   */
  void deleted(KMAcctImap*);

protected:
  friend class KMAcctMgr;
  friend class KMPasswdDialog;
  KMAcctImap(KMAcctMgr* owner, const QString& accountName);

  /**
   * Very primitive en/de-cryption so that the password is not
   * readable in the config file. But still very easy breakable.
   */
  QString encryptStr(const QString &inStr) const;
  QString decryptStr(const QString &inStr) const;

  QString mLogin, mPasswd;
  QString mHost, mAuth;
  QString mPrefix;
  unsigned short int mPort;
  bool    mStorePasswd;
  bool    mAutoExpunge;
  bool    mUseSSL;
  bool    mUseTLS;
  bool    mHiddenFolders;
  bool    mProgressEnabled;
  int     mTotal;
  bool    mIdle;
  QTimer  mIdleTimer;
  KIO::Slave *mSlave;
  KIO::MetaData mSlaveConfig;
  QList<KMImapJob> mJobList;
  KMFolderImap *mFolder;

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
   * Only delete information about the job
   */
  void slotSimpleResult(KIO::Job * job);

  /**
   * Display an error message, that connecting failed
   */
  void slotSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);
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
#endif /*KMAcctImap_h*/
