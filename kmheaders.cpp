// kmheaders.cpp


#include <qstrlist.h>
#include "kmcomposewin.h"
#include <kfiledialog.h>
#include <qpalette.h>
#include <qcolor.h>
#include <qdatetime.h>
#include <qheader.h>
#include <qdragobject.h>
#include <kimgio.h>

#include <kapp.h>
#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>

#include "kmfolder.h"
#include "kmheaders.h"
#include "kmmessage.h"
#include "kbusyptr.h"
#include "kmdragdata.h"
#include "kmglobal.h"
#include "kmmainwin.h"
#include "kfileio.h"
#include "kmfiltermgr.h"
#include "kfontutils.h"
#include "kmfoldermgr.h"

QPixmap* KMHeaders::pixNew = 0;
QPixmap* KMHeaders::pixUns = 0;
QPixmap* KMHeaders::pixDel = 0;
QPixmap* KMHeaders::pixOld = 0;
QPixmap* KMHeaders::pixRep = 0;
QPixmap* KMHeaders::pixQueued = 0;
QPixmap* KMHeaders::pixSent = 0;
QPixmap* KMHeaders::pixFwd = 0;
QIconSet* KMHeaders::up = 0;
QIconSet* KMHeaders::down = 0;

//-----------------------------------------------------------------------------
// KMHeaderToFolderDrag method definitions
KMHeaderToFolderDrag::KMHeaderToFolderDrag( QWidget * parent, const char * name )
    : QStoredDrag( "KMHeaderToFolderDrag/magic", parent, name )
{
}

bool KMHeaderToFolderDrag::canDecode( QDragMoveEvent* e )
{
    return e->provides( "KMHeaderToFolderDrag/magic" );
}

//-----------------------------------------------------------------------------
// KMHeaderItem method definitions

class KMHeaderItem : public QListViewItem
{

public:
  KMFolder *mFolder;
  int mMsgId;
  QColor *mColor;
  QString mSortDate, mSortSubject, mSortSender, mSortArrival;
  KMPaintInfo *mPaintInfo;
  
  // Constuction a new list view item with the given colors and pixmap
  KMHeaderItem( QListView* parent, KMFolder* folder, int msgId, 
		KMPaintInfo *aPaintInfo )
    : QListViewItem( parent ), 
      mFolder( folder ),
      mMsgId( msgId ),
      mPaintInfo( aPaintInfo )
  {
    irefresh();
  }

  // Update the msgId this item corresponds to.
  void setMsgId( int aMsgId )
  {
    mMsgId = aMsgId;
  }
  
  // Profiling note: About 30% of the time taken to initialize the
  // listview is spent in this function. About 60% is spent in operator
  // new and QListViewItem::QListViewItem.
  void irefresh()
  {
    QString result;
    KMMsgStatus flag;
    QString fromStr, subjStr;
    KMMsgBase *mMsgBase = mFolder->getMsgBase( mMsgId );
    assert(mMsgBase!=NULL);

    flag = mMsgBase->status();
    setText( 0, " " + QString( QChar( (char)flag )));

    if (flag == KMMsgStatusQueued || flag == KMMsgStatusSent)
      fromStr = KMMessage::stripEmailAddr(mMsgBase->to());
    else
      fromStr = KMMessage::stripEmailAddr(mMsgBase->from());
    if (fromStr.isEmpty()) fromStr = i18n("Unknown");
    if (fromStr.isEmpty()) {
      debug( QString("Null message %1").arg( mMsgId ) );
    }
    if (fromStr == i18n("Unknown")) {
      debug( QString("Null messagex %1").arg( mMsgId ) );
    }
    setText( 1, fromStr.simplifyWhiteSpace() );

    subjStr = mMsgBase->subject();

    if (subjStr.isEmpty()) subjStr = i18n("No Subject");
    setText( 2, subjStr.simplifyWhiteSpace() );

    time_t mDate = mMsgBase->date();
    setText( 3, QString( ctime( &mDate )).simplifyWhiteSpace() );

    mColor = &mPaintInfo->colFore;
    switch (flag)
    {
    case KMMsgStatusNew:
      setPixmap( 0, *KMHeaders::pixNew );
      mColor = &mPaintInfo->colNew;
      break;
    case KMMsgStatusUnread:
      setPixmap( 0, *KMHeaders::pixUns );
      mColor = &mPaintInfo->colUnread;
      break;
    case KMMsgStatusDeleted:
      setPixmap( 0, *KMHeaders::pixDel );
      break;
    case KMMsgStatusReplied:
      setPixmap( 0, *KMHeaders::pixRep );
      break;
    case KMMsgStatusForwarded:
      setPixmap( 0, *KMHeaders::pixFwd );
      break;
    case KMMsgStatusQueued:
      setPixmap( 0, *KMHeaders::pixQueued );
      break;
    case KMMsgStatusSent:
      setPixmap( 0, *KMHeaders::pixSent );
      break;
    default:
      setPixmap( 0, *KMHeaders::pixOld );
      break;
    };

    const int dateLength = 100;
    char cDate[dateLength + 1];
    strftime( cDate, dateLength, "%Y:%j:%T", gmtime( &mDate ));
    mSortDate = cDate;

    mSortArrival = QString( "%1" ).arg( mMsgId, 8, 36 );
    mSortSender = text(1).lower() + " " + mSortDate;
    mSortSubject = KMMsgBase::skipKeyword( text(2).lower() ) + " " + mSortDate;
  }

  // Retrun the msgId of the message associated with this item
  int msgId()
  {
    return mMsgId;
  }

  // Updte this item to summarise a new folder and message
  void reset( KMFolder *aFolder, int aMsgId )
  {
    mFolder = aFolder;
    mMsgId = aMsgId;    
    irefresh();
  }

