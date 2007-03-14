/*
 * Copyright (c) 2004 Carsten Burghardt <burghardt@kde.org>
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
#ifndef LISTJOB_H
#define LISTJOB_H

#include "folderjob.h"
#include "imapaccountbase.h"

class KMFolderImap;
class KMFolderCachedImap;
class KMAcctImap;
class KMAcctCachedImap;
class FolderStorage;
class KUrl;
class KJob; 
namespace KIO {
  class Job;
}

namespace KPIM {
  class ProgressItem;
}

namespace KMail {

/**
 * Generic folder list job for (d)imap accounts
 */
class ListJob : public FolderJob
{
  Q_OBJECT
public:
  /**
   * Create a new job
   * @param storage the parent folder, either provide this or a path
   * @param account the ImapAccountBase
   * @param type Type of subscription
   * @param complete list all folders or only next level
   * @param path the listing path;
   *             if empty the path of the folder will be taken
   * @param item a parent ProgressItem
   */
  ListJob( ImapAccountBase* account, ImapAccountBase::ListType type,
           FolderStorage* storage = 0, const QString& path = QString(),
           bool complete = false, KPIM::ProgressItem* item = 0 );

  virtual ~ListJob();

  /**
   * Set whether the listing should include only folders that the
   * account is subscribed to locally. This is different from the server
   * side subscription managed by the ctor parameter.
   */
  void setHonorLocalSubscription( bool value );
  
  /**
   * Return whether the listing includes only folders that the
   * account is subscribed to locally. This is different from the server
   * side subscription managed by the ctor parameter.
   */
  bool honorLocalSubscription() const;

  virtual void execute();

  /** Path */
  void setPath( const QString& path ) { mPath = path; }

  /** Storage */
  void setStorage( FolderStorage* st ) { mStorage = st; }

  /** Set this to true for a complete listing */
  void setComplete( bool complete ) { mComplete = complete; }

  /** Set parent progress item */
  void setParentProgressItem( KPIM::ProgressItem* it ) { 
    mParentProgressItem = it; }

  /** Set the namespace for this listing */
  void setNamespace( const QString& ns ) { mNamespace = ns; }

protected:
  /**
   * Does the actual KIO::listDir
   */
  void doListing( const KUrl& url, const ImapAccountBase::jobData& jd );

protected slots:
  /**
   * Is called when the listing is done
   * Passes the folders and the jobData to the responding folder
   */
  void slotListResult( KJob* job );

  /**
   * Collects the folder information
   */
  void slotListEntries( KIO::Job* job, const KIO::UDSEntryList& uds );

  /**
   * Called from the account when a connection was established
   */
  void slotConnectionResult( int errorCode, const QString& errorMsg );

signals:
  /**
   * Emitted when new folders have been received
   */
  void receivedFolders( const QStringList&, const QStringList&,
      const QStringList&, const QStringList&, const ImapAccountBase::jobData& );

protected:
  FolderStorage* mStorage;
  ImapAccountBase* mAccount;
  ImapAccountBase::ListType mType;
  bool mComplete;
  bool mHonorLocalSubscription;
  QString mPath;
  QStringList mSubfolderNames, mSubfolderPaths,
              mSubfolderMimeTypes, mSubfolderAttributes;
  KPIM::ProgressItem* mParentProgressItem;
  QString mNamespace;
};

} // namespace

#endif /* LISTJOB_H */

