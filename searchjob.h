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
#ifndef SEARCHJOB_H
#define SEARCHJOB_H

#include <qstringlist.h>
#include "folderjob.h"

class KMFolderImap;
class KMSearchPattern;
class KURL;

namespace KIO {
  class Job;
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
             KMSearchPattern* pattern, Q_UINT32 serNum = 0 );

  virtual ~SearchJob();

  // Execute
  virtual void execute();

protected:
  // searches the complete folder with the pattern
  void searchCompleteFolder();

  // checks a single message with the pattern
  void searchSingleMessage();

  // creates an imap search command
  QString searchStringFromPattern( KMSearchPattern* );

protected slots:
  // called when the folder is complete and ready to be searched
  void slotSearchFolderComplete();

  void slotSearchData( KIO::Job* job, const QString& data );

  // message is downloaded and searched
  void slotSearchMessageArrived( KMMessage* msg );

  // error handling
  void slotSearchResult( KIO::Job *job );

  // imap search result from a single message
  void slotSearchDataSingleMessage( KIO::Job* job, const QString& data );

  // single message was downloaded and is checked
  void slotSearchSingleMessage( KMMessage* msg );

signals:
  void searchDone( QValueList<Q_UINT32>, KMSearchPattern* );

  void searchDone( Q_UINT32, KMSearchPattern* );

protected:
  KMFolderImap* mFolder;
  ImapAccountBase* mAccount;
  KMSearchPattern* mSearchPattern;
  KMSearchPattern* mLocalSearchPattern;
  Q_UINT32 mSerNum;
    // saves the results of the imap search
  QStringList mImapSearchHits;
  // collects the serial numbers from imap and local search
  QValueList<Q_UINT32> mSearchSerNums;
  // the remaining messages that have to be downloaded for local search
  uint mRemainingMsgs;

};

} // namespace

#endif /* SEARCHJOB_H */