  // Change color (new/unread/read status has changed)
  void setColor( QColor *c )
  {
    mColor = c;
    repaint();
  }

// Begin this code may be relicensed by Troll Tech  
  void paintCell( QPainter * p, const QColorGroup & cg,
				int column, int width, int align )
  {
    // Change width() if you change this.

    if ( !p )
        return;

    QListView *lv = listView();
    int r = lv ? lv->itemMargin() : 1;
    const QPixmap * icon = pixmap( column );
    int marg = lv ? lv->itemMargin() : 1;

    if (!mPaintInfo->pixmapOn)
      p->fillRect( 0, 0, width, height(), cg.base() );
    else {
      QRect rect = lv->itemRect( this );
      int cw = 0;
      cw = lv->header()->cellPos( column );
      
      p->drawTiledPixmap( 0, 0, width, height(), 
			  mPaintInfo->pixmap,
			  rect.left() + cw + lv->contentsX(), 
			  rect.top() + lv->contentsY() );
    }
    
    if ( isSelected() &&
         (column==0 || listView()->allColumnsShowFocus()) ) {
      p->fillRect( r - marg, 0, width - r + marg, height(),
		   cg.brush( QColorGroup::Highlight ) );
      p->setPen( cg.highlightedText() );
    } else {
      p->setPen( *mColor );
    }
    
    if ( icon ) {
        p->drawPixmap( r, (height()-icon->height())/2, *icon );
        r += icon->width() + listView()->itemMargin();
    }

    QString t = text( column );
    if ( !t.isEmpty() ) {
        p->drawText( r, 0, width-marg-r, height(),
                     align | AlignVCenter, t );
    }
  }
  // End this code may be relicensed by Troll Tech  

  virtual QString key( int column, bool /*ascending*/ ) const {
    if (column == 3) {
      if (mPaintInfo->orderOfArrival)
	return mSortArrival;
      else
	return mSortDate;
    }
    else if (column == 2)
      return mSortSubject;
    else if (column == 1)
      return mSortSender;
    else
      return text(column);
  }
};

#include "qcstring.h"

//-----------------------------------------------------------------------------
KMHeaders::KMHeaders(KMMainWin *aOwner, QWidget *parent,
		     const char *name) :
  KMHeadersInherited(parent, name)
{
  KIconLoader* loader = KGlobal::iconLoader();
  static bool pixmapsLoaded = FALSE;
  //qInitImageIO();
  kimgioRegister();

  mOwner  = aOwner;
  mFolder = NULL;
  getMsgIndex = -1;
  mSortCol = KMMsgList::sfDate;
  mSortDescending = FALSE;
  mTopItem = 0;
  setMultiSelection( TRUE );
  setAllColumnsShowFocus( TRUE );

  readConfig();
  addColumn( i18n("F"), 18 );
  addColumn( i18n("Sender"), 200 );
  addColumn( i18n("Subject"), 270 );
  addColumn( i18n("Date"), 300 );

  if (!pixmapsLoaded)
  {
    pixmapsLoaded = TRUE;
    pixNew   = new QPixmap( loader->loadIcon("kmmsgnew") );
    pixUns   = new QPixmap( loader->loadIcon("kmmsgunseen") );
    pixDel   = new QPixmap( loader->loadIcon("kmmsgdel") );
    pixOld   = new QPixmap( loader->loadIcon("kmmsgold") );
    pixRep   = new QPixmap( loader->loadIcon("kmmsgreplied") );
    pixQueued= new QPixmap( loader->loadIcon("kmmsgqueued") );
    pixSent  = new QPixmap( loader->loadIcon("kmmsgsent") );
    pixFwd   = new QPixmap( loader->loadIcon("kmmsgforwarded") );
    up = new QIconSet( BarIcon("abup" ), QIconSet::Small );
    down = new QIconSet( BarIcon("abdown" ), QIconSet::Small );
  }

  connect(this, SIGNAL(doubleClicked(QListViewItem*)),
  	  this,SLOT(selectMessage(QListViewItem*)));
  connect(this,SIGNAL(currentChanged(QListViewItem*)),
	  this,SLOT(highlightMessage(QListViewItem*)));
  
  beginSelection = 0;
  endSelection = 0;
}


//-----------------------------------------------------------------------------
KMHeaders::~KMHeaders ()
{
  if (mFolder)
  {
    writeFolderConfig();
    mFolder->close();
  }
}


//-----------------------------------------------------------------------------
// Support for backing pixmap
void KMHeaders::paintEmptyArea( QPainter * p, const QRect & rect )
{
  if (mPaintInfo.pixmapOn)
    p->drawTiledPixmap( rect.left(), rect.top(), rect.width(), rect.height(), 
			mPaintInfo.pixmap, 
			rect.left() + contentsX(), 
			rect.top() + contentsY() );
  else 
    p->fillRect( rect, colorGroup().base() );
}


//-----------------------------------------------------------------------------
void KMHeaders::readConfig (void)
{
  KConfig* config = app->config();
  QString fntStr;

  // Backing pixmap support
  config->setGroup("Pixmaps");
  QString pixmapFile = config->readEntry("Headers","");
  mPaintInfo.pixmapOn = FALSE;
  if (pixmapFile != "") {
    mPaintInfo.pixmapOn = TRUE;
    mPaintInfo.pixmap = QPixmap( pixmapFile );
  }

  // Custom/System colors
  config->setGroup("Reader");
  QColor c1=QColor(app->palette().normal().text());
  QColor c2=QColor("blue");
  QColor c3=QColor("red");
  QColor c4=QColor(app->palette().normal().base());

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    mPaintInfo.colFore = config->readColorEntry("ForegroundColor",&c1);
    mPaintInfo.colBack = config->readColorEntry("BackgroundColor",&c4);
    QPalette newPal = palette();
    newPal.setColor( QColorGroup::Base, mPaintInfo.colBack );
    setPalette( newPal );
    mPaintInfo.colNew = config->readColorEntry("LinkColor",&c3);
    mPaintInfo.colUnread = config->readColorEntry("FollowedColor",&c2);
  }
  else {
    mPaintInfo.colFore = c1;
    mPaintInfo.colBack = c4;
    QPalette newPal = palette();
    newPal.setColor( QColorGroup::Base, c4 );
    setPalette( newPal );
    mPaintInfo.colNew = c3;
    mPaintInfo.colUnread = c2;
  }

  // Custom/System fonts
  config->setGroup("Fonts");
  if (!(config->readBoolEntry("defaultFonts",TRUE))) {
    fntStr = config->readEntry("list-font", "helvetica-medium-r-12");
    setFont(kstrToFont(fntStr));
  }
  else
    setFont(KGlobal::generalFont());
}


