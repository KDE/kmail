// kmheaders.cpp

#include <config.h>
#include <stdlib.h>
#include <kiconloader.h>
#include <qpopupmenu.h>
#include <qcursor.h>

#include <qheader.h>
#include <qptrstack.h>
#include <qptrqueue.h>
#include <qpainter.h>
#include <qtextcodec.h>
#include <qregexp.h>
#include <qbitmap.h>
#include <qheader.h>

#include <kaction.h>
#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>
#include <kimageio.h>
#include <kmessagebox.h>
#include <kfiledialog.h>


#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmheaders.h"
#include "kbusyptr.h"
#include "kmmainwin.h"
#include "kmcomposewin.h"
#include "kmfiltermgr.h"
#include "kmsender.h"
#include "kmundostack.h"
#include "kmmsgdict.h"
#ifdef SCORING
#include "kmscoring.h"
#endif
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
QPixmap* KMHeaders::pixFullySigned = 0;
QPixmap* KMHeaders::pixPartiallySigned = 0;
QPixmap* KMHeaders::pixFullyEncrypted = 0;
QPixmap* KMHeaders::pixPartiallyEncrypted = 0;
QPixmap* KMHeaders::pixFiller = 0;
QPixmap* KMHeaders::pixUndefined = 0;
QPixmap* KMHeaders::pixEncryptionProblematic = 0;
QPixmap* KMHeaders::pixSignatureProblematic = 0;
QIconSet* KMHeaders::up = 0;
QIconSet* KMHeaders::down = 0;
bool KMHeaders::mTrue = true;
bool KMHeaders::mFalse = false;

//-----------------------------------------------------------------------------
// KMHeaderItem method definitions

class KMHeaderItem : public KListViewItem
{

public:
  int mMsgId;
    QString mKey;
  // WARNING: Do not add new member variables to the class

  // Constuction a new list view item with the given colors and pixmap
    KMHeaderItem( QListView* parent, int msgId, QString key = QString::null )
    : KListViewItem( parent ),
	  mMsgId( msgId ),
	  mKey(key)
  {
    irefresh();
  }

  // Constuction a new list view item with the given parent, colors, & pixmap
    KMHeaderItem( QListViewItem* parent, int msgId, QString key = QString::null )
    : KListViewItem( parent ),
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
	  mMsgBase->status() == KMMsgStatusUnread ||
          mMsgBase->status() == KMMsgStatusFlag)
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
      KListViewItem::setOpen( true );
    } else
	KListViewItem::setOpen( false );
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
			if (qstricmp(headers->folder()->whoField(), "To")==0)
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
	tmp = headers->mDate.dateString( mMsgBase->date() );
    } else if(col == headers->paintInfo()->sizeCol
      && headers->paintInfo()->showSize) {
        if (headers->folder()->protocol() == "imap")
        {
          QCString cstr;
          headers->folder()->getMsgString(mMsgId, cstr);
          int a = cstr.find("\nX-Length: ");
          int b = cstr.find("\n", a+1);
          tmp = KIO::convertSize(strtoul(cstr.mid(a+11, b-a-11).data(), 0, 10));
        } else tmp = KIO::convertSize(mMsgBase->msgSize());

#ifdef SCORING
    } else if(col == headers->paintInfo()->scoreCol) {
      tmp.setNum(headers->messageScore(mMsgId));
#endif
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

  typedef QValueList<QPixmap> PixmapList;

  QPixmap pixmapMerge( PixmapList pixmaps ) const {
      int width = 0;
      int height = 0;
      for ( PixmapList::ConstIterator it = pixmaps.begin();
            it != pixmaps.end(); ++it ) {
          width += (*it).width();
          height = QMAX( height, (*it).height() );
      }

      QPixmap res( width, height );
      QBitmap mask( width, height );

      int x = 0;
      for ( PixmapList::ConstIterator it = pixmaps.begin();
          it != pixmaps.end(); ++it ) {
          bitBlt( &res, x, 0, &(*it) );
          bitBlt( &mask, x, 0, (*it).mask() );
          x += (*it).width();
      }

      res.setMask( mask );
      return res;
  }


  const QPixmap * pixmap( int col) const
  {
    if(!col) {
      KMHeaders *headers = static_cast<KMHeaders*>(listView());
      KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );

      PixmapList pixmaps;

      switch (mMsgBase->status())
      {
        case KMMsgStatusNew:
            pixmaps << *KMHeaders::pixNew;
            break;
        case KMMsgStatusUnread:
            pixmaps << *KMHeaders::pixUns;
            break;
        case KMMsgStatusDeleted:
            pixmaps << *KMHeaders::pixDel;
            break;
        case KMMsgStatusReplied:
            pixmaps << *KMHeaders::pixRep;
            break;
        case KMMsgStatusForwarded:
            pixmaps << *KMHeaders::pixFwd;
            break;
        case KMMsgStatusQueued:
            pixmaps << *KMHeaders::pixQueued;
            break;
        case KMMsgStatusSent:
            pixmaps <<  *KMHeaders::pixSent;
            break;
        case KMMsgStatusFlag:
            pixmaps << *KMHeaders::pixFlag;
            break;
        default:
            pixmaps << *KMHeaders::pixOld;
            break;
      }

      // Only merge the crypto icons in if that is configured.
      if( headers->paintInfo()->showCryptoIcons ) {
          if( mMsgBase->encryptionState() == KMMsgFullyEncrypted )
              pixmaps << *KMHeaders::pixFullyEncrypted;
          else if( mMsgBase->encryptionState() == KMMsgPartiallyEncrypted )
              pixmaps << *KMHeaders::pixPartiallyEncrypted;
          else if( mMsgBase->encryptionState() == KMMsgEncryptionStateUnknown )
              pixmaps << *KMHeaders::pixUndefined;
          else if( mMsgBase->encryptionState() == KMMsgEncryptionProblematic )
              pixmaps << *KMHeaders::pixEncryptionProblematic;
          else
              pixmaps << *KMHeaders::pixFiller;

          if( mMsgBase->signatureState() == KMMsgFullySigned )
              pixmaps << *KMHeaders::pixFullySigned;
          else if( mMsgBase->signatureState() == KMMsgPartiallySigned )
              pixmaps << *KMHeaders::pixPartiallySigned;
          else if( mMsgBase->signatureState() == KMMsgSignatureStateUnknown )
              pixmaps << *KMHeaders::pixUndefined;
          else if( mMsgBase->signatureState() == KMMsgSignatureProblematic )
              pixmaps << *KMHeaders::pixSignatureProblematic;
          else
              pixmaps << *KMHeaders::pixFiller;
      }

      static QPixmap mergedpix;
      mergedpix = pixmapMerge( pixmaps );
      return &mergedpix;
    }
    return NULL;
  }

  void paintCell( QPainter * p, const QColorGroup & cg,
				int column, int width, int align )
  {
    KMHeaders *headers = static_cast<KMHeaders*>(listView());
    if (headers->noRepaint) return;
    if (!headers->folder()) return;
    QColorGroup _cg( cg );
    QColor c = _cg.text();
    QColor *color;

    KMMsgBase *mMsgBase = headers->folder()->getMsgBase( mMsgId );
    if (!mMsgBase) return;
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

    if( column == headers->paintInfo()->dateCol )
      p->setFont(headers->dateFont);

    KListViewItem::paintCell( p, _cg, column, width, align );

    _cg.setColor( QColorGroup::Text, c );
  }

  static QString generate_key( int id, KMHeaders *headers, KMMsgBase *msg, const KPaintInfo *paintInfo, int sortOrder)
  {
    // It appears, that QListView in Qt-3.0 asks for the key
    // in QListView::clear(), which is called from
    // readSortOrder()
    if (!msg) return QString::null;

    int column = sortOrder & ((1 << 5) - 1);
    QString ret = QChar( (char)sortOrder );
    QString sortArrival = QString( "%1" )
      .arg( kernel->msgDict()->getMsgSerNum(headers->folder(), id), 0, 36 );
    while (sortArrival.length() < 7) sortArrival = "0" + sortArrival;

    if (column == paintInfo->dateCol) {
      if (paintInfo->orderOfArrival)
        return ret + sortArrival;
      else {
        static const int dateLength = 30;
        char cDate[dateLength + 1];
        const time_t date = msg->date();
        strftime( cDate, dateLength, "%Y:%j:%H:%M:%S", gmtime( &date ));
        return ret + cDate + sortArrival;
      }
    } else if (column == paintInfo->senderCol) {
      QString tmp;
      if (qstricmp(headers->folder()->whoField(), "To")==0)
        tmp = msg->toStrip();
      else
        tmp = msg->fromStrip();
      return ret + tmp.lower() + " " + sortArrival;
    } else if (column == paintInfo->subCol) {
      if (paintInfo->status)
        return ret + QString( QChar( (uint)msg->status() )) + sortArrival;
      return ret + KMMsgBase::skipKeyword( msg->subject().lower() ) + " " + sortArrival;
    }
    else if (column == paintInfo->sizeCol) {
      QString len;
      if (headers->folder()->protocol() == "imap")
      {
        QCString cstr;
        headers->folder()->getMsgString(id, cstr);
        int a = cstr.find("\nX-Length: ");
        int b = cstr.find("\n", a+1);
        len = QString( "%1" ).arg( cstr.mid(a+11, b-a-11) );
      } else {
        len = QString( "%1" ).arg( msg->msgSize() );
      }
      while (len.length() < 9) len = "0" + len;
      return ret + len;
    }
    return ret + "missing key"; //you forgot something!!
  }

  virtual QString key( int column, bool /*ascending*/ ) const
  {
    KMHeaders *headers = static_cast<KMHeaders*>(listView());
    int sortOrder = column;
    if (headers->mPaintInfo.orderOfArrival)
      sortOrder |= (1 << 6);
    if (headers->mPaintInfo.status)
      sortOrder |= (1 << 5);
    //This code should stay pretty much like this, if you are adding new
    //columns put them in generate_key
    if(mKey.isEmpty() || mKey[0] != (char)sortOrder) {
      KMHeaders *headers = static_cast<KMHeaders*>(listView());
      return ((KMHeaderItem *)this)->mKey =
	generate_key(mMsgId, headers, headers->folder()->getMsgBase( mMsgId ),
		     headers->paintInfo(), sortOrder);
    }
    return mKey;
  }

  // Sorting by date doesn't really need to be locale aware
  virtual int compare( QListViewItem * i, int col, bool ascending ) const
  {
    KMHeaders *headers = static_cast<KMHeaders*>(listView());
    if (col == headers->paintInfo()->dateCol)
    {
      KMHeaderItem *hi = static_cast<KMHeaderItem*>(i);
      if (headers->paintInfo()->orderOfArrival)
      {
        int a = kernel->msgDict()->getMsgSerNum(headers->folder(), mMsgId);
        int b = kernel->msgDict()->getMsgSerNum(headers->folder(), hi->mMsgId);
        if (a < b) return -1;
        if (a == b) return 0;
        return 1;
      } else {
        return key( col, ascending ).compare( i->key(col, ascending) );
      }
    } else {
      return KListViewItem::compare(i, col, ascending);
    }
  }

  QListViewItem* firstChildNonConst() /* Non const! */ {
    enforceSortOrder(); // Try not to rely on QListView implementation details
    return firstChild();
  }
};

