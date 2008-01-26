/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2000 Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

//Factor byteswap stuff into one header file

#include <config.h>

#include "kmfoldersearch.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmsearchpattern.h"
#include "kmmsgdict.h"
#include "index.h"
#include "jobscheduler.h"

#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <utime.h>

#include <qfile.h>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 32 bit value.  */
#ifndef kmail_swap_32
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |               \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif
#endif // kmail_swap_32

// Current version of the .index.search files
#define IDS_SEARCH_VERSION 1000
// The asterisk at the end is important
#define IDS_SEARCH_HEADER "# KMail-Search-IDs V%d\n*"
#define IDS_SEARCH_HEADER_LEN 30


KMSearch::KMSearch(QObject * parent, const char * name)
    :QObject(parent, name)
{
    mRemainingFolders = -1;
    mRecursive = true;
    mRunByIndex = mRunning = false;
    mRoot = 0;
    mSearchPattern = 0;
    mFoundCount = 0;
    mSearchCount = 0;

    mProcessNextBatchTimer = new QTimer(0, "mProcessNextBatchTimer");
    connect(mProcessNextBatchTimer, SIGNAL(timeout()), this, SLOT(slotProcessNextBatch()));
}

KMSearch::~KMSearch()
{
    delete mProcessNextBatchTimer;
    delete mSearchPattern;
}

bool KMSearch::write(QString location) const
{
    KConfig config(location);
    config.setGroup("Search Folder");
    if (mSearchPattern)
        mSearchPattern->writeConfig(&config);
    if (mRoot.isNull())
        config.writeEntry("Base", "");
    else
        config.writeEntry("Base", mRoot->idString());
    config.writeEntry("Recursive", recursive());
    return true;
}

bool KMSearch::read(QString location)
{
    KConfig config( location );
    config.setGroup( "Search Folder" );
    if ( !mSearchPattern )
        mSearchPattern = new KMSearchPattern();
    mSearchPattern->readConfig( &config );
    QString rootString = config.readEntry( "Base" );
    mRoot = kmkernel->findFolderById( rootString );
    mRecursive = config.readBoolEntry( "Recursive" );
    return true;
}

void KMSearch::setSearchPattern(KMSearchPattern *searchPattern)
{
    if ( running() )
        stop();
    if ( mSearchPattern != searchPattern ) {
        delete mSearchPattern;
        mSearchPattern = searchPattern;
    }
}

bool KMSearch::inScope(KMFolder* folder) const
{
    if ( mRoot.isNull() || folder == mRoot )
        return true;
    if ( !recursive() )
        return false;

    KMFolderDir *rootDir = mRoot->child();
    KMFolderDir *ancestorDir = folder->parent();
    while ( ancestorDir ) {
        if ( ancestorDir == rootDir )
            return true;
        ancestorDir = ancestorDir->parent();
    }
    return false;
}

void KMSearch::start()
{
    //close all referenced folders
    QValueListIterator<QGuardedPtr<KMFolder> > fit;
    for (fit = mOpenedFolders.begin(); fit != mOpenedFolders.end(); ++fit) {
        if (!(*fit))
            continue;
        (*fit)->close( "kmsearch" );
    }
    mOpenedFolders.clear();
    mFolders.clear();

    if ( running() )
        return;

    if ( !mSearchPattern ) {
        emit finished(true);
        return;
    }

    mFoundCount = 0;
    mSearchCount = 0;
    mRunning = true;
    mRunByIndex = false;
    // check if this query can be done with the index
    if ( kmkernel->msgIndex() && kmkernel->msgIndex()->startQuery( this ) ) {
        mRunByIndex = true;
        return;
    }

    mFolders.append( mRoot );
    if ( recursive() )
    {
        //Append all descendants to folders
        KMFolderNode* node;
        KMFolder* folder;
        QValueListConstIterator<QGuardedPtr<KMFolder> > it;
        for ( it = mFolders.begin(); it != mFolders.end(); ++it )
        {
            folder = *it;
            KMFolderDir *dir = 0;
            if ( folder )
                dir = folder->child();
            else
                dir = &kmkernel->folderMgr()->dir();
            if ( !dir )
                continue;
            QPtrListIterator<KMFolderNode> it(*dir);
            while ( (node = it.current()) ) {
                ++it;
                if ( !node->isDir() ) {
                    KMFolder* kmf = dynamic_cast<KMFolder*>( node );
                    if ( kmf )
                        mFolders.append( kmf );
                }
            }
        }
    }

    mRemainingFolders = mFolders.count();
    mLastFolder = QString::null;
    mProcessNextBatchTimer->start( 0, true );
}

