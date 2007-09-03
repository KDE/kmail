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
// Dynamic search folder

#ifndef kmfoldersearch_h
#define kmfoldersearch_h

#include <qguardedptr.h>
#include <qvaluelist.h>
#include <qvaluevector.h>
#include <qvaluestack.h>
#include <qmap.h>
#include "kmfolder.h"
#include "folderstorage.h"

/** A search folder is a folder that shows the result of evaluating a
    search expression. This folder is dynamically updated as the
    search expression is applied to new mail (from a pop or imap server
    or from a local account).

    The index for a search folder consists of a cache of serial
    numbers of all messages that currently match the search.
**/

typedef QValueList<Q_UINT32> SerNumList;
class KMSearchPattern;
class KMFolderImap;
class KMFolderSearchJob;
class KMIndexSearchTarget;
class QTimer;

namespace KMail {
   class AttachmentStrategy;
}
using KMail::AttachmentStrategy;

class KMSearch: public QObject
{
  Q_OBJECT

public:
  KMSearch(QObject * parent = 0, const char * name = 0);
  ~KMSearch();

  bool write(QString location) const;
  bool read(QString location);
  bool recursive() const { return mRecursive; }
  void setRecursive(bool recursive) { if (running()) stop(); mRecursive = recursive; }
  KMFolder* root() const { return mRoot; }
  void setRoot(KMFolder *folder) { if (running()) stop(); mRoot = folder; }
  bool inScope(KMFolder* folder) const;
  //Takes ownership of @searchPattern
  void setSearchPattern(KMSearchPattern *searchPattern);
  KMSearchPattern* searchPattern() const { return mSearchPattern; }
  void start();
  bool running() const { return mRunning; }
  void stop();
  int foundCount() const { return mFoundCount; }
  int searchCount() const { return mSearchCount; }
  QString currentFolder() const { return mLastFolder; }

public slots:
  void indexFinished();

signals:
  void found(Q_UINT32 serNum);
  void finished(bool success);

protected slots:
  void slotProcessNextBatch();
  void slotSearchFolderResult( KMFolder*, QValueList<Q_UINT32>,
                               const KMSearchPattern*, bool );

protected:
  friend class ::KMIndexSearchTarget;
  void setRunning(bool b) { mRunning = b; }
  void setFoundCount(int f) { mFoundCount = f; }
  void setCurrentFolder(const QString &f) { mLastFolder = f; }

private:
  int mRemainingFolders;
  bool mRecursive, mRunning, mIdle, mRunByIndex;
  QGuardedPtr<KMFolder> mRoot;
  KMSearchPattern* mSearchPattern;
  QValueList<QGuardedPtr<KMFolder> > mFolders, mOpenedFolders;
  QValueList<QGuardedPtr<KMFolderImap> > mIncompleteFolders;
  SerNumList mSerNums;
  QString mLastFolder;
  int mFoundCount;
  int mSearchCount;
  QTimer *mProcessNextBatchTimer;
};

class KMFolderSearch: public FolderStorage
{
  Q_OBJECT
  friend class ::KMFolderSearchJob;
public:
  KMFolderSearch(KMFolder* folder, const char* name=0);
  virtual ~KMFolderSearch();

  /** Returns the type of this folder */
  virtual KMFolderType folderType() const { return KMFolderTypeSearch; }

  // Sets and runs the search used by the folder
  void setSearch(KMSearch *search);
  // Returns the current search used by the folder
  const KMSearch* search() const;
  // Stops the current search
  void stopSearch() { if (mSearch) mSearch->stop(); }

  virtual KMMessage* getMsg(int idx);
  virtual void ignoreJobsForMessage( KMMessage* );

  virtual void tryReleasingFolder(KMFolder* folder);

  /** Returns true if this folder can be moved */
  virtual bool isMoveable() const { return false; }

protected slots:
  // Reads search definition for this folder and creates a KMSearch
  bool readSearch();
  // Runs the current search again
  void executeSearch();
  // Called when the search is finished
  void searchFinished(bool success);
  // Look at a new message and if it matches search() add it to the cache
  void examineAddedMessage(KMFolder *folder, Q_UINT32 serNum);
  // Look at a removed message and remove it from the cache
  void examineRemovedMessage(KMFolder *folder, Q_UINT32 serNum);
  // Look at a message whose status has changed
  void examineChangedMessage(KMFolder *folder, Q_UINT32 serNum, int delta);
  // The serial numbers for a folder have been invalidated, deal with it
  void examineInvalidatedFolder(KMFolder *folder);
  // A folder has been deleted, deal with it
  void examineRemovedFolder(KMFolder *folder);
  // Propagate the msgHeaderChanged signal
  void propagateHeaderChanged(KMFolder *folder, int idx);

public slots:
  // Appends the serial number to the cached list of messages that match
  // the search for this folder
  void addSerNum(Q_UINT32 serNum);
  // Removes the serial number from the cached list of messages that match
  // the search for this folder
  void removeSerNum(Q_UINT32 serNum);

  /** Incrementally update the index if possible else call writeIndex */
  virtual int updateIndex();

  // Examine the message
  void slotSearchExamineMsgDone( KMFolder*, Q_UINT32 serNum,
                                 const KMSearchPattern*, bool );

public:
  //See base class for documentation
  virtual int addMsg(KMMessage* msg, int* index_return = 0);
  virtual int open(const char *owner);
  virtual int canAccess();
  virtual void sync();
  virtual void reallyDoClose(const char* owner);
  virtual int create();
  virtual int compact( bool );
  virtual bool isReadOnly() const;
  virtual const KMMsgBase* getMsgBase(int idx) const;
  virtual KMMsgBase* getMsgBase(int idx);
  virtual int find(const KMMsgBase* msg) const;
  virtual QString indexLocation() const;
  virtual int writeIndex( bool createEmptyIndex = false );
  DwString getDwString(int idx);
  Q_UINT32 serNum(int idx) { return mSerNums[idx]; }

protected:
  virtual FolderJob* doCreateJob(KMMessage *msg, FolderJob::JobType jt,
                                 KMFolder *folder, QString partSpecifier,
                                 const AttachmentStrategy *as ) const;
  virtual FolderJob* doCreateJob(QPtrList<KMMessage>& msgList, const QString& sets,
                                 FolderJob::JobType jt, KMFolder *folder) const;
  virtual KMMessage* readMsg(int idx);
  virtual bool readIndex();
  virtual int removeContents();
  virtual int expungeContents();
  virtual int count(bool cache = false) const;
  virtual KMMsgBase* takeIndexEntry(int idx);
  virtual KMMsgInfo* setIndexEntry(int idx, KMMessage *msg);
  virtual void clearIndex(bool autoDelete=true, bool syncDict = false);
  virtual void truncateIndex();

private:
  QValueVector<Q_UINT32> mSerNums;
  QValueList<QGuardedPtr<KMFolder> > mFolders;
  QValueStack<Q_UINT32> mUnexaminedMessages;
  FILE *mIdsStream;
  KMSearch *mSearch;
  bool mInvalid, mUnlinked;
  bool mTempOpened;
  QTimer *mExecuteSearchTimer;
  QMap<const KMFolder*, unsigned int>mFoldersCurrentlyBeingSearched;
};
#endif /*kmfoldersearch_h*/

