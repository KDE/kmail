#ifndef __KMHEADERS
#define __KMHEADERS

#include <qwidget.h>
#include <qstrlist.h>
#include <klistview.h>
#include <kfoldertree.h>
#include <qmemarray.h>
#include <qmap.h>
#include <qdragobject.h>
#include <qdict.h>
#include "kmmessage.h"
#include "kmime_util.h"
#include <kpopupmenu.h>

class KMFolder;
class KMMessage;
class KMMsgBase;
class KMMainWidget;
class QPalette;
class KMHeaderItem;
class QPixmap;
class QIconSet;
class QDateTime;
class KMSortCacheItem;

typedef QPtrList<KMMsgBase> KMMessageList;
typedef QValueList<Q_UINT32> SerNumList;
typedef QMap<int,KMFolder*> KMMenuToFolder;
enum NestingPolicy { AlwaysOpen = 0, DefaultOpen, DefaultClosed, OpenUnread };
enum LoopOnGotoUnreadValue { DontLoop = 0, LoopInCurrentFolder, LoopInAllFolders };

/** The widget that shows the contents of folders */
#define KMHeadersInherited KListView
class KMHeaders : public KListView
{
  Q_OBJECT
  friend class KMHeaderItem; // For easy access to the pixmaps

public:
  KMHeaders(KMMainWidget *owner, QWidget *parent=0, const char *name=0);
  virtual ~KMHeaders();

  /** A new folder has been selected update the list of headers shown */
  virtual void setFolder(KMFolder *, bool jumpToFirst = false);

  /** Return the folder whose message headers are being displayed */
  KMFolder* folder(void) { return mFolder; }

  /** read the config file and update nested state if necessary */
  void refreshNestedState(void);

  /** Set current message. If id<0 then the first message is shown,
    if id>count() the last message is shown. */
  virtual void setCurrentMsg(int msgId);

  /** Get a list of all items in the current thread */
  QPtrList<QListViewItem> currentThread() const;

  /** Set all messages in the current thread to status @p status
      or toggle it, if specified. */
  virtual void setThreadStatus(KMMsgStatus status, bool toggle=false);

  /* Set message status to read if it is new, or unread */
  virtual void setMsgRead(int msgId);

  /** The following methods processes all selected messages. */
  virtual void setMsgStatus(KMMsgStatus status, bool toggle=false);
  virtual void deleteMsg();
  virtual void applyFiltersOnMsg();
  virtual void undo();
  virtual bool canUndo() const;
  virtual void resendMsg();
  virtual KMHeaderItem * prepareMove( int *contentX, int *contentY );
  virtual void finalizeMove( KMHeaderItem *item, int contentX, int contentY );

  /** If destination is 0 then the messages are deleted, otherwise
    they are moved to this folder. */
  virtual void moveMsgToFolder(KMFolder* destination);

  /** Messages are duplicated and added to given folder.
      If aMsg is set this one will be written to the destination folder. */
  virtual void copyMsgToFolder(KMFolder* destination,
                               KMMessage* aMsg = 0);

  /** Returns list of selected messages. Mark the corresponding
      header items to be deleted, if specified. */
  virtual KMMessageList* selectedMsgs(bool toBeDeleted = false);

  /** Returns index of message returned by last getMsg() call */
  int indexOfGetMsg (void) const { return getMsgIndex; }

  /** Returns pointer to owning main window. */
  KMMainWidget* owner(void) const { return mOwner; }

  /** PaintInfo pointer */
  const KPaintInfo *paintInfo(void) const { return &mPaintInfo; }

  /** Read config options. */
  virtual void readConfig(void);

  /** Read color options and set palette. */
  virtual void readColorConfig(void);

  /** Refresh the list of message headers shown */
  virtual void reset(void);

  /** Scroll to show new mail */
  void showNewMail();

  /** Return the current message */
  virtual KMMessage* currentMsg();
  /** Return the current list view item */
  virtual KMHeaderItem* currentHeaderItem();
  /** Return the index of the message corresponding to the current item */
  virtual int currentItemIndex();
  /** Set the current item to the one corresponding to the given msg id */
  virtual void setCurrentItemByIndex( int msgIdx );
  /** Return the message id of the top most visible item */
  virtual int topItemIndex();
  /** Make the item corresponding to the message with the given id the
      top most visible item. */
  virtual void setTopItemByIndex( int aMsgIdx );
  virtual void setNestedOverride( bool override );
  virtual void setSubjectThreading( bool subjThreading );
  /** Double force items to always be open */
  virtual void setOpen ( QListViewItem *, bool );