void KMSearch::stop()
{
    if ( !running() )
        return;
    if ( mRunByIndex ) {
        if ( kmkernel->msgIndex() )
            kmkernel->msgIndex()->stopQuery( this );
    } else {
        mIncompleteFolders.clear();
        QValueListConstIterator<QGuardedPtr<KMFolder> > jt;
        for ( jt = mOpenedFolders.begin(); jt != mOpenedFolders.end(); ++jt ) {
            KMFolder *folder = *jt;
            if ( !folder )
                continue;
            // explicitely stop jobs for this folder as it will not be closed below
            // when the folder is currently selected
            if ( folder->folderType() == KMFolderTypeImap ) {
                KMAcctImap *account =
                    static_cast<KMFolderImap*>( folder->storage() )->account();
                account->ignoreJobsForFolder( folder );
            }
            folder->storage()->search( 0 );
            mSearchCount += folder->count();
            folder->close("kmsearch");
        }
    }
    mRemainingFolders = -1;
    mOpenedFolders.clear();
    mFolders.clear();
    mLastFolder = QString::null;
    mRunByIndex = mRunning = false;
    emit finished(false);
}

void KMSearch::indexFinished() {
    mRunning = false;
    mRunByIndex = false;
}

void KMSearch::slotProcessNextBatch()
{
    if ( !running() )
        return;

    if ( mFolders.count() != 0 )
    {
        KMFolder *folder = *( mFolders.begin() );
        mFolders.erase( mFolders.begin() );
        if ( folder )
        {
            mLastFolder = folder->label();
            folder->open("kmsearch");
            mOpenedFolders.append( folder );
            connect( folder->storage(),
                SIGNAL( searchResult( KMFolder*, QValueList<Q_UINT32>, const KMSearchPattern*, bool ) ),
                this,
                SLOT( slotSearchFolderResult( KMFolder*, QValueList<Q_UINT32>, const KMSearchPattern*, bool ) ) );
            folder->storage()->search( mSearchPattern );
        } else
            --mRemainingFolders;
        mProcessNextBatchTimer->start( 0, true );
        return;
    }
}

void KMSearch::slotSearchFolderResult( KMFolder* folder,
                                       QValueList<Q_UINT32> serNums,
                                       const KMSearchPattern* pattern,
                                       bool complete )
{
    if ( pattern != mSearchPattern )
      return;
    kdDebug(5006) << k_funcinfo << folder->label() << " found " << serNums.count() << endl;
    mLastFolder = folder->label();
    QValueListIterator<Q_UINT32> it;
    for ( it = serNums.begin(); it != serNums.end(); ++it )
    {
      emit found( *it );
      ++mFoundCount;
    }
    if ( complete )
    {
      disconnect( folder->storage(),
          SIGNAL( searchResult( KMFolder*, QValueList<Q_UINT32>,
                                const KMSearchPattern*, bool ) ),
          this,
          SLOT( slotSearchFolderResult( KMFolder*, QValueList<Q_UINT32>,
                                        const KMSearchPattern*, bool ) ) );
      --mRemainingFolders;
      mSearchCount += folder->count();
      folder->close("kmsearch");
      mOpenedFolders.remove( folder );
      if ( mRemainingFolders <= 0 )
      {
        mRemainingFolders = 0;
        mRunning = false;
        mLastFolder = QString::null;
        mRemainingFolders = -1;
        mFolders.clear();
        emit finished( true );
      }
    }
}