//-----------------------------------------------------------------------------
void KMHeaders::reset(void)
{
  int top = topItemIndex();
  int id = currentItemIndex();
  clear();
  mItems.resize(0);
  updateMessageList();
  setCurrentMsg(id);
  setTopItemByIndex(top);
}


//-----------------------------------------------------------------------------
void KMHeaders::readFolderConfig (void)
{
  KConfig* config = app->config();
  assert(mFolder!=NULL);
  int pathLen = mFolder->path().length() - folderMgr->basePath().length();
  QString path = mFolder->path().right( pathLen );

  if (!path.isEmpty())
    path = path.right( path.length() - 1 ) + "/";
  config->setGroup("Folder-" + path + mFolder->name());
  setColumnWidth(1, config->readNumEntry("SenderWidth", 200));
  setColumnWidth(2, config->readNumEntry("SubjectWidth", 270));
  setColumnWidth(3, config->readNumEntry("DateWidth", 300));

  mSortCol = config->readNumEntry("SortColumn", (int)KMMsgList::sfDate);
  mSortDescending = (mSortCol < 0);
  mSortCol = abs(mSortCol);

  mTopItem = config->readNumEntry("Top", 0);
  mCurrentItem = config->readNumEntry("Current", 0);

  mPaintInfo.orderOfArrival = config->readBoolEntry( "OrderOfArrival", TRUE );
}


//-----------------------------------------------------------------------------
void KMHeaders::writeFolderConfig (void)
{
  KConfig* config = app->config();
  assert(mFolder!=NULL);
  int pathLen = mFolder->path().length() - folderMgr->basePath().length();
  QString path = mFolder->path().right( pathLen );

  if (!path.isEmpty())
    path = path.right( path.length() - 1 ) + "/";
  config->setGroup("Folder-" + path + mFolder->name());
  config->writeEntry("SenderWidth", columnWidth(1));
  config->writeEntry("SubjectWidth", columnWidth(2));
  config->writeEntry("DateWidth", columnWidth(3));
  config->writeEntry("SortColumn", (mSortDescending ? -mSortCol : mSortCol));
  config->writeEntry("Top", topItemIndex());
  config->writeEntry("Current", currentItemIndex());
  config->writeEntry("OrderOfArrival", mPaintInfo.orderOfArrival);
}


//-----------------------------------------------------------------------------
void KMHeaders::setFolder (KMFolder *aFolder)
{
  int id;
  QString str;
  bool autoUpd = isUpdatesEnabled();
  setUpdatesEnabled(FALSE);
  header()->setUpdatesEnabled(FALSE);
  viewport()->setUpdatesEnabled(FALSE);

  setColumnText( mSortCol, QIconSet( QPixmap()), columnText( mSortCol ));
  if (mFolder && mFolder==aFolder)
  {
    int top = topItemIndex();
    id = currentItemIndex();
    updateMessageList();
    setCurrentMsg(id);
    setTopItemByIndex(top);
  }
  else
  {
    if (mFolder)
    {
      mFolder->markNewAsUnread();
      writeFolderConfig();
      disconnect(mFolder, SIGNAL(msgHeaderChanged(int)),
		 this, SLOT(msgHeaderChanged(int)));
      disconnect(mFolder, SIGNAL(msgAdded(int)),
		 this, SLOT(msgAdded(int)));
      disconnect(mFolder, SIGNAL(msgRemoved(int)),
		 this, SLOT(msgRemoved(int)));
      disconnect(mFolder, SIGNAL(changed()),
		 this, SLOT(msgChanged()));
      disconnect(mFolder, SIGNAL(statusMsg(const QString&)),
		 mOwner, SLOT(statusMsg(const QString&)));
      mFolder->close();
    }

    mFolder = aFolder;

    if (mFolder)
    {
      connect(mFolder, SIGNAL(msgHeaderChanged(int)),
	      this, SLOT(msgHeaderChanged(int)));
      connect(mFolder, SIGNAL(msgAdded(int)),
	      this, SLOT(msgAdded(int)));
      connect(mFolder, SIGNAL(msgRemoved(int)),
	      this, SLOT(msgRemoved(int)));
      connect(mFolder, SIGNAL(changed()),
	      this, SLOT(msgChanged()));
      connect(mFolder, SIGNAL(statusMsg(const QString&)),
	      mOwner, SLOT(statusMsg(const QString&)));
      readFolderConfig();
      mFolder->open();
    }

    updateMessageList();

    if (mFolder)
    {
      KMHeaderItem *item = static_cast<KMHeaderItem*>(firstChild());
      while (item && item->itemAbove())
	item = static_cast<KMHeaderItem*>(item->itemAbove());
      if (item)
	id = findUnread(TRUE, item->msgId(), TRUE);
      else
	id = -1;

      if ((id >= 0) && (id < (int)mItems.size()))
      {
        setMsgRead(id);
	setCurrentItemByIndex(id);
	if ((mSortCol == 3) && !mSortDescending)
	  setTopItemByIndex( id );
	/* Doesn't work, that is center this item, don't know why not
	int h = (mItems[id]->height()+1)/2;
	setTopItemByIndex( id );
	center( contentsX(), itemPos( mItems[id] )+h );
	*/
      }
      else
      {
        setMsgRead(mCurrentItem);
        // setTopItemByIndex Doesn't seem to work the first time, that is
	// when selecting the inbox at startup. I use the 
	// wordAroundQListViewLimitation method and a timer to get around this.
	//	debug (QString("mTopItem %1  mCurrentItem %2").arg(mTopItem).arg(mCurrentItem));
	setTopItemByIndex(mTopItem);
	setCurrentItemByIndex(mCurrentItem);
	//	debug (QString("mTopItem %1  mCurrentItem %2").arg(topItemIndex()).arg(currentItemIndex()));
      }
    }
    else setCurrentItemByIndex(0);
    makeHeaderVisible();
  }

  if (mFolder)
  {
    if (stricmp(mFolder->whoField(), "To")==0)
      setColumnText( 1, i18n("Receiver") );
    else
      setColumnText( 1, i18n("Sender") );

    str = i18n("%1 Messages, %2 unread.")
      .arg(mFolder->count())
      .arg(mFolder->countUnread());
    if (mFolder->isReadOnly()) str += i18n("Folder is read-only.");
    mOwner->statusMsg(str);
  }

  QString colText = i18n( "Date" );
  if (mPaintInfo.orderOfArrival)
    colText = i18n( "Date (Order of Arrival)" );
  setColumnText( 3, colText);
  if (!mSortDescending)
    setColumnText( mSortCol, *up, columnText( mSortCol ));
  else
    setColumnText( mSortCol, *down, columnText( mSortCol ));

  setUpdatesEnabled(autoUpd);
  viewport()->setUpdatesEnabled(autoUpd);
  header()->setUpdatesEnabled(autoUpd);
  if (autoUpd) repaint();
  if (autoUpd) viewport()->repaint();
  if (autoUpd) header()->repaint();
}

