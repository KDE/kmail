// kmheaders.cpp

#include <stdlib.h>

#include <qstrlist.h>
#include <qpalette.h>
#include <qcolor.h>
#include <qdatetime.h>
#include <qheader.h>
#include <qdragobject.h>
#include <qstack.h>
#include <qqueue.h>
#include <qheader.h>

#include <kaction.h>
#include <kapp.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kstdaccel.h>
#include <kdebug.h>
#include <kimageio.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kglobalsettings.h>

#include "kmfolder.h"
#include "kmheaders.h"
#include "kmmessage.h"
#include "kbusyptr.h"
#include "kmdragdata.h"
#include "kmglobal.h"
#include "kmmainwin.h"
#include "kmcomposewin.h"
#include "kfileio.h"
#include "kmfiltermgr.h"
#include "kmfoldermgr.h"
#include "kmsender.h"
#include "kmundostack.h"
#include "kmreaderwin.h"
#include "kmacctimap.h"
#include "kmscoring.h"
#include "mailinglist-magic.h"

#include <mimelib/enum.h>
#include <mimelib/field.h>
#include <mimelib/mimepp.h>

#include <stdlib.h>

#if 0 //timing utilities
#include <qdatetime.h>
#define CREATE_TIMER(x) int x=0, x ## _tmp=0; QTime x ## _tmp2
#define START_TIMER(x) x ## _tmp2 = QTime::currentTime()
#define GRAB_TIMER(x) x ## _tmp2.msecsTo(QTime::currentTime())
#define END_TIMER(x) x += GRAB_TIMER(x); x ## _tmp++
#define SHOW_TIMER(x) qDebug(#x " == %d (%d)", x, x ## _tmp)
#else
#define CREATE_TIMER(x)
#define START_TIMER(x)
#define GRAB_TIMER(x)
#define END_TIMER(x)
#define SHOW_TIMER(x)
#endif

QPixmap* KMHeaders::pixNew = 0;
QPixmap* KMHeaders::pixUns = 0;
QPixmap* KMHeaders::pixDel = 0;
QPixmap* KMHeaders::pixOld = 0;
QPixmap* KMHeaders::pixRep = 0;
QPixmap* KMHeaders::pixQueued = 0;
QPixmap* KMHeaders::pixSent = 0;
QPixmap* KMHeaders::pixFwd = 0;
QPixmap* KMHeaders::pixFlag = 0;
QIconSet* KMHeaders::up = 0;
QIconSet* KMHeaders::down = 0;
bool KMHeaders::mTrue = true;
bool KMHeaders::mFalse = false;

//-----------------------------------------------------------------------------
// KMHeaderToFolderDrag method definitions
KMHeaderToFolderDrag::KMHeaderToFolderDrag( QWidget * parent,
					    const char * name )
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
  int mMsgId;
    QString mKey;
  // WARNING: Do not add new member variables to the class

  // Constuction a new list view item with the given colors and pixmap
    KMHeaderItem( QListView* parent, int msgId, QString key = QString::null )
    : QListViewItem( parent ),
	  mMsgId( msgId ),
	  mKey(key)
  {
    irefresh();
  }

  // Constuction a new list view item with the given parent, colors, & pixmap
    KMHeaderItem( QListViewItem* parent, int msgId, QString key = QString::null )
    : QListViewItem( parent ),
	  mMsgId( msgId ),
	  mKey(key)
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
    KMHeaders *headers = static_cast<KMHeaders*>(listView());
    int threadingPolicy = headers->getNestingPolicy();
    if(threadingPolicy == 3) {
      KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );
      if (mMsgBase->status() == KMMsgStatusNew ||
	  mMsgBase->status() == KMMsgStatusUnread)
        threadingPolicy = 1;
    }
    if (threadingPolicy < 2) {
    KMHeaderItem * topOfThread = this;
    while(topOfThread->parent())
    	topOfThread = (KMHeaderItem*)topOfThread->parent();
       topOfThread->setOpen( true );
  }
  }

  // Return the msgId of the message associated with this item
  int msgId()
  {
    return mMsgId;
  }

  // Update this item to summarise a new folder and message
  void reset( int aMsgId )
  {
    mMsgId = aMsgId;
    irefresh();
  }

  //Opens all children in the thread
  void setOpen( bool open )
  {
    if (open){
      QListViewItem * lvchild;
      lvchild = firstChild();
      while (lvchild){
	lvchild->setOpen( true );
	lvchild = lvchild->nextSibling();
      }
      QListViewItem::setOpen( true );
    } else
	QListViewItem::setOpen( false );
  }

  QString text( int col) const
  {
    KMHeaders *headers = static_cast<KMHeaders*>(listView());
    KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );
    QString tmp;

    if(col == headers->paintInfo()->flagCol) {
      if (headers->paintInfo()->flagCol >= 0)
	tmp = QString( QChar( (char)mMsgBase->status() ));

    } else if(col == headers->paintInfo()->senderCol) {
      KMFolder *folder = headers->folder();
      if (folder == kernel->outboxFolder() || folder == kernel->sentFolder()
          || folder == kernel->draftsFolder())
        tmp = mMsgBase->toStrip();
      else
        tmp = mMsgBase->fromStrip();
      if (tmp.isEmpty())
        tmp = i18n("Unknown");
      else
        tmp = tmp.simplifyWhiteSpace();

    } else if(col == headers->paintInfo()->subCol) {
      tmp = mMsgBase->subject();
      if (tmp.isEmpty())
	tmp = i18n("No Subject");
      else
	tmp = tmp.simplifyWhiteSpace();

    } else if(col == headers->paintInfo()->dateCol) {
        tmp = KMHeaders::formatDate(mMsgBase->date(), headers->paintInfo()->dateDisplay );
    } else if(col == headers->paintInfo()->sizeCol
      && headers->paintInfo()->showSize) {
        tmp.setNum(mMsgBase->msgSize());

    } else if(col == headers->paintInfo()->scoreCol) {
      tmp.setNum(headers->messageScore(mMsgId));
    }

    return tmp;
  }

  void setup()
  {
    widthChanged();
    const int ph = KMHeaders::pixNew->height();
    QListView *v = listView();
    int h = QMAX( v->fontMetrics().height(), ph ) + 2*v->itemMargin();
    h = QMAX( h, QApplication::globalStrut().height());
    if ( h % 2 > 0 )
      h++;
    setHeight( h );
  }

  const QPixmap * pixmap( int col) const
  {
    if(!col) {
      QPixmap *pix = NULL;
      KMHeaders *headers = static_cast<KMHeaders*>(listView());
      KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );
      switch (mMsgBase->status())
      {
	case KMMsgStatusNew:
	  pix = KMHeaders::pixNew;
	  break;
        case KMMsgStatusUnread:
	  pix = KMHeaders::pixUns;
	  break;
        case KMMsgStatusDeleted:
	  pix = KMHeaders::pixDel;
	  break;
        case KMMsgStatusReplied:
	  pix = KMHeaders::pixRep;
	  break;
	case KMMsgStatusForwarded:
	  pix = KMHeaders::pixFwd;
	  break;
	case KMMsgStatusQueued:
	  pix = KMHeaders::pixQueued;
	  break;
	case KMMsgStatusSent:
	  pix = KMHeaders::pixSent;
	  break;
	case KMMsgStatusFlag:
	  pix = KMHeaders::pixFlag;
	  break;
	default:
	  pix = KMHeaders::pixOld;
	  break;
      }
      return pix;
    }
    return NULL;
  }

  void paintCell( QPainter * p, const QColorGroup & cg,
				int column, int width, int align )
  {
    KMHeaders *headers = static_cast<KMHeaders*>(listView());
    QColorGroup _cg( cg );
    QColor c = _cg.text();
    QColor *color;

    KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );
    switch (mMsgBase->status())
    {
      case KMMsgStatusNew:
	color = (QColor*)(&headers->paintInfo()->colNew);
	break;
      case KMMsgStatusUnread:
	color = (QColor*)(&headers->paintInfo()->colUnread);
	break;
      case KMMsgStatusFlag:
	color = (QColor *)(&headers->paintInfo()->colFlag);
	break;
      default:
	color = (QColor *)(&headers->paintInfo()->colFore);
	break;
    }

    _cg.setColor( QColorGroup::Text, *color );

    KConfig *conf = kapp->config();
    KConfigGroupSaver saver(conf, "Fonts");
    if( column == headers->paintInfo()->dateCol ) {
      if (!conf->readBoolEntry("defaultFonts",TRUE)) {
        QFont folderFont = QFont("courier");
        p->setFont(conf->readFontEntry("list-date-font", &folderFont));
      } else {
        p->setFont(KGlobalSettings::generalFont());
      }
    }

    QListViewItem::paintCell( p, _cg, column, width, align );

    _cg.setColor( QColorGroup::Text, c );
  }

  static QString generate_key( int id, KMMsgBase *msg, const KMPaintInfo *paintInfo, int column)
  {
    QString ret = QString("%1") .arg( (char)column );
    QString sortArrival = QString( "%1" ).arg( id, 8, 36 );
    time_t mDate = msg->date();
    const int dateLength = 30;
    char cDate[dateLength + 1];
    strftime( cDate, dateLength, "%Y:%j:%H:%M:%S", gmtime( &mDate ));
    QString sortDate = cDate + sortArrival;
    if (column == paintInfo->dateCol) {
      if (paintInfo->orderOfArrival)
	return ret + sortArrival;
      else
	return ret + sortDate;
    } else if (column == paintInfo->senderCol) {
      QString tmp;
      KMFolder *folder = msg->parent();
      if (folder == kernel->outboxFolder() || folder == kernel->sentFolder()
	  || folder == kernel->draftsFolder())
	tmp = msg->toStrip();
      else
	tmp = msg->fromStrip();
      return ret + tmp.lower() + " " + sortArrival;
    } else if (column == paintInfo->subCol) {
      if (paintInfo->status)
	return ret + QString( QChar( (uint)msg->status() ));
      return ret + KMMsgBase::skipKeyword( msg->subject().lower() ) + " " + sortArrival;
    }
    else if (column == paintInfo->sizeCol) {
      return ret + QString( "%1" ).arg( msg->msgSize(), 9 );
    }
      return ret + "missing key"; //you forgot something!!
  }

  virtual QString key( int column, bool /*ascending*/ ) const
  {
    //This code should stay pretty much like this, if you are adding new
    //columns put them in generate_key
    const char req_col = (char)column;
    if(mKey.isEmpty() || mKey.left(1) != &req_col) {
      KMHeaders *headers = static_cast<KMHeaders*>(listView());
      return ((KMHeaderItem *)this)->mKey =
	generate_key(mMsgId, headers->folder()->getMsgBase( mMsgId ),
		     headers->paintInfo(), column);
    }	
    return mKey;
  }

  QListViewItem* firstChildNonConst() /* Non const! */ {
    enforceSortOrder(); // Try not to rely on QListView implementation details
    return firstChild();
  }
};

