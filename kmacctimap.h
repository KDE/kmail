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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
}

//-----------------------------------------------------------------------------
class KMAcctImap: public KMail::ImapAccountBase
{
  Q_OBJECT
  friend class KMail::ImapJob;

protected: // ### Hacks
  void setPrefixHook();

public:
  virtual ~KMAcctImap();

  /** A weak assignment operator */
  virtual void pseudoAssign( const KMAccount * a );

  /**
   * Inherited methods.
   */
  virtual QString type(void) const;
  virtual void processNewMail(bool);

  /**
   * Kill all jobs related the the specified folder/msg
   */
  virtual void ignoreJobsForMessage( KMMessage * msg );
  virtual void ignoreJobsForFolder( KMFolder * folder );

  /**
   * Kill the slave if any jobs are active
   */
  void killAllJobs( bool disconnectSlave=false );

  /**
   * Set the account idle or busy
   */
  void setIdle(bool aIdle) { mIdle = aIdle; }

  /**
   * Set the top level pseudo folder
   */
  virtual void setImapFolder(KMFolderImap *);

  /**
   * Lists the directory starting from @p path
   * All parameters (onlySubscribed, secondStep, parent) are included
   * in the jobData
   * connects to slotListResult and slotListEntries
   */
  void listDirectory(QString path, bool onlySubscribed,
      bool secondStep = FALSE, KMFolder* parent = NULL);

  /** 
   * Starts the folderlisting for the root folder
   */   
  virtual void listDirectory();

  /**
   * Read config file entries. This method is called by the account
   * manager when a new account is created. The config group is
   * already properly set by the caller.
   */
  virtual void readConfig(KConfig& config);  

public slots:
  void processNewMail() { processNewMail(TRUE); }

  /**
   * Display an error message
   */
  void slotSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

  /**
   * updates the new-mail-check folderlist
   */
  void slotUpdateFolderList();

protected:
  friend class KMAcctMgr;
  friend class KMPasswdDialog;
  KMAcctImap(KMAcctMgr* owner, const QString& accountName);

  QPtrList<KMail::ImapJob> mJobList;
  QGuardedPtr<KMFolderImap> mFolder;

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
  void postProcessNewMail( KMFolder * f ) { ImapAccountBase::postProcessNewMail( f ); }

};

#endif /*KMAcctImap_h*/
