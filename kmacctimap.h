/**
 * kmacctimap.h
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
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

#include "imapaccountbase.h"

class KMFolderImap;
class KMFolderTreeItem;
namespace KMail {
  class ImapJob;
}
namespace KIO {
  class Job;
};

//-----------------------------------------------------------------------------
class KMAcctImap: public KMail::ImapAccountBase
{
  Q_OBJECT
  friend class KMail::ImapJob;

protected: // ### Hacks
  void setPrefixHook();

public:
  typedef KMail::ImapAccountBase base;

  virtual ~KMAcctImap();

  /** A weak assignment operator */
  virtual void pseudoAssign( const KMAccount * a );

  /**
   * Inherited methods.
   */
  virtual QString type(void) const;
  virtual void processNewMail(bool);

  struct jobData
  {
    QByteArray data;
    QCString cdata;
    QStringList items;
    KMFolderImap *parent;
    int total, done, offset;
    bool inboxOnly, quiet;
    QPtrList<KMMessage> msgList;
  };
  QMap<KIO::Job *, jobData> mapJobData;

  /**
   * Update the progress bar
   */
  void displayProgress();

  /**
   * Kill all jobs related the the specified folder
   */
  void killJobsForItem(KMFolderTreeItem * fti);

  virtual void ignoreJobsForMessage( KMMessage * msg );

  /**
   * Kill the slave if any jobs are active
   */
  void killAllJobs( bool disconnectSlave=false );

  /**
   * Set the account idle or busy
   */
  void setIdle(bool aIdle) { mIdle = aIdle; }

  void slaveDied() { mSlave = 0; killAllJobs(); }

  /**
   * Set the top level pseudo folder
   */
  virtual void setImapFolder(KMFolderImap *);

  /**
   * Initialize a jobData structure
   */
  static void initJobData(jobData &jd);

  /**
   * Open a folder and close it again when the network transfer is finished
   */
  int tempOpenFolder(KMFolder *folder);

public slots:
  void processNewMail() { processNewMail(TRUE); }

  /**
   * Display an error message
   */
  void slotSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

protected:
  friend class KMAcctMgr;
  friend class KMPasswdDialog;
  KMAcctImap(KMAcctMgr* owner, const QString& accountName);

  QPtrList<KMail::ImapJob> mJobList;
  KMFolderImap *mFolder;
  QPtrList<QGuardedPtr<KMFolder> > mOpenFolders;

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

  /** new-mail-notification for the current folder (is called via folderComplete) */
  void postProcessNewMail(KMFolderImap*, bool);
  void postProcessNewMail( KMFolder * f ) { base::postProcessNewMail( f ); }

};

#endif /*KMAcctImap_h*/
