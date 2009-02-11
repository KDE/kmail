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
#ifndef SEARCHJOB_H
#define SEARCHJOB_H

#include "folderjob.h"

#include <QStringList>
#include <QList>

class KMFolderImap;
class KMSearchPattern;
class KJob;
namespace KIO {
  class Job;
}

namespace KPIM {
  class ProgressItem;
}

namespace KMail {

class ImapAccountBase;

/**
 * Search job
 */
class SearchJob : public FolderJob
{
  Q_OBJECT
public:
  /**
   * Creates a new job
   * @param folder the folder that should be searched
   * @param account the ImapAccountBase of the folder
   * @param pattern the search pattern
   * @param serNum if you specify the serNum only this is checked
   */
  SearchJob( KMFolderImap* folder, ImapAccountBase* account,
             const KMSearchPattern* pattern, quint32 serNum = 0 );

  virtual ~SearchJob();

  // Execute
  virtual void execute();

protected:
  // searches the complete folder with the pattern
  void searchCompleteFolder();

  // checks a single message with the pattern
  void searchSingleMessage();

  // creates an imap search command
  QString searchStringFromPattern( const KMSearchPattern* );

  // returns true if all uids can be mapped to sernums
  bool canMapAllUIDs();

  // if we need to download messages
  bool needsDownload();

protected slots:
  // search the folder
  // is called when all uids can be mapped to sernums
  void slotSearchFolder();

  // processes the server answer
  void slotSearchData( KJob* job, const QString& data, const QString& );

  // message is downloaded and searched
  void slotSearchMessageArrived( KMMessage* msg );

  // error handling for all cases
  void slotSearchResult( KJob *job );

  // imap search result from a single message
  void slotSearchDataSingleMessage( KJob* job, const QString& data, const QString& );

  // the user canceled the search progress
  void slotAbortSearch( KPIM::ProgressItem* item );

signals:
  // emitted when a list of matching serial numbers was found
  void searchDone( QList<quint32>, const KMSearchPattern*, bool complete );

  // emitted when a single message (identified by the serial number) was checked
  void searchDone( quint32, const KMSearchPattern*, bool matches );

protected:
  KMFolderImap* mFolder;
  ImapAccountBase* mAccount;
  const KMSearchPattern* mSearchPattern;
  KMSearchPattern* mLocalSearchPattern;
  quint32 mSerNum;
    // saves the results of the imap search
  QStringList mImapSearchHits;
  // collects the serial numbers from imap and local search
  QList<quint32> mSearchSerNums;
  // the remaining messages that have to be downloaded for local search
  uint mRemainingMsgs;
  // progress item for local searches
  KPIM::ProgressItem *mProgress;
  bool mUngetCurrentMsg;

};

} // namespace

#endif /* SEARCHJOB_H */

