// Author: Sam Magnuson <zachsman@wiw.org>
// License GPL -- indexing logic for KMail

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmmsgindex.h"

#include "kmsearchpattern.h"
#include "kmfoldersearch.h"
#include "kmfoldermgr.h"
#include "kmmsgdict.h"
#include "kmkernel.h"

#include "mimelib/message.h"
#include "mimelib/headers.h"
#include "mimelib/utility.h"
#include "mimelib/enum.h"
#include "mimelib/body.h"
#include "mimelib/bodypart.h"
#include "mimelib/field.h"

#include <kdebug.h>

#include <qdict.h>
#include <qapplication.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

//#define USE_MMAP
class
KMMsgIndexRef
{
#ifdef USE_MMAP
    Q_UINT32 *mRef;
#endif
    int mFD, mSize;
public:
    KMMsgIndexRef(int f, int size);
    ~KMMsgIndexRef() { }
    bool error();

    void resize(int size);
    void sync();

    bool write(int off, Q_UINT32 val);
    Q_UINT32 read(int off, bool *ok=NULL);
};
KMMsgIndexRef::KMMsgIndexRef(int f, int size) :
    mFD(f), mSize(size)
{
#ifdef USE_MMAP
    if(mSize != 0)
	mRef = (Q_UINT32*)mmap(0, mSize * sizeof(Q_INT32),
			       PROT_READ|PROT_WRITE, MAP_SHARED, mFD, 0);
    else
	mRef = (Q_UINT32*)MAP_FAILED;
#endif
}
void KMMsgIndexRef::sync()
{
#ifdef USE_MMAP
    if(mRef != MAP_FAILED)
	msync(mRef, mSize * sizeof(Q_INT32), MS_SYNC);
#endif
}
bool KMMsgIndexRef::error()
{
#ifdef USE_MMAP
    if(mRef == MAP_FAILED)
	return TRUE;
#endif
    return FALSE;
}
void KMMsgIndexRef::resize(int newSize)
{
#ifdef USE_MMAP
    if(mRef != MAP_FAILED)
	munmap(mRef, mSize * sizeof(Q_INT32));
    if(ftruncate(mFD, newSize * sizeof(Q_INT32)) == -1) {
	for(Q_INT32 i = mSize; i < newSize; i++)
	    ::write(mFD, &i, sizeof(i));
    }
#endif
    mSize = newSize;
#ifdef USE_MMAP
    mRef = (Q_UINT32*)mmap(0, mSize * sizeof(Q_INT32),
			   PROT_READ|PROT_WRITE, MAP_SHARED, mFD, 0);
#endif
}
bool KMMsgIndexRef::write(int off, Q_UINT32 val)
{
    if(off > mSize)
	return FALSE;
#ifdef USE_MMAP
    mRef[off] = val;
#else
    lseek(mFD, off * sizeof(Q_INT32), SEEK_SET);
    ::write(mFD, &val, sizeof(val));
#endif
    return TRUE;
}
Q_UINT32 KMMsgIndexRef::read(int off, bool *ok)
{
    if(off > mSize) {
	if(ok)
	    *ok = FALSE;
	return 0;
    }
#ifdef USE_MMAP
    return mRef[off];
#else
    Q_UINT32 ret;
    lseek(mFD, off * sizeof(Q_INT32), SEEK_SET);
    ::read(mFD, &ret, sizeof(ret));
    return ret;
#endif
    return 0;
}

inline bool
km_isSeparator(const char *content, uint i, uint content_len)
{
    return !(isalnum(content[i]) ||
	    (i < content_len - 1 && content[i+1] != '\n' &&
	     content[i+1] != '\t' && content[i+1] != ' ' &&
	     (content[i] == '.' || content[i] == '-' ||
	      content[i] == '\\' || content[i] == '/' ||
	      content[i] == '\'' || content[i] == ':')));
}

inline bool
km_isSeparator(const QChar *content, uint i, uint content_len)
{
    return !(content[i].isLetterOrNumber() ||
	    (i < content_len - 1 && content[i+1] != '\n' &&
	     content[i+1] != '\t' && content[i+1] != ' ' &&
	     (content[i] == '.' || content[i] == '-' ||
	      content[i] == '\\' || content[i] == '/' ||
	      content[i] == '\'' || content[i] == ':')));
}

inline bool
km_isSeparator(const QString &s, int i, int content_len=-1)
{
    return km_isSeparator(s.unicode(), i,
			  content_len < 0 ? s.length() : content_len);
}

inline bool
km_isSeparated(const QString &f)
{
    for(uint i=0, l=f.length(); i < f.length(); i++) {
	if(km_isSeparator(f.unicode(), i, l))
	    return TRUE;
    }
    return FALSE;
}

inline QStringList
km_separate(const QString &f)
{
    if(!km_isSeparated(f))
	return QStringList(f);
    QStringList ret;
    uint i_o = 0;
    for(uint i=0, l=f.length(); i < f.length(); i++) {
	if(km_isSeparator(f.unicode(), i, l)) {
	    QString chnk = f.mid(i_o, i - i_o).latin1();
	    if(!chnk.isEmpty())
		ret << chnk;
	    i_o = i+1;
	}
    }
    if(i_o != f.length()) {
	QString chnk = f.mid(i_o, f.length() - i_o);
	if(!chnk.isEmpty())
	    ret << chnk;
    }
    return ret;
}

enum {
    HEADER_BYTEORDER = 0,
    HEADER_VERSION = 1,
    HEADER_COMPLETE = 2,
    HEADER_COUNT = 3,
    HEADER_USED = 4,
    HEADER_INDEXED = 5,
    HEADER_REMOVED = 6,
    HEADER_end = 7,

