// Author: Don Sanders <sanders@kde.org>
// License GPL

//Factor byteswap stuff into one header file

#include "kmfoldersearch.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmsearchpattern.h"
#include <qfileinfo.h>

#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>
#include "kmmsgdict.h"
#include "kmmsgindex.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <utime.h>
#include <config.h>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 32 bit value.  */
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

// Current version of the .index.search files
#define IDS_VERSION 1000
// The asterisk at the end is important
#define IDS_HEADER "# KMail-Search-IDs V%d\n*"
#define IDS_HEADER_LEN 30


KMSearch::KMSearch(QObject * parent, const char * name)
    :QObject(parent, name)
{
  mRemainingFolders = -1;
  mRemainingMessages = -1;
  mRecursive = true;
  mRunByIndex = mRunning = false;
  mIdle = false;
  mRoot = 0;
  mSearchPattern = 0;
  mSearchedCount = 0;
  mFoundCount = 0;
}

KMSearch::~KMSearch()
{
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
  KConfig config(location);
  config.setGroup("Search Folder");
  if (!mSearchPattern)
      mSearchPattern = new KMSearchPattern();
  mSearchPattern->readConfig(&config);
  QString rootString = config.readEntry("Base");
  mRoot = kernel->folderMgr()->findIdString(rootString);
  if (mRoot.isNull())
      mRoot = kernel->imapFolderMgr()->findIdString(rootString);
  mRecursive = config.readBoolEntry("Recursive");
  return true;
}

void KMSearch::setSearchPattern(KMSearchPattern *searchPattern)
{
    if (running())
	stop();
    if (mSearchPattern != searchPattern) {
	delete mSearchPattern;
	mSearchPattern = searchPattern;
    }
}

bool KMSearch::inScope(KMFolder* folder) const
{
    if (mRoot.isNull() || QGuardedPtr<KMFolder>(folder) == mRoot)
	return true;
    if (!recursive())
	return false;

    KMFolderDir *rootDir = 0;
    if (mRoot.isNull())
	rootDir = &kernel->folderMgr()->dir();
    else
	rootDir = mRoot->child();
    KMFolderDir *ancestorDir = folder->parent();
    while (ancestorDir) {
	if (ancestorDir == rootDir)
	    return true;
	ancestorDir = ancestorDir->parent();
    }
    return false;
}

void KMSearch::start()
{
    if (running())
	return;

    if (!mSearchPattern) {
	emit finished(true);
	return;
    }

    mSearchedCount = 0;
    mFoundCount = 0;
    mRunning = true;
    mRunByIndex = false;
    if(kernel->msgIndex() && kernel->msgIndex()->startQuery(this)) {
	mRunByIndex = true;
	return;
    }

    QValueList<QGuardedPtr<KMFolder> > folders;
    folders.append(mRoot);
    if (recursive()) { //Append all descendants to folders
  	KMFolderNode* node;
	KMFolder* folder;
	QValueListConstIterator<QGuardedPtr<KMFolder> > it;
	for (it = folders.begin(); it != folders.end(); ++it) {
	    folder = *it;
	    KMFolderDir *dir = 0;
	    if (folder)
		dir = folder->child();
	    else
		dir = &kernel->folderMgr()->dir();
	    if (!dir)
		continue;
	    QPtrListIterator<KMFolderNode> it(*dir);
	    while ((node = it.current())) {
		++it;
		if (!node->isDir())
		{
		    KMFolder* kmf = dynamic_cast<KMFolder*>(node);
		    if (kmf)
			folders.append(kmf);
		}
	    }
	}
    }

    mLastFolder = QString::null;
    mRemainingFolders = folders.count();
    mRemainingMessages = 0;
    QValueListConstIterator<QGuardedPtr<KMFolder> > it;
    for (it = folders.begin(); it != folders.end(); ++it) {
	KMFolder *folder = *it;
	if (!folder) {
	    --mRemainingFolders;
	    continue;
	}
	//TODO: Get rid of this protocol check, need a bool KMFolder::isComplete()
	if (folder->folderType() == KMFolderTypeImap) {
	    KMFolderImap *imapFolder = dynamic_cast<KMFolderImap*>(folder);
	    if (imapFolder && imapFolder->getContentState() ==
		KMFolderImap::imapNoInformation) {
		mIncompleteFolders.append(imapFolder);
		connect(imapFolder, SIGNAL(folderComplete(KMFolderImap*, bool)),
			SLOT(slotFolderComplete(KMFolderImap*, bool)));
		imapFolder->getFolder();
	    } else {
		mFolders.append(folder);
	    }
	} else {
	    mFolders.append(folder);
	}
    }

    QTimer::singleShot(0, this, SLOT(slotProcessNextBatch()));
}