//-----------------------------------------------------------------------------
KMHeaders::KMHeaders(KMMainWin *aOwner, QWidget *parent,
		     const char *name) :
  KMHeadersInherited(parent, name),
  mScoringManager(KMScoringManager::globalScoringManager())
{
  static bool pixmapsLoaded = FALSE;
  //qInitImageIO();
  KImageIO::registerFormats();
  mOwner  = aOwner;
  mFolder = NULL;
  getMsgIndex = -1;
  mTopItem = 0;
  setMultiSelection( TRUE );
  setAllColumnsShowFocus( TRUE );
  mNested = false;
  nestingPolicy = 3;
  mNestedOverride = false;
  mousePressed = FALSE;
  mSortInfo.dirty = TRUE;
  mSortInfo.fakeSort = 0;
  mSortInfo.removed = 0;

  // Espen 2000-05-14: Getting rid of thick ugly frames
  setLineWidth(0);

  if (!mScoringManager) {
    kdDebug() << "KMHeaders::KMHeaders() : no globalScoringManager"
              << endl;
  }

  readConfig();

  mPaintInfo.flagCol = -1;
  mPaintInfo.subCol    = mPaintInfo.flagCol   + 1;
  mPaintInfo.senderCol = mPaintInfo.subCol    + 1;
  mPaintInfo.dateCol   = mPaintInfo.senderCol + 1;
  mPaintInfo.sizeCol   = mPaintInfo.dateCol   + 1;
  mPaintInfo.scoreCol  = mPaintInfo.sizeCol   + 0;
  mPaintInfo.showScore = false;
  mPaintInfo.orderOfArrival = false;
  mPaintInfo.status = false;
  showingScore = false;
  mSortCol = KMMsgList::sfDate;
  mSortDescending = FALSE;
  setShowSortIndicator(true);
  setFocusPolicy( WheelFocus );

  addColumn( i18n("Subject"), 310 );
  addColumn( i18n("Sender"), 170 );
  addColumn( i18n("Date"), 170 );

  if (mPaintInfo.showSize) {
    addColumn( i18n("Size"), 80 );
    setColumnAlignment( mPaintInfo.sizeCol, AlignRight );
    showingSize = true;
  } else {
    showingSize = false;
  }

  if (!pixmapsLoaded)
  {
    pixmapsLoaded = TRUE;
    pixNew   = new QPixmap( UserIcon("kmmsgnew") );
    pixUns   = new QPixmap( UserIcon("kmmsgunseen") );
    pixDel   = new QPixmap( UserIcon("kmmsgdel") );
    pixOld   = new QPixmap( UserIcon("kmmsgold") );
    pixRep   = new QPixmap( UserIcon("kmmsgreplied") );
    pixQueued= new QPixmap( UserIcon("kmmsgqueued") );
    pixSent  = new QPixmap( UserIcon("kmmsgsent") );
    pixFwd   = new QPixmap( UserIcon("kmmsgforwarded") );
    pixFlag  = new QPixmap( UserIcon("kmmsgflag") );
    up = new QIconSet( UserIcon("abup" ), QIconSet::Small );
    down = new QIconSet( UserIcon("abdown" ), QIconSet::Small );
  }

  connect(this, SIGNAL(doubleClicked(QListViewItem*)),
  	  this,SLOT(selectMessage(QListViewItem*)));
  connect(this,SIGNAL(currentChanged(QListViewItem*)),
	  this,SLOT(highlightMessage(QListViewItem*)));
  resetCurrentTime();

  beginSelection = 0;
  endSelection = 0;
}


//-----------------------------------------------------------------------------
KMHeaders::~KMHeaders ()
{
  if (mFolder)
  {
    writeFolderConfig();
    writeSortOrder();
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

bool KMHeaders::event(QEvent *e)
{
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
     return true;
  }
  return KMHeadersInherited::event(e);
}


//-----------------------------------------------------------------------------
void KMHeaders::readColorConfig (void)
{
  KConfig* config = kapp->config();
  // Custom/System colors
  KConfigGroupSaver saver(config, "Reader");
  QColor c1=QColor(kapp->palette().normal().text());
  QColor c2=QColor("red");
  QColor c3=QColor("blue");
  QColor c4=QColor(kapp->palette().normal().base());
  QColor c5=QColor(0,0x7F,0);

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    mPaintInfo.colFore = config->readColorEntry("ForegroundColor",&c1);
    mPaintInfo.colBack = config->readColorEntry("BackgroundColor",&c4);
    QPalette newPal = kapp->palette();
    newPal.setColor( QColorGroup::Base, mPaintInfo.colBack );
    newPal.setColor( QColorGroup::Text, mPaintInfo.colFore );
    setPalette( newPal );
    mPaintInfo.colNew = config->readColorEntry("NewMessage",&c2);
    mPaintInfo.colUnread = config->readColorEntry("UnreadMessage",&c3);
    mPaintInfo.colFlag = config->readColorEntry("FlagMessage",&c5);
  }
  else {
    mPaintInfo.colFore = c1;
    mPaintInfo.colBack = c4;
    QPalette newPal = kapp->palette();
    newPal.setColor( QColorGroup::Base, c4 );
    newPal.setColor( QColorGroup::Text, c1 );
    setPalette( newPal );
    mPaintInfo.colNew = c2;
    mPaintInfo.colUnread = c3;
    mPaintInfo.colFlag = c5;
  }
}

//-----------------------------------------------------------------------------
void KMHeaders::readConfig (void)
{
  KConfig* config = kapp->config();

  // Backing pixmap support
  { // area for config group "Pixmaps"
    KConfigGroupSaver saver(config, "Pixmaps");
    QString pixmapFile = config->readEntry("Headers","");
    mPaintInfo.pixmapOn = FALSE;
    if (pixmapFile != "") {
      mPaintInfo.pixmapOn = TRUE;
      mPaintInfo.pixmap = QPixmap( pixmapFile );
    }
  }

  { // area for config group "General"
    KConfigGroupSaver saver(config, "General");
    mPaintInfo.showSize = config->readBoolEntry("showMessageSize");
    mPaintInfo.dateDisplay = FancyDate;
    QString dateDisplay = config->readEntry( "dateDisplay", "fancyDate" );
    if ( dateDisplay == "ctime" )
      mPaintInfo.dateDisplay = CTime;
    else if ( dateDisplay == "localized" )
      mPaintInfo.dateDisplay = Localized;
  }

  readColorConfig();

  // Custom/System fonts
  { // area for config group "General"
    KConfigGroupSaver saver(config, "Fonts");
    if (!(config->readBoolEntry("defaultFonts",TRUE))) {
      QFont listFont = QFont("helvetica");
      setFont(config->readFontEntry("list-font", &listFont));
    }
    else
      setFont(KGlobalSettings::generalFont());
  }

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
void KMHeaders::refreshNestedState(void)
{
  bool oldState = mNested;
  int oldNestPolicy = nestingPolicy;
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, "Geometry");
  mNested = config->readBoolEntry( "nestedMessages", FALSE );

  nestingPolicy = config->readNumEntry( "nestingPolicy", 3 );
  if ((nestingPolicy!=oldNestPolicy)||(oldState != mNested))
  {
    setRootIsDecorated( nestingPolicy != 0 && mNested != mNestedOverride );
    reset();
  }

}

//-----------------------------------------------------------------------------
void KMHeaders::readFolderConfig (void)
{
  KConfig* config = kapp->config();
  assert(mFolder!=NULL);

  KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());
  mNestedOverride = config->readBoolEntry( "threadMessagesOverride", false );
  setColumnWidth(mPaintInfo.subCol, config->readNumEntry("SubjectWidth", 310));
  setColumnWidth(mPaintInfo.senderCol, config->readNumEntry("SenderWidth", 170));
  setColumnWidth(mPaintInfo.dateCol, config->readNumEntry("DateWidth", 170));
  if (mPaintInfo.showSize) {
    int x = config->readNumEntry("SizeWidth", 80);
    setColumnWidth(mPaintInfo.sizeCol, x>0?x:10);
  }

  if (mPaintInfo.showScore) {
    int x = config->readNumEntry("ScoreWidth", 50);
    setColumnWidth(mPaintInfo.scoreCol, x>0?x:10);
  }

  mSortCol = config->readNumEntry("SortColumn", (int)KMMsgList::sfDate);
  mSortDescending = (mSortCol < 0);
  mSortCol = abs(mSortCol) - 1;

  mTopItem = config->readNumEntry("Top", 0);
  mCurrentItem = config->readNumEntry("Current", 0);

  mPaintInfo.orderOfArrival = config->readBoolEntry( "OrderOfArrival", TRUE );
  mPaintInfo.status = config->readBoolEntry( "Status", FALSE );

  { //area for config group "Geometry"
    KConfigGroupSaver saver(config, "Geometry");
    mNested = config->readBoolEntry( "nestedMessages", FALSE );
    nestingPolicy = config->readNumEntry( "nestingPolicy", 3 );
  }

  setRootIsDecorated( nestingPolicy != 0 && mNested != mNestedOverride );
}


//-----------------------------------------------------------------------------
void KMHeaders::writeFolderConfig (void)
{
  KConfig* config = kapp->config();
  int mSortColAdj = mSortCol + 1;

  assert(mFolder!=NULL);

  KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());
  config->writeEntry("SenderWidth", columnWidth(mPaintInfo.senderCol));
  config->writeEntry("SubjectWidth", columnWidth(mPaintInfo.subCol));
  config->writeEntry("DateWidth", columnWidth(mPaintInfo.dateCol));
  if (mPaintInfo.showSize)
    config->writeEntry("SizeWidth", columnWidth(mPaintInfo.sizeCol));

  if (mPaintInfo.showScore)
      config->writeEntry("ScoreWidth", columnWidth(mPaintInfo.scoreCol));

  config->writeEntry("SortColumn", (mSortDescending ? -mSortColAdj : mSortColAdj));
  config->writeEntry("Top", topItemIndex());
  config->writeEntry("Current", currentItemIndex());
  config->writeEntry("OrderOfArrival", mPaintInfo.orderOfArrival);
  config->writeEntry("Status", mPaintInfo.status);
}


