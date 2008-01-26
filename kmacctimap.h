/* -*- mode: C++ -*-
 * kmacctimap.h
 *
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 *
 * This file is based on popaccount.h by Don Sanders
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KMAcctImap_h
#define KMAcctImap_h

#include "imapaccountbase.h"
#include <qdict.h>

class KMFolderImap;
class KMFolderTreeItem;
namespace KMail {
  class ImapJob;
  class ActionScheduler;
}
namespace KIO {
  class Job;
}
class FolderStorage;

//-----------------------------------------------------------------------------
class KMAcctImap: public KMail::ImapAccountBase
{
  Q_OBJECT
  friend class KMail::ImapJob;

public:
  virtual ~KMAcctImap();

  /** A weak assignment operator */
  virtual void pseudoAssign( const KMAccount * a );

  /**
   * Inherited methods.
   */
  virtual QString type(void) const;
  virtual void processNewMail(bool);
  ConnectionState makeConnection();

  /**
   * Kill all jobs related the the specified folder/msg
   */
  virtual void ignoreJobsForMessage( KMMessage * msg );
  virtual void ignoreJobsForFolder( KMFolder * folder );
  virtual void removeSlaveJobsForFolder( KMFolder * folder );

  /**
   * Kill the slave if any jobs are active
   */
  virtual void killAllJobs( bool disconnectSlave=false );

  /**
   * Set the top level pseudo folder
   */
  virtual void setImapFolder(KMFolderImap *);

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

  /**
   * Returns the root folder of this account
   */
  virtual FolderStorage* const rootFolder() const;

  /**
   * Queues a message for automatic filtering
   */
  void execFilters(Q_UINT32 serNum);
    
public slots:
  /**
   * updates the new-mail-check folderlist
   */
  void slotFiltered(Q_UINT32 serNum);
  void slotUpdateFolderList();

protected:
  friend class ::AccountManager;
  KMAcctImap(AccountManager* owner, const QString& accountName, uint id);
  /**
   * Handle an error coming from a KIO job
   * See ImapAccountBase::handleJobError for details.
   */
  virtual bool handleError( int error, const QString &errorMsg, KIO::Job* job, const QString& context, bool abortSync = false );
  virtual void cancelMailCheck();

  QPtrList<KMail::ImapJob> mJobList;
  QGuardedPtr<KMFolderImap> mFolder;

protected slots:
  /** new-mail-notification for the current folder (is called via folderComplete) */
  void postProcessNewMail(KMFolderImap*, bool);
  /**
   * new-mail-notification for not-selected folders (is called via
   * numUnreadMsgsChanged)
   */
  void postProcessNewMail( KMFolder * f );

  /**
   * hooked up to the progress item signaling cancellation.
   * Cleanup and reset state.
   */
  void slotMailCheckCanceled();

  /**
   * called to reset the connection error status
   */
  void slotResetConnectionError();
    
  /**
   * Slots for automatic filtering
   */
  void slotFolderSelected( KMFolderImap*, bool );
  int slotFilterMsg( KMMessage* );

private:
  int mCountRemainChecks;
  /** used to reset connection errors */
  QTimer mErrorTimer;
  QValueList<Q_UINT32> mFilterSerNums;
  QDict<int> mFilterSerNumsToSave;
  KMail::ActionScheduler *mScheduler;
};

#endif /*KMAcctImap_h*/