// QListView::setContentsPos doesn't seem to work
// until after the list view has been shown at least
// once.
void KMHeaders::workAroundQListViewLimitation()
{
  setTopItemByIndex(mTopItem);
  setCurrentItemByIndex(mCurrentItem);
}

//-----------------------------------------------------------------------------
void KMHeaders::msgChanged()
{
  int i = topItemIndex();
  int cur = currentItemIndex();
  if (!isUpdatesEnabled()) return;
  updateMessageList();
  setTopItemByIndex( i );
  setCurrentMsg(cur);
  setSelected( currentItem(), TRUE );
}


//-----------------------------------------------------------------------------
void KMHeaders::msgAdded(int id)
{
  if (!isUpdatesEnabled()) return;
  mItems.resize( mFolder->count() );
  KMMsgBase* mb = mFolder->getMsgBase( id );
  assert(mb != NULL); // otherwise using count() above is wrong

  KMHeaderItem* hi = new KMHeaderItem( this, mFolder, id, &mPaintInfo );
  mItems[id] = hi;

  msgHeaderChanged(id);
}


//-----------------------------------------------------------------------------
void KMHeaders::msgRemoved(int id)
{
  if (!isUpdatesEnabled()) return;

  if ((id < 0) || (id >= (int)mItems.size()))
    return;
  delete mItems[id];
  for (int i = id; i < (int)mItems.size() - 1; ++i) {
    mItems[i] = mItems[i+1];
    mItems[i]->setMsgId( i );
  }
  mItems.resize( mItems.size() - 1 );
  triggerUpdate();
}


//-----------------------------------------------------------------------------
void KMHeaders::msgHeaderChanged(int msgId)
{
  QString fromStr, subjStr;

  if (msgId<0 || msgId >= (int)mItems.size() || !isUpdatesEnabled()) return;
  mItems[msgId]->irefresh();
  mItems[msgId]->repaint();
}

/*
// Fixme! Delete this
//-----------------------------------------------------------------------------
void KMHeaders::headerClicked(int column)
{
  int idx = currentItemIndex();
  KMMsgBasePtr cur;
  QString sortStr = "(unknown)";
  QString msg;
  static bool working = FALSE;

  if (working) return;
  working = TRUE;

  kbp->busy();

  if (idx >= 0) cur = (*mFolder)[idx];
  else cur = NULL;

  if (mSortCol == column)
  {
    if (!mSortDescending) mSortDescending = TRUE;
    else
    {
      mSortCol = (int)KMMsgList::sfNone;
      mSortDescending = FALSE;
      sortStr = i18n("order of arrival");
    }
  }
  else
  {
    mSortCol = column;
    mSortDescending = FALSE;
  }

  if (mSortCol==(int)KMMsgList::sfSubject) sortStr = i18n("subject");
  else if (mSortCol==(int)KMMsgList::sfDate) sortStr = i18n("date");
  else if (mSortCol==(int)KMMsgList::sfFrom) sortStr = i18n("sender");
  else if (mSortCol==(int)KMMsgList::sfStatus) sortStr = i18n("status");

  if (mSortDescending) msg = i18n("Sorting messages descending by %1")
				   .arg(sortStr);
  else msg = i18n("Sorting messages ascending by %1").arg(sortStr);
  mOwner->statusMsg(msg);

  sort();

  if (cur) idx = mFolder->find(cur);
  else idx = 0;

  setCurrentMsg(idx);
  idx -= 3;
  if (idx < 0) idx = 0;
  setTopItemByIndex(idx);

  mOwner->statusMsg(msg);
  kapp->processEvents(200);
  working = FALSE;
  kbp->idle();
}
*/
                                                             
//-----------------------------------------------------------------------------
void KMHeaders::setMsgStatus (KMMsgStatus status, int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
    msg->setStatus(status);
}


