
#include "kmsearchpattern.h"
#include "kmfoldersearch.h"
#include "kmmsgindex.h"
#include "kmfoldermgr.h"
#include "kmfolderdir.h"
#include "kmmsgdict.h"
#include "kmmessage.h"
#include "kdebug.h"

#include "mimelib/message.h"
#include "mimelib/string.h"
#include "mimelib/headers.h"
#include "mimelib/utility.h"
#include "mimelib/enum.h"
#include "mimelib/body.h"

#include <qdict.h>
#include <qtimer.h>
#include <qintdict.h>
#include <qapplication.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#define KMMSGINDEX_TMP_EXT ".tmp"

KMMsgIndex::KMMsgIndex(QObject *o, const char *n) : 
    QObject(o, n), mIndexState(INDEX_IDLE)
{
    mActiveSearches.setAutoDelete(TRUE);
    reset(FALSE);
    mTermIndex.loc = kernel->folderMgr()->basePath() + "/.kmsgindex_search";
    mTermTOC.loc = kernel->folderMgr()->basePath() + "/.kmsgindex_toc";

    readIndex();
    connect(kernel->folderMgr(), SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
	    this, SLOT(addMsg(KMFolder*, Q_UINT32)));
}

KMMsgIndex::~KMMsgIndex()
{
    reset();
    if(mIndexState == INDEX_CREATE) { //we weren't done anyway..
	unlink(QString(mTermIndex.loc+KMMSGINDEX_TMP_EXT).latin1());
	unlink(QString(mTermTOC.loc+KMMSGINDEX_TMP_EXT).latin1());
    }
}

void
KMMsgIndex::reset(bool clean)
{
    mActiveSearches.clear();
    if(clean)
	mTermTOC.map.clear();
    if(mTermTOC.fd != -1) {
	if(clean)
	    close(mTermTOC.fd);
	mTermTOC.fd = -1;
    }
    if(create.timer_id != -1) {
	if(clean)
	    killTimer(create.timer_id);
	create.timer_id = -1;
    }
    if(mTermIndex.ref != MAP_FAILED) {
	if(clean)
	    munmap(mTermIndex.ref, mTermIndex.count * sizeof(Q_INT32));
	mTermIndex.ref = (Q_UINT32*)MAP_FAILED;
    }
    mTermIndex.known = mTermIndex.used = mTermIndex.count = 0;
    if(mTermIndex.fd != -1) {
	if(clean)
	    close(mTermIndex.fd);
	mTermIndex.fd = -1;
    }
}

int
KMMsgIndex::allocTermChunk(int cnt)
{
    int ret = mTermIndex.used;
    mTermIndex.used += cnt; //update the used
    if(mTermIndex.count < mTermIndex.used) { //time for a remap
	munmap(mTermIndex.ref, mTermIndex.count * sizeof(Q_INT32));
	int oldcount = mTermIndex.count;
	mTermIndex.count = QMAX(mTermIndex.count * 2, mTermIndex.used);
	if(ftruncate(mTermIndex.fd, 
		     mTermIndex.count * sizeof(Q_INT32)) == -1) {
	    for(Q_INT32 i = oldcount; i < mTermIndex.count; i++) 
		write(mTermIndex.fd, &i, sizeof(i));
	}
	mTermIndex.ref = (Q_UINT32*)mmap(0, mTermIndex.count * sizeof(Q_INT32),
					 PROT_READ|PROT_WRITE, MAP_SHARED, 
					 mTermIndex.fd, 0);
	mTermIndex.ref[1] = mTermIndex.count;
    }
    mTermIndex.ref[2] = mTermIndex.used;
    return ret;
}

bool 
KMMsgIndex::isKillTerm(const char *term, int term_len)
{
    if(!term || term_len < 1)
	return TRUE;
#if 0
    if(term_len <= 1) //one letter just isn't enough..
	return TRUE;
#endif
#if 0
    { //no numbers!
	int numlen = 0;
	for(numlen = 0; numlen < term_len; numlen++) {
	    if(!isdigit(term[numlen]))
		break;
	}
	if(numlen == term_len)
	    return TRUE;
    }
#endif
#if 0
    { //static words lists, maybe hacky
	static QDict<void> *killDict = NULL;
	if(!killDict) {
	    killDict = new QDict<void>();
	    const char *kills[] = { "from",
				    "for", "to", "the", "but", NULL };
	    for(int i = 0; kills[i]; i++)
		killDict->insert(kills[i], (void*)1);
	}
	if(term_len <= 1 || killDict->find(term))
	    return TRUE;
    }
#endif
    return FALSE;
}