//-----------------------------------------------------------------------------
KMFolderSearch::KMFolderSearch(KMFolder* folder, const char* name)
  : FolderStorage(folder, name)
{
    mIdsStream = 0;
    mSearch = 0;
    mInvalid = false;
    mUnlinked = true;
    mTempOpened = false;
    setNoChildren(true);

    //Hook up some slots for live updating of search folders
    //TODO: Optimize folderInvalidated, folderAdded, folderRemoved
    connect(kmkernel->folderMgr(), SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
            this, SLOT(examineAddedMessage(KMFolder*, Q_UINT32)));
    connect(kmkernel->folderMgr(), SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
            this, SLOT(examineRemovedMessage(KMFolder*, Q_UINT32)));
    connect(kmkernel->folderMgr(), SIGNAL(msgChanged(KMFolder*, Q_UINT32, int)),
            this, SLOT(examineChangedMessage(KMFolder*, Q_UINT32, int)));
    connect(kmkernel->folderMgr(), SIGNAL(folderInvalidated(KMFolder*)),
            this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kmkernel->folderMgr(), SIGNAL(folderAdded(KMFolder*)),
            this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kmkernel->folderMgr(), SIGNAL(folderRemoved(KMFolder*)),
            this, SLOT(examineRemovedFolder(KMFolder*)));
    connect(kmkernel->folderMgr(), SIGNAL(msgHeaderChanged(KMFolder*,int)),
            this, SLOT(propagateHeaderChanged(KMFolder*,int)));

    connect(kmkernel->imapFolderMgr(), SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
            this, SLOT(examineAddedMessage(KMFolder*, Q_UINT32)));
    connect(kmkernel->imapFolderMgr(), SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
            this, SLOT(examineRemovedMessage(KMFolder*, Q_UINT32)));
    connect(kmkernel->imapFolderMgr(), SIGNAL(msgChanged(KMFolder*, Q_UINT32, int)),
            this, SLOT(examineChangedMessage(KMFolder*, Q_UINT32, int)));
    connect(kmkernel->imapFolderMgr(), SIGNAL(folderInvalidated(KMFolder*)),
            this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kmkernel->imapFolderMgr(), SIGNAL(folderAdded(KMFolder*)),
            this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kmkernel->imapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
            this, SLOT(examineRemovedFolder(KMFolder*)));
    connect(kmkernel->imapFolderMgr(), SIGNAL(msgHeaderChanged(KMFolder*,int)),
            this, SLOT(propagateHeaderChanged(KMFolder*,int)));

    connect(kmkernel->dimapFolderMgr(), SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
            this, SLOT(examineAddedMessage(KMFolder*, Q_UINT32)));
    connect(kmkernel->dimapFolderMgr(), SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
            this, SLOT(examineRemovedMessage(KMFolder*, Q_UINT32)));
    connect(kmkernel->dimapFolderMgr(), SIGNAL(msgChanged(KMFolder*, Q_UINT32, int)),
            this, SLOT(examineChangedMessage(KMFolder*, Q_UINT32, int)));
    connect(kmkernel->dimapFolderMgr(), SIGNAL(folderInvalidated(KMFolder*)),
            this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kmkernel->dimapFolderMgr(), SIGNAL(folderAdded(KMFolder*)),
            this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kmkernel->dimapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
            this, SLOT(examineRemovedFolder(KMFolder*)));
    connect(kmkernel->dimapFolderMgr(), SIGNAL(msgHeaderChanged(KMFolder*,int)),
            this, SLOT(propagateHeaderChanged(KMFolder*,int)));

  mExecuteSearchTimer = new QTimer(0, "mExecuteSearchTimer");
  connect(mExecuteSearchTimer, SIGNAL(timeout()),
          this, SLOT(executeSearch()));
}

KMFolderSearch::~KMFolderSearch()
{
    delete mExecuteSearchTimer;
    delete mSearch;
    mSearch = 0;
    if (mOpenCount > 0)
        close("~foldersearch", TRUE);
}

void KMFolderSearch::setSearch(KMSearch *search)
{
    truncateIndex(); //new search old index is obsolete
    emit cleared();
    mInvalid = false;
    setDirty( true ); //have to write the index
    if (!mUnlinked) {
        unlink(QFile::encodeName(indexLocation()));
        mUnlinked = true;
    }
    if (mSearch != search) {
        mSearch->stop();
        delete mSearch;
        mSearch = search; // take ownership
        if (mSearch) {
            QObject::connect(search, SIGNAL(found(Q_UINT32)),
                    SLOT(addSerNum(Q_UINT32)));
            QObject::connect(search, SIGNAL(finished(bool)),
                    SLOT(searchFinished(bool)));
        }
    }
    if (mSearch)
        mSearch->write(location());
    clearIndex();
    mTotalMsgs = 0;
    mUnreadMsgs = 0;
    emit numUnreadMsgsChanged( folder() );
    emit changed(); // really want a kmfolder cleared signal
    /* TODO There is KMFolder::cleared signal now. Adjust. */
    if (mSearch)
        mSearch->start();
    open("foldersearch"); // will be closed in searchFinished
}

void KMFolderSearch::executeSearch()
{
    if (mSearch)
        mSearch->stop();
    setSearch(mSearch);
    invalidateFolder();
}

const KMSearch* KMFolderSearch::search() const
{
    return mSearch;
}

void KMFolderSearch::searchFinished(bool success)
{
    if (!success)
        mSerNums.clear();
    close("foldersearch");
}