//-----------------------------------------------------------------------------
void KMHeaders::applyFiltersOnMsg(int /*msgId*/)
{
  KMMessage* msg;
  KMMessageList* msgList = selectedMsgs();
  int idx, cur = firstSelectedMsg();
  int topX = contentsX();
  int topY = contentsY();

  QListViewItem *qlvi = currentItem();
  QListViewItem *next = qlvi;
  while (next && next->isSelected())
    next = next->itemBelow();
  if (!next || (next && next->isSelected())) {
    next = qlvi;
    while (next && next->isSelected())
      next = next->itemAbove();
  }

  for (idx=cur, msg=msgList->first(); msg; msg=msgList->next())
    if (filterMgr->process(msg) == 2) {
      // something went horribly wrong (out of space?)
      perror("Critical error: Unable to process messages (out of space?)");
      warning(i18n("Critical error: Unable to process messages (out of space?)"));
      break;
    }
  
  if (cur > (int)mItems.size()) cur = mItems.size()-1;
  clearSelection();
  setContentsPos( topX, topY );
  if (next) {
    setCurrentItem( next );
    setSelected( next, TRUE );
  }
  makeHeaderVisible();
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgRead (int msgId)
{
  KMMessage* msg;
  KMMsgStatus st;
  
  for (msg=getMsg(msgId); msg; msg=getMsg())
    {
      st = msg->status();
      if (st==KMMsgStatusNew || st==KMMsgStatusUnread ||
	  st==KMMsgStatusRead || st==KMMsgStatusOld)
	{
	  msg->setStatus(KMMsgStatusOld);
	}
    }
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg (int msgId)
{
  if (mFolder != trashFolder)
  {
    // move messages into trash folder
    moveMsgToFolder(trashFolder, msgId);
  }
  else
  {
    // We are in the trash folder -> really delete messages
    moveMsgToFolder(NULL, msgId);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::saveMsg (int msgId)
{
  KMMessage* msg;
  QCString str;
  QString fileName = KFileDialog::getSaveFileName(".", "*");

  if (fileName.isEmpty()) return;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    str += "From ???@??? 00:00:00 1997 +0000\n";
    str += msg->asString();
    str += "\n";
  }

  if (kCStringToFile(str, fileName, TRUE))
    mOwner->statusMsg(i18n("Message(s) saved."));
  else
    mOwner->statusMsg(i18n("Failed to save message(s)."));
}


//-----------------------------------------------------------------------------
void KMHeaders::resendMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg, *newMsg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  newMsg = new KMMessage;
  newMsg->fromString(msg->asString());
  newMsg->initHeader();
  newMsg->setTo(msg->to());
  newMsg->setSubject(msg->subject());

  win = new KMComposeWin;
  win->setMsg(newMsg, FALSE);
  win->show();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createForward());
  win->show();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::replyToMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createReply(FALSE));
  win->show();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::replyAllToMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createReply(TRUE));
  win->show();
  kbp->idle();
}

//-----------------------------------------------------------------------------
void KMHeaders::moveSelectedToFolder( int menuId )
{
  if (mMenuToFolder[menuId])
    moveMsgToFolder( mMenuToFolder[menuId] );
}