bool
KMMsgIndex::addTerm(const char *term, int term_len, Q_UINT32 serNum)
{
    if(mTermIndex.ref == MAP_FAILED)
	return FALSE;
    if(isKillTerm(term, term_len)) 
	return TRUE; //sorta..

    if(!mTermTOC.map.contains(term)) {
	int first_chunk_size = 6; //just enough for the one..
	int off = allocTermChunk(first_chunk_size);
	mTermTOC.map.insert(term, off); //mark in TOC
	write(mTermTOC.fd, &term_len, sizeof(term_len));
	write(mTermTOC.fd, term, term_len);
	write(mTermTOC.fd, &off, sizeof(off));

	//special case to mark the tail for the first
	mTermIndex.ref[off++] = off+1;
	first_chunk_size--;

	//now mark in index
	mTermIndex.ref[off++] = first_chunk_size;
	mTermIndex.ref[off++] = 4; //include the header
	mTermIndex.ref[off++] = 0; 
	mTermIndex.ref[off] = serNum;
    } else {
	int map_off = mTermTOC.map[term], tail = mTermIndex.ref[map_off];
	uint len = mTermIndex.ref[tail];
	if(len == mTermIndex.ref[tail+1]) {
	    len *= 2; //double
	    int blk = allocTermChunk(len); 
	    mTermIndex.ref[map_off] = mTermIndex.ref[tail+2] = blk;
	    mTermIndex.ref[blk++] = len;
	    mTermIndex.ref[blk++] = 4; //include the header
	    mTermIndex.ref[blk++] = 0; 
	    mTermIndex.ref[blk] = serNum;
	} else {
	    mTermIndex.ref[tail+mTermIndex.ref[tail+1]] = serNum;
	    mTermIndex.ref[tail+1]++;
	}
    }
    return TRUE;
}

bool
KMMsgIndex::processMsg(Q_UINT32 serNum)
{
    int idx = -1;
    KMFolder *folder = 0;
    kernel->msgDict()->getLocation(serNum, &folder, &idx);
    if(!folder || (idx == -1))
	return FALSE;
    if(mOpenedFolders.findIndex(folder) == -1) {
	folder->open();
	mOpenedFolders.append(folder);
    }
    
    bool ret = TRUE;
    KMMessage *msg = folder->getMsg(idx);
    const DwMessage *dw_msg = msg->asDwMessage();

    DwHeaders& headers = dw_msg->Headers();
    //TODO: process headers

    DwString body;
    if(headers.HasContentTransferEncoding()) {
	switch(headers.ContentTransferEncoding().AsEnum()) {
	case DwMime::kCteBase64: {
	    DwString raw_body = dw_msg->Body().AsString();
	    DwDecodeBase64(raw_body, body);
	    break; }
	case DwMime::kCteQuotedPrintable: {
	    DwString raw_body = dw_msg->Body().AsString();
	    DwDecodeQuotedPrintable(raw_body, body);
	    break; }
	default:
	    body = dw_msg->Body().AsString();
	    break;
	}
    } else {
	body = dw_msg->Body().AsString();
    }

    uint build_i = 0;
    char build_str[64]; //TODO: maybe we shouldn't hard code to 64
    const char *body_s = body.data();
    for(uint i = 0; i < body.length(); i++) {
	if(isalnum(body_s[i]) && build_i < 63) {
	    build_str[build_i++] = tolower(body_s[i]);
	} else if(build_i) {
	    build_str[build_i] = 0;
	    if(!addTerm(build_str, build_i, serNum)) {
		ret = FALSE;
		break;
	    }
	    build_i = 0;
	}
    }
    if(ret && build_i) {
	build_str[build_i] = 0;
	if(!addTerm(build_str, build_i, serNum))
	    ret = FALSE;
    }
    folder->unGetMsg(idx); //I don't need it anymore.. 

    mTermIndex.ref[3] = ++mTermIndex.known; //update known count
#if 0
    msync(mTermIndex.ref, mTermIndex.count * sizeof(Q_INT32), MS_SYNC);
    sync();
#endif
    return ret;
}

