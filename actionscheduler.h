/* Asynchronous filtering
 *
 * Author: Don Sanders <sanders@kde.org>
 * License: GPL
 */

#ifndef actionscheduler_h
#define actionscheduler_h

#include "qobject.h"
#include "qguardedptr.h"
#include "qtimer.h"
#include "kmfilteraction.h" // for KMFilterAction::ReturnCode
#include "kmfilter.h"
#include "kmfiltermgr.h" // KMFilterMgr::FilterSet
class KMHeaders;

/* A class for asynchronous filtering of messages */

namespace KMail {

class ActionScheduler : public QObject
{
  Q_OBJECT

public:
  enum ReturnCode { ResultOk, ResultError, ResultCriticalError };

  ActionScheduler(KMFilterMgr::FilterSet set,
		  QPtrList<KMFilter> filters,
                  KMHeaders *headers = 0,
		  KMFolder *srcFolder = 0);
  ~ActionScheduler();

  /** The action scheduler will be deleted after the finish signal is emitted
      if this property is set to true */
  void setAutoDestruct( bool );

  /** Apply all filters even if they don't match */
  void setAlwaysMatch( bool );

  /** Set a default folder to move messages into */
  void setDefaultDestinationFolder( KMFolder* );

  /** Set a folder to monitor for new messages to filter */
  void setSourceFolder( KMFolder* );

  /** Set a list of filters to work with
   The current list will not be updated until the queue
   of messages left to process is empty */
  void setFilterList( QPtrList<KMFilter> filters );

  /** Queue a message for filtering */
  void execFilters(const QValueList<Q_UINT32> serNums);
  void execFilters(const QPtrList<KMMsgBase> msgList);
  void execFilters(KMMsgBase* msgBase);
  void execFilters(Q_UINT32 serNum);

signals:
  /** Emitted when filtering is completed */
  void result(ReturnCode);

public slots:
  /** Called back by asynchronous actions when they have completed */
  void actionMessage(KMFilterAction::ReturnCode = KMFilterAction::GoOn);

private slots:
  KMMsgBase* messageBase(Q_UINT32 serNum);
  KMMessage* message(Q_UINT32 serNum);
  void finish();

  int tempOpenFolder(KMFolder* aFolder);
  void tempCloseFolders();

  //Fetching slots
  void fetchMessage();
  void messageFetched( KMMessage *msg );
  void msgAdded( KMFolder*, Q_UINT32 );
  void enqueue(Q_UINT32 serNum);

  //Filtering slots
  void processMessage();
  void messageRetrieved(KMMessage*);
  void filterMessage();
  void moveMessage();
  void moveMessageFinished(bool);

private:
  static KMFolderMgr *tempFolderMgr;
  static int refCount, count;
  QValueListIterator<Q_UINT32> mMessageIt;
  QValueListIterator<KMFilter> mFilterIt;
  QValueList<Q_UINT32> mSerNums, mFetchSerNums;
  QValueList<QGuardedPtr<KMFolder> > mOpenFolders;
  QValueList<KMFilter> mFilters, mQueuedFilters;
  KMFilterAction* mFilterAction;
  KMFilterMgr::FilterSet mSet;
  KMHeaders *mHeaders;
  QGuardedPtr<KMFolder> mSrcFolder, mDestFolder;
  bool mExecuting, mExecutingLock, mFetchExecuting;
  bool mUnget, mFetchUnget;
  bool mIgnore;
  bool mFiltersAreQueued;
  bool mAutoDestruct;
  bool mAlwaysMatch;
  Q_UINT32 mOriginalSerNum;
  bool mDeleteSrcFolder;
  ReturnCode mResult;
  QTimer *finishTimer, *fetchMessageTimer, *tempCloseFoldersTimer;
  QTimer *processMessageTimer, *filterMessageTimer;
};

}

#endif /*actionscheduler_h*/