//-----------------------------------------------------------------------------
void KMHeaders::moveMsgToFolder (KMFolder* destFolder, int msgId)
{
  KMMessageList* msgList;
  KMMessage *msg;
  KMMsgBase *curMsg = 0;
  int top, rc;
  bool doUpd;

  kbp->busy();
  top = topItemIndex();

  if (destFolder) {
    if(destFolder->open() != 0)
      return;
  }

  int contentX, contentY;
  QListViewItem *curItem;
  KMHeaderItem *item;
  curItem = currentItem();
  while (curItem && curItem->isSelected() && curItem->itemBelow())
    curItem = curItem->itemBelow();
  while (curItem && curItem->isSelected() && curItem->itemAbove())
    curItem = curItem->itemAbove();
  item = static_cast<KMHeaderItem*>(curItem);
  if (item  && !item->isSelected())
    curMsg = mFolder->getMsgBase(item->msgId());

  contentX = contentsX();
  contentY = contentsY();

  msgList = selectedMsgs(msgId);
  bool autoUpd = isUpdatesEnabled();
  doUpd = (msgList->count() >= 1);
  if (doUpd) {
    setUpdatesEnabled(FALSE);
    header()->setUpdatesEnabled(FALSE);
    viewport()->setUpdatesEnabled(FALSE);
  }

  for (rc=0, msg=msgList->first(); msg && !rc; msg=msgList->next())
  {
    if (destFolder) {
      // "deleting" messages means moving them into the trash folder
      rc = destFolder->moveMsg(msg);
    }
    else
    {
      // really delete messages that are already in the trash folder
      mFolder->removeMsg(msg);
      delete msg;
    }
  }

  if (doUpd)
  {
    updateMessageList();

    if (curMsg) {
      debug ("new message should be current!");
      setCurrentMsg( mFolder->find( curMsg ) );
      setSelected( currentItem(), TRUE );
    }
    else
      emit selected( 0 );
    setContentsPos( contentX, contentY );
    makeHeaderVisible();
    setUpdatesEnabled(autoUpd);
    viewport()->setUpdatesEnabled(autoUpd);
    header()->setUpdatesEnabled(autoUpd);
    if (autoUpd) repaint();
    if (autoUpd) viewport()->repaint();
    if (autoUpd) header()->repaint();
  }

  if (destFolder) destFolder->close();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::copySelectedToFolder(int menuId ) 
{
  if (mMenuToFolder[menuId])
    copyMsgToFolder( mMenuToFolder[menuId] );
}


//-----------------------------------------------------------------------------
void KMHeaders::copyMsgToFolder (KMFolder* destFolder, int msgId)
{
  KMMessageList* msgList;
  KMMessage *msg, *newMsg;
  int top, rc;

  if (!destFolder) return;

  kbp->busy();
  top = topItemIndex();

  destFolder->open();
  msgList = selectedMsgs(msgId);
  for (rc=0, msg=msgList->first(); msg && !rc; msg=msgList->next())
  {
    newMsg = new KMMessage;
    newMsg->fromString(msg->asString());
    assert(newMsg != NULL);

    rc = destFolder->addMsg(newMsg);
  }
  destFolder->close();

  QListViewItem *item;
  for (item = firstChild(); item; item = item->nextSibling())
    if (item->isSelected())
      setSelected( item, FALSE );

  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::setCurrentMsg(int cur)
{
  if (cur >= mFolder->count()) cur = mFolder->count() - 1;
  if ((cur >= 0) && (cur < (int)mItems.size())) {
    clearSelection();
    setCurrentItem( mItems[cur] );
    setSelected( mItems[cur], TRUE );
  }
  makeHeaderVisible();
}


//-----------------------------------------------------------------------------
KMMessageList* KMHeaders::selectedMsgs(int idx)
{
  KMMessage* msg;

  mSelMsgList.clear();
  for (msg=getMsg(idx); msg; msg=getMsg())
    mSelMsgList.append(msg);

  return &mSelMsgList;
}


//-----------------------------------------------------------------------------
int KMHeaders::firstSelectedMsg() const
{
  int selectedMsg = -1;
  QListViewItem *item;
  for (item = firstChild(); item; item = item->nextSibling())
    if (item->isSelected()) {
      selectedMsg = (static_cast<KMHeaderItem*>(item))->msgId();
      break;
    }
  return selectedMsg;
}


//-----------------------------------------------------------------------------
KMMessage* KMHeaders::getMsg (int msgId)
{

  if (!mFolder || msgId < -2)
  {
    getMsgIndex = -1;
    return NULL;
  }

  if (msgId >= 0)
  {
    getMsgIndex = msgId;
    getMsgItem = 0;
    getMsgMulti = FALSE;
    return mFolder->getMsg(msgId);
  }

  if (msgId == -1)
  {
    getMsgMulti = TRUE;
    getMsgIndex = currentItemIndex();
    getMsgItem = static_cast<KMHeaderItem*>(currentItem());
    QListViewItem *qitem;
    for (qitem = firstChild(); qitem; qitem = qitem->nextSibling())
      if (qitem->isSelected()) {
	KMHeaderItem *item = static_cast<KMHeaderItem*>(qitem);
	getMsgIndex = item->msgId();
	getMsgItem = item;
	break;
      }

    return (getMsgIndex>=0 ? mFolder->getMsg(getMsgIndex) : (KMMessage*)NULL);
  }

  if (getMsgIndex < 0) return NULL;

  if (getMsgMulti)
  {
    QListViewItem *qitem = getMsgItem->nextSibling();
    for (; qitem; qitem = qitem->nextSibling())
      if (qitem->isSelected()) {
	KMHeaderItem *item = static_cast<KMHeaderItem*>(qitem);
	getMsgIndex = item->msgId();
	getMsgItem = item;
	return mFolder->getMsg(getMsgIndex);
      }
  }

  getMsgIndex = -1;
  getMsgItem = 0;
  return NULL;
}


//-----------------------------------------------------------------------------
void KMHeaders::nextMessage()
{
  QListViewItem *lvi = currentItem();
  if (lvi && lvi->itemBelow()) {
    clearSelection();
    setSelected( lvi, FALSE );
    lvi->repaint();
    setSelected( lvi->itemBelow(), TRUE );
    setCurrentItem(lvi->itemBelow());
    makeHeaderVisible();
   }
}

//-----------------------------------------------------------------------------
void KMHeaders::prevMessage()
{
  QListViewItem *lvi = currentItem();
  if (lvi && lvi->itemAbove()) {
    clearSelection();
    setSelected( lvi, FALSE );
    lvi->repaint();
    setSelected( lvi->itemAbove(), TRUE );
    setCurrentItem(lvi->itemAbove());
    makeHeaderVisible();
  }
}

//-----------------------------------------------------------------------------
int KMHeaders::findUnread(bool aDirNext, int aStartAt, bool onlyNew)
{
  KMMsgBase* msgBase = NULL;
  KMHeaderItem* item;

  if (!mFolder) return -1;
  if (!(mFolder->count()) > 0) return -1;

  if ((aStartAt >= 0) && (aStartAt < (int)mItems.size()))
    item = mItems[aStartAt];
  else {
    item = currentHeaderItem();
    if (!item) 
      item = static_cast<KMHeaderItem*>(firstChild());
    if (!item) 
      return -1;
    
    if (aDirNext) 
      item = static_cast<KMHeaderItem*>(item->itemBelow());
    else
      item = static_cast<KMHeaderItem*>(item->itemAbove());
  }

  while (item) {
    msgBase = mFolder->getMsgBase(item->msgId());
    if (!onlyNew && msgBase && msgBase->isUnread()) break;
    if (onlyNew && msgBase && msgBase->isNew()) break;
    if (aDirNext)
      item = static_cast<KMHeaderItem*>(item->itemBelow());
    else
      item = static_cast<KMHeaderItem*>(item->itemAbove());
  }
  if (item)
    return item->msgId();

  
  // A cludge to try to keep the number of unread messages in sync
  if (!onlyNew)
    mFolder->correctUnreadMsgsCount();
  return -1;
}


//-----------------------------------------------------------------------------
void KMHeaders::nextUnreadMessage()
{
  int i = findUnread(TRUE);
  setCurrentMsg(i);
}


//-----------------------------------------------------------------------------
void KMHeaders::prevUnreadMessage()
{
  int i = findUnread(FALSE);
  setCurrentMsg(i);
}


//-----------------------------------------------------------------------------
void KMHeaders::makeHeaderVisible()
{
  if (currentItem())
    ensureItemVisible( currentItem() );
}

//-----------------------------------------------------------------------------
void KMHeaders::highlightMessage(QListViewItem* lvi)
{
  KMHeaderItem *item = static_cast<KMHeaderItem*>(lvi);
  if (!item)
    return;

  int idx = item->msgId();

  mOwner->statusMsg("");
  emit selected(mFolder->getMsg(idx));
  if (idx >= 0) setMsgRead(idx);
  mItems[idx]->irefresh();
  mItems[idx]->repaint();
}


//-----------------------------------------------------------------------------
void KMHeaders::selectMessage(QListViewItem* lvi)
{
  KMHeaderItem *item = static_cast<KMHeaderItem*>(lvi);
  if (!item)
    return;

  int idx = item->msgId();
  emit activated(mFolder->getMsg(idx));
  if (idx >= 0) setMsgRead(idx);
}


//-----------------------------------------------------------------------------
void KMHeaders::updateMessageList(void)
{
  long i;
  KMMsgBase* mb;
  bool autoUpd;

  KMHeadersInherited::setSorting( mSortCol, !mSortDescending );
  //    clear();
  //    mItems.resize(0);
  if (!mFolder)
  {
    clear();
    mItems.resize(0);
    repaint();
    return;
  }

  // About 60% of the time spent in populating the list view in spent
  // in operator new and QListViewItem::QListViewItem. Reseting an item
  // instead of new'ing takes only about 1/3 as long. Hence this attempt
  // reuse list view items when possibly.
  //

  //  kbp->busy();
  autoUpd = isUpdatesEnabled();
  setUpdatesEnabled(FALSE);
  int oldSize = mItems.size();
  for (int temp = oldSize; temp > mFolder->count(); --temp)
    if (mItems[temp-1])
      delete mItems[temp-1];
  
  mItems.resize( mFolder->count() );
  for (i=0; i<mFolder->count(); i++)
  {
    mb = mFolder->getMsgBase(i);
    assert(mb != NULL); // otherwise using count() above is wrong

    if (i >= oldSize) {
      KMHeaderItem* hi = new KMHeaderItem( this, mFolder, i, &mPaintInfo );
      mItems[i] = hi;
    }
    else
      mItems[i]->reset( mFolder, i );
  }

  sort();

  // Reggie: This is comment especially for you.
  //
  // Unless QListView::updateGeometries is called first my calls to
  // setContentsPos (which calls QScrollView::setContentsPos)
  // doesn't work. (The vertical scroll bar hasn't been updated
  // I guess).
  //
  // I think you need to reimplement setContentsPos in QListView
  // and make sure that updateGeometries has been called if necessary.
  //
  // I was calling QListView::updateContents in order for updateGeometries
  // to be called (since the latter is private). But this was causing
  // flicker as it forces an update even if I have setUpdatesEnabled(FALSE)
  // (Things were ok in QT 2.0.2 but 2.1 forces an update).
  //
  // Now I call ensureItemVisible, because this will call updateGeometries
  // if the maybeHeight of the Root QListViewItem is -1, which it seems
  // to be (I guess it is marked as invalid after items are deleted/inserted).

  if (firstChild())
    ensureItemVisible(firstChild());
  //  updateContents(); // -sanders Started causing flicker in QT 2.1cvs :-(

  setUpdatesEnabled(autoUpd);
  if (autoUpd) repaint();
  // WABA: The following line is somehow necassery
  highlightMessage(currentItem()); 
}


//-----------------------------------------------------------------------------
// KMail selection - simple description
// Normal click - select and make current just this item  unselect all others
// Shift click  - select all items from current item to clicked item
//                can be used multiple times
// Cntrl click -  select this item in addition to current selection make this
//                item the current item.
void KMHeaders::contentsMousePressEvent(QMouseEvent* e)
{
  beginSelection = currentItem();
  presspos = e->pos();
  mousePressed = TRUE;
  QListViewItem *lvi = itemAt( contentsToViewport( e->pos() ));
  if (!lvi) {
    KMHeadersInherited::contentsMousePressEvent(e);
    return;
  }
  
  setCurrentItem( lvi );
  if ((e->button() == LeftButton) && 
      !(e->state() & ControlButton) &&
      !(e->state() & ShiftButton)) {
    if (!(lvi->isSelected())) {
      clearSelection();
      KMHeadersInherited::contentsMousePressEvent(e);
    }
  }
  else if ((e->button() == LeftButton) && (e->state() & ShiftButton)) {
    if (!shiftSelection( beginSelection, lvi ))
      shiftSelection( lvi, beginSelection );
  }
  else if ((e->button() == LeftButton) && (e->state() & ControlButton)) {
    setSelected( lvi, !lvi->isSelected() );
  }
  else if (e->button() == RightButton)
  {
    if (!(lvi->isSelected())) {
      clearSelection();
      setSelected( lvi, TRUE );
    }
    slotRMB();
  }
}

void KMHeaders::contentsMouseReleaseEvent(QMouseEvent* e)
{
  QListViewItem *endSelection = itemAt( contentsToViewport( e->pos() ));

  if ((e->button() == LeftButton) 
      && !(e->state() & ControlButton) 
      && !(e->state() & ShiftButton)) {
    clearSelectionExcept( endSelection );
  }

  if (e->button() != RightButton)
    KMHeadersInherited::contentsMouseReleaseEvent(e);

  beginSelection = 0;
  endSelection = 0;
  mousePressed = FALSE;
}

void KMHeaders::contentsMouseMoveEvent( QMouseEvent* e )
{
    if ( mousePressed && (e->pos() - presspos).manhattanLength() > 4 ) {
	mousePressed = FALSE;
        QListViewItem *item = itemAt( contentsToViewport(presspos) );
        if ( item ) {
	  KMHeaderToFolderDrag* d = new KMHeaderToFolderDrag(viewport());
	  d->drag();
	}
    }
}

void KMHeaders::clearSelectionExcept( QListViewItem *exception )
{
  QListViewItem *item;
  for (item = firstChild(); item; item = item->nextSibling())
    if (item->isSelected() && (item != exception))
      setSelected( item, FALSE );
}

bool KMHeaders::shiftSelection( QListViewItem *begin, QListViewItem *end )
{
  QListViewItem *search = begin;
  while (search && search->itemBelow() && (search != end))
    search = search->itemBelow();
  if (search && (search == end)) {
    while (search && (search != begin)) {
      setSelected( search, TRUE );
      search = search->itemAbove();
    }
    setSelected( search, TRUE );
    return TRUE;
  }
  return FALSE;
}

//-----------------------------------------------------------------------------
void KMHeaders::slotRMB()
{
  if (!topLevelWidget()) return; // safe bet

  QPopupMenu *menu = new QPopupMenu;

  /* Very obscure feature, strange implementation
     TODO: Reimplement this.
  if (colId == 0) // popup status menu
  {
    connect(menu, SIGNAL(activated(int)), topLevelWidget(),
          SLOT(slotSetMsgStatus(int)));
    menu->insertItem(i18n("New"), (int)KMMsgStatusNew);
    menu->insertItem(i18n("Unread"), (int)KMMsgStatusUnread);
    menu->insertItem(i18n("Read"), (int)KMMsgStatusOld);
    menu->insertItem(i18n("Replied"), (int)KMMsgStatusReplied);
    menu->insertItem(i18n("Queued"), (int)KMMsgStatusQueued);
    menu->insertItem(i18n("Sent"), (int)KMMsgStatusSent);
  }
  */

  KMFolderDir *dir = &folderMgr->dir();
  mMenuToFolder.clear();

  QPopupMenu *msgMoveMenu;
  msgMoveMenu =  mOwner->folderToPopupMenu( dir, TRUE, this, &mMenuToFolder );
  QPopupMenu *msgCopyMenu;
  msgCopyMenu =  mOwner->folderToPopupMenu( dir, FALSE, this, &mMenuToFolder );

  menu->insertItem(i18n("&Reply..."), topLevelWidget(),
		   SLOT(slotReplyToMsg()));
  menu->insertItem(i18n("Reply &All..."), topLevelWidget(),
		   SLOT(slotReplyAllToMsg()));
  menu->insertItem(i18n("&Forward..."), topLevelWidget(),
		   SLOT(slotForwardMsg()), Key_F);
  menu->insertSeparator();
  menu->insertItem(i18n("&Move to"), msgMoveMenu);
  menu->insertItem(i18n("&Copy to"), msgCopyMenu);
  menu->insertItem(i18n("&Delete"), topLevelWidget(),
		   SLOT(slotDeleteMsg()), Key_D);

  menu->exec (QCursor::pos(), 0);
  delete menu;
}

//-----------------------------------------------------------------------------
KMHeaderItem* KMHeaders::currentHeaderItem()
{
  return static_cast<KMHeaderItem*>(currentItem());
}

//-----------------------------------------------------------------------------
int KMHeaders::currentItemIndex()
{
  KMHeaderItem* item = currentHeaderItem();
  if (item)
    return item->msgId();
  else
    return -1;
}

//-----------------------------------------------------------------------------
void KMHeaders::setCurrentItemByIndex(int msgIdx)
{
  if ((msgIdx >= 0) && (msgIdx < (int)mItems.size())) {
    clearSelection();
    setSelected( mItems[msgIdx], TRUE );
    setCurrentItem( mItems[msgIdx] );
  }
}

//-----------------------------------------------------------------------------
int KMHeaders::topItemIndex()
{
  KMHeaderItem *item = static_cast<KMHeaderItem*>(itemAt(QPoint(1,1)));
  if (item)
    return item->msgId();
  else
    return -1;
}

// If sorting ascending by date/ooa then try to scroll list when new mail
// arrives to show it, but don't scroll current item out of view.
void KMHeaders::showNewMail()
{
  if (mSortCol != 3)
    return;
 for( int i = 0; i < (int)mItems.size(); ++i)
   if (mFolder->getMsgBase(i)->isNew()) {
     if (!mSortDescending)
       setTopItemByIndex( currentItemIndex() );
     break;
   }
}

void KMHeaders::setTopItemByIndex( int aMsgIdx)
{
  int msgIdx = aMsgIdx;
  if (msgIdx < 0)
    msgIdx = 0;
  else if (msgIdx >= (int)mItems.size())
    msgIdx = mItems.size() - 1;
  if ((msgIdx >= 0) && (msgIdx < (int)mItems.size()))
    setContentsPos( 0, itemPos( mItems[msgIdx] ));
}

// Update the pixmap for list view header.
// TODO: Implement support for order of arrival sorting
void KMHeaders::setSorting( int column, bool ascending )
{
  if (column != -1) {
    if (column != mSortCol)
      setColumnText( mSortCol, QIconSet( QPixmap()), columnText( mSortCol ));
    mSortCol = column;
    mSortDescending = !ascending;

    if (!ascending && (column == 3))
      mPaintInfo.orderOfArrival = !mPaintInfo.orderOfArrival;

    QString colText = i18n( "Date" );
    if (mPaintInfo.orderOfArrival)
      colText = i18n( "Date (Order of Arrival)" );
    setColumnText( 3, colText);
    
    if (ascending)
      setColumnText( column, *up, columnText(column));
    else
      setColumnText( column, *down, columnText( column ));
  }
  KMHeadersInherited::setSorting( column, ascending );
}

//-----------------------------------------------------------------------------
#include "kmheaders.moc"