//-----------------------------------------------------------------------------
KMHeaders::KMHeaders(KMMainWin *aOwner, QWidget *parent,
		     const char *name) :
  KMHeadersInherited(parent, name)
#ifdef SCORING
  , mScoringManager(KMScoringManager::globalScoringManager())
#endif
{
    static bool pixmapsLoaded = FALSE;
  //qInitImageIO();
  KImageIO::registerFormats();
  mOwner  = aOwner;
  mFolder = NULL;
  noRepaint = FALSE;
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
  mJumpToUnread = false;
  setLineWidth(0);
  // popup-menu
  header()->setClickEnabled(true);
  header()->installEventFilter(this);
  mPopup = new KPopupMenu;
  mPopup->insertTitle(i18n("Select columns"));
  mPopup->setCheckable(true);
  mSizeColumn = mPopup->insertItem(i18n("Size Column"), this, SLOT(slotToggleSizeColumn()));

#ifdef SCORING
  if (!mScoringManager) {
    kdDebug(5006) << "KMHeaders::KMHeaders() : no globalScoringManager"
              << endl;
  }
#endif

  readConfig();

  mPaintInfo.flagCol = -1;
  mPaintInfo.subCol    = mPaintInfo.flagCol   + 1;
  mPaintInfo.senderCol = mPaintInfo.subCol    + 1;
  mPaintInfo.dateCol   = mPaintInfo.senderCol + 1;
  mPaintInfo.sizeCol   = mPaintInfo.dateCol   + 1;
#ifdef SCORING
  mPaintInfo.scoreCol  = mPaintInfo.sizeCol   + 0;
  mPaintInfo.showScore = false;
#endif
  mPaintInfo.orderOfArrival = false;
  mPaintInfo.status = false;
#ifdef SCORING
  showingScore = false;
#endif
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
    pixFullySigned = new QPixmap( UserIcon( "kmmsgfullysigned" ) );
    pixPartiallySigned = new QPixmap( UserIcon( "kmmsgpartiallysigned" ) );
    pixFullyEncrypted = new QPixmap( UserIcon( "kmmsgfullyencrypted" ) );
    pixPartiallyEncrypted = new QPixmap( UserIcon( "kmmsgpartiallyencrypted" ) );
    pixFiller = new QPixmap( UserIcon( "kmmsgfiller" ) );
    pixUndefined = new QPixmap( UserIcon( "kmundefined" ) );
    pixEncryptionProblematic = new QPixmap( UserIcon( "kmmsgencryptionproblematic" ) );
    pixSignatureProblematic = new QPixmap( UserIcon( "kmmsgsignatureproblematic" ) );
    up = new QIconSet( UserIcon("abup" ), QIconSet::Small );
    down = new QIconSet( UserIcon("abdown" ), QIconSet::Small );
  }

  connect( this, SIGNAL( contextMenuRequested( QListViewItem*, const QPoint &, int )),
	   this, SLOT( rightButtonPressed( QListViewItem*, const QPoint &, int )));
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
  writeConfig();
}

//-----------------------------------------------------------------------------
bool KMHeaders::eventFilter ( QObject *o, QEvent *e )
{
  if ( e->type() == QEvent::MouseButtonPress &&
      static_cast<QMouseEvent*>(e)->button() == RightButton &&
      o->isA("QHeader") )
  {
    mPopup->popup( static_cast<QMouseEvent*>(e)->globalPos() );
    return true;
  }
  return KListView::eventFilter(o, e);
}

//-----------------------------------------------------------------------------
void KMHeaders::slotToggleSizeColumn ()
{
  mPaintInfo.showSize = !mPaintInfo.showSize;
  mPopup->setItemChecked(mSizeColumn, mPaintInfo.showSize); 

  // we need to write it back so that 
  // the configure-dialog knows the correct status
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, "General");
  config->writeEntry("showMessageSize", mPaintInfo.showSize);

  setFolder(mFolder);
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
  }
  return KMHeadersInherited::event(e);
}