  NestingPolicy getNestingPolicy() const { return nestingPolicy; }
  /** Returns true if the current header list is threaded. */
  bool isThreaded() const {
    return mNested != mNestedOverride; // xor
  }

  /** Find next/prev unread message. Starts at currentItem() if startAt
    is unset. */
  virtual int findUnread(bool findNext, int startAt=-1, bool onlyNew = false, bool acceptCurrent = false);

  /** Return the config option LoopOnGotoUnread */
  LoopOnGotoUnreadValue loopOnGotoUnread() { return mLoopOnGotoUnread; }

  void highlightMessage(QListViewItem*, bool markitread);
  
  /** return a string relativ to the current time */
  static QString fancyDate( time_t otime );

  QFont dateFont;

  bool noRepaint;

  // filter events for popup
  bool eventFilter ( QObject *o, QEvent *e );

signals:
  /** emitted when the list view item corresponding to this message
      has been selected */
  virtual void selected(KMMessage *);
  /** emitted when the list view item corresponding to this message
      has been double clicked */
  virtual void activated(KMMessage *);
  /** emitted when we might be about to delete messages */
  virtual void maybeDeleting();

public slots:
  void workAroundQListViewLimitation();

  /** For when a list view item has been double clicked */
  void selectMessage(QListViewItem*);
  /** For when a list view item has been selected */
  void highlightMessage(QListViewItem*);
  /** For when righ mouse button is pressed */
  void slotRMB();
  /** Refresh list view item corresponding to the messae with the given id */
  void msgHeaderChanged(KMFolder *folder, int msgId);
  /** For when the list of messages in a folder has changed */
  void msgChanged();
  /** For when the folder has been cleared */
  void folderCleared();
  /** For when the message with the given message id has been added to a folder */
  void msgAdded(int);
  /** For when the message with the given id has been removed for a folder */
  void msgRemoved(int, QString, QString);
  /** Make the next header visible scrolling if necessary */
  void nextMessage();
  /** Same as nextMessage() but don't clear the current selection */
  void selectNextMessage();
  /** Make the previous header visible scrolling if necessary */
  void prevMessage();
  /** Same as prevMessage() but don't clear the current selection */
  void selectPrevMessage();
  /** Make the nextUnread message header visible scrolling if necessary, returning 
    true if an unread message is found */
  bool nextUnreadMessage(bool acceptCurrent = false);
  /** Make the previous message header visible scrolling if necessary, returning
    true if an unread message is found */
  bool prevUnreadMessage();
  /** Don't show a drag cursor */
  void slotNoDrag();
  /** timer function to set the current time regularly */
  void resetCurrentTime();

  /** Expands (@p expand == true) or collapses (@p expand == false)
      the current thread. */
  void slotExpandOrCollapseThread( bool expand );
  /** Expands (@p expand == true) or collapses (@p expand == false)
      all threads */
  void slotExpandOrCollapseAllThreads( bool expand );

  virtual void ensureCurrentItemVisible();

  /** Select an item and if it is the parent of a closed thread, also
    recursively select its children. */   
  virtual void setSelected(QListViewItem *item, bool selected);

  /** switch size-column */
  void slotToggleSizeColumn();

  /** Provide information about number of messages in a folder */
  void setFolderInfoStatus();

protected:
  static QPixmap *pixNew, *pixUns, *pixDel, *pixRead, *pixRep, *pixSent,
    *pixQueued, *pixFwd, *pixFlag, *pixWatched, *pixIgnored,
    *pixFullySigned, *pixPartiallySigned, *pixUndefinedSigned,
    *pixFullyEncrypted, *pixPartiallyEncrypted, *pixUndefinedEncrypted,
      *pixFiller, *pixEncryptionProblematic,
      *pixSignatureProblematic;

  /** Look for color changes */
  virtual bool event(QEvent *e);

  /** Overridden to support backing pixmap */
  virtual void paintEmptyArea( QPainter * p, const QRect & rect );

  /** Ensure the current item is visible */
  void makeHeaderVisible();

  /** Auxillary method to findUnread */
  void findUnreadAux( KMHeaderItem*&, bool &, bool, bool );

  /** Returns message index of first selected message of the messages
    where the message with the given id is in. This for finding the correct
    message that shall be the current message after move/delete of multiple
    messages. */
  virtual int firstSelectedMsg() const;

  /** Read per-folder config options and apply them. */
  virtual void readFolderConfig(void);

  /** Write per-folder config options. */
  virtual void writeFolderConfig(void);

