#ifndef __KMHEADERS
#define __KMHEADERS

#include <qwidget.h>
#include <qstrlist.h>
#define private public
#include <qlistview.h>
#include <qarray.h>
#include <qmap.h>
#include <qdragobject.h>
#include <qdict.h>
#include "kmmessage.h"

class KMFolder;
class KMMessage;
class KMMsgBase;
class KMMainWin;
class QPalette;
class KMHeaderItem;
class QPixmap;
class QIconSet;

typedef QList<KMMsgBase> KMMessageList;
typedef QMap<int,KMFolder*> KMMenuToFolder;

// A special drag class for header list to folder tree DnD operations
class KMHeaderToFolderDrag: public QStoredDrag {
public:
    KMHeaderToFolderDrag( QWidget * parent = 0, const char * name = 0 );
    ~KMHeaderToFolderDrag() {};

    static bool canDecode( QDragMoveEvent* e );
};

// Information shared by all items in a list view
struct KMPaintInfo {
  bool pixmapOn;
  QPixmap pixmap; 
  QColor colFore;
  QColor colBack;
  QColor colNew;
  QColor colUnread;
  bool showSize;      // Do we display the message size?
  bool orderOfArrival;
  bool status;
  int flagCol;
  int senderCol;
  int subCol;
  int dateCol;
  int sizeCol;
};

#define KMHeadersInherited QListView
class KMHeaders : public QListView
{
  Q_OBJECT
    friend KMHeaderItem; // For easy access to the pixmaps

public:
  KMHeaders(KMMainWin *owner, QWidget *parent=0, const char *name=0);
  virtual ~KMHeaders();

  // A new folder has been selected update the list of headers shown
  virtual void setFolder(KMFolder *);

  // Return the folder whose message headers are being displayed
  KMFolder* folder(void) { return mFolder; }

  // read the config file and update nested state if necessary
  void refreshNestedState(void);

  /** Set current message. If id<0 then the first message is shown,
    if id>count() the last message is shown. */
  virtual void setCurrentMsg(int msgId);

  /** The following methods process the message in the folder with
    the given msgId, or if no msgId is given all selected
    messages are processed. */
  virtual void setMsgStatus(KMMsgStatus status, int msgId=-1);
  virtual void setMsgRead(int msgId=-1);
  virtual void deleteMsg(int msgId=-1);
  virtual void applyFiltersOnMsg(int msgId=-1);
  virtual void saveMsg(int msgId = -1);
  virtual void undo();
  virtual void forwardMsg();
  virtual void bounceMsg();
  virtual void replyToMsg();
  virtual void redirectMsg();
  virtual void replyAllToMsg();
  virtual void resendMsg();

  /** If destination==NULL the messages are deleted, otherwise
    they are moved to this folder. */
  virtual void moveMsgToFolder(KMFolder* destination, int msgId=-1);

  /** Messages are duplicated and added to given folder. */
  virtual void copyMsgToFolder(KMFolder* destination, int msgId=-1);

 /** Returns list of selected messages or a list with the message with
    the given Id if msgId >= 0. Do not delete the returned list. */
  virtual KMMessageList* selectedMsgs(int msgId=-1);

  /** Returns message with given id or current message if no
    id is given. First call with msgId==-1 returns first
    selected message, subsequent calls with no argument
    return the following selected messages. */
  KMMessage* getMsg (int msgId=-2);

  /** Returns index of message returned by last getMsg() call */
  int indexOfGetMsg (void) const { return getMsgIndex; }

  /** Returns pointer to owning main window. */
  KMMainWin* owner(void) const { return mOwner; }

  /** Read config options. */
  virtual void readConfig(void);

  /** Read color options and set palette. */
  virtual void readColorConfig(void);
  
  // Refresh the list of message headers shown
  virtual void reset(void);

  // Scroll to show new mail
  void showNewMail();

  // Return the current message
  virtual KMMessage* currentMsg();
  // Return the current list view item
  virtual KMHeaderItem* currentHeaderItem();
  // Return the index of the message corresponding to the current item
  virtual int currentItemIndex();
  // Set the current item to the one corresponding to the given msg id
  virtual void setCurrentItemByIndex( int msgIdx );
  // Return the message id of the top most visible item
  virtual int topItemIndex();
  // Make the item corresponding to the message with the given id the
  // top most visible item.
  virtual void setTopItemByIndex( int aMsgIdx );
  virtual void setNestedOverride( bool override );
  // Double force items to always be open
  virtual void setOpen ( QListViewItem *, bool );

signals:
  // emitted when the list view item corresponding to this message
  // has been selected
  virtual void selected(KMMessage *);
  // emitted when the list view item corresponding to this message
  // has been double clicked
  virtual void activated(KMMessage *);
  // emitted when we might be about to delete messages
  virtual void maybeDeleting();

public slots:
  void workAroundQListViewLimitation();