void KMFolderSearch::addSerNum(Q_UINT32 serNum)
{
    if (mInvalid) // A new search is scheduled don't bother doing anything
        return;
    int idx = -1;
    KMFolder *aFolder = 0;
    KMMsgDict::instance()->getLocation(serNum, &aFolder, &idx);
    // warn instead of assert() because of
    // https://intevation.de/roundup/kolab/issue2216
    if (!aFolder || (idx == -1)) {
        kdDebug(5006) << "Not adding message with serNum " << serNum
                      << ": folder is " << aFolder << ", index is " << idx << endl;
        return;
    }
    if(mFolders.findIndex(aFolder) == -1) {
        aFolder->open("foldersearch");
        mFolders.append(aFolder);
    }
    setDirty( true ); //TODO append a single entry to .ids file and sync.
    if (!mUnlinked) {
        unlink(QFile::encodeName(indexLocation()));
        mUnlinked = true;
    }
    mSerNums.append(serNum);
    KMMsgBase *mb = aFolder->getMsgBase(idx);
    if (mb && (mb->isUnread() || mb->isNew())) {
       if (mUnreadMsgs == -1)
           mUnreadMsgs = 0;
       ++mUnreadMsgs;
       emit numUnreadMsgsChanged( folder() );
    }
    emitMsgAddedSignals(mSerNums.count()-1);
}

void KMFolderSearch::removeSerNum(Q_UINT32 serNum)
{
    QValueVector<Q_UINT32>::const_iterator it;
    int i = 0;
    for(it = mSerNums.begin(); it != mSerNums.end(); ++it, ++i)
        if ((*it) == serNum) {
            int idx = -1;
            KMFolder *aFolder = 0;
            KMMsgDict::instance()->getLocation(serNum, &aFolder, &idx);
            assert(aFolder && (idx != -1));
            emit msgRemoved(folder(), serNum);
            removeMsg(i);
            return;
        }
    if (!mUnlinked) {
        unlink(QFile::encodeName(indexLocation()));
        mUnlinked = true;
    }
}

int KMFolderSearch::addMsg(KMMessage*, int* index_return)
{
    //Not supported search folder can't own messages
    *index_return = -1;
    return 0;
}

bool KMFolderSearch::readSearch()
{
    mSearch = new KMSearch;
    QObject::connect(mSearch, SIGNAL(found(Q_UINT32)), SLOT(addSerNum(Q_UINT32)));
    QObject::connect(mSearch, SIGNAL(finished(bool)), SLOT(searchFinished(bool)));
    return mSearch->read(location());
}

int KMFolderSearch::open(const char *)
{
    mOpenCount++;
    kmkernel->jobScheduler()->notifyOpeningFolder( folder() );
    if (mOpenCount > 1)
        return 0;  // already open

    readConfig();
    if (!mSearch && !readSearch())
        return -1;

    emit cleared();
    if (!mSearch || !search()->running())
        if (!readIndex()) {
            executeSearch();
        }

    return 0;
}

int KMFolderSearch::canAccess()
{
    assert(!folder()->name().isEmpty());

    if (access(QFile::encodeName(location()), R_OK | W_OK | X_OK) != 0)
        return 1;
    return 0;
}

void KMFolderSearch::sync()
{
    if (mDirty) {
        if (mSearch)
            mSearch->write(location());
        updateIndex();
    }
}

void KMFolderSearch::reallyDoClose(const char* owner)
{
    if (mAutoCreateIndex) {
        if (mSearch)
            mSearch->write(location());
        updateIndex();
        if (mSearch && search()->running())
            mSearch->stop();
        writeConfig();
    }

    //close all referenced folders
    QValueListIterator<QGuardedPtr<KMFolder> > fit;
    for (fit = mFolders.begin(); fit != mFolders.end(); ++fit) {
        if (!(*fit))
            continue;
        (*fit)->close("foldersearch");
    }
    mFolders.clear();

    clearIndex(TRUE);

    if (mIdsStream)
        fclose(mIdsStream);

    mOpenCount   = 0;
    mIdsStream = 0;
    mUnreadMsgs  = -1;
}

