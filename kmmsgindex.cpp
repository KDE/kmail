
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

enum {
    HEADER_BYTEORDER = 0,
    HEADER_COUNT = 1,
    HEADER_USED = 2,
    HEADER_KNOWN = 3,
    HEADER_end = 4,

    CHUNK_HEADER_COUNT = 0,
    CHUNK_HEADER_USED = 1,
    CHUNK_HEADER_NEXT = 2,
    CHUNK_HEADER_end = 3
};


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
	mTermIndex.ref[HEADER_COUNT] = mTermIndex.count;
    }
    mTermIndex.ref[HEADER_USED] = mTermIndex.used;
    return ret;
}

bool 
KMMsgIndex::isKillTerm(const char *term, uchar term_len)
{
    if(!term || term_len < 1)
	return TRUE;
#if 1
    if(term_len == 1) //one letter just isn't enough..
	return TRUE;
#endif
#if 1
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
#if 1
    { //static words lists, maybe hacky
	static QDict<void> *killDict = NULL;
	if(!killDict) {
	    killDict = new QDict<void>();
	    const char *kills[] = { "from",
				    "for", "to", "the", "but", NULL };
	    for(int i = 0; kills[i]; i++)
		killDict->insert(kills[i], (void*)1);
	}
	if(killDict->find(term))
	    return TRUE;
    }
#endif
    return FALSE;
}

int
KMMsgIndex::addBucket(int where, Q_UINT32 serNum)
{
    int ret = where;
    if(where == -1) {
	int first_chunk_size = 6; //just enough for the one..
	int off = ret = allocTermChunk(first_chunk_size);

	//special case to mark the tail for the first
	mTermIndex.ref[off++] = off+1;
	first_chunk_size--;

	//now mark in index
	mTermIndex.ref[off+CHUNK_HEADER_COUNT] = first_chunk_size;
	mTermIndex.ref[off+CHUNK_HEADER_USED] = 4; //include the header
	mTermIndex.ref[off+CHUNK_HEADER_end] = serNum;
    } else {
	uint len = mTermIndex.ref[where+CHUNK_HEADER_COUNT];
	if(len == mTermIndex.ref[where+CHUNK_HEADER_USED]) {
	    len *= 2; //double
	    int blk = ret = allocTermChunk(len); 
	    mTermIndex.ref[where+CHUNK_HEADER_NEXT] = blk;
	    mTermIndex.ref[blk+CHUNK_HEADER_COUNT] = len;
	    mTermIndex.ref[blk+CHUNK_HEADER_USED] = 4; //include the header
	    mTermIndex.ref[blk+CHUNK_HEADER_end] = serNum;
	} else {
	    mTermIndex.ref[where+
			   mTermIndex.ref[where+CHUNK_HEADER_USED]] = serNum;
	    mTermIndex.ref[where+CHUNK_HEADER_USED]++;
	}
    }
    return ret;
}