//-----------------------------------------------------------------------------
void KMHeaders::readColorConfig (void)
{
  KConfig* config = kapp->config();
  // Custom/System colors
  KConfigGroupSaver saver(config, "Reader");
  QColor c1=QColor(kapp->palette().active().text());
  QColor c2=QColor("red");
  QColor c3=QColor("blue");
  QColor c4=QColor(kapp->palette().active().base());
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
    mPopup->setItemChecked(mSizeColumn, mPaintInfo.showSize); 
    mPaintInfo.showCryptoIcons = config->readBoolEntry( "showCryptoIcons", false );

    KMime::DateFormatter::FormatType t =
      (KMime::DateFormatter::FormatType) config->readNumEntry("dateFormat", KMime::DateFormatter::Fancy ) ;
    mDate.setCustomFormat( config->readEntry("customDateFormat", QString::null ) );
    mDate.setFormat( t );
  }

  readColorConfig();

  // Custom/System fonts
  { // area for config group "General"
    KConfigGroupSaver saver(config, "Fonts");
    if (!(config->readBoolEntry("defaultFonts",TRUE)))
    {
      QFont listFont = QFont("helvetica");
      setFont(config->readFontEntry("list-font", &listFont));
      dateFont = QFont("courier");
      dateFont = config->readFontEntry("list-date-font", &dateFont);
    } else {
      dateFont = KGlobalSettings::generalFont();
      setFont(dateFont);
    }
  }

  // Behavior
  {
    KConfigGroupSaver saver(config, "Behaviour");
    mLoopOnGotoUnread = config->readBoolEntry( "LoopOnGotoUnread", true );
    mJumpToUnread = config->readBoolEntry( "JumpToUnread", false );
  }

  restoreLayout(config, "Header-Geometry");
}


//-----------------------------------------------------------------------------
void KMHeaders::reset(void)
{
  int top = topItemIndex();
  int id = currentItemIndex();
  noRepaint = TRUE;
  clear();
  noRepaint = FALSE;
  mItems.resize(0);
  updateMessageList();
  setCurrentMsg(id);
  setTopItemByIndex(top);
}

//-----------------------------------------------------------------------------
void KMHeaders::refreshNestedState(void)
{
  bool oldState = mNested != mNestedOverride;
  int oldNestPolicy = nestingPolicy;
  KConfig* config = kapp->config();
  KConfigGroupSaver saver(config, "Geometry");
  mNested = config->readBoolEntry( "nestedMessages", FALSE );

  nestingPolicy = config->readNumEntry( "nestingPolicy", 3 );
  if ((nestingPolicy != oldNestPolicy) ||
    (oldState != (mNested != mNestedOverride)))
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
  config->writeEntry("SortColumn", (mSortDescending ? -mSortColAdj : mSortColAdj));
  config->writeEntry("Top", topItemIndex());
  config->writeEntry("Current", currentItemIndex());
  config->writeEntry("OrderOfArrival", mPaintInfo.orderOfArrival);
  config->writeEntry("Status", mPaintInfo.status);
}

//-----------------------------------------------------------------------------
void KMHeaders::writeConfig (void)
{
  saveLayout(kapp->config(), "Header-Geometry");
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
    writeFolderConfig();
    readFolderConfig();
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
      // System folders remain open but we also should write the index from
      // time to time
      if (mFolder->dirty()) mFolder->writeIndex();
    }

    mSortInfo.removed = 0;
    mFolder = aFolder;
    mSortInfo.dirty = TRUE;
    mOwner->editAction->setEnabled(mFolder ?  (kernel->folderIsDraftOrOutbox(mFolder)): false );
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
      if (mNested != mNestedOverride) {
        noRepaint = TRUE;
        clear();
        noRepaint = FALSE;
	mItems.resize( 0 );
      }

#ifdef SCORING
      if (mScoringManager)
        mScoringManager->initCache((mFolder) ? mFolder->name() : QString());

      mPaintInfo.showScore = mScoringManager->hasRulesForCurrentGroup();
#endif
      readFolderConfig();

      CREATE_TIMER(kmfolder_open);
      START_TIMER(kmfolder_open);
      mFolder->open();
      END_TIMER(kmfolder_open);
      SHOW_TIMER(kmfolder_open);

      if (mNested != mNestedOverride) {
        noRepaint = TRUE;
	clear();
        noRepaint = FALSE;
	mItems.resize( 0 );
      }
    }

#ifdef SCORING
    if (mScoringManager)
      mScoringManager->initCache((mFolder) ? mFolder->name() : QString());
#endif
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
    if (mPaintInfo.showSize) {
      colText = i18n( "Size" );
      if (showingSize) {
        setColumnText( mPaintInfo.sizeCol, colText);
      } else {
        // add in the size field
        addColumn(colText);

	setColumnAlignment( mPaintInfo.sizeCol, AlignRight );
#ifdef SCORING
        mPaintInfo.scoreCol++;
#endif
      }
      showingSize = true;
    } else {
      if (showingSize) {
        // remove the size field
        removeColumn(mPaintInfo.sizeCol);
#ifdef SCORING
        mPaintInfo.scoreCol--;
#endif
      }
      showingSize = false;
    }

#ifdef SCORING
    mPaintInfo.showScore = mScoringManager->hasRulesForCurrentGroup();
    if (mPaintInfo.showScore) {
      colText = i18n( "Score" );
      if (showingScore) {
        setColumnText( mPaintInfo.scoreCol, colText);
      } else {
        mPaintInfo.scoreCol = addColumn(colText);
        setColumnAlignment( mPaintInfo.scoreCol, AlignRight );
      }
      showingScore = true;
    } else {
      if (showingScore) {
        removeColumn(mPaintInfo.scoreCol);
        showingScore = false;
      }
    }
#endif
  }
   kdDebug(5006) << "end " << (QTime::currentTime().second()*1000)+QTime::currentTime().msec() <<  k_funcinfo << endl;
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
  emit maybeDeleting();
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

  if (mNested != mNestedOverride) {
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
    // ### workaround the fact that Qt 3.0.1's QListView doesn't emit
    // currentChanged() on setCurrentItem():
    /*own slot*/highlightMessage( firstChild() );
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
  QValueList<int> ids;
  for (qitem = firstChild(); qitem; qitem = qitem->itemBelow())
    if (qitem->isSelected()) {
      KMHeaderItem *item = static_cast<KMHeaderItem*>(qitem);
      if (mFolder->protocol() == "imap")
        ids.append(item->msgId());
      else
        mFolder->setStatus(item->msgId(), status);
    }

  if ( !ids.empty() )
    mFolder->setStatus(ids, status);
}


QPtrList<QListViewItem> KMHeaders::currentThread() const
{
  if (!mFolder) return QPtrList<QListViewItem>();

  // starting with the current item...
  QListViewItem *curItem = currentItem();
  if (!curItem) return QPtrList<QListViewItem>();

  // ...find the top-level item:
  QListViewItem *topOfThread = curItem;
  while ( topOfThread->parent() )
    topOfThread = topOfThread->parent();

  // collect the items in this thread:
  QPtrList<QListViewItem> list;
  QListViewItem *topOfNextThread = topOfThread->nextSibling();
  for ( QListViewItemIterator it( topOfThread ) ;
	it.current() && it.current() != topOfNextThread ; ++it )
    list.append( it.current() );
  return list;
}

void KMHeaders::setThreadStatus(KMMsgStatus status)
{
  QPtrList<QListViewItem> curThread = currentThread();
  QPtrListIterator<QListViewItem> it( curThread );

  QValueList<int> ids;
  for ( it.toFirst() ; it.current() ; ++it )
    ids << static_cast<KMHeaderItem*>(*it)->msgId();
  if ( !ids.isEmpty() )
    mFolder->setStatus( ids, status );
}