    CHUNK_HEADER_COUNT = 0,
    CHUNK_HEADER_USED = 1,
    CHUNK_HEADER_NEXT = 2,
    CHUNK_HEADER_end = 3,

    TOC_BODY = 0,
    TOC_HEADER_NAME = 1,
    TOC_HEADER_DATA = 2
};
#define KMMSGINDEX_VERSION 6067
static int kmindex_grow_increment = 40960; //grow this many buckets at a time

KMMsgIndex::KMMsgIndex(QObject *o, const char *n) :
    QObject(o, n), mIndexState(INDEX_IDLE), delay_cnt(0), mLastSearch()
{
    mTermIndex.loc = kmkernel->folderMgr()->basePath() + "/.kmmsgindex_search";
    mTermTOC.loc = kmkernel->folderMgr()->basePath() + "/.kmmsgindex_toc";
    mTermProcessed.loc = kmkernel->folderMgr()->basePath() +
			 "/.kmmsgindex_processed";
}

void KMMsgIndex::init()
{
    mActiveSearches.setAutoDelete(TRUE);
    reset(FALSE);
    readIndex();
    connect(kmkernel->folderMgr(), SIGNAL(msgRemoved(KMFolder*, Q_UINT32)),
	    this, SLOT(slotRemoveMsg(KMFolder*, Q_UINT32)));
    connect(kmkernel->folderMgr(), SIGNAL(msgAdded(KMFolder*, Q_UINT32)),
	    this, SLOT(slotAddMsg(KMFolder*, Q_UINT32)));
}

void KMMsgIndex::remove()
{
    unlink(mTermIndex.loc.latin1());
    unlink(mTermTOC.loc.latin1());
    unlink(mTermProcessed.loc.latin1());
}

// resets the state of the indexer to nothing (if clean it is assumed
// anything not initialized is cleaned up..
void
KMMsgIndex::reset(bool clean)
{
    //active searches
    if(clean)
	mActiveSearches.clear();
    //create
    if(create.timer_id != -1) {
	if(clean)
	    killTimer(create.timer_id);
	create.timer_id = -1;
    }
    //restore
    if(restore.timer_id != -1) {
	if(clean)
	    killTimer(restore.timer_id);
	restore.timer_id = -1;
    }
    //TOC
    if(clean)
	mTermTOC.body.clear();
    if(mTermTOC.fd != -1) {
	if(clean)
	    close(mTermTOC.fd);
	mTermTOC.fd = -1;
    }
    mTermTOC.h.next_hnum = 0;
    if(clean) {
	mTermTOC.h.header_lookup.clear();
	mTermTOC.h.headers.clear();
    }
    //processed
    if(mTermProcessed.fd != -1) {
	if(clean)
	    close(mTermProcessed.fd);
	mTermProcessed.fd = -1;
    }
    if(clean)
	mTermProcessed.known.clear();
    restore.reading_processed = FALSE;
    //index
    {
	if(clean)
	    delete mTermIndex.ref;
	mTermIndex.ref = NULL;
    }
    mTermIndex.removed = mTermIndex.indexed = 0;
    mTermIndex.used = mTermIndex.count = 0;
    if(mTermIndex.fd != -1) {
	if(clean)
	    close(mTermIndex.fd);
	mTermIndex.fd = -1;
    }
}

// finds cnt buckets and allocates to make room
int
KMMsgIndex::allocTermChunk(int cnt)
{
    int ret = mTermIndex.used;
    mTermIndex.used += cnt; //update the used
    if(mTermIndex.count < mTermIndex.used) { //time for a remap
	mTermIndex.count = QMAX(mTermIndex.count + kmindex_grow_increment,
				mTermIndex.used);
	mTermIndex.ref->resize(mTermIndex.count);
	mTermIndex.ref->write(HEADER_COUNT, mTermIndex.count);
    }
    mTermIndex.ref->write(HEADER_USED, mTermIndex.used);
    return ret;
}

// returns whether header is a header we care about..
bool
KMMsgIndex::isKillHeader(const char *header, uchar header_len)
{
    const char *watched_headers[] = { "Subject", "From", "To", "CC", "BCC",
				      "Reply-To", "Organization", "List-ID",
				      "X-Mailing-List", "X-Loop", "X-Mailer",
				      NULL };
    for(int i = 0; watched_headers[i]; i++) {
	if(!strncmp(header, watched_headers[i], header_len))
	    return FALSE;
    }
    return TRUE;
}

//returns whether a term is in the stop list..
bool
KMMsgIndex::isKillTerm(const char *term, uchar term_len)
{
    if(!term || term_len < 1)
	return TRUE;
    if(term_len <= 2) //too few letters..
	return TRUE;
    { //no numbers!
	int numlen = 0;
	if(term[numlen] == '+' || term[numlen] == '-')
	    numlen++;
	for( ; numlen < term_len; numlen++) {
	    if(!isdigit(term[numlen]) || term[numlen] == '.')
		break;
	}
	if(numlen == term_len - 1 && term[numlen] == '%')
	    numlen++;
	if(numlen == term_len)
	    return TRUE;
    }
    { //static kill words list
	static QDict<void> *killDict = NULL;
	if(!killDict) {
	    killDict = new QDict<void>();
	    const char *kills[] = { "from", "kmail", "is", "in", "and",
				    "it", "this", "of", "that", "on",
				    "you", "if", "be", "not",
				    "with", "for", "to", "the", "but",
				    NULL };
	    for(int i = 0; kills[i]; i++)
		killDict->insert(kills[i], (void*)1);
	}
	if(killDict->find(term))
	    return TRUE;
    }
    return FALSE;
}