int KMFolderSearch::create()
{
    int old_umask;
    int rc = unlink(QFile::encodeName(location()));
    if (!rc)
        return rc;
    rc = 0;

    assert(!folder()->name().isEmpty());
    assert(mOpenCount == 0);

    kdDebug(5006) << "Creating folder " << location() << endl;
    if (access(QFile::encodeName(location()), F_OK) == 0) {
        kdDebug(5006) << "KMFolderSearch::create call to access function failed."
            << endl;
        return EEXIST;
    }

    old_umask = umask(077);
    FILE *mStream = fopen(QFile::encodeName(location()), "w+");
    umask(old_umask);
    if (!mStream) return errno;
    fclose(mStream);

    clearIndex();
    if (!mSearch) {
        mSearch = new KMSearch();
        QObject::connect(mSearch, SIGNAL(found(Q_UINT32)), SLOT(addSerNum(Q_UINT32)));
        QObject::connect(mSearch, SIGNAL(finished(bool)), SLOT(searchFinished(bool)));
    }
    mSearch->write(location());
    mOpenCount++;
    mChanged = false;
    mUnreadMsgs = 0;
    mTotalMsgs = 0;
    return rc;
}

int KMFolderSearch::compact( bool )
{
    needsCompact = false;
    return 0;
}

bool KMFolderSearch::isReadOnly() const
{
    return false; //TODO: Make it true and get that working ok
}

FolderJob* KMFolderSearch::doCreateJob(KMMessage*, FolderJob::JobType,
                                     KMFolder*, QString, const AttachmentStrategy* ) const
{
    // Should never be called
    assert(0);
    return 0;
}

FolderJob* KMFolderSearch::doCreateJob(QPtrList<KMMessage>&, const QString&,
                                       FolderJob::JobType, KMFolder*) const
{
    // Should never be called
    assert(0);
    return 0;
}

const KMMsgBase* KMFolderSearch::getMsgBase(int idx) const
{
    int folderIdx = -1;
    KMFolder *folder = 0;
    if (idx < 0 || (Q_UINT32)idx >= mSerNums.count())
        return 0;
    KMMsgDict::instance()->getLocation(mSerNums[idx], &folder, &folderIdx);
    assert(folder && (folderIdx != -1));
    return folder->getMsgBase(folderIdx);
}

KMMsgBase* KMFolderSearch::getMsgBase(int idx)
{
    int folderIdx = -1;
    KMFolder *folder = 0;
    if (idx < 0 || (Q_UINT32)idx >= mSerNums.count())
        return 0;
    KMMsgDict::instance()->getLocation(mSerNums[idx], &folder, &folderIdx);
    if (!folder || folderIdx == -1)
        return 0; //exceptional case
    return folder->getMsgBase(folderIdx);
}

//-----------------------------------------------------------------------------
KMMessage* KMFolderSearch::getMsg(int idx)
{
    int folderIdx = -1;
    KMFolder *folder = 0;
    if (idx < 0 || (Q_UINT32)idx >= mSerNums.count())
        return 0;
    KMMsgDict::instance()->getLocation(mSerNums[idx], &folder, &folderIdx);
    assert(folder && (folderIdx != -1));
    KMMessage* msg = folder->getMsg( folderIdx );
    return msg;
}

//-------------------------------------------------------------
void
KMFolderSearch::ignoreJobsForMessage( KMMessage* msg )
{
    if ( !msg || msg->transferInProgress() )
        return;
    /* While non-imap folders manage their jobs themselves, imap ones let
       their account manage them. Therefor first clear the jobs managed by
       this folder via the inherited method, then clear the imap ones. */
    FolderStorage::ignoreJobsForMessage( msg );

    if (msg->parent()->folderType() == KMFolderTypeImap) {
        KMAcctImap *account =
            static_cast<KMFolderImap*>( msg->storage() )->account();
        if( !account )
            return;
        account->ignoreJobsForMessage( msg );
    }
}


int KMFolderSearch::find(const KMMsgBase* msg) const
{
    int pos = 0;
    Q_UINT32 serNum = msg->getMsgSerNum();
    QValueVector<Q_UINT32>::const_iterator it;
    for(it = mSerNums.begin(); it != mSerNums.end(); ++it) {
        if ((*it) == serNum)
            return pos;
        ++pos;
    }
    return -1;
}

QString KMFolderSearch::indexLocation() const
{
    QString sLocation(folder()->path());

    if (!sLocation.isEmpty()) sLocation += '/';
    sLocation += '.';
    sLocation += dotEscape(fileName());
    sLocation += ".index";
    sLocation += ".search";

    return sLocation;
}

int KMFolderSearch::updateIndex()
{
    if (mSearch && search()->running())
        unlink(QFile::encodeName(indexLocation()));
    else
        if (dirty())
            return writeIndex();
    return 0;
}

