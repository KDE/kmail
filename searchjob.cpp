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

#include "searchjob.h"
#include "kmfolderimap.h"
#include "imapaccountbase.h"
#include "kmsearchpattern.h"
#include "kmfolder.h"
#include "imapjob.h"
#include "kmmsgdict.h"

#include <kdebug.h>
#include <kurl.h>
#include <kio/scheduler.h>
#include <kio/job.h>
#include <kio/global.h>
#include <klocale.h>
#include <kmessagebox.h>

using namespace KMail;

SearchJob::SearchJob( KMFolderImap* folder, ImapAccountBase* account,
    KMSearchPattern* pattern, Q_UINT32 serNum )
 : FolderJob( 0, tOther, (folder ? folder->folder() : 0) ),
   mFolder( folder ), mAccount( account ), mSearchPattern( pattern ),
   mSerNum( serNum ), mRemainingMsgs( 0 )
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
  KIO::Scheduler::assignJobToSlave(mAccount->slave(), job);
  connect( job, SIGNAL(infoMessage(KIO::Job*,const QString&)),
      SLOT(slotSearchData(KIO::Job*,const QString&)) );
  connect( job, SIGNAL(result(KIO::Job *)),
      SLOT(slotSearchResult(KIO::Job *)) );
}

//-----------------------------------------------------------------------------
QString SearchJob::searchStringFromPattern( KMSearchPattern* pattern )
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
    if ( (*it)->function() == KMSearchRule::FuncContainsNot )
      result = "NOT ";
    else if ( (*it)->function() == KMSearchRule::FuncIsGreater &&
              (*it)->field() == "<size>" )
      result = "LARGER ";
    else if ( (*it)->function() == KMSearchRule::FuncIsLess &&
              (*it)->field() == "<size>" )
      result = "SMALLER ";
    else if ( (*it)->function() != KMSearchRule::FuncContains )
      accept = false;

    if ( (*it)->field() == "<message>" )
      result += "TEXT \"" + (*it)->contents() + "\"";
    else if ( (*it)->field() == "<body>" )
      result += "BODY \"" + (*it)->contents() + "\"";
    else if ( (*it)->field() == "<recipients>" ) 
      result += " (OR HEADER To \"" + (*it)->contents() + "\" HEADER Cc \"" +
        (*it)->contents() + "\" HEADER Bcc \"" + (*it)->contents() + "\")";
    else if ( (*it)->field() == "<size>" )
      result += (*it)->contents();
    else if ( (*it)->field() == "<age in days>" ||
              (*it)->field() == "<status>" ||
              (*it)->field() == "<any header>" )
      accept = false;
    else
      result += "HEADER "+ field + " \"" + (*it)->contents() + "\"";

    if ( result.isEmpty() )
      accept = false;

    if ( accept )
      parts += result;
    else
      mLocalSearchPattern->append( *it );
  }
  
  QString search;
  if ( pattern->op() == KMSearchPattern::OpOr )
    search = "(OR " + parts.join(" ") + ")";
  else
    search = parts.join(" ");

  kdDebug(5006) << k_funcinfo << search << ";localSearch=" << mLocalSearchPattern->asString() << endl;
  return search;
}

//-----------------------------------------------------------------------------
void SearchJob::slotSearchData( KIO::Job* job, const QString& data )
{
  if ( job && job->error() )
   return; 

  if ( mLocalSearchPattern->isEmpty() && data.isEmpty() )
  {
    // no local search and the server found nothing
    QValueList<Q_UINT32> serNums;
    emit searchDone( serNums, mSearchPattern );
  } else
  {
    // remember the uids the server found
    mImapSearchHits = QStringList::split( " ", data );

    // get the folder to make sure we have all messages
    connect ( mFolder, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
        this, SLOT( slotSearchFolderComplete()) );
    mFolder->getFolder();
  }
}