// finds a free bucket starting at where to put serNum in the dbase
int
KMMsgIndex::addBucket(int where, Q_UINT32 serNum)
{
    int ret = where;
    if(where == -1) {
	//enough for two (and the tail)..
	int first_chunk_size = CHUNK_HEADER_end + 2 + 1;
	int off = ret = allocTermChunk(first_chunk_size);

	//special case to mark the tail for the first
	mTermIndex.ref->write(off, off+1);
	off++;
	first_chunk_size--;

	//now mark in index
	mTermIndex.ref->write(off+CHUNK_HEADER_COUNT, first_chunk_size);
	mTermIndex.ref->write(off+CHUNK_HEADER_USED, CHUNK_HEADER_end + 1);
	mTermIndex.ref->write(off+CHUNK_HEADER_end, serNum);
    } else {
	uint len = mTermIndex.ref->read(where+CHUNK_HEADER_COUNT);
	if(len == mTermIndex.ref->read(where+CHUNK_HEADER_USED)) {
	    len = 34; //let's make a bit more room this time..
	    int blk = ret = allocTermChunk(len);
	    mTermIndex.ref->write(where+CHUNK_HEADER_NEXT, blk);
	    mTermIndex.ref->write(blk+CHUNK_HEADER_COUNT, len);
	    mTermIndex.ref->write(blk+CHUNK_HEADER_USED, CHUNK_HEADER_end + 1);
	    mTermIndex.ref->write(blk+CHUNK_HEADER_end, serNum);
	} else {
	    mTermIndex.ref->write(where+
		  mTermIndex.ref->read(where+CHUNK_HEADER_USED), serNum);
	    mTermIndex.ref->write(where+CHUNK_HEADER_USED,
			  mTermIndex.ref->read(where+CHUNK_HEADER_USED)+1);
	}
    }
    return ret;
}

// adds the body term to the index
bool
KMMsgIndex::addBodyTerm(const char *term, uchar term_len, Q_UINT32 serNum)
{
    if(mTermIndex.ref->error())
	return FALSE;
    if(isKillTerm(term, term_len))
	return TRUE; //sorta..
    if(mIndexState == INDEX_RESTORE) //just have to finish reading..
	restoreState();

    if(!mTermTOC.body.contains(term)) {
	int w = addBucket(-1, serNum);
	mTermTOC.body.insert(term, w); //mark in TOC
	const uchar marker = TOC_BODY;
	write(mTermTOC.fd, &marker, sizeof(marker));
	write(mTermTOC.fd, &term_len, sizeof(term_len));
	write(mTermTOC.fd, term, term_len);
	write(mTermTOC.fd, &w, sizeof(w));
    } else {
	int map_off = mTermTOC.body[term],
		  w = addBucket(mTermIndex.ref->read(map_off), serNum);
	if(w != -1)
	    mTermIndex.ref->write(map_off, w);
    }
    return TRUE;
}

// adds the body term to the index
bool
KMMsgIndex::addHeaderTerm(Q_UINT16 hnum, const char *term,
			  uchar term_len, Q_UINT32 serNum)
{
    if(mTermIndex.ref->error())
	return FALSE;
    if(isKillTerm(term, term_len))
	return TRUE; //sorta..
    if(mIndexState == INDEX_RESTORE) //just have to finish reading..
	restoreState();

    if(!mTermTOC.h.headers.contains(hnum))
	mTermTOC.h.headers.insert(hnum, QMap<QCString, int>());
    if(!mTermTOC.h.headers[hnum].contains(term)) {
	int w = addBucket(-1, serNum);
	mTermTOC.h.headers[hnum].insert(term, w);
	const uchar marker = TOC_HEADER_DATA;
	write(mTermTOC.fd, &marker, sizeof(marker));
	write(mTermTOC.fd, &hnum, sizeof(hnum));
	write(mTermTOC.fd, &term_len, sizeof(term_len));
	write(mTermTOC.fd, term, term_len);
	write(mTermTOC.fd, &w, sizeof(w));
    } else {
	int map_off = mTermTOC.h.headers[hnum][term],
		  w = addBucket(mTermIndex.ref->read(map_off), serNum);
	if(w != -1)
	    mTermIndex.ref->write(map_off, w);
    }
    return TRUE;
}

