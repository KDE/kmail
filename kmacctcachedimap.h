/**
 *  kmacctcachedimap.h
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
  void listDirectory(QString path, ListType subscription,
      bool secondStep = FALSE, KMFolder* parent = NULL, bool reset = false);

  /**
   * Starts the folderlisting for the root folder
   */
  virtual void listDirectory();

  /**
   * Handle an error coming from a KIO job
   * See ImapAccountBase::handleJobError for details.
   */
  virtual bool handleJobError( int error, const QString &errorMsg, KIO::Job* job, const QString& context, bool abortSync = false );

public slots:
  void processNewMail() { processNewMail( mFolder, true ); }

protected:
  friend class KMAcctMgr;
  KMAcctCachedImap(KMAcctMgr* owner, const QString& accountName, uint id);

protected slots:

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
};

#endif /*KMAcctCachedImap_h*/
