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
  virtual void processNewMail( bool interactive );

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
   * Abort running mail checks
   */
  virtual void cancelMailCheck();

  /**
   * Set the top level pseudo folder
   */
  virtual void setImapFolder(KMFolderCachedImap *);

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
   * Remember that a folder got explicitely deleted
   */
  void addDeletedFolder( const QString& subFolderPath );

  /**
   * Ask if a folder was explicitely deleted in this session
   */
  bool isDeletedFolder( const QString& subFolderPath ) const;

  /**
   * Ask if a folder was explicitely deleted in a previous session
   */
  bool isPreviouslyDeletedFolder( const QString& subFolderPath ) const;

  /**
   * Remove folder from the "deleted folders" list
   */
  void removeDeletedFolder( const QString& subFolderPath );

  /**
   * Add a folder's unread count to the new "unread messages count", done during a sync after getting new mail
   */
  void addUnreadMsgCount( int msgs );

  /**
   * Add a folder's unread count to the last "unread messages count", i.e. the counts before getting new mail
   */
  void addLastUnreadMsgCount( int msgs );

protected:
  friend class KMAcctMgr;
  KMAcctCachedImap(KMAcctMgr* owner, const QString& accountName, uint id);

protected slots:
  /** new-mail-notification for the current folder (is called via folderComplete) */
  void postProcessNewMail(KMFolderCachedImap*, bool);

  virtual void slotCheckQueuedFolders();

private:
  void processNewMail( KMFolderCachedImap* folder, bool interactive, bool recurse );

private:
  QPtrList<CachedImapJob> mJobList;
  KMFolderCachedImap *mFolder;
  bool mProgressDialogEnabled;
  QStringList mDeletedFolders; // folders deleted in this session
  QStringList mPreviouslyDeletedFolders; // folders deleted in a previous session
};

#endif /*KMAcctCachedImap_h*/