// processes the message at serNum and returns the number of terms processed
int
KMMsgIndex::processMsg(Q_UINT32 serNum)
{
    if(mIndexState == INDEX_RESTORE) {
	create.serNums.push(serNum);
	return -1;
    }
    if(mTermProcessed.known[serNum])
	return -1;

    int idx = -1;
    KMFolder *folder = 0;
    kmkernel->msgDict()->getLocation(serNum, &folder, &idx);
    if(!folder || (idx == -1) || (idx >= folder->count()))
	return -1;
    if(mOpenedFolders.findIndex(folder) == -1) {
	folder->open();
	mOpenedFolders.append(folder);
    }

    int ret = 0;
    bool unget = !folder->getMsgBase(idx)->isMessage();
    KMMessage *msg = folder->getMsg(idx);
    const DwMessage *dw_msg = msg->asDwMessage();
    DwHeaders& headers = dw_msg->Headers();
    uchar build_i = 0;
    char build_str[255];

    //process header
    for(DwField *field = headers.FirstField(); field; field = field->Next()) {
	if(isKillHeader(field->FieldNameStr().data(),
			field->FieldNameStr().length()))
	    continue;
	const char *name    = field->FieldNameStr().c_str(),
		   *content = field->FieldBodyStr().data();
	uint content_len = field->FieldBodyStr().length();
	Q_UINT16 hnum = 0;
	if(mTermTOC.h.header_lookup.contains(name)) {
	    hnum = mTermTOC.h.header_lookup[name];
	} else {
	    hnum = mTermTOC.h.next_hnum++;
	    mTermTOC.h.header_lookup.insert(name, hnum);
	    const uchar marker = TOC_HEADER_NAME;
	    write(mTermTOC.fd, &marker, sizeof(marker));
	    uchar len = field->FieldNameStr().length();
	    write(mTermTOC.fd, &len, sizeof(len));
	    write(mTermTOC.fd, name, len);
	    write(mTermTOC.fd, &hnum, sizeof(hnum));
	}
	for(uint i = 0; i < content_len; i++) {
	    if(build_i < 254 && !km_isSeparator(content, i, content_len)) {
		build_str[build_i++] = tolower(content[i]);
	    } else if(build_i) {
		build_str[build_i] = 0;
		if(addHeaderTerm(hnum, build_str, build_i, serNum))
		    ret++;
		build_i = 0;
	    }
	}
	if(build_i) {
	    build_str[build_i] = 0;
	    if(addHeaderTerm(hnum, build_str, build_i, serNum))
		ret++;
	    build_i = 0;
	}
    }

    //process body
    const DwEntity *dw_ent = msg->asDwMessage();
    DwString dw_body;
    DwString body;
    if(dw_ent && dw_ent->hasHeaders() && dw_ent->Headers().HasContentType() &&
       (dw_ent->Headers().ContentType().Type() == DwMime::kTypeText)) {
	dw_body = dw_ent->Body().AsString();
    } else {
	dw_ent = msg->getFirstDwBodyPart();
	if (dw_ent)
	    dw_body = msg->getFirstDwBodyPart()->AsString();
    }
    if(dw_ent && dw_ent->hasHeaders() && dw_ent->Headers().HasContentType() &&
       (dw_ent->Headers().ContentType().Type() == DwMime::kTypeText)) {
      DwHeaders& headers = dw_ent->Headers();
      if(headers.HasContentTransferEncoding()) {
	switch(headers.ContentTransferEncoding().AsEnum()) {
	case DwMime::kCteBase64: {
	    DwString raw_body = dw_body;
	    DwDecodeBase64(raw_body, body);
	    break; }
	case DwMime::kCteQuotedPrintable: {
	    DwString raw_body = dw_body;
	    DwDecodeQuotedPrintable(raw_body, body);
	    break; }
	default:
	    body = dw_body;
	    break;
	}
      } else {
	  body = dw_body;
      }
    }
    QDict<void> found_terms;
    const char *body_s = body.data();
    for(uint i = 0; i < body.length(); i++) {
	if(build_i < 254 && !km_isSeparator(body_s, i, body.length())) {
	    build_str[build_i++] = tolower(body_s[i]);
	} else if(build_i) {
	    build_str[build_i] = 0;
	    if(!found_terms[build_str] && addBodyTerm(build_str, build_i,
						      serNum)) {
		found_terms.insert(build_str, (void*)1);
		ret++;
	    }
	    build_i = 0;
	}
    }
    if(build_i) {
	build_str[build_i] = 0;
	if(!found_terms[build_str] && addBodyTerm(build_str,
						  build_i, serNum)) {
	    found_terms.insert(build_str, (void*)1);
	    ret++;
	}
    }
    if (unget)
      folder->unGetMsg(idx); //I don't need it anymore..
    mTermIndex.ref->write(HEADER_INDEXED, ++mTermIndex.indexed);
    mTermProcessed.known.insert(serNum, (void*)1);
    write(mTermProcessed.fd, &serNum, sizeof(serNum));
    return ret;
}

//Determines if it is time for another cleanup
bool
KMMsgIndex::isTimeForClean()
{
    return (mTermIndex.removed > 2500 && //minimum
	    mTermIndex.removed * 4 >= mTermIndex.indexed && //fraction removed
	    (mLastSearch.isNull() ||  //never
	     mLastSearch.secsTo(QTime::currentTime()) > 60 * 60 * 2)); //hours
}

//removes bogus entries from the index, and optimizes the index file
void
KMMsgIndex::cleanUp()
{
    if(mIndexState != INDEX_IDLE)
	return;
    reset(TRUE);
    remove();
    recreateIndex();
}

// flushes all open file descriptors..
void
KMMsgIndex::flush()
{
#if 0
    mTermIndex.ref->sync();
    sync();
#endif
}

// slot fired when a serial number is no longer used
void
KMMsgIndex::slotRemoveMsg(KMFolder *, Q_UINT32)
{
    mTermIndex.ref->write(HEADER_REMOVED, ++mTermIndex.removed);
}

// slot fired when new serial numbers are allocated..
void
KMMsgIndex::slotAddMsg(KMFolder *, Q_UINT32 serNum)
{
    if(mIndexState == INDEX_CREATE) {
	create.serNums.push(serNum);
    } else if(isTimeForClean()) {
	cleanUp();
    } else {
	processMsg(serNum);
	flush();
    }
}

