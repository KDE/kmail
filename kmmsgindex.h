/* Message indexing
 *
 * Author: Sam Magnuson <sam@trolltech.com>
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
#include <qvaluelist.h>
#include <qvaluestack.h>
#include <qguardedptr.h>

class KMIndexSearchTarget;
class KMSearchPattern;
class KMSearchRule;
class KMSearch;

class KMMsgIndex : public QObject
{
    Q_OBJECT
    enum { INDEX_IDLE, INDEX_CLEANUP, INDEX_CREATE } mIndexState;
    QValueList<QGuardedPtr<KMFolder> > mOpenedFolders;

    struct {
	int fd;
	Q_INT32 count, used, known;
	Q_UINT32 *ref;
	QString loc;
    } mTermIndex;
    struct {
	int fd;
	QMap<QString, int> map;
	QString loc;
    } mTermTOC;
    struct {
	int timer_id;
	QValueStack<Q_UINT32> serNums;
	QValueStack<QGuardedPtr<KMFolder> > folders;
    } create, cleanup;
    QIntDict<KMIndexSearchTarget> mActiveSearches;

public:
    KMMsgIndex(QObject *parent=NULL, const char *name=NULL);
    ~KMMsgIndex();

    bool canHandleQuery(KMSearchPattern *);
    QValueList<Q_UINT32> query(KMSearchRule *);
    QValueList<Q_UINT32> query(KMSearchPattern *);

    //convenient..
    bool startQuery(KMSearch *);
    bool stopQuery(KMSearch *);
    bool stopQuery(int);

    //number of items indexed
    int indexed() const { return mTermIndex.known; }
private:
    //internal
    int allocTermChunk(int);
    bool isKillTerm(const char *, int);
    bool addTerm(const char *, int, Q_UINT32);
    bool processMsg(Q_UINT32);
    void reset(bool =TRUE);
    bool recreateIndex();
    void readIndex();

protected:
    void timerEvent(QTimerEvent *);

private slots:
    void addMsg(KMFolder *, Q_UINT32);
};

class KMIndexSearchTarget : public QObject
{
    Q_OBJECT
    int mId;
    QValueStack<Q_UINT32> mSearchResult;
    QGuardedPtr<KMSearch> mSearch;
public:
    KMIndexSearchTarget(KMSearch *);
    ~KMIndexSearchTarget() { stop(); }

    void stop(bool stop_search=TRUE) {
	if(stop_search && mSearch) 
	    emit proxyFinished(false);
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
