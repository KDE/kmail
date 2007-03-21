// -*- mode: C++; c-file-style: "gnu" -*-
// kmheaders.cpp

#include <config.h>

#include "kmheaders.h"
#include "headeritem.h"
using KMail::HeaderItem;

#include "kcursorsaver.h"
#include "kmcommands.h"
#include "kmmainwidget.h"
#include "kmfiltermgr.h"
#include "undostack.h"
#include "kmmsgdict.h"
#include "kmdebug.h"
#include "kmfoldertree.h"
#include "kmfolderimap.h"
#include "folderjob.h"
using KMail::FolderJob;
#include "actionscheduler.h"
using KMail::ActionScheduler;
#include "messagecopyhelper.h"
using KMail::MessageCopyHelper;
#include "broadcaststatus.h"
using KPIM::BroadcastStatus;
#include "progressmanager.h"
using KPIM::ProgressManager;
using KPIM::ProgressItem;
#include <maillistdrag.h>
#include "globalsettings.h"
using namespace KPIM;

#include <kactionmenu.h>
#include <kacceleratormanager.h>
#include <kglobalsettings.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kmenu.h>
#include <kimageio.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <ktoggleaction.h>
#include <kconfiggroup.h>

#include <mimelib/enum.h>
#include <mimelib/field.h>
#include <mimelib/mimepp.h>

//Added by qt3to4:
#include <q3header.h>
#include <q3ptrstack.h>
#include <q3ptrqueue.h>
#include <QMenu>
#include <Q3MemArray>
#include <q3listview.h>

#include <QApplication>
#include <QBuffer>
#include <QEvent>
#include <QFile>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QTextCodec>

#include <stdlib.h>
#include <errno.h>

#include "textsource.h"

QPixmap* KMHeaders::pixNew = 0;
QPixmap* KMHeaders::pixUns = 0;
QPixmap* KMHeaders::pixDel = 0;
QPixmap* KMHeaders::pixRead = 0;
QPixmap* KMHeaders::pixRep = 0;
QPixmap* KMHeaders::pixQueued = 0;
QPixmap* KMHeaders::pixTodo = 0;
QPixmap* KMHeaders::pixSent = 0;
QPixmap* KMHeaders::pixFwd = 0;
QPixmap* KMHeaders::pixFlag = 0;
QPixmap* KMHeaders::pixWatched = 0;
QPixmap* KMHeaders::pixIgnored = 0;
QPixmap* KMHeaders::pixSpam = 0;
QPixmap* KMHeaders::pixHam = 0;
QPixmap* KMHeaders::pixFullySigned = 0;
QPixmap* KMHeaders::pixPartiallySigned = 0;
QPixmap* KMHeaders::pixUndefinedSigned = 0;
QPixmap* KMHeaders::pixFullyEncrypted = 0;
QPixmap* KMHeaders::pixPartiallyEncrypted = 0;
QPixmap* KMHeaders::pixUndefinedEncrypted = 0;
QPixmap* KMHeaders::pixEncryptionProblematic = 0;
QPixmap* KMHeaders::pixSignatureProblematic = 0;
QPixmap* KMHeaders::pixAttachment = 0;
QPixmap* KMHeaders::pixReadFwd = 0;
QPixmap* KMHeaders::pixReadReplied = 0;
QPixmap* KMHeaders::pixReadFwdReplied = 0;


