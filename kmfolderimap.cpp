/**
 * kmfolderimap.cpp
 *
 * Copyright (c) 2001 Kurt Granroth <granroth@kde.org>
 * Copyright (c) 2000-2002 Michael Haeckel <haeckel@kde.org>
 *
 * This file is based on kmacctimap.coo by Michael Haeckel which was
 * based on popaccount.cpp by Don Sanders
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
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfolder.h"
#include "kmfolderimap.h"
#include "kmfoldermbox.h"
#include "kmfoldertree.h"
#include "kmmsgdict.h"
#include "undostack.h"
#include "kmfoldermgr.h"
#include "kmfiltermgr.h"
#include "kmmsgdict.h"
#include "imapaccountbase.h"
using KMail::ImapAccountBase;
#include "imapjob.h"
using KMail::ImapJob;
#include "attachmentstrategy.h"
using KMail::AttachmentStrategy;
#include "progressmanager.h"
using KPIM::ProgressItem;
using KPIM::ProgressManager;
#include "listjob.h"
using KMail::ListJob;
#include "kmsearchpattern.h"
#include "searchjob.h"
using KMail::SearchJob;
#include "renamejob.h"
using KMail::RenameJob;

#include <kdebug.h>
#include <kio/scheduler.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kconfiggroup.h>

#include <QBuffer>
#include <QList>
#include <QTextCodec>
#include <QByteArray>
//Added by qt3to4:
#include <Q3StyleSheet>

#include <assert.h>

KMFolderImap::KMFolderImap(KMFolder* folder, const char* aName)
  : KMFolderMbox(folder, aName)
{
  mContentState = imapNoInformation;
  mSubfolderState = imapNoInformation;
  mAccount = 0;
  mIsSelected = false;
  mLastUid = 0;
  mCheckFlags = true;
  mCheckMail = true;
  mCheckingValidity = false;
  mUserRights = 0;
  mAlreadyRemoved = false;
  mHasChildren = ChildrenUnknown;
  mMailCheckProgressItem = 0;
  mListDirProgressItem = 0;
  mAddMessageProgressItem = 0;
  mReadOnly = false;

  connect (this, SIGNAL( folderComplete( KMFolderImap*, bool ) ),
           this, SLOT( slotCompleteMailCheckProgress()) );
}

KMFolderImap::~KMFolderImap()
{
  if (mAccount) {
    mAccount->removeSlaveJobsForFolder( folder() );
    /* Now that we've removed ourselves from the accounts jobs map, kill all
       ongoing operations and reset mailcheck if we were deleted during an
       ongoing mailcheck of our account. Not very gracefull, but safe, and the
       only way I can see to reset the account state cleanly. */
    if ( mAccount->checkingMail( folder() ) ) {
       mAccount->killAllJobs();
    }
  }
  writeConfig();
  if (kmkernel->undoStack()) kmkernel->undoStack()->folderDestroyed( folder() );
  mMetaDataMap.setAutoDelete( true );
  mMetaDataMap.clear();
  mUidMetaDataMap.setAutoDelete( true );
  mUidMetaDataMap.clear();
}


//-----------------------------------------------------------------------------
void KMFolderImap::close(bool aForced)
{
  if (mOpenCount <= 0 ) return;
  if (mOpenCount > 0) mOpenCount--;
  if (mOpenCount > 0 && !aForced) return;
  if (isSelected() && !aForced) {
      kWarning(5006) << "Trying to close the selected folder " << label() <<
          " - ignoring!" << endl;
      return;
  }
  // FIXME is this still needed?
  if (account())
    account()->ignoreJobsForFolder( folder() );
  int idx = count();
  while (--idx >= 0) {
    if ( mMsgList[idx]->isMessage() ) {
      KMMessage *msg = static_cast<KMMessage*>(mMsgList[idx]);
      if (msg->transferInProgress())
          msg->setTransferInProgress( false );
    }
  }
  // The inherited close will decrement again, so we have to adjust.
  mOpenCount++;
  KMFolderMbox::close(aForced);
}

KMFolder* KMFolderImap::trashFolder() const
{
  QString trashStr = account()->trash();
  return kmkernel->imapFolderMgr()->findIdString( trashStr );
}

//-----------------------------------------------------------------------------
KMMessage* KMFolderImap::getMsg(int idx)
{
  if(!(idx >= 0 && idx <= count()))
    return 0;

  KMMsgBase* mb = getMsgBase(idx);
  if (!mb) return 0;
  if (mb->isMessage())
  {
    return ((KMMessage*)mb);
  } else {
    KMMessage* msg = FolderStorage::getMsg( idx );
    if ( msg ) // set it incomplete as the msg was not transferred from the server
      msg->setComplete( false );
    return msg;
  }
}

//-----------------------------------------------------------------------------
KMAcctImap* KMFolderImap::account() const
{
  if ( !mAccount ) {
    KMFolderDir *parentFolderDir = dynamic_cast<KMFolderDir*>( folder()->parent() );
    if ( !parentFolderDir ) {
      kWarning() << k_funcinfo << "No parent folder dir found for " << name() << endl;
      return 0;
    }
    KMFolder *parentFolder = parentFolderDir->owner();
    if ( !parentFolder ) {
      kWarning() << k_funcinfo << "No parent folder found for " << name() << endl;
      return 0;
    }
    KMFolderImap *parentStorage = dynamic_cast<KMFolderImap*>( parentFolder->storage() );
    if ( parentStorage )
      mAccount = parentStorage->account();
  }
  return mAccount;
}

void KMFolderImap::setAccount(KMAcctImap *aAccount)
{
  mAccount = aAccount;
  if( !folder() || !folder()->child() ) return;
  KMFolderNode* node;
  QList<KMFolderNode*>::const_iterator it;
  for ( it = folder()->child()->begin();
      ( node = *it ) && it != folder()->child()->end(); ++it )
  {
    if (!(*it)->isDir())
      static_cast<KMFolderImap*>(static_cast<KMFolder*>(*it)->storage())->setAccount(aAccount);
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::readConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroup group(config, "Folder-" + folder()->idString());
  mCheckMail = group.readEntry( "checkmail", true );

  mUidValidity = group.readEntry("UidValidity");
  if ( mImapPath.isEmpty() ) {
    setImapPath( group.readEntry("ImapPath") );
  }
  if (QString(objectName()).toUpper() == "INBOX" && mImapPath == "/INBOX/")
  {
    folder()->setSystemFolder( true );
    folder()->setLabel( i18n("inbox") );
  }
  mNoContent = group.readEntry( "NoContent", false );
  mReadOnly = group.readEntry( "ReadOnly", false );

  KMFolderMbox::readConfig();
}

//-----------------------------------------------------------------------------
void KMFolderImap::writeConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroup group(config, "Folder-" + folder()->idString());
  group.writeEntry("checkmail", mCheckMail);
  group.writeEntry("UidValidity", mUidValidity);
  group.writeEntry("ImapPath", mImapPath);
  group.writeEntry("NoContent", mNoContent);
  group.writeEntry("ReadOnly", mReadOnly);
  KMFolderMbox::writeConfig();
}