void
KMMsgIndex::addMsg(KMFolder *, Q_UINT32 serNum)
{
    if(mIndexState == INDEX_IDLE) 
	processMsg(serNum);
    else if(mIndexState == INDEX_CREATE) 
	create.serNums.push(serNum);
    else if(mIndexState == INDEX_CLEANUP) 
	cleanup.serNums.push(serNum);
}

void
KMMsgIndex::timerEvent(QTimerEvent *e)
{
    if(mIndexState == INDEX_CREATE && e->timerId() == create.timer_id) {
	if(qApp->hasPendingEvents()) //nah, next time..
	    return;
	int handled = 0;
	const int max_to_handle = 40;
	while(!create.serNums.isEmpty()) {
	    if(handled++ == max_to_handle)
		return;
	    processMsg(create.serNums.pop());
	}
	if(KMFolder *f = create.folders.pop()) {
	    if(mOpenedFolders.findIndex(f) == -1) {
		f->open();
		mOpenedFolders.append(f);
	    }
	    for(int i = 0, s; i < f->count(); ++i) {
		s = kernel->msgDict()->getMsgSerNum(f, i);
		if(handled++ < max_to_handle)
		    processMsg(s);
		else
		    create.serNums.push(s);
	    }
	} else {
	    killTimer(create.timer_id);
	    create.timer_id = -1;
	    mIndexState = INDEX_IDLE;
	    QValueListConstIterator<QGuardedPtr<KMFolder> > it;
	    for (it = mOpenedFolders.begin(); 
		 it != mOpenedFolders.end(); ++it) {
		KMFolder *folder = *it;
		if(folder)
		    folder->close();
	    }
	    mOpenedFolders.clear();
	    create.folders.clear();

	    //tricky move the file around without closing/opening/re-reading..
	    if(!link(QString(mTermIndex.loc+KMMSGINDEX_TMP_EXT).latin1(),
		     mTermIndex.loc.latin1()) && 
	       !link(QString(mTermTOC.loc+KMMSGINDEX_TMP_EXT).latin1(),
		     mTermTOC.loc.latin1())) {
		unlink(QString(mTermIndex.loc+KMMSGINDEX_TMP_EXT).latin1());
		unlink(QString(mTermTOC.loc+KMMSGINDEX_TMP_EXT).latin1());
	    } else qDebug("Whoa! not sure what to do..");
	}
    }
}