//-----------------------------------------------------------------------------
void KMHeaders::setFolder (KMFolder *aFolder, bool jumpToFirst)
{
  CREATE_TIMER(set_folder);
  START_TIMER(set_folder);

  int id;
  QString str;

  mSortInfo.fakeSort = 0;
  setColumnText( mSortCol, QIconSet( QPixmap()), columnText( mSortCol ));
  if (mFolder && mFolder==aFolder) {
    int top = topItemIndex();
    id = currentItemIndex();
    updateMessageList();
    setCurrentMsg(id);
    setTopItemByIndex(top);
  } else {
    if (mFolder) {
    // WABA: Make sure that no KMReaderWin is still using a msg
    // from this folder, since it's msg's are about to be deleted.
      highlightMessage(0, false);
      mFolder->markNewAsUnread();
      writeFolderConfig();
      disconnect(mFolder, SIGNAL(msgHeaderChanged(int)),
		 this, SLOT(msgHeaderChanged(int)));
      disconnect(mFolder, SIGNAL(msgAdded(int)),
		 this, SLOT(msgAdded(int)));
      disconnect(mFolder, SIGNAL(msgRemoved(int,QString)),
		 this, SLOT(msgRemoved(int,QString)));
      disconnect(mFolder, SIGNAL(changed()),
		 this, SLOT(msgChanged()));
      disconnect(mFolder, SIGNAL(statusMsg(const QString&)),
		 mOwner, SLOT(statusMsg(const QString&)));
	    writeSortOrder();
      mFolder->close();
    }

    mSortInfo.removed = 0;
    mFolder = aFolder;
    mSortInfo.dirty = TRUE;
    mOwner->editAction->setEnabled(mFolder ?  ( (mFolder ==
						 kernel->draftsFolder()) ||
						(mFolder == kernel->outboxFolder()) ): false );
    mOwner->replyListAction->setEnabled(mFolder ? mFolder->isMailingList() :
      false);

    if (mFolder)
    {
      connect(mFolder, SIGNAL(msgHeaderChanged(int)),
	      this, SLOT(msgHeaderChanged(int)));
      connect(mFolder, SIGNAL(msgAdded(int)),
	      this, SLOT(msgAdded(int)));
      connect(mFolder, SIGNAL(msgRemoved(int,QString)),
	      this, SLOT(msgRemoved(int,QString)));
      connect(mFolder, SIGNAL(changed()),
	      this, SLOT(msgChanged()));
      connect(mFolder, SIGNAL(statusMsg(const QString&)),
	      mOwner, SLOT(statusMsg(const QString&)));

      // Not very nice, but if we go from nested to non-nested
      // in the folderConfig below then we need to do this otherwise
      // updateMessageList would do something unspeakable
      if ((mNested && !mNestedOverride) || (!mNested && mNestedOverride)) {
	clear();
	mItems.resize( 0 );
      }

      if (mScoringManager)
        mScoringManager->initCache((mFolder) ? mFolder->name() : QString());

      mPaintInfo.showScore = mScoringManager->hasRulesForCurrentGroup();
      readFolderConfig();

      CREATE_TIMER(kmfolder_open);
      START_TIMER(kmfolder_open);
      mFolder->open();
      END_TIMER(kmfolder_open);
      SHOW_TIMER(kmfolder_open);

      if ((mNested && !mNestedOverride) || (!mNested && mNestedOverride)) {
	clear();
	mItems.resize( 0 );
      }
    }

    if (mScoringManager)
      mScoringManager->initCache((mFolder) ? mFolder->name() : QString());
  }

  CREATE_TIMER(updateMsg);
  START_TIMER(updateMsg);
  updateMessageList(!jumpToFirst); // jumpToFirst seem inverted - don
  END_TIMER(updateMsg);
  SHOW_TIMER(updateMsg);
  makeHeaderVisible();

  if (mFolder)
    setFolderInfoStatus();

  QString colText = i18n( "Sender" );
  if (mFolder && (qstricmp(mFolder->whoField(), "To")==0))
    colText = i18n("Receiver");
  setColumnText( mPaintInfo.senderCol, colText);

  colText = i18n( "Date" );
  if (mPaintInfo.orderOfArrival)
    colText = i18n( "Date (Order of Arrival)" );
  setColumnText( mPaintInfo.dateCol, colText);

  colText = i18n( "Subject" );
  if (mPaintInfo.status)
    colText = colText + i18n( " (Status)" );
  setColumnText( mPaintInfo.subCol, colText);


  if (mFolder) {
    KConfig *config = kapp->config();
    KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());

    if (mPaintInfo.showSize) {
      colText = i18n( "Size" );
      if (showingSize) {
        setColumnText( mPaintInfo.sizeCol, colText);
      } else {
        // add in the size field
        int x = config->readNumEntry("SizeWidth", 80);
        addColumn(colText, x>0?x:10);
	setColumnAlignment( mPaintInfo.sizeCol, AlignRight );
        mPaintInfo.scoreCol++;
      }
      showingSize = true;
    } else {
      if (showingSize) {
        // remove the size field
        config->writeEntry("SizeWidth", columnWidth(mPaintInfo.sizeCol));
        removeColumn(mPaintInfo.sizeCol);
        mPaintInfo.scoreCol--;
      }
      showingSize = false;
    }

    mPaintInfo.showScore = mScoringManager->hasRulesForCurrentGroup();
    if (mPaintInfo.showScore) {
      colText = i18n( "Score" );
      if (showingScore) {
        setColumnText( mPaintInfo.scoreCol, colText);
      } else {
        int x = config->readNumEntry("ScoreWidth", 50);
        mPaintInfo.scoreCol = addColumn(colText, x>0?x:10);
        setColumnAlignment( mPaintInfo.scoreCol, AlignRight );
      }
      showingScore = true;
    } else {
      if (showingScore) {
        removeColumn(mPaintInfo.scoreCol);
        showingScore = false;
      }
    }
  }
   qDebug("end %d %s:%d", (QTime::currentTime().second()*1000)+QTime::currentTime().msec(), __FILE__,__LINE__);
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
  KMHeaderItem* hi = 0;
  if (!isUpdatesEnabled()) return;

  mItems.resize( mFolder->count() );
  KMMsgBase* mb = mFolder->getMsgBase( id );
  assert(mb != NULL); // otherwise using count() above is wrong

  if ((mNested && !mNestedOverride) || (!mNested && mNestedOverride)) {
    QString msgId = mb->msgIdMD5();
    if (msgId.isNull())
      msgId = "";
    QString replyToId = mb->replyToIdMD5();

    if(mIdTree.isEmpty()) {
      QString md5;
      for(int x = 0; x < mFolder->count() - 1; x++) {
	if(mItems[x]) {
	  md5 = mFolder->getMsgBase(x)->msgIdMD5();
	  if(md5.isEmpty()) continue;
	  if(mIdTree[md5])
	    ;
	  else
	    mIdTree.insert(md5, mItems[x]);
	}
      }
    }

    if (replyToId.isEmpty() || !mIdTree[replyToId])
      hi = new KMHeaderItem( this, id );
    else {
      KMHeaderItem *parent = mIdTree[replyToId];
      assert(parent);
      hi = new KMHeaderItem( parent, id );
    }
    if (!mIdTree[msgId])
      mIdTree.replace( msgId, hi );
  }
  else
    hi = new KMHeaderItem( this, id );

  mItems[id] = hi;
  appendUnsortedItem(hi); //inserted into sorted list
  if (mSortInfo.fakeSort) {
      QObject::disconnect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
      KMHeadersInherited::setSorting(mSortCol, !mSortDescending );
      mSortInfo.fakeSort = 0;
  }

  msgHeaderChanged(id);

  if ((childCount() == 1) && hi) {
    setSelected( hi, true );
    setCurrentItem( firstChild() );
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::msgRemoved(int id, QString msgId)
{
  if (!isUpdatesEnabled()) return;

  if ((id < 0) || (id >= (int)mItems.size()))
    return;

  //Not necesary with optimizations, FIXME! ##Sam
  mSortInfo.removed = 1; // Need serial ids to optimize this out
  mSortInfo.dirty = TRUE;
  if (mSortInfo.fakeSort) {
      QObject::disconnect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
      KMHeadersInherited::setSorting(mSortCol, !mSortDescending );
      mSortInfo.fakeSort = 0;
  }

  mIdTree.remove(msgId);

  // Reparent children of item into top level
  QListViewItem *myParent = mItems[id];
  QListViewItem *myChild = myParent->firstChild();
  while (myChild) {
    QListViewItem *lastChild = myChild;
    myChild = myChild->nextSibling();
    myParent->takeItem(lastChild);
    insertItem(lastChild);
  }

  if (currentItem() == mItems[id])
    mPrevCurrent = 0;
  KMHeaderItem *removedItem = mItems[id];
  for (int i = id; i < (int)mItems.size() - 1; ++i) {
    mItems[i] = mItems[i+1];
    mItems[i]->setMsgId( i );
  }
  mItems.resize( mItems.size() - 1 );
  delete removedItem;
}


//-----------------------------------------------------------------------------
void KMHeaders::msgHeaderChanged(int msgId)
{
  if (msgId<0 || msgId >= (int)mItems.size() || !isUpdatesEnabled()) return;
  mItems[msgId]->irefresh();
  mItems[msgId]->repaint();
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgStatus (KMMsgStatus status, int /*msgId*/)
{
  QListViewItem *qitem;
  bool unget;
  for (qitem = firstChild(); qitem; qitem = qitem->itemBelow())
    if (qitem->isSelected()) {
      KMHeaderItem *item = static_cast<KMHeaderItem*>(qitem);
      KMMsgBase *msgBase = mFolder->getMsgBase(item->msgId());
      msgBase->setStatus(status);
      if (mFolder->account())
      {
        unget = !mFolder->isMessage(item->msgId());
        KMMessage *msg = mFolder->getMsg(item->msgId());
        mFolder->account()->setStatus(msg, status);
        if (unget) mFolder->unGetMsg(item->msgId());
      }
    }
}


//-----------------------------------------------------------------------------
int KMHeaders::slotFilterMsg(KMMessage *msg)
{
  int filterResult;
  KMFolder *parent = msg->parent();
  if (parent)
    parent->removeMsg( msg );
  msg->setParent(0);
  filterResult = kernel->filterMgr()->process(msg,KMFilterMgr::All);
  if (filterResult == 2) {
    // something went horribly wrong (out of space?)
    perror("Critical error: Unable to process messages (out of space?)");
    KMessageBox::information(0,
  	i18n("Critical error: Unable to process messages (out of space?)"));
    return 2;
  }
  if (!msg->parent()) {
    parent->addMsg( msg );
  }
  if (msg->parent()) // unGet this msg
    msg->parent()->unGetMsg( msg->parent()->count() -1 );
  return filterResult;
}


//-----------------------------------------------------------------------------
void KMHeaders::setFolderInfoStatus ()
{
  QString str;
  str = i18n("%n message, %1.", "%n messages, %1.", mFolder->count())
    .arg(i18n("%n unread", "%n unread", mFolder->countUnread()));
  if (mFolder->isReadOnly()) str += i18n("Folder is read-only.");
  mOwner->statusMsg(str);
}

//-----------------------------------------------------------------------------
void KMHeaders::applyFiltersOnMsg(int /*msgId*/)
{
  KMMsgBase* msgBase;
  KMMessage* msg;

  emit maybeDeleting();

  disconnect(this,SIGNAL(currentChanged(QListViewItem*)),
	     this,SLOT(highlightMessage(QListViewItem*)));
  KMMessageList* msgList = selectedMsgs();
  int topX = contentsX();
  int topY = contentsY();

  if (msgList->isEmpty())
    return;
  QListViewItem *qlvi = currentItem();
  QListViewItem *next = qlvi;
  while (next && next->isSelected())
    next = next->itemBelow();
  if (!next || (next && next->isSelected())) {
    next = qlvi;
    while (next && next->isSelected())
      next = next->itemAbove();
  }

  clearSelection();

  for (msgBase=msgList->first(); msgBase; msgBase=msgList->next()) {
    int idx = mFolder->find(msgBase);
    assert(idx != -1);
    msg = mFolder->getMsg(idx);
    if (mFolder->account() && !msg->isComplete())
    {
      if (msg->transferInProgress()) continue;
      msg->setTransferInProgress(TRUE);
      KMImapJob *imapJob = new KMImapJob(msg);
      connect(imapJob, SIGNAL(messageRetrieved(KMMessage*)),
        SLOT(slotFilterMsg(KMMessage*)));
    } else {
      if (slotFilterMsg(msg) == 2) break;
    }
  }

  setContentsPos( topX, topY );
  emit selected( 0 );
  if (next) {
    setCurrentItem( next );
    setSelected( next, TRUE );
    highlightMessage( next, false);
  }
  else if (currentItem()) {
    setSelected( currentItem(), TRUE );
    highlightMessage( currentItem(), false);
  }
  else
    emit selected( 0 );

  makeHeaderVisible();
  connect(this,SIGNAL(currentChanged(QListViewItem*)),
	  this,SLOT(highlightMessage(QListViewItem*)));
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
	  st==KMMsgStatusRead)
	{
	  msg->setStatus(KMMsgStatusOld);
          if (mFolder->account())
            mFolder->account()->setStatus(msg, KMMsgStatusOld);
	}
    }
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg (int msgId)
{
  if (mFolder != kernel->trashFolder())
  {
    // move messages into trash folder
    moveMsgToFolder(kernel->trashFolder(), msgId);
  }
  else
  {
    // We are in the trash folder -> really delete messages
    moveMsgToFolder(NULL, msgId);
  }
   mSortInfo.dirty = TRUE;
  mOwner->statusMsg("");
  //  triggerUpdate();
}


//-----------------------------------------------------------------------------
void KMHeaders::saveMsg (int msgId)
{
  KMMessage* msg;
  QString str;
  KURL url = KFileDialog::getSaveURL(QString::null, QString::null);

  if( url.isEmpty() )
    return;

  if( !url.isLocalFile() )
  {
    KMessageBox::sorry( 0L, i18n( "Only local files supported yet." ) );
    return;
  }

  QString fileName = url.path();

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    str += "From " + msg->fromEmail() + " " + msg->dateShortStr() + "\n";
    str += msg->asString();
    str += "\n";
  }

  if (kCStringToFile(str.latin1(), fileName, TRUE))
    mOwner->statusMsg(i18n("Message(s) saved."));
  else
    mOwner->statusMsg(i18n("Failed to save message(s)."));
}