void KMSearch::stop()
{
    if (!running())
	return;
    if(mRunByIndex) {
	if(kernel->msgIndex())
	    kernel->msgIndex()->stopQuery(this);
    } else {
	//kill all pending jobs
	QValueListConstIterator<QGuardedPtr<KMFolderImap> > it;
	for (it = mIncompleteFolders.begin();
	     it != mIncompleteFolders.end(); ++it) {
	    KMFolder *folder = *it;
	    if (folder)
		disconnect(folder,
			   SIGNAL(folderComplete(KMFolderImap*, bool)),
			   this,
			   SLOT(slotFolderComplete(KMFolderImap*, bool)));
	}
	mIncompleteFolders.clear();
	QValueListConstIterator<QGuardedPtr<KMFolder> > jt;
	for (jt = mOpenedFolders.begin(); jt != mOpenedFolders.end(); ++jt) {
	    KMFolder *folder = *jt;
	    if (folder)
		folder->close();
	}
    }
    mOpenedFolders.clear();
    mRemainingMessages = -1;
    mRemainingFolders = -1;
    mFolders.clear();
    mLastFolder = "";
    mRunByIndex = mRunning = false;
    mIdle = false;
    emit finished(false);
}

void KMSearch::slotProcessNextBatch()
{
    if (!running())
	return;
    mIdle = false;

    if (mSerNums.count() != 0) {
	int i = 100;
	QValueListIterator<Q_UINT32> it;
	for (it = mSerNums.begin(); it != mSerNums.end();) {
	    if (--i == 0)
		break;

	    Q_UINT32 serNum = *it;
	    it = mSerNums.erase(it);
	    --mRemainingMessages;
	    ++mSearchedCount;

	    int idx = -1;
	    KMFolder *folder = 0;
	    kernel->msgDict()->getLocation(serNum, &folder, &idx);
	    if (!folder || (idx == -1) || (idx >= folder->count())) {
		continue;
	    }

	    //TODO: matches should try header rules first and check 
            // rule->needsBody() and download imap message if needed
            KMMessage *msg = folder->getMsg(idx);
	    if (mSearchPattern && !mSearchPattern->matches(msg))
		continue;
	    emit found(serNum);
	    ++mFoundCount;
	}
	QTimer::singleShot(0, this, SLOT(slotProcessNextBatch()));
	return;
    }

    if (mFolders.count() != 0) {
	--mRemainingFolders;
	KMFolder *folder = *(mFolders.begin());
	if (folder->isSystemFolder())
	    mLastFolder = i18n(folder->name().utf8());
	else
	    mLastFolder = folder->name();
	mFolders.erase(mFolders.begin());
	if (folder) {
	    folder->open();
	    mOpenedFolders.append(folder);
	    for(int i = 0; i < folder->count(); ++i) {
		Q_UINT32 serNum = kernel->msgDict()->getMsgSerNum(folder, i);
		++mRemainingMessages;
		mSerNums.append(serNum);
	    }
	}
	QTimer::singleShot(0, this, SLOT(slotProcessNextBatch()));
	return;
    }
    if (mRemainingFolders == 0) {
	mRunning = false;
	QValueListConstIterator<QGuardedPtr<KMFolder> > it;
	for (it = mOpenedFolders.begin(); it != mOpenedFolders.end(); ++it) {
	    KMFolder *folder = *it;
	    if (folder)
		folder->close();
	}
	mOpenedFolders.clear();
	mRemainingMessages = -1;
	mRemainingFolders = -1;
	mFolders.clear();
	mLastFolder = "";
	emit finished(true);
	return;
    }

    //wait for imap folders to be retrieved
    mIdle = true;
}

void KMSearch::slotFolderComplete(KMFolderImap *folder, bool success)
{
  disconnect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
             this, SLOT(slotFolderComplete(KMFolderImap*, bool)));

  if (success) {
      //if !(mSearchPattern.hasBodyRules())
          mFolders.append(folder);
      //else
      //  get all the bodies of messages in the folder, need kroupware body caching
      if (mIdle)
	  QTimer::singleShot(0, this, SLOT(slotProcessNextBatch()));
  } else {
      stop();
  }
}