bool
KMMsgIndex::addTerm(const char *term, uchar term_len, Q_UINT32 serNum)
{
    if(mTermIndex.ref == MAP_FAILED)
	return FALSE;
    if(isKillTerm(term, term_len)) 
	return TRUE; //sorta..

    if(!mTermTOC.map.contains(term)) {
	int w = addBucket(-1, serNum);
	mTermTOC.map.insert(term, w); //mark in TOC
	write(mTermTOC.fd, &term_len, sizeof(term_len));
	write(mTermTOC.fd, term, term_len);
	write(mTermTOC.fd, &w, sizeof(w));
    } else {
	int map_off = mTermTOC.map[term], 
		  w = addBucket(mTermIndex.ref[map_off], serNum);
	if(w != -1) 
	    mTermIndex.ref[map_off] = w;
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

    uchar build_i = 0;
    char build_str[255];
    const char *body_s = body.data();
    for(uint i = 0; i < body.length(); i++) {
	if(build_i < 253 && 
	   (isalnum(body_s[i]) || 
	    ((body_s[i] == '.' || body_s[i] == '-' || body_s[i] == '\'') && 
	     body_s[i+1] != '\n' && 
	     body_s[i+1] != '\t' && body_s[i+1] != ' '))) {
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

    mTermIndex.ref[HEADER_KNOWN] = ++mTermIndex.known; //update known count
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
    mTermIndex.ref[HEADER_BYTEORDER] = byteOrder;
    mTermIndex.ref[HEADER_COUNT] = mTermIndex.count;
    mTermIndex.ref[HEADER_USED] = mTermIndex.used; //including this header
    mTermIndex.ref[HEADER_KNOWN] = mTermIndex.known;

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
		uchar len;
		char *in = NULL;
		int in_len = 0;
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
	if((*it)->field() == "<body>") {
	    if((*it)->function() != KMSearchRule::FuncContains)
		return FALSE;
	    QString match = (*it)->contents().lower();
	    if(match.contains(' ')) {
		uint killed = 0;
		QStringList sl = QStringList::split(' ', match);
		for(QStringList::Iterator it = sl.begin(); 
		    it != sl.end(); ++it) {
		    QString str = (*it).lower();
		    if(isKillTerm(str.latin1(), str.length()))
			killed++;
		}
		if(killed == sl.count())
		    return FALSE;
	    } else if(isKillTerm(match.latin1(), match.length())) {
		return FALSE;
	    }
	} else {
	    return FALSE;
	}
    }
    return TRUE;
}

QValueList<Q_UINT32>
KMMsgIndex::values(int begin_chunk, int end_chunk)
{
    QValueList<Q_UINT32> ret;
    for(int off = begin_chunk; TRUE; 
	off = mTermIndex.ref[off+CHUNK_HEADER_NEXT]) {
	uint used = mTermIndex.ref[off+CHUNK_HEADER_USED];
	for(uint i = CHUNK_HEADER_end; i < used; i++) 
	    ret << mTermIndex.ref[off+i];
	if(mTermIndex.ref[off] != used || off == end_chunk) 
		break;
    }
    return ret;
}

QValueList<Q_UINT32> // Actually does the search.. 
KMMsgIndex::query(KMSearchRule *rule, bool full_phrase_search)
{
    QValueList<Q_UINT32> ret;
    if(rule->field() == "<body>" && 
       rule->function() == KMSearchRule::FuncContains) {
	QString match = rule->contents().lower();
	if(match.contains(' ')) { //phrase search..
	    QValueList<Q_UINT32> tmp;
	    QStringList sl = QStringList::split(' ', match);
	    for(QStringList::Iterator it = sl.begin(); it != sl.end(); ++it) {
		QString str = (*it).lower();
		if(!isKillTerm(str.latin1(), str.length())) {
		    int map_off = mTermTOC.map[match];
		    if(ret.isEmpty()) {
			ret = values(map_off+1, mTermIndex.ref[map_off]);
		    } else {
			tmp = values(map_off+1, mTermIndex.ref[map_off]);
			if(tmp.count() < ret.count())
			    ret = tmp;
		    }
		}
	    }
	    if(full_phrase_search) {
		tmp.clear();
		for(QValueListIterator<Q_UINT32> it = ret.begin(); 
		    it != ret.end(); ++it) {
		    int idx = -1;
		    KMMessage msg;
		    KMFolder *folder = 0;
		    kernel->msgDict()->getLocation((*it), &folder, &idx);
		    if(!folder || (idx == -1))
			continue;
		    if(rule->matches(folder->getDwString(idx), msg))
			tmp << (*it);
		}
	    }
	    ret = tmp;
	} else if(!isKillTerm(match.latin1(), match.length()) &&
		  mTermTOC.map.contains(match)) {
	    int map_off = mTermTOC.map[match];
	    ret = values(map_off+1, mTermIndex.ref[map_off]);
	}
    }
    return ret;
}

QValueList<Q_UINT32>
KMMsgIndex::query(KMSearchPattern *pat, bool full_phrase_search)
{
    QValueList<Q_UINT32> ret;
    if(pat->isEmpty() || !canHandleQuery(pat))
	return ret;

    if(pat->count() == 1) {
	ret = query(pat->first(), full_phrase_search);
    } else {
	bool first = TRUE;
	QIntDict<void> foundDict;
	for(QPtrListIterator<KMSearchRule> it(*pat); it.current(); ++it) {
	    QValueList<Q_UINT32> tmp = query((*it), full_phrase_search);
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
	    s->searchPattern(), FALSE);
	for(QValueListConstIterator<Q_UINT32> it = lst.begin();
	    it != lst.end(); ++it) 
	    mSearchResult.push((*it));
    }
    if(s->searchPattern()->count() == 1) {
	mVerifyResult = s->searchPattern()->first()->contents().contains(' ');
    } else {
	for(QPtrListIterator<KMSearchRule> it(*s->searchPattern()); 
	    it.current(); ++it) {
	    if((*it)->contents().contains(' ')) {
		mVerifyResult = TRUE;
		break;
	    }
	}
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
	const uint max_src = mVerifyResult ? 20 : 100;
	int stop_at = QMIN(mSearchResult.count(), max_src), found = 0;
	for(int i = 0, idx; i < stop_at; i++) {
	    Q_UINT32 serNum = mSearchResult.pop();
	    kernel->msgDict()->getLocation(serNum, &folder, &idx);
	    if (!folder || (idx == -1))
		continue;
	    if(mVerifyResult && //full phrase..
	       !mSearch->searchPattern()->matches(folder->getDwString(idx)))
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
	stop(TRUE);
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
#include "kmmsgindex.moc"