// handles the lazy processing of messages
void
KMMsgIndex::timerEvent(QTimerEvent *e)
{
    if(qApp->hasPendingEvents()) //nah, some other time..
	delay_cnt = 10;
    else if(delay_cnt)
	--delay_cnt;
    else if(mIndexState == INDEX_CREATE && e->timerId() == create.timer_id)
	createState(FALSE);
    else if(mIndexState == INDEX_RESTORE && e->timerId() == restore.timer_id)
	restoreState(FALSE);
}

bool
KMMsgIndex::createState(bool finish)
{
    int terms = 0, processed = 0, skipped = 0;
    const int max_terms = 300, max_process = 30;
    while(!create.serNums.isEmpty()) {
	if(!finish &&
	   (terms >= max_terms || processed >= max_process ||
	    skipped >= (max_process*4))) {
	    flush();
	    return TRUE;
	}
	int cnt = processMsg(create.serNums.pop());
	if(cnt == -1) {
	    skipped++;
	} else {
	    terms += cnt;
	    processed++;
	}
    }
    if(KMFolder *f = create.folders.pop()) {
	if(mOpenedFolders.findIndex(f) == -1) {
	    f->open();
	    mOpenedFolders.append(f);
	}
	for(int i = 0, s; i < f->count(); ++i) {
	    s = kmkernel->msgDict()->getMsgSerNum(f, i);
	    if(finish ||
	       (terms < max_terms && processed < max_process &&
		skipped < (max_process*4))) {
		int cnt = processMsg(s);
		if(cnt == -1) {
		    skipped++;
		} else {
		    terms += cnt;
		    processed++;
		}
	    } else if(!mTermProcessed.known[s]){
		create.serNums.push(s);
	    }
	}
	if(finish) {
	    while(!createState(TRUE));
	    return TRUE;
	}
    } else {
	mIndexState = INDEX_IDLE;
	killTimer(create.timer_id);
	create.timer_id = -1;
	QValueListConstIterator<QGuardedPtr<KMFolder> > it;
	for (it = mOpenedFolders.begin();
	     it != mOpenedFolders.end(); ++it) {
	    KMFolder *folder = *it;
	    if(folder)
		folder->close();
	}
	mOpenedFolders.clear();
	create.folders.clear();
	mTermIndex.ref->write(HEADER_COMPLETE, 1);
	return TRUE;
    }
    flush();
    return FALSE;
}

// reads in some terms from the index (non-blocking) if finish is true it
// will read in everything left to do. It is possible for this to turn from the
// RESTORE state into the CREATE state - so you must handle this case if you
// need to use the index immediately (ie finish is true)
bool
KMMsgIndex::restoreState(bool finish) {
    if(mIndexState != INDEX_RESTORE)
	return FALSE;
    uchar marker, len;
    char in[255];
    Q_UINT32 off;
    for(int cnt = 0; finish || cnt < 400; cnt++) {
	if(restore.reading_processed) {
	    Q_UINT32 ser;
	    if(!read(mTermProcessed.fd, &ser, sizeof(ser))) {
		mIndexState = INDEX_IDLE;
		killTimer(restore.timer_id);
		restore.timer_id = -1;
		if(restore.restart_index) {
		    mIndexState = INDEX_CREATE;
		    syncIndex();
		}
		break;
	    }
	    mTermProcessed.known.insert(ser, (void*)1);
	} else {
	    if(!read(mTermTOC.fd, &marker, sizeof(marker)))
		restore.reading_processed = TRUE;
	    if(marker == TOC_BODY) {
		read(mTermTOC.fd, &len, sizeof(len));
		read(mTermTOC.fd, in, len);
		in[len] = 0;
		read(mTermTOC.fd, &off, sizeof(off));
		mTermTOC.body.insert(in, off);
	    } else if(marker == TOC_HEADER_DATA) {
		Q_UINT16 hnum;
		read(mTermTOC.fd, &hnum, sizeof(hnum));
		read(mTermTOC.fd, &len, sizeof(len));
		read(mTermTOC.fd, in, len);
		in[len] = 0;
		read(mTermTOC.fd, &off, sizeof(off));
		if(!mTermTOC.h.headers.contains(hnum)) {
		    QMap<QCString, int> map;
		    map.insert(in, off);
		    mTermTOC.h.headers.insert(hnum, map);
		} else {
		    mTermTOC.h.headers[hnum].insert(in, off);
		}
	    } else if(marker == TOC_HEADER_NAME) {
		read(mTermTOC.fd, &len, sizeof(len));
		read(mTermTOC.fd, in, len);
		in[len] = 0;
		Q_UINT16 hnum;
		read(mTermTOC.fd, &hnum, sizeof(hnum));
		if(!mTermTOC.h.header_lookup.contains(in)) {
		    mTermTOC.h.header_lookup.insert(in, hnum);
		    mTermTOC.h.next_hnum = hnum + 1;
		}
	    }
	}
    }
    return TRUE;
}