//-----------------------------------------------------------------------------
void KMHeaders::resendMsg ()
{
  KMComposeWin *win;
  KMMessage *newMsg, *msg = currentMsg();
  if (!msg) return;

  kernel->kbp()->busy();
  newMsg = new KMMessage;
  newMsg->fromString(msg->asString());
  newMsg->removeHeaderField("Message-Id");
  newMsg->initHeader();
  newMsg->setCharset(msg->codec()->name());
  newMsg->setTo(msg->to());
  newMsg->setSubject(msg->subject());

  win = new KMComposeWin;
  win->setMsg(newMsg, FALSE);
  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::bounceMsg ()
{
  KMMessage *newMsg, *msg = currentMsg();

  if (!msg) return;

  newMsg = msg->createBounce( TRUE /* with UI */);
  // Queue the message for sending, so the user can still intercept
  // it. This is currently for testing
  if (newMsg)
    kernel->msgSender()->send(newMsg, FALSE);
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardMsg ()
{
  KMComposeWin *win;
  KMMessageList* msgList = selectedMsgs();

  if (msgList->count() >= 2) {
    // ask if they want a mime digest forward

    if (KMessageBox::questionYesNo(this, i18n("Forward selected messages as"
                                              " a MIME digest?"))
                                                      == KMessageBox::Yes) {
      // we default to the first identity to save prompting the user
      // (the messages could have different identities)
      QString id = "";
      KMMessage *fwdMsg = new KMMessage;
      KMMessagePart *msgPart = new KMMessagePart;
      QString msgPartText;
      int msgCnt = 0; // incase there are some we can't forward for some reason

      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->mMsg->Headers().ContentType().CreateBoundary(1);
      msgPartText = i18n("\nThis is a MIME digest forward.  The content of the"
                         " message is contained in the attachment(s).\n\n\n"
                         "--\n");
      kdDebug() << "Doing a mime digest forward\n" << endl;
      // iterate through all the messages to be forwarded
      for (KMMsgBase *mb = msgList->first(); mb; mb = msgList->next()) {
        int idx = mFolder->find(mb);
        if (idx < 0) continue;
        KMMessage *thisMsg = mFolder->getMsg(idx);
        if (!thisMsg) continue;
        // set the identity
        if (id.length() == 0)
          id = thisMsg->headerField("X-KMail-Identity");
        // set the part header
        msgPartText += "--";
        msgPartText += fwdMsg->mMsg->Headers().ContentType().Boundary().c_str();
        msgPartText += "\nContent-Type: MESSAGE/RFC822";
        msgPartText += QString("; CHARSET=%1").arg(thisMsg->charset());
        msgPartText += "\n";
        kdDebug() << "Adding message ID " << thisMsg->id() << "\n" << endl;
        DwHeaders dwh;
        dwh.MessageId().CreateDefault();
        msgPartText += QString("Content-ID: %1\n").arg(dwh.MessageId().AsString().c_str());
        msgPartText += QString("Content-Description: %1").arg(thisMsg->subject());
        if (!thisMsg->subject().contains("(fwd)"))
          msgPartText += " (fwd)";
        msgPartText += "\n\n";
        // set the part
        msgPartText += thisMsg->headerAsString();
        msgPartText += "\n";
        msgPartText += thisMsg->body();
        msgPartText += "\n";     // eot
        msgCnt++;
      }
      kdDebug() << "Done adding messages to the digest\n" << endl;
      msgPart->setTypeStr("MULTIPART");
      msgPart->setSubtypeStr(QString("Digest; boundary=\"%1\"").arg(fwdMsg->mMsg->Headers().ContentType().Boundary().c_str()));
      msgPart->setName("unnamed");
      msgPart->setCte(DwMime::kCte7bit);   // does it have to be 7bit?
      msgPart->setContentDescription(QString("Digest of %1 messages.").arg(msgCnt));
      // THIS HAS TO BE AFTER setCte()!!!!
      msgPart->setBodyEncoded(QCString(msgPartText.ascii()));
      kdDebug() << "Launching composer window\n" << endl;
      kernel->kbp()->busy();
      win = new KMComposeWin(fwdMsg, id);
      win->addAttach(msgPart);
      win->show();
      kernel->kbp()->idle();
      return;
    } else {            // NO MIME DIGEST, Multiple forward
      QString id = "";
      QString msgText = "";
      for (KMMsgBase *mb = msgList->first(); mb; mb = msgList->next()) {
        int idx = mFolder->find(mb);
        if (idx < 0) continue;
        KMMessage *thisMsg = mFolder->getMsg(idx);
        if (!thisMsg) continue;

        // set the identity
        if (id.length() == 0)
          id = thisMsg->headerField("X-KMail-Identity");

        // add the message to the body
        if (KMMessage::sHdrStyle == KMReaderWin::HdrAll) {
          msgText += "\n\n----------  " + KMMessage::sForwardStr + "  ----------\n";
          msgText += thisMsg->asString();
          msgText = QString::fromUtf8(thisMsg->asQuotedString(msgText,
            "", QString::null, FALSE, false, false));
          msgText += "\n-------------------------------------------------------\n";
        } else {
          msgText += "\n\n----------  " + KMMessage::sForwardStr + "  ----------\n";
          msgText += "Subject: " + thisMsg->subject() + "\n";
          msgText += "Date: " + thisMsg->dateStr() + "\n";
          msgText += "From: " + thisMsg->from() + "\n";
          msgText += "To: " + thisMsg->to() + "\n";
	  if (!thisMsg->cc().isEmpty()) msgText += "Cc: " + thisMsg->cc() + "\n";
          msgText += "\n";
          msgText = QString::fromUtf8(thisMsg->asQuotedString(msgText,
            "", QString::null, FALSE, false, false));
          msgText += "\n-------------------------------------------------------\n";
        }
      }
      KMMessage *fwdMsg = new KMMessage;
      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->setCharset("utf-8");
      fwdMsg->setBody(msgText.utf8());
      kernel->kbp()->busy();
      win = new KMComposeWin(fwdMsg, id);
      win->setCharset("");
      win->show();
      kernel->kbp()->idle();
      return;
    }
  }

  // forward a single message at most.

  KMMessage *msg = currentMsg();
  if (!msg) return;

  QString id = msg->headerField( "X-KMail-Identity" );
  if ( id.isEmpty() )
    id = mFolder->identity();
  kernel->kbp()->busy();
  win = new KMComposeWin(msg->createForward(), id);
  win->setCharset(msg->codec()->name(), TRUE);
  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardAttachedMsg ()
{
  KMComposeWin *win;
  KMMessageList* msgList = selectedMsgs();
  QString id = "";

  if (msgList->count() >= 2) {
    // don't respect X-KMail-Identity headers because they might differ for
    // the selected mails
    id = mFolder->identity();
  }
  else if (msgList->count() == 1) {
    KMMessage *msg = currentMsg();
    id = msg->headerField( "X-KMail-Identity" );
    if ( id.isEmpty() )
      id = mFolder->identity();
  }

  KMMessage *fwdMsg = new KMMessage;

  fwdMsg->initHeader(id);
  fwdMsg->setAutomaticFields(true);

  kdDebug() << "Launching composer window\n" << endl;
  kernel->kbp()->busy();
  win = new KMComposeWin(fwdMsg, id);

  kdDebug() << "Doing forward as attachment\n" << endl;
  // iterate through all the messages to be forwarded
  for (KMMsgBase *mb = msgList->first(); mb; mb = msgList->next()) {
    int idx = mFolder->find(mb);
    if (idx < 0) continue;
    KMMessage *thisMsg = mFolder->getMsg(idx);
    if (!thisMsg) continue;
    // set the part
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setTypeStr("message");
    msgPart->setSubtypeStr("rfc822");
    msgPart->setCharset(thisMsg->charset());
    msgPart->setName("forwarded message");
    msgPart->setCte(DwMime::kCte8bit);   // is 8bit O.K.?
    msgPart->setContentDescription(thisMsg->from()+": "+thisMsg->subject());
    // THIS HAS TO BE AFTER setCte()!!!!
    msgPart->setBodyEncoded(QCString(thisMsg->asString()));
    msgPart->setCharset("");

    thisMsg->setStatus(KMMsgStatusForwarded);

    win->addAttach(msgPart);
  }

  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::redirectMsg()
{
  KMComposeWin *win;
  KMMessage *msg = currentMsg();

  if (!msg) return;

  kernel->kbp()->busy();
  win = new KMComposeWin();
  win->setMsg(msg->createRedirect(), FALSE);
  win->setCharset(msg->codec()->name());
  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::noQuoteReplyToMsg()
{
  KMComposeWin *win;
  KMMessage *msg = currentMsg();
  QString id;

  if (!msg)
    return;

  kernel->kbp()->busy();
  id = msg->headerField( "X-KMail-Identity" );
  if ( id.isEmpty() )
    id = mFolder->identity();
  win = new KMComposeWin(msg->createReply(FALSE, FALSE, "", TRUE),id);
  win->setCharset(msg->codec()->name(), TRUE);
  win->setReplyFocus(false);
  win->show();
  kernel->kbp()->idle();
}

//-----------------------------------------------------------------------------
void KMHeaders::replyToMsg (QString selection)
{
  KMComposeWin *win;
  KMMessage *msg = currentMsg();
  QString id;

  if (!msg)
    return;

  kernel->kbp()->busy();
  id = msg->headerField( "X-KMail-Identity" );
  if ( id.isEmpty() )
    id = mFolder->identity();
  win = new KMComposeWin(msg->createReply(FALSE, FALSE, selection),id);
  win->setCharset(msg->codec()->name(), TRUE);
  win->setReplyFocus();
  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::replyAllToMsg (QString selection)
{
  KMComposeWin *win;
  KMMessage *msg = currentMsg();
  QString id;

  if (!msg) return;

  kernel->kbp()->busy();
  id = msg->headerField( "X-KMail-Identity" );
  if ( id.isEmpty() )
    id = mFolder->identity();
  win = new KMComposeWin(msg->createReply(TRUE, FALSE, selection),id);
  win->setCharset(msg->codec()->name(), TRUE);
  win->setReplyFocus();
  win->show();
  kernel->kbp()->idle();
}

//-----------------------------------------------------------------------------
void KMHeaders::replyListToMsg (QString selection)
{
  KMComposeWin *win;
  KMMessage *msg = currentMsg();
  QString id;

  if (!msg) return;

  kernel->kbp()->busy();
  id = msg->headerField( "X-KMail-Identity" );
  if ( id.isEmpty() )
    id = mFolder->identity();
  win = new KMComposeWin(msg->createReply(true, true, selection),id);
  win->setCharset(msg->codec()->name(), TRUE);
  win->setReplyFocus();
  win->show();
  kernel->kbp()->idle();
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
  if (destFolder == mFolder)
    return;

  KMMessageList* msgList;
  KMMessage *msg;
  KMMsgBase *msgBase, *curMsg = 0;
  int top, rc;

  emit maybeDeleting();

  disconnect(this,SIGNAL(currentChanged(QListViewItem*)),
	     this,SLOT(highlightMessage(QListViewItem*)));
  kernel->kbp()->busy();
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

  // The following is a rather delicate process. We can't allow getMsg
  // to be called on messages in msgList as then we will try to operate on a
  // dangling pointer below (assuming a KMMsgInfo* is deleted and a KMMessage*
  // is created when getMsg is called).
  //
  // But KMMainWin was modified recently so that exactly that happened
  // (a slot was connected to the selectionChanged signal.
  //
  // So we block all signals for awhile to avoid this.

  msgList = selectedMsgs(msgId);
  blockSignals( true ); // don't emit signals when the current message is

  int index;
  for (rc=0, msgBase=msgList->first(); msgBase && !rc; msgBase=msgList->next())
  {
    int idx = mFolder->find(msgBase);
    assert(idx != -1);
    msg = mFolder->getMsg(idx);
    if (msg->transferInProgress()) continue;

    if (destFolder) {
      rc = destFolder->moveMsg(msg, &index);
      if (rc == 0 && index != -1) {
	KMMsgBase *mb = destFolder->unGetMsg( destFolder->count() - 1 );
	kernel->undoStack()->pushAction( mb->msgIdMD5(), mFolder, destFolder );
      }
    }
    else
    {
      // really delete messages that are already in the trash folder
      mFolder->removeMsg(msg);
      delete msg;
    }
  }

  blockSignals( false );

  emit selected( 0 );
  if (curMsg) {
    kdDebug() << "new message should be current!" << endl;
    setSelected( currentItem(), TRUE );
    setCurrentMsg( mFolder->find( curMsg ) );
    highlightMessage( currentItem(), true);
  }
  else
    emit selected( 0 );

  setContentsPos( contentX, contentY );
  makeHeaderVisible();
  connect(this,SIGNAL(currentChanged(QListViewItem*)),
	     this,SLOT(highlightMessage(QListViewItem*)));

  if (destFolder) destFolder->close();
  kernel->kbp()->idle();
}

bool KMHeaders::canUndo() const
{
    return ( kernel->undoStack()->size() > 0 );
}

//-----------------------------------------------------------------------------
void KMHeaders::undo()
{
  KMMessage *msg;
  QString msgIdMD5;
  KMFolder *folder, *curFolder;
  if (kernel->undoStack()->popAction(msgIdMD5, folder, curFolder))
  {
    curFolder->open();
    int idx = curFolder->find(msgIdMD5);
    if (idx == -1) // message moved to folder that has been emptied
      return;
    msg = curFolder->getMsg( idx );
    folder->moveMsg( msg );
    if (folder->count() > 1)
      folder->unGetMsg( folder->count() - 1 );
    curFolder->close();
  }
  else
  {
    // Sorry.. stack is empty..
    KMessageBox::sorry(this, i18n("I can't undo anything, sorry!"));
  }
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
  KMMsgBase *msgBase;
  KMMessage *msg, *newMsg;
  int top, rc, index, idx = -1;
  bool isMessage;

  if (!destFolder) return;

  kernel->kbp()->busy();
  top = topItemIndex();

  destFolder->open();
  msgList = selectedMsgs(msgId);
  for (rc=0, msgBase=msgList->first(); msgBase && !rc; msgBase=msgList->next())
  {
    if (isMessage = msgBase->isMessage())
    {
      msg = static_cast<KMMessage*>(msgBase);
    } else {
      idx = mFolder->find(msgBase);
      assert(idx != -1);
      msg = mFolder->getMsg(idx);
    }

    if (mFolder->account() && mFolder->account() == destFolder->account())
    {
      new KMImapJob(msg, KMImapJob::tCopyMessage, destFolder);
    } else {
      newMsg = new KMMessage;
      newMsg->fromString(msg->asString());
      newMsg->setComplete(msg->isComplete());

      if (mFolder->account() && !newMsg->isComplete())
      {
        newMsg->setParent(msg->parent());
        KMImapJob *imapJob = new KMImapJob(newMsg);
        connect(imapJob, SIGNAL(messageRetrieved(KMMessage*)),
          destFolder, SLOT(reallyAddCopyOfMsg(KMMessage*)));
      } else {
        rc = destFolder->addMsg(newMsg, &index);
        if (rc == 0 && index != -1)
          destFolder->unGetMsg( destFolder->count() - 1 );
      }
    }
    if (!isMessage)
    {
      assert(idx != -1);
      mFolder->unGetMsg( idx );
    }
  }
  destFolder->close();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::setCurrentMsg(int cur)
{
  if (!mFolder) return;
  if (cur >= mFolder->count()) cur = mFolder->count() - 1;
  if ((cur >= 0) && (cur < (int)mItems.size())) {
    clearSelection();
    setCurrentItem( mItems[cur] );
    setSelected( mItems[cur], TRUE );
  }
  makeHeaderVisible();
  setFolderInfoStatus();
}


//-----------------------------------------------------------------------------
KMMessageList* KMHeaders::selectedMsgs(int /*idx*/)
{
  QListViewItem *qitem;

  mSelMsgList.clear();
  for (qitem = firstChild(); qitem; qitem = qitem->itemBelow())
    if (qitem->isSelected()) {
      KMHeaderItem *item = static_cast<KMHeaderItem*>(qitem);
      KMMsgBase *msgBase = mFolder->getMsgBase(item->msgId());
      mSelMsgList.append(msgBase);
    }

  return &mSelMsgList;
}


//-----------------------------------------------------------------------------
int KMHeaders::firstSelectedMsg() const
{
  int selectedMsg = -1;
  QListViewItem *item;
  for (item = firstChild(); item; item = item->itemBelow())
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
    getMsgIndex = -1;
    getMsgItem = 0;
    QListViewItem *qitem;
    for (qitem = firstChild(); qitem; qitem = qitem->itemBelow())
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
    QListViewItem *qitem = getMsgItem->itemBelow();
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
    QListViewItem *temp = lvi;
    while (temp) {
	temp->firstChild();
	temp = temp->parent();
    }

    clearSelection();
    setSelected( lvi, FALSE );
    lvi->repaint();
    setSelected( lvi->itemBelow(), TRUE );
    setCurrentItem(lvi->itemBelow());
    makeHeaderVisible();
    setFolderInfoStatus();
   }
}

//-----------------------------------------------------------------------------
void KMHeaders::prevMessage()
{
  QListViewItem *lvi = currentItem();
  if (lvi && lvi->itemAbove()) {
    QListViewItem *temp = lvi;
    while (temp) {
	temp->firstChild();
	temp = temp->parent();
    }

    clearSelection();
    setSelected( lvi, FALSE );
    lvi->repaint();
    setSelected( lvi->itemAbove(), TRUE );
    setCurrentItem(lvi->itemAbove());
    makeHeaderVisible();
    setFolderInfoStatus();
  }
}

//-----------------------------------------------------------------------------
void KMHeaders::findUnreadAux( KMHeaderItem*& item,
					bool & foundUnreadMessage,
					bool onlyNew,
					bool aDirNext )
{
  KMMsgBase* msgBase = 0;

  while (item) {
    msgBase = mFolder->getMsgBase(item->msgId());
    if (msgBase && msgBase->isUnread())
      foundUnreadMessage = true;

    if (!onlyNew && msgBase && msgBase->isUnread()) break;
    if (onlyNew && msgBase && msgBase->isNew()) break;
    if (aDirNext)
      item = static_cast<KMHeaderItem*>(item->itemBelow());
    else
      item = static_cast<KMHeaderItem*>(item->itemAbove());
  }
}

//-----------------------------------------------------------------------------
int KMHeaders::findUnread(bool aDirNext, int aStartAt, bool onlyNew, bool acceptCurrent)
{
  KMHeaderItem *item, *pitem;
  bool foundUnreadMessage = false;

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

    if ( !acceptCurrent )
        if (aDirNext)
            item = static_cast<KMHeaderItem*>(item->itemBelow());
        else
            item = static_cast<KMHeaderItem*>(item->itemAbove());
  }

  pitem =  item;

  findUnreadAux( item, foundUnreadMessage, onlyNew, aDirNext );

  // We have found an unread item, but it is not necessary the
  // first unread item.
  //
  // Find the ancestor of the unread item closest to the
  // root and recursively sort all of that ancestors children.
  if (item) {
    QListViewItem *next = item;
    while (next->parent())
      next = next->parent();
    next = static_cast<KMHeaderItem*>(next)->firstChildNonConst();
    while (next && (next != item))
      if (static_cast<KMHeaderItem*>(next)->firstChildNonConst())
	next = next->firstChild();
      else if (next->nextSibling())
	next = next->nextSibling();
      else {
	while (next && (next != item)) {
	  next = next->parent();
	  if (next == item)
	    break;
	  if (next && next->nextSibling()) {
	    next = next->nextSibling();
	    break;
	  }
	}
      }
  }

  item = pitem;

  findUnreadAux( item, foundUnreadMessage, onlyNew, aDirNext );
  if (item)
    return item->msgId();


  // A kludge to try to keep the number of unread messages in sync
  int unread = mFolder->countUnread();
  if (((unread == 0) && foundUnreadMessage) ||
      ((unread > 0) && !foundUnreadMessage)) {
    mFolder->correctUnreadMsgsCount();
  }
  return -1;
}

//-----------------------------------------------------------------------------
void KMHeaders::nextUnreadMessage()
{
    int i = findUnread(TRUE);
    setCurrentMsg(i);
    ensureCurrentItemVisible();
}

void KMHeaders::ensureCurrentItemVisible()
{
    int i = currentItemIndex();
    if ((i >= 0) && (i < (int)mItems.size()))
        center( contentsX(), itemPos(mItems[i]), 0, 9.0 );
}

//-----------------------------------------------------------------------------
void KMHeaders::prevUnreadMessage()
{
  int i = findUnread(FALSE);
  setCurrentMsg(i);
  ensureCurrentItemVisible();
}


//-----------------------------------------------------------------------------
void KMHeaders::slotNoDrag()
{
  mousePressed = FALSE;
}


//-----------------------------------------------------------------------------
void KMHeaders::makeHeaderVisible()
{
  if (currentItem())
    ensureItemVisible( currentItem() );
}

//-----------------------------------------------------------------------------
void KMHeaders::highlightMessage(QListViewItem* lvi, bool markitread)
{
  KMHeaderItem *item = static_cast<KMHeaderItem*>(lvi);
  if (lvi != mPrevCurrent) {
    if (mPrevCurrent)
    {
      KMMessage *prevMsg = mFolder->getMsg(mPrevCurrent->msgId());
      if (prevMsg)
      {
        if (mFolder->account()) KMImapJob::ignoreJobsForMessage(prevMsg);
        if (!prevMsg->transferInProgress())
          mFolder->unGetMsg(mPrevCurrent->msgId());
      }
    }
    mPrevCurrent = item;
  }
  if (!item)
  {
    emit selected( NULL ); return;
  }

  int idx = item->msgId();
  KMMessage *msg = mFolder->getMsg(idx);
  if (!msg || msg->transferInProgress())
  {
    emit selected( NULL );
    mPrevCurrent = NULL;
    return;
  }

  mOwner->statusMsg("");
  if (markitread && idx >= 0) setMsgRead(idx);
  mItems[idx]->irefresh();
  mItems[idx]->repaint();
  emit selected(mFolder->getMsg(idx));
  setFolderInfoStatus();
}

QString KMHeaders::formatDate( time_t time, KMDateDisplay option)
{
    switch ( option )
    {
    case FancyDate:
        return KMHeaders::fancyDate( time );
    case CTime:
        return QString::fromLatin1( ctime(  &time ) ).stripWhiteSpace() ;
    case Localized:
    {
        QDateTime tmp;
        tmp.setTime_t( time );
        return KGlobal::locale()->formatDateTime( tmp );
    }
    }
    return QString::null;
}

QDateTime *KMHeaders::now = 0;
time_t KMHeaders::now_time = 0;

QString KMHeaders::fancyDate( time_t otime )
{
    if ( otime <= 0 )
        return i18n( "unknown" );

    if ( !now ) {
        time( &now_time );
        now = new QDateTime();
        now->setTime_t( now_time );
    }

    QDateTime old;
    old.setTime_t( otime );

    // not more than an hour in the future
    if ( now_time + 60 * 60 >= otime ) {
        time_t diff = now_time - otime;

        if ( diff < 24 * 60 * 60 ) {
            if ( old.date().year() == now->date().year() &&
                 old.date().dayOfYear() == now->date().dayOfYear() )
                return i18n( "Today %1" ).arg( KGlobal::locale()->
                                               formatTime( old.time(), true ) );
        }
        if ( diff < 2 * 24 * 60 * 60 ) {
            QDateTime yesterday( now->addDays( -1 ) );
            if ( old.date().year() == yesterday.date().year() &&
                 old.date().dayOfYear() == yesterday.date().dayOfYear() )
                return i18n( "Yesterday %1" ).arg( KGlobal::locale()->
                                                   formatTime( old.time(), true) );
        }
        for ( int i = 3; i < 7; i++ )
            if ( diff < i * 24 * 60 * 60 ) {
                QDateTime weekday( now->addDays( -i + 1 ) );
                if ( old.date().year() == weekday.date().year() &&
                     old.date().dayOfYear() == weekday.date().dayOfYear() )
                    return i18n( "1. weekday, 2. time", "%1 %2" ).
                        arg( KGlobal::locale()->weekDayName( old.date().dayOfWeek() ) ).
                        arg( KGlobal::locale()->
                             formatTime( old.time(), true) );
            }
    }

    return KGlobal::locale()->
        formatDateTime( old );
}

void KMHeaders::resetCurrentTime()
{
    delete now;
    now = 0;
    now_time = 0;
    QTimer::singleShot( 1000, this, SLOT( resetCurrentTime() ) );
}

//-----------------------------------------------------------------------------
void KMHeaders::selectMessage(QListViewItem* lvi)
{
  KMHeaderItem *item = static_cast<KMHeaderItem*>(lvi);
  if (!item)
    return;

  int idx = item->msgId();
  KMMessage *msg = mFolder->getMsg(idx);
  if (!msg->transferInProgress())
  {
    emit activated(mFolder->getMsg(idx));
    if (idx >= 0) setMsgRead(idx);
  }

  if (mFolder != kernel->outboxFolder() && mFolder != kernel->draftsFolder())
    setOpen(lvi, !lvi->isOpen());
}


//-----------------------------------------------------------------------------
void KMHeaders::recursivelyAddChildren( int i, KMHeaderItem *parent )
{
  KMMsgBase* mb;
  mb = mFolder->getMsgBase( i );
  assert( mb );
  QString msgId = mb->msgIdMD5();
  if (msgId.isNull())
    msgId = "";

  mIdTree.replace( msgId, parent );
  assert( mTreeSeen[msgId] );
  if (*(mTreeSeen[msgId])) // this can happen in the pathological case of
    // multiple messages having the same id. This case, even the extra
    // pathological version where messages have the same id and different
    // reply-To-Ids, should be handled ok. Later messages with duplicate
    // ids will be shown as children of the first one in the bunch.
    return;
  mTreeSeen.replace( msgId, &mTrue );

  // iterator over items in children list (exclude parent)
  // recusively add them as children of parent
  QValueList<int> *messageList = mTree[msgId];
  assert(messageList);

  QValueList<int>::Iterator it;
  for (it = messageList->begin(); it != messageList->end(); ++it) {
    if (*it == i)
      continue;

	KMHeaderItem* hi = new KMHeaderItem( parent, *it );
    assert(mItems[*it] == 0);
    mItems.operator[](*it) = hi;
    recursivelyAddChildren( *it, hi );
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::updateMessageList(bool set_selection)
{
  mPrevCurrent = 0;
  KMHeadersInherited::setSorting( mSortCol, !mSortDescending );
  if (!mFolder) {
    clear();
    mItems.resize(0);
    repaint();
    return;
  }
#if 0
  disconnect(this,SIGNAL(currentChanged(QListViewItem*)),
  	     this,SLOT(highlightMessage(QListViewItem*)));
#endif
  readSortOrder(set_selection);
#if 0
  connect(this,SIGNAL(currentChanged(QListViewItem*)),
  	  this,SLOT(highlightMessage(QListViewItem*)));
#endif
}

int
KMHeaders::messageScore(int msgId)
{
  if (!mScoringManager) return 0;

  if ( !mPaintInfo.showScore ) {
    kdDebug() << "KMFolder::scoreMessages() : Skipping this group - no rules for it"
              << endl;
    return 0;
  }

  QCString cStr;

  folder()->getMsgString(msgId, cStr);

  if (!cStr) {
    kdDebug() << "KMFolder::scoreMessages() : Skipping msg" << endl;
    return 0;
  }

  KMScorableArticle smsg(cStr);

  mScoringManager->applyRules(smsg);

  return smsg.score();

}


//-----------------------------------------------------------------------------
// KMail Header list selection/navigation description
//
// If the selection state changes the reader window is updated to show the
// current item.
//
// (The selection state of a message or messages can be changed by pressing
//  space, or normal/shift/cntrl clicking).
//
// The following keyboard events are supported when the messages headers list
// has focus, Ctrl+Key_Down, Ctrl+Key_Up, Ctrl+Key_Home, Ctrl+Key_End,
// Ctrl+Key_Next, Ctrl+Key_Prior, these events change the current item but do
// not change the selection state.
//
// See contentsMousePressEvent below for a description of mouse selection
// behaviour.
//
// Exception: When shift selecting either with mouse or key press the reader
// window is updated regardless of whether of not the selection has changed.
void KMHeaders::keyPressEvent( QKeyEvent * e )
{
    bool cntrl = (e->state() & ControlButton );
    bool shft = (e->state() & ShiftButton );
    QListViewItem *cur = currentItem();

    if (!e || !firstChild())
      return;

    // If no current item, make some first item current when a key is pressed
    if (!cur) {
      setCurrentItem( firstChild() );
      return;
    }

    // Handle space key press
    if (cur->isSelectable() && e->ascii() == ' ' ) {
	setSelected( cur, !cur->isSelected() );
	highlightMessage( cur, true);
	return;
    }

    if (cntrl) {
      if (!shft)
	disconnect(this,SIGNAL(currentChanged(QListViewItem*)),
		   this,SLOT(highlightMessage(QListViewItem*)));
      switch (e->key()) {
      case Key_Down:
      case Key_Up:
      case Key_Home:
      case Key_End:
      case Key_Next:
      case Key_Prior:
      case Key_Escape:
	KMHeadersInherited::keyPressEvent( e );
      }
      if (!shft)
	connect(this,SIGNAL(currentChanged(QListViewItem*)),
		this,SLOT(highlightMessage(QListViewItem*)));
    }
}

//-----------------------------------------------------------------------------
// KMail mouse selection - simple description
// Normal click - select and make current just this item  unselect all others
// Shift click  - select all items from current item to clicked item
//                can be used multiple times
// Cntrl click -  select this item in addition to current selection make this
//                item the current item.
void KMHeaders::contentsMousePressEvent(QMouseEvent* e)
{
  beginSelection = currentItem();
  presspos = e->pos();
  QListViewItem *lvi = itemAt( contentsToViewport( e->pos() ));
  if (!lvi) {
    KMHeadersInherited::contentsMousePressEvent(e);
    return;
  }

  setCurrentItem( lvi );
  if ((e->button() == LeftButton) &&
      !(e->state() & ControlButton) &&
      !(e->state() & ShiftButton)) {
    mousePressed = TRUE;
    if (!(lvi->isSelected())) {
      clearSelection();
      KMHeadersInherited::contentsMousePressEvent(e);
    } else {
      KMHeadersInherited::contentsMousePressEvent(e);
      lvi->setSelected( TRUE );
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

void KMHeaders::highlightMessage(QListViewItem* i)
{
    highlightMessage( i, true );
}

//-----------------------------------------------------------------------------
void KMHeaders::clearSelectionExcept( QListViewItem *exception )
{
  QListViewItem *item;
  for (item = firstChild(); item; item = item->itemBelow())
    if (item->isSelected() && (item != exception))
      setSelected( item, FALSE );
}

//-----------------------------------------------------------------------------
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

  QPopupMenu *menu = new QPopupMenu(this);

  mMenuToFolder.clear();

  mOwner->updateMessageMenu();

  QPopupMenu *msgMoveMenu = new QPopupMenu(menu);
  mOwner->folderToPopupMenu( NULL, TRUE, this, &mMenuToFolder, msgMoveMenu );
  QPopupMenu *msgCopyMenu = new QPopupMenu(menu);
  mOwner->folderToPopupMenu( NULL, FALSE, this, &mMenuToFolder, msgCopyMenu );

  bool out_folder = (mFolder == kernel->outboxFolder()) || (mFolder == kernel->draftsFolder());
  if ( out_folder )
     mOwner->editAction->plug(menu);
  else {
     mOwner->replyAction->plug(menu);
     mOwner->replyAllAction->plug(menu);
     mOwner->forwardAction->plug(menu);
     mOwner->forwardAttachedAction->plug(menu);
     mOwner->redirectAction->plug(menu);
     mOwner->bounceAction->plug(menu);
       }
  menu->insertSeparator();

  if ( !out_folder )
      mOwner->filterMenu->plug( menu );

  menu->insertItem(i18n("&Move to"), msgMoveMenu);
  menu->insertItem(i18n("&Copy to"), msgCopyMenu);
  if ( !out_folder )
      mOwner->statusMenu->plug( menu );

  menu->insertSeparator();
  mOwner->printAction->plug(menu);
  mOwner->saveAsAction->plug(menu);
  menu->insertSeparator();
  mOwner->deleteAction->plug(menu);
  menu->exec (QCursor::pos(), 0);
  delete menu;
}

//-----------------------------------------------------------------------------
KMMessage* KMHeaders::currentMsg()
{
  KMHeaderItem *hi = currentHeaderItem();
  if (!hi)
    return 0;
  else
    return mFolder->getMsg(hi->msgId());
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
    bool unchanged = (currentItem() == mItems[msgIdx]);
    setCurrentItem( mItems[msgIdx] );
    setSelected( mItems[msgIdx], TRUE );
    if (unchanged)
       highlightMessage( mItems[msgIdx], true);
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
  if (mSortCol != mPaintInfo.dateCol)
    return;
 for( int i = 0; i < (int)mItems.size(); ++i)
   if (mFolder->getMsgBase(i)->isNew()) {
     if (!mSortDescending)
       setTopItemByIndex( currentItemIndex() );
     break;
   }
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void KMHeaders::setNestedOverride( bool override )
{
  mSortInfo.dirty = TRUE;
  mNestedOverride = override;
  setRootIsDecorated( nestingPolicy != 0 && mNested != mNestedOverride );
  QString sortFile = mFolder->indexLocation() + ".sorted";
  unlink(sortFile.local8Bit());
  reset();
}

//-----------------------------------------------------------------------------
void KMHeaders::setOpen( QListViewItem *item, bool open )
{
  if( (nestingPolicy) || open )
  KMHeadersInherited::setOpen( item, open );
}

//-----------------------------------------------------------------------------
void KMHeaders::setSorting( int column, bool ascending )
{
  if (column != -1) {
    if (column != mSortCol)
      setColumnText( mSortCol, QIconSet( QPixmap()), columnText( mSortCol ));
    if(column != mSortInfo.column || ascending != mSortInfo.ascending) { //dirtied
	QObject::disconnect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
	mSortInfo.dirty = TRUE;
    }

    mSortCol = column;
    mSortDescending = !ascending;

    if (!ascending && (column == mPaintInfo.dateCol))
      mPaintInfo.orderOfArrival = !mPaintInfo.orderOfArrival;

    if (!ascending && (column == mPaintInfo.subCol))
      mPaintInfo.status = !mPaintInfo.status;

    QString colText = i18n( "Date" );
    if (mPaintInfo.orderOfArrival)
      colText = i18n( "Date (Order of Arrival)" );
    setColumnText( mPaintInfo.dateCol, colText);

    colText = i18n( "Subject" );
    if (mPaintInfo.status)
      colText = colText + i18n( " (Status)" );
    setColumnText( mPaintInfo.subCol, colText);
  }
  KMHeadersInherited::setSorting( column, ascending );
}

//Flatten the list and write it to disk
#define KMAIL_SORT_VERSION 1001
#define KMAIL_SORT_FILE(x) x->indexLocation() + ".sorted"
#define KMAIL_SORT_HEADER "## KMail Sort V%04d\n\t"
#define KMAIL_MAGIC_HEADER_OFFSET 21 //strlen(KMAIL_SORT_HEADER)
static void internalWriteItem(FILE *sortStream, int msgid, int parent_id,
			      QString key, bool update_discover=TRUE)
{
  fwrite(&msgid, sizeof(msgid), 1, sortStream);
  fwrite(&parent_id, sizeof(parent_id), 1, sortStream);
  int len = key.length() * 2;
  fwrite(&len, sizeof(len), 1, sortStream);
  if (len)
    fwrite(key.unicode(), len, 1, sortStream);

  if (update_discover) {
    //update the discovered change count
      int discovered_count = 0;
      fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 16, SEEK_SET);
      fread(&discovered_count, sizeof(discovered_count), 1, sortStream);
      discovered_count++;
      fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 16, SEEK_SET);
      fwrite(&discovered_count, sizeof(discovered_count), 1, sortStream);
  }
}

bool KMHeaders::writeSortOrder()
{
  if (mSortInfo.removed)
    return TRUE; // Need serial ids to optimize this out
  QString sortFile = KMAIL_SORT_FILE(mFolder);

  if (!mSortInfo.dirty) {
    struct stat stat_tmp;
    if(stat(sortFile, &stat_tmp) == -1) {
	mSortInfo.dirty = TRUE;
    }
  }
  if (mSortInfo.dirty) {
    QString tempName = sortFile + ".temp";
    unlink(tempName.local8Bit());
    FILE *sortStream = fopen(tempName.local8Bit(), "w");
    if (!sortStream)
      return FALSE;
    mSortInfo.dirty = FALSE;
    fprintf(sortStream, KMAIL_SORT_HEADER, KMAIL_SORT_VERSION);
    //magic header information
    int column = mSortCol, ascending=!mSortDescending;
    int threaded = (mNested && !mNestedOverride) || (!mNested && mNestedOverride);
    int discovered_count = 0, sorted_count=0, appended=0;
    fwrite(&column, sizeof(column), 1, sortStream);
    fwrite(&ascending, sizeof(ascending), 1, sortStream);
    fwrite(&threaded, sizeof(threaded), 1, sortStream);
    fwrite(&appended, sizeof(appended), 1, sortStream);
    fwrite(&discovered_count, sizeof(discovered_count), 1, sortStream);
    fwrite(&sorted_count, sizeof(sorted_count), 1, sortStream);
	
    QStack<KMHeaderItem> items;
    {
      QStack<QListViewItem> s;
      for (QListViewItem * i = firstChild(); i; ) {
	items.push((KMHeaderItem *)i);
	if ( i->firstChild() ) {
	  s.push( i );
	  i = i->firstChild();
	} else if( i->nextSibling()) {
	  i = i->nextSibling();
	} else {
	    for(i=NULL; !i && s.count(); i = s.pop()->nextSibling());
	}
      }
    }
    KMMsgBase *kmb;
    while(KMHeaderItem *i = items.pop()) {
      kmb = mFolder->getMsgBase( i->mMsgId );

      QString replymd5 = kmb->replyToIdMD5();
      int parent_id = -2; //no parent, top level
      if(!replymd5.isEmpty()) {
	if(mIdTree.isEmpty()) {
	  QString md5;
	  for(int x = 0; x < mFolder->count(); x++) {
	    if(mItems[x]) {
	      md5 = mFolder->getMsgBase(x)->msgIdMD5();
	      if(md5.isEmpty()) continue;
	      if(mIdTree[md5])
		  ;
	      else
		  mIdTree.insert(md5, mItems[x]);
	    }
	  }
	}
	KMHeaderItem *p = mIdTree[replymd5];
	if(p)
	  parent_id = p->mMsgId;
	else
	  parent_id = -1;
      }
      internalWriteItem(sortStream, i->mMsgId, parent_id,
			i->key(mSortCol, !mSortDescending), FALSE);
      //double check for magic headers
      sorted_count++;
    }

    //magic header twice, case they've changed
    fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET, SEEK_SET);
    fwrite(&column, sizeof(column), 1, sortStream);
    fwrite(&ascending, sizeof(ascending), 1, sortStream);
    fwrite(&threaded, sizeof(threaded), 1, sortStream);
    fwrite(&appended, sizeof(appended), 1, sortStream);
    fwrite(&discovered_count, sizeof(discovered_count), 1, sortStream);
    fwrite(&sorted_count, sizeof(sorted_count), 1, sortStream);
    if (sortStream && ferror(sortStream)) {
	fclose(sortStream);
	unlink(sortFile.local8Bit());
	kdDebug() << "Error: Failure modifying " << sortFile.latin1() << " (No space left on device?)" << endl;
	kdDebug() << "Abnormally terminating to prevent data loss, now." << endl;
	kdDebug() << __FILE__ << ":" << __LINE__ << endl;
	exit(1);
    }
    fclose(sortStream);
    rename(tempName.local8Bit(), sortFile.local8Bit());
  }

  return TRUE;
}


void KMHeaders::appendUnsortedItem(KMHeaderItem *khi)
{
  QString sortFile = KMAIL_SORT_FILE(mFolder);
  if(FILE *sortStream = fopen(sortFile.local8Bit(), "r+")) {
    KMMsgBase *kmb = mFolder->getMsgBase( khi->mMsgId );
    int parent_id = -2; //no parent, top level
    if(khi->parent())
      parent_id = ((KMHeaderItem *)khi->parent())->mMsgId;
    else if(!kmb->replyToIdMD5().isEmpty())
      parent_id = -1;
    internalWriteItem(sortStream, khi->mMsgId, parent_id,
		      khi->key(mSortCol, !mSortDescending));

    //update the appended flag
    int appended = 1;
    fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 12, SEEK_SET);
    fwrite(&appended, sizeof(appended), 1, sortStream);

    if (sortStream && ferror(sortStream)) {
	fclose(sortStream);
	unlink(sortFile.local8Bit());
	kdDebug() << "Error: Failure modifying " << sortFile.latin1() << " (No space left on device?)" << endl;
	kdDebug() << "Abnormally terminating to prevent data loss, now." << endl;
	kdDebug() << __FILE__ << ":" << __LINE__ << endl;
	exit(1);
    }
    fclose(sortStream);
  } else {
    mSortInfo.dirty = TRUE;
  }
}

void KMHeaders::dirtySortOrder(int column)
{
    mSortInfo.dirty = TRUE;
    QObject::disconnect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
    setSorting(column, mSortInfo.column == column ? !mSortInfo.ascending : TRUE);
}

class KMSortCacheItem {
    KMHeaderItem *mItem;
    KMSortCacheItem *mParent;
    int mId, mSortOffset;
    QString mKey;

    QList<KMSortCacheItem> mSortedChildren;
    int mUnsortedCount, mUnsortedSize;
    KMSortCacheItem **mUnsortedChildren;

public:
    inline KMSortCacheItem() : mItem(NULL), mParent(NULL), mId(-1), mSortOffset(-1),
	mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0) { }
    inline KMSortCacheItem(int i, QString k, int o=-1)
	: mItem(NULL), mParent(NULL), mId(i), mSortOffset(o), mKey(k),
	  mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0) { }
    inline ~KMSortCacheItem() { if(mUnsortedChildren) free(mUnsortedChildren); }

    inline KMSortCacheItem *parent() const { return mParent; } //can't be set, only by the parent
    inline bool hasChildren() const
	{ return mSortedChildren.count() || mUnsortedCount; }
    inline const QList<KMSortCacheItem> *sortedChildren() const
	{ return &mSortedChildren; }
    inline KMSortCacheItem **unsortedChildren(int &count) const
	{ count = mUnsortedCount; return mUnsortedChildren; }
    void addSortedChild(KMSortCacheItem *i) {
	i->mParent = this;
	mSortedChildren.append(i);
    }
    void addUnsortedChild(KMSortCacheItem *i) {
	i->mParent = this;
	if(!mUnsortedChildren)
	    mUnsortedChildren = (KMSortCacheItem **)malloc((mUnsortedSize = 25) * sizeof(KMSortCacheItem *));
	else if(mUnsortedCount >= mUnsortedSize)
	    mUnsortedChildren = (KMSortCacheItem **)realloc(mUnsortedChildren,
							    (mUnsortedSize *= 2) * sizeof(KMSortCacheItem *));
	mUnsortedChildren[mUnsortedCount++] = i;
    }

    inline KMHeaderItem *item() const { return mItem; }
    inline void setItem(KMHeaderItem *i) { ASSERT(!mItem); mItem = i; }

    inline const QString &key() const { return mKey; }
    inline void setKey(const QString &key) { mKey = key; }

    inline int id() const { return mId; }
    inline void setId(int id) { mId = id; }

    inline int offset() const { return mSortOffset; }
    inline void setOffset(int x) { mSortOffset = x; }

    void updateSortFile(FILE *, bool =FALSE);
};

void KMSortCacheItem::updateSortFile(FILE *sortStream, bool waiting_for_parent)
{	
    if(mSortOffset == -1) {
	fseek(sortStream, 0, SEEK_END);
	mSortOffset = ftell(sortStream);
    } else {
	fseek(sortStream, mSortOffset, SEEK_SET);
    }

    int parent_id = -2;
    if(!waiting_for_parent) {
	if(mParent)
	    parent_id = mParent->id();
	else
	    parent_id = -1;
    }
    internalWriteItem(sortStream, mId, parent_id, mKey);
}

static bool compare_ascending = FALSE;
static int compare_KMSortCacheItem(const void *s1, const void *s2)
{
    if ( !s1 || !s2 )
	return 0;
    KMSortCacheItem **b1 = (KMSortCacheItem **)s1;
    KMSortCacheItem **b2 = (KMSortCacheItem **)s2;
    int ret = qstrcmp((*b1)->key(), (*b2)->key());
    if(compare_ascending)
	ret = -ret;
    return ret;
}

bool KMHeaders::readSortOrder(bool set_selection)
{
    //all cases
    int column, ascending, threaded, discovered_count, sorted_count, appended;
    bool unread_exists = false;
    QArray<KMSortCacheItem *> sortCache(mFolder->count());
    KMSortCacheItem root;
    root.setId(-666); //mark of the root!

    //threaded cases
    QList<KMSortCacheItem> unparented;
    mIdTree.clear();
    if (mIdTree.size() < 2*(unsigned)mFolder->count())
	mIdTree.resize( 2*mFolder->count() );

    //cleanup
    clear();
    mItems.resize( mFolder->count() );
    for (int i=0; i<mFolder->count(); i++) {
	sortCache[i] = 0;
	mItems[i] = 0;
    }

    QString sortFile = KMAIL_SORT_FILE(mFolder);
    FILE *sortStream = fopen(sortFile.local8Bit(), "r+");
    mSortInfo.fakeSort = 0;

    if(sortStream) {
	mSortInfo.fakeSort = 1;
	int version;
	fscanf(sortStream, KMAIL_SORT_HEADER, &version);
	if(version == KMAIL_SORT_VERSION) {
	    fread(&column, sizeof(column), 1, sortStream);
	    fread(&ascending, sizeof(ascending), 1, sortStream);
	    fread(&threaded, sizeof(threaded), 1, sortStream);
	    fread(&appended, sizeof(appended), 1, sortStream);
	    fread(&discovered_count, sizeof(discovered_count), 1, sortStream);
	    fread(&sorted_count, sizeof(sorted_count), 1, sortStream);
	
	    qDebug( "foo sorted_count %d, discovered_count %d, foler count %d", sorted_count, discovered_count, mFolder->count() );

            if (sorted_count + discovered_count > mFolder->count()) { //sanity check
		kdDebug() << "Whoa! " << __FILE__ << ":" << __LINE__ << endl;
		fclose(sortStream);
		sortStream = NULL;
	    }
	    else if(!(threaded && ((mNested && !mNestedOverride) || (!mNested && mNestedOverride))) ||
	       (threaded && !((mNested && !mNestedOverride) || (!mNested && mNestedOverride))) ||
	       sorted_count <= mFolder->count())  {

		//Hackyness to work around qlistview problems
		KMHeadersInherited::setSorting(-1);
		header()->setSortIndicator(column, ascending);
		QObject::connect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
		//setup mSortInfo here now, as above may change it
		mSortInfo.dirty = FALSE;
		mSortInfo.column = (short)column;
		mSortInfo.ascending = (compare_ascending = ascending);


		KMSortCacheItem *item;
		int id, len, parent, offset, x;
		QChar *tmp_qchar = NULL;
		int tmp_qchar_len = 0;
		QString key;

		CREATE_TIMER(parse);
		START_TIMER(parse);
		for(x = 0; !feof(sortStream); x++) {
		    offset = ftell(sortStream);
		    //parse
		    if(!fread(&id, sizeof(id), 1, sortStream) || //short read means to end
		       !fread(&parent, sizeof(parent), 1, sortStream) ||
		       !fread(&len, sizeof(len), 1, sortStream)) {
			break;
		    }
		    if(len) {
			if(len > tmp_qchar_len)
			    tmp_qchar = (QChar *)realloc(tmp_qchar, len);
			if(!fread(tmp_qchar, len, 1, sortStream))
			    break;
			key = QString(tmp_qchar, len / 2);
		    } else {
			key = QString(""); //yuck
		    }

		    if ((item=sortCache[id])) {
			item->setKey(key);
			item->setId(id);
			item->setOffset(offset);
		    } else {
			item = sortCache[id] = new KMSortCacheItem(id, key, offset);
		    }
		    if (threaded && parent != -2) {
			if(parent == -1) {
			    unparented.append(item);
			    root.addUnsortedChild(item);
			} else {
			    if( ! sortCache[parent] )
				sortCache[parent] = new KMSortCacheItem;
			    sortCache[parent]->addUnsortedChild(item);
			}
		    } else {
			if(x < sorted_count )
			    root.addSortedChild(item);
			else {
			    root.addUnsortedChild(item);
			}
		    }
		}
		if (x != sorted_count + discovered_count) {// sanity check
		    qDebug( "Whoa: x %d, sorted_count %d, discovered_count %d, count %d", x, sorted_count, discovered_count, mFolder->count() );
		    fclose(sortStream);
		    sortStream = NULL;
		}

		if(tmp_qchar)
		    free(tmp_qchar);
		END_TIMER(parse);
		SHOW_TIMER(parse);
	    } else {
		fclose(sortStream);
		sortStream = NULL;
	    }
	} else {
	    fclose(sortStream);
	    sortStream = NULL;
	}
    }

    if (!sortStream) {
	mSortInfo.dirty = TRUE;
	mSortInfo.column = column = mSortCol;
	mSortInfo.ascending = ascending = !mSortDescending;
	threaded = ((mNested && !mNestedOverride) || (!mNested && mNestedOverride));
	sorted_count = discovered_count = appended = 0;
	KMHeadersInherited::setSorting( mSortCol, !mSortDescending );
    }

    //fill in empty holes
    if((sorted_count + discovered_count) < mFolder->count()) {
	CREATE_TIMER(holes);
	START_TIMER(holes);
	KMMsgBase *msg = NULL;
	for(int x = 0; x < mFolder->count(); x++) {
	    if(!sortCache[x] && (msg=mFolder->getMsgBase(x))) {
		sortCache[x] = new KMSortCacheItem(
		    x, KMHeaderItem::generate_key(x, msg, &mPaintInfo, column));
		if(threaded)
		    unparented.append(sortCache[x]);
		else
		    root.addUnsortedChild(sortCache[x]);
		if(sortStream)
		    sortCache[x]->updateSortFile(sortStream, TRUE);
		discovered_count++;
		appended = 1;
	    }
	}
	END_TIMER(holes);
	SHOW_TIMER(holes);
    }

    //make sure we've placed everything in parent/child relationship
    if (appended && threaded && !unparented.isEmpty()) {
	CREATE_TIMER(reparent);
	START_TIMER(reparent);
	KMSortCacheItem *i;
	QDict<KMSortCacheItem> msgs(mFolder->count() * 2);
	for(int x = 0; x < mFolder->count(); x++) {
	    QString md5 = mFolder->getMsgBase(x)->msgIdMD5();
	    if(md5.isEmpty()) continue;
	    msgs.insert(md5, sortCache[x]);
	}
	for(QListIterator<KMSortCacheItem> it(unparented); it.current(); ++it) {
	    i = msgs[mFolder->getMsgBase((*it)->id())->replyToIdMD5()];
	    if(i) {
		i->addUnsortedChild((*it));
		if(sortStream)
		    (*it)->updateSortFile(sortStream);
	    } else { //oh well we tried, to the root with you!
		root.addUnsortedChild((*it));
	    }
	}
	END_TIMER(reparent);
	SHOW_TIMER(reparent);
    }

    //create headeritems
    int first_unread = -1;
    CREATE_TIMER(header_creation);
    START_TIMER(header_creation);
    KMHeaderItem *khi;
    KMSortCacheItem *i, *new_kci;
    QQueue<KMSortCacheItem> s;
    s.enqueue(&root);
    do {
	i = s.dequeue();
	const QList<KMSortCacheItem> *sorted = i->sortedChildren();
	int unsorted_count, unsorted_off=0;
	KMSortCacheItem **unsorted = i->unsortedChildren(unsorted_count);
	if(unsorted)
	    qsort(unsorted, unsorted_count, sizeof(KMSortCacheItem *), //sort
		  compare_KMSortCacheItem);
	//merge two sorted lists of siblings
	for(QListIterator<KMSortCacheItem> it(*sorted);
	    (unsorted && unsorted_off < unsorted_count) || it.current(); ) {
	    if(it.current() &&
	       (!unsorted || unsorted_off >= unsorted_count ||
		(ascending && (*it)->key() >= unsorted[unsorted_off]->key()) ||
		(!ascending && (*it)->key() < unsorted[unsorted_off]->key()))) {
		new_kci = (*it);
		++it;
	    } else {
		new_kci = unsorted[unsorted_off++];
	    }
	    if(new_kci->item() || new_kci->parent() != i) //could happen if you reparent
		continue;

	    if(threaded && i->item())
		khi = new KMHeaderItem(i->item(), new_kci->id(), new_kci->key());
	    else
		khi = new KMHeaderItem(this, new_kci->id(), new_kci->key());
	    new_kci->setItem(mItems[new_kci->id()] = khi);
	    if(new_kci->hasChildren())
		s.enqueue(new_kci);
	    if(set_selection && mFolder->getMsgBase(new_kci->id())->status() == KMMsgStatusNew)
		unread_exists = true;
	}
    } while(!s.isEmpty());

    for(int x = 0; x < mFolder->count(); x++) {	    //cleanup
	delete sortCache[x];
	sortCache[x] = NULL;
    }
    END_TIMER(header_creation);
    SHOW_TIMER(header_creation);

    if(sortStream) { //update the .sorted file now
	if( discovered_count * discovered_count > sorted_count ) {
	    mSortInfo.dirty = TRUE;
	} else {
	    //update the appended flag
	    appended = 0;
	    fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 12, SEEK_SET);
	    fwrite(&appended, sizeof(appended), 1, sortStream);
	}
    }

    //show a message
    CREATE_TIMER(selection);
    START_TIMER(selection);
    if(set_selection) {
	if (unread_exists) {
	    KMHeaderItem *item = static_cast<KMHeaderItem*>(firstChild());
	    while (item) {
		if (mFolder->getMsgBase(item->msgId())->status() == KMMsgStatusNew) {
		    first_unread = item->msgId();
		    break;
		}
		item = static_cast<KMHeaderItem*>(item->itemBelow());
	    }
	}
	
	if(first_unread == -1 ) {
	    setMsgRead(mCurrentItem);
	    setTopItemByIndex(mTopItem);
	    setCurrentItemByIndex((mCurrentItem >= 0) ? mCurrentItem : 0);
	} else {	
	    setMsgRead(first_unread);
	    setCurrentItemByIndex(first_unread);
	    makeHeaderVisible();
	    center( contentsX(), itemPos(mItems[first_unread]), 0, 9.0 );
	}
    }
    END_TIMER(selection);
    SHOW_TIMER(selection);
    if (sortStream && ferror(sortStream)) {
	fclose(sortStream);
	unlink(sortFile.local8Bit());
	kdDebug() << "Error: Failure modifying " << sortFile.latin1() << " (No space left on device?)" << endl;
	kdDebug() << "Abnormally terminating to prevent data loss, now." << endl;
	kdDebug() << __FILE__ << ":" << __LINE__ << endl;
	exit(1);
    }
    if(sortStream)
	fclose(sortStream);
    return TRUE;
}


//-----------------------------------------------------------------------------
#include "kmheaders.moc"