//-----------------------------------------------------------------------------
int KMHeaders::slotFilterMsg(KMMessage *msg)
{
  int filterResult = kernel->filterMgr()->process(msg,KMFilterMgr::Explicit);
  if (filterResult == 2) {
    // something went horribly wrong (out of space?)
    perror("Critical error: Unable to process messages (out of space?)");
    KMessageBox::information(0,
  	i18n("Critical error: Unable to process messages (out of space?)"));
    return 2;
  }
  if (msg->parent()) { // unGet this msg
    int idx = -1;
    KMFolder * p = 0;
    kernel->msgDict()->getLocation( msg, &p, &idx );
    //    if ( p != msg->parent() ) {
    //      kdDebug() << p << " != " << msg->parent() << " for message "
    //		<< msg->subject() << ", " << msg->fromStrip() << endl;
    //
    //    }
    assert( p == msg->parent() ); assert( idx >= 0 );
    p->unGetMsg( idx );
  }
  return filterResult;
}


void KMHeaders::slotExpandOrCollapseThread( bool expand )
{
  if ( !isThreaded() ) return;
  // find top-level parent of currentItem().
  QListViewItem *item = currentItem();
  if ( !item ) return;
  while ( item->parent() )
    item = item->parent();
  KMHeaderItem * hdrItem = static_cast<KMHeaderItem*>(item);
  hdrItem->setOpen( expand );
  if ( !expand ) // collapse can hide the current item:
    setCurrentMsg( hdrItem->msgId() );
  ensureItemVisible( currentItem() );
}

void KMHeaders::slotExpandOrCollapseAllThreads( bool expand )
{
  if ( !isThreaded() ) return;
  for ( QListViewItem *item = firstChild() ;
	item ; item = item->nextSibling() )
    static_cast<KMHeaderItem*>(item)->setOpen( expand );
  if ( !expand ) { // collapse can hide the current item:
    QListViewItem * item = currentItem();
    while ( item->parent() )
      item = item->parent();
    setCurrentMsg( static_cast<KMHeaderItem*>(item)->msgId() );
  }
  ensureItemVisible( currentItem() );
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
    if ((mFolder->protocol() == "imap") && !msg->isComplete())
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
  kernel->filterMgr()->cleanup();

  setContentsPos( topX, topY );
  emit selected( 0 );
  if (next) {
    setCurrentItem( next );
    setSelected( next, TRUE );
    highlightMessage( next, true);
  }
  else if (currentItem()) {
    setSelected( currentItem(), TRUE );
    highlightMessage( currentItem(), true);
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
        mFolder->setStatus(msg, KMMsgStatusOld);
    }
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg (int msgId)
{
  KMFolder* folder = NULL;
  
  //make sure we have an associated folder (root of folder tree does not).  
  if (!mFolder)
    return;
  
  if (mFolder->protocol() == "imap")
  {
    KMFolderImap* fi = static_cast<KMFolderImap*> (mFolder);
    KMFolder* trash = kernel->imapFolderMgr()->findIdString(fi->account()->trash());
    if (!trash) trash = kernel->trashFolder();
    if (mFolder != trash)
    {
      folder = trash;
    }

  } else {

    if (mFolder != kernel->trashFolder())
    {
      // move to trash folder
      folder = kernel->trashFolder();
    }
  }

  // move messages
  moveMsgToFolder(folder, msgId);

  mSortInfo.dirty = TRUE;
  mOwner->statusMsg("");
  //  triggerUpdate();
}


//-----------------------------------------------------------------------------
void KMHeaders::saveMsg (int msgId, QPtrList<KMMessage>* msgList)
{
  KMMessage* msg;
  QCString str;
  if (!msgList)
    msgList = selectedMessages();
  QString subject;
  if (msgList->count() == 1) subject = msgList->first()->subject();
  while (subject.find(':') != -1)
    subject = subject.mid(subject.find(':') + 1).stripWhiteSpace();
  subject.replace(QRegExp(QChar(QDir::separator())), "_");
  KURL url = KFileDialog::getSaveURL(subject, QString::null);

  if( url.isEmpty() )
    return;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    str += "From " + msg->fromEmail() + " " + msg->dateShortStr() + "\n";
    str += msg->asString();
    str += "\n";
  }

  QByteArray ba = str;
  ba.resize(ba.size() - 1);
  kernel->byteArrayToRemoteFile(ba, url);
}