int KMFolderSearch::writeIndex( bool )
{
    // TODO:If we fail to write the index we should panic the kernel
    // TODO:and the same for other folder types too, and the msgDict.
    QString filename = indexLocation();
    int old_umask = umask(077);
    QString tempName = filename + ".temp";
    unlink(QFile::encodeName(tempName));

    // We touch the folder, otherwise the index is regenerated, if KMail is
    // running, while the clock switches from daylight savings time to normal time
    utime(QFile::encodeName(location()), 0);

    FILE *tmpIndexStream = fopen(QFile::encodeName(tempName), "w");
    umask(old_umask);

    if (!tmpIndexStream) {
        kdDebug(5006) << "Cannot write '" << filename
            << strerror(errno) << " (" << errno << ")" << endl;
        truncate(QFile::encodeName(filename), 0);
        return -1;
    }
    fprintf(tmpIndexStream, IDS_SEARCH_HEADER, IDS_SEARCH_VERSION);
    Q_UINT32 byteOrder = 0x12345678;
    fwrite(&byteOrder, sizeof(byteOrder), 1, tmpIndexStream);

    Q_UINT32 count = mSerNums.count();
    if (!fwrite(&count, sizeof(count), 1, tmpIndexStream)) {
        fclose(tmpIndexStream);
        truncate(QFile::encodeName(filename), 0);
        return -1;
    }

    QValueVector<Q_UINT32>::iterator it;
    for(it = mSerNums.begin(); it != mSerNums.end(); ++it) {
        Q_UINT32 serNum = *it;
        if (!fwrite(&serNum, sizeof(serNum), 1, tmpIndexStream))
            return -1;
    }
    if (ferror(tmpIndexStream)) return ferror(tmpIndexStream);
    if (fflush(tmpIndexStream) != 0) return errno;
    if (fsync(fileno(tmpIndexStream)) != 0) return errno;
    if (fclose(tmpIndexStream) != 0) return errno;

    ::rename(QFile::encodeName(tempName), QFile::encodeName(indexLocation()));
    mDirty = FALSE;
    mUnlinked = FALSE;

    return 0;
}

DwString KMFolderSearch::getDwString(int idx)
{
    return getMsgBase(idx)->parent()->getDwString( idx );
}

KMMessage* KMFolderSearch::readMsg(int idx)
{
    int folderIdx = -1;
    KMFolder *folder = 0;
    KMMsgDict::instance()->getLocation(mSerNums[idx], &folder, &folderIdx);
    assert(folder && (folderIdx != -1));
    return folder->getMsg( folderIdx );
}

bool KMFolderSearch::readIndex()
{
    clearIndex();
    QString filename = indexLocation();
    mIdsStream = fopen(QFile::encodeName(filename), "r+");
    if (!mIdsStream)
        return false;

    int version = 0;
    fscanf(mIdsStream, IDS_SEARCH_HEADER, &version);
    if (version != IDS_SEARCH_VERSION) {
        fclose(mIdsStream);
        mIdsStream = 0;
        return false;
    }
    bool swapByteOrder;
    Q_UINT32 byte_order;
    if (!fread(&byte_order, sizeof(byte_order), 1, mIdsStream)) {
        fclose(mIdsStream);
        mIdsStream = 0;
        return false;
    }
    swapByteOrder = (byte_order == 0x78563412);

    Q_UINT32 count;
    if (!fread(&count, sizeof(count), 1, mIdsStream)) {
        fclose(mIdsStream);
        mIdsStream = 0;
        return false;
    }
    if (swapByteOrder)
        count = kmail_swap_32(count);

    mUnreadMsgs = 0;
    mSerNums.reserve(count);
    for (unsigned int index = 0; index < count; index++) {
        Q_UINT32 serNum;
        int folderIdx = -1;
        KMFolder *folder = 0;
        bool readOk = fread(&serNum, sizeof(serNum), 1, mIdsStream);
        if (!readOk) {
            clearIndex();
            fclose(mIdsStream);
            mIdsStream = 0;
            return false;
        }
        if (swapByteOrder)
            serNum = kmail_swap_32(serNum);

        KMMsgDict::instance()->getLocation( serNum, &folder, &folderIdx );
        if (!folder || (folderIdx == -1)) {
            clearIndex();
            fclose(mIdsStream);
            mIdsStream = 0;
            return false;
        }
        mSerNums.push_back(serNum);
        if(mFolders.findIndex(folder) == -1) {
            if (mInvalid) //exceptional case for when folder has invalid ids
                return false;
            folder->open("foldersearch");
            mFolders.append(folder);
        }
        KMMsgBase *mb = folder->getMsgBase(folderIdx);
        if (!mb) //Exceptional case our .ids file is messed up
            return false;
        if (mb->isNew() || mb->isUnread()) {
            if (mUnreadMsgs == -1) ++mUnreadMsgs;
            ++mUnreadMsgs;
        }
    }
    mTotalMsgs = mSerNums.count();
    fclose(mIdsStream);
    mIdsStream = 0;
    mUnlinked = true;
    return true;
}