//-----------------------------------------------------------------------------
KMFolderSearch::KMFolderSearch(KMFolderDir* parent, const QString& name)
  : KMFolder(parent, name)
{
    mIdsStream = 0;
    mSearch = 0;
    mInvalid = false;
    mUnlinked = true;
    mTempOpened = false;

    //Hook up some slots for live updating of search folders
    //TODO: Optimize folderInvalidated, folderAdded, folderRemoved
    connect(kernel->folderMgr(), SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
	    this, SLOT(examineAddedMessage(KMFolder*, Q_UINT32)));
    connect(kernel->folderMgr(), SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
	    this, SLOT(examineRemovedMessage(KMFolder*, Q_UINT32)));
    connect(kernel->folderMgr(), SIGNAL(msgChanged(KMFolder*, Q_UINT32, int)),
	    this, SLOT(examineChangedMessage(KMFolder*, Q_UINT32, int)));
    connect(kernel->folderMgr(), SIGNAL(folderInvalidated(KMFolder*)),
	    this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kernel->folderMgr(), SIGNAL(folderAdded(KMFolder*)),
	    this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kernel->folderMgr(), SIGNAL(folderRemoved(KMFolder*)),
	    this, SLOT(examineRemovedFolder(KMFolder*)));
    connect(kernel->folderMgr(), SIGNAL(msgHeaderChanged(KMFolder*,int)),
	    this, SLOT(propagateHeaderChanged(KMFolder*,int)));

    connect(kernel->imapFolderMgr(), SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
	    this, SLOT(examineAddedMessage(KMFolder*, Q_UINT32)));
    connect(kernel->imapFolderMgr(), SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
	    this, SLOT(examineRemovedMessage(KMFolder*, Q_UINT32)));
    connect(kernel->imapFolderMgr(), SIGNAL(msgChanged(KMFolder*, Q_UINT32, int)),
	    this, SLOT(examineChangedMessage(KMFolder*, Q_UINT32, int)));
    connect(kernel->imapFolderMgr(), SIGNAL(folderInvalidated(KMFolder*)),
	    this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kernel->imapFolderMgr(), SIGNAL(folderAdded(KMFolder*)),
	    this, SLOT(examineInvalidatedFolder(KMFolder*)));
    connect(kernel->imapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
	    this, SLOT(examineRemovedFolder(KMFolder*)));
    connect(kernel->imapFolderMgr(), SIGNAL(msgHeaderChanged(KMFolder*,int)),
	    this, SLOT(propagateHeaderChanged(KMFolder*,int)));
}

KMFolderSearch::~KMFolderSearch()
{
    delete mSearch;
    mSearch = 0;
    if (mOpenCount > 0)
	close(TRUE);
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
    emit numUnreadMsgsChanged(this);
    emit changed(); // really want a kmfolder cleared signal
    /* TODO There is KMFolder::cleared signal now. Adjust. */
    if (mSearch)
	mSearch->start();
    open();
}

void KMFolderSearch::executeSearch()
{
    if (mSearch)
	mSearch->stop();
    setSearch(mSearch);
    if ( parent() )
	parent()->manager()->invalidateFolder(kernel->msgDict(), this);
}

const KMSearch* KMFolderSearch::search() const
{
    return mSearch;
}

void KMFolderSearch::searchFinished(bool success)
{
    if (!success)
	mSerNums.clear();
    close();
}

void KMFolderSearch::addSerNum(Q_UINT32 serNum)
{
    if (mInvalid) // A new search is scheduled don't bother doing anything
	return;
    int idx = -1;
    KMFolder *folder = 0;
    kernel->msgDict()->getLocation(serNum, &folder, &idx);
    assert(folder && (idx != -1));
    if(mFolders.findIndex(folder) == -1) {
	folder->open();
	// Exceptional case, for when folder has invalid ids
	if (mInvalid)
	    return;
	mFolders.append(folder);
    }
    setDirty( true ); //TODO append a single entry to .ids file and sync.
    if (!mUnlinked) {
	unlink(QFile::encodeName(indexLocation()));
	mUnlinked = true;
    }
    mSerNums.append(serNum);
    KMMsgBase *mb = folder->getMsgBase(idx);
    if (mb->isUnread() || mb->isNew()) {
       if (mUnreadMsgs == -1)
           mUnreadMsgs = 0;
       ++mUnreadMsgs;
       emit numUnreadMsgsChanged( this );
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
	    KMFolder *folder = 0;
	    kernel->msgDict()->getLocation(serNum, &folder, &idx);
	    assert(folder && (idx != -1));
	    emit msgRemoved(this, serNum);
	    removeMsg(i);
	    return;
	}
    if (!mUnlinked) {
	unlink(QFile::encodeName(indexLocation()));
	mUnlinked = true;
    }
}