//-----------------------------------------------------------------------------
void SearchJob::slotSearchFolderComplete()
{  
  disconnect ( mFolder, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
            this, SLOT( slotSearchFolderComplete()) );

  if ( mLocalSearchPattern->isEmpty() ) {
    // search for the serial number of the UIDs
    QValueList<Q_UINT32> serNums;
    for ( int i = 0; i < mFolder->count(); ++i ) {
      KMMsgBase * base = mFolder->getMsgBase( i );
      if ( mImapSearchHits.find( QString::number( base->UID() ) ) != mImapSearchHits.end() ) {
        Q_UINT32 serNum = kmkernel->msgDict()->getMsgSerNum( mFolder->folder(), i );
        serNums.append( serNum );
      }
    }
    emit searchDone( serNums, mSearchPattern );
  } else {
    // we have search patterns that can not be handled by the server
    mRemainingMsgs = mFolder->count();
    if ( mRemainingMsgs == 0 ) {
      emit searchDone( mSearchSerNums, mSearchPattern );
      return;
    }

    // Let's see if all we need is status, that we can do locally. Optimization.
    bool needToDownload = false;
    for ( QPtrListIterator<KMSearchRule> it( *mLocalSearchPattern ) ; it.current() ; ++it ) {
      if ( (*it)->field() != "<status>" ) {
        needToDownload = true;
        break;
      }
    }

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
        emit searchDone( serNums, mSearchPattern );
        return;
      }
    }
    unsigned int numMsgs = mRemainingMsgs;
    for ( unsigned int i = 0; i < numMsgs ; ++i ) {
      KMMessage * msg = mFolder->getMsg( i );
      if ( needToDownload ) {
        ImapJob *job = new ImapJob( msg );
        job->setParentFolder( mFolder );
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
  --mRemainingMsgs;
  if ( msg ) { // messageRetrieved(0) is always possible
    if ( mLocalSearchPattern->op() == KMSearchPattern::OpAnd ) {
      // imap and local search have to match
      if ( mLocalSearchPattern->matches( msg ) &&
          ( mImapSearchHits.isEmpty() ||
           mImapSearchHits.find( QString::number(msg->UID() ) ) != mImapSearchHits.end() ) ) {
        Q_UINT32 serNum = msg->getMsgSerNum();
        mSearchSerNums.append( serNum );
      }
    } else if ( mLocalSearchPattern->op() == KMSearchPattern::OpOr ) {
      // imap or local search have to match
      if ( mLocalSearchPattern->matches( msg ) ||
          mImapSearchHits.find( QString::number(msg->UID()) ) != mImapSearchHits.end() ) {
        Q_UINT32 serNum = msg->getMsgSerNum();
        mSearchSerNums.append( serNum );
      }
    }
    int idx = -1;
    KMFolder * p = 0;
    kmkernel->msgDict()->getLocation( msg, &p, &idx );
    if ( idx != -1 )
      mFolder->unGetMsg( idx );
  }
  if ( mRemainingMsgs == 0 ) {
    emit searchDone( mSearchSerNums, mSearchPattern );
  }
}

//-----------------------------------------------------------------------------
void SearchJob::slotSearchResult( KIO::Job *job )
{
  if ( job->error() )
  {
    mAccount->handleJobError( job, i18n("Error while searching.") );
    if ( mSearchSerNums.empty() )
    {
      QValueList<Q_UINT32> serNums;
      emit searchDone( serNums, mSearchPattern );
    } else
      emit searchDone( 0, mSearchPattern );
  }
}

//-----------------------------------------------------------------------------
void SearchJob::searchSingleMessage()
{
  QString searchString = searchStringFromPattern( mSearchPattern );
  if ( searchString.isEmpty() )
  { 
    // download the message and search local
    int idx = -1;
    KMFolder *aFolder = 0;
    kmkernel->msgDict()->getLocation( mSerNum, &aFolder, &idx );

    KMMessage * msg = mFolder->getMsg( idx );
    ImapJob *job = new ImapJob( msg );
    job->setParentFolder( mFolder );
    connect( job, SIGNAL(messageRetrieved(KMMessage*)),
        this, SLOT(slotSearchSingleMessage(KMMessage*)) );
    job->start();
  } else
  {
    // imap search
    int idx = -1;
    KMFolder *aFolder = 0;
    kmkernel->msgDict()->getLocation( mSerNum, &aFolder, &idx );
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
  if ( job && job->error() )
   return;

  if ( !data.isEmpty() )
    emit searchDone( mSerNum, mSearchPattern );
  else
    emit searchDone( 0, mSearchPattern );
}
 
//-----------------------------------------------------------------------------
void SearchJob::slotSearchSingleMessage( KMMessage* msg )
{
  if ( msg && mLocalSearchPattern->matches( msg ) )
    emit searchDone( mSerNum, mSearchPattern );
  else
    emit searchDone( 0, mSearchPattern );
}

#include "searchjob.moc"