int KMFolderSearch::removeContents()
{
    unlink(QFile::encodeName(location()));
    unlink(QFile::encodeName(indexLocation()));
    mUnlinked = true;
    return 0;
}

int KMFolderSearch::expungeContents()
{
    setSearch(new KMSearch());
    return 0;
}

int KMFolderSearch::count(bool cache) const
{
    Q_UNUSED(cache);
    return mSerNums.count();
}

KMMsgBase* KMFolderSearch::takeIndexEntry(int idx)
{
    assert(idx >= 0 && idx < (int)mSerNums.count());
    KMMsgBase *msgBase = getMsgBase(idx);
    QValueVector<Q_UINT32>::iterator it = mSerNums.begin();
    mSerNums.erase(&it[idx]);
    return msgBase;
}

KMMsgInfo* KMFolderSearch::setIndexEntry(int idx, KMMessage *msg)
{
    assert(idx >= 0 && idx < (int)mSerNums.count());
    Q_UNUSED( idx );
    return msg->storage()->setIndexEntry(msg->parent()->find(msg), msg);
}

void KMFolderSearch::clearIndex(bool, bool)
{
  //close all referenced folders
  QValueListIterator<QGuardedPtr<KMFolder> > fit;
  for (fit = mFolders.begin(); fit != mFolders.end(); ++fit) {
    if (!(*fit))
      continue;
    (*fit)->close("foldersearch");
  }
  mFolders.clear();

  mSerNums.clear();
}

void KMFolderSearch::truncateIndex()
{
    truncate(QFile::encodeName(indexLocation()), IDS_SEARCH_HEADER_LEN);
}

void KMFolderSearch::examineAddedMessage(KMFolder *aFolder, Q_UINT32 serNum)
{
    if (!search() && !readSearch())
        return;
    if (!search()->inScope(aFolder))
        return;
    if (!mTempOpened) {
        open("foldersearch");
        mTempOpened = true;
    }

    if (!search()->searchPattern())
        return;

    int idx = -1;
    KMFolder *folder = 0;
    KMMsgDict::instance()->getLocation(serNum, &folder, &idx);
    assert(folder && (idx != -1));
    assert(folder == aFolder);
    KMFolderOpener openFolder(folder, "foldersearch");

    // if we are already checking this folder, refcount
    if ( mFoldersCurrentlyBeingSearched.contains( folder ) ) {
      unsigned int count = mFoldersCurrentlyBeingSearched[folder];
      mFoldersCurrentlyBeingSearched.replace( folder, count+1 );
    } else {
      connect( folder->storage(),
              SIGNAL( searchDone( KMFolder*, Q_UINT32, const KMSearchPattern*, bool ) ),
              this,
              SLOT( slotSearchExamineMsgDone( KMFolder*, Q_UINT32,
                      const KMSearchPattern*, bool ) ) );
      mFoldersCurrentlyBeingSearched.insert( folder, 1 );
    }
    folder->storage()->search( search()->searchPattern(), serNum );
}

void KMFolderSearch::slotSearchExamineMsgDone( KMFolder* folder,
                                               Q_UINT32 serNum,
                                               const KMSearchPattern* pattern,
                                               bool matches )
{
    if ( search()->searchPattern() != pattern ) return;
    kdDebug(5006) << folder->label() << ": serNum " << serNum
     << " matches?" << matches << endl;
    KMFolderOpener openFolder(folder, "foldersearch");

    Q_ASSERT( mFoldersCurrentlyBeingSearched.contains( folder ) );

    unsigned int count = mFoldersCurrentlyBeingSearched[folder];
    if ( count == 1 ) {
      disconnect( folder->storage(),
                  SIGNAL( searchDone( KMFolder*, Q_UINT32,
                                      const KMSearchPattern*, bool ) ),
                  this,
                  SLOT( slotSearchExamineMsgDone( KMFolder*, Q_UINT32,
                                                  const KMSearchPattern*, bool ) ) );
      mFoldersCurrentlyBeingSearched.remove( folder );
    } else {
      mFoldersCurrentlyBeingSearched.replace( folder, count-1 );
    }

    if ( !matches ) {
        QValueVector<Q_UINT32>::const_iterator it;
        it = qFind( mSerNums.begin(), mSerNums.end(), serNum );
        if (it != mSerNums.end()) {
            removeSerNum( serNum );
        }
        return;
    }

//    if (mSearch->running()) {
//        mSearch->stop();
//        mExecuteSearchTimer->start( 0, true );
//    } else {
        QValueVector<Q_UINT32>::const_iterator it;
        it = qFind( mSerNums.begin(), mSerNums.end(), serNum );
        if (it == mSerNums.end()) {
            addSerNum( serNum );
        }
//    }
}