QCString& KMFolderSearch::getMsgString(int idx, QCString& mDest)
{
    KMFolder *folder = getMsgBase(idx)->parent();
    assert(folder);
    return folder->getMsgString(folder->find(getMsgBase(idx)), mDest);
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

int KMFolderSearch::open()
{
    mOpenCount++;
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
    assert(!name().isEmpty());

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

void KMFolderSearch::close(bool force)
{
    if (mOpenCount <= 0) return;
    if (mOpenCount > 0) mOpenCount--;
    if (mOpenCount > 0 && !force) return;

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
	(*fit)->close();
    }
    mFolders.clear();

    clearIndex(TRUE);

    if (mIdsStream)
	fclose(mIdsStream);

    mOpenCount   = 0;
    mIdsStream = 0;
    mUnreadMsgs  = -1;
}

int KMFolderSearch::create(bool)
{
    int old_umask;
    int rc = unlink(QFile::encodeName(location()));
    if (!rc)
	return rc;
    rc = 0;

    assert(!name().isEmpty());
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
    mQuiet = 0;
    mChanged = false;
    mUnreadMsgs = 0;
    mTotalMsgs = 0;
    return rc;
}

int KMFolderSearch::compact()
{
    needsCompact = false;
    return 0;
}

bool KMFolderSearch::isReadOnly() const
{
    return false; //TODO: Make it true and get that working ok
}

FolderJob* KMFolderSearch::doCreateJob(KMMessage*, FolderJob::JobType,
                                     KMFolder*, QString) const
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
    kernel->msgDict()->getLocation(mSerNums[idx], &folder, &folderIdx);
    assert(folder && (folderIdx != -1));
    return folder->getMsgBase(folderIdx);
}