//-----------------------------------------------------------------------------
void KMHeaders::resendMsg ()
{
  KMComposeWin *win;
  KMMessage *newMsg, *msg = currentMsg();
  if (!msg || !msg->codec()) return;

  kernel->kbp()->busy();
  newMsg = new KMMessage;
  newMsg->fromString(msg->asString());
  newMsg->initHeader();
  newMsg->setCharset(msg->codec()->mimeName());
  newMsg->setTo(msg->to());
  newMsg->setSubject(msg->subject());

  win = new KMComposeWin(&mOwner->mCryptPlugList);
  win->setMsg(newMsg, FALSE);
  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::bounceMsg (KMMessage* msg)
{
  KMMessage *newMsg; 
  if (!msg)
   msg = currentMsg();

  if (!msg) return;

  newMsg = msg->createBounce( TRUE /* with UI */);
  if (newMsg)
    kernel->msgSender()->send(newMsg, kernel->msgSender()->sendImmediate());
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardMsg (QPtrList<KMMessage>* msgList)
{
  KMComposeWin *win;
  if (!msgList)
    msgList = selectedMessages();

  if (msgList->count() >= 2) {
    // ask if they want a mime digest forward

    if (KMessageBox::questionYesNo(this, i18n("Forward selected messages as"
                                              " a MIME digest?"))
                                                      == KMessageBox::Yes) {
      // we default to the first identity to save prompting the user
      // (the messages could have different identities)
      uint id = 0;
      KMMessage *fwdMsg = new KMMessage;
      KMMessagePart *msgPart = new KMMessagePart;
      QString msgPartText;
      int msgCnt = 0; // incase there are some we can't forward for some reason

      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->mMsg->Headers().ContentType().CreateBoundary(1);
      msgPartText = i18n("\nThis is a MIME digest forward. The content of the"
                         " message is contained in the attachment(s).\n\n\n"
                         "--\n");
      kdDebug(5006) << "Doing a mime digest forward\n" << endl;
      // iterate through all the messages to be forwarded
      for (KMMsgBase *mb = msgList->first(); mb; mb = msgList->next()) {
        int idx = mFolder->find(mb);
        if (idx < 0) continue;
        KMMessage *thisMsg = mFolder->getMsg(idx);
        if (!thisMsg) continue;
        // set the identity
        if (id == 0)
          id = thisMsg->headerField("X-KMail-Identity").stripWhiteSpace().toUInt();
        // set the part header
        msgPartText += "--";
        msgPartText += fwdMsg->mMsg->Headers().ContentType().Boundary().c_str();
        msgPartText += "\nContent-Type: MESSAGE/RFC822";
        msgPartText += QString("; CHARSET=%1").arg(thisMsg->charset());
        msgPartText += "\n";
        kdDebug(5006) << "Adding message ID " << thisMsg->id() << "\n" << endl;
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
        fwdMsg->link(thisMsg, KMMsgStatusForwarded);
      }
      kdDebug(5006) << "Done adding messages to the digest\n" << endl;
      QCString tmp;
      msgPart->setTypeStr("MULTIPART");
      tmp.sprintf( "Digest; boundary=\"%s\"",
		   fwdMsg->mMsg->Headers().ContentType().Boundary().c_str() );
      msgPart->setSubtypeStr( tmp );
      msgPart->setName("unnamed");
      msgPart->setCte(DwMime::kCte7bit);   // does it have to be 7bit?
      msgPart->setContentDescription(QString("Digest of %1 messages.").arg(msgCnt));
      // THIS HAS TO BE AFTER setCte()!!!!
      msgPart->setBodyEncoded(QCString(msgPartText.ascii()));
      kdDebug(5006) << "Launching composer window\n" << endl;
      kernel->kbp()->busy();
      win = new KMComposeWin(&mOwner->mCryptPlugList, fwdMsg, id);
      win->addAttach(msgPart);
      win->show();
      kernel->kbp()->idle();
      return;
    } else {            // NO MIME DIGEST, Multiple forward
      uint id = 0;
      QCString msgText = "";
      KMMessageList linklist;
      for (KMMsgBase *mb = msgList->first(); mb; mb = msgList->next()) {
        int idx = mFolder->find(mb);
        if (idx < 0) continue;
        KMMessage *thisMsg = mFolder->getMsg(idx);
        if (!thisMsg) continue;

        // set the identity
        if (id == 0)
          id = thisMsg->headerField("X-KMail-Identity").stripWhiteSpace().toUInt();

        msgText += thisMsg->createForwardBody();
        linklist.append(thisMsg);
      }

      KMMessage *fwdMsg = new KMMessage;
      fwdMsg->initHeader(id);
      fwdMsg->setAutomaticFields(true);
      fwdMsg->setCharset("utf-8");
      fwdMsg->setBody(msgText);

      for (KMMsgBase *mb = linklist.first(); mb; mb = linklist.next()) {
        KMMessage *thisMsg = static_cast<KMMessage *>(mb);
        fwdMsg->link(thisMsg, KMMsgStatusForwarded);
      }

      kernel->kbp()->busy();
      win = new KMComposeWin(&mOwner->mCryptPlugList, fwdMsg, id);
      win->setCharset("");
      win->show();
      kernel->kbp()->idle();
      return;
    }
  }

  // forward a single message at most.

  KMMessage *msg = msgList->getFirst();
  if (!msg || !msg->codec()) return;

  kernel->kbp()->busy();
  win = new KMComposeWin(&mOwner->mCryptPlugList, msg->createForward());
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardAttachedMsg (QPtrList<KMMessage>* msgList)
{
  KMComposeWin *win;
  if (!msgList)
    msgList = selectedMessages();
  uint id = 0;
  KMMessage *fwdMsg = new KMMessage;

  if (msgList->count() >= 2) {
    // don't respect X-KMail-Identity headers because they might differ for
    // the selected mails
    id = mFolder->identity();
    fwdMsg->initHeader(id);
  }
  else if (msgList->count() == 1) {
    KMMessage *msg = msgList->getFirst();
    fwdMsg->initFromMessage(msg);
  }

  fwdMsg->setAutomaticFields(true);

  kdDebug(5006) << "Launching composer window\n" << endl;
  kernel->kbp()->busy();
  win = new KMComposeWin(&mOwner->mCryptPlugList, fwdMsg, id);

  kdDebug(5006) << "Doing forward as attachment" << endl;
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
    msgPart->setBodyEncoded(thisMsg->asString());
    msgPart->setCharset("");

    fwdMsg->link(thisMsg, KMMsgStatusForwarded);
    win->addAttach(msgPart);
  }

  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::redirectMsg(KMMessage* msg)
{
  KMComposeWin *win;
  if (!msg)
   msg = currentMsg();

  if (!msg || !msg->codec()) return;

  kernel->kbp()->busy();
  win = new KMComposeWin(&mOwner->mCryptPlugList);
  win->setMsg(msg->createRedirect(), FALSE);
  win->setCharset(msg->codec()->mimeName());
  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::noQuoteReplyToMsg(KMMessage* msg)
{
  KMComposeWin *win;
  if (!msg)
    msg = currentMsg();
  QString id;

  if (!msg || !msg->codec())
    return;

  kernel->kbp()->busy();
  win = new KMComposeWin(&mOwner->mCryptPlugList, msg->createReply(FALSE, FALSE, "", TRUE));
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus(false);
  win->show();
  kernel->kbp()->idle();
}

//-----------------------------------------------------------------------------
void KMHeaders::replyToMsg (QString selection, KMMessage *msg)
{
  KMComposeWin *win;
  if (!msg)
     msg = currentMsg();
  QString id;

  if (!msg || !msg->codec())
    return;

  kernel->kbp()->busy();
  win = new KMComposeWin(&mOwner->mCryptPlugList, msg->createReply(FALSE, FALSE, selection));
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus();
  win->show();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::replyAllToMsg (QString selection, KMMessage* msg)
{
  KMComposeWin *win;
  if (!msg)
   msg = currentMsg();
  QString id;

  if (!msg || !msg->codec()) return;

  kernel->kbp()->busy();
  win = new KMComposeWin(&mOwner->mCryptPlugList, msg->createReply(TRUE, FALSE, selection));
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus();
  win->show();
  kernel->kbp()->idle();
}

//-----------------------------------------------------------------------------
void KMHeaders::replyListToMsg (QString selection, KMMessage* msg)
{
  KMComposeWin *win;
  if (!msg)
   msg = currentMsg();
  QString id;

  if (!msg || !msg->codec()) return;

  kernel->kbp()->busy();
  win = new KMComposeWin(&mOwner->mCryptPlugList, msg->createReply(true, true, selection));
  win->setCharset(msg->codec()->mimeName(), TRUE);
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
  int rc;

  emit maybeDeleting();

  disconnect(this,SIGNAL(currentChanged(QListViewItem*)),
	     this,SLOT(highlightMessage(QListViewItem*)));
  kernel->kbp()->busy();

  if (destFolder && destFolder->open() != 0)
  {
      kernel->kbp()->idle();
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
  QPtrList<KMMessage> list;
  for (rc=0, msgBase=msgList->first(); msgBase && !rc; msgBase=msgList->next())
  {
    int idx = mFolder->find(msgBase);
    assert(idx != -1);
    msg = mFolder->getMsg(idx);
    if (msg->transferInProgress()) continue;

    if (destFolder) {
      if (destFolder->protocol() == "imap")
        list.append(msg);
      else
      {
        rc = destFolder->moveMsg(msg, &index);
        if (rc == 0 && index != -1) {
          KMMsgBase *mb = destFolder->unGetMsg( destFolder->count() - 1 );
          kernel->undoStack()->pushAction( mb->getMsgSerNum(), mFolder, destFolder );
        }
      }
    }
    else
    {
      // really delete messages that are already in the trash folder
      if (mFolder->protocol() == "imap")
        list.append(msg);
      else
      {
        mFolder->removeMsg(msg);
        delete msg;
      }
    }
  }
  if (!list.isEmpty())
  {
    if (destFolder)
      destFolder->moveMsg(list, &index);
    else
      mFolder->removeMsg(list);
  }

  blockSignals( false );

  emit selected( 0 );
  if (curMsg) {
    kdDebug(5006) << "new message should be current!" << endl;
    setSelected( currentItem(), TRUE );
    setCurrentMsg( mFolder->find( curMsg ) );
    highlightMessage( currentItem(), false);
  }
  else
    emit selected( 0 );

  setContentsPos( contentX, contentY );
  makeHeaderVisible();
  connect(this,SIGNAL(currentChanged(QListViewItem*)),
	     this,SLOT(highlightMessage(QListViewItem*)));

  if (destFolder) {
      destFolder->sync();
      destFolder->close();
  }
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
  ulong serNum;
  int idx = -1;
  KMFolder *folder, *curFolder, *oldCurFolder;
  if (kernel->undoStack()->popAction(serNum, folder, oldCurFolder))
  {
    kernel->msgDict()->getLocation(serNum, &curFolder, &idx);
    if (idx == -1 || curFolder != oldCurFolder)
      return;
    curFolder->open();
    msg = curFolder->getMsg( idx );
    folder->moveMsg( msg );
    if (folder->count() > 1)
      folder->unGetMsg( folder->count() - 1 );
    curFolder->close();
  }
  else
  {
    // Sorry.. stack is empty..
    KMessageBox::sorry(this, i18n("There is nothing to undo!"));
  }
}

//-----------------------------------------------------------------------------
void KMHeaders::copySelectedToFolder(int menuId )
{
  if (mMenuToFolder[menuId])
    copyMsgToFolder( mMenuToFolder[menuId] );
}


//-----------------------------------------------------------------------------
void KMHeaders::copyMsgToFolder(KMFolder* destFolder,
                                int msgId,
                                KMMessage* aMsg)
{
  KMMessageList* msgList;
  KMMsgBase *msgBase;
  KMMessage *msg, *newMsg;
  int rc, index, idx = -1;
  bool isMessage;
  QPtrList<KMMessage> list;

  if (!destFolder) return;

    
  bool useParam_aMsg = (NULL != aMsg);

  kernel->kbp()->busy();

  destFolder->open();
  msgList = useParam_aMsg ? NULL : selectedMsgs(msgId);
  for ( rc=0, msgBase = (useParam_aMsg ? NULL : msgList->first());
        useParam_aMsg || (msgBase && !rc);
        msgBase = (useParam_aMsg ? NULL : msgList->next()) )
  {
    if( useParam_aMsg )
      msg = aMsg;
    else {
      if (isMessage = msgBase->isMessage())
      {
        msg = static_cast<KMMessage*>(msgBase);
      } else {
        idx = mFolder->find(msgBase);
        assert(idx != -1);
        msg = mFolder->getMsg(idx);
      }
    }

    if ((mFolder->protocol() == "imap") && (destFolder->protocol() == "imap") &&
	(static_cast<KMFolderImap*>(mFolder)->account() ==
	 static_cast<KMFolderImap*>(destFolder)->account()))
    {
      list.append(msg);
    } else {
      newMsg = new KMMessage;
      newMsg->fromString(msg->asString());
      newMsg->setStatus(msg->status());
      newMsg->setComplete(msg->isComplete());

      if ((mFolder->protocol() == "imap") && !newMsg->isComplete())
      {
        kernel->filterMgr()->tempOpenFolder(destFolder);
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

    if (!isMessage && list.isEmpty())
    {
      assert(idx != -1);
      mFolder->unGetMsg( idx );
    }
    
    // To process only one single message?
    if( useParam_aMsg )
      break;
      
  } // end for

  if (!list.isEmpty())
  {
    // copy the message(s); note: the list is empty afterwards!
    KMFolderImap *imapDestFolder = static_cast<KMFolderImap*>(destFolder);
    imapDestFolder->copyMsg(list);
    imapDestFolder->getFolder();
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
KMMessageList* KMHeaders::selectedMsgs(int)
{
  mSelMsgBaseList.clear();
  for (QListViewItemIterator it(this); it.current(); it++)
    if (it.current()->isSelected()) {
      KMHeaderItem *item = static_cast<KMHeaderItem*>(it.current());
      KMMsgBase *msgBase = mFolder->getMsgBase(item->msgId());
      mSelMsgBaseList.append(msgBase);
    }

  return &mSelMsgBaseList;
}

//-----------------------------------------------------------------------------
QPtrList<KMMessage>* KMHeaders::selectedMessages()
{
  mSelMsgList.clear();
  for (QListViewItemIterator it(this); it.current(); it++)
    if (it.current()->isSelected()) {
      KMHeaderItem *item = static_cast<KMHeaderItem*>(it.current());
      KMMessage *msg = mFolder->getMsg(item->msgId());
      mSelMsgList.append(msg);
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
    clearSelection();
    setSelected( lvi, FALSE );
    selectNextMessage();
  }
}

void KMHeaders::selectNextMessage()
{
 QListViewItem *lvi = currentItem();
  if (lvi && lvi->itemBelow()) {
    QListViewItem *temp = lvi;
    while (temp) {
      temp->firstChild();
      temp = temp->parent();
    }
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
    clearSelection();
    setSelected( lvi, FALSE );
    selectPrevMessage();
  }
}

void KMHeaders::selectPrevMessage()
{
  QListViewItem *lvi = currentItem();
  if (lvi && lvi->itemAbove()) {
    QListViewItem *temp = lvi;
    while (temp) {
      temp->firstChild();
      temp = temp->parent();
    }
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
  KMHeaderItem *lastUnread = 0;
  /* itemAbove() is _slow_ */
  if (aDirNext)
  {
    while (item) {
      msgBase = mFolder->getMsgBase(item->msgId());
      if (msgBase && msgBase->isUnread())
        foundUnreadMessage = true;

      if (!onlyNew && msgBase && msgBase->isUnread()) break;
      if (onlyNew && msgBase && msgBase->isNew()) break;
      item = static_cast<KMHeaderItem*>(item->itemBelow());
    }
  } else {
    KMHeaderItem *newItem = static_cast<KMHeaderItem*>(firstChild());
    while (newItem)
    {
      msgBase = mFolder->getMsgBase(newItem->msgId());
      if (msgBase && msgBase->isUnread())
        foundUnreadMessage = true;
      if (!onlyNew && msgBase && msgBase->isUnread()
          || onlyNew && msgBase && msgBase->isNew())
        lastUnread = newItem;
      if (newItem == item) break;
      newItem = static_cast<KMHeaderItem*>(newItem->itemBelow());
    }
    item = lastUnread;
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
    if (!item) {
      if (aDirNext)
	item = static_cast<KMHeaderItem*>(firstChild());
      else
	item = static_cast<KMHeaderItem*>(lastChild());
    }
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
void KMHeaders::nextUnreadMessage(bool acceptCurrent)
{
  if ( !mFolder->countUnread() ) return;
  int i = findUnread(TRUE, -1, false, acceptCurrent);
  if ( i < 0 && mLoopOnGotoUnread )
  {
    KMHeaderItem * first = static_cast<KMHeaderItem*>(firstChild());
    if ( first )
      i = findUnread(TRUE, first->msgId(), false, acceptCurrent); // from top
  }
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
  if ( !mFolder->countUnread() ) return;
  int i = findUnread(FALSE);
  if ( i < 0 && mLoopOnGotoUnread ) {
    KMHeaderItem * last = static_cast<KMHeaderItem*>(lastItem());
    if ( last )
      i = findUnread(FALSE, last->msgId() ); // from bottom
  }
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
        if (mFolder->protocol() == "imap") KMImapJob::ignoreJobsForMessage(prevMsg);
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

void KMHeaders::resetCurrentTime()
{
    mDate.reset();
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
  }

//  if (kernel->folderIsDraftOrOutbox(mFolder))
//    setOpen(lvi, !lvi->isOpen());
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
    noRepaint = TRUE;
    clear();
    noRepaint = FALSE;
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

#ifdef SCORING
int
KMHeaders::messageScore(int msgId)
{
  if (!mScoringManager) return 0;

  if ( !mPaintInfo.showScore ) {
    kdDebug(5006) << "KMFolder::scoreMessages() : Skipping this group - no rules for it"
              << endl;
    return 0;
  }

  QCString cStr;

  folder()->getMsgString(msgId, cStr);

  if (!cStr) {
    kdDebug(5006) << "KMFolder::scoreMessages() : Skipping msg" << endl;
    return 0;
  }

  KMScorableArticle smsg(cStr);

  mScoringManager->applyRules(smsg);

  return smsg.score();

}
#endif


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
	highlightMessage( cur, false);
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
// Handle RMB press, show pop up menu
void KMHeaders::rightButtonPressed( QListViewItem *lvi, const QPoint &, int )
{
  if (!lvi)
    return;

  if (!(lvi->isSelected())) {
    clearSelection();
  }
  setSelected( lvi, TRUE );
  slotRMB();
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
  // This slot isn't called anymore if the RMB is pressed (Qt 3.0.1)
  //kdDebug(5006) << "MB pressed: " << e->button() << endl;
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
    mousePressed = TRUE;
  }
  else if ((e->button() == LeftButton) && (e->state() & ControlButton)) {
    setSelected( lvi, !lvi->isSelected() );
    mousePressed = TRUE;
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
  if (mousePressed && 
      (e->pos() - presspos).manhattanLength() > KGlobalSettings::dndEventDelay()) {
    mousePressed = FALSE;
    QListViewItem *item = itemAt( contentsToViewport(presspos) );
    if ( item ) {
      QDragObject *d = new QStoredDrag("x-kmail-drag/message", viewport());

      // Are multiple messages selected?
      unsigned int count = 0;
      for( QListViewItemIterator it(this); (it.current() && count < 2); it++ )
        if( it.current()->isSelected() )
          count++;

      // Set pixmap
      QPixmap pixmap;
      if( count == 1 )
        pixmap = QPixmap( DesktopIcon("message", KIcon::SizeSmall) );
      else
        pixmap = QPixmap( DesktopIcon("kmultiple", KIcon::SizeSmall) );

      // Calculate hotspot (as in Konqueror)
      if( !pixmap.isNull() ) {
        QPoint hotspot( pixmap.width() / 2, pixmap.height() / 2 );
        d->setPixmap( pixmap, hotspot );
      }
      d->drag();
    }
  }
}

void KMHeaders::highlightMessage(QListViewItem* i)
{
    highlightMessage( i, false );
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
  mOwner->folderToPopupMenu( TRUE, this, &mMenuToFolder, msgMoveMenu );
  QPopupMenu *msgCopyMenu = new QPopupMenu(menu);
  mOwner->folderToPopupMenu( FALSE, this, &mMenuToFolder, msgCopyMenu );

  bool out_folder = kernel->folderIsDraftOrOutbox(mFolder);
  if ( out_folder )
     mOwner->editAction->plug(menu);
  else {
     mOwner->replyAction->plug(menu);
     mOwner->replyAllAction->plug(menu);
     mOwner->action("message_forward")->plug(menu);
     mOwner->bounceAction->plug(menu);
       }
  menu->insertSeparator();

  if ( !out_folder )
      mOwner->filterMenu->plug( menu );

  menu->insertItem(i18n("&Move To"), msgMoveMenu);
  menu->insertItem(i18n("&Copy To"), msgCopyMenu);
  if ( !out_folder ) {
      mOwner->statusMenu->plug( menu );
      if ( mOwner->threadStatusMenu->isEnabled() )
	mOwner->threadStatusMenu->plug( menu );
  }

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
       highlightMessage( mItems[msgIdx], false);
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
  if( nestingPolicy || open)
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
  ensureCurrentItemVisible();
}

//Flatten the list and write it to disk
#define KMAIL_SORT_VERSION 1007
#define KMAIL_SORT_FILE(x) x->indexLocation() + ".sorted"
#define KMAIL_SORT_HEADER "## KMail Sort V%04d\n\t"
#define KMAIL_MAGIC_HEADER_OFFSET 21 //strlen(KMAIL_SORT_HEADER)
#define KMAIL_MAX_KEY_LEN 16384
static void internalWriteItem(FILE *sortStream, int msgid, int parent_id,
			      QString key, bool update_discover=TRUE)
{
  fwrite(&msgid, sizeof(msgid), 1, sortStream);
  fwrite(&parent_id, sizeof(parent_id), 1, sortStream);
  Q_INT32 len = key.length() * sizeof(QChar);
  fwrite(&len, sizeof(len), 1, sortStream);
  if (len)
    fwrite(key.unicode(), QMIN(len, KMAIL_MAX_KEY_LEN), 1, sortStream);

  if (update_discover) {
    //update the discovered change count
      Q_INT32 discovered_count = 0;
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
    if(stat(sortFile.local8Bit(), &stat_tmp) == -1) {
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
    Q_INT32 byteOrder = 0x12345678;
    Q_INT32 column = mSortCol;
    Q_INT32 ascending= !mSortDescending;
    Q_INT32 threaded = (mNested != mNestedOverride);
    Q_INT32 appended=0;
    Q_INT32 discovered_count = 0;
    Q_INT32 sorted_count=0;
    fwrite(&byteOrder, sizeof(byteOrder), 1, sortStream);
    fwrite(&column, sizeof(column), 1, sortStream);
    fwrite(&ascending, sizeof(ascending), 1, sortStream);
    fwrite(&threaded, sizeof(threaded), 1, sortStream);
    fwrite(&appended, sizeof(appended), 1, sortStream);
    fwrite(&discovered_count, sizeof(discovered_count), 1, sortStream);
    fwrite(&sorted_count, sizeof(sorted_count), 1, sortStream);

    QPtrStack<KMHeaderItem> items;
    {
      QPtrStack<QListViewItem> s;
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
    fwrite(&byteOrder, sizeof(byteOrder), 1, sortStream);
    fwrite(&column, sizeof(column), 1, sortStream);
    fwrite(&ascending, sizeof(ascending), 1, sortStream);
    fwrite(&threaded, sizeof(threaded), 1, sortStream);
    fwrite(&appended, sizeof(appended), 1, sortStream);
    fwrite(&discovered_count, sizeof(discovered_count), 1, sortStream);
    fwrite(&sorted_count, sizeof(sorted_count), 1, sortStream);
    if (sortStream && ferror(sortStream)) {
	fclose(sortStream);
	unlink(sortFile.local8Bit());
	kdWarning(5006) << "Error: Failure modifying " << sortFile << " (No space left on device?)" << endl;
	kdWarning(5006) << __FILE__ << ":" << __LINE__ << endl;
	KMKernel::self()->emergencyExit( i18n("Failure modifying %1\n(No space left on device?)").arg( sortFile ));
    }
    fclose(sortStream);
    ::rename(tempName.local8Bit(), sortFile.local8Bit());
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
    Q_INT32 appended = 1;
    fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 16, SEEK_SET);
    fwrite(&appended, sizeof(appended), 1, sortStream);

    if (sortStream && ferror(sortStream)) {
	fclose(sortStream);
	unlink(sortFile.local8Bit());
	kdWarning(5006) << "Error: Failure modifying " << sortFile << " (No space left on device?)" << endl;
	kdWarning(5006) << __FILE__ << ":" << __LINE__ << endl;
	KMKernel::self()->emergencyExit( i18n("Failure modifying %1\n(No space left on device?)").arg( sortFile ));
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

    QPtrList<KMSortCacheItem> mSortedChildren;
    int mUnsortedCount, mUnsortedSize;
    KMSortCacheItem **mUnsortedChildren;

public:
    KMSortCacheItem() : mItem(NULL), mParent(NULL), mId(-1), mSortOffset(-1),
	mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0) { }
    KMSortCacheItem(int i, QString k, int o=-1)
	: mItem(NULL), mParent(NULL), mId(i), mSortOffset(o), mKey(k),
	  mUnsortedCount(0), mUnsortedSize(0), mUnsortedChildren(0) { }
    ~KMSortCacheItem() { if(mUnsortedChildren) free(mUnsortedChildren); }

    KMSortCacheItem *parent() const { return mParent; } //can't be set, only by the parent
    bool hasChildren() const
	{ return mSortedChildren.count() || mUnsortedCount; }
    const QPtrList<KMSortCacheItem> *sortedChildren() const
	{ return &mSortedChildren; }
    KMSortCacheItem **unsortedChildren(int &count) const
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

    KMHeaderItem *item() const { return mItem; }
    void setItem(KMHeaderItem *i) { Q_ASSERT(!mItem); mItem = i; }

    const QString &key() const { return mKey; }
    void setKey(const QString &key) { mKey = key; }

    int id() const { return mId; }
    void setId(int id) { mId = id; }

    int offset() const { return mSortOffset; }
    void setOffset(int x) { mSortOffset = x; }

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
    int ret = (*b1)->key().compare((*b2)->key());
    if(compare_ascending)
	ret = -ret;
    return ret;
}

bool KMHeaders::readSortOrder(bool set_selection)
{
    //all cases
    Q_INT32 column, ascending, threaded, discovered_count, sorted_count, appended;
    bool unread_exists = false;
    QMemArray<KMSortCacheItem *> sortCache(mFolder->count());
    KMSortCacheItem root;
    QString replyToIdMD5;
    root.setId(-666); //mark of the root!

    //threaded cases
    QPtrList<KMSortCacheItem> unparented;
    mIdTree.clear();
    if (mIdTree.size() < 2*(unsigned)mFolder->count())
	mIdTree.resize( 2*mFolder->count() );

    //cleanup
    noRepaint = TRUE;
    clear();
    noRepaint = FALSE;
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
	  Q_INT32 byteOrder = 0;
	  fread(&byteOrder, sizeof(byteOrder), 1, sortStream);
	  if (byteOrder == 0x12345678)
	  {
	    fread(&column, sizeof(column), 1, sortStream);
	    fread(&ascending, sizeof(ascending), 1, sortStream);
	    fread(&threaded, sizeof(threaded), 1, sortStream);
	    fread(&appended, sizeof(appended), 1, sortStream);
	    fread(&discovered_count, sizeof(discovered_count), 1, sortStream);
	    fread(&sorted_count, sizeof(sorted_count), 1, sortStream);

            if (sorted_count + discovered_count > mFolder->count()) { //sanity check
		kdDebug(5006) << "Whoa.0! " << __FILE__ << ":" << __LINE__ << endl;
		fclose(sortStream);
		sortStream = NULL;
	    }
	    else if ((threaded != (mNested != mNestedOverride)) ||
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
		int id, len, parent, x;
		QChar *tmp_qchar = NULL;
		int tmp_qchar_len = 0;
		const int mFolderCount = mFolder->count();
		bool error = false;
		QString key;

		CREATE_TIMER(parse);
		START_TIMER(parse);
		for(x = 0; !feof(sortStream) && !error; x++) {
		    off_t offset = ftell(sortStream);
		    //parse
		    if(!fread(&id, sizeof(id), 1, sortStream) || //short read means to end
		       !fread(&parent, sizeof(parent), 1, sortStream) ||
		       !fread(&len, sizeof(len), 1, sortStream)) {
			break;
		    }
		    if ((id < 0) || (id >= mFolderCount) ||
			(parent < -2) || (parent >= mFolderCount)) { // sanity checking
			kdDebug(5006) << "Whoa.1! " << __FILE__ << ":" << __LINE__ << endl;
			error = true;
			continue;
		    }
		    if ((len < 0) || (len > KMAIL_MAX_KEY_LEN)) {
			kdDebug(5006) << "Whoa.2! len " << len << " " << __FILE__ << ":" << __LINE__ << endl;
			error = true;
			continue;
		    }
		    if(len) {
			if(len > tmp_qchar_len) {
			    tmp_qchar = (QChar *)realloc(tmp_qchar, len);
			    tmp_qchar_len = len;
			}
			if(!fread(tmp_qchar, len, 1, sortStream))
			    break;
			key = QString(tmp_qchar, len / 2);
		    } else {
			key = QString(""); //yuck
		    }

		    if ((item=sortCache[id])) {
			if (item->id() != -1) {
			    kdDebug(5006) << "Whoa.3! " << __FILE__ << ":" << __LINE__ << endl;
			    error = true;
			    continue;
			}
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
		if (error || (x != sorted_count + discovered_count)) {// sanity check
		    kdDebug(5006) << "Whoa: x " << x << ", sorted_count " << sorted_count << ", discovered_count " << discovered_count << ", count " << mFolder->count() << endl;
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
	  }
	  else {
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
	threaded = (mNested != mNestedOverride);
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
		int sortOrder = column;
		if (mPaintInfo.orderOfArrival)
		    sortOrder |= (1 << 6);
		if (mPaintInfo.status)
		    sortOrder |= (1 << 5);
		sortCache[x] = new KMSortCacheItem(
		    x, KMHeaderItem::generate_key(x, this, msg, &mPaintInfo, sortOrder));
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
	for(QPtrListIterator<KMSortCacheItem> it(unparented); it.current(); ++it) {
	    replyToIdMD5 = mFolder->getMsgBase((*it)->id())->replyToIdMD5();
	    if(!replyToIdMD5.isEmpty() && (i = msgs[replyToIdMD5])) {
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
    QPtrQueue<KMSortCacheItem> s;
    s.enqueue(&root);
    do {
	i = s.dequeue();
	const QPtrList<KMSortCacheItem> *sorted = i->sortedChildren();
	int unsorted_count, unsorted_off=0;
	KMSortCacheItem **unsorted = i->unsortedChildren(unsorted_count);
	if(unsorted)
	    qsort(unsorted, unsorted_count, sizeof(KMSortCacheItem *), //sort
		  compare_KMSortCacheItem);
	//merge two sorted lists of siblings
	for(QPtrListIterator<KMSortCacheItem> it(*sorted);
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
	    if(set_selection && mFolder->getMsgBase(new_kci->id())->status() == KMMsgStatusNew ||
		set_selection && mFolder->getMsgBase(new_kci->id())->status() == KMMsgStatusUnread)
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
	    fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 16, SEEK_SET);
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
		bool isUnread = false;
		if (mJumpToUnread) // search unread messages
		    if (mFolder->getMsgBase(item->msgId())->status() == KMMsgStatusUnread)
			isUnread = true;
		
		if (mFolder->getMsgBase(item->msgId())->status() == KMMsgStatusNew || isUnread) {
		    first_unread = item->msgId();
		    break;
		}
		item = static_cast<KMHeaderItem*>(item->itemBelow());
	    }
	}

	if(first_unread == -1 ) {
	    setTopItemByIndex(mTopItem);
	    setCurrentItemByIndex((mCurrentItem >= 0) ? mCurrentItem : 0);
	} else {
	    setCurrentItemByIndex(first_unread);
	    makeHeaderVisible();
	    center( contentsX(), itemPos(mItems[first_unread]), 0, 9.0 );
	}
    } else {
        setTopItemByIndex(mTopItem);
        setCurrentItemByIndex((mCurrentItem >= 0) ? mCurrentItem : 0);
    }
    END_TIMER(selection);
    SHOW_TIMER(selection);
    if (sortStream && ferror(sortStream)) {
	fclose(sortStream);
	unlink(sortFile.local8Bit());
	kdWarning(5006) << "Error: Failure modifying " << sortFile << " (No space left on device?)" << endl;
	kdWarning(5006) << __FILE__ << ":" << __LINE__ << endl;
	KMKernel::self()->emergencyExit( i18n("Failure modifying %1\n(No space left on device?)").arg( sortFile ));
    }
    if(sortStream)
	fclose(sortStream);
    return TRUE;
}


//-----------------------------------------------------------------------------
#include "kmheaders.moc"



