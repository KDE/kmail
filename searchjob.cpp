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

#include "searchjob.h"
#include "kmfolderimap.h"
#include "imapaccountbase.h"
#include "kmsearchpattern.h"
#include "kmfolder.h"
#include "imapjob.h"
#include "kmmsgdict.h"

#include <progressmanager.h>
using KPIM::ProgressItem;
using KPIM::ProgressManager;

#include <kdebug.h>
#include <kurl.h>
#include <kio/scheduler.h>
#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qstylesheet.h>

namespace KMail {

SearchJob::SearchJob( KMFolderImap* folder, ImapAccountBase* account,
                      const KMSearchPattern* pattern, Q_UINT32 serNum )
 : FolderJob( 0, tOther, (folder ? folder->folder() : 0) ),
   mFolder( folder ), mAccount( account ), mSearchPattern( pattern ),
   mSerNum( serNum ), mRemainingMsgs( 0 ), mProgress( 0 ),
   mUngetCurrentMsg( false )
{
}

SearchJob::~SearchJob()
{
}

void SearchJob::execute()
{
  if ( mSerNum == 0 )
  {
    searchCompleteFolder();
  } else {
    searchSingleMessage();
  }
}

//-----------------------------------------------------------------------------
void SearchJob::searchCompleteFolder()
{
  // generate imap search command and save local search patterns
  QString searchString = searchStringFromPattern( mSearchPattern );

  if ( searchString.isEmpty() ) // skip imap search and download the messages
    return slotSearchData( 0, QString::null );

  // do the IMAP search  
  KURL url = mAccount->getUrl();
  url.setPath( mFolder->imapPath() + ";SECTION=" + searchString );
  QByteArray packedArgs;
  QDataStream stream( packedArgs, IO_WriteOnly );
  stream << (int) 'E' << url;
  KIO::SimpleJob *job = KIO::special( url, packedArgs, false );
  if ( mFolder->imapPath() != QString( "/" ) )
  {
    KIO::Scheduler::assignJobToSlave( mAccount->slave(), job );
    connect( job, SIGNAL( infoMessage( KIO::Job*, const QString& ) ),
      SLOT( slotSearchData( KIO::Job*, const QString& ) ) );
    connect( job, SIGNAL( result( KIO::Job * ) ),
      SLOT( slotSearchResult( KIO::Job * ) ) );
  }
  else
  { // for the "/ folder" of an imap account, searching blocks the kioslave
    slotSearchData( job, QString() );
    slotSearchResult( job );
  }
}

//-----------------------------------------------------------------------------
QString SearchJob::searchStringFromPattern( const KMSearchPattern* pattern )
{
  QStringList parts;
  // this is for the search pattern that can only be done local
  mLocalSearchPattern = new KMSearchPattern();
  mLocalSearchPattern->setOp( pattern->op() );

  for ( QPtrListIterator<KMSearchRule> it( *pattern ) ; it.current() ; ++it )
  {
    // construct an imap search command
    bool accept = true;
    QString result;
    QString field = (*it)->field();
    // check if the operation is supported
    if ( (*it)->function() == KMSearchRule::FuncContainsNot ) {
      result = "NOT ";
    } else if ( (*it)->function() == KMSearchRule::FuncIsGreater &&
              (*it)->field() == "<size>" ) {
      result = "LARGER ";
    } else if ( (*it)->function() == KMSearchRule::FuncIsLess &&
              (*it)->field() == "<size>" ) {
      result = "SMALLER ";
    } else if ( (*it)->function() != KMSearchRule::FuncContains ) {
      // can't be handled by imap
      accept = false;
    }

    // now see what should be searched
    if ( (*it)->field() == "<message>" ) {
      result += "TEXT \"" + (*it)->contents() + "\"";
    } else if ( (*it)->field() == "<body>" ) {
      result += "BODY \"" + (*it)->contents() + "\"";
    } else if ( (*it)->field() == "<recipients>" ) {
      result += " (OR HEADER To \"" + (*it)->contents() + "\" HEADER Cc \"" +
        (*it)->contents() + "\" HEADER Bcc \"" + (*it)->contents() + "\")";
    } else if ( (*it)->field() == "<size>" ) {
      result += (*it)->contents();
    } else if ( (*it)->field() == "<age in days>" ||
              (*it)->field() == "<status>" ||
              (*it)->field() == "<any header>" ) {
      accept = false;
    } else {
      result += "HEADER "+ field + " \"" + (*it)->contents() + "\"";
    }

    if ( result.isEmpty() ) {
      accept = false;
    }

    if ( accept ) {
      parts += result;
    } else {
      mLocalSearchPattern->append( *it );
    }
  }
  
  QString search;
  if ( !parts.isEmpty() ) {
    if ( pattern->op() == KMSearchPattern::OpOr && parts.size() > 1 ) {
      search = "(OR " + parts.join(" ") + ")";
    } else {
      // and's are simply joined
      search = parts.join(" ");
    }
  }

  kdDebug(5006) << k_funcinfo << search << ";localSearch=" << mLocalSearchPattern->asString() << endl;
  return search;
}

//-----------------------------------------------------------------------------
void SearchJob::slotSearchData( KIO::Job* job, const QString& data )
{
  if ( job && job->error() ) {
    // error is handled in slotSearchResult
    return; 
  }

  if ( mLocalSearchPattern->isEmpty() && data.isEmpty() )
  {
    // no local search and the server found nothing
    QValueList<Q_UINT32> serNums;
    emit searchDone( serNums, mSearchPattern, true );
  } else
  {
    // remember the uids the server found
    mImapSearchHits = QStringList::split( " ", data );

    if ( canMapAllUIDs() ) 
    {
      slotSearchFolder();
    } else
    {
      // get the folder to make sure we have all messages
      connect ( mFolder, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
          this, SLOT( slotSearchFolder()) );
      mFolder->getFolder();
    }
  }
}

//-----------------------------------------------------------------------------
bool SearchJob::canMapAllUIDs()
{
  for ( QStringList::Iterator it = mImapSearchHits.begin(); 
        it != mImapSearchHits.end(); ++it ) 
  {
    if ( mFolder->serNumForUID( (*it).toULong() ) == 0 )
      return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void SearchJob::slotSearchFolder()
{  
  disconnect ( mFolder, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
            this, SLOT( slotSearchFolder()) );

  if ( mLocalSearchPattern->isEmpty() ) {
    // pure imap search - now get the serial number for the UIDs
    QValueList<Q_UINT32> serNums;
    for ( QStringList::Iterator it = mImapSearchHits.begin(); 
        it != mImapSearchHits.end(); ++it ) 
    {
      ulong serNum = mFolder->serNumForUID( (*it).toULong() );
      // we need to check that the local folder does contain a message for this UID. 
      // scenario: server responds with a list of UIDs.  While the search was running, filtering or bad juju moved a message locally
      // serNumForUID will happily return 0 for the missing message, and KMFolderSearch::addSerNum() will fail its assertion.
      if ( serNum != 0 ) 
        serNums.append( serNum );
    }
    emit searchDone( serNums, mSearchPattern, true );
  } else {
    // we have search patterns that can not be handled by the server
    mRemainingMsgs = mFolder->count();
    if ( mRemainingMsgs == 0 ) {
      emit searchDone( mSearchSerNums, mSearchPattern, true );
      return;
    }

    // Let's see if all we need is status, that we can do locally. Optimization.
    bool needToDownload = needsDownload();
    if ( needToDownload ) {
      // so we need to download all messages and check
      QString question = i18n("To execute your search all messages of the folder %1 "
          "have to be downloaded from the server. This may take some time. "
          "Do you want to continue your search?").arg( mFolder->label() );
      if ( KMessageBox::warningContinueCancel( 0, question,
            i18n("Continue Search"), i18n("&Search"), 
            "continuedownloadingforsearch" ) != KMessageBox::Continue ) 
      {
        QValueList<Q_UINT32> serNums;
        emit searchDone( serNums, mSearchPattern, true );
        return;
      }
    }
    unsigned int numMsgs = mRemainingMsgs;
    // progress
    mProgress = ProgressManager::createProgressItem(
        "ImapSearchDownload" + ProgressManager::getUniqueID(),
        i18n("Downloading emails from IMAP server"),
        i18n( "URL: %1" ).arg( QStyleSheet::escape( mFolder->folder()->prettyURL() ) ),
        true,
        mAccount->useSSL() || mAccount->useTLS() );
    mProgress->setTotalItems( numMsgs );
    connect ( mProgress, SIGNAL( progressItemCanceled( KPIM::ProgressItem*)),
        this, SLOT( slotAbortSearch( KPIM::ProgressItem* ) ) );

    for ( unsigned int i = 0; i < numMsgs ; ++i ) {
      KMMessage * msg = mFolder->getMsg( i );
      if ( needToDownload ) {
        ImapJob *job = new ImapJob( msg );
        job->setParentFolder( mFolder );
        job->setParentProgressItem( mProgress );
        connect( job, SIGNAL(messageRetrieved(KMMessage*)),
            this, SLOT(slotSearchMessageArrived(KMMessage*)) );
        job->start();
      } else {
        slotSearchMessageArrived( msg );
      }
    }
  }
}

//-----------------------------------------------------------------------------
void SearchJob::slotSearchMessageArrived( KMMessage* msg )
{
  if ( mProgress )
  {
    mProgress->incCompletedItems();
    mProgress->updateProgress();
  }
  --mRemainingMsgs;
  bool matches = false;
  if ( msg ) { // messageRetrieved(0) is always possible
    if ( mLocalSearchPattern->op() == KMSearchPattern::OpAnd ) {
      // imap and local search have to match
      if ( mLocalSearchPattern->matches( msg ) &&
          ( mImapSearchHits.isEmpty() ||
           mImapSearchHits.find( QString::number(msg->UID() ) ) != mImapSearchHits.end() ) ) {
        Q_UINT32 serNum = msg->getMsgSerNum();
        mSearchSerNums.append( serNum );
        matches = true;
      }
    } else if ( mLocalSearchPattern->op() == KMSearchPattern::OpOr ) {
      // imap or local search have to match
      if ( mLocalSearchPattern->matches( msg ) ||
          mImapSearchHits.find( QString::number(msg->UID()) ) != mImapSearchHits.end() ) {
        Q_UINT32 serNum = msg->getMsgSerNum();
        mSearchSerNums.append( serNum );
        matches = true;
      }
    }
    int idx = -1;
    KMFolder * p = 0;
    KMMsgDict::instance()->getLocation( msg, &p, &idx );
    if ( idx != -1 && mUngetCurrentMsg )
      mFolder->unGetMsg( idx );
  }
  if ( mSerNum > 0 )
  {
    emit searchDone( mSerNum, mSearchPattern, matches );
  } else {
    bool complete = ( mRemainingMsgs == 0 );
    if ( complete && mProgress )
    {
      mProgress->setComplete();
      mProgress = 0;
    }
    if ( matches || complete )
    {
      emit searchDone( mSearchSerNums, mSearchPattern, complete );
      mSearchSerNums.clear();
    }
  }
}

//-----------------------------------------------------------------------------
void SearchJob::slotSearchResult( KIO::Job *job )
{
  if ( job->error() )
  {
    mAccount->handleJobError( job, i18n("Error while searching.") );
    if ( mSerNum == 0 )
    {
      // folder
      QValueList<Q_UINT32> serNums;
      emit searchDone( serNums, mSearchPattern, true );
    } else {
      // message
      emit searchDone( mSerNum, mSearchPattern, false );
    }
  }
}

//-----------------------------------------------------------------------------
void SearchJob::searchSingleMessage()
{
  QString searchString = searchStringFromPattern( mSearchPattern );
  if ( searchString.isEmpty() )
  {
    // no imap search
    slotSearchDataSingleMessage( 0, QString::null );
  } else
  {
    // imap search
    int idx = -1;
    KMFolder *aFolder = 0;
    KMMsgDict::instance()->getLocation( mSerNum, &aFolder, &idx );
    assert(aFolder && (idx != -1));
    KMMsgBase *mb = mFolder->getMsgBase( idx );

    // only search for that UID
    searchString += " UID " + QString::number( mb->UID() );
    KURL url = mAccount->getUrl();
    url.setPath( mFolder->imapPath() + ";SECTION=" + searchString );
    QByteArray packedArgs;
    QDataStream stream( packedArgs, IO_WriteOnly );
    stream << (int) 'E' << url;
    KIO::SimpleJob *job = KIO::special( url, packedArgs, false );
    KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
    connect( job, SIGNAL(infoMessage(KIO::Job*,const QString&)),
        SLOT(slotSearchDataSingleMessage(KIO::Job*,const QString&)) );
    connect( job, SIGNAL(result(KIO::Job *)),
        SLOT(slotSearchResult(KIO::Job *)) );
  }
}

//-----------------------------------------------------------------------------
void SearchJob::slotSearchDataSingleMessage( KIO::Job* job, const QString& data )
{
  if ( job && job->error() ) {
    // error is handled in slotSearchResult
    return;
  }

  if ( mLocalSearchPattern->isEmpty() ) {
    // we are done
    emit searchDone( mSerNum, mSearchPattern, !data.isEmpty() );
    return;
  }
  // remember what the server found
  mImapSearchHits = QStringList::split( " ", data );

  // add the local search
  int idx = -1;
  KMFolder *aFolder = 0;
  KMMsgDict::instance()->getLocation( mSerNum, &aFolder, &idx );
  assert(aFolder && (idx != -1));
  mUngetCurrentMsg = !mFolder->getMsgBase( idx )->isMessage();
  KMMessage * msg = mFolder->getMsg( idx );
  if ( needsDownload() ) {
    ImapJob *job = new ImapJob( msg );
    job->setParentFolder( mFolder );
    connect( job, SIGNAL(messageRetrieved(KMMessage*)),
        this, SLOT(slotSearchMessageArrived(KMMessage*)) );
    job->start();
  } else {
    slotSearchMessageArrived( msg );
  }
}
 
//-----------------------------------------------------------------------------
void SearchJob::slotAbortSearch( KPIM::ProgressItem* item )
{
  if ( item )
    item->setComplete();
  mAccount->killAllJobs();
  QValueList<Q_UINT32> serNums;
  emit searchDone( serNums, mSearchPattern, true );
}

//-----------------------------------------------------------------------------
bool SearchJob::needsDownload()
{
  for ( QPtrListIterator<KMSearchRule> it( *mLocalSearchPattern ) ; it.current() ; ++it ) {
    if ( (*it)->field() != "<status>" ) {
      return true;
    }
  }
  return false;
}

} // namespace KMail

#include "searchjob.moc"