//-----------------------------------------------------------------------------
void KMFolderImap::remove()
{
  if ( mAlreadyRemoved || !account() )
  {
    // override
    FolderStorage::remove();
    return;
  }
  KUrl url = account()->getUrl();
  url.setPath(imapPath());
  if ( account()->makeConnection() == ImapAccountBase::Error ||
       imapPath().isEmpty() )
  {
    emit removed(folder(), false);
    return;
  }
  KIO::SimpleJob *job = KIO::file_delete(url, false);
  KIO::Scheduler::assignJobToSlave(account()->slave(), job);
  ImapAccountBase::jobData jd(url.url());
  jd.progressItem = ProgressManager::createProgressItem(
                      "ImapFolderRemove" + ProgressManager::getUniqueID(),
                      i18n("Removing folder"),
                      i18n( "URL: %1", Q3StyleSheet::escape( folder()->prettyUrl() ) ),
                      false,
                      account()->useSSL() || account()->useTLS() );
  account()->insertJob(job, jd);
  connect(job, SIGNAL(result(KJob *)),
          this, SLOT(slotRemoveFolderResult(KJob *)));
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotRemoveFolderResult(KJob *job)
{
  ImapAccountBase::JobIterator it = account()->findJob(static_cast<KIO::Job*>(job));
  if ( it == account()->jobsEnd() ) return;
  if (job->error())
  {
    account()->handleJobError( static_cast<KIO::Job*>(job), i18n("Error while removing a folder.") );
    emit removed(folder(), false);
  } else {
    account()->removeJob(it);
    FolderStorage::remove();
  }

}

//-----------------------------------------------------------------------------
void KMFolderImap::removeMsg(int idx, bool quiet)
{
  if (idx < 0)
    return;

  if (!quiet)
  {
    KMMessage *msg = getMsg(idx);
    deleteMessage(msg);
  }

  mLastUid = 0;
  KMFolderMbox::removeMsg(idx);
}

void KMFolderImap::removeMsg( const QList<KMMessage*>& msgList, bool quiet )
{
  if ( msgList.isEmpty() ) return;
  if (!quiet)
    deleteMessage(msgList);

  mLastUid = 0;

  /* Remove the messages from the local store as well.
     We don't call KMFolderInherited::removeMsg(QPtrList<KMMessage>) but
     iterate ourselves, as that would call KMFolderImap::removeMsg(int)
     and not the one from the store we want to be used. */

  QList<KMMessage*>::const_iterator it;
  for ( it = msgList.begin(); it != msgList.end(); ++it ) {
    int idx = find( (*it) );
    assert( idx != -1);
    // ATTENTION port me to maildir
    KMFolderMbox::removeMsg(idx, quiet);
  }
}

//-----------------------------------------------------------------------------
int KMFolderImap::rename( const QString& newName, KMFolderDir *aParent )
{
  if ( !aParent )
    KMFolderMbox::rename( newName );
  kmkernel->folderMgr()->contentsChanged();
  return 0;
}

//-----------------------------------------------------------------------------
void KMFolderImap::addMsgQuiet(KMMessage* aMsg)
{
  KMFolder *aFolder = aMsg->parent();
  quint32 serNum = 0;
  aMsg->setTransferInProgress( false );
  if (aFolder) {
    serNum = aMsg->getMsgSerNum();
    kmkernel->undoStack()->pushSingleAction( serNum, aFolder, folder() );
    int idx = aFolder->find( aMsg );
    assert( idx != -1 );
    aFolder->take( idx );
  } else {
    kDebug(5006) << k_funcinfo << "no parent" << endl;
  }
  if ( !account()->hasCapability("uidplus") ) {
    // Remember the status with the MD5 as key
    // so it can be transfered to the new message
    mMetaDataMap.insert( aMsg->msgIdMD5(),
        new KMMsgMetaData( aMsg->status(), serNum ) );
  }

  delete aMsg;
  aMsg = 0;
  getFolder();
}

//-----------------------------------------------------------------------------
void KMFolderImap::addMsgQuiet(QList<KMMessage*> msgList)
{
  if ( mAddMessageProgressItem )
  {
    mAddMessageProgressItem->setComplete();
    mAddMessageProgressItem = 0;
  }
  KMFolder *aFolder = msgList.first()->parent();
  int undoId = -1;
  bool uidplus = account()->hasCapability("uidplus");
  QList<KMMessage*>::const_iterator it;
  for ( it = msgList.begin(); it != msgList.end(); ++it )
  {
    KMMessage* msg = (*it);
    if ( undoId == -1 )
      undoId = kmkernel->undoStack()->newUndoAction( aFolder, folder() );
    if ( msg->getMsgSerNum() > 0 )
      kmkernel->undoStack()->addMsgToAction( undoId, msg->getMsgSerNum() );
    if ( !uidplus ) {
      // Remember the status with the MD5 as key
      // so it can be transfered to the new message
      mMetaDataMap.insert( msg->msgIdMD5(),
          new KMMsgMetaData( msg->status(), msg->getMsgSerNum() ) );
    }
    msg->setTransferInProgress( false );
  }
  if ( aFolder ) {
    aFolder->take( msgList );
  } else {
    kDebug(5006) << k_funcinfo << "no parent" << endl;
  }
  while ( !msgList.empty() )
    msgList.takeFirst();
  getFolder();
}

//-----------------------------------------------------------------------------
int KMFolderImap::addMsg(KMMessage* aMsg, int* aIndex_ret)
{
  QList<KMMessage*> list;
  list.append(aMsg);
  QList<int> index;
  int ret = addMsg(list, index);
  aIndex_ret = &index.first();
  return ret;
}

int KMFolderImap::addMsg(QList<KMMessage*>& msgList, QList<int>& aIndex_ret)
{
  KMMessage *aMsg = msgList.first();
  KMFolder *msgParent = aMsg->parent();

  ImapJob *imapJob = 0;
  if (msgParent)
  {
    if (msgParent->folderType() == KMFolderTypeImap)
    {
      if (static_cast<KMFolderImap*>(msgParent->storage())->account() == account())
      {
        // make sure the messages won't be deleted while we work with them
        QList<KMMessage*>::const_iterator it;
        for ( it = msgList.begin(); it != msgList.end(); ++it )
          (*it)->setTransferInProgress(true);

        if (folder() == msgParent)
        {
          // transfer the whole message, e.g. a draft-message is canceled and re-added to the drafts-folder
          QList<KMMessage*>::const_iterator it;
          for ( it = msgList.begin(); it != msgList.end(); ++it )
          {
            KMMessage* msg = (*it);
            if (!msg->isComplete())
            {
              int idx = msgParent->find(msg);
              assert(idx != -1);
              msg = msgParent->getMsg(idx);
            }
            imapJob = new ImapJob(msg, ImapJob::tPutMessage, this);
            connect(imapJob, SIGNAL(messageStored(KMMessage*)),
                     SLOT(addMsgQuiet(KMMessage*)));
            connect(imapJob, SIGNAL(result(KMail::FolderJob*)),
                SLOT(slotCopyMsgResult(KMail::FolderJob*)));
            imapJob->start();
          }

        } else {

          // get the messages and the uids
          QList<ulong> uids;
          getUids(msgList, uids);

          // get the sets (do not sort the uids)
          QStringList sets = makeSets(uids, false);

          for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
          {
            // we need the messages that belong to the current set to pass them to the ImapJob
            QList<KMMessage*> temp_msgs = splitMessageList(*it, msgList);
            if ( temp_msgs.isEmpty() ) kDebug(5006) << "Wow! KMFolderImap::splitMessageList() returned an empty list!" << endl;
            imapJob = new ImapJob(temp_msgs, *it, ImapJob::tMoveMessage, this);
            connect(imapJob, SIGNAL(messageCopied(QList<KMMessage*>)),
                SLOT(addMsgQuiet(QList<KMMessage*>)));
            connect(imapJob, SIGNAL(result(KMail::FolderJob*)),
                SLOT(slotCopyMsgResult(KMail::FolderJob*)));
            imapJob->start();
          }
        }
        return 0;
      }
      else
      {
        // different account, check if messages can be added
        KMMessage *msg;
        foreach ( msg, msgList )
        {
          int index;
          if (!canAddMsgNow(msg, &index)) {
            aIndex_ret << index;
            msgList.removeAll(msg);
          } else {
            if (!msg->transferInProgress())
              msg->setTransferInProgress(true);
          }
        }
      }
    } // if imap
  }

  if ( !msgList.isEmpty() )
  {
    // transfer from local folders or other accounts
    KMMessage* msg;
    foreach ( msg, msgList )
    {
      if ( !msg->transferInProgress() )
        msg->setTransferInProgress( true );
    }
    imapJob = new ImapJob( msgList, QString(), ImapJob::tPutMessage, this );
    if ( !mAddMessageProgressItem && msgList.count() > 1 )
    {
      // use a parent progress if we have more than 1 message
      // otherwise the normal progress is more accurate
      mAddMessageProgressItem = ProgressManager::createProgressItem(
          "Uploading"+ProgressManager::getUniqueID(),
          i18n("Uploading message data"),
          i18n("Destination folder: %1", Q3StyleSheet::escape( folder()->prettyUrl() ) ),
          true,
          account()->useSSL() || account()->useTLS() );
      mAddMessageProgressItem->setTotalItems( msgList.count() );
      connect ( mAddMessageProgressItem, SIGNAL( progressItemCanceled( KPIM::ProgressItem*)),
          account(), SLOT( slotAbortRequested( KPIM::ProgressItem* ) ) );
      imapJob->setParentProgressItem( mAddMessageProgressItem );
    }
    connect( imapJob, SIGNAL( messageCopied(QList<KMMessage*>) ),
        SLOT( addMsgQuiet(QList<KMMessage*>) ) );
    connect( imapJob, SIGNAL(result(KMail::FolderJob*)),
             SLOT(slotCopyMsgResult(KMail::FolderJob*)) );
    imapJob->start();
  }

  return 0;
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotCopyMsgResult( KMail::FolderJob* job )
{
  kDebug(5006) << k_funcinfo << job->error() << endl;
  if ( job->error() ) // getFolder() will not be called in this case
    emit folderComplete( this, false );
}

//-----------------------------------------------------------------------------
void KMFolderImap::copyMsg(QList<KMMessage*>& msgList)
{
  if ( !account()->hasCapability("uidplus") ) {
    KMMessage *msg;
    foreach ( msg, msgList ) {
      // Remember the status with the MD5 as key
      // so it can be transfered to the new message
      mMetaDataMap.insert( msg->msgIdMD5(), new KMMsgMetaData( msg->status() ) );
    }
  }

  QList<ulong> uids;
  getUids(msgList, uids);
  QStringList sets = makeSets(uids, false);
  for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
  {
    // we need the messages that belong to the current set to pass them to the ImapJob
    QList<KMMessage*> temp_msgs = splitMessageList(*it, msgList);

    ImapJob *job = new ImapJob(temp_msgs, *it, ImapJob::tCopyMessage, this);
    connect(job, SIGNAL(messageCopied(QPtrList<KMMessage>)),
            SLOT(addMsgQuiet(QPtrList<KMMessage>)));
    connect(job, SIGNAL(result(KMail::FolderJob*)),
            SLOT(slotCopyMsgResult(KMail::FolderJob*)));
    job->start();
  }
}

//-----------------------------------------------------------------------------
QList<KMMessage*> KMFolderImap::splitMessageList(const QString& set,
                                                   QList<KMMessage*>& msgList)
{
  int lastcomma = set.lastIndexOf(",");
  int lastdub = set.lastIndexOf(":");
  int last = 0;
  if (lastdub > lastcomma) last = lastdub;
  else last = lastcomma;
  last++;
  if (last < 0) last = set.length();
  // the last uid of the current set
  const QString last_uid = set.right(set.length() - last);
  QList<KMMessage*> temp_msgs;
  QString uid;
  if (!last_uid.isEmpty())
  {
    KMMessage* msg = 0;
    while ( !msgList.empty() && (msg = msgList.takeFirst()) )
    {
      // append the msg to the new list and delete it from the old
      temp_msgs.append(msg);
      uid.setNum( msg->UID() );
      // remove modifies the current
      if (uid == last_uid) break;
    }
  }
  else
  {
    // probably only one element
    temp_msgs = msgList;
  }

  return temp_msgs;
}

//-----------------------------------------------------------------------------
KMMessage* KMFolderImap::take(int idx)
{
  KMMsgBase* mb(mMsgList[idx]);
  if (!mb) return 0;
  if (!mb->isMessage()) readMsg(idx);

  KMMessage *msg = static_cast<KMMessage*>(mb);
  deleteMessage(msg);

  mLastUid = 0;
  return KMFolderMbox::take(idx);
}

void KMFolderImap::take(QList<KMMessage*> msgList)
{
  deleteMessage(msgList);

  mLastUid = 0;
  KMFolderMbox::take(msgList);
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotListNamespaces()
{
  disconnect( account(), SIGNAL( connectionResult(int, const QString&) ),
      this, SLOT( slotListNamespaces() ) );
  if ( account()->makeConnection() == ImapAccountBase::Error )
  {
    kWarning(5006) << "slotListNamespaces - got no connection" << endl;
    return;
  } else if ( account()->makeConnection() == ImapAccountBase::Connecting )
  {
    // wait for the connectionResult
    kDebug(5006) << "slotListNamespaces - waiting for connection" << endl;
    connect( account(), SIGNAL( connectionResult(int, const QString&) ),
        this, SLOT( slotListNamespaces() ) );
    return;
  }
  kDebug(5006) << "slotListNamespaces" << endl;
  // reset subfolder states recursively
  setSubfolderState( imapNoInformation );
  mSubfolderState = imapListingInProgress;
  account()->setHasInbox( false );

  ImapAccountBase::ListType type = ImapAccountBase::List;
  if ( account()->onlySubscribedFolders() )
    type = ImapAccountBase::ListSubscribed;

  ImapAccountBase::nsMap map = account()->namespaces();
  QStringList personal = map[ImapAccountBase::PersonalNS];
  // start personal namespace listing and send it directly to slotListResult
  for ( QStringList::Iterator it = personal.begin(); it != personal.end(); ++it )
  {
    KMail::ListJob* job = new KMail::ListJob( account(), type, this,
	account()->addPathToNamespace( *it ) );
    job->setNamespace( *it );
    connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
        this, SLOT(slotListResult(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
    job->start();
  }

  // and now we list all other namespaces and check them ourself
  QStringList ns = map[ImapAccountBase::OtherUsersNS];
  ns += map[ImapAccountBase::SharedNS];
  for ( QStringList::Iterator it = ns.begin(); it != ns.end(); ++it )
  {
    KMail::ListJob* job = new  KMail::ListJob( account(), type, this, account()->addPathToNamespace( *it ) );
    connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
        this, SLOT(slotCheckNamespace(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
    job->start();
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotCheckNamespace( const QStringList& subfolderNames,
                                       const QStringList& subfolderPaths,
                                       const QStringList& subfolderMimeTypes,
                                       const QStringList& subfolderAttributes,
                                       const ImapAccountBase::jobData& jobData )
{
  kDebug(5006) << "slotCheckNamespace - " << subfolderNames.join(",") << endl;

  // get a correct foldername:
  // strip / and make sure it does not contain the delimiter
  QString name = jobData.path.mid( 1, jobData.path.length()-2 );
  name.remove( account()->delimiterForNamespace( name ) );
  if ( name.isEmpty() ) {
    // happens when an empty namespace is defined
    slotListResult( subfolderNames, subfolderPaths,
        subfolderMimeTypes, subfolderAttributes, jobData );
    return;
  }

  folder()->createChildFolder();
  KMFolderNode *node = 0;
  QList<KMFolderNode*>::const_iterator it;
  for ( it = folder()->child()->begin();
      ( node = *it ) && it != folder()->child()->end(); ++it ) {
    if ( !node->isDir() && node->name() == name )
      break;
  }
  if ( subfolderNames.isEmpty() ) {
    if ( node ) {
      kDebug(5006) << "delete namespace folder " << name << endl;
      KMFolder *fld = static_cast<KMFolder*>(node);
      KMFolderImap* nsFolder = static_cast<KMFolderImap*>(fld->storage());
      nsFolder->setAlreadyRemoved( true );
      kmkernel->imapFolderMgr()->remove( fld );
    }
  } else {
    if ( node ) {
      // folder exists so pass on the attributes
      kDebug(5006) << "found namespace folder " << name << endl;
      if ( !account()->listOnlyOpenFolders() ) {
        KMFolderImap* nsFolder =
          static_cast<KMFolderImap*>(static_cast<KMFolder*>(node)->storage());
        nsFolder->slotListResult( subfolderNames, subfolderPaths,
            subfolderMimeTypes, subfolderAttributes, jobData );
      }
    } else {
      // create folder
      kDebug(5006) << "create namespace folder " << name << endl;
      KMFolder *fld = folder()->child()->createFolder( name );
      if ( fld ) {
        KMFolderImap* f = static_cast<KMFolderImap*> ( fld->storage() );
        f->initializeFrom( this, account()->addPathToNamespace( name ),
            "inode/directory" );
        if ( !account()->listOnlyOpenFolders() ) {
          f->slotListResult( subfolderNames, subfolderPaths,
              subfolderMimeTypes, subfolderAttributes, jobData );
        }
      }
      kmkernel->imapFolderMgr()->contentsChanged();
    }
  }
}

//-----------------------------------------------------------------------------
bool KMFolderImap::listDirectory()
{
  if ( !account() ||
       ( account() && account()->makeConnection() == ImapAccountBase::Error ) )
  {
    kDebug(5006) << "KMFolderImap::listDirectory - got no connection" << endl;
    return false;
  }

  if ( this == account()->rootFolder() )
  {
    // a new listing started
    slotListNamespaces();
    return true;
  }
  mSubfolderState = imapListingInProgress;

  // get the folders
  ImapAccountBase::ListType type = ImapAccountBase::List;
  if ( account()->onlySubscribedFolders() )
    type = ImapAccountBase::ListSubscribed;
  KMail::ListJob* job = new  KMail::ListJob( account(), type, this );
  job->setParentProgressItem( account()->listDirProgressItem() );
  job->setHonorLocalSubscription( true );
  connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
      this, SLOT(slotListResult(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
  job->start();

  return true;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListResult( const QStringList& subfolderNames,
                                   const QStringList& subfolderPaths,
                                   const QStringList& subfolderMimeTypes,
                                   const QStringList& subfolderAttributes,
                                   const ImapAccountBase::jobData& jobData )
{
  mSubfolderState = imapFinished;
  //kDebug(5006) << label() << ": folderNames=" << subfolderNames << " folderPaths="
  //<< subfolderPaths << " mimeTypes=" << subfolderMimeTypes << endl;

  // don't react on changes
  kmkernel->imapFolderMgr()->quiet(true);

  bool root = ( this == account()->rootFolder() );
  folder()->createChildFolder();
  if ( root && !account()->hasInbox() )
  {
    // create the INBOX
    initInbox();
  }

  // see if we have a better parent
  // if you have a prefix that contains a folder (e.g "INBOX.") the folders
  // need to be created underneath it
  if ( root && !subfolderNames.empty() )
  {
    KMFolderImap* parent = findParent( subfolderPaths.first(), subfolderNames.first() );
    if ( parent )
    {
      kDebug(5006) << "KMFolderImap::slotListResult - pass listing to "
        << parent->label() << endl;
      parent->slotListResult( subfolderNames, subfolderPaths,
          subfolderMimeTypes, subfolderAttributes, jobData );
      // cleanup
      QStringList list;
      checkFolders( list, jobData.curNamespace );
      // finish
      emit directoryListingFinished( this );
      kmkernel->imapFolderMgr()->quiet( false );
      return;
    }
  }

  bool emptyList = ( root && subfolderNames.empty() );
  if ( !emptyList )
  {
    checkFolders( subfolderNames, jobData.curNamespace );
  }

  KMFolderImap *f = 0;
  KMFolderNode *node = 0;
  for ( int i = 0; i < subfolderNames.count(); i++ )
  {
    bool settingsChanged = false;
    // create folders if necessary
    QList<KMFolderNode*>::const_iterator it;
    for ( it = folder()->child()->begin();
        ( node = *it ) && it != folder()->child()->end(); ++it ) {
      if ( !node->isDir() && node->name() == subfolderNames[i] )
        break;
    }
    if ( node ) {
      f = static_cast<KMFolderImap*>(static_cast<KMFolder*>(node)->storage());
    }
    else if ( subfolderPaths[i].toUpper() != "/INBOX/" )
    {
      kDebug(5006) << "create folder " << subfolderNames[i] << endl;
      KMFolder *fld = folder()->child()->createFolder(subfolderNames[i]);
      if ( fld ) {
        f = static_cast<KMFolderImap*> ( fld->storage() );
        settingsChanged = true;
      } else {
        kWarning(5006) << "can't create folder " << subfolderNames[i] << endl;
      }
    }
    if ( f ) {
      // sanity check
      if ( f->imapPath().isEmpty() ) {
        settingsChanged = true;
      }
      // update progress
      account()->listDirProgressItem()->incCompletedItems();
      account()->listDirProgressItem()->updateProgress();
      account()->listDirProgressItem()->setStatus( folder()->prettyUrl() + i18n(" completed") );

      f->initializeFrom( this, subfolderPaths[i], subfolderMimeTypes[i] );
      f->setChildrenState( subfolderAttributes[i] );
      if ( account()->listOnlyOpenFolders() &&
           f->hasChildren() != FolderStorage::ChildrenUnknown )
      {
        settingsChanged = true;
      }

      if ( settingsChanged )
      {
        // tell the tree our information changed
        kmkernel->imapFolderMgr()->contentsChanged();
      }
      if ( ( subfolderMimeTypes[i] == "message/directory" ||
             subfolderMimeTypes[i] == "inode/directory" ) &&
           !account()->listOnlyOpenFolders() )
      {
        f->listDirectory();
      }
    } else {
      kWarning(5006) << "can't find folder " << subfolderNames[i] << endl;
    }
  } // for subfolders

  // now others should react on the changes
  kmkernel->imapFolderMgr()->quiet( false );
  emit directoryListingFinished( this );
  account()->listDirProgressItem()->setComplete();
}

//-----------------------------------------------------------------------------
void KMFolderImap::initInbox()
{
  KMFolderImap *f = 0;
  KMFolderNode *node = 0;

  QList<KMFolderNode*>::const_iterator it;
  for ( it = folder()->child()->begin();
      ( node = *it ) && it != folder()->child()->end(); ++it ) {
    if (!node->isDir() && node->name() == "INBOX") break;
  }
  if (node) {
    f = static_cast<KMFolderImap*>(static_cast<KMFolder*>(node)->storage());
  } else {
    f = static_cast<KMFolderImap*>
      (folder()->child()->createFolder("INBOX", true)->storage());
    if ( f ) {
      f->folder()->setLabel( i18n("inbox") );
    }
    kmkernel->imapFolderMgr()->contentsChanged();
  }
  if ( f ) {
    f->initializeFrom( this, "/INBOX/", "message/directory" );
    f->setChildrenState( QString() );
  }
  // so we have an INBOX
  account()->setHasInbox( true );
}

//-----------------------------------------------------------------------------
KMFolderImap* KMFolderImap::findParent( const QString& path, const QString& name )
{
  QString parent = path.left( path.length() - name.length() - 2 );
  if ( parent.length() > 1 )
  {
    // extract name of the parent
    parent = parent.right( parent.length() - 1 );
    if ( parent != label() )
    {
      KMFolderNode *node = 0;
      // look for a better parent
      QList<KMFolderNode*>::const_iterator it;
      for ( it = folder()->child()->begin();
          ( node = *it ) && it != folder()->child()->end(); ++it )
      {
        if ( node->name() == parent )
        {
          KMFolder* fld = static_cast<KMFolder*>(node);
          KMFolderImap* imapFld = static_cast<KMFolderImap*>( fld->storage() );
          return imapFld;
        }
      }
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
void KMFolderImap::checkFolders( const QStringList& subfolderNames,
    const QString& myNamespace )
{
  QList<KMFolder*> toRemove;
  if ( !folder()->child() ) {
    return;
  }
  KMFolderNode *node = 0;
  QList<KMFolderNode*>::const_iterator it;
  for ( it = folder()->child()->begin();
      ( node = *it ) && it != folder()->child()->end(); ++it )
  {
    if ( !node->isDir() && !subfolderNames.contains(node->name()) )
    {
      KMFolder* fld = static_cast<KMFolder*>(node);
      KMFolderImap* imapFld = static_cast<KMFolderImap*>( fld->storage() );
      // as more than one namespace can be listed in the root folder we need to make sure
      // that the folder is within the current namespace
      bool isInNamespace = ( myNamespace.isEmpty() ||
          myNamespace == account()->namespaceForFolder( imapFld ) );
      kDebug(5006) << node->name() << " in namespace " << myNamespace << ":" <<
        isInNamespace << endl;
      // ignore some cases
      QString name = node->name();
      bool ignore = ( ( this == account()->rootFolder() ) &&
          ( imapFld->imapPath() == "/INBOX/" ||
            account()->isNamespaceFolder( name ) ||
	    !isInNamespace ) );
      // additional sanity check for broken folders
      if ( imapFld->imapPath().isEmpty() ) {
        ignore = false;
      }
      if ( !ignore )
      {
        // remove the folder without server round trip
        kDebug(5006) << "checkFolders - " << node->name() << " disappeared" << endl;
        imapFld->setAlreadyRemoved( true );
        toRemove.append( fld );
      } else {
        kDebug(5006) << "checkFolders - " << node->name() << " ignored" << endl;
      }
    }
  }
  // remove folders
  QList<KMFolder*>::const_iterator jt;
  for ( jt = toRemove.constBegin(); jt != toRemove.constEnd(); ++jt )
    if ( *jt )
      kmkernel->imapFolderMgr()->remove( *jt );
}

//-----------------------------------------------------------------------------
void KMFolderImap::initializeFrom( KMFolderImap* parent, QString folderPath,
                                   QString mimeType )
{
  setAccount( parent->account() );
  setImapPath( folderPath );
  setNoContent( mimeType == "inode/directory" );
  setNoChildren( mimeType == "message/digest" );
}

//-----------------------------------------------------------------------------
void KMFolderImap::setChildrenState( QString attributes )
{
  // update children state
  if ( attributes.contains( "haschildren", Qt::CaseSensitive ) )
  {
    setHasChildren( FolderStorage::HasChildren );
  } else if ( attributes.contains( "hasnochildren", Qt::CaseSensitive ) ||
              attributes.contains( "noinferiors", Qt::CaseSensitive ) )
  {
    setHasChildren( FolderStorage::HasNoChildren );
  } else
  {
    if ( account()->listOnlyOpenFolders() ) {
      setHasChildren( FolderStorage::HasChildren );
    } else {
      setHasChildren( FolderStorage::ChildrenUnknown );
    }
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::checkValidity()
{
  if (!account()) {
    emit folderComplete(this, false);
    return;
  }
  KUrl url = account()->getUrl();
  url.setPath(imapPath() + ";UID=0:0");
  kDebug(5006) << "KMFolderImap::checkValidity of: " << imapPath() << endl;

  // Start with a clean slate
  disconnect( account(), SIGNAL( connectionResult(int, const QString&) ),
              this, SLOT( checkValidity() ) );

  KMAcctImap::ConnectionState connectionState = account()->makeConnection();
  if ( connectionState == ImapAccountBase::Error ) {
    kDebug(5006) << "KMFolderImap::checkValidity - got no connection" << endl;
    emit folderComplete(this, false);
    mContentState = imapNoInformation;
    return;
  } else if ( connectionState == ImapAccountBase::Connecting ) {
    // We'll wait for the connectionResult signal from the account. If it
    // errors, the above will catch it.
    kDebug(5006) << "CheckValidity - waiting for connection" << endl;
    connect( account(), SIGNAL( connectionResult(int, const QString&) ),
        this, SLOT( checkValidity() ) );
    return;
  }
  // Only check once at a time.
  if ( mCheckingValidity ) {
    kDebug(5006) << "KMFolderImap::checkValidity - already checking" << endl;
    return;
  }
  // otherwise we already are inside a mailcheck
  if ( !mMailCheckProgressItem ) {
    ProgressItem* parent = ( account()->checkingSingleFolder() ? 0 :
        account()->mailCheckProgressItem() );
    mMailCheckProgressItem = ProgressManager::createProgressItem(
              parent,
              "MailCheck" + folder()->prettyUrl(),
              folder()->prettyUrl(),
              i18n("checking"),
              false,
              account()->useSSL() || account()->useTLS() );
  } else {
    mMailCheckProgressItem->setProgress(0);
  }
  if ( account()->mailCheckProgressItem() ) {
    account()->mailCheckProgressItem()->setStatus( folder()->prettyUrl() );
  }
  open();
  ImapAccountBase::jobData jd( url.url() );
  KIO::SimpleJob *job = KIO::get(url, false, false);
  KIO::Scheduler::assignJobToSlave(account()->slave(), job);
  account()->insertJob(job, jd);
  connect(job, SIGNAL(result(KJob *)),
          SLOT(slotCheckValidityResult(KJob *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          SLOT(slotSimpleData(KIO::Job *, const QByteArray &)));
  // Only check once at a time.
  mCheckingValidity = true;
}


//-----------------------------------------------------------------------------
ulong KMFolderImap::lastUid()
{
  if ( mLastUid > 0 )
    return mLastUid;
  open();
  if (count() > 0)
  {
    KMMsgBase * base = getMsgBase(count()-1);
    mLastUid = base->UID();
  }
  close();
  return mLastUid;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotCheckValidityResult(KJob * job)
{
  kDebug(5006) << "KMFolderImap::slotCheckValidityResult of: " << fileName() << endl;
  mCheckingValidity = false;
  ImapAccountBase::JobIterator it = account()->findJob(static_cast<KIO::Job*>(job));
  if ( it == account()->jobsEnd() ) return;
  if (job->error()) {
    if ( job->error() != KIO::ERR_ACCESS_DENIED ) {
      // we suppress access denied messages because they are normally a result of
      // explicitly set ACLs. Do not save this information (e.g. setNoContent) so that
      // we notice when this changes
      account()->handleJobError( static_cast<KIO::Job*>(job), i18n("Error while querying the server status.") );
    }
    mContentState = imapNoInformation;
    emit folderComplete(this, false);
    close();
  } else {
    QByteArray cstr((*it).data.data(), (*it).data.size());
    int a = cstr.indexOf("X-uidValidity: ");
    int b = cstr.indexOf("\r\n", a);
    QString uidv;
    if ( (b - a - 15) >= 0 )
      uidv = cstr.mid(a + 15, b - a - 15);
    a = cstr.indexOf("X-Access: ");
    b = cstr.indexOf("\r\n", a);
    QString access;
    if ( (b - a - 10) >= 0 )
      access = cstr.mid(a + 10, b - a - 10);
    mReadOnly = access == "Read only";
    a = cstr.indexOf("X-Count: ");
    b = cstr.indexOf("\r\n", a);
    int exists = -1;
    bool ok = false;
    if ( (b - a - 9) >= 0 )
      exists = cstr.mid(a + 9, b - a - 9).toInt(&ok);
    if ( !ok ) exists = -1;
    QString startUid;
    if (uidValidity() != uidv)
    {
      // uidValidity changed
      kDebug(5006) << k_funcinfo << "uidValidty changed from "
       << uidValidity() << " to " << uidv << endl;
      if ( !uidValidity().isEmpty() )
      {
        account()->ignoreJobsForFolder( folder() );
        mUidMetaDataMap.clear();
      }
      mLastUid = 0;
      setUidValidity(uidv);
      writeConfig();
    } else {
      if (!mCheckFlags)
        startUid = QString::number(lastUid() + 1);
    }
    account()->removeJob(it);
    if ( mMailCheckProgressItem ) {
      if ( startUid.isEmpty() ) {
        // flags for all messages are loaded
        mMailCheckProgressItem->setTotalItems( exists );
      } else {
        // only an approximation but doesn't hurt
        int remain = exists - count();
        if ( remain < 0 ) remain = 1;
        mMailCheckProgressItem->setTotalItems( remain );
      }
      mMailCheckProgressItem->setCompletedItems( 0 );
    }
    reallyGetFolder( startUid );
    close();
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::getAndCheckFolder(bool force)
{
  if (mNoContent)
    return getFolder(force);

  if ( account() )
    account()->processNewMailSingleFolder( folder() );
  if (force) {
    // force an update
    mCheckFlags = true;
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::getFolder(bool force)
{
  mGuessedUnreadMsgs = -1;
  if (mNoContent)
  {
    mContentState = imapFinished;
    emit folderComplete(this, true);
    return;
  }
  open();
  mContentState = imapListingInProgress;
  if (force) {
    // force an update
    mCheckFlags = true;
  }
  checkValidity();
  close();
}

//-----------------------------------------------------------------------------
void KMFolderImap::reallyGetFolder(const QString &startUid)
{
  KUrl url = account()->getUrl();
  if ( account()->makeConnection() != ImapAccountBase::Connected )
  {
    mContentState = imapNoInformation;
    emit folderComplete(this, false);
    return;
  }
  quiet(true);
  if (startUid.isEmpty())
  {
    if ( mMailCheckProgressItem )
      mMailCheckProgressItem->setStatus( i18n("Retrieving message status") );
    url.setPath(imapPath() + ";SECTION=UID FLAGS");
    KIO::SimpleJob *job = KIO::listDir(url, false);
    open();
    KIO::Scheduler::assignJobToSlave(account()->slave(), job);
    ImapAccountBase::jobData jd( url.url(), folder() );
    jd.cancellable = true;
    account()->insertJob(job, jd);
    connect(job, SIGNAL(result(KJob *)),
            this, SLOT(slotListFolderResult(KJob *)));
    connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
            this, SLOT(slotListFolderEntries(KIO::Job *,
            const KIO::UDSEntryList &)));
  } else {
    mContentState = imapDownloadInProgress;
    if ( mMailCheckProgressItem )
      mMailCheckProgressItem->setStatus( i18n("Retrieving messages") );
    url.setPath(imapPath() + ";UID=" + startUid
      + ":*;SECTION=ENVELOPE");
    KIO::SimpleJob *newJob = KIO::get(url, false, false);
    KIO::Scheduler::assignJobToSlave(account()->slave(), newJob);
    ImapAccountBase::jobData jd( url.url(), folder() );
    jd.cancellable = true;
    open();
    account()->insertJob(newJob, jd);
    connect(newJob, SIGNAL(result(KJob *)),
            this, SLOT(slotGetLastMessagesResult(KJob *)));
    connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
            this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
  }
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListFolderResult(KJob * job)
{
  ImapAccountBase::JobIterator it = account()->findJob(static_cast<KIO::Job*>(job));
  if ( it == account()->jobsEnd() ) return;
  QString uids;
  if (job->error())
  {
    account()->handleJobError( static_cast<KIO::Job*>(job),
         i18n("Error while listing the contents of the folder %1.", label() ) );
    account()->removeJob(it);
    finishMailCheck( imapNoInformation );
    return;
  }
  mCheckFlags = false;
  QStringList::Iterator uid;
  /*
    The code below does the following:
    - for each mail in the local store and each entry we got from the server,
      compare the local uid with the one from the server and update the status
      flags of the mails
    - for all mails that are not already locally present, start a job which
      gets the envelope of each
    - remove all locally present mails if the server does not list them anymore
  */
  if ( count() ) {
    int idx = 0, c, serverFlags;
    ulong mailUid, serverUid;
    uid = (*it).items.begin();
    while ( idx < count() && uid != (*it).items.end() ) {
      KMMsgBase *msgBase = getMsgBase( idx );
      mailUid = msgBase->UID();
      // parse the uid from the server and the flags out of the list from
      // the server. Format: 1234, 1
      c = (*uid).indexOf(",");
      serverUid = (*uid).left( c ).toLong();
      serverFlags = (*uid).mid( c+1 ).toInt();
      if ( mailUid < serverUid ) {
        removeMsg( idx, true );
      } else if ( mailUid == serverUid ) {
        // if this is a read only folder, ignore status updates from the server
        // since we can't write our status back our local version is what has to
        // be considered correct.
        if (!mReadOnly)
          flagsToStatus( msgBase, serverFlags, false );
        idx++;
        uid = (*it).items.erase(uid);
        if ( msgBase->getMsgSerNum() > 0 ) {
          saveMsgMetaData( static_cast<KMMessage*>(msgBase) );
        }
      }
      else break;  // happens only, if deleted mails reappear on the server
    }
    // remove all remaining entries in the local cache, they are no longer
    // present on the server
    while (idx < count()) removeMsg(idx, true);
  }
  // strip the flags from the list of uids, so it can be reused
  for (uid = (*it).items.begin(); uid != (*it).items.end(); ++uid)
    (*uid).truncate((*uid).indexOf(","));
  ImapAccountBase::jobData jd( QString::null, (*it).parent );
  jd.total = (*it).items.count();
  if (jd.total == 0) {
    finishMailCheck( imapFinished );
    account()->removeJob(it);
    return;
  }
  if ( mMailCheckProgressItem ) {
    // next step for the progressitem
    mMailCheckProgressItem->setCompletedItems( 0 );
    mMailCheckProgressItem->setTotalItems( jd.total );
    mMailCheckProgressItem->setProgress( 0 );
    mMailCheckProgressItem->setStatus( i18n("Retrieving messages") );
  }

  QStringList sets;
  uid = (*it).items.begin();
  if (jd.total == 1) sets.append(*uid + ':' + *uid);
  else sets = makeSets( (*it).items );
  account()->removeJob(it); // don't use *it below

  if ( sets.isEmpty() ) {
    close();
  }

  // Now kick off the getting of envelopes for the new mails in the folder
  for (QStringList::Iterator i = sets.begin(); i != sets.end(); ++i) {
    mContentState = imapDownloadInProgress;
    KUrl url = account()->getUrl();
    url.setPath(imapPath() + ";UID=" + *i + ";SECTION=ENVELOPE");
    KIO::SimpleJob *newJob = KIO::get(url, false, false);
    jd.url = url.url();
    KIO::Scheduler::assignJobToSlave(account()->slave(), newJob);
    account()->insertJob(newJob, jd);
    connect(newJob, SIGNAL(result(KJob *)),
        this, (*i == sets.at(sets.size() - 1))
        ? SLOT(slotGetLastMessagesResult(KJob *))
        : SLOT(slotGetMessagesResult(KJob *)));
    connect(newJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
        this, SLOT(slotGetMessagesData(KIO::Job *, const QByteArray &)));
  }
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotListFolderEntries(KIO::Job * job,
  const KIO::UDSEntryList & uds)
{
  ImapAccountBase::JobIterator it = account()->findJob(job);
  if ( it == account()->jobsEnd() ) return;
  for (KIO::UDSEntryList::ConstIterator udsIt = uds.begin();
    udsIt != uds.end(); udsIt++)
  {
    const QString name = udsIt->stringValue( KIO::UDS_NAME );
    const QString mimeType = udsIt->stringValue( KIO::UDS_MIME_TYPE );
    const long long flags = udsIt->numberValue( KIO::UDS_ACCESS );
    if ((mimeType == "message/rfc822-imap" || mimeType == "message/rfc822") &&
        !(flags & 8)) {
      (*it).items.append(name + ',' + QString::number(flags));
      if ( mMailCheckProgressItem ) {
        mMailCheckProgressItem->incCompletedItems();
        mMailCheckProgressItem->updateProgress();
      }
    }
  }
}


//-----------------------------------------------------------------------------
void KMFolderImap::flagsToStatus(KMMsgBase *msg, int flags, bool newMsg)
{
  MessageStatus status;

  if ( !msg ) return;

  if (flags & 4)
    status.setImportant();
  if (flags & 2)
    status.setReplied();
  if (flags & 1)
    status.setOld();

  // In case the message does not have the seen flag set, override our local
  // notion that it is read. Otherwise the count of unread messages and the
  // number of messages which actually show up as read can go out of sync.
  if ( ( msg->status().isOfUnknownStatus() ) || !(flags&1) ) {
    if (newMsg)
      status.setNew();
    else
      status.setUnread();
  }
  msg->setStatus( status );
}


//-----------------------------------------------------------------------------
QString KMFolderImap::statusToFlags( const MessageStatus& status )
{
  QString flags;
  if ( status.isDeleted() )
    flags = "\\DELETED";
  else {
    if ( status.isOld() || status.isRead() )
      flags = "\\SEEN ";
    if ( status.isReplied() )
      flags += "\\ANSWERED ";
    if ( status.isImportant() )
      flags += "\\FLAGGED";
  }

  return flags.simplified();
}

//-------------------------------------------------------------
void
KMFolderImap::ignoreJobsForMessage( KMMessage* msg )
{
  if ( !msg || msg->transferInProgress() ||
       !msg->parent() || msg->parent()->folderType() != KMFolderTypeImap )
    return;
  KMAcctImap *account;
  if ( !(account = static_cast<KMFolderImap*>(msg->storage())->account()) )
    return;

  account->ignoreJobsForMessage( msg );
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotGetMessagesData(KIO::Job * job, const QByteArray & data)
{
  if ( data.isEmpty() ) return; // optimization
  ImapAccountBase::JobIterator it = account()->findJob(job);
  if ( it == account()->jobsEnd() ) return;
  (*it).cdata += data;
  int pos = (*it).cdata.indexOf("\r\n--IMAPDIGEST");
  if ( pos == -1 ) {
    // if we do not find the pattern in the complete string we will not find
    // it in a substring.
    return;
  }
  if (pos > 0)
  {
    int p = (*it).cdata.indexOf("\r\nX-uidValidity:");
    if (p != -1) setUidValidity((*it).cdata
      .mid(p + 17, (*it).cdata.indexOf("\r\n", p+1) - p - 17));
    int c = (*it).cdata.indexOf("\r\nX-Count:");
    if ( c != -1 )
    {
      bool ok;
      int exists = (*it).cdata.mid( c+10,
          (*it).cdata.indexOf("\r\n", c+1) - c-10 ).toInt(&ok);
      if ( ok && exists < count() ) {
        kDebug(5006) << "KMFolderImap::slotGetMessagesData - server has less messages (" <<
          exists << ") then folder (" << count() << "), so reload" << endl;
        open();
        reallyGetFolder( QString() );
        (*it).cdata.remove(0, pos);
        return;
      } else if ( ok ) {
        int delta = exists - count();
        if ( mMailCheckProgressItem ) {
          mMailCheckProgressItem->setTotalItems( delta );
        }
      }
    }
    (*it).cdata.remove(0, pos);
  }
  pos = (*it).cdata.indexOf("\r\n--IMAPDIGEST", 1);
  int flags;
  while (pos >= 0)
  {
    KMMessage *msg = new KMMessage;
    msg->setComplete( false );
    msg->setReadyToShow( false );
    // nothing between the boundaries, older UWs do that
    if ( pos != 14 ) {
      msg->fromString( (*it).cdata.mid(16, pos - 16) );
      flags = msg->headerField("X-Flags").toInt();
      ulong uid = msg->UID();
      KMMsgMetaData *md =  0;
      if ( mUidMetaDataMap.find( uid ) ) {
        md =  mUidMetaDataMap[uid];
      }
      ulong serNum = 0;
      if ( md ) {
        serNum = md->serNum();
      }
      bool ok = true;
      if ( uid <= lastUid() && serNum > 0 ) {
        // the UID is already known so no need to create it
        ok = false;
      }
      // deleted flag
      if ( flags & 8 )
        ok = false;
      if ( !ok ) {
        delete msg;
        msg = 0;
      } else {
        if ( serNum > 0 ) {
          // assign the sernum from the cache
          msg->setMsgSerNum( serNum );
        }
        // Transfer the status, if it is cached.
        if ( md ) {
          msg->setStatus( md->messageStatus() );
        } else if ( !account()->hasCapability("uidplus") ) {
          // see if we have cached the msgIdMD5 and get the status +
          // serial number from there
          QString id = msg->msgIdMD5();
          if ( mMetaDataMap.find( id ) ) {
            md =  mMetaDataMap[id];
            msg->setStatus( md->messageStatus() );
            if ( md->serNum() != 0 && serNum == 0 ) {
              msg->setMsgSerNum( md->serNum() );
            }
            mMetaDataMap.remove( id );
            delete md;
          }
        }
        KMFolderMbox::addMsg(msg, 0);
        // Merge with the flags from the server.
        flagsToStatus((KMMsgBase*)msg, flags);
        // set the correct size
        msg->setMsgSizeServer( msg->headerField("X-Length").toUInt() );
        msg->setUID(uid);
        if ( msg->getMsgSerNum() > 0 ) {
          saveMsgMetaData( msg );
        }
        // Filter messages that have arrived in the inbox folder
        if ( folder()->isSystemFolder() && imapPath() == "/INBOX/"
            && kmkernel->filterMgr()->atLeastOneIncomingFilterAppliesTo( account()->id() ) )
            account()->execFilters( msg->getMsgSerNum() );

        if ( count() > 1 ) {
          unGetMsg(count() - 1);
        }
        mLastUid = uid;
        if ( mMailCheckProgressItem ) {
          mMailCheckProgressItem->incCompletedItems();
          mMailCheckProgressItem->updateProgress();
        }
      }
    }
    (*it).cdata.remove(0, pos);
    (*it).done++;
    pos = (*it).cdata.indexOf("\r\n--IMAPDIGEST", 1);
  } // while
}

//-------------------------------------------------------------
FolderJob*
KMFolderImap::doCreateJob( KMMessage *msg, FolderJob::JobType jt,
                           KMFolder *folder, QString partSpecifier,
                           const AttachmentStrategy *as ) const
{
  KMFolderImap* kmfi = folder? dynamic_cast<KMFolderImap*>(folder->storage()) : 0;
  if ( jt == FolderJob::tGetMessage && partSpecifier == "STRUCTURE" &&
       account() && account()->loadOnDemand() &&
       ( msg->msgSizeServer() > 5000 || msg->msgSizeServer() == 0 ) &&
       ( msg->signatureState() == KMMsgNotSigned ||
         msg->signatureState() == KMMsgSignatureStateUnknown ) &&
       ( msg->encryptionState() == KMMsgNotEncrypted ||
         msg->encryptionState() == KMMsgEncryptionStateUnknown ) )
  {
    // load-on-demand: retrieve the BODYSTRUCTURE and to speed things up also the headers
    // this is not activated for small or signed messages
    ImapJob *job = new ImapJob( msg, jt, kmfi, "HEADER" );
    job->start();
    ImapJob *job2 = new ImapJob( msg, jt, kmfi, "STRUCTURE", as );
    job2->start();
    job->setParentFolder( this );
    return job;
  } else {
    // download complete message or part (attachment)
    if ( partSpecifier == "STRUCTURE" ) // hide from outside
      partSpecifier.clear();

    ImapJob *job = new ImapJob( msg, jt, kmfi, partSpecifier );
    job->setParentFolder( this );
    return job;
  }
}

//-------------------------------------------------------------
FolderJob*
KMFolderImap::doCreateJob( QList<KMMessage*>& msgList, const QString& sets,
                           FolderJob::JobType jt, KMFolder *folder ) const
{
  KMFolderImap* kmfi = dynamic_cast<KMFolderImap*>(folder->storage());
  ImapJob *job = new ImapJob( msgList, sets, jt, kmfi );
  job->setParentFolder( this );
  return job;
}

//-----------------------------------------------------------------------------
void KMFolderImap::getMessagesResult(KIO::Job * job, bool lastSet)
{
  ImapAccountBase::JobIterator it = account()->findJob(job);
  if ( it == account()->jobsEnd() ) return;
  if (job->error()) {
    account()->handleJobError( job, i18n("Error while retrieving messages.") );
    finishMailCheck( imapNoInformation );
    return;
  }
  if (lastSet) {
    finishMailCheck( imapFinished );
    account()->removeJob(it);
  }
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotGetLastMessagesResult(KJob * job)
{
  getMessagesResult(static_cast<KIO::Job*>(job), true);
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotGetMessagesResult(KJob * job)
{
  getMessagesResult(static_cast<KIO::Job*>(job), false);
}


//-----------------------------------------------------------------------------
void KMFolderImap::createFolder(const QString &name, const QString& parentPath,
                                bool askUser)
{
  kDebug(5006) << "KMFolderImap::createFolder - name=" << name << ",parent=" <<
    parentPath << ",askUser=" << askUser << endl;
  if ( account()->makeConnection() != ImapAccountBase::Connected ) {
    kWarning(5006) << "KMFolderImap::createFolder - got no connection" << endl;
    return;
  }
  KUrl url = account()->getUrl();
  QString parent = ( parentPath.isEmpty() ? imapPath() : parentPath );
  QString path = account()->createImapPath( parent, name );
  if ( askUser ) {
    path += "/;INFO=ASKUSER";
  }
  url.setPath( path );

  KIO::SimpleJob *job = KIO::mkdir(url);
  KIO::Scheduler::assignJobToSlave(account()->slave(), job);
  ImapAccountBase::jobData jd( url.url(), folder() );
  jd.items = QStringList( name );
  account()->insertJob(job, jd);
  connect(job, SIGNAL(result(KJob *)),
          this, SLOT(slotCreateFolderResult(KJob *)));
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotCreateFolderResult(KJob * job)
{
  ImapAccountBase::JobIterator it = account()->findJob(static_cast<KIO::Job*>(job));
  if ( it == account()->jobsEnd() ) return;

  QString name;
  if ( it.data().items.count() > 0 )
    name = it.data().items.first();

  if (job->error())
  {
    if ( job->error() == KIO::ERR_COULD_NOT_MKDIR ) {
      // Creating a folder failed, remove it from the tree.
      account()->listDirectory( );
    }
    account()->handleJobError( static_cast<KIO::Job*>(job), i18n("Error while creating a folder.") );
    emit folderCreationResult( name, false );
  } else {
    listDirectory();
    account()->removeJob(static_cast<KIO::Job*>(job));
    emit folderCreationResult( name, true );
  }
}


//-----------------------------------------------------------------------------
static QTextCodec *sUtf7Codec = 0;

QTextCodec * KMFolderImap::utf7Codec()
{
  if (!sUtf7Codec) sUtf7Codec = QTextCodec::codecForName("utf-7");
  return sUtf7Codec;
}


//-----------------------------------------------------------------------------
QString KMFolderImap::encodeFileName(const QString &name)
{
  QString result = utf7Codec()->fromUnicode(name);
  return KUrl::toPercentEncoding(result, "/");
}


//-----------------------------------------------------------------------------
QString KMFolderImap::decodeFileName(const QString &name)
{
  QString result = KUrl::fromPercentEncoding(name.toLatin1());
  return utf7Codec()->toUnicode(result.toLatin1());
}

//-----------------------------------------------------------------------------
bool KMFolderImap::autoExpunge()
{
  if (account())
    return account()->autoExpunge();

  return false;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotSimpleData(KIO::Job * job, const QByteArray & data)
{
  if ( data.isEmpty() ) return; // optimization
  ImapAccountBase::JobIterator it = account()->findJob(job);
  if ( it == account()->jobsEnd() ) return;
  QBuffer buff(&(*it).data);
  buff.open(QIODevice::WriteOnly | QIODevice::Append);
  buff.write(data.data(), data.size());
  buff.close();
}

//-----------------------------------------------------------------------------
void KMFolderImap::deleteMessage(KMMessage * msg)
{
  mUidMetaDataMap.remove( msg->UID() );
  mMetaDataMap.remove( msg->msgIdMD5() );
  KUrl url = account()->getUrl();
  KMFolderImap *msg_parent = static_cast<KMFolderImap*>(msg->storage());
  ulong uid = msg->UID();
  /* If the uid is empty the delete job below will nuke all mail in the
     folder, so we better safeguard against that. See ::expungeFolder, as
     to why. :( */
  if ( uid == 0 ) {
     kDebug( 5006 ) << "KMFolderImap::deleteMessage: Attempt to delete "
                        "an empty UID. Aborting."  << endl;
     return;
  }
  url.setPath(msg_parent->imapPath() + ";UID=" + QString::number(uid) );
  if ( account()->makeConnection() != ImapAccountBase::Connected )
    return;
  KIO::SimpleJob *job = KIO::file_delete(url, false);
  KIO::Scheduler::assignJobToSlave(account()->slave(), job);
  ImapAccountBase::jobData jd( url.url(), 0 );
  account()->insertJob(job, jd);
  connect(job, SIGNAL(result(KJob *)),
          account(), SLOT(slotSimpleResult(KJob *)));
}

void KMFolderImap::deleteMessage(const QList<KMMessage*>& msgList)
{
  QList<KMMessage*>::const_iterator it;
  KMMessage *msg;
  for ( it = msgList.begin(); it != msgList.end(); ++it ) {
    msg = (*it);
    mUidMetaDataMap.remove( msg->UID() );
    mMetaDataMap.remove( msg->msgIdMD5() );
  }

  QList<ulong> uids;
  getUids(msgList, uids);
  QStringList sets = makeSets(uids);

  KUrl url = account()->getUrl();
  KMFolderImap *msg_parent = static_cast<KMFolderImap*>(msgList.first()->storage());
  for ( QStringList::Iterator it = sets.begin(); it != sets.end(); ++it )
  {
    QString uid = *it;
    // Don't delete with no uid, that nukes the folder. Should not happen, but
    // better safe than sorry.
    if ( uid.isEmpty() ) continue;
    url.setPath(msg_parent->imapPath() + ";UID=" + uid);
    if ( account()->makeConnection() != ImapAccountBase::Connected )
      return;
    KIO::SimpleJob *job = KIO::file_delete(url, false);
    KIO::Scheduler::assignJobToSlave(account()->slave(), job);
    ImapAccountBase::jobData jd( url.url(), 0 );
    account()->insertJob(job, jd);
    connect(job, SIGNAL(result(KJob *)),
        account(), SLOT(slotSimpleResult(KJob *)));
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::setStatus(int idx, const MessageStatus& status, bool toggle)
{
  QList<int> ids; ids.append(idx);
  setStatus(ids, status, toggle);
}

void KMFolderImap::setStatus(QList<int>& ids, const MessageStatus& status, bool toggle)
{
  open();
  FolderStorage::setStatus(ids, status, toggle);
  if ( mReadOnly ) {
    return;
  }

  /* The status has been already set in the local index. Update the flags on
   * the server. To avoid doing that for each message individually, group them
   * by the status string they will be assigned and make sets for each of those
   * groups of mails. This is necessary because the imap kio_slave status job
   * does not append flags but overwrites them. Example:
   *
   * 2 important mails and 3 unimportant mail, all unread. Mark all as read calls
   * this method with a list of uids. The 2 important mails need to get the string
   * \SEEN \FLAGGED while the others need to get just \SEEN. Build sets for each
   * of those and sort them, so the server can handle them efficiently. */
  QMap< QString, QStringList > groups;
  for ( QList<int>::Iterator it = ids.begin(); it != ids.end(); ++it ) {
    KMMessage *msg = 0;
    bool unget = !isMessage(*it);
    msg = getMsg(*it);
    if (!msg) continue;
    QString flags = statusToFlags( msg->status() );
    // Collect uids for each type of flags.
    groups[flags].append(QString::number(msg->UID()));
    if (unget) unGetMsg(*it);
  }
  QMap< QString, QStringList >::Iterator dit;
  for ( dit = groups.begin(); dit != groups.end(); ++dit ) {
     QByteArray flags = dit.key().toLatin1();
     QStringList sets = makeSets( (*dit), true );
     // Send off a status setting job for each set.
     for (  QStringList::Iterator slit = sets.begin(); slit != sets.end(); ++slit ) {
       QString imappath = imapPath() + ";UID=" + ( *slit );
       account()->setImapStatus(folder(), imappath, flags);
     }
  }
  if ( mContentState == imapListingInProgress ) {
    // we're currently get'ing this folder
    // to make sure that we get the latest flags abort the current listing and
    // create a new one
    kDebug(5006) << "Set status during folder listing, restarting listing." << endl;
    disconnect(this, SLOT(slotListFolderResult(KJob *)));
    quiet( false );
    reallyGetFolder( QString() );
  }
  close();
}

//-----------------------------------------------------------------------------
QStringList KMFolderImap::makeSets(const QStringList& uids, bool sort)
{
  QList<ulong> tmp;
  for ( QStringList::ConstIterator it = uids.begin(); it != uids.end(); ++it )
    tmp.append( (*it).toInt() );
  return makeSets(tmp, sort);
}

QStringList KMFolderImap::makeSets( QList<ulong>& uids, bool sort )
{
  QStringList sets;
  QString set;

  if (uids.size() == 1)
  {
    sets.append(QString::number(uids.first()));
    return sets;
  }

  if (sort) qSort(uids);

  ulong last = 0;
  // needed to make a uid like 124 instead of 124:124
  bool inserted = false;
  /* iterate over uids and build sets like 120:122,124,126:150 */
  for ( QList<ulong>::Iterator it = uids.begin(); it != uids.end(); ++it )
  {
    if (it == uids.begin() || set.isEmpty()) {
      set = QString::number(*it);
      inserted = true;
    } else
    {
      if (last+1 != *it)
      {
        // end this range
        if (inserted)
          set += ',' + QString::number(*it);
        else
          set += ':' + QString::number(last) + ',' + QString::number(*it);
        inserted = true;
        if (set.length() > 100)
        {
          // just in case the server has a problem with longer lines..
          sets.append(set);
          set = "";
        }
      } else {
        inserted = false;
      }
    }
    last = *it;
  }
  // last element
  if (!inserted)
    set += ':' + QString::number(uids.last());

  if (!set.isEmpty()) sets.append(set);

  return sets;
}

//-----------------------------------------------------------------------------
void KMFolderImap::getUids(QList<int>& ids, QList<ulong>& uids)
{
  KMMsgBase *msg = 0;
  // get the uids
  for ( QList<int>::Iterator it = ids.begin(); it != ids.end(); ++it )
  {
    msg = getMsgBase(*it);
    if (!msg) continue;
    uids.append(msg->UID());
  }
}

void KMFolderImap::getUids(const QList<KMMessage*>& msgList, QList<ulong>& uids)
{
  QList<KMMessage*>::const_iterator it;
  for ( it = msgList.begin(); it != msgList.end(); ++it ) {
    if ( (*it)->UID() > 0 ) {
      uids.append( (*it)->UID() );
    }
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::expungeFolder(KMFolderImap * aFolder, bool quiet)
{
  aFolder->setNeedsCompacting(false);
  KUrl url = account()->getUrl();
  url.setPath(aFolder->imapPath() + ";UID=*");
  if ( account()->makeConnection() != ImapAccountBase::Connected )
    return;
  KIO::SimpleJob *job = KIO::file_delete(url, false);
  KIO::Scheduler::assignJobToSlave(account()->slave(), job);
  ImapAccountBase::jobData jd( url.url(), 0 );
  jd.quiet = quiet;
  account()->insertJob(job, jd);
  connect(job, SIGNAL(result(KJob *)),
          account(), SLOT(slotSimpleResult(KJob *)));
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotProcessNewMail( int errorCode, const QString &errorMsg )
{
  Q_UNUSED( errorMsg );
  disconnect( account(), SIGNAL( connectionResult(int, const QString&) ),
              this, SLOT( slotProcessNewMail(int, const QString&) ) );
  if ( !errorCode )
    processNewMail( false );
  else
    emit numUnreadMsgsChanged( folder() );
}

//-----------------------------------------------------------------------------
bool KMFolderImap::processNewMail(bool)
{
   // a little safety
  if ( !account() ) {
    kDebug(5006) << "KMFolderImap::processNewMail - account is null!" << endl;
    return false;
  }
  if ( imapPath().isEmpty() ) {
    kDebug(5006) << "KMFolderImap::processNewMail - imapPath of " << objectName() << " is empty!" << endl;
    // remove it locally
    setAlreadyRemoved( true );
    kmkernel->imapFolderMgr()->remove( folder() );
    return false;
  }
  // check the connection
  if ( account()->makeConnection() == ImapAccountBase::Error ) {
    kDebug(5006) << "KMFolderImap::processNewMail - got no connection!" << endl;
    return false;
  } else if ( account()->makeConnection() == ImapAccountBase::Connecting )
  {
    // wait
    kDebug(5006) << "KMFolderImap::processNewMail - waiting for connection: " << label() << endl;
    connect( account(), SIGNAL( connectionResult(int, const QString&) ),
        this, SLOT( slotProcessNewMail(int, const QString&) ) );
    return true;
  }
  KUrl url = account()->getUrl();
  if (mReadOnly)
    url.setPath(imapPath() + ";SECTION=UIDNEXT");
  else
    url.setPath(imapPath() + ";SECTION=UNSEEN");

  mMailCheckProgressItem = ProgressManager::createProgressItem(
              "MailCheckAccount" + account()->name(),
              "MailCheck" + folder()->prettyUrl(),
              folder()->prettyUrl(),
              i18n("updating message counts"),
              false,
              account()->useSSL() || account()->useTLS() );

  KIO::SimpleJob *job = KIO::stat(url, false);
  KIO::Scheduler::assignJobToSlave(account()->slave(), job);
  ImapAccountBase::jobData jd(url.url(), folder() );
  jd.cancellable = true;
  account()->insertJob(job, jd);
  connect(job, SIGNAL(result(KJob *)),
          SLOT(slotStatResult(KJob *)));
  return true;
}


//-----------------------------------------------------------------------------
void KMFolderImap::slotStatResult(KJob * job)
{
  slotCompleteMailCheckProgress();
  ImapAccountBase::JobIterator it = account()->findJob(static_cast<KIO::Job*>(job));
  if ( it == account()->jobsEnd() ) return;
  account()->removeJob(it);
  if (job->error())
  {
    account()->handleJobError( static_cast<KIO::Job*>(job), i18n("Error while getting folder information.") );
  } else {
    KIO::UDSEntry uds = static_cast<KIO::StatJob*>(job)->statResult();
    const long long count = uds.numberValue( KIO::UDS_SIZE );
    if ( mReadOnly ) {
      mGuessedUnreadMsgs = -1;
      mGuessedUnreadMsgs = countUnread() + count - lastUid() - 1;
      if (mGuessedUnreadMsgs < 0) mGuessedUnreadMsgs = 0;
    } else {
      mGuessedUnreadMsgs = count;
    }
  }
}

//-----------------------------------------------------------------------------
int KMFolderImap::create()
{
  readConfig();
  mUnreadMsgs = -1;
  return KMFolderMbox::create();
}

QList<ulong> KMFolderImap::splitSets(const QString uids)
{
  QList<ulong> uidlist;

  // ex: 1205,1204,1203,1202,1236:1238
  QString buffer = QString();
  int setstart = -1;
  // iterate over the uids
  for (int i = 0; i < uids.length(); i++)
  {
    QChar chr = uids[i];
    if (chr == ',')
    {
      if (setstart > -1)
      {
        // a range (uid:uid) was before
        for (int j = setstart; j <= buffer.toInt(); j++)
        {
          uidlist.append(j);
        }
        setstart = -1;
      } else {
        // single uid
        uidlist.append(buffer.toInt());
      }
      buffer = "";
    } else if (chr == ':') {
      // remember the start of the range
      setstart = buffer.toInt();
      buffer = "";
    } else if (chr.category() == QChar::Number_DecimalDigit) {
      // digit
      buffer += chr;
    } else {
      // ignore
    }
  }
  // process the last data
  if (setstart > -1)
  {
    for (int j = setstart; j <= buffer.toInt(); j++)
    {
      uidlist.append(j);
    }
  } else {
    uidlist.append(buffer.toInt());
  }

  return uidlist;
}

//-----------------------------------------------------------------------------
int KMFolderImap::expungeContents()
{
  // nuke the local cache
  int rc = KMFolderMbox::expungeContents();

  // set the deleted flag for all messages in the folder
  KUrl url = account()->getUrl();
  url.setPath( imapPath() + ";UID=1:*");
  if ( account()->makeConnection() == ImapAccountBase::Connected )
  {
    KIO::SimpleJob *job = KIO::file_delete(url, false);
    KIO::Scheduler::assignJobToSlave(account()->slave(), job);
    ImapAccountBase::jobData jd( url.url(), 0 );
    jd.quiet = true;
    account()->insertJob(job, jd);
    connect(job, SIGNAL(result(KJob *)),
            account(), SLOT(slotSimpleResult(KJob *)));
  }
  /* Is the below correct? If we are expunging (in the folder sense, not the imap sense),
     why delete but not (imap-)expunge? Since the folder is not active there is no concept
     of "leaving the folder", so the setting really has little to do with it. */
  // if ( autoExpunge() )
    expungeFolder(this, true);
  getFolder();

  return rc;
}

//-----------------------------------------------------------------------------
void
KMFolderImap::setUserRights( unsigned int userRights )
{
  mUserRights = userRights;
  kDebug(5006) << imapPath() << " setUserRights: " << userRights << endl;
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotCompleteMailCheckProgress()
{
  if ( mMailCheckProgressItem ) {
    mMailCheckProgressItem->setComplete();
    mMailCheckProgressItem = 0;
    emit numUnreadMsgsChanged( folder() );
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::setSubfolderState( imapState state )
{
  mSubfolderState = state;
  if ( state == imapNoInformation && folder()->child() )
  {
    // pass through to children
    KMFolderNode* node;
    QList<KMFolderNode*>::const_iterator it;
    for ( it = folder()->child()->begin();
        ( node = *it ) && it != folder()->child()->end(); ++it )
    {
      ++it;
      if (node->isDir()) continue;
      KMFolder *folder = static_cast<KMFolder*>(node);
      static_cast<KMFolderImap*>(folder->storage())->setSubfolderState( state );
    }
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::setIncludeInMailCheck( bool check )
{
  bool changed = ( mCheckMail != check );
  mCheckMail = check;
  if ( changed )
    account()->slotUpdateFolderList();
}

//-----------------------------------------------------------------------------
void KMFolderImap::setAlreadyRemoved( bool removed )
{
  mAlreadyRemoved = removed;
  if ( folder()->child() )
  {
    // pass through to children
    KMFolderNode* node;
    QList<KMFolderNode*>::const_iterator it;
    for ( it = folder()->child()->begin();
        ( node = *it ) && it != folder()->child()->end(); ++it )
    {
      ++it;
      if (node->isDir()) continue;
      KMFolder *folder = static_cast<KMFolder*>(node);
      static_cast<KMFolderImap*>(folder->storage())->setAlreadyRemoved( removed );
    }
  }
}

void KMFolderImap::slotCreatePendingFolders( int errorCode, const QString& errorMsg )
{
  Q_UNUSED( errorMsg );
  disconnect( account(), SIGNAL( connectionResult( int, const QString& ) ),
              this, SLOT( slotCreatePendingFolders( int, const QString& ) ) );
  if ( !errorCode ) {
    QStringList::Iterator it = mFoldersPendingCreation.begin();
    for ( ; it != mFoldersPendingCreation.end(); ++it ) {
      createFolder( *it );
    }
  }
  mFoldersPendingCreation.clear();
}

//-----------------------------------------------------------------------------
void KMFolderImap::search( const KMSearchPattern* pattern )
{
  if ( !pattern || pattern->isEmpty() )
  {
    // not much to do here
    QList<quint32> serNums;
    emit searchResult( folder(), serNums, pattern, true );
    return;
  }
  SearchJob* job = new SearchJob( this, account(), pattern );
  connect( job, SIGNAL( searchDone( QList<quint32>, const KMSearchPattern*, bool ) ),
           this, SLOT( slotSearchDone( QList<quint32>, const KMSearchPattern*, bool ) ) );
  job->start();
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotSearchDone( QList<quint32> serNums,
                                   const KMSearchPattern* pattern,
                                   bool complete )
{
  emit searchResult( folder(), serNums, pattern, complete );
}

//-----------------------------------------------------------------------------
void KMFolderImap::search( const KMSearchPattern* pattern, quint32 serNum )
{
  if ( !pattern || pattern->isEmpty() )
  {
    // not much to do here
    emit searchDone( folder(), serNum, pattern, false );
    return;
  }
  SearchJob* job = new SearchJob( this, account(), pattern, serNum );
  connect( job, SIGNAL( searchDone( quint32, const KMSearchPattern*, bool ) ),
           this, SLOT( slotSearchDone( quint32, const KMSearchPattern*, bool ) ) );
  job->start();
}

//-----------------------------------------------------------------------------
void KMFolderImap::slotSearchDone( quint32 serNum, const KMSearchPattern* pattern,
                                   bool matches )
{
  emit searchDone( folder(), serNum, pattern, matches );
}

//-----------------------------------------------------------------------------
bool KMFolderImap::isMoveable() const
{
  return ( hasChildren() == HasNoChildren &&
      !folder()->isSystemFolder() ) ? true : false;
}

//-----------------------------------------------------------------------------
const ulong KMFolderImap::serNumForUID( ulong uid )
{
  if ( mUidMetaDataMap.find( uid ) ) {
    KMMsgMetaData *md = mUidMetaDataMap[uid];
    return md->serNum();
  } else {
    kDebug(5006) << "serNumForUID: unknown uid " << uid << endl;
    return 0;
  }
}

//-----------------------------------------------------------------------------
void KMFolderImap::saveMsgMetaData( KMMessage* msg, ulong uid )
{
  if ( uid == 0 ) {
    uid = msg->UID();
  }
  ulong serNum = msg->getMsgSerNum();
  mUidMetaDataMap.replace( uid, new KMMsgMetaData( msg->status(), serNum ) );
}

//-----------------------------------------------------------------------------
void KMFolderImap::setImapPath( const QString& path )
{
  if ( path.isEmpty() ) {
    kWarning(5006) << k_funcinfo << "ignoring empty path" << endl;
  } else {
    mImapPath = path;
  }
}

void KMFolderImap::finishMailCheck( imapState state )
{
  quiet( false );
  mContentState = state;
  emit folderComplete( this, mContentState == imapFinished );
  close();
}

#include "kmfolderimap.moc"