bool
KMMsgIndex::recreateIndex()
{
    if(mIndexState != INDEX_IDLE)
	return FALSE;

    mTermTOC.fd = open(QString(mTermTOC.loc+KMMSGINDEX_TMP_EXT).latin1(), 
		       O_RDWR|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
    mTermIndex.fd = open(QString(mTermIndex.loc+KMMSGINDEX_TMP_EXT).latin1(), 
			 O_RDWR|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
    Q_UINT32 byteOrder = 0x12345678;
    mTermIndex.count = 10240; //some big value..
    mTermIndex.used = 4; //including the header
    if(ftruncate(mTermIndex.fd, mTermIndex.count * sizeof(Q_INT32)) == -1) {
	for(Q_INT32 i = 0; i < mTermIndex.count; i++) 
	    write(mTermIndex.fd, &i, sizeof(i));
    }
    mTermIndex.ref = (Q_UINT32*)mmap(0, mTermIndex.count * sizeof(Q_INT32), 
				     PROT_READ|PROT_WRITE, 
				     MAP_SHARED, mTermIndex.fd, 0);
    mTermIndex.ref[0] = byteOrder;
    mTermIndex.ref[1] = mTermIndex.count;
    mTermIndex.ref[2] = mTermIndex.used; //including this header
    mTermIndex.ref[3] = mTermIndex.known;

    QValueStack<QGuardedPtr<KMFolderDir> > folders;
    folders.push(&(kernel->folderMgr()->dir()));
    while(KMFolderDir *dir = folders.pop()) {
	for(KMFolderNode *child = dir->first(); child; child = dir->next()) {
	    if(child->isDir()) 
		folders.push((KMFolderDir*)child);
	    else 
		create.folders.push((KMFolder*)child);
	}
    }

    mIndexState = INDEX_CREATE;
    create.timer_id = startTimer(0);
    return TRUE;
}

void
KMMsgIndex::readIndex()
{
    bool read_success = FALSE;
    if((mTermTOC.fd = open(mTermTOC.loc.latin1(), O_RDWR)) != -1) {
	if((mTermIndex.fd = open(mTermIndex.loc.latin1(), O_RDWR)) != -1) {
	    Q_INT32 byteOrder = 0;
	    read(mTermIndex.fd, &byteOrder, sizeof(byteOrder));
	    if(byteOrder == 0x12345678) {
		read(mTermIndex.fd, 
		     &mTermIndex.count, sizeof(mTermIndex.count));
		read(mTermIndex.fd, 
		     &mTermIndex.used, sizeof(mTermIndex.used));
		read(mTermIndex.fd, 
		     &mTermIndex.known, sizeof(mTermIndex.known));
		mTermIndex.ref = (Q_UINT32*)
				 mmap(0, mTermIndex.count * sizeof(Q_INT32), 
				      PROT_READ|PROT_WRITE, MAP_SHARED, 
				      mTermIndex.fd, 0);
		if(mTermIndex.ref == MAP_FAILED)
		    goto error_with_read;

		//read in the TOC
		int in_len = 0;
		char *in = NULL;
		int len;
		Q_UINT32 off;
		while(1) {
		    if(!read(mTermTOC.fd, &len, sizeof(len)))
			break;
		    if(len+1 > in_len) {
			in_len = len+1;
			if(in) 
			    in = (char *)realloc(in, in_len);
			else 
			    in = (char *)malloc(in_len);
		    }
		    read(mTermTOC.fd, in, len);
		    read(mTermTOC.fd, &off, sizeof(off));
		    in[len] = 0;
		    mTermTOC.map.insert(in, off);
		}
		if(in)
		    free(in);
		read_success = TRUE;
	    }
	}
    }
error_with_read:
    if(!read_success) {
	reset();
	unlink(mTermIndex.loc.latin1());
	unlink(mTermTOC.loc.latin1());
	recreateIndex();
    }
}

bool
KMMsgIndex::canHandleQuery(KMSearchPattern *pat)
{
    if(mIndexState == INDEX_CREATE) //not while we are creating the index..
	return FALSE;

    if(pat->op() != KMSearchPattern::OpAnd &&
       pat->op() != KMSearchPattern::OpOr)
	return FALSE;
    for(QPtrListIterator<KMSearchRule> it(*pat); it.current(); ++it) {
	if((*it)->field() != "<body>" || 
	   (*it)->function() != KMSearchRule::FuncContains)
	    return FALSE;
    }
    return TRUE;
}

QValueList<Q_UINT32> // Actually does the search.. 
KMMsgIndex::query(KMSearchRule *rule)
{
    QValueList<Q_UINT32> ret;
    if(rule->field() == "<body>" && 
       rule->function() == KMSearchRule::FuncContains) {
	QString match = rule->contents().lower();
	if(match.contains(' ')) {
	    bool found_word = FALSE;
	    QStringList sl = QStringList::split(' ', match);
	    for(QStringList::Iterator it = sl.begin(); it != sl.end(); ++it) {
		QString tmp = (*it).lower();
		if(!isKillTerm(tmp.latin1(), tmp.length())) {
		    found_word = TRUE;
		    match = tmp;
		    break;
		}
	    }
	    if(!found_word) 
		return ret;
	} else if(isKillTerm(match.latin1(), match.length())) {
	    match = "";
	}
	if(match.isEmpty() || !mTermTOC.map.contains(match))
	    return ret;
	int map_off = mTermTOC.map[match];
	for(int off = map_off+1; TRUE; off = mTermIndex.ref[off+2]) {
	    uint used = mTermIndex.ref[off+1];
	    for(uint i = 3; i < used; i++) 
		ret << mTermIndex.ref[off+i];
	    if(mTermIndex.ref[off] != used || off == map_off) 
		break;
	}
	if(match != rule->contents()) {
	    KMMessage msg;
	    QValueList<Q_UINT32> phrases;
	    for(QValueListIterator<Q_UINT32> it = ret.begin(); 
		it != ret.end(); ++it) {
		int idx = -1;
		KMFolder *folder = 0;
		kernel->msgDict()->getLocation((*it), &folder, &idx);
		if(!folder || (idx == -1))
		    continue;
		if(rule->matches(folder->getDwString(idx), msg))
		    phrases << (*it);
	    }
	    ret = phrases;
	}
    }
    return ret;
}

QValueList<Q_UINT32>
KMMsgIndex::query(KMSearchPattern *pat)
{
    QValueList<Q_UINT32> ret;
    if(pat->isEmpty() || !canHandleQuery(pat))
	return ret;

    if(pat->count() == 1) {
	ret = query(pat->first());
    } else {
	bool first = TRUE;
	QIntDict<void> foundDict;
	for(QPtrListIterator<KMSearchRule> it(*pat); it.current(); ++it) {
	    QValueList<Q_UINT32> tmp = query((*it));
	    if(first) {
		first = FALSE;
		for(QValueListIterator<Q_UINT32> it = tmp.begin(); 
		    it != tmp.end(); ++it) 
		    foundDict.insert((long int)(*it), (void*)1);
	    } else {
		if(pat->op() == KMSearchPattern::OpAnd) {
		    QIntDict<void> andDict;
		    for(QValueListIterator<Q_UINT32> it = tmp.begin(); 
			it != tmp.end(); ++it) {
			if(foundDict[(*it)])
			    andDict.insert((*it), (void*)1);
		    }
		    foundDict = andDict;
		} else if(pat->op() == KMSearchPattern::OpOr) {
		    for(QValueListIterator<Q_UINT32> it = tmp.begin(); 
			it != tmp.end(); ++it) {
			if(!foundDict[(*it)])
			    foundDict.insert((long int)(*it), (void*)1);
		    }
		}
	    }
	}
	for(QIntDictIterator<void> it(foundDict); it.current(); ++it)
	    ret << it.currentKey();
    }
    return ret;
}

/* Code to bind to a KMSearch */
KMIndexSearchTarget::KMIndexSearchTarget(KMSearch *s)
{ 
    mSearch = s; 
    mId = startTimer(0);
    {
	QValueList<Q_UINT32> lst = kernel->msgIndex()->query(
	    s->searchPattern());
	for(QValueListConstIterator<Q_UINT32> it = lst.begin();
	    it != lst.end(); ++it) 
	    mSearchResult.push((*it));
    }
    QObject::connect(this, SIGNAL(proxyFound(Q_UINT32)), 
		     s, SIGNAL(found(Q_UINT32)));
    QObject::connect(this, SIGNAL(proxyFinished(bool)),
		     s, SIGNAL(finished(bool)));
}
void
KMIndexSearchTarget::timerEvent(QTimerEvent *)
{
    if(qApp->hasPendingEvents())
	return; //no time now
    bool finished = FALSE;
    if(mSearch) {
	KMFolder *folder;
	int stop_at = QMIN(mSearchResult.count(), 20), found = 0;
	for(int i = 0, idx; i < stop_at; i++) {
	    Q_UINT32 serNum = mSearchResult.pop();
	    kernel->msgDict()->getLocation(serNum, &folder, &idx);
	    if (!folder || (idx == -1))
		continue;
	    if(mSearch->inScope(folder)) {
		found++;
		emit proxyFound(serNum);
	    }
	}
	mSearch->setSearchedCount(mSearch->searchedCount()+stop_at);
	if(found)
	    mSearch->setFoundCount(mSearch->foundCount()+found);
	if(mSearchResult.isEmpty()) 
	    finished = TRUE;
    } else {
	finished = TRUE;
    }
    if(finished) {
	if(mSearch && mSearch->running())
	    mSearch->setRunning(FALSE);
	stop(FALSE);
	killTimer(mId);
	kernel->msgIndex()->stopQuery(id());
    }
    //!!!!! do nothing else because we might be deleted..
}
bool
KMMsgIndex::startQuery(KMSearch *s)
{
    if(!canHandleQuery(s->searchPattern()))
	return FALSE;
    KMIndexSearchTarget *targ = new KMIndexSearchTarget(s);
    mActiveSearches.insert(targ->id(), targ);
    return TRUE;
}
bool
KMMsgIndex::stopQuery(KMSearch *s)
{
    int id = -1;
    for(QIntDictIterator<KMIndexSearchTarget> it(mActiveSearches); 
	it.current(); ++it) {
	if(it.current()->search() == s) {
	    it.current()->stop(FALSE);
	    id = it.currentKey();
	    break;
	}
    }
    if(id == -1) 
	return FALSE;
    return stopQuery(id);
}
bool
KMMsgIndex::stopQuery(int id)
{
    return mActiveSearches.remove(id);
}