  /** Write global config options. */
  virtual void writeConfig(void);

  /** Handle shift and control selection */
  virtual void contentsMousePressEvent(QMouseEvent*);
  virtual void contentsMouseReleaseEvent(QMouseEvent* e);
  virtual void keyPressEvent( QKeyEvent * e );

  /** Unselect all items except one */
  virtual void clearSelectionExcept( QListViewItem *exception );

  /** Select all items in list from begin to end, return FALSE
     if end occurs before begin in the list */
  virtual bool shiftSelection( QListViewItem *begin, QListViewItem *end );

  /** Called when a header is clicked */
  virtual void setSorting( int column, bool ascending = TRUE);

  /** To initiate a drag operation */
  void contentsMouseMoveEvent( QMouseEvent *e );

  /** reimplemented in order to update the frame width in case of a changed
      GUI style */
  void styleChange( QStyle& oldStyle );

  /** Set the width of the frame to a reasonable value for the current GUI
      style */
  void setStyleDependantFrameWidth();

protected slots:
  /** Move messages corresponding to the selected items to the folder
      corresponding to the given menuId */
  virtual void moveSelectedToFolder( int menuId );
  /** Same thing but copy */
  virtual void copySelectedToFolder( int menuId );
  /** Apply the filter Rules to a single message */
  virtual int slotFilterMsg( KMMessage * );
  /** dirties the sort order */
  void dirtySortOrder(int);
  /** show context menu */
  void rightButtonPressed( QListViewItem *, const QPoint &, int );

private slots:
  void slotMoveCompleted( bool success );
  void slotMoveAborted( );

private:
  /** Is equivalent to clearing the list and inserting an item for
      each message in the current folder */
  virtual void updateMessageList(bool set_selection=FALSE);

  /** Currently associated folder */
  KMFolder* mFolder;
  /** The KMMainWin for status bar updates */
  KMMainWidget* mOwner;
  /** Top most visible item */
  int mTopItem;
  /** Current item */
  int mCurrentItem;
  /** Map messages ids into KMHeaderItems */
  QMemArray<KMHeaderItem*> mItems;
  QDict< KMSortCacheItem > mSortCacheItems;
  QDict< QPtrList< KMSortCacheItem > > mSubjectLists;	
  void buildThreadingTree( QMemArray<KMSortCacheItem *> sortCache );
  void buildSubjectThreadingTree( QMemArray<KMSortCacheItem *> sortCache );
  /** Find a msg to thread mb below */
  KMSortCacheItem* findParent(KMSortCacheItem *item);
  KMSortCacheItem* findParentBySubject(KMSortCacheItem *item);

  bool mNested, mNestedOverride, mSubjThreading;
  NestingPolicy nestingPolicy;
  QPtrList<KMHeaderItem> mImperfectlyThreadedList;

  /** These must replaced by something better! */
  static bool mTrue, mFalse;

  /** are we currently showing the size field? */
  bool showingSize;
  /** Updated as side effect of KMHeaders::getMsg */
  int getMsgIndex;
  /** ditto */
  bool getMsgMulti;
  /** ditto */
  KMHeaderItem* getMsgItem;
  /** @ref KMHeaders::selectedMsgs isn't reentrant */
  KMMessageList mSelMsgBaseList;
  QPtrList<KMMessage> mSelMsgList;
  KMHeaderItem* mPrevCurrent;

  /** For shift selection */
  QListViewItem *beginSelection, *endSelection;
  /** Current colours and backing pixmap */
  KPaintInfo mPaintInfo;

  int mSortCol;
  bool mSortDescending;

  /** Icons shown in header */
  static QIconSet *up, *down;
  /** Map menu id into a folder */
  KMMenuToFolder mMenuToFolder;

  /** Drag and drop support */
  bool mousePressed;
  /** ditto */
  QPoint presspos;

  struct {
      uint ascending : 1;
      uint dirty : 1;
      short column;
      uint fakeSort : 1;
      uint removed : 1;
  } mSortInfo;
  void appendItemToSortFile(KMHeaderItem *);
  bool writeSortOrder();
  bool readSortOrder(bool set_selection=FALSE);

  KMime::DateFormatter mDate;
  /** value of config key Behaviour/LoopOnGotoUnread */
  LoopOnGotoUnreadValue mLoopOnGotoUnread;
  bool mJumpToUnread;
  bool mReaderWindowActive;

  /** popup to switch columns */
  KPopupMenu* mPopup;
  int mSizeColumn;
};

#endif
