/**
 * kmacctcachedimap.h
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

#ifndef KMAcctCachedImap_h
#define KMAcctCachedImap_h

#include "imapaccountbase.h"

#include <qguardedptr.h>

class KMFolderCachedImap;
class KMFolderTreeItem;
class KMImapJob;
namespace KMail {
  class IMAPProgressDialog;
};
namespace KIO {
  class Job;
};

class KMAcctCachedImap: public KMail::ImapAccountBase
{
  Q_OBJECT
  friend class KMImapJob;
  friend class KMCachedImapJob;

protected: // ### Hacks
  void setPrefixHook();

public:
  typedef KMail::ImapAccountBase base;

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
  virtual void processNewMail(bool);

  struct jobData
  {
    jobData( QString _url = QString::fromLatin1(""), KMFolderCachedImap *_parent = 0,
	     int _total = 1, int _done = 0, bool _quiet = false, bool _inboxOnly = false )
      : url(_url), parent(_parent), total(_total), done(_done), offset(0),
	inboxOnly(_inboxOnly), quiet(_quiet)
      {}
    QString path;
    QString url;
    QByteArray data;
    QCString cdata;
    QStringList items;
    KMFolderCachedImap *parent;
    int total, done, offset;
    bool inboxOnly, quiet;
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

public slots:
  void processNewMail() { processNewMail(TRUE); }

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
  void postProcessNewMail( KMFolder * f ) { base::postProcessNewMail( f ); }

private:
  QPtrList<KMCachedImapJob> mJobList;
  KMFolderCachedImap *mFolder;
  mutable QGuardedPtr<KMail::IMAPProgressDialog> mProgressDlg;
  bool mProgressDialogEnabled;
};

#endif /*KMAcctCachedImap_h*/