//-----------------------------------------------------------------------------
KMHeaders::KMHeaders( KMMainWidget *aOwner, QWidget *parent ) :
  K3ListView( parent )
{
  static bool pixmapsLoaded = false;
  //qInitImageIO();
  mOwner  = aOwner;
  mFolder = 0;
  noRepaint = false;
  getMsgIndex = -1;
  mTopItem = 0;
  setSelectionMode( Q3ListView::Extended );
  setAllColumnsShowFocus( true );
  mNested = false;
  nestingPolicy = OpenUnread;
  mNestedOverride = false;
  mSubjThreading = true;
  mMousePressed = false;
  mSortInfo.dirty = true;
  mSortInfo.fakeSort = 0;
  mSortInfo.removed = 0;
  mSortInfo.column = 0;
  mSortCol = 2; // 2 == date
  mSortDescending = false;
  mSortInfo.ascending = false;
  mReaderWindowActive = false;
  mRoot = new SortCacheItem;
  mRoot->setId(-666); //mark of the root!
  setStyleDependantFrameWidth();
  // popup-menu
  header()->setClickEnabled(true);
  header()->installEventFilter(this);
  mPopup = new KMenu(this);
  mPopup->addTitle(i18n("View Columns"));
  QAction* act = mPopup->addAction(i18n("Status"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_STATUS);
  act = mPopup->addAction(i18n("Important"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_IMPORTANT);
  act = mPopup->addAction(i18n("Todo"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_TODO);
  act = mPopup->addAction(i18n("Attachment"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_ATTACHMENT);
  act = mPopup->addAction(i18n("Spam/Ham"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_SPAM_HAM);
  act = mPopup->addAction(i18n("Watched/Ignored"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_WATCHED_IGNORED);
  act = mPopup->addAction(i18n("Signature"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_SIGNED);
  act = mPopup->addAction(i18n("Encryption"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_CRYPTO);
  act = mPopup->addAction(i18n("Size"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_SIZE);
  act = mPopup->addAction(i18n("Receiver"));
  act->setCheckable(true);
  mColumns.insert(act, KPaintInfo::COL_RECEIVER);

  connect(mPopup, SIGNAL(triggered(QAction*)), SLOT(slotToggleColumn(QAction*)));

  setShowSortIndicator(true);
  setFocusPolicy( Qt::WheelFocus );

  if (!pixmapsLoaded)
  {
    pixmapsLoaded = true;
    pixNew                   = new QPixmap( UserIcon( "kmmsgnew"                   ) );
    pixUns                   = new QPixmap( UserIcon( "kmmsgunseen"                ) );
    pixDel                   = new QPixmap( UserIcon( "kmmsgdel"                   ) );
    pixRead                  = new QPixmap( UserIcon( "kmmsgread"                  ) );
    pixRep                   = new QPixmap( UserIcon( "kmmsgreplied"               ) );
    pixQueued                = new QPixmap( UserIcon( "kmmsgqueued"                ) );
    pixTodo                  = new QPixmap( UserIcon( "kmmsgtodo"                  ) );
    pixSent                  = new QPixmap( UserIcon( "kmmsgsent"                  ) );
    pixFwd                   = new QPixmap( UserIcon( "kmmsgforwarded"             ) );
    pixFlag                  = new QPixmap( UserIcon( "kmmsgflag"                  ) );
    pixWatched               = new QPixmap( UserIcon( "kmmsgwatched"               ) );
    pixIgnored               = new QPixmap( UserIcon( "kmmsgignored"               ) );
    pixSpam                  = new QPixmap( UserIcon( "kmmsgspam"                  ) );
    pixHam                   = new QPixmap( UserIcon( "kmmsgham"                   ) );
    pixFullySigned           = new QPixmap( UserIcon( "kmmsgfullysigned"           ) );
    pixPartiallySigned       = new QPixmap( UserIcon( "kmmsgpartiallysigned"       ) );
    pixUndefinedSigned       = new QPixmap( UserIcon( "kmmsgundefinedsigned"       ) );
    pixFullyEncrypted        = new QPixmap( UserIcon( "kmmsgfullyencrypted"        ) );
    pixPartiallyEncrypted    = new QPixmap( UserIcon( "kmmsgpartiallyencrypted"    ) );
    pixUndefinedEncrypted    = new QPixmap( UserIcon( "kmmsgundefinedencrypted"    ) );
    pixEncryptionProblematic = new QPixmap( UserIcon( "kmmsgencryptionproblematic" ) );
    pixSignatureProblematic  = new QPixmap( UserIcon( "kmmsgsignatureproblematic"  ) );
    pixAttachment            = new QPixmap( UserIcon( "kmmsgattachment"            ) );
    pixReadFwd               = new QPixmap( UserIcon( "kmmsgread_fwd"              ) );
    pixReadReplied           = new QPixmap( UserIcon( "kmmsgread_replied"          ) );
    pixReadFwdReplied        = new QPixmap( UserIcon( "kmmsgread_fwd_replied"      ) );
  }

  header()->setStretchEnabled( false );
  header()->setResizeEnabled( false );

  mPaintInfo.subCol      = addColumn( i18n("Subject"), 310 );
  mPaintInfo.senderCol   = addColumn( i18n("Sender"),  170 );
  mPaintInfo.dateCol     = addColumn( i18n("Date"),    170 );
  mPaintInfo.sizeCol     = addColumn( i18n("Size"),      0 );
  mPaintInfo.receiverCol = addColumn( i18n("Receiver"),  0 );

  mPaintInfo.statusCol         = addColumn( *pixNew           , "", 0 );
  mPaintInfo.importantCol      = addColumn( *pixFlag          , "", 0 );
  mPaintInfo.todoCol           = addColumn( *pixTodo          , "", 0 );
  mPaintInfo.attachmentCol     = addColumn( *pixAttachment    , "", 0 );
  mPaintInfo.spamHamCol        = addColumn( *pixSpam          , "", 0 );
  mPaintInfo.watchedIgnoredCol = addColumn( *pixWatched       , "", 0 );
  mPaintInfo.signedCol         = addColumn( *pixFullySigned   , "", 0 );
  mPaintInfo.cryptoCol         = addColumn( *pixFullyEncrypted, "", 0 );

  setResizeMode( Q3ListView::NoColumn );

  // only the non-optional columns shall be resizeable
  header()->setResizeEnabled( true, mPaintInfo.subCol );
  header()->setResizeEnabled( true, mPaintInfo.senderCol );
  header()->setResizeEnabled( true, mPaintInfo.dateCol );

  connect( this, SIGNAL( contextMenuRequested( Q3ListViewItem*, const QPoint &, int )),
           this, SLOT( rightButtonPressed( Q3ListViewItem*, const QPoint &, int )));
  connect(this, SIGNAL(doubleClicked(Q3ListViewItem*)),
          this,SLOT(selectMessage(Q3ListViewItem*)));
  connect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
          this,SLOT(highlightMessage(Q3ListViewItem*)));
  resetCurrentTime();

  mSubjectLists.setAutoDelete( true );

  mMoveMessages = false;
  connect( this, SIGNAL(selectionChanged()), SLOT(updateActions()) );
}


//-----------------------------------------------------------------------------
KMHeaders::~KMHeaders ()
{
  if ( mFolder ) {
    writeFolderConfig();
    writeSortOrder();
    if ( mFolder->folderType() == KMFolderTypeImap ) {
      KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder->storage());
      imap->setSelected( false );
    }
    mFolder->close();
  }
  writeConfig();
  delete mRoot;
}

//-----------------------------------------------------------------------------
bool KMHeaders::eventFilter ( QObject *o, QEvent *e )
{
  if ( e->type() == QEvent::MouseButtonPress &&
      static_cast<QMouseEvent*>(e)->button() == Qt::RightButton &&
       ::qobject_cast<Q3Header *>(o) )
  {
    // if we currently only show one of either sender/receiver column
    // modify the popup text in the way, that it displays the text of the other of the two
    QAction* act = mColumns.key(KPaintInfo::COL_RECEIVER);
    if ( mPaintInfo.showReceiver )
      act->setText(i18n("Receiver"));
    else
      if ( mFolder && (mFolder->whoField().toLower() == "to") )
          act->setText(i18n("Sender"));
      else
          act->setText(i18n("Receiver"));

    mPopup->popup( static_cast<QMouseEvent*>(e)->globalPos() );
    return true;
  }
  return K3ListView::eventFilter(o, e);
}

//-----------------------------------------------------------------------------

void KMHeaders::slotToggleColumn(QAction* act, int mode)
{
  bool *show = 0;
  int  *col  = 0;
  int  width = 0;

  switch ( mColumns[act] )
  {
    case KPaintInfo::COL_SIZE:
    {
      show  = &mPaintInfo.showSize;
      col   = &mPaintInfo.sizeCol;
      width = 80;
      break;
    }
    case KPaintInfo::COL_ATTACHMENT:
    {
      show  = &mPaintInfo.showAttachment;
      col   = &mPaintInfo.attachmentCol;
      width = pixAttachment->width();
      break;
    }
    case KPaintInfo::COL_IMPORTANT:
    {
      show  = &mPaintInfo.showImportant;
      col   = &mPaintInfo.importantCol;
      width = pixFlag->width();
      break;
    }
    case KPaintInfo::COL_TODO:
    {
      show  = &mPaintInfo.showTodo;
      col   = &mPaintInfo.todoCol;
      width = pixTodo->width();
      break;
    }
    case KPaintInfo::COL_SPAM_HAM:
    {
      show  = &mPaintInfo.showSpamHam;
      col   = &mPaintInfo.spamHamCol;
      width = pixSpam->width();
      break;
    }
    case KPaintInfo::COL_WATCHED_IGNORED:
    {
      show  = &mPaintInfo.showWatchedIgnored;
      col   = &mPaintInfo.watchedIgnoredCol;
      width = pixWatched->width();
      break;
    }
    case KPaintInfo::COL_STATUS:
    {
      show  = &mPaintInfo.showStatus;
      col   = &mPaintInfo.statusCol;
      width = pixNew->width();
      break;
    }
    case KPaintInfo::COL_SIGNED:
    {
      show  = &mPaintInfo.showSigned;
      col   = &mPaintInfo.signedCol;
      width = pixFullySigned->width();
      break;
    }
    case KPaintInfo::COL_CRYPTO:
    {
      show  = &mPaintInfo.showCrypto;
      col   = &mPaintInfo.cryptoCol;
      width = pixFullyEncrypted->width();
      break;
    }
    case KPaintInfo::COL_RECEIVER:
    {
      show  = &mPaintInfo.showReceiver;
      col   = &mPaintInfo.receiverCol;
      width = 170;
      break;
    }
    case KPaintInfo::COL_SCORE: ; // only used by KNode
    // don't use default, so that the compiler tells us you forgot to code here for a new column
  }

  assert(show);

  if (mode == -1)
    *show = !*show;
  else
    *show = mode;

  act->setChecked(*show);

  if (*show) {
    header()->setResizeEnabled(true, *col);
    setColumnWidth(*col, width);
  }
  else {
    header()->setResizeEnabled(false, *col);
    header()->setStretchEnabled(false, *col);
    hideColumn(*col);
  }

  // if we change the visibility of the receiver column,
  // the sender column has to show either the sender or the receiver
  if ( mColumns[act] ==  KPaintInfo::COL_RECEIVER ) {
    QString colText = i18n( "Sender" );
    if ( mFolder && (mFolder->whoField().toLower() == "to") && !mPaintInfo.showReceiver)
      colText = i18n( "Receiver" );
    setColumnText( mPaintInfo.senderCol, colText );
  }

  if (mode == -1)
    writeConfig();
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
    p->fillRect( rect, palette().color( QPalette::Base ) );
}

bool KMHeaders::event(QEvent *e)
{
  bool result = K3ListView::event(e);
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
  }
  return result;
}


//-----------------------------------------------------------------------------
void KMHeaders::readColorConfig (void)
{
  // Custom/System colors
  KConfigGroup config( KMKernel::config(), "Reader" );
  QColor c1=QColor(qApp->palette().color( QPalette::Text ));
  QColor c2=QColor("red");
  QColor c3=QColor("blue");
  QColor c4=QColor(qApp->palette().color( QPalette::Base ));
  QColor c5=QColor(0,0x7F,0);
  QColor c6=QColor(0,0x98,0);
  QColor c7=KGlobalSettings::alternateBackgroundColor();

  if (!config.readEntry( "defaultColors", true ) ) {
    mPaintInfo.colFore = config.readEntry( "ForegroundColor", c1 );
    mPaintInfo.colBack = config.readEntry( "BackgroundColor", c4 );
    QPalette newPal = qApp->palette();
    newPal.setColor( QPalette::Base, mPaintInfo.colBack );
    newPal.setColor( QPalette::Text, mPaintInfo.colFore );
    setPalette( newPal );
    mPaintInfo.colNew = config.readEntry( "NewMessage", c2 );
    mPaintInfo.colUnread = config.readEntry( "UnreadMessage", c3 );
    mPaintInfo.colFlag = config.readEntry( "FlagMessage", c5 );
    mPaintInfo.colTodo = config.readEntry( "TodoMessage", c6 );
    c7 = config.readEntry( "AltBackgroundColor", c7 );
  }
  else {
    mPaintInfo.colFore = c1;
    mPaintInfo.colBack = c4;
    QPalette newPal = qApp->palette();
    newPal.setColor( QPalette::Base, c4 );
    newPal.setColor( QPalette::Text, c1 );
    setPalette( newPal );
    mPaintInfo.colNew = c2;
    mPaintInfo.colUnread = c3;
    mPaintInfo.colFlag = c5;
    mPaintInfo.colTodo = c6;
  }
  setAlternateBackground(c7);
}

//-----------------------------------------------------------------------------
void KMHeaders::readConfig (void)
{
  // Backing pixmap support
  { // area for config group "Pixmaps"
    KConfigGroup config( KMKernel::config(), "Pixmaps" );
    QString pixmapFile = config.readEntry("Headers");
    mPaintInfo.pixmapOn = false;
    if (!pixmapFile.isEmpty()) {
      mPaintInfo.pixmapOn = true;
      mPaintInfo.pixmap = QPixmap( pixmapFile );
    }
  }

  { // area for config group "General"
    KConfigGroup config( KMKernel::config(), "General" );
    bool show = config.readEntry( "showMessageSize", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_SIZE), show);

    show = config.readEntry( "showAttachmentColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_ATTACHMENT), show);

    show = config.readEntry( "showImportantColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_IMPORTANT), show);

    show = config.readEntry( "showTodoColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_TODO), show);

    show = config.readEntry( "showSpamHamColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_SPAM_HAM), show);

    show = config.readEntry( "showWatchedIgnoredColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_WATCHED_IGNORED), show);

    show = config.readEntry( "showStatusColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_STATUS), show);

    show = config.readEntry( "showSignedColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_SIGNED), show);

    show = config.readEntry( "showCryptoColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_CRYPTO), show);

    show = config.readEntry( "showReceiverColumn", false );
    slotToggleColumn(mColumns.key(KPaintInfo::COL_RECEIVER), show);

    mPaintInfo.showCryptoIcons = config.readEntry( "showCryptoIcons", false );
    mPaintInfo.showAttachmentIcon = config.readEntry( "showAttachmentIcon", true );

    KMime::DateFormatter::FormatType t =
      (KMime::DateFormatter::FormatType) config.readEntry( "dateFormat",
                                                           ( int )KMime::DateFormatter::Fancy );
    mDate.setCustomFormat( config.readEntry( "customDateFormat", QString() ) );
    mDate.setFormat( t );
  }

  readColorConfig();

  // Custom/System fonts
  { // area for config group "General"
    KConfigGroup config( KMKernel::config(), "Fonts" );
    if ( !( config.readEntry( "defaultFonts", true ) ) )
    {
      QFont listFont( KGlobalSettings::generalFont() );
      listFont = config.readEntry( "list-font", listFont );
      setFont( listFont );
      mNewFont = config.readEntry( "list-new-font", listFont );
      mUnreadFont = config.readEntry( "list-unread-font", listFont );
      mImportantFont = config.readEntry( "list-important-font", listFont );
      mTodoFont = config.readEntry( "list-todo-font", listFont );
      mDateFont = KGlobalSettings::fixedFont();
      mDateFont = config.readEntry( "list-date-font", mDateFont );
    } else {
      mNewFont= mUnreadFont = mImportantFont = mDateFont = mTodoFont =
        KGlobalSettings::generalFont();
      setFont( mDateFont );
    }
  }

  // Behavior
  {
    KConfigGroup config( KMKernel::config(), "Geometry" );
    mReaderWindowActive = config.readEntry( "readerWindowMode", "below" ) != "hide";
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::reset()
{
  int top = topItemIndex();
  int id = currentItemIndex();
  noRepaint = true;
  clear();
  QString colText = i18n( "Sender" );
  if ( mFolder && (mFolder->whoField().toLower() == "to") && !mPaintInfo.showReceiver)
    colText = i18n( "Receiver" );
  setColumnText( mPaintInfo.senderCol, colText );
  noRepaint = false;
  mItems.resize(0);
  updateMessageList();
  setCurrentMsg(id);
  setTopItemByIndex(top);
  ensureCurrentItemVisible();
}

//-----------------------------------------------------------------------------
void KMHeaders::refreshNestedState(void)
{
  bool oldState = isThreaded();
  NestingPolicy oldNestPolicy = nestingPolicy;
  KConfigGroup config( KMKernel::config(), "Geometry" );
  mNested = config.readEntry( "nestedMessages", false );

  nestingPolicy = (NestingPolicy) config.readEntry( "nestingPolicy",
                                                    ( int ) OpenUnread );
  if ((nestingPolicy != oldNestPolicy) ||
    (oldState != isThreaded()))
  {
    setRootIsDecorated( nestingPolicy != AlwaysOpen && isThreaded() );
    reset();
  }

}

//-----------------------------------------------------------------------------
void KMHeaders::readFolderConfig (void)
{
  if (!mFolder) return;

  KConfigGroup config( KMKernel::config(), "Folder-" + mFolder->idString() );
  mNestedOverride = config.readEntry( "threadMessagesOverride", false );
  mSortCol = config.readEntry( "SortColumn", mSortCol  + 1 /* inited to date column */);
  mSortDescending = (mSortCol < 0);
  mSortCol = abs(mSortCol) - 1;

  mTopItem = config.readEntry( "Top", 0 );
  mCurrentItem = config.readEntry( "Current", 0 );
  mCurrentItemSerNum = config.readEntry( "CurrentSerialNum", 0 );

  mPaintInfo.orderOfArrival = config.readEntry( "OrderOfArrival", true );
  mPaintInfo.status = config.readEntry( "Status", false );

  { //area for config group "Geometry"
    KConfigGroup config( KMKernel::config(), "Geometry" );
    mNested = config.readEntry( "nestedMessages", false );
    nestingPolicy = (NestingPolicy) config.readEntry( "nestingPolicy",
                                                      ( int ) OpenUnread );
  }

  setRootIsDecorated( nestingPolicy != AlwaysOpen && isThreaded() );
  mSubjThreading = config.readEntry( "threadMessagesBySubject", true );
}


//-----------------------------------------------------------------------------
void KMHeaders::writeFolderConfig (void)
{
  if (!mFolder) return;
  int mSortColAdj = mSortCol + 1;

  KConfigGroup config( KMKernel::config(), "Folder-" + mFolder->idString() );
  config.writeEntry("SortColumn", (mSortDescending ? -mSortColAdj : mSortColAdj));
  config.writeEntry("Top", topItemIndex());
  config.writeEntry("Current", currentItemIndex());
  HeaderItem* current = currentHeaderItem();
  ulong sernum = 0;
  if ( current && mFolder->getMsgBase( current->msgId() ) )
    sernum = mFolder->getMsgBase( current->msgId() )->getMsgSerNum();
  config.writeEntry("CurrentSerialNum", (qulonglong) sernum );

  config.writeEntry("OrderOfArrival", mPaintInfo.orderOfArrival );
  config.writeEntry("Status", mPaintInfo.status );
}

//-----------------------------------------------------------------------------
void KMHeaders::writeConfig (void)
{
  saveLayout( KMKernel::config(), "Header-Geometry" );
  KConfigGroup config( KMKernel::config(), "General" );
  config.writeEntry("showMessageSize"         , mPaintInfo.showSize);
  config.writeEntry("showAttachmentColumn"    , mPaintInfo.showAttachment);
  config.writeEntry("showImportantColumn"     , mPaintInfo.showImportant);
  config.writeEntry("showTodoColumn"          , mPaintInfo.showTodo);
  config.writeEntry("showSpamHamColumn"       , mPaintInfo.showSpamHam);
  config.writeEntry("showWatchedIgnoredColumn", mPaintInfo.showWatchedIgnored);
  config.writeEntry("showStatusColumn"        , mPaintInfo.showStatus);
  config.writeEntry("showSignedColumn"        , mPaintInfo.showSigned);
  config.writeEntry("showCryptoColumn"        , mPaintInfo.showCrypto);
  config.writeEntry("showReceiverColumn"      , mPaintInfo.showReceiver);
}

//-----------------------------------------------------------------------------
void KMHeaders::setFolder( KMFolder *aFolder, bool forceJumpToUnread )
{
  CREATE_TIMER(set_folder);
  START_TIMER(set_folder);

  int id;
  QString str;

  mSortInfo.fakeSort = 0;
  if ( mFolder && static_cast<KMFolder*>(mFolder) == aFolder ) {
    int top = topItemIndex();
    id = currentItemIndex();
    writeFolderConfig();
    readFolderConfig();
    updateMessageList(); // do not change the selection
    setCurrentMsg(id);
    setTopItemByIndex(top);
  } else {
    if (mFolder) {
    // WABA: Make sure that no KMReaderWin is still using a msg
    // from this folder, since it's msg's are about to be deleted.
      highlightMessage(0, false);

      disconnect(mFolder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
          this, SLOT(setFolderInfoStatus()));

      mFolder->markNewAsUnread();
      writeFolderConfig();
      disconnect(mFolder, SIGNAL(msgHeaderChanged(KMFolder*,int)),
                 this, SLOT(msgHeaderChanged(KMFolder*,int)));
      disconnect(mFolder, SIGNAL(msgAdded(int)),
                 this, SLOT(msgAdded(int)));
      disconnect(mFolder, SIGNAL( msgRemoved( int, QString ) ),
                 this, SLOT( msgRemoved( int, QString ) ) );
      disconnect(mFolder, SIGNAL(changed()),
                 this, SLOT(msgChanged()));
      disconnect(mFolder, SIGNAL(cleared()),
                 this, SLOT(folderCleared()));
      disconnect(mFolder, SIGNAL(expunged( KMFolder* )),
                 this, SLOT(folderCleared()));
      disconnect( mFolder, SIGNAL( statusMsg( const QString& ) ),
                  BroadcastStatus::instance(), SLOT( setStatusMsg( const QString& ) ) );
      disconnect(mFolder, SIGNAL(viewConfigChanged()), this, SLOT(reset()));
      writeSortOrder();
      mFolder->close();
      // System folders remain open but we also should write the index from
      // time to time
      if (mFolder->dirty()) mFolder->writeIndex();
    }

    mSortInfo.removed = 0;
    mFolder = aFolder;
    mSortInfo.dirty = true;
    mOwner->editAction()->setEnabled( mFolder ?
                         ( kmkernel->folderIsDraftOrOutbox( mFolder ) ||
                           kmkernel->folderIsTemplates( mFolder ) ) : false );
    mOwner->useAction()->setEnabled( mFolder ?
                         ( kmkernel->folderIsTemplates( mFolder ) ) : false );
    mOwner->replyListAction()->setEnabled( mFolder ?
                         mFolder->isMailingListEnabled() : false );
    if (mFolder)
    {
      connect(mFolder, SIGNAL(msgHeaderChanged(KMFolder*,int)),
              this, SLOT(msgHeaderChanged(KMFolder*,int)));
      connect(mFolder, SIGNAL(msgAdded(int)),
              this, SLOT(msgAdded(int)));
      connect(mFolder, SIGNAL(msgRemoved(int,QString)),
              this, SLOT(msgRemoved(int,QString)));
      connect(mFolder, SIGNAL(changed()),
              this, SLOT(msgChanged()));
      connect(mFolder, SIGNAL(cleared()),
              this, SLOT(folderCleared()));
      connect(mFolder, SIGNAL(expunged( KMFolder* )),
                 this, SLOT(folderCleared()));
      connect(mFolder, SIGNAL(statusMsg(const QString&)),
              BroadcastStatus::instance(), SLOT( setStatusMsg( const QString& ) ) );
      connect(mFolder, SIGNAL(numUnreadMsgsChanged(KMFolder*)),
          this, SLOT(setFolderInfoStatus()));
      connect(mFolder, SIGNAL(viewConfigChanged()), this, SLOT(reset()));

      // Not very nice, but if we go from nested to non-nested
      // in the folderConfig below then we need to do this otherwise
      // updateMessageList would do something unspeakable
      if (isThreaded()) {
        noRepaint = true;
        clear();
        noRepaint = false;
        mItems.resize( 0 );
      }

      readFolderConfig();

      CREATE_TIMER(kmfolder_open);
      START_TIMER(kmfolder_open);
      mFolder->open();
      END_TIMER(kmfolder_open);
      SHOW_TIMER(kmfolder_open);

      if (isThreaded()) {
        noRepaint = true;
        clear();
        noRepaint = false;
        mItems.resize( 0 );
      }
    }

    CREATE_TIMER(updateMsg);
    START_TIMER(updateMsg);
    updateMessageList(true, forceJumpToUnread);
    END_TIMER(updateMsg);
    SHOW_TIMER(updateMsg);
    makeHeaderVisible();
    setFolderInfoStatus();

    QString colText = i18n( "Sender" );
    if (mFolder && (mFolder->whoField().toLower() == "to") && !mPaintInfo.showReceiver)
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
  }

  updateActions();

  END_TIMER(set_folder);
  SHOW_TIMER(set_folder);
}

//-----------------------------------------------------------------------------
void KMHeaders::msgChanged()
{
  if (mFolder->count() == 0) { // Folder cleared
    clear();
    return;
  }
  int i = topItemIndex();
  int cur = currentItemIndex();
  if (!updatesEnabled()) return;
  QString msgIdMD5;
  Q3ListViewItem *item = currentItem();
  HeaderItem *hi = dynamic_cast<HeaderItem*>(item);
  if (item && hi) {
    // get the msgIdMD5 to compare it later
    KMMsgBase *mb = mFolder->getMsgBase(hi->msgId());
    if (mb)
      msgIdMD5 = mb->msgIdMD5();
  }
//  if (!updatesEnabled()) return;
  // prevent IMAP messages from scrolling to top
  disconnect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
             this,SLOT(highlightMessage(Q3ListViewItem*)));
  // remember all selected messages
  QList<int> curItems = selectedItems();
  updateMessageList(); // do not change the selection
  // restore the old state, but move up when there are unread message just out of view
  HeaderItem *topOfList = mItems[i];
  item = firstChild();
  Q3ListViewItem *unreadItem = 0;
  while(item && item != topOfList) {
    KMMsgBase *msg = mFolder->getMsgBase( static_cast<HeaderItem*>(item)->msgId() );
    if ( msg->status().isUnread() || msg->status().isNew() ) {
      if ( !unreadItem )
        unreadItem = item;
    } else
      unreadItem = 0;
    item = item->itemBelow();
  }
  if(unreadItem == 0)
      unreadItem = topOfList;
  setContentsPos( 0, itemPos( unreadItem ));
  setCurrentMsg( cur );
  setSelectedByIndex( curItems, true );
  connect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
          this,SLOT(highlightMessage(Q3ListViewItem*)));

  // if the current message has changed then emit
  // the selected signal to force an update

  // Normally the serial number of the message would be
  // used to do this, but because we don't yet have
  // guaranteed serial numbers for IMAP messages fall back
  // to using the MD5 checksum of the msgId.
  item = currentItem();
  hi = dynamic_cast<HeaderItem*>(item);
  if (item && hi) {
    KMMsgBase *mb = mFolder->getMsgBase(hi->msgId());
    if (mb) {
      if (msgIdMD5.isEmpty() || (msgIdMD5 != mb->msgIdMD5()))
        emit selected(mFolder->getMsg(hi->msgId()));
    } else {
      emit selected(0);
    }
  } else
    emit selected(0);
}


//-----------------------------------------------------------------------------
void KMHeaders::msgAdded(int id)
{
  HeaderItem* hi = 0;
  if (!updatesEnabled()) return;

  CREATE_TIMER(msgAdded);
  START_TIMER(msgAdded);

  assert( mFolder->getMsgBase( id ) ); // otherwise using count() is wrong

  /* Create a new SortCacheItem to be used for threading. */
  SortCacheItem *sci = new SortCacheItem;
  sci->setId(id);
  if (isThreaded()) {
    // make sure the id and subject dicts grow, if necessary
    if (mSortCacheItems.count() == (uint)mFolder->count()
        || mSortCacheItems.count() == 0) {
      kDebug (5006) << "KMHeaders::msgAdded - Resizing id and subject trees of " << mFolder->label()
       << ": before=" << mSortCacheItems.count() << " ,after=" << (mFolder->count()*2) << endl;
      mSortCacheItems.resize(mFolder->count()*2);
      mSubjectLists.resize(mFolder->count()*2);
    }
    QString msgId = mFolder->getMsgBase(id)->msgIdMD5();
    if (msgId.isNull())
      msgId = "";
    QString replyToId = mFolder->getMsgBase(id)->replyToIdMD5();

    SortCacheItem *parent = findParent( sci );
    if (!parent && mSubjThreading) {
      parent = findParentBySubject( sci );
      if (parent && sci->isImperfectlyThreaded()) {
        // The parent we found could be by subject, in which case it is
        // possible, that it would be preferable to thread it below us,
        // not the other way around. Check that. This is not only
        // cosmetic, as getting this wrong leads to circular threading.
        if (msgId == mFolder->getMsgBase(parent->item()->msgId())->replyToIdMD5()
         || msgId == mFolder->getMsgBase(parent->item()->msgId())->replyToAuxIdMD5())
          parent = NULL;
      }
    }

    if (parent && mFolder->getMsgBase(parent->id())->status().isWatched())
      mFolder->getMsgBase(id)->setStatus( MessageStatus::statusWatched() );
    else if (parent && mFolder->getMsgBase(parent->id())->status().isIgnored())
      mFolder->getMsgBase(id)->setStatus( MessageStatus::statusIgnored() );
    if (parent)
      hi = new HeaderItem( parent->item(), id );
    else
      hi = new HeaderItem( this, id );

    // o/` ... my buddy and me .. o/`
    hi->setSortCacheItem(sci);
    sci->setItem(hi);

    // Update and resize the id trees.
    mItems.resize( mFolder->count() );
    mItems[id] = hi;

    if ( !msgId.isEmpty() )
      mSortCacheItems.replace(msgId, sci);
    /* Add to the list of potential parents for subject threading. But only if
     * we are top level. */
    if (mSubjThreading && parent) {
      QString subjMD5 = mFolder->getMsgBase(id)->strippedSubjectMD5();
      if (subjMD5.isEmpty()) {
        mFolder->getMsgBase(id)->initStrippedSubjectMD5();
        subjMD5 = mFolder->getMsgBase(id)->strippedSubjectMD5();
      }
      if( !subjMD5.isEmpty()) {
        if ( !mSubjectLists.find(subjMD5) )
          mSubjectLists.insert(subjMD5, new QList<SortCacheItem*>());
        // insertion sort by date. See buildThreadTrees for details.
        int p=0;
        QList<SortCacheItem*>::const_iterator it;
        for (it = (*mSubjectLists[subjMD5]).constBegin();
            it != (*mSubjectLists[subjMD5]).constEnd(); ++it) {
          KMMsgBase *mb = mFolder->getMsgBase((*it)->id());
          if ( mb->date() < mFolder->getMsgBase(id)->date())
            break;
          p++;
        }
        mSubjectLists[subjMD5]->insert( p, sci);
        sci->setSubjectThreadingList( mSubjectLists[subjMD5] );
      }
    }
    // The message we just added might be a better parent for one of the as of
    // yet imperfectly threaded messages. Let's find out.

    /* In case the current item is taken during reparenting, prevent qlistview
     * from selecting some unrelated item as a result of take() emitting
     * currentChanged. */
    disconnect( this, SIGNAL(currentChanged(Q3ListViewItem*)),
           this, SLOT(highlightMessage(Q3ListViewItem*)));

    if ( !msgId.isEmpty() ) {
      QList<HeaderItem*>::iterator it = mImperfectlyThreadedList.begin();
      HeaderItem *cur;
      while ( (cur = (*it)) && it != mImperfectlyThreadedList.end() ) {
        ++it;
        int tryMe = cur->msgId();
        // Check, whether our message is the replyToId or replyToAuxId of
        // this one. If so, thread it below our message, unless it is already
        // correctly threaded by replyToId.
        bool perfectParent = true;
        KMMsgBase *otherMsg = mFolder->getMsgBase(tryMe);
        if ( !otherMsg ) {
          kDebug(5006) << "otherMsg is NULL !!! tryMe: " << tryMe << endl;
          continue;
        }
        QString otherId = otherMsg->replyToIdMD5();
        if (msgId != otherId) {
          if (msgId != otherMsg->replyToAuxIdMD5())
            continue;
          else {
            if (!otherId.isEmpty() && mSortCacheItems.find(otherId))
              continue;
            else
              // Thread below us by aux id, but keep on the list of
              // imperfectly threaded messages.
              perfectParent = false;
          }
        }
        Q3ListViewItem *newParent = mItems[id];
        Q3ListViewItem *msg = mItems[tryMe];

        if (msg->parent())
          msg->parent()->takeItem(msg);
        else
          takeItem(msg);
        newParent->insertItem(msg);
        HeaderItem *hi = static_cast<HeaderItem*>( newParent );
        hi->sortCacheItem()->addSortedChild( cur->sortCacheItem() );

        makeHeaderVisible();

        if (perfectParent) {
          mImperfectlyThreadedList.removeAll( mItems[tryMe] );
          // The item was imperfectly thread before, now it's parent
          // is there. Update the .sorted file accordingly.
          QString sortFile = KMAIL_SORT_FILE(mFolder);
          FILE *sortStream = fopen(QFile::encodeName(sortFile), "r+");
          if (sortStream) {
            mItems[tryMe]->sortCacheItem()->updateSortFile( sortStream, mFolder );
            fclose (sortStream);
          }
        }
      }
    }
    // Add ourselves only now, to avoid circularity above.
    if (hi && hi->sortCacheItem()->isImperfectlyThreaded())
      mImperfectlyThreadedList.append(hi);
  } else {
    // non-threaded case
    hi = new HeaderItem( this, id );
    mItems.resize( mFolder->count() );
    mItems[id] = hi;
    // o/` ... my buddy and me .. o/`
    hi->setSortCacheItem(sci);
    sci->setItem(hi);
  }
  if (mSortInfo.fakeSort) {
    QObject::disconnect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
    K3ListView::setSorting(mSortCol, !mSortDescending );
    mSortInfo.fakeSort = 0;
  }
  appendItemToSortFile(hi); //inserted into sorted list

  msgHeaderChanged(mFolder,id);

  if ((childCount() == 1) && hi) {
    setSelected( hi, true );
    setCurrentItem( firstChild() );
    setSelectionAnchor( currentItem() );
    highlightMessage( currentItem() );
  }

  /* restore signal */
  connect( this, SIGNAL(currentChanged(Q3ListViewItem*)),
           this, SLOT(highlightMessage(Q3ListViewItem*)));

  emit msgAddedToListView( hi );
  END_TIMER(msgAdded);
  SHOW_TIMER(msgAdded);
}


//-----------------------------------------------------------------------------
void KMHeaders::msgRemoved(int id, QString msgId )
{
  if (!updatesEnabled()) return;

  if ((id < 0) || (id >= (int)mItems.size()))
    return;
  /*
   * qlistview has its own ideas about what to select as the next
   * item once this one is removed. Sine we have already selected
   * something in prepare/finalizeMove that's counter productive
   */
  disconnect( this, SIGNAL(currentChanged(Q3ListViewItem*)),
              this, SLOT(highlightMessage(Q3ListViewItem*)));

  HeaderItem *removedItem = mItems[id];
  if (!removedItem) return;
  HeaderItem *curItem = currentHeaderItem();

  for (int i = id; i < (int)mItems.size() - 1; ++i) {
    mItems[i] = mItems[i+1];
    mItems[i]->setMsgId( i );
    mItems[i]->sortCacheItem()->setId( i );
  }

  mItems.resize( mItems.size() - 1 );

  if (isThreaded() && mFolder->count()) {
    if ( !msgId.isEmpty() && mSortCacheItems[msgId] ) {
      if (mSortCacheItems[msgId] == removedItem->sortCacheItem())
        mSortCacheItems.remove(msgId);
    }
    // Remove the message from the list of potential parents for threading by
    // subject.
    if ( mSubjThreading && removedItem->sortCacheItem()->subjectThreadingList() )
      removedItem->sortCacheItem()->subjectThreadingList()->removeAll( removedItem->sortCacheItem() );

    // Reparent children of item.
    Q3ListViewItem *myParent = removedItem;
    Q3ListViewItem *myChild = myParent->firstChild();
    Q3ListViewItem *threadRoot = myParent;
    while (threadRoot->parent())
      threadRoot = threadRoot->parent();
    QString key = static_cast<HeaderItem*>(threadRoot)->key(mSortCol, !mSortDescending);

    QList<Q3ListViewItem*> childList;
    while (myChild) {
      HeaderItem *item = static_cast<HeaderItem*>(myChild);
      // Just keep the item at top level, if it will be deleted anyhow
      if ( !item->aboutToBeDeleted() ) {
        childList.append(myChild);
      }
      myChild = myChild->nextSibling();
      if ( item->aboutToBeDeleted() ) {
        myParent->takeItem( item );
        insertItem( item );
        mRoot->addSortedChild( item->sortCacheItem() );
      }
      item->setTempKey( key + item->key( mSortCol, !mSortDescending ));
      if (mSortInfo.fakeSort) {
        QObject::disconnect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
        K3ListView::setSorting(mSortCol, !mSortDescending );
        mSortInfo.fakeSort = 0;
      }
    }

    QList<Q3ListViewItem*>::iterator it;
    for ( it = childList.begin(); it != childList.end() ; ++it ) {
      Q3ListViewItem *lvi = *it;
      HeaderItem *item = static_cast<HeaderItem*>(lvi);
      SortCacheItem *sci = item->sortCacheItem();
      SortCacheItem *parent = findParent( sci );
      if ( !parent && mSubjThreading )
        parent = findParentBySubject( sci );

      Q_ASSERT( !parent || parent->item() != removedItem );
      myParent->takeItem(lvi);
      if ( parent && parent->item() != item && parent->item() != removedItem ) {
        parent->item()->insertItem(lvi);
        parent->addSortedChild( sci );
      } else {
        insertItem(lvi);
        mRoot->addSortedChild( sci );
      }

      if ((!parent || sci->isImperfectlyThreaded())
                      && !mImperfectlyThreadedList.count( item ))
        mImperfectlyThreadedList.append(item);

      if (parent && !sci->isImperfectlyThreaded()
          && mImperfectlyThreadedList.count( item ))
        mImperfectlyThreadedList.removeAll(item);
    }
  }
  // Make sure our data structures are cleared.
  if (!mFolder->count())
      folderCleared();

  mImperfectlyThreadedList.removeAll( removedItem );
#ifdef DEBUG
  // This should never happen, in this case the folders are inconsistent.
  while ( mImperfectlyThreadedList.findRef( removedItem ) != -1 ) {
    mImperfectlyThreadedList.remove();
    kDebug(5006) << "Remove doubled item from mImperfectlyThreadedList: " << removedItem << endl;
  }
#endif
  delete removedItem;
  // we might have rethreaded it, in which case its current state will be lost
  if ( curItem ) {
    if ( curItem != removedItem ) {
      setCurrentItem( curItem );
      setSelectionAnchor( currentItem() );
    } else {
      // We've removed the current item, which means it was removed from
      // something other than a user move or copy, which would have selected
      // the next logical mail. This can happen when the mail is deleted by
      // a filter, or some other behind the scenes action. Select something
      // sensible, then, and make sure the reader window is cleared.
      emit maybeDeleting();
      int contentX, contentY;
      HeaderItem *nextItem = prepareMove( &contentX, &contentY );
      finalizeMove( nextItem, contentX, contentY );
    }
  }
  /* restore signal */
  connect( this, SIGNAL(currentChanged(Q3ListViewItem*)),
           this, SLOT(highlightMessage(Q3ListViewItem*)));
}


//-----------------------------------------------------------------------------
void KMHeaders::msgHeaderChanged(KMFolder*, int msgId)
{
  if (msgId<0 || msgId >= (int)mItems.size() || !updatesEnabled()) return;
  HeaderItem *item = mItems[msgId];
  if (item) {
    item->irefresh();
    item->repaint();
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgStatus( const MessageStatus& status, bool toggle)
{
  SerNumList serNums;
  Q3ListViewItemIterator it(this, Q3ListViewItemIterator::Selected|Q3ListViewItemIterator::Visible);
  while( it.current() ) {
    if ( it.current()->isSelected() && it.current()->isVisible() ) {
      if ( it.current()->parent() && ( !it.current()->parent()->isOpen() ) ) {
        // the item's parent is closed, don't traverse any more of this subtree
        Q3ListViewItem * lastAncestorWithSiblings = it.current()->parent();
        // travel towards the root until we find an ancestor with siblings
        while ( ( lastAncestorWithSiblings->depth() > 0 ) && !lastAncestorWithSiblings->nextSibling() )
          lastAncestorWithSiblings = lastAncestorWithSiblings->parent();
        // move the iterator to that ancestor's next sibling
        it = Q3ListViewItemIterator( lastAncestorWithSiblings->nextSibling() );
        continue;
      }

      HeaderItem *item = static_cast<HeaderItem*>(it.current());
      KMMsgBase *msgBase = mFolder->getMsgBase(item->msgId());
      serNums.append( msgBase->getMsgSerNum() );
    }
    ++it;
  }
  if (serNums.empty())
    return;

  KMCommand *command = new KMSetStatusCommand( status, serNums, toggle );
  command->start();
}


QList<Q3ListViewItem*> KMHeaders::currentThread() const
{
  if (!mFolder) return QList<Q3ListViewItem*>();

  // starting with the current item...
  Q3ListViewItem *curItem = currentItem();
  if (!curItem) return QList<Q3ListViewItem*>();

  // ...find the top-level item:
  Q3ListViewItem *topOfThread = curItem;
  while ( topOfThread->parent() )
    topOfThread = topOfThread->parent();

  // collect the items in this thread:
  QList<Q3ListViewItem*> list;
  Q3ListViewItem *topOfNextThread = topOfThread->nextSibling();
  for ( Q3ListViewItemIterator it( topOfThread ) ;
        it.current() && it.current() != topOfNextThread ; ++it )
    list.append( it.current() );
  return list;
}

void KMHeaders::setThreadStatus( const MessageStatus& status, bool toggle)
{
  QList<Q3ListViewItem*> curThread = currentThread();
  QList<Q3ListViewItem*>::const_iterator it;
  SerNumList serNums;

  for ( it = curThread.constBegin() ; it != curThread.constEnd() ; ++it ) {
    int id = static_cast<HeaderItem*>(*it)->msgId();
    KMMsgBase *msgBase = mFolder->getMsgBase( id );
    serNums.append( msgBase->getMsgSerNum() );
  }

  if (serNums.empty())
    return;

  KMCommand *command = new KMSetStatusCommand( status, serNums, toggle );
  command->start();
}

//-----------------------------------------------------------------------------
int KMHeaders::slotFilterMsg(KMMessage *msg)
{
  if ( !msg ) return 2; // messageRetrieve(0) is always possible
  msg->setTransferInProgress(false);
  int filterResult = kmkernel->filterMgr()->process(msg,KMFilterMgr::Explicit);
  if (filterResult == 2) {
    // something went horribly wrong (out of space?)
    kmkernel->emergencyExit( i18n("Unable to process messages: " ) + QString::fromLocal8Bit(strerror(errno)));
    return 2;
  }
  if (msg->parent()) { // unGet this msg
    int idx = -1;
    KMFolder * p = 0;
    KMMsgDict::instance()->getLocation( msg, &p, &idx );
    assert( p == msg->parent() ); assert( idx >= 0 );
    p->unGetMsg( idx );
  }

  return filterResult;
}


void KMHeaders::slotExpandOrCollapseThread( bool expand )
{
  if ( !isThreaded() ) return;
  // find top-level parent of currentItem().
  Q3ListViewItem *item = currentItem();
  if ( !item ) return;
  clearSelection();
  item->setSelected( true );
  while ( item->parent() )
    item = item->parent();
  HeaderItem * hdrItem = static_cast<HeaderItem*>(item);
  hdrItem->setOpenRecursive( expand );
  if ( !expand ) // collapse can hide the current item:
    setCurrentMsg( hdrItem->msgId() );
  ensureItemVisible( currentItem() );
}

void KMHeaders::slotExpandOrCollapseAllThreads( bool expand )
{
  if ( !isThreaded() ) return;

  Q3ListViewItem * item = currentItem();
  if( item ) {
    clearSelection();
    item->setSelected( true );
  }

  for ( Q3ListViewItem *item = firstChild() ;
        item ; item = item->nextSibling() )
    static_cast<HeaderItem*>(item)->setOpenRecursive( expand );
  if ( !expand ) { // collapse can hide the current item:
    Q3ListViewItem * item = currentItem();
    if( item ) {
      while ( item->parent() )
        item = item->parent();
      setCurrentMsg( static_cast<HeaderItem*>(item)->msgId() );
    }
  }
  ensureItemVisible( currentItem() );
}

//-----------------------------------------------------------------------------
void KMHeaders::setStyleDependantFrameWidth()
{
  // set the width of the frame to a reasonable value for the current GUI style
  int frameWidth;
#if 0 // is this hack still needed with kde4?
  if( !qstrcmp( style()->metaObject()->className(), "KeramikStyle" ) )
    frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth ) - 1;
  else
#endif
    frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  if ( frameWidth < 0 )
    frameWidth = 0;
  if ( frameWidth != lineWidth() )
    setLineWidth( frameWidth );
}

//-----------------------------------------------------------------------------
void KMHeaders::styleChange( QStyle& oldStyle )
{
  setStyleDependantFrameWidth();
  K3ListView::styleChange( oldStyle );
}

//-----------------------------------------------------------------------------
void KMHeaders::setFolderInfoStatus ()
{
  if ( !mFolder ) return;
  QString str;
  const int unread = mFolder->countUnread();
  if ( static_cast<KMFolder*>(mFolder) == kmkernel->outboxFolder() )
    str = unread ? i18np( "1 unsent", "%1 unsent", unread ) : i18n( "0 unsent" );
  else
    str = unread ? i18np( "1 unread", "%1 unread", unread ) : i18n( "0 unread" );
  const int count = mFolder->count();
  str = count ? i18np( "1 message, %2.", "%1 messages, %2.", count, str )
              : i18n( "0 messages" ); // no need for "0 unread" to be added here
  if ( mFolder->isReadOnly() )
    str = i18nc("%1 = n messages, m unread.", "%1 Folder is read-only.", str );
  BroadcastStatus::instance()->setStatusMsg(str);
}

//-----------------------------------------------------------------------------
void KMHeaders::applyFiltersOnMsg()
{
  if (ActionScheduler::isEnabled() ||
      kmkernel->filterMgr()->atLeastOneOnlineImapFolderTarget()) {
    // uses action scheduler
    KMFilterMgr::FilterSet set = KMFilterMgr::Explicit;
    QList<KMFilter*> filters = kmkernel->filterMgr()->filters();
    ActionScheduler *scheduler = new ActionScheduler( set, filters, this );
    scheduler->setAutoDestruct( true );

    int contentX, contentY;
    HeaderItem *nextItem = prepareMove( &contentX, &contentY );
    KMMessageList msgList = *selectedMsgs(true);
    finalizeMove( nextItem, contentX, contentY );

    KMMsgBase *msg;
    foreach( msg, msgList )
      scheduler->execFilters( msg );
  } else {
    int contentX, contentY;
    HeaderItem *nextItem = prepareMove( &contentX, &contentY );

    KMMessageList* msgList = selectedMsgs();
    if (msgList->isEmpty())
      return;
    finalizeMove( nextItem, contentX, contentY );

    CREATE_TIMER(filter);
    START_TIMER(filter);

    KCursorSaver busy( KBusyPtr::busy() );
    int msgCount = 0;
    int msgCountToFilter = msgList->count();
    ProgressItem* progressItem =
      ProgressManager::createProgressItem (
          "filter"+ProgressManager::getUniqueID(),
          i18n( "Filtering messages" ) );
    progressItem->setTotalItems( msgCountToFilter );
    QList<KMMsgBase*>::const_iterator it;
    for ( it = msgList->begin(); it != msgList->end(); it++ ) {
      KMMsgBase* msgBase = (*it);
      int diff = msgCountToFilter - ++msgCount;
      if ( diff < 10 || !( msgCount % 10 ) || msgCount <= 10 ) {
        progressItem->updateProgress();
        QString statusMsg = i18n( "Filtering message %1 of %2",
                                  msgCount, msgCountToFilter );
        KPIM::BroadcastStatus::instance()->setStatusMsg( statusMsg );
        qApp->processEvents( QEventLoop::ExcludeUserInputEvents, 50 );
      }
      int idx = msgBase->parent()->find(msgBase);
      assert(idx != -1);
      KMMessage * msg = msgBase->parent()->getMsg(idx);
      if (msg->transferInProgress()) continue;
      msg->setTransferInProgress(true);
      if ( !msg->isComplete() )
      {
	FolderJob *job = mFolder->createJob(msg);
	connect(job, SIGNAL(messageRetrieved(KMMessage*)),
		SLOT(slotFilterMsg(KMMessage*)));
	job->start();
      } else {
	if (slotFilterMsg(msg) == 2) break;
      }
      progressItem->incCompletedItems();
    }
    progressItem->setComplete();
    progressItem = 0;
    END_TIMER(filter);
    SHOW_TIMER(filter);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgRead (int msgId)
{
  KMMsgBase *msgBase = mFolder->getMsgBase( msgId );
  if (!msgBase)
    return;

  SerNumList serNums;
  if (msgBase->status().isNew() || msgBase->status().isUnread()) {
    serNums.append( msgBase->getMsgSerNum() );
  }

  KMCommand *command = new KMSetStatusCommand( MessageStatus::statusRead(), serNums );
  command->start();
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg ()
{
  //make sure we have an associated folder (root of folder tree does not).
  if (!mFolder)
    return;

  int contentX, contentY;
  HeaderItem *nextItem = prepareMove( &contentX, &contentY );
  KMMessageList msgList = *selectedMsgs(true);
  finalizeMove( nextItem, contentX, contentY );

  KMCommand *command = new KMDeleteMsgCommand( mFolder, msgList );
  connect( command, SIGNAL( completed( KMCommand * ) ),
           this, SLOT( slotMoveCompleted( KMCommand * ) ) );
  command->start();

  BroadcastStatus::instance()->setStatusMsg("");
  //  triggerUpdate();
}


//-----------------------------------------------------------------------------
void KMHeaders::moveSelectedToFolder( QAction* act )
{
  if (mMenuToFolder[act])
    moveMsgToFolder( mMenuToFolder[act] );
}

//-----------------------------------------------------------------------------
HeaderItem* KMHeaders::prepareMove( int *contentX, int *contentY )
{
  HeaderItem *ret = 0;
  emit maybeDeleting();

  disconnect( this, SIGNAL(currentChanged(Q3ListViewItem*)),
              this, SLOT(highlightMessage(Q3ListViewItem*)));

  Q3ListViewItem *curItem;
  HeaderItem *item;
  curItem = currentItem();
  while (curItem && curItem->isSelected() && curItem->itemBelow())
    curItem = curItem->itemBelow();
  while (curItem && curItem->isSelected() && curItem->itemAbove())
    curItem = curItem->itemAbove();
  item = static_cast<HeaderItem*>(curItem);

  *contentX = contentsX();
  *contentY = contentsY();

  if (item  && !item->isSelected())
    ret = item;

  return ret;
}

//-----------------------------------------------------------------------------
void KMHeaders::finalizeMove( HeaderItem *item, int contentX, int contentY )
{
  emit selected( 0 );
  clearSelection();

  if ( item ) {
    setCurrentItem( item );
    setSelected( item, true );
    setSelectionAnchor( currentItem() );
    mPrevCurrent = 0;
    highlightMessage( item, false);
  }

  setContentsPos( contentX, contentY );
  makeHeaderVisible();
  connect( this, SIGNAL(currentChanged(Q3ListViewItem*)),
           this, SLOT(highlightMessage(Q3ListViewItem*)));
}


//-----------------------------------------------------------------------------
void KMHeaders::moveMsgToFolder ( KMFolder* destFolder, bool askForConfirmation )
{
  if ( destFolder == mFolder ) return; // Catch the noop case

  KMMessageList msgList = *selectedMsgs();
  if ( msgList.isEmpty( ) ) return;
  if ( !destFolder && askForConfirmation &&    // messages shall be deleted
       KMessageBox::warningContinueCancel(this,
         i18np("<qt>Do you really want to delete the selected message?<br>"
              "Once deleted, it cannot be restored.</qt>",
              "<qt>Do you really want to delete the %1 selected messages?<br>"
              "Once deleted, they cannot be restored.</qt>", msgList.count() ),
	 msgList.count()>1 ? i18n("Delete Messages") : i18n("Delete Message"), KStandardGuiItem::del(),
	 "NoConfirmDelete") == KMessageBox::Cancel )
    return;  // user canceled the action

  // remember the message to select afterwards
  int contentX, contentY;
  HeaderItem *nextItem = prepareMove( &contentX, &contentY );
  msgList = *selectedMsgs(true);
  finalizeMove( nextItem, contentX, contentY );

  KMCommand *command = new KMMoveCommand( destFolder, msgList );
  connect( command, SIGNAL( completed( KMCommand * ) ),
           this, SLOT( slotMoveCompleted( KMCommand * ) ) );
  command->start();

}

void KMHeaders::slotMoveCompleted( KMCommand *command )
{
  kDebug(5006) << k_funcinfo << command->result() << endl;
  bool deleted = static_cast<KMMoveCommand *>( command )->destFolder() == 0;
  if ( command->result() == KMCommand::OK ) {
    // make sure the current item is shown
    makeHeaderVisible();
    BroadcastStatus::instance()->setStatusMsg(
       deleted ? i18n("Messages deleted successfully.") : i18n("Messages moved successfully") );
  } else {
    /* The move failed or the user canceled it; reset the state of all
     * messages involved and repaint.
     *
     * Note: This potentially resets too many items if there is more than one
     *       move going on. Oh well, I suppose no animals will be harmed.
     * */
    for (Q3ListViewItemIterator it(this); it.current(); it++) {
      HeaderItem *item = static_cast<HeaderItem*>(it.current());
      if ( item->aboutToBeDeleted() ) {
        item->setAboutToBeDeleted ( false );
        item->setSelectable ( true );
        KMMsgBase *msgBase = mFolder->getMsgBase(item->msgId());
        if ( msgBase->isMessage() ) {
          KMMessage *msg = static_cast<KMMessage *>(msgBase);
          if ( msg ) msg->setTransferInProgress( false, true );
        }
      }
    }
    triggerUpdate();
    if ( command->result() == KMCommand::Failed )
      BroadcastStatus::instance()->setStatusMsg(
           deleted ? i18n("Deleting messages failed.") : i18n("Moving messages failed.") );
    else
      BroadcastStatus::instance()->setStatusMsg(
           deleted ? i18n("Deleting messages canceled.") : i18n("Moving messages canceled.") );
 }
 mOwner->updateMessageActions();
}

bool KMHeaders::canUndo() const
{
    return ( kmkernel->undoStack()->size() > 0 );
}

//-----------------------------------------------------------------------------
void KMHeaders::undo()
{
  kmkernel->undoStack()->undo();
}

//-----------------------------------------------------------------------------
void KMHeaders::copySelectedToFolder( QAction* act )
{
  if (mMenuToFolder[act])
    copyMsgToFolder( mMenuToFolder[act] );
}


//-----------------------------------------------------------------------------
void KMHeaders::copyMsgToFolder(KMFolder* destFolder, KMMessage* aMsg)
{
  if ( !destFolder )
    return;

  KMCommand * command = 0;
  if (aMsg)
    command = new KMCopyCommand( destFolder, aMsg );
  else {
    KMMessageList msgList = *selectedMsgs();
    command = new KMCopyCommand( destFolder, msgList );
  }

  command->start();
}


//-----------------------------------------------------------------------------
void KMHeaders::setCurrentMsg(int cur)
{
  if (!mFolder) return;
  if (cur >= mFolder->count()) cur = mFolder->count() - 1;
  if ((cur >= 0) && (cur < (int)mItems.size())) {
    clearSelection();
    setCurrentItem( mItems[cur] );
    setSelected( mItems[cur], true );
    setSelectionAnchor( currentItem() );
  }
  makeHeaderVisible();
  setFolderInfoStatus();
}

//-----------------------------------------------------------------------------
void KMHeaders::setSelected( Q3ListViewItem *item, bool selected )
{
  if ( !item )
    return;

  if ( item->isVisible() )
    K3ListView::setSelected( item, selected );

  // If the item is the parent of a closed thread recursively select
  // children .
  if ( isThreaded() && !item->isOpen() && item->firstChild() ) {
      Q3ListViewItem *nextRoot = item->itemBelow();
      Q3ListViewItemIterator it( item->firstChild() );
      for( ; (*it) != nextRoot; ++it ) {
        if ( (*it)->isVisible() )
           (*it)->setSelected( selected );
      }
  }
}

void KMHeaders::setSelectedByIndex( QList<int> items, bool selected )
{
  for ( QList<int>::Iterator it = items.begin(); it != items.end(); ++it )
  {
    if ( ((*it) >= 0) && ((*it) < (int)mItems.size()) )
    {
      setSelected( mItems[(*it)], selected );
    }
  }
}

void KMHeaders::clearSelectableAndAboutToBeDeleted( quint32 serNum )
{
  // fugly, but I see no way around it
  for (Q3ListViewItemIterator it(this); it.current(); it++) {
    HeaderItem *item = static_cast<HeaderItem*>(it.current());
    if ( item->aboutToBeDeleted() ) {
      KMMsgBase *msgBase = mFolder->getMsgBase( item->msgId() );
      if ( serNum == msgBase->getMsgSerNum() ) {
        item->setAboutToBeDeleted ( false );
        item->setSelectable ( true );
      }
    }
  }
  triggerUpdate();
}

//-----------------------------------------------------------------------------
KMMessageList* KMHeaders::selectedMsgs(bool toBeDeleted)
{
  mSelMsgBaseList.clear();
  for (Q3ListViewItemIterator it(this); it.current(); it++) {
    if ( it.current()->isSelected() && it.current()->isVisible() ) {
      HeaderItem *item = static_cast<HeaderItem*>(it.current());
      if ( !item->aboutToBeDeleted() ) { // we are already working on this one
        if (toBeDeleted) {
          // make sure the item is not uselessly rethreaded and not selectable
          item->setAboutToBeDeleted ( true );
          item->setSelectable ( false );
        }
        KMMsgBase *msgBase = mFolder->getMsgBase(item->msgId());
        mSelMsgBaseList.append(msgBase);
      }
    }
  }
  return &mSelMsgBaseList;
}

//-----------------------------------------------------------------------------
QList<int> KMHeaders::selectedItems()
{
  QList<int> items;
  for ( Q3ListViewItemIterator it(this); it.current(); it++ )
  {
    if ( it.current()->isSelected() && it.current()->isVisible() )
    {
      HeaderItem* item = static_cast<HeaderItem*>( it.current() );
      items.append( item->msgId() );
    }
  }
  return items;
}

//-----------------------------------------------------------------------------
int KMHeaders::firstSelectedMsg() const
{
  int selectedMsg = -1;
  Q3ListViewItem *item;
  for (item = firstChild(); item; item = item->itemBelow())
    if (item->isSelected()) {
      selectedMsg = (static_cast<HeaderItem*>(item))->msgId();
      break;
    }
  return selectedMsg;
}

//-----------------------------------------------------------------------------
void KMHeaders::nextMessage()
{
  Q3ListViewItem *lvi = currentItem();
  if (lvi && lvi->itemBelow()) {
    clearSelection();
    setSelected( lvi, false );
    selectNextMessage();
    setSelectionAnchor( currentItem() );
    ensureCurrentItemVisible();
  }
}

void KMHeaders::selectNextMessage()
{
  Q3ListViewItem *lvi = currentItem();
  if( lvi ) {
    Q3ListViewItem *below = lvi->itemBelow();
    Q3ListViewItem *temp = lvi;
    if (lvi && below ) {
      while (temp) {
        temp->firstChild();
        temp = temp->parent();
      }
      lvi->repaint();
      /* test to see if we need to unselect messages on back track */
      (below->isSelected() ? setSelected(lvi, false) : setSelected(below, true));
      setCurrentItem(below);
      makeHeaderVisible();
      setFolderInfoStatus();
    }
  }
}

//-----------------------------------------------------------------------------
void KMHeaders::prevMessage()
{
  Q3ListViewItem *lvi = currentItem();
  if (lvi && lvi->itemAbove()) {
    clearSelection();
    setSelected( lvi, false );
    selectPrevMessage();
    setSelectionAnchor( currentItem() );
    ensureCurrentItemVisible();
  }
}

void KMHeaders::selectPrevMessage()
{
  Q3ListViewItem *lvi = currentItem();
  if( lvi ) {
    Q3ListViewItem *above = lvi->itemAbove();
    Q3ListViewItem *temp = lvi;

    if (lvi && above) {
      while (temp) {
        temp->firstChild();
        temp = temp->parent();
      }
      lvi->repaint();
      /* test to see if we need to unselect messages on back track */
      (above->isSelected() ? setSelected(lvi, false) : setSelected(above, true));
      setCurrentItem(above);
      makeHeaderVisible();
      setFolderInfoStatus();
    }
  }
}


void KMHeaders::incCurrentMessage()
{
  Q3ListViewItem *lvi = currentItem();
  if ( lvi && lvi->itemBelow() ) {

    disconnect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
               this,SLOT(highlightMessage(Q3ListViewItem*)));
    setCurrentItem( lvi->itemBelow() );
    ensureCurrentItemVisible();
    setFocus();
    connect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
               this,SLOT(highlightMessage(Q3ListViewItem*)));
  }
}

void KMHeaders::decCurrentMessage()
{
  Q3ListViewItem *lvi = currentItem();
  if ( lvi && lvi->itemAbove() ) {
    disconnect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
               this,SLOT(highlightMessage(Q3ListViewItem*)));
    setCurrentItem( lvi->itemAbove() );
    ensureCurrentItemVisible();
    setFocus();
    connect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
            this,SLOT(highlightMessage(Q3ListViewItem*)));
  }
}

void KMHeaders::selectCurrentMessage()
{
  setCurrentMsg( currentItemIndex() );
  highlightMessage( currentItem() );
}

//-----------------------------------------------------------------------------
void KMHeaders::findUnreadAux( HeaderItem*& item,
                                        bool & foundUnreadMessage,
                                        bool onlyNew,
                                        bool aDirNext )
{
  KMMsgBase* msgBase = 0;
  HeaderItem *lastUnread = 0;
  /* itemAbove() is _slow_ */
  if (aDirNext)
  {
    while (item) {
      msgBase = mFolder->getMsgBase(item->msgId());
      if (!msgBase) continue;
      MessageStatus status = msgBase->status();
      if ( status.isUnread() || status.isNew() )
        foundUnreadMessage = true;

      if ( !onlyNew && ( status.isUnread() || status.isNew() ) )
        break;
      if ( onlyNew && status.isNew() )
        break;
      item = static_cast<HeaderItem*>(item->itemBelow());
    }
  } else {
    HeaderItem *newItem = static_cast<HeaderItem*>(firstChild());
    while (newItem)
    {
      msgBase = mFolder->getMsgBase(newItem->msgId());
      if (!msgBase) continue;
      MessageStatus status = msgBase->status();
      if ( status.isUnread() || status.isNew() )
        foundUnreadMessage = true;
      if ( !onlyNew && ( status.isUnread() || status.isNew() )
          || onlyNew && status.isNew())
        lastUnread = newItem;
      if (newItem == item) break;
      newItem = static_cast<HeaderItem*>(newItem->itemBelow());
    }
    item = lastUnread;
  }
}

//-----------------------------------------------------------------------------
int KMHeaders::findUnread(bool aDirNext, int aStartAt, bool onlyNew, bool acceptCurrent)
{
  HeaderItem *item, *pitem;
  bool foundUnreadMessage = false;

  if (!mFolder) return -1;
  if (mFolder->count() == -1) return -1;

  if ((aStartAt >= 0) && (aStartAt < (int)mItems.size()))
    item = mItems[aStartAt];
  else {
    item = currentHeaderItem();
    if (!item) {
      if (aDirNext)
        item = static_cast<HeaderItem*>(firstChild());
      else
        item = static_cast<HeaderItem*>(lastChild());
    }
    if (!item)
      return -1;

    if ( !acceptCurrent )
        if (aDirNext)
            item = static_cast<HeaderItem*>(item->itemBelow());
        else
            item = static_cast<HeaderItem*>(item->itemAbove());
  }

  pitem =  item;

  findUnreadAux( item, foundUnreadMessage, onlyNew, aDirNext );

  // We have found an unread item, but it is not necessary the
  // first unread item.
  //
  // Find the ancestor of the unread item closest to the
  // root and recursively sort all of that ancestors children.
  if (item) {
    Q3ListViewItem *next = item;
    while (next->parent())
      next = next->parent();
    next = static_cast<HeaderItem*>(next)->firstChildNonConst();
    while (next && (next != item))
      if (static_cast<HeaderItem*>(next)->firstChildNonConst())
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
bool KMHeaders::nextUnreadMessage(bool acceptCurrent)
{
  if ( !mFolder || !mFolder->countUnread() ) return false;
  int i = findUnread(true, -1, false, acceptCurrent);
  if ( i < 0 && GlobalSettings::self()->loopOnGotoUnread() !=
        GlobalSettings::EnumLoopOnGotoUnread::DontLoop )
  {
    HeaderItem * first = static_cast<HeaderItem*>(firstChild());
    if ( first )
      i = findUnread(true, first->msgId(), false, acceptCurrent); // from top
  }
  if ( i < 0 )
    return false;
  setCurrentMsg(i);
  ensureCurrentItemVisible();
  return true;
}

void KMHeaders::ensureCurrentItemVisible()
{
    int i = currentItemIndex();
    if ((i >= 0) && (i < (int)mItems.size()))
        center( contentsX(), itemPos(mItems[i]), 0, 9.0 );
}

//-----------------------------------------------------------------------------
bool KMHeaders::prevUnreadMessage()
{
  if ( !mFolder || !mFolder->countUnread() ) return false;
  int i = findUnread(false);
  if ( i < 0 && GlobalSettings::self()->loopOnGotoUnread() !=
        GlobalSettings::EnumLoopOnGotoUnread::DontLoop )
  {
    HeaderItem * last = static_cast<HeaderItem*>(lastItem());
    if ( last )
      i = findUnread(false, last->msgId() ); // from bottom
  }
  if ( i < 0 )
    return false;
  setCurrentMsg(i);
  ensureCurrentItemVisible();
  return true;
}


//-----------------------------------------------------------------------------
void KMHeaders::slotNoDrag()
{
  mMousePressed = false;
}


//-----------------------------------------------------------------------------
void KMHeaders::makeHeaderVisible()
{
  if (currentItem())
    ensureItemVisible( currentItem() );
}

//-----------------------------------------------------------------------------
void KMHeaders::highlightMessage(Q3ListViewItem* lvi, bool markitread)
{
  // shouldn't happen but will crash if it does
  if (lvi && !lvi->isSelectable()) return;

  HeaderItem *item = static_cast<HeaderItem*>(lvi);
  if (lvi != mPrevCurrent) {
    if (mPrevCurrent && mFolder)
    {
      KMMessage *prevMsg = mFolder->getMsg(mPrevCurrent->msgId());
      if (prevMsg && mReaderWindowActive)
      {
        mFolder->ignoreJobsForMessage(prevMsg);
        if (!prevMsg->transferInProgress())
          mFolder->unGetMsg(mPrevCurrent->msgId());
      }
    }
    mPrevCurrent = item;
  }

  if (!item) {
    emit selected( 0 ); return;
  }

  int idx = item->msgId();
  if (mReaderWindowActive) {
    KMMessage *msg = mFolder->getMsg(idx);
    if (!msg ) {
      emit selected( 0 );
      mPrevCurrent = 0;
      return;
    }
  }

  BroadcastStatus::instance()->setStatusMsg("");
  if (markitread && idx >= 0) setMsgRead(idx);
  mItems[idx]->irefresh();
  mItems[idx]->repaint();
  emit selected( mFolder->getMsg(idx) );
  setFolderInfoStatus();
}

void KMHeaders::highlightCurrentThread()
{
  QList<Q3ListViewItem*> curThread = currentThread();
  QList<Q3ListViewItem*>::const_iterator it;

  for ( it = curThread.constBegin() ; it != curThread.constEnd() ; ++it ) {
      Q3ListViewItem *lvi = *it;
      lvi->setSelected( true );
      lvi->repaint();
  }
}

void KMHeaders::resetCurrentTime()
{
    mDate.reset();
    QTimer::singleShot( 1000, this, SLOT( resetCurrentTime() ) );
}

//-----------------------------------------------------------------------------
void KMHeaders::selectMessage(Q3ListViewItem* lvi)
{
  HeaderItem *item = static_cast<HeaderItem*>(lvi);
  if (!item)
    return;

  int idx = item->msgId();
  KMMessage *msg = mFolder->getMsg(idx);
  if (msg && !msg->transferInProgress())
  {
    emit activated(mFolder->getMsg(idx));
  }

//  if (kmkernel->folderIsDraftOrOutbox(mFolder))
//    setOpen(lvi, !lvi->isOpen());
}


//-----------------------------------------------------------------------------
void KMHeaders::updateMessageList( bool set_selection, bool forceJumpToUnread )
{
  mPrevCurrent = 0;
  noRepaint = true;
  clear();
  noRepaint = false;
  K3ListView::setSorting( mSortCol, !mSortDescending );
  if (!mFolder) {
    mItems.resize(0);
    repaint();
    return;
  }
  readSortOrder( set_selection, forceJumpToUnread );
  emit messageListUpdated();
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
// Exception: When shift selecting either with mouse or key press the reader
// window is updated regardless of whether of not the selection has changed.
void KMHeaders::keyPressEvent( QKeyEvent * e )
{
    bool cntrl = (e->modifiers() & Qt::ControlModifier );
    bool shft = (e->modifiers() & Qt::ShiftModifier );
    Q3ListViewItem *cur = currentItem();

    if (!firstChild())
      return;

    // If no current item, make some first item current when a key is pressed
    if (!cur) {
      setCurrentItem( firstChild() );
      setSelectionAnchor( currentItem() );
      return;
    }

    // Handle space key press
    if (cur->isSelectable() && e->text() == " " ) {
        setSelected( cur, !cur->isSelected() );
        highlightMessage( cur, false);
        return;
    }

    if (cntrl) {
      if (!shft)
        disconnect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
                   this,SLOT(highlightMessage(Q3ListViewItem*)));
      switch (e->key()) {
      case Qt::Key_Down:
      case Qt::Key_Up:
      case Qt::Key_Home:
      case Qt::Key_End:
      case Qt::Key_PageDown:
      case Qt::Key_PageUp:
      case Qt::Key_Escape:
        K3ListView::keyPressEvent( e );
      }
      if (!shft)
        connect(this,SIGNAL(currentChanged(Q3ListViewItem*)),
                this,SLOT(highlightMessage(Q3ListViewItem*)));
    }
}

//-----------------------------------------------------------------------------
// Handle RMB press, show pop up menu
void KMHeaders::rightButtonPressed( Q3ListViewItem *lvi, const QPoint &, int )
{
  if (!lvi)
    return;

  if (!(lvi->isSelected())) {
    clearSelection();
  }
  setSelected( lvi, true );
  slotRMB();
}

//-----------------------------------------------------------------------------
void KMHeaders::contentsMousePressEvent(QMouseEvent* e)
{
  mPressPos = e->pos();
  Q3ListViewItem *lvi = itemAt( contentsToViewport( e->pos() ));
  bool wasSelected = false;
  bool rootDecoClicked = false;
  if (lvi) {
     wasSelected = lvi->isSelected();
     rootDecoClicked =
        (  mPressPos.x() <= header()->cellPos(  header()->mapToActual(  0 ) ) +
           treeStepSize() * (  lvi->depth() + (  rootIsDecorated() ? 1 : 0 ) ) + itemMargin() )
        && (  mPressPos.x() >= header()->cellPos(  header()->mapToActual(  0 ) ) );

     if ( rootDecoClicked ) {
        // Check if our item is the parent of a closed thread and if so, if the root
        // decoration of the item was clicked (i.e. the +/- sign) which would expand
        // the thread. In that case, deselect all children, so opening the thread
        // doesn't cause a flicker.
        if ( !lvi->isOpen() && lvi->firstChild() ) {
           Q3ListViewItem *nextRoot = lvi->itemBelow();
           Q3ListViewItemIterator it( lvi->firstChild() );
           for( ; (*it) != nextRoot; ++it )
              (*it)->setSelected( false );
        }
     }
  }

  // let klistview do it's thing, expanding/collapsing, selection/deselection
  K3ListView::contentsMousePressEvent(e);
  /* QListView's shift-select selects also invisible items. Until that is
     fixed, we have to deselect hidden items here manually, so the quick
     search doesn't mess things up. */
  if ( e->modifiers() & Qt::ShiftModifier ) {
    Q3ListViewItemIterator it( this, Q3ListViewItemIterator::Invisible );
    while ( it.current() ) {
      it.current()->setSelected( false );
      ++it;
    }
  }

  if ( rootDecoClicked ) {
      // select the thread's children after closing if the parent is selected
     if ( lvi && !lvi->isOpen() && lvi->isSelected() )
        setSelected( lvi, true );
  }

  if ( lvi && !rootDecoClicked ) {
    if ( lvi != currentItem() )
      highlightMessage( lvi );
    /* Explicitely set selection state. This is necessary because we want to
     * also select all children of closed threads when the parent is selected. */

    // unless ctrl mask, set selected if it isn't already
    if ( !( e->modifiers() & Qt::ControlModifier ) && !wasSelected )
      setSelected( lvi, true );
    // if ctrl mask, toggle selection
    if ( e->modifiers() & Qt::ControlModifier )
      setSelected( lvi, !wasSelected );

    if ((e->button() == Qt::LeftButton) )
      mMousePressed = true;
  }
}

//-----------------------------------------------------------------------------
void KMHeaders::contentsMouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() != Qt::RightButton)
    K3ListView::contentsMouseReleaseEvent(e);

  mMousePressed = false;
}

//-----------------------------------------------------------------------------
void KMHeaders::contentsMouseMoveEvent( QMouseEvent* e )
{
  if (mMousePressed &&
      (e->pos() - mPressPos).manhattanLength() > KGlobalSettings::dndEventDelay()) {
    mMousePressed = false;
    Q3ListViewItem *item = itemAt( contentsToViewport(mPressPos) );
    if ( item ) {
      MailList mailList;
      unsigned int count = 0;
      for( Q3ListViewItemIterator it(this); it.current(); it++ )
        if( it.current()->isSelected() ) {
          HeaderItem *item = static_cast<HeaderItem*>(it.current());
          KMMsgBase *msg = mFolder->getMsgBase(item->msgId());
          // FIXME: msg can be null here which crashes.  I think it's a race
          //        because it's very hard to reproduce. (GS)
          MailSummary mailSummary( msg->getMsgSerNum(), msg->msgIdMD5(),
                                   msg->subject(), msg->fromStrip(),
                                   msg->toStrip(), msg->date() );
          mailList.append( mailSummary );
          ++count;
        }
      QDrag *drag = new QDrag( viewport() );
      drag->setMimeData( new QMimeData );
      mailList.populateMimeData( drag->mimeData(), new KMTextSource );

      // Set pixmap
      QPixmap pixmap;
      if( count == 1 )
        pixmap = QPixmap( DesktopIcon("message", K3Icon::SizeSmall) );
      else
        pixmap = QPixmap( DesktopIcon("kmultiple", K3Icon::SizeSmall) );

      // Calculate hotspot (as in Konqueror)
      if( !pixmap.isNull() ) {
        drag->setHotSpot( QPoint( pixmap.width() / 2, pixmap.height() / 2 ) );
        drag->setPixmap( pixmap );
      }
      drag->start();
    }
  }
}

void KMHeaders::highlightMessage(Q3ListViewItem* i)
{
    highlightMessage( i, false );
}

//-----------------------------------------------------------------------------
void KMHeaders::slotRMB()
{
  if (!topLevelWidget()) return; // safe bet

  QMenu *menu = new QMenu(this);

  mMenuToFolder.clear();

  mOwner->updateMessageMenu();

  bool out_folder = kmkernel->folderIsDraftOrOutbox( mFolder );
  bool tem_folder = kmkernel->folderIsTemplates( mFolder );
  if ( out_folder ) {
     menu->addAction( mOwner->editAction() );
  }
  else if (tem_folder) {
     menu->addAction( mOwner->useAction() );
     menu->addAction( mOwner->editAction() );
  }
  else {
     // show most used actions
     if( !mFolder->isSent() )
       menu->addAction( mOwner->replyMenu() );
     menu->addAction( mOwner->forwardMenu() );
     if(mOwner->sendAgainAction()->isEnabled()) {
       menu->addAction( mOwner->sendAgainAction() );
     }
  }
  menu->addSeparator();

  QMenu *msgCopyMenu = new QMenu(menu);
  mOwner->folderTree()->folderToPopupMenu( KMFolderTree::CopyMessage, this,
      &mMenuToFolder, msgCopyMenu );
  msgCopyMenu->setTitle(i18n("&Copy To"));
  menu->addMenu( msgCopyMenu );

  if ( mFolder->isReadOnly() ) {
    QAction* act = menu->addAction( i18n("&Move To") );
    act->setEnabled( false );
  } else {
    QMenu *msgMoveMenu = new QMenu(menu);
    mOwner->folderTree()->folderToPopupMenu( KMFolderTree::MoveMessage, this,
        &mMenuToFolder, msgMoveMenu );
    msgMoveMenu->setTitle(i18n("&Move To"));
    menu->addMenu( msgMoveMenu) ;
  }
  menu->addSeparator();
  menu->addAction( mOwner->statusMenu() ); // Mark Message menu
  if ( mOwner->threadStatusMenu()->isEnabled() ) {
    menu->addAction( mOwner->threadStatusMenu() ); // Mark Thread menu
  }

  if ( !out_folder ) {
    menu->addSeparator();
    menu->addAction( mOwner->filterMenu() ); // Create Filter menu
    menu->addAction( mOwner->action("apply_filter_actions") );
  }

  menu->addSeparator();
  menu->addAction( mOwner->printAction() );
  menu->addAction( mOwner->saveAsAction() );
  menu->addAction( mOwner->saveAttachmentsAction() );
  menu->addSeparator();
  if ( mFolder->isTrash() ) {
    menu->addAction( mOwner->deleteAction() );
    if ( mOwner->trashThreadAction()->isEnabled() )
      menu->addAction( mOwner->deleteThreadAction() );
  } else {
    menu->addAction( mOwner->trashAction() );
    if ( mOwner->trashThreadAction()->isEnabled() )
      menu->addAction( mOwner->trashThreadAction() );
  }
  KAcceleratorManager::manage(menu);
  kmkernel->setContextMenuShown( true );
  menu->exec(QCursor::pos(), 0);
  kmkernel->setContextMenuShown( false );
  delete menu;
}

//-----------------------------------------------------------------------------
KMMessage* KMHeaders::currentMsg()
{
  HeaderItem *hi = currentHeaderItem();
  if (!hi)
    return 0;
  else
    return mFolder->getMsg(hi->msgId());
}

//-----------------------------------------------------------------------------
HeaderItem* KMHeaders::currentHeaderItem()
{
  return static_cast<HeaderItem*>(currentItem());
}

//-----------------------------------------------------------------------------
int KMHeaders::currentItemIndex()
{
  HeaderItem* item = currentHeaderItem();
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
    setSelected( mItems[msgIdx], true );
    setSelectionAnchor( currentItem() );
    if (unchanged)
       highlightMessage( mItems[msgIdx], false);
  }
}

//-----------------------------------------------------------------------------
int KMHeaders::topItemIndex()
{
  HeaderItem *item = static_cast<HeaderItem*>( itemAt( QPoint( 1, 1 ) ) );
  if ( item )
    return item->msgId();
  else
    return -1;
}

//-----------------------------------------------------------------------------
void KMHeaders::setTopItemByIndex( int aMsgIdx)
{
  if ( aMsgIdx < 0 || static_cast<unsigned int>( aMsgIdx ) >= mItems.size() )
    return;
  const Q3ListViewItem * const item = mItems[aMsgIdx];
  if ( item )
    setContentsPos( 0, itemPos( item ) );
}

//-----------------------------------------------------------------------------
void KMHeaders::setNestedOverride( bool override )
{
  mSortInfo.dirty = true;
  mNestedOverride = override;
  setRootIsDecorated( nestingPolicy != AlwaysOpen
                      && isThreaded() );
  QString sortFile = mFolder->indexLocation() + ".sorted";
  unlink(QFile::encodeName(sortFile));
  reset();
}

//-----------------------------------------------------------------------------
void KMHeaders::setSubjectThreading( bool aSubjThreading )
{
  mSortInfo.dirty = true;
  mSubjThreading = aSubjThreading;
  QString sortFile = mFolder->indexLocation() + ".sorted";
  unlink(QFile::encodeName(sortFile));
  reset();
}

//-----------------------------------------------------------------------------
void KMHeaders::setOpen( Q3ListViewItem *item, bool open )
{
  if ((nestingPolicy != AlwaysOpen)|| open)
      ((HeaderItem*)item)->setOpenRecursive( open );
}

//-----------------------------------------------------------------------------
const KMMsgBase* KMHeaders::getMsgBaseForItem( const Q3ListViewItem *item ) const
{
  const HeaderItem *hi = static_cast<const HeaderItem *> ( item );
  return mFolder->getMsgBase( hi->msgId() );
}

//-----------------------------------------------------------------------------
void KMHeaders::setSorting( int column, bool ascending )
{
  if (column != -1) {
  // carsten: really needed?
//    if (column != mSortCol)
//      setColumnText( mSortCol, QIcon( QPixmap()), columnText( mSortCol ));
    if(mSortInfo.dirty || column != mSortInfo.column || ascending != mSortInfo.ascending) { //dirtied
        QObject::disconnect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
        mSortInfo.dirty = true;
    }

    assert(column >= 0);
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
  K3ListView::setSorting( column, ascending );
  ensureCurrentItemVisible();
  // Make sure the config and .sorted file are updated, otherwise stale info
  // is read on new imap mail. ( folder->folderComplete() -> readSortOrder() ).
  if ( mFolder ) {
    writeFolderConfig();
    writeSortOrder();
  }
}

//Flatten the list and write it to disk
static void internalWriteItem(FILE *sortStream, KMFolder *folder, int msgid,
                              int parent_id, QString key,
                              bool update_discover=true)
{
  unsigned long msgSerNum;
  unsigned long parentSerNum;
  msgSerNum = KMMsgDict::instance()->getMsgSerNum( folder, msgid );
  if (parent_id >= 0)
    parentSerNum = KMMsgDict::instance()->getMsgSerNum( folder, parent_id ) + KMAIL_RESERVED;
  else
    parentSerNum = (unsigned long)(parent_id + KMAIL_RESERVED);

  fwrite(&msgSerNum, sizeof(msgSerNum), 1, sortStream);
  fwrite(&parentSerNum, sizeof(parentSerNum), 1, sortStream);
  qint32 len = key.length() * sizeof(QChar);
  fwrite(&len, sizeof(len), 1, sortStream);
  if (len)
    fwrite(key.unicode(), qMin(len, KMAIL_MAX_KEY_LEN), 1, sortStream);

  if (update_discover) {
    //update the discovered change count
      qint32 discovered_count = 0;
      fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 20, SEEK_SET);
      fread(&discovered_count, sizeof(discovered_count), 1, sortStream);
      discovered_count++;
      fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 20, SEEK_SET);
      fwrite(&discovered_count, sizeof(discovered_count), 1, sortStream);
  }
}

void KMHeaders::folderCleared()
{
    mSortCacheItems.clear(); //autoDelete is true
    mSubjectLists.clear();
    mImperfectlyThreadedList.clear();
    mPrevCurrent = 0;
    emit selected(0);
}

bool KMHeaders::writeSortOrder()
{
  QString sortFile = KMAIL_SORT_FILE(mFolder);

  if (!mSortInfo.dirty) {
    struct stat stat_tmp;
    if(stat(QFile::encodeName(sortFile), &stat_tmp) == -1) {
        mSortInfo.dirty = true;
    }
  }
  if (mSortInfo.dirty) {
    if (!mFolder->count()) {
      // Folder is empty now, remove the sort file.
      unlink(QFile::encodeName(sortFile));
      return true;
    }
    QString tempName = sortFile + ".temp";
    unlink(QFile::encodeName(tempName));
    FILE *sortStream = fopen(QFile::encodeName(tempName), "w");
    if (!sortStream)
      return false;

    mSortInfo.ascending = !mSortDescending;
    mSortInfo.dirty = false;
    mSortInfo.column = mSortCol;
    fprintf(sortStream, KMAIL_SORT_HEADER, KMAIL_SORT_VERSION);
    //magic header information
    qint32 byteOrder = 0x12345678;
    qint32 column = mSortCol;
    qint32 ascending= !mSortDescending;
    qint32 threaded = isThreaded();
    qint32 appended=0;
    qint32 discovered_count = 0;
    qint32 sorted_count=0;
    fwrite(&byteOrder, sizeof(byteOrder), 1, sortStream);
    fwrite(&column, sizeof(column), 1, sortStream);
    fwrite(&ascending, sizeof(ascending), 1, sortStream);
    fwrite(&threaded, sizeof(threaded), 1, sortStream);
    fwrite(&appended, sizeof(appended), 1, sortStream);
    fwrite(&discovered_count, sizeof(discovered_count), 1, sortStream);
    fwrite(&sorted_count, sizeof(sorted_count), 1, sortStream);

    Q3PtrStack<HeaderItem> items;
    {
      Q3PtrStack<Q3ListViewItem> s;
      for (Q3ListViewItem * i = firstChild(); i; ) {
        items.push((HeaderItem *)i);
        if ( i->firstChild() ) {
          s.push( i );
          i = i->firstChild();
        } else if( i->nextSibling()) {
          i = i->nextSibling();
        } else {
            for(i=0; !i && s.count(); i = s.pop()->nextSibling());
        }
      }
    }

    KMMsgBase *kmb;
    while(HeaderItem *i = items.pop()) {
      int parent_id = -1; //no parent, top level
      if (threaded) {
        kmb = mFolder->getMsgBase( i->msgId() );
        assert(kmb); // I have seen 0L come out of this, called from
                   // KMHeaders::setFolder(0xgoodpointer, false);
                   // I see this crash too. after rebuilding a broken index on a dimap folder. always
        QString replymd5 = kmb->replyToIdMD5();
        QString replyToAuxId = kmb->replyToAuxIdMD5();
        SortCacheItem *p = NULL;
        if(!replymd5.isEmpty())
          p = mSortCacheItems[replymd5];

        if (p)
          parent_id = p->id();
        // We now have either found a parent, or set it to -1, which means that
        // it will be reevaluated when a message is added, for example. If there
        // is no replyToId and no replyToAuxId and the message is not prefixed,
        // this message is top level, and will always be, so no need to
        // reevaluate it.
        if (replymd5.isEmpty()
            && replyToAuxId.isEmpty()
            && !kmb->subjectIsPrefixed() )
          parent_id = -2;
        // FIXME also mark messages with -1 as -2 a certain amount of time after
        // their arrival, since it becomes very unlikely that a new parent for
        // them will show up. (Ingo suggests a month.) -till
      }
      internalWriteItem(sortStream, mFolder, i->msgId(), parent_id,
                        i->key(mSortCol, !mSortDescending), false);
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
        unlink(QFile::encodeName(sortFile));
        kWarning(5006) << "Error: Failure modifying " << sortFile << " (No space left on device?)" << endl;
        kWarning(5006) << __FILE__ << ":" << __LINE__ << endl;
        kmkernel->emergencyExit( i18n("Failure modifying %1\n(No space left on device?)", sortFile ));
    }
    fclose(sortStream);
    ::rename(QFile::encodeName(tempName), QFile::encodeName(sortFile));
  }

  return true;
}

void KMHeaders::appendItemToSortFile(HeaderItem *khi)
{
  QString sortFile = KMAIL_SORT_FILE(mFolder);
  if(FILE *sortStream = fopen(QFile::encodeName(sortFile), "r+")) {
    int parent_id = -1; //no parent, top level

    if (isThreaded()) {
      SortCacheItem *sci = khi->sortCacheItem();
      KMMsgBase *kmb = mFolder->getMsgBase( khi->msgId() );
      if(sci->parent() && !sci->isImperfectlyThreaded())
        parent_id = sci->parent()->id();
      else if(kmb->replyToIdMD5().isEmpty()
           && kmb->replyToAuxIdMD5().isEmpty()
           && !kmb->subjectIsPrefixed())
        parent_id = -2;
    }

    internalWriteItem(sortStream, mFolder, khi->msgId(), parent_id,
                      khi->key(mSortCol, !mSortDescending), false);

    //update the appended flag FIXME obsolete?
    qint32 appended = 1;
    fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 16, SEEK_SET);
    fwrite(&appended, sizeof(appended), 1, sortStream);
    fseek(sortStream, KMAIL_MAGIC_HEADER_OFFSET + 16, SEEK_SET);

    if (sortStream && ferror(sortStream)) {
        fclose(sortStream);
        unlink(QFile::encodeName(sortFile));
        kWarning(5006) << "Error: Failure modifying " << sortFile << " (No space left on device?)" << endl;
        kWarning(5006) << __FILE__ << ":" << __LINE__ << endl;
        kmkernel->emergencyExit( i18n("Failure modifying %1\n(No space left on device?)", sortFile ));
    }
    fclose(sortStream);
  } else {
    mSortInfo.dirty = true;
  }
}

void KMHeaders::dirtySortOrder(int column)
{
    mSortInfo.dirty = true;
    QObject::disconnect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
    setSorting(column, mSortInfo.column == column ? !mSortInfo.ascending : true);
}

// -----------------
void SortCacheItem::updateSortFile( FILE *sortStream, KMFolder *folder,
                                      bool waiting_for_parent, bool update_discover)
{
    if(mSortOffset == -1) {
        fseek(sortStream, 0, SEEK_END);
        mSortOffset = ftell(sortStream);
    } else {
        fseek(sortStream, mSortOffset, SEEK_SET);
    }

    int parent_id = -1;
    if(!waiting_for_parent) {
        if(mParent && !isImperfectlyThreaded())
            parent_id = mParent->id();
    }
    internalWriteItem(sortStream, folder, mId, parent_id, mKey, update_discover);
}

static bool compare_ascending = false;
static bool compare_toplevel = true;
static int compare_SortCacheItem(const void *s1, const void *s2)
{
    if ( !s1 || !s2 )
        return 0;
    SortCacheItem **b1 = (SortCacheItem **)s1;
    SortCacheItem **b2 = (SortCacheItem **)s2;
    int ret = (*b1)->key().compare((*b2)->key());
    if(compare_ascending || !compare_toplevel)
        ret = -ret;
    return ret;
}

// Debugging helpers
void KMHeaders::printSubjectThreadingTree()
{
    Q3DictIterator< QList< SortCacheItem* > > it ( mSubjectLists );
    kDebug(5006) << "SubjectThreading tree: " << endl;
    for( ; it.current(); ++it ) {
      QList<SortCacheItem*> list = *( it.current() );
      QList<SortCacheItem*>::const_iterator it2;
      kDebug(5006) << "Subject MD5: " << it.currentKey() << " list: " << endl;
      for( it2 = list.constBegin(); it2 != list.constEnd(); ++it2 ) {
        SortCacheItem *sci = (*it2);
        kDebug(5006) << "     item:" << sci << " sci id: " << sci->id() << endl;
      }
    }
    kDebug(5006) << endl;
}

void KMHeaders::printThreadingTree()
{
    kDebug(5006) << "Threading tree: " << endl;
    Q3DictIterator<SortCacheItem> it( mSortCacheItems );
    kDebug(5006) << endl;
    for( ; it.current(); ++it ) {
      SortCacheItem *sci = it.current();
      kDebug(5006) << "MsgId MD5: " << it.currentKey() << " message id: " << sci->id() << endl;
    }
    for (int i = 0; i < (int)mItems.size(); ++i) {
      HeaderItem *item = mItems[i];
      int parentCacheId = item->sortCacheItem()->parent()?item->sortCacheItem()->parent()->id():0;
      kDebug( 5006 ) << "SortCacheItem: " << item->sortCacheItem()->id() << " parent: " << parentCacheId << endl;
      kDebug( 5006 ) << "Item: " << item << " sortCache: " << item->sortCacheItem() << " parent: " << item->sortCacheItem()->parent() << endl;
    }
    kDebug(5006) << endl;
}

// -------------------------------------

void KMHeaders::buildThreadingTree( Q3MemArray<SortCacheItem *> sortCache )
{
    mSortCacheItems.clear();
    mSortCacheItems.resize( mFolder->count() * 2 );

    // build a dict of all message id's
    for(int x = 0; x < mFolder->count(); x++) {
        KMMsgBase *mi = mFolder->getMsgBase(x);
        QString md5 = mi->msgIdMD5();
        if(!md5.isEmpty())
            mSortCacheItems.replace(md5, sortCache[x]);
    }
}


void KMHeaders::buildSubjectThreadingTree( Q3MemArray<SortCacheItem *> sortCache )
{
    mSubjectLists.clear();  // autoDelete is true
    mSubjectLists.resize( mFolder->count() * 2 );

    for(int x = 0; x < mFolder->count(); x++) {
        // Only a lot items that are now toplevel
        if ( sortCache[x]->parent()
          && sortCache[x]->parent()->id() != -666 ) continue;
        KMMsgBase *mi = mFolder->getMsgBase(x);
        QString subjMD5 = mi->strippedSubjectMD5();
        if (subjMD5.isEmpty()) {
            mFolder->getMsgBase(x)->initStrippedSubjectMD5();
            subjMD5 = mFolder->getMsgBase(x)->strippedSubjectMD5();
        }
        if( subjMD5.isEmpty() ) continue;

        /* For each subject, keep a list of items with that subject
         * (stripped of prefixes) sorted by date. */
        if (!mSubjectLists.find(subjMD5))
            mSubjectLists.insert(subjMD5, new QList<SortCacheItem*>());
        /* Insertion sort by date. These lists are expected to be very small.
         * Also, since the messages are roughly ordered by date in the store,
         * they should mostly be prepended at the very start, so insertion is
         * cheap. */
        int p=0;
        QList<SortCacheItem*>::const_iterator it;
        for ( it = (*mSubjectLists[subjMD5]).constBegin();
              it != (*mSubjectLists[subjMD5]).constEnd(); ++it) {
          KMMsgBase *mb = mFolder->getMsgBase((*it)->id());
          if ( mb->date() < mi->date())
              break;
          p++;
        }
        mSubjectLists[subjMD5]->insert( p, sortCache[x]);
        sortCache[x]->setSubjectThreadingList( mSubjectLists[subjMD5] );
    }
}


SortCacheItem* KMHeaders::findParent(SortCacheItem *item)
{
    SortCacheItem *parent = NULL;
    if (!item) return parent;
    KMMsgBase *msg =  mFolder->getMsgBase(item->id());
    QString replyToIdMD5 = msg->replyToIdMD5();
    item->setImperfectlyThreaded(true);
    /* First, try if the message our Reply-To header points to
     * is available to thread below. */
    if(!replyToIdMD5.isEmpty()) {
        parent = mSortCacheItems[replyToIdMD5];
        if (parent)
            item->setImperfectlyThreaded(false);
    }
    if (!parent) {
        // If we don't have a replyToId, or if we have one and the
        // corresponding message is not in this folder, as happens
        // if you keep your outgoing messages in an OUTBOX, for
        // example, try the list of references, because the second
        // to last will likely be in this folder. replyToAuxIdMD5
        // contains the second to last one.
        QString  ref = msg->replyToAuxIdMD5();
        if (!ref.isEmpty())
            parent = mSortCacheItems[ref];
    }
    return parent;
}

SortCacheItem* KMHeaders::findParentBySubject(SortCacheItem *item)
{
    SortCacheItem *parent = NULL;
    if (!item) return parent;

    KMMsgBase *msg =  mFolder->getMsgBase(item->id());

    // Let's try by subject, but only if the  subject is prefixed.
    // This is necessary to make for example cvs commit mailing lists
    // work as expected without having to turn threading off alltogether.
    if (!msg->subjectIsPrefixed())
        return parent;

    QString replyToIdMD5 = msg->replyToIdMD5();
    QString subjMD5 = msg->strippedSubjectMD5();
    if (!subjMD5.isEmpty() && mSubjectLists[subjMD5]) {
        /* Iterate over the list of potential parents with the same
         * subject, and take the closest one by date. */
        QList<SortCacheItem*>::const_iterator it2;
        for ( it2 = (*mSubjectLists[subjMD5]).constBegin();
              it2 != (*mSubjectLists[subjMD5]).constEnd(); ++it2) {
            KMMsgBase *mb = mFolder->getMsgBase((*it2)->id());
            if ( !mb ) return parent;
            // make sure it's not ourselves
            if ( item == (*it2) ) continue;
            int delta = msg->date() - mb->date();
            // delta == 0 is not allowed, to avoid circular threading
            // with duplicates.
            if (delta > 0 ) {
                // Don't use parents more than 6 weeks older than us.
                if (delta < 3628899)
                    parent = (*it2);
                break;
            }
        }
    }
    return parent;
}

bool KMHeaders::readSortOrder( bool set_selection, bool forceJumpToUnread )
{
    kDebug(5006) << k_funcinfo << endl << kBacktrace() << endl;
    //all cases
    qint32 column, ascending, threaded, discovered_count, sorted_count, appended;
    qint32 deleted_count = 0;
    bool unread_exists = false;
    bool jumpToUnread = (GlobalSettings::self()->actionEnterFolder() ==
                         GlobalSettings::EnumActionEnterFolder::SelectFirstUnreadNew) ||
                        forceJumpToUnread;
    Q3MemArray<SortCacheItem *> sortCache(mFolder->count());
    bool error = false;

    //threaded cases
    QList<SortCacheItem*> unparented;
    mImperfectlyThreadedList.clear();

    //cleanup
    mItems.fill( 0, mFolder->count() );
    sortCache.fill( 0 );

    mRoot->clearChildren();

    QString sortFile = KMAIL_SORT_FILE(mFolder);
    FILE *sortStream = fopen(QFile::encodeName(sortFile), "r+");
    mSortInfo.fakeSort = 0;

    if(sortStream) {
        mSortInfo.fakeSort = 1;
        int version = 0;
        if (fscanf(sortStream, KMAIL_SORT_HEADER, &version) != 1)
          version = -1;
        if(version == KMAIL_SORT_VERSION) {
          qint32 byteOrder = 0;
          fread(&byteOrder, sizeof(byteOrder), 1, sortStream);
          if (byteOrder == 0x12345678)
          {
            fread(&column, sizeof(column), 1, sortStream);
            fread(&ascending, sizeof(ascending), 1, sortStream);
            fread(&threaded, sizeof(threaded), 1, sortStream);
            fread(&appended, sizeof(appended), 1, sortStream);
            fread(&discovered_count, sizeof(discovered_count), 1, sortStream);
            fread(&sorted_count, sizeof(sorted_count), 1, sortStream);

            //Hackyness to work around qlistview problems
            K3ListView::setSorting(-1);
            header()->setSortIndicator(column, ascending);
            QObject::connect(header(), SIGNAL(clicked(int)), this, SLOT(dirtySortOrder(int)));
            //setup mSortInfo here now, as above may change it
            mSortInfo.dirty = false;
            mSortInfo.column = (short)column;
            mSortInfo.ascending = (compare_ascending = ascending);

            SortCacheItem *item;
            unsigned long serNum, parentSerNum;
            int id, len, parent, x;
            QChar *tmp_qchar = 0;
            int tmp_qchar_len = 0;
            const int mFolderCount = mFolder->count();
            QString key;

            CREATE_TIMER(parse);
            START_TIMER(parse);
            for(x = 0; !feof(sortStream) && !error; x++) {
                off_t offset = ftell(sortStream);
                KMFolder *folder;
                //parse
                if(!fread(&serNum, sizeof(serNum), 1, sortStream) || //short read means to end
                   !fread(&parentSerNum, sizeof(parentSerNum), 1, sortStream) ||
                   !fread(&len, sizeof(len), 1, sortStream)) {
                    break;
                }
                if ((len < 0) || (len > KMAIL_MAX_KEY_LEN)) {
                    kDebug(5006) << "Whoa.2! len " << len << " " << __FILE__ << ":" << __LINE__ << endl;
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

                KMMsgDict::instance()->getLocation(serNum, &folder, &id);
                if (folder != mFolder) {
                    ++deleted_count;
                    continue;
                }
                if (parentSerNum < KMAIL_RESERVED) {
                    parent = (int)parentSerNum - KMAIL_RESERVED;
                } else {
                    KMMsgDict::instance()->getLocation(parentSerNum - KMAIL_RESERVED, &folder, &parent);
                    if (folder != mFolder)
                        parent = -1;
                }
                if ((id < 0) || (id >= mFolderCount) ||
                    (parent < -2) || (parent >= mFolderCount)) { // sanity checking
                    kDebug(5006) << "Whoa.1! " << __FILE__ << ":" << __LINE__ << endl;
                    error = true;
                    continue;
                }

                if ((item=sortCache[id])) {
                    if (item->id() != -1) {
                        kDebug(5006) << "Whoa.3! " << __FILE__ << ":" << __LINE__ << endl;
                        error = true;
                        continue;
                    }
                    item->setKey(key);
                    item->setId(id);
                    item->setOffset(offset);
                } else {
                    item = sortCache[id] = new SortCacheItem(id, key, offset);
                }
                if (threaded && parent != -2) {
                    if(parent == -1) {
                        unparented.append(item);
                        mRoot->addUnsortedChild(item);
                    } else {
                        if( ! sortCache[parent] ) {
                            sortCache[parent] = new SortCacheItem;
                        }
                        sortCache[parent]->addUnsortedChild(item);
                    }
                } else {
                    if(x < sorted_count )
                        mRoot->addSortedChild(item);
                    else {
                        mRoot->addUnsortedChild(item);
                    }
                }
            }
            if (error || (x != sorted_count + discovered_count)) {// sanity check
                kDebug(5006) << endl << "Whoa: x " << x << ", sorted_count " << sorted_count << ", discovered_count " << discovered_count << ", count " << mFolder->count() << endl << endl;
                fclose(sortStream);
                sortStream = 0;
            }

            if(tmp_qchar)
                free(tmp_qchar);
            END_TIMER(parse);
            SHOW_TIMER(parse);
          }
          else {
              fclose(sortStream);
              sortStream = 0;
          }
        } else {
            fclose(sortStream);
            sortStream = 0;
        }
    }

    if (!sortStream) {
        mSortInfo.dirty = true;
        mSortInfo.column = column = mSortCol;
        mSortInfo.ascending = ascending = !mSortDescending;
        threaded = (isThreaded());
        sorted_count = discovered_count = appended = 0;
        K3ListView::setSorting( mSortCol, !mSortDescending );
    }
    //fill in empty holes
    if((sorted_count + discovered_count - deleted_count) < mFolder->count()) {
        CREATE_TIMER(holes);
        START_TIMER(holes);
        KMMsgBase *msg = 0;
        for(int x = 0; x < mFolder->count(); x++) {
            if((!sortCache[x] || (sortCache[x]->id() < 0)) && (msg=mFolder->getMsgBase(x))) {
                int sortOrder = column;
                if (mPaintInfo.orderOfArrival)
                    sortOrder |= (1 << 6);
                if (mPaintInfo.status)
                    sortOrder |= (1 << 5);
                sortCache[x] = new SortCacheItem(
                    x, HeaderItem::generate_key( this, msg, &mPaintInfo, sortOrder ));
                if(threaded)
                    unparented.append(sortCache[x]);
                else
                    mRoot->addUnsortedChild(sortCache[x]);
                if(sortStream)
                    sortCache[x]->updateSortFile(sortStream, mFolder, true, true);
                discovered_count++;
                appended = 1;
            }
        }
        END_TIMER(holes);
        SHOW_TIMER(holes);
    }

    // Make sure we've placed everything in parent/child relationship. All
    // messages with a parent id of -1 in the sort file are reevaluated here.
    if (threaded) buildThreadingTree( sortCache );
    QList<SortCacheItem*> toBeSubjThreaded;

    if (threaded && !unparented.isEmpty()) {
        CREATE_TIMER(reparent);
        START_TIMER(reparent);

        QList<SortCacheItem*>::const_iterator it;
        for ( it = unparented.constBegin();
              it != unparented.constEnd(); ++it) {
          SortCacheItem *item = (*it);
          SortCacheItem *parent = findParent( item );
          // If we have a parent, make sure it's not ourselves
          if ( parent && (parent != (*it)) ) {
              parent->addUnsortedChild((*it));
              if(sortStream)
                  (*it)->updateSortFile(sortStream, mFolder);
          } else {
              // if we will attempt subject threading, add to the list,
              // otherwise to the root with them
              if (mSubjThreading)
                toBeSubjThreaded.append((*it));
              else
                mRoot->addUnsortedChild((*it));
          }
        }

        if (mSubjThreading) {
            buildSubjectThreadingTree( sortCache );
            QList<SortCacheItem*>::const_iterator it;
            for ( it = toBeSubjThreaded.constBegin();
                  it != toBeSubjThreaded.constEnd(); ++it) {
                SortCacheItem *item = (*it);
                SortCacheItem *parent = findParentBySubject( item );

                if ( parent ) {
                    parent->addUnsortedChild((*it));
                    if(sortStream)
                      (*it)->updateSortFile(sortStream, mFolder);
                } else {
                    //oh well we tried, to the root with you!
                    mRoot->addUnsortedChild((*it));
                }
            }
        }
        END_TIMER(reparent);
        SHOW_TIMER(reparent);
    }
    //create headeritems
    CREATE_TIMER(header_creation);
    START_TIMER(header_creation);
    HeaderItem *khi;
    SortCacheItem *i, *new_kci;
    Q3PtrQueue<SortCacheItem> s;
    s.enqueue(mRoot);
    compare_toplevel = true;
    do {
        i = s.dequeue();
        const QList<SortCacheItem*> *sorted = i->sortedChildren();
        int unsorted_count, unsorted_off=0;
        SortCacheItem **unsorted = i->unsortedChildren(unsorted_count);
        if(unsorted)
            qsort(unsorted, unsorted_count, sizeof(SortCacheItem *), //sort
                  compare_SortCacheItem);

        /* The sorted list now contains all sorted children of this item, while
         * the (aptly named) unsorted array contains all as of yet unsorted
         * ones. It has just been qsorted, so it is in itself sorted. These two
         * sorted lists are now merged into one. */
        QList<SortCacheItem*>::const_iterator it;
        for ( it = sorted->constBegin();
              (unsorted && unsorted_off < unsorted_count) || it != sorted->constEnd(); ) {
            /* As long as we have something in the sorted list and there is
               nothing unsorted left, use the item from the sorted list. Also
               if we are sorting descendingly and the sorted item is supposed
               to be sorted before the unsorted one do so. In the ascending
               case we invert the logic for non top level items. */
            if( (*it) &&
                ( !unsorted || unsorted_off >= unsorted_count
                ||
                ( ( !ascending || (ascending && !compare_toplevel) )
                  && (*it)->key() < unsorted[unsorted_off]->key() )
                ||
                (  ascending && (*it)->key() >= unsorted[unsorted_off]->key() )
                )
               )
            {
                new_kci = (*it);
                ++it;
            } else {
                /* Otherwise use the next item of the unsorted list */
                new_kci = unsorted[unsorted_off++];
            }
            if(new_kci->item() || new_kci->parent() != i) //could happen if you reparent
                continue;

            if(threaded && i->item()) {
                // If the parent is watched or ignored, propagate that to it's
                // children
                if (mFolder->getMsgBase(i->id())->status().isWatched())
                  mFolder->getMsgBase(new_kci->id())->setStatus( MessageStatus::statusWatched() );
                if (mFolder->getMsgBase(i->id())->status().isIgnored())
                  mFolder->getMsgBase(new_kci->id())->setStatus( MessageStatus::statusIgnored() );
                khi = new HeaderItem(i->item(), new_kci->id(), new_kci->key());
            } else {
                khi = new HeaderItem(this, new_kci->id(), new_kci->key());
            }
            new_kci->setItem(mItems[new_kci->id()] = khi);
            if(new_kci->hasChildren())
                s.enqueue(new_kci);
            // we always jump to new messages, but we only jump to
            // unread messages if we are told to do so
            if ( ( mFolder->getMsgBase(new_kci->id())->status().isNew() &&
                   GlobalSettings::self()->actionEnterFolder() ==
                   GlobalSettings::EnumActionEnterFolder::SelectFirstNew ) ||
                 ( ( mFolder->getMsgBase(new_kci->id())->status().isNew() ||
                     mFolder->getMsgBase(new_kci->id())->status().isUnread() ) &&
                   jumpToUnread ) )
            {
              unread_exists = true;
            }
        }
        // If we are sorting by date and ascending the top level items are sorted
        // ascending and the threads themselves are sorted descending. One wants
        // to have new threads on top but the threads themselves top down.
        if (mSortCol == paintInfo()->dateCol)
          compare_toplevel = false;
    } while(!s.isEmpty());

    for(int x = 0; x < mFolder->count(); x++) {     //cleanup
        if (!sortCache[x]) { // not yet there?
            continue;
        }

        if (!sortCache[x]->item()) { // we missed a message, how did that happen ?
            kDebug(5006) << "KMHeaders::readSortOrder - msg could not be threaded. "
                  << endl << "Please talk to your threading counselor asap. " <<  endl;
            khi = new HeaderItem(this, sortCache[x]->id(), sortCache[x]->key());
            sortCache[x]->setItem(mItems[sortCache[x]->id()] = khi);
        }
        // Add all imperfectly threaded items to a list, so they can be
        // reevaluated when a new message arrives which might be a better parent.
        // Important for messages arriving out of order.
        if (threaded && sortCache[x]->isImperfectlyThreaded()) {
            mImperfectlyThreadedList.append(sortCache[x]->item());
        }
        // Set the reverse mapping HeaderItem -> SortCacheItem. Needed for
        // keeping the data structures up to date on removal, for example.
        sortCache[x]->item()->setSortCacheItem(sortCache[x]);
    }

    if (getNestingPolicy()<2)
      for (HeaderItem *khi=static_cast<HeaderItem*>(firstChild()); khi!=0;khi=static_cast<HeaderItem*>(khi->nextSibling()))
        khi->setOpen(true);

    END_TIMER(header_creation);
    SHOW_TIMER(header_creation);

    if(sortStream) { //update the .sorted file now
        // heuristic for when it's time to rewrite the .sorted file
        if( discovered_count * discovered_count > sorted_count - deleted_count ) {
            mSortInfo.dirty = true;
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
        int first_unread = -1;
        if (unread_exists) {
            HeaderItem *item = static_cast<HeaderItem*>(firstChild());
            while (item) {
              if ( ( mFolder->getMsgBase(item->msgId())->status().isNew() &&
                     GlobalSettings::self()->actionEnterFolder() ==
                     GlobalSettings::EnumActionEnterFolder::SelectFirstNew ) ||
                   ( ( mFolder->getMsgBase(item->msgId())->status().isNew() ||
                       mFolder->getMsgBase(item->msgId())->status().isUnread() ) &&
                     jumpToUnread ) )
              {
                first_unread = item->msgId();
                break;
              }
              item = static_cast<HeaderItem*>(item->itemBelow());
            }
        }

        if(first_unread == -1 ) {
            setTopItemByIndex(mTopItem);
            if ( mCurrentItem >= 0 )
              setCurrentItemByIndex( mCurrentItem );
            else if ( mCurrentItemSerNum > 0 )
              setCurrentItemBySerialNum( mCurrentItemSerNum );
            else
              setCurrentItemByIndex( 0 );
        } else {
            setCurrentItemByIndex(first_unread);
            makeHeaderVisible();
            center( contentsX(), itemPos(mItems[first_unread]), 0, 9.0 );
        }
    } else {
        // only reset the selection if we have no current item
        if (mCurrentItem <= 0) {
          setTopItemByIndex(mTopItem);
          setCurrentItemByIndex(0);
        }
    }
    END_TIMER(selection);
    SHOW_TIMER(selection);
    if (error || (sortStream && ferror(sortStream))) {
        if ( sortStream )
            fclose(sortStream);
        unlink(QFile::encodeName(sortFile));
        kWarning(5006) << "Error: Failure modifying " << sortFile << " (No space left on device?)" << endl;
        kWarning(5006) << __FILE__ << ":" << __LINE__ << endl;

        return true;
    }
    if(sortStream)
        fclose(sortStream);

    return true;
}

//-----------------------------------------------------------------------------
void KMHeaders::setCurrentItemBySerialNum( unsigned long serialNum )
{
  // Linear search == slow. Don't overuse this method.
  // It's currently only used for finding the current item again
  // after expiry deleted mails (so the index got invalidated).
  for (int i = 0; i < (int)mItems.size() - 1; ++i) {
    KMMsgBase *mMsgBase = mFolder->getMsgBase( i );
    if ( mMsgBase->getMsgSerNum() == serialNum ) {
      bool unchanged = (currentItem() == mItems[i]);
      setCurrentItem( mItems[i] );
      setSelected( mItems[i], true );
      setSelectionAnchor( currentItem() );
      if ( unchanged )
        highlightMessage( currentItem(), false );
      ensureCurrentItemVisible();
      return;
    }
  }
  // Not found. Maybe we should select the last item instead?
  kDebug(5006) << "KMHeaders::setCurrentItem item with serial number " << serialNum << " NOT FOUND" << endl;
}

void KMHeaders::copyMessages()
{
  mCopiedMessages.clear();
  KMMessageList* list = selectedMsgs();
  for ( int i = 0; i < list->count(); ++ i )
    mCopiedMessages << list->at( i )->getMsgSerNum();
  mMoveMessages = false;
  updateActions();
}

void KMHeaders::cutMessages()
{
  mCopiedMessages.clear();
  KMMessageList* list = selectedMsgs();
  for ( int i = 0; i < list->count(); ++ i )
    mCopiedMessages << list->at( i )->getMsgSerNum();
  mMoveMessages = true;
  updateActions();
}

void KMHeaders::pasteMessages()
{
  new MessageCopyHelper( mCopiedMessages, folder(), mMoveMessages, this );
  if ( mMoveMessages ) {
    mCopiedMessages.clear();
    updateActions();
  }
}

void KMHeaders::updateActions()
{
  QAction *copy = owner()->action( "copy_messages" );
  QAction *cut = owner()->action( "cut_messages" );
  QAction *paste = owner()->action( "paste_messages" );

  if ( selectedItems().isEmpty() ) {
    copy->setEnabled( false );
    cut->setEnabled( false );
  } else {
    copy->setEnabled( true );
    if ( folder() && folder()->isReadOnly() )
      cut->setEnabled( false );
    else
      cut->setEnabled( true );
  }

  if ( mCopiedMessages.isEmpty() || !folder() || folder()->isReadOnly() )
    paste->setEnabled( false );
  else
    paste->setEnabled( true );
}

void KMHeaders::setCopiedMessages(const QList<quint32> & msgs, bool move)
{
  mCopiedMessages = msgs;
  mMoveMessages = move;
  updateActions();
}

#include "kmheaders.moc"
