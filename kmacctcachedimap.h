/**
 * kmacctcachedimap.h
 *
 * Copyright (c) 2002-2003 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
 * Copyright (c) 2002-2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
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

#ifndef KMAcctCachedImap_h
#define KMAcctCachedImap_h

#include "imapaccountbase.h"

#include <qguardedptr.h>

class KMFolderCachedImap;
class KMFolderTreeItem;
namespace KMail {
  class IMAPProgressDialog;
  class FolderJob;
  class ImapJob;
  class CachedImapJob;
}
using KMail::ImapJob;
using KMail::CachedImapJob;

namespace KIO {
  class Job;
}

class KMAcctCachedImap: public KMail::ImapAccountBase
{
  Q_OBJECT
  friend class ImapJob;
  friend class CachedImapJob;

protected: // ### Hacks
  void setPrefixHook();

public:
  virtual ~KMAcctCachedImap();
  virtual void init();

  /** A weak assignment operator */
  virtual void pseudoAssign( const KMAccount * a );

  /**
   * Overloaded to make sure it's never set for cached IMAP.
   */
  virtual void setAutoExpunge(bool);

  /**
   * Inherited methods.
   */
  virtual QString type() const;
  virtual void processNewMail( bool i ) { processNewMail( mFolder, i ); }

  void processNewMail( KMFolderCachedImap*, bool );

  /**
   * Update the progress bar
   */
  virtual void displayProgress();

  /**
   * Kill all jobs related the the specified folder
   */
  void killJobsForItem(KMFolderTreeItem * fti);

  /**
   * Kill the slave if any jobs are active
   */
  virtual void killAllJobs( bool disconnectSlave=false );

  /**
   * Set the account idle or busy
   */
  void setIdle(bool aIdle) { mIdle = aIdle; }

  void slaveDied() { mSlave = 0; killAllJobs(); }

  /**
   * Set the top level pseudo folder
   */
  virtual void setImapFolder(KMFolderCachedImap *);

  KMail::IMAPProgressDialog * imapProgressDialog() const;
  bool isProgressDialogEnabled() const { return mProgressDialogEnabled; }
  void setProgressDialogEnabled( bool enable ) { mProgressDialogEnabled = enable; }

  virtual void readConfig( /*const*/ KConfig/*Base*/ & config );
  virtual void writeConfig( KConfig/*Base*/ & config ) /*const*/;

  /**
   * Invalidate the local cache.
   */
  virtual void invalidateIMAPFolders();
  virtual void invalidateIMAPFolders( KMFolderCachedImap* );

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

public slots:
  void processNewMail() { processNewMail( mFolder, true ); }

  /**
   * Display an error message
   */
  void slotSlaveError(KIO::Slave *aSlave, int, const QString &errorMsg);

protected:
  friend class KMAcctMgr;
  KMAcctCachedImap(KMAcctMgr* owner, const QString& accountName);

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
  void postProcessNewMail(KMFolderCachedImap*, bool);
  void postProcessNewMail( KMFolder * f ) { ImapAccountBase::postProcessNewMail( f ); }

private:
  QPtrList<CachedImapJob> mJobList;
  KMFolderCachedImap *mFolder;
  mutable QGuardedPtr<KMail::IMAPProgressDialog> mProgressDlg;
  bool mProgressDialogEnabled;
  bool mSyncActive;
};

#endif /*KMAcctCachedImap_h*/