  // For when a list view item has been double clicked
  void selectMessage(QListViewItem*);
  // For nested message view, recusively add all children of a message
  void recursivelyAddChildren( int i, KMHeaderItem *parent );
  // For when a list view item has been selected 
  void highlightMessage(QListViewItem*);
  // For when righ mouse button is pressed
  void slotRMB();
  // Refresh list view item corresponding to the messae with the given id
  void msgHeaderChanged(int msgId);
  // For when the list of messages in a folder has changed
  void msgChanged();
  // For when the message with the given message id has been added to a folder
  void msgAdded(int);
  // For when the message with the given id has been removed for a folder
  void msgRemoved(int,QString);
  // Make the next header visible scrolling if necessary
  void nextMessage();
  // Make the previous header visible scrolling if necessary
  void prevMessage();
  // Make the nextUnread message header visible scrolling if necessary
  void nextUnreadMessage();
  // Make the previous message header visible scrolling if necessary
  void prevUnreadMessage();
 
protected:
  static QPixmap *pixNew, *pixUns, *pixDel, *pixOld, *pixRep, *pixSent, 
    *pixQueued, *pixFwd;

  // Look for color changes
  virtual bool event(QEvent *e);

  // Overrided to support backing pixmap
  virtual void paintEmptyArea( QPainter * p, const QRect & rect );

  // Ensure the current item is visible
  void makeHeaderVisible();

  /** Find next/prev unread message. Starts at currentItem() if startAt
    is unset. */
  virtual int findUnread(bool findNext, int startAt=-1, bool onlyNew = FALSE);

  /** Returns message index of first selected message of the messages
    where the message with the given id is in. This for finding the correct
    message that shall be the current message after move/delete of multiple
    messages. */
  virtual int firstSelectedMsg() const;

  /** Read per-folder config options and apply them. */
  virtual void readFolderConfig(void);

  /** Write per-folder config options. */
  virtual void writeFolderConfig(void);

  /* Handle shift and control selection */
  virtual void contentsMousePressEvent(QMouseEvent*);
  virtual void contentsMouseReleaseEvent(QMouseEvent* e);
  virtual void keyPressEvent( QKeyEvent * e );

  /* Unselect all items except one */
  virtual void clearSelectionExcept( QListViewItem *exception );

  /* Select all items in list from begin to end, return FALSE
     if end occurs before begin in the list */
  virtual bool shiftSelection( QListViewItem *begin, QListViewItem *end );  

  /** Called when a header is clicked */
  virtual void setSorting( int column, bool ascending = TRUE);

  // To initiate a drag operation
  void contentsMouseMoveEvent( QMouseEvent *e );

protected slots:
  // Move messages corresponding to the selected items to the folder
  // corresponding to the given menuId
  virtual void moveSelectedToFolder( int menuId );
  // Same thing but copy 
  virtual void copySelectedToFolder( int menuId );

private:
  // Is equivalent to clearing the list and inserting an item for
  // each message in the current folder
  virtual void updateMessageList(void);

  KMFolder* mFolder;            // Currently associated folder
  KMMainWin* mOwner;            // The KMMainWin for status bar updates

  int mTopItem;                 // Top most visible item
  int mCurrentItem;             // Current item
  QArray<KMHeaderItem*> mItems; // Map messages ids into KMHeaderItems
  QDict< KMHeaderItem > mIdTree;
  QDict< KMHeaderItem > mPhantomIdTree;
  QDict< QValueList< int > > mTree;
  QDict< bool > mTreeSeen;
  QDict< bool > mTreeToplevel;
  bool mNested, mNestedOverride;

  static bool mTrue, mFalse;    // These must replaced by something better!

  bool showingSize;             // are we currently showing the size field?
  int getMsgIndex;              // Updated as side effect of KMHeaders::getMsg
  bool getMsgMulti;             // ditto
  KMHeaderItem* getMsgItem;     // ditto
  KMMessageList mSelMsgList;    // KMHeaders::selectedMsgs isn't reentrant
  KMHeaderItem* mPrevCurrent;

  QListViewItem *beginSelection, *endSelection; // For shift selection

  KMPaintInfo mPaintInfo;       // Current colours and backing pixmap

  int mSortCol;
  bool mSortDescending;
  static QIconSet *up, *down;   // Icons shown in header
  KMMenuToFolder mMenuToFolder; // Map menu id into a folder

  bool mousePressed;             // Drag and drop support
  QPoint presspos;              // ditto
};

#endif
