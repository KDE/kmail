/*  -*- mode: C++ -*-
    Action Scheduler

    This file is part of KMail, the KDE mail client.
    Copyright (c) Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef actionscheduler_h
#define actionscheduler_h

#include "kmfilteraction.h" // for KMFilterAction::ReturnCode
#include "kmfilter.h"
#include "kmfiltermgr.h" // KMFilterMgr::FilterSet
#include "kmcommands.h"

#include <qobject.h>
#include <qguardedptr.h>
#include <qtimer.h>

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
  void moveMessageFinished( KMCommand *command );

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
