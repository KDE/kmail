/* Message indexing
 *
 * Author: Sam Magnuson <zachsman@wiw.org>
 * This code is under GPL
 *
 */
#ifndef kmmsgindex_h
#define kmmsgindex_h

#include "kmfolder.h"

#include <qmap.h>
#include <qobject.h>
#include <qstring.h>
#include <qthread.h>
#include <qintdict.h>
#include <qdatetime.h>
#include <qvaluelist.h>
#include <qvaluestack.h>
#include <qguardedptr.h>

class KMMsgIndexRef;
class KMIndexSearchTarget;
class KMSearchPattern;
class KMSearchRule;
class KMSearch;

class KMMsgIndex : public QObject
{
    Q_OBJECT
    enum { INDEX_IDLE, INDEX_RESTORE, INDEX_CREATE } mIndexState;
    QValueList<QGuardedPtr<KMFolder> > mOpenedFolders;

    //book-keeping
    struct {
	int fd;
	Q_INT32 count, used, indexed, removed;
	KMMsgIndexRef *ref;
	QString loc;
    } mTermIndex;
    struct {
	int fd;
	QString loc;
	QMap<QCString, int> body;
	struct {
	    int next_hnum;
	    QMap<QCString, Q_UINT16> header_lookup;
	    QMap<Q_UINT16, QMap<QCString, int> > headers;
	} h;
    } mTermTOC;
    struct {
	int fd;
	QString loc;
	QIntDict<void> known;
    } mTermProcessed;

    //state information
    struct {
	int timer_id;
	QValueStack<Q_UINT32> serNums;
	QValueStack<QGuardedPtr<KMFolder> > folders;
	//these are used for partial creates..
    } create;
    struct {
	int timer_id;
	bool reading_processed, restart_index;
    } restore;
    int delay_cnt;
    QTime mLastSearch; //when last index lookup was performed
    QIntDict<KMIndexSearchTarget> mActiveSearches; //for async search

public:
    KMMsgIndex(QObject *parent=NULL, const char *name=NULL);
    ~KMMsgIndex() { reset(); }
    
    void init();
    void remove();

    bool canHandleQuery(KMSearchRule *);
    bool canHandleQuery(KMSearchPattern *);
    QValueList<Q_UINT32> query(KMSearchRule *, bool full_phrase_search=TRUE);
    QValueList<Q_UINT32> query(KMSearchPattern *, bool full_phras_search=TRUE);

    //convenient..
    bool startQuery(KMSearch *);
    bool stopQuery(KMSearch *);
    bool stopQuery(int id) { return mActiveSearches.remove(id); }

    //force the cleanup..
    void cleanUp();

private:
    //internal
    bool recreateIndex();
    void readIndex();
    void syncIndex();

    void flush();
    void reset(bool =TRUE);

    bool createState(bool =TRUE);
    bool restoreState(bool =TRUE);

    int addBucket(int, Q_UINT32);
    QValueList<Q_UINT32> find(QString, bool, KMSearchRule *, bool =FALSE,
			      Q_UINT16 =0);
    void values(int, int, QValueList<Q_UINT32> &);
    void values(int, int, QIntDict<void> &);

    int processMsg(Q_UINT32);
    bool addHeaderTerm(Q_UINT16, const char *, uchar, Q_UINT32);
    bool addBodyTerm(const char *, uchar, Q_UINT32);
    int allocTermChunk(int);

    bool isTimeForClean();
    bool isKillTerm(const char *, uchar);
    bool isKillHeader(const char *, uchar);

protected:
    void timerEvent(QTimerEvent *);

private slots:
    void slotAddMsg(KMFolder *, Q_UINT32);
    void slotRemoveMsg(KMFolder *, Q_UINT32);
};

class KMIndexSearchTarget : public QObject
{
    Q_OBJECT
    int mId;
    bool mVerifyResult;
    QValueList<QGuardedPtr<KMFolder> > mOpenedFolders;
    QValueStack<Q_UINT32> mSearchResult;
    QGuardedPtr<KMSearch> mSearch;
public:
    KMIndexSearchTarget(KMSearch *);
    ~KMIndexSearchTarget();

    void stop(bool stop_search=TRUE) {
	if(stop_search && mSearch)
	    emit proxyFinished(true);
	mSearch = NULL;
    }
    KMSearch *search() const { return mSearch; }
    int id() const { return mId; }

protected:
    void timerEvent(QTimerEvent *);

signals:
    void proxyFound(Q_UINT32);
    void proxyFinished(bool);
};

#endif /* __KMMSGINDEX_H__ */