void KMFolderSearch::examineRemovedMessage(KMFolder *folder, Q_UINT32 serNum)
{
    if (!search() && !readSearch())
        return;
    if (!search()->inScope(folder))
        return;
    if (!mTempOpened) {
        open("foldersearch");
        mTempOpened = true;
    }

    if (mSearch->running()) {
        mExecuteSearchTimer->start(0, true);
    } else {
        removeSerNum(serNum);
    }
}

void KMFolderSearch::examineChangedMessage(KMFolder *aFolder, Q_UINT32 serNum, int delta)
{
    if (!search() && !readSearch())
        return;
    if (!search()->inScope(aFolder))
        return;
    if (!mTempOpened) {
        open("foldersearch");
        mTempOpened = true;
    }
    QValueVector<Q_UINT32>::const_iterator it;
    it = qFind( mSerNums.begin(), mSerNums.end(), serNum );
    if (it != mSerNums.end()) {
        mUnreadMsgs += delta;
        emit numUnreadMsgsChanged( folder() );
        emit msgChanged( folder(), serNum, delta );
    }
}

void KMFolderSearch::examineInvalidatedFolder(KMFolder *folder)
{
    if (!search() && !readSearch())
        return;
    if (!search()->inScope(folder))
        return;
    if (mTempOpened) {
        close("foldersearch");
        mTempOpened = false;
    }

    mInvalid = true;
    if (mSearch)
        mSearch->stop();

    if (!mUnlinked) {
        unlink(QFile::encodeName(indexLocation()));
        mUnlinked = true;
    }

    if (!isOpened()) //give up, until the user manually opens the folder
        return;

    if (!mTempOpened) {
        open("foldersearch");
        mTempOpened = true;
    }
    mExecuteSearchTimer->start(0, true);
}

void KMFolderSearch::examineRemovedFolder(KMFolder *folder)
{
    examineInvalidatedFolder(folder);
    if (mSearch->root() == folder) {
        delete mSearch;
        mSearch = 0;
    }
}

void KMFolderSearch::propagateHeaderChanged(KMFolder *aFolder, int idx)
{
    int pos = 0;
    if (!search() && !readSearch())
        return;
    if (!search()->inScope(aFolder))
        return;
    if (!mTempOpened) {
        open("foldersearch");
        mTempOpened = true;
    }

    Q_UINT32 serNum = KMMsgDict::instance()->getMsgSerNum(aFolder, idx);
    QValueVector<Q_UINT32>::const_iterator it;
    for(it = mSerNums.begin(); it != mSerNums.end(); ++it) {
        if ((*it) == serNum) {
            emit msgHeaderChanged(folder(), pos);
            break;
        }
        ++pos;
    }
    // let's try if the message matches our search
    KMFolderOpener openAFolder(aFolder, "foldersearch");

    // if we are already checking this folder, refcount
    if ( mFoldersCurrentlyBeingSearched.contains( aFolder ) ) {
      unsigned int count = mFoldersCurrentlyBeingSearched[aFolder];
      mFoldersCurrentlyBeingSearched.replace( aFolder, count+1 );
    } else {
      connect( aFolder->storage(),
              SIGNAL( searchDone( KMFolder*, Q_UINT32, const KMSearchPattern*, bool ) ),
              this,
              SLOT( slotSearchExamineMsgDone( KMFolder*, Q_UINT32,
                      const KMSearchPattern*, bool ) ) );
      mFoldersCurrentlyBeingSearched.insert( aFolder, 1 );
    }
    aFolder->storage()->search( search()->searchPattern(), serNum );
}

void KMFolderSearch::tryReleasingFolder(KMFolder* folder)
{
  // We'll succeed releasing the folder only if mTempOpened and mOpenCount==1.
  // Otherwise if mOpenCount>1 (e.g while the search dialog is up), we would just keep closing/reopening for nothing
  if ( mTempOpened && mOpenCount == 1 )
  {
    examineInvalidatedFolder( folder );
  }
}

#include "kmfoldersearch.moc"
