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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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
#include "folderjob.h"

#include <QList>
#include <QTime>
#include <QPointer>
#include <akonadi/collection.h>

class KMHeaders;

namespace KMail {

/* A class for asynchronous filtering of messages */
class ActionScheduler : public QObject
{
  Q_OBJECT

public:
  enum ReturnCode { ResultOk, ResultError, ResultCriticalError };

  ActionScheduler(KMFilterMgr::FilterSet set,
		  QList<KMFilter*> filters,
		  const Akonadi::Collection &srcFolder = Akonadi::Collection());
  ~ActionScheduler();

  /** The action scheduler will be deleted after the finish signal is emitted
      if this property is set to true */
  void setAutoDestruct( bool );

  /** Apply all filters even if they don't match */
  void setAlwaysMatch( bool );

  /**
   * Ignoring the filter set means the action scheduler will not care about
   * the filter set which was set in the constructor. Instead, it will apply
   * all fiters on the message.
   * This is useful when the user manually applies a filter.
   *
   * The default is false.
   */
  void setIgnoreFilterSet( bool ignore );

  /** Set a folder to monitor for new messages to filter */
  void setSourceFolder( const Akonadi::Collection& );

  /** Set a list of filters to work with
   The current list will not be updated until the queue
   of messages left to process is empty */
  void setFilterList( QList<KMFilter*> filters );

  /** Set the id of the account associated with this scheduler */
  void setAccountId( uint id  ) { mAccountId = id; mAccount = true; }

  /** Clear the id of the account associated with this scheduler */
  void clearAccountId() { mAccountId = 0; mAccount = false; }

  /** Queue a message for filtering */
  void execFilters( const Akonadi::Item & );

  static QString debug();
  static bool isEnabled();

  /** Ignores the given message on the next msgAdded()
      This is needed when using pipe-through filters, because the
      changes made by the filter have to be written back. */
  void addMsgToIgnored( const Akonadi::Item &item );

signals:
  /** Emitted when filtering is completed */
  void result(ReturnCode);
  void filtered( const Akonadi::Item &item );

public slots:
  /** Called back by asynchronous actions when they have completed */
  void actionMessage(KMFilterAction::ReturnCode = KMFilterAction::GoOn);

  /** Called back by asynchronous copy actions when they have completed */
  void copyMessageFinished( KMCommand *command );

private slots:
  void finish();

  //Fetching slots
  void fetchMessage();
  void messageFetched( const Akonadi::Item& item );
  void msgAdded(const Akonadi::Collection &, const Akonadi::Item &item );
  void enqueue(const Akonadi::Item &item);

  //Filtering slots
  void processMessage();
  void filterMessage();
  void moveMessage();
  void moveMessageFinished( KMCommand *command );
  void timeOut();

  void messageFetchResult( KJob* job );

private:
  static QList<ActionScheduler*> *schedulerList; // for debugging
#if 0
  static KMFolderMgr *tempFolderMgr;
#endif
  static int refCount, count;
  static bool sEnabled, sEnabledChecked;

  // Iterates over the messages in mItems, describes which message is being
  // filtered currently.
  // In processMessage(), this iterator is set to the next message which is available
  // for processing (thus processMessage() is called once for every message).
  Akonadi::Item::List::Iterator mMessageIt;

  // Iterates over all available filters. Used in filterMessage(), which is
  // called once for every filter.
  QList<KMFilter*>::iterator mFilterIt;

  // Iterates over all filter actions of the current filter. Used in
  // actionMessage(), which is called once for every filter action.
  QList<KMFilterAction*>::iterator mFilterActionIt;

  // List of serial numbers of message that are in the temporary filter folder
  // and await processing.
  // Serial numbers are added as soon as new messages are added to the temp folder,
  // and removed as soon as the message is moved back to the original
  // source folder (or the target folder if specified in a filter action).
  Akonadi::Item::List mItems;

  // List of serial numbers of messages that need to be fetched from the orginal
  // source folder.
  // Once they are fetched, the messages are copied into the temporary filter folder.
  // Serial numbers are added when execFilters() is called by the user, and
  // removed as soon as the message is fetched from the original source folder.
  Akonadi::Item::List mFetchItems;

  //List of serial numbers to be ignored in msgAdded()
  Akonadi::Item::List mIgnoredItems;

  //QList<QPointer<KMFolder> > mOpenFolders;
  QList<KMFilter*> mFilters, mQueuedFilters;
  KMFilterAction* mFilterAction;
  KMFilterMgr::FilterSet mSet;
  KMHeaders *mHeaders;
  Akonadi::Collection mSrcFolder, mDestFolder;
  bool mExecuting, mExecutingLock, mFetchExecuting;
  bool mFiltersAreQueued;
  bool mIgnoreFilterSet;
  bool mAutoDestruct;
  bool mAlwaysMatch;
  bool mAccount;
  uint mAccountId;
  Akonadi::Item mOriginalItem;
  bool mDeleteSrcFolder;
  ReturnCode mResult;
  QTimer *finishTimer, *fetchMessageTimer;
  QTimer *processMessageTimer, *filterMessageTimer;
  QTimer *timeOutTimer;
  QTime timeOutTime;
  KMCommand *lastCommand;
  QPointer<KJob> lastJob;
};

}

#endif /*actionscheduler_h*/
