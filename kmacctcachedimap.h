/**
 *  kmacctcachedimap.h
 *
 *  Copyright (c) 2002-2004 Bo Thorsen <bo@sonofthor.dk>
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
class KMFolder;
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
  KMFolderCachedImap* imapFolder() const { return mFolder; }

  virtual void readConfig( /*const*/ KConfig/*Base*/ & config );
  virtual void writeConfig( KConfig/*Base*/ & config ) /*const*/;

  /**
   * Invalidate the local cache.
   */
  virtual void invalidateIMAPFolders();
  virtual void invalidateIMAPFolders( KMFolderCachedImap* );

  /**
   * Remember that a folder got explicitely deleted - including all child folders
   */
  void addDeletedFolder( KMFolder* folder );

  /**
   * Remember that a folder got explicitely deleted - NOT including all child folders
   * This is used when renaming a folder.
   */
  void addDeletedFolder( const QString& imapPath );

  /**
   * Ask if a folder was explicitely deleted in this session
   */
  bool isDeletedFolder( const QString& subFolderPath ) const;

  /**
   * Ask if a folder was explicitely deleted in a previous session
   */
  bool isPreviouslyDeletedFolder( const QString& subFolderPath ) const;

  /**
   * return the imap path to the deleted folder, as well as the paths for any child folders
   */
  QStringList deletedFolderPaths( const QString& subFolderPath ) const;

  /**
   * Remove folder from the "deleted folders" list
   */
  void removeDeletedFolder( const QString& subFolderPath );

  /**
   * Remember that a folder was renamed
   */
  void addRenamedFolder( const QString& subFolderPath,
                         const QString& oldLabel, const QString& newName );

  /**
   * Remove folder from "renamed folders" list
   * Warning: @p subFolderPath is the OLD path
   */
  void removeRenamedFolder( const QString& subFolderPath );

  struct RenamedFolder {
    RenamedFolder() {} // for QMap
    RenamedFolder( const QString& oldLabel, const QString& newName )
      : mOldLabel( oldLabel ), mNewName( newName ) {}
    QString mOldLabel;
    QString mNewName;
  };

  /**
   * Returns new name for folder if it was renamed
   */
  QString renamedFolder( const QString& imapPath ) const;
  /**
   * Returns the list of folders that were renamed
   */
  const QMap<QString, RenamedFolder>& renamedFolders() const { return mRenamedFolders; }

  /**
   * Add a folder's unread count to the new "unread messages count", done during a sync after getting new mail
   */
  void addUnreadMsgCount( const KMFolderCachedImap *folder, int countUnread );

  /**
   * Add a folder's unread count to the last "unread messages count", i.e. the counts before getting new mail
   */
  void addLastUnreadMsgCount( const KMFolderCachedImap *folder,
                              int countLastUnread );

  /**
   * Returns the root folder of this account
   */
  virtual FolderStorage* const rootFolder() const;
  
  /** return if the account passed the annotation test  */
  bool annotationCheckPassed(){ return mAnnotationCheckPassed;};
  void setAnnotationCheckPassed( bool a ){ mAnnotationCheckPassed = a; };

protected:
  friend class KMAcctMgr;
  KMAcctCachedImap(KMAcctMgr* owner, const QString& accountName, uint id);

protected slots:
  /** new-mail-notification for the current folder (is called via folderComplete) */
  void postProcessNewMail(KMFolderCachedImap*, bool);

  void slotProgressItemCanceled( KPIM::ProgressItem* );

  virtual void slotCheckQueuedFolders();

private:
  QValueList<KMFolderCachedImap*> killAllJobsInternal( bool disconnectSlave );
  void processNewMail( KMFolderCachedImap* folder, bool interactive, bool recurse );

private:
  QPtrList<CachedImapJob> mJobList;
  KMFolderCachedImap *mFolder;
  QStringList mDeletedFolders; // folders deleted in this session
  QStringList mPreviouslyDeletedFolders; // folders deleted in a previous session
  QMap<QString, RenamedFolder> mRenamedFolders;
  bool mAnnotationCheckPassed;
};

#endif /*KMAcctCachedImap_h*/