KMMsgBase* KMFolderSearch::getMsgBase(int idx)
{
    int folderIdx = -1;
    KMFolder *folder = 0;
    if (idx < 0 || (Q_UINT32)idx >= mSerNums.count())
	return 0;
    kernel->msgDict()->getLocation(mSerNums[idx], &folder, &folderIdx);
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
    kernel->msgDict()->getLocation(mSerNums[idx], &folder, &folderIdx);
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
  KMFolderSearchInherited::ignoreJobsForMessage( msg );

  if (msg->parent()->folderType() == KMFolderTypeImap) {
    KMAcctImap *account;
    if ( !(account = static_cast<KMFolderImap*>(msg->parent())->account()) )
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
    QString sLocation(path());

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

int KMFolderSearch::writeIndex()
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
    fprintf(tmpIndexStream, IDS_HEADER, IDS_VERSION);
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
    kernel->msgDict()->getLocation(mSerNums[idx], &folder, &folderIdx);
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
    fscanf(mIdsStream, IDS_HEADER, &version);
    if (version != IDS_VERSION) {
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

	kernel->msgDict()->getLocation( serNum, &folder, &folderIdx );
	if (!folder || (folderIdx == -1)) {
	    clearIndex();
	    fclose(mIdsStream);
	    mIdsStream = 0;
	    return false;
	}
	mSerNums.push_back(serNum);
	if(mFolders.findIndex(folder) == -1) {
	    folder->open();
	    if (mInvalid) //exceptional case for when folder has invalid ids
		return false;
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
  int res = KMFolder::count(cache);
  if (res == -1) {
      // open()
      res = mSerNums.count();
      // close()
  }
  return res;
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
    return msg->parent()->setIndexEntry(msg->parent()->find(msg), msg);
}

void KMFolderSearch::clearIndex(bool, bool)
{
    mSerNums.clear();
}

void KMFolderSearch::fillDictFromIndex(KMMsgDict *)
{
    // No dict for search folders, as search folders don't own any messages
    return;
}

void KMFolderSearch::truncateIndex()
{
    truncate(QFile::encodeName(indexLocation()), IDS_HEADER_LEN);
}

void KMFolderSearch::examineAddedMessage(KMFolder *aFolder, Q_UINT32 serNum)
{
    if (!search() && !readSearch())
	return;
    if (!search()->inScope(aFolder))
	return;
    if (!mTempOpened) {
	open();
	mTempOpened = true;
    }

    if (!search()->searchPattern())
	return;

    int idx = -1;
    KMFolder *folder = 0;
    kernel->msgDict()->getLocation(serNum, &folder, &idx);
    assert(folder && (idx != -1));
    assert(folder == aFolder);
    if (!folder->isOpened())
      return;

    if (folder->folderType() == KMFolderTypeImap) {
        // Unless there is a search currently running, add the message to
	// the list of ones to check on folderCompleted and hook up the signal.
	if (!mSearch->running()) {
	    mUnexaminedMessages.push(serNum);
            disconnect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
	           this, SLOT (examineCompletedFolder(KMFolderImap*, bool)));
	    connect(folder, SIGNAL(folderComplete(KMFolderImap*, bool)),
	           this, SLOT (examineCompletedFolder(KMFolderImap*, bool)));
	}
    } else {
        KMMessage *msg = folder->getMsg(idx);
        if (search()->searchPattern()->matches(msg))
            if (mSearch->running()) {
                mSearch->stop();
                QTimer::singleShot(0, this, SLOT(executeSearch()));
            } else {
                addSerNum(serNum);
            }
    }
}

void KMFolderSearch::examineCompletedFolder(KMFolderImap *aFolder, bool success)
{
    if (!success) return;
    disconnect (aFolder, SIGNAL(folderComplete(KMFolderImap*, bool)),
                this, SLOT(examineCompletedFolder(KMFolderImap*, bool)));
    Q_UINT32 serNum;
    while (!mUnexaminedMessages.isEmpty()) {
        serNum = mUnexaminedMessages.pop();
        int idx = -1;
        KMFolder *folder = 0;
        kernel->msgDict()->getLocation(serNum, &folder, &idx);
        assert(folder && (idx != -1));
        // Could it be another folder?
        assert(folder == aFolder);
        // FIXME separate headers and body matching
        folder->open();
        KMMessage *msg = folder->getMsg(idx);
        if (search()->searchPattern()->matches(msg))
            addSerNum(serNum);
        folder->close();
    }
}

void KMFolderSearch::examineRemovedMessage(KMFolder *folder, Q_UINT32 serNum)
{
    if (!search() && !readSearch())
	return;
    if (!search()->inScope(folder))
	return;
    if (!mTempOpened) {
	open();
	mTempOpened = true;
    }

    if (mSearch->running()) {
	mSearch->stop();
	QTimer::singleShot(0, this, SLOT(executeSearch()));
    } else {
	removeSerNum(serNum);
    }
}

void KMFolderSearch::examineChangedMessage(KMFolder *folder, Q_UINT32 serNum, int delta)
{
    if (!search() && !readSearch())
	return;
    if (!search()->inScope(folder))
	return;
    if (!mTempOpened) {
	open();
	mTempOpened = true;
    }
    QValueVector<Q_UINT32>::const_iterator it;
    it = qFind( mSerNums.begin(), mSerNums.end(), serNum );
    if (it != mSerNums.end()) {
	mUnreadMsgs += delta;
	emit numUnreadMsgsChanged( this );
	emit msgChanged( this, serNum, delta );
    }
}

void KMFolderSearch::examineInvalidatedFolder(KMFolder *folder)
{
    if (!search() && !readSearch())
	return;
    if (!search()->inScope(folder))
	return;
    if (mTempOpened) {
	close();
	mTempOpened = false;
    }

    mInvalid = true;
    if (mSearch)
	mSearch->stop();

    removeContents();
    if (!isOpened()) //give up, until the user manually opens the folder
	return;

    if (!mTempOpened) {
	open();
	mTempOpened = true;
    }
    QTimer::singleShot(0, this, SLOT(executeSearch()));
}

void KMFolderSearch::examineRemovedFolder(KMFolder *folder)
{
    examineInvalidatedFolder(folder);
    if (mSearch->root() == folder) {
	delete mSearch;
	mSearch = 0;
    }
}

void KMFolderSearch::propagateHeaderChanged(KMFolder *folder, int idx)
{
    int pos = 0;
    if (!search() && !readSearch())
	return;
    if (!search()->inScope(folder))
	return;
    if (!mTempOpened) {
	open();
	mTempOpened = true;
    }

    Q_UINT32 serNum = kernel->msgDict()->getMsgSerNum(folder, idx);
    QValueVector<Q_UINT32>::const_iterator it;
    for(it = mSerNums.begin(); it != mSerNums.end(); ++it) {
	if ((*it) == serNum) {
	    emit msgHeaderChanged(this, pos);
	    return;
	}
	++pos;
    }
}

#include "kmfoldersearch.moc"
