/**
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
#ifndef LISTJOB_H
#define LISTJOB_H

#include "folderjob.h"
#include "imapaccountbase.h"

class KMFolderImap;
class KMFolderCachedImap;
class KMAcctImap;
class KMAcctCachedImap;
class FolderStorage;
class KURL;

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
   * @param secondStep if this is the second listing (when a prefix is set)
   * @param complete list all folders or only next level
   * @param hasInbox if you already have an inbox
   * @param path the listing path;
   *             if empty the path of the folder will be taken
   * @param item a parent ProgressItem
   */
  ListJob( FolderStorage* storage, ImapAccountBase* account,
           ImapAccountBase::ListType type,
           bool secondStep = false, bool complete = false,
           bool hasInbox = false, const QString& path = QString::null,
           KPIM::ProgressItem* item = 0 );

  virtual ~ListJob();

  virtual void execute();

protected:
  /**
   * Does the actual KIO::listDir
   */
  void doListing( const KURL& url, const ImapAccountBase::jobData& jd );

protected slots:
  /**
   * Is called when the listing is done
   * Passes the folders and the jobData to the responding folder
   */
  void slotListResult( KIO::Job* job );

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
  bool mHasInbox;
  bool mSecondStep;
  bool mComplete;
  QString mPath;
  QStringList mSubfolderNames, mSubfolderPaths,
              mSubfolderMimeTypes, mSubfolderAttributes;
  KPIM::ProgressItem* mParentProgressItem;
};

} // namespace

#endif /* LISTJOB_H */