// nulls the current index and begins a refresh of the indexed data..
bool
KMMsgIndex::recreateIndex()
{
    if(mIndexState != INDEX_IDLE)
	return FALSE;

    mIndexState = INDEX_CREATE;
    mTermProcessed.fd = open(mTermProcessed.loc.latin1(),
			     O_WRONLY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
    mTermTOC.fd = open(mTermTOC.loc.latin1(),
		       O_RDWR|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
    mTermIndex.fd = open(mTermIndex.loc.latin1(),
			 O_RDWR|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE);
    mTermIndex.count = kmindex_grow_increment;
    mTermIndex.used = HEADER_end;
    mTermIndex.ref = new KMMsgIndexRef(mTermIndex.fd, 0);
    mTermIndex.ref->resize(mTermIndex.count);
    mTermIndex.ref->write(HEADER_BYTEORDER, 0x12345678);
    mTermIndex.ref->write(HEADER_VERSION, KMMSGINDEX_VERSION);
    mTermIndex.ref->write(HEADER_COMPLETE, 0); //marker for incomplete index
    mTermIndex.ref->write(HEADER_COUNT, mTermIndex.count);
    mTermIndex.ref->write(HEADER_USED, mTermIndex.used);//including this header
    mTermIndex.ref->write(HEADER_INDEXED, mTermIndex.indexed);
    mTermIndex.ref->write(HEADER_REMOVED, mTermIndex.removed);
    syncIndex();
    return TRUE;
}

// processes all current messages as if they were newly added
void
KMMsgIndex::syncIndex()
{
    if(mIndexState != INDEX_CREATE)
	return;
    QValueStack<QGuardedPtr<KMFolderDir> > folders;
    folders.push(&(kmkernel->folderMgr()->dir()));
    while(KMFolderDir *dir = folders.pop()) {
	for(KMFolderNode *child = dir->first(); child; child = dir->next()) {
	    if(child->isDir())
		folders.push((KMFolderDir*)child);
	    else
		create.folders.push((KMFolder*)child);
	}
    }
    if(create.timer_id == -1)
	create.timer_id = startTimer(0);
}

// read the existing index and load into memory
void
KMMsgIndex::readIndex()
{
    if(mIndexState != INDEX_IDLE)
	return;
    mIndexState = INDEX_RESTORE;
    bool read_success = FALSE;
    if((mTermTOC.fd = open(mTermTOC.loc.latin1(), O_RDWR)) != -1) {
	if((mTermIndex.fd = open(mTermIndex.loc.latin1(), O_RDWR)) != -1) {
	    mTermProcessed.fd = open(mTermProcessed.loc.latin1(), O_RDWR);

	    Q_INT32 byteOrder = 0, version;
	    read(mTermIndex.fd, &byteOrder, sizeof(byteOrder));
	    if(byteOrder != 0x12345678)
		goto error_with_read;
	    read(mTermIndex.fd, &version, sizeof(version));
	    if(version != KMMSGINDEX_VERSION)
		goto error_with_read;
	    Q_UINT32 complete_index = 0;
	    read(mTermIndex.fd, &complete_index, sizeof(complete_index));
	    restore.restart_index = !complete_index;
	    read(mTermIndex.fd, &mTermIndex.count, sizeof(mTermIndex.count));
	    read(mTermIndex.fd, &mTermIndex.used, sizeof(mTermIndex.used));
	    read(mTermIndex.fd, &mTermIndex.indexed,
		 sizeof(mTermIndex.indexed));
	    read(mTermIndex.fd, &mTermIndex.removed,
		 sizeof(mTermIndex.removed));
	    mTermIndex.ref = new KMMsgIndexRef(mTermIndex.fd,
					       mTermIndex.count);
	    if(mTermIndex.ref->error())
		goto error_with_read;
	    restore.timer_id = startTimer(0);
	    read_success = TRUE;
	}
    }
error_with_read:
    if(!read_success) {
	mIndexState = INDEX_IDLE;
	reset();
	remove();
	recreateIndex();
    }
}

// returns whether rule is a valid rule to be processed by the indexer
bool
KMMsgIndex::canHandleQuery(KMSearchRule *rule)
{
    if(mIndexState == INDEX_RESTORE) //just have to finish reading..
	restoreState(); //this might flip us into INDEX_CREATE state..
    if(mIndexState != INDEX_IDLE) //not while we are doing other stuff..
	return FALSE;
    if(rule->field().isEmpty() || rule->contents().isEmpty()) //not a real search
	return TRUE;
    if(rule->function() != KMSearchRule::FuncEquals &&
       rule->function() != KMSearchRule::FuncContains) {
	return FALSE;
    } else if(rule->field().left(1) == "<") {
	if(rule->field() == "<body>" || rule->field() == "<message>") {
	    if(rule->function() != KMSearchRule::FuncContains)
		return FALSE;
	} else if(rule->field() != "<recipients>") { //unknown..
	    return FALSE;
	}
    } else if(isKillHeader(rule->field().data(), rule->field().length())) {
	return FALSE;
    }
    QString match = rule->contents().lower();     //general case
    if(km_isSeparated(match)) {
	uint killed = 0;
	QStringList sl = km_separate(match);
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
    return TRUE;
}

// returns whether pat is a valid pattern to be processed by the indexer
bool
KMMsgIndex::canHandleQuery(KMSearchPattern *pat)
{
    if(mIndexState == INDEX_RESTORE) //just have to finish reading..
	restoreState(); //this might flip us into INDEX_CREATE state..
    if(mIndexState != INDEX_IDLE) //not while we are creating the index..
	return FALSE;
    if(pat->op() != KMSearchPattern::OpAnd &&
       pat->op() != KMSearchPattern::OpOr)
	return FALSE;
    for(QPtrListIterator<KMSearchRule> it(*pat); it.current(); ++it) {
	if(!canHandleQuery((*it)))
	    return FALSE;
    }
    return TRUE;
}

// returns the data set at begin_chunk through to end_chunk
void
KMMsgIndex::values(int begin_chunk, int end_chunk, QValueList<Q_UINT32> &lst)
{
    lst.clear();
    for(int off = begin_chunk; TRUE;
	off = mTermIndex.ref->read(off+CHUNK_HEADER_NEXT)) {
	uint used = mTermIndex.ref->read(off+CHUNK_HEADER_USED);
	for(uint i = CHUNK_HEADER_end; i < used; i++)
	    lst << mTermIndex.ref->read(off+i);
	if(mTermIndex.ref->read(off) != used || off == end_chunk)
	    break;
    }
}

// returns the data set at begin_chunk through to end_chunk
void
KMMsgIndex::values(int begin_chunk, int end_chunk, QIntDict<void> &dct)
{
    dct.clear();
    for(int off = begin_chunk; TRUE;
	off = mTermIndex.ref->read(off+CHUNK_HEADER_NEXT)) {
	uint used = mTermIndex.ref->read(off+CHUNK_HEADER_USED);
	for(uint i = CHUNK_HEADER_end; i < used; i++)
	    dct.insert(mTermIndex.ref->read(off+i), (void*)1);
	if(mTermIndex.ref->read(off) != used || off == end_chunk)
	    break;
    }
}

// performs an actual search in the index
QValueList<Q_UINT32>
KMMsgIndex::find(QString data, bool contains, KMSearchRule *rule,
		 bool body, Q_UINT16 hnum)
{
    QValueList<Q_UINT32> ret;
    if(!body && !mTermTOC.h.headers.contains(hnum))
	return ret;
    if(contains) {
	QIntDict<void> foundDict;
	QMap<QCString, int> *map = &(mTermTOC.body);
	if(!body)
	    map = &(mTermTOC.h.headers[hnum]);
	QStringList sl = km_separate(data);
	for(QStringList::Iterator slit = sl.begin();
	    slit != sl.end(); ++slit) {
	    for(QMapIterator<QCString, int> it = map->begin();
		it != map->end(); ++it) {
		QString qstr = it.key();
		bool matches = FALSE;
		if(sl.count() == 1)
		    matches = qstr.contains((*slit));
		else if(slit == sl.begin())
		    matches = qstr.endsWith((*slit));
		else if(slit == sl.end())
		    matches = qstr.startsWith((*slit));
		else
		    matches = (qstr == (*slit));
		if(matches) {
		    QValueList<Q_UINT32> tmp = find(it.key(), FALSE, rule,
						    body, hnum);
		    for(QValueListIterator<Q_UINT32> tmp_it = tmp.begin();
			tmp_it != tmp.end(); ++tmp_it) {
			if(!foundDict[(*tmp_it)])
			    foundDict.insert((*tmp_it), (void*)1);
		    }
		}
	    }
	}
	for(QIntDictIterator<void> it(foundDict); it.current(); ++it)
	    ret << it.currentKey();
	return ret;
    }
    mLastSearch = QTime::currentTime();

    bool exhaustive_search = FALSE;
    if(km_isSeparated(data)) { //phrase search..
	bool first = TRUE;
	QIntDict<void> foundDict;
	QStringList sl = km_separate(data);
	for(QStringList::Iterator it = sl.begin(); it != sl.end(); ++it) {
	    if(!isKillTerm((*it).latin1(), (*it).length())) {
		QCString cstr((*it).latin1());
		int map_off = 0;
		if(body) {
		    if(!mTermTOC.body.contains(cstr))
			return ret;
		    map_off = mTermTOC.body[cstr];
		} else {
		    if(!mTermTOC.h.headers[hnum].contains(cstr))
			return ret;
		    map_off = mTermTOC.h.headers[hnum][cstr];
		}
		if(first) {
		    first = FALSE;
		    values(map_off+1, mTermIndex.ref->read(map_off),
			   foundDict);
		} else {
		    QIntDict<void> andDict;
		    QValueList<Q_UINT32> tmp;
		    values(map_off+1, mTermIndex.ref->read(map_off), tmp);
		    for(QValueListIterator<Q_UINT32> it = tmp.begin();
			it != tmp.end(); ++it) {
			if(foundDict[(*it)])
			    andDict.insert((*it), (void*)1);
		    }
		    foundDict = andDict;
		}
	    }
	}
	for(QIntDictIterator<void> it(foundDict); it.current(); ++it)
	    ret << it.currentKey();
	exhaustive_search = TRUE;
    } else if(!isKillTerm(data.latin1(), data.length())) {
	QCString cstr(data.latin1());
	int map_off = -1;
	if(body) {
	    if(mTermTOC.body.contains(cstr))
		map_off = mTermTOC.body[cstr];
	} else {
	    if(mTermTOC.h.headers[hnum].contains(cstr))
		map_off = mTermTOC.h.headers[hnum][cstr];
	}
	if(map_off != -1)
	    values(map_off+1, mTermIndex.ref->read(map_off), ret);
    }
    if(!ret.isEmpty() && rule &&
       (exhaustive_search || rule->function() == KMSearchRule::FuncEquals)) {
	QValueList<Q_UINT32> tmp;
	for(QValueListIterator<Q_UINT32> it = ret.begin();
	    it != ret.end(); ++it) {
	    int idx = -1, ser = (*it);
	    KMFolder *folder = 0;
	    kmkernel->msgDict()->getLocation(ser, &folder, &idx);
	    if(!folder || (idx == -1))
              continue;
	    KMMessage *msg = folder->getMsg(idx);
	    if(rule->matches(msg))
		tmp << ser;
	}
	return tmp;
    }
    return ret;
}


// processes rule and performs the indexed look up, if exhaustive_search
// is true it will interpret body() as a full phrase rather than AND'd set
QValueList<Q_UINT32>
KMMsgIndex::query(KMSearchRule *rule, bool exhaustive_search)
{
    if(!canHandleQuery(rule) || rule->field().isEmpty() || rule->contents().isEmpty())
	return QValueList<Q_UINT32>();
    if(rule->field().left(1) == "<") {
	if((rule->field() == "<body>" || rule->field() == "<message>") &&
	   rule->function() == KMSearchRule::FuncContains) {
	    return find(rule->contents().lower(),
			TRUE, exhaustive_search ? rule : NULL, TRUE);
	} else if(rule->field() == "<recipients>") {
	    bool first = TRUE;
	    QIntDict<void> foundDict;
	    QString contents = rule->contents().lower();
	    const char *hdrs[] = { "To", "CC", "BCC", NULL };
	    for(int i = 0; hdrs[i]; i++) {
		int l = strlen(hdrs[i]);
		if(isKillHeader(hdrs[i], l)) //can't really happen
		    continue;
		QValueList<Q_UINT32> tmp = find(contents,
			rule->function() == KMSearchRule::FuncContains,
			exhaustive_search ? rule : NULL, FALSE,
			mTermTOC.h.header_lookup[hdrs[i]]);
		if(first) {
		    first = FALSE;
		    for(QValueListIterator<Q_UINT32> it = tmp.begin();
			it != tmp.end(); ++it)
			foundDict.insert((*it), (void*)1);
		} else {
		    for(QValueListIterator<Q_UINT32> it = tmp.begin();
			it != tmp.end(); ++it) {
			if(!foundDict[(*it)])
			    foundDict.insert((*it), (void*)1);
		    }
		}
	    }
	    QValueList<Q_UINT32> ret;
	    for(QIntDictIterator<void> it(foundDict); it.current(); ++it)
		ret << it.currentKey();
	    return ret;
	}
	return QValueList<Q_UINT32>(); //can't really happen..
    }
    //general header case..
    if(!mTermTOC.h.header_lookup.contains(rule->field()))
	return QValueList<Q_UINT32>();
    return find(rule->contents().lower(),
		rule->function() == KMSearchRule::FuncContains,
		exhaustive_search ? rule : NULL, FALSE,
		mTermTOC.h.header_lookup[rule->field()]);

}

// processes rule and performs the indexed look up, if exhaustive_search
// is true it will interpret body()[s] as full phrases rather than AND'd sets
QValueList<Q_UINT32>
KMMsgIndex::query(KMSearchPattern *pat, bool exhaustive_search)
{
    QValueList<Q_UINT32> ret;
    if(pat->isEmpty() || !canHandleQuery(pat))
	return ret;

    if(pat->count() == 1) {
	ret = query(pat->first(), exhaustive_search);
    } else {
	bool first = TRUE;
	QIntDict<void> foundDict;
	for(QPtrListIterator<KMSearchRule> it(*pat); it.current(); ++it) {
	    if((*it)->field().isEmpty() || (*it)->contents().isEmpty())
		continue;
	    QValueList<Q_UINT32> tmp = query((*it), exhaustive_search);
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

// Code to bind to a KMSearch
KMIndexSearchTarget::KMIndexSearchTarget(KMSearch *s) : QObject(NULL, NULL),
							mVerifyResult(FALSE)
{
    mSearch = s;
    mId = startTimer(0);
    {
	QValueList<Q_UINT32> lst = kmkernel->msgIndex()->query(
	    s->searchPattern(), FALSE);
	for(QValueListConstIterator<Q_UINT32> it = lst.begin();
	    it != lst.end(); ++it)
	    mSearchResult.push((*it));
    }
    for(QPtrListIterator<KMSearchRule> it(*s->searchPattern());
	it.current(); ++it) {
	if((*it)->function() != KMSearchRule::FuncContains ||
	   km_isSeparated((*it)->contents())) {
	    mVerifyResult = TRUE;
	    break;
	}
    }
    QObject::connect(this, SIGNAL(proxyFound(Q_UINT32)),
		     s, SIGNAL(found(Q_UINT32)));
    QObject::connect(this, SIGNAL(proxyFinished(bool)),
		     s, SIGNAL(finished(bool)));
}
KMIndexSearchTarget::~KMIndexSearchTarget()
{
    stop();
    QValueListConstIterator<QGuardedPtr<KMFolder> > it;
    for (it = mOpenedFolders.begin(); it != mOpenedFolders.end(); ++it) {
	KMFolder *folder = *it;
	if(folder)
	    folder->close();
    }
    mOpenedFolders.clear();
}
void
KMIndexSearchTarget::timerEvent(QTimerEvent *)
{
    if(qApp->hasPendingEvents())
	return; //no time now
    bool finished = FALSE;
    if(mSearch) {
	KMFolder *folder;
	const uint max_src = mVerifyResult ? 100 : 500;
	int stop_at = QMIN(mSearchResult.count(), max_src);
	for(int i = 0, idx; i < stop_at; i++) {
	    Q_UINT32 serNum = mSearchResult.pop();
	    kmkernel->msgDict()->getLocation(serNum, &folder, &idx);
	    if (!folder || (idx == -1))
		continue;
	    if(mSearch->inScope(folder)) {
		mSearch->setSearchedCount(mSearch->searchedCount()+1);
		mSearch->setCurrentFolder(folder->label());
		if(mVerifyResult) { //full phrase..
		    if(mOpenedFolders.findIndex(folder) == -1) {
			folder->open();
			mOpenedFolders.append(folder);
		    }
		    if(!mSearch->searchPattern()->matches(
			   folder->getDwString(idx)))
			continue;
		}
		mSearch->setFoundCount(mSearch->foundCount()+1);
		emit proxyFound(serNum);
	    }
	}
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
	kmkernel->msgIndex()->stopQuery(id());
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
