// -*- mode: C++; c-file-style: "gnu" -*-
// kmmainwidget.cpp
//#define MALLOC_DEBUG 1

#include <kwin.h>

#ifdef MALLOC_DEBUG
#include <malloc.h>
#endif

#undef Unsorted // X headers...
#include <qaccel.h>

#include <kopenwith.h>

#include <kmessagebox.h>

#include <kaccelmanager.h>
#include <kglobalsettings.h>
#include <kstdaccel.h>
#include <kkeydialog.h>
#include <kcharsets.h>
#include <knotifyclient.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <ktip.h>
#include <knotifydialog.h>
#include <kstandarddirs.h>
#include <dcopclient.h>

#include "kcursorsaver.h"
#include "kmbroadcaststatus.h"
#include "kmfoldermgr.h"
#include "kmfolderdia.h"
#include "kmacctmgr.h"
#include "kmfilter.h"
#include "kmfoldertree.h"
#include "kmreadermainwin.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"
#include "kmcomposewin.h"
#include "kmfolderseldlg.h"
#include "kmfiltermgr.h"
#include "kmsender.h"
#include "kmaddrbook.h"
#include "kmversion.h"
#include "kmfldsearch.h"
#include "kmacctfolder.h"
#include "undostack.h"
#include "kmcommands.h"
#include "kmmainwidget.h"
#include "kmmainwin.h"
#include "kmsystemtray.h"
#include "vacation.h"
using KMail::Vacation;
#include "subscriptiondialog.h"
using KMail::SubscriptionDialog;
#include "attachmentstrategy.h"
using KMail::AttachmentStrategy;
#include "headerstrategy.h"
using KMail::HeaderStrategy;
#include "headerstyle.h"
using KMail::HeaderStyle;
#include "folderjob.h"
using KMail::FolderJob;
#include "mailinglist-magic.h"
#include "kmgroupware.h"

#include <assert.h>
#include <kstatusbar.h>

#include <kmime_mdn.h>
#include <kmime_header_parsing.h>
using namespace KMime;
using KMime::Types::AddrSpecList;

#include "kmmainwidget.moc"

//-----------------------------------------------------------------------------
KMMainWidget::KMMainWidget(QWidget *parent, const char *name,
			   KActionCollection *actionCollection, KConfig* config ) :
    QWidget(parent, name)
{
  // must be the first line of the constructor:
  mSearchWin = 0;
  mStartupDone = FALSE;
  mIntegrated  = TRUE;
  mFolder = 0;
  mFolderThreadPref = false;
  mFolderThreadSubjPref = true;
  mReaderWindowActive = true;
  mReaderWindowBelow = true;
  mFolderHtmlPref = false;
  mCountJobs = 0;
  mSystemTray = 0;
  mDestructed = false;
  mActionCollection = actionCollection;
  mTopLayout = new QVBoxLayout(this);
  mFilterActions.setAutoDelete(true);
  mFilterCommands.setAutoDelete(true);
  mJob = 0;
  mConfig = config;

  mPanner1Sep << 1 << 1;
  mPanner2Sep << 1 << 1;

  setMinimumSize(400, 300);

  kmkernel->groupware().setMainWidget( this );

  readPreConfig();
  createWidgets();

  setupStatusBar();
  setupActions();

  readConfig();

  activatePanners();

  QTimer::singleShot( 0, this, SLOT( slotShowStartupFolder() ));

  connect(kmkernel->acctMgr(), SIGNAL( checkedMail(bool, bool)),
          SLOT( slotMailChecked(bool, bool)));

  connect(kmkernel, SIGNAL( configChanged() ),
          this, SLOT( slotConfigChanged() ));

  // display the full path to the folder in the caption
  connect(mFolderTree, SIGNAL(currentChanged(QListViewItem*)),
      this, SLOT(slotChangeCaption(QListViewItem*)));
  connect( KMBroadcastStatus::instance(), SIGNAL(statusMsg( const QString& )),
	   this, SLOT(statusMsg( const QString& )));

  if ( kmkernel->firstInstance() )
    QTimer::singleShot( 200, this, SLOT(slotShowTipOnStart()) );

  toggleSystray(mSystemTrayOnNew, mSystemTrayMode);

  // must be the last line of the constructor:
  mStartupDone = TRUE;
}


//-----------------------------------------------------------------------------
//The kernel may have already been deleted when this method is called,
//perform all cleanup that requires the kernel in destruct()
KMMainWidget::~KMMainWidget()
{
  destruct();
}


//-----------------------------------------------------------------------------
//This method performs all cleanup that requires the kernel to exist.
void KMMainWidget::destruct()
{
  if (mDestructed)
    return;
  if (mSearchWin)
    mSearchWin->close();
  writeConfig();
  writeFolderConfig();
  delete mHeaders;
  delete mFolderTree;
  delete mSystemTray;
  delete mMsgView;
  mDestructed = true;
}


//-----------------------------------------------------------------------------
void KMMainWidget::readPreConfig(void)
{
  const KConfigGroup geometry( KMKernel::config(), "Geometry" );
  const KConfigGroup general( KMKernel::config(), "General" );

  mLongFolderList = geometry.readEntry( "FolderList", "long" ) != "short";
  mEncodingStr = general.readEntry("encoding", "").latin1();
  mReaderWindowActive = geometry.readEntry( "readerWindowMode", "below" ) != "hide";
  mReaderWindowBelow = geometry.readEntry( "readerWindowMode", "below" ) == "below";
}


//-----------------------------------------------------------------------------
void KMMainWidget::readFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());
  mFolderThreadPref = config->readBoolEntry( "threadMessagesOverride", false );
  mFolderThreadSubjPref = config->readBoolEntry( "threadMessagesBySubject", true );
  mFolderHtmlPref = config->readBoolEntry( "htmlMailOverride", false );
}


//-----------------------------------------------------------------------------
void KMMainWidget::writeFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());
  config->writeEntry( "threadMessagesOverride", mFolderThreadPref );
  config->writeEntry( "threadMessagesBySubject", mFolderThreadSubjPref );
  config->writeEntry( "htmlMailOverride", mFolderHtmlPref );
}


//-----------------------------------------------------------------------------
void KMMainWidget::readConfig(void)
{
  KConfig *config = KMKernel::config();

  bool oldLongFolderList =  mLongFolderList;
  bool oldReaderWindowActive = mReaderWindowActive;
  bool oldReaderWindowBelow = mReaderWindowBelow;

  QString str;
  QSize siz;

  if (mStartupDone)
  {
    writeConfig();

    readPreConfig();
    mHeaders->refreshNestedState();

    bool layoutChanged = ( oldLongFolderList != mLongFolderList )
                    || ( oldReaderWindowActive != mReaderWindowActive )
                    || ( oldReaderWindowBelow != mReaderWindowBelow );


    if( layoutChanged ) {
      hide();
      // delete all panners
      delete mPanner1; // will always delete the others
      createWidgets();
    }

  }

  // read "Reader" config options
  KConfigGroup readerConfig( config, "Reader" );
  mHtmlPref = readerConfig.readBoolEntry( "htmlMail", false );
  // restore the toggle action to the saved value; this is also read during
  // the reader initialization
  if (mMsgView)
    toggleFixFontAction()->setChecked( readerConfig.readBoolEntry( "useFixedFont",
                                                               false ) );

  mHtmlPref = readerConfig.readBoolEntry( "htmlMail", false );

  { // area for config group "Geometry"
    KConfigGroupSaver saver(config, "Geometry");
    mThreadPref = config->readBoolEntry( "nestedMessages", false );
    // size of the mainwin
    QSize defaultSize(750,560);
    siz = config->readSizeEntry("MainWin", &defaultSize);
    if (!siz.isEmpty())
      resize(siz);
    // default width of the foldertree
    static const int folderpanewidth = 250;

    const int folderW = config->readNumEntry( "FolderPaneWidth", folderpanewidth );
    const int headerW = config->readNumEntry( "HeaderPaneWidth", width()-folderpanewidth );
    const int headerH = config->readNumEntry( "HeaderPaneHeight", 180 );
    const int readerH = config->readNumEntry( "ReaderPaneHeight", 280 );

    mPanner1Sep.clear();
    mPanner2Sep.clear();
    QValueList<int> & widths = mLongFolderList ? mPanner1Sep : mPanner2Sep ;
    QValueList<int> & heights = mLongFolderList ? mPanner2Sep : mPanner1Sep ;

    widths << folderW << headerW;
    heights << headerH << readerH;

    bool layoutChanged = ( oldLongFolderList != mLongFolderList )
                    || ( oldReaderWindowActive != mReaderWindowActive )
                    || ( oldReaderWindowBelow != mReaderWindowBelow );

    if (!mStartupDone || layoutChanged )
    {
      /** unread / total columns
       * as we have some dependencies in this widget
       * it's better to manage these here */
      // The columns are shown by default.
      int unreadColumn = config->readNumEntry("UnreadColumn", -1);
      int totalColumn = config->readNumEntry("TotalColumn", -1);

      /* we need to _activate_ them in the correct order
      * this is ugly because we can't use header()->moveSection
      * but otherwise the restoreLayout from KMFolderTree
      * doesn't know that to do */
      if (unreadColumn != -1 && unreadColumn < totalColumn)
        mFolderTree->toggleColumn(KMFolderTree::unread);
      if (totalColumn != -1)
        mFolderTree->toggleColumn(KMFolderTree::total);
      if (unreadColumn != -1 && unreadColumn > totalColumn)
        mFolderTree->toggleColumn(KMFolderTree::unread);

    }
  }

  if (mMsgView)
    mMsgView->readConfig();
  slotSetEncoding();
  mHeaders->readConfig();
  mHeaders->restoreLayout(KMKernel::config(), "Header-Geometry");
  mFolderTree->readConfig();

  { // area for config group "General"
    KConfigGroupSaver saver(config, "General");
    mSendOnCheck = config->readBoolEntry("sendOnCheck",false);
    mBeepOnNew = config->readBoolEntry("beep-on-mail", false);
    mSystemTrayOnNew = config->readBoolEntry("systray-on-mail", false);
    mSystemTrayMode = config->readBoolEntry("systray-on-new", true) ?
      KMSystemTray::OnNewMail :
      KMSystemTray::AlwaysOn;
    mConfirmEmpty = config->readBoolEntry("confirm-before-empty", true);
    // startup-Folder, defaults to system-inbox
	mStartupFolder = config->readEntry("startupFolder", kmkernel->inboxFolder()->idString());
    if (!mStartupDone)
    {
      // check mail on startup
      bool check = config->readBoolEntry("checkmail-startup", false);
      if (check) slotCheckMail();
    }
  }

  // Re-activate panners
  if (mStartupDone)
  {

    // Update systray
    toggleSystray(mSystemTrayOnNew, mSystemTrayMode);

    bool layoutChanged = ( oldLongFolderList != mLongFolderList )
                    || ( oldReaderWindowActive != mReaderWindowActive )
                    || ( oldReaderWindowBelow != mReaderWindowBelow );
    if ( layoutChanged ) {
      activatePanners();
    }

    //    kmkernel->kbp()->busy(); //Crashes KMail
    mFolderTree->reload();
    QListViewItem *qlvi = mFolderTree->indexOfFolder(mFolder);
    if (qlvi!=0) {
      mFolderTree->setCurrentItem(qlvi);
      mFolderTree->setSelected(qlvi,TRUE);
    }


    // sanders - New code
    mHeaders->setFolder(mFolder, true);
    if (mMsgView) {
      int aIdx = mHeaders->currentItemIndex();
      if (aIdx != -1)
        mMsgView->setMsg( mFolder->getMsg(aIdx), true );
      else
        mMsgView->clear( true );
    }
    updateMessageActions();
    show();
    // sanders - Maybe this fixes a bug?

  }
  updateMessageMenu();
}


//-----------------------------------------------------------------------------
void KMMainWidget::writeConfig(void)
{
  QString s;
  KConfig *config = KMKernel::config();
  KConfigGroup geometry( config, "Geometry" );
  KConfigGroup general( config, "General" );

  if (mMsgView)
    mMsgView->writeConfig();

  mFolderTree->writeConfig();

  geometry.writeEntry( "MainWin", this->geometry().size() );

  const QValueList<int> widths = ( mLongFolderList ? mPanner1 : mPanner2 )->sizes();
  const QValueList<int> heights = ( mLongFolderList ? mPanner2 : mPanner1 )->sizes();

  geometry.writeEntry( "FolderPaneWidth", widths[0] );
  geometry.writeEntry( "HeaderPaneWidth", widths[1] );
  geometry.writeEntry( "HeaderPaneHeight", heights[0] );
  geometry.writeEntry( "ReaderPaneHeight", heights[1] );

  // save the state of the unread/total-columns
  geometry.writeEntry( "UnreadColumn", mFolderTree->unreadIndex() );
  geometry.writeEntry( "TotalColumn", mFolderTree->totalIndex() );

  general.writeEntry("encoding", QString(mEncodingStr));
}


//-----------------------------------------------------------------------------
void KMMainWidget::createWidgets(void)
{
  QAccel *accel = new QAccel(this, "createWidgets()");

  // Create the splitters according to the layout settings
  QWidget *headerParent = 0, *folderParent = 0,
            *mimeParent = 0, *messageParent = 0;

#if KDE_IS_VERSION( 3, 1, 92 )
  const bool opaqueResize = KGlobalSettings::opaqueResize();
#else
  const bool opaqueResize = true;
#endif
  if ( mLongFolderList ) {
    // superior splitter: folder tree vs. rest
    // inferior splitter: headers vs. message vs. mime tree
    mPanner1 = new QSplitter( Qt::Horizontal, this, "panner 1" );
    mPanner1->setOpaqueResize( opaqueResize );
    Qt::Orientation orientation = mReaderWindowBelow ? Qt::Vertical : Qt::Horizontal;
    mPanner2 = new QSplitter( orientation, mPanner1, "panner 2" );
    mPanner2->setOpaqueResize( opaqueResize );
    folderParent = mPanner1;
    headerParent = mimeParent = messageParent = mPanner2;
  } else /* !mLongFolderList */ {
    // superior splitter: ( folder tree + headers ) vs. message vs. mime
    // inferior splitter: folder tree vs. headers
    mPanner1 = new QSplitter( Qt::Vertical, this, "panner 1" );
    mPanner1->setOpaqueResize( opaqueResize );
    mPanner2 = new QSplitter( Qt::Horizontal, mPanner1, "panner 2" );
    mPanner2->setOpaqueResize( opaqueResize );
    headerParent = folderParent = mPanner2;
    mimeParent = messageParent = mPanner1;
  }

#ifndef NDEBUG
  if( mPanner1 ) mPanner1->dumpObjectTree();
  if( mPanner2 ) mPanner2->dumpObjectTree();
#endif

  mTopLayout->add( mPanner1 );

  // BUG -sanders these accelerators stop working after switching
  // between long/short folder layout
  // Probably need to disconnect them first.

  // create list of messages
#ifndef NDEBUG
  headerParent->dumpObjectTree();
#endif
  mHeaders = new KMHeaders(this, headerParent, "headers");
  mHeaders->setFullWidth(true);
  if (mReaderWindowActive) {
    connect(mHeaders, SIGNAL(selected(KMMessage*)),
            this, SLOT(slotMsgSelected(KMMessage*)));
  }
  connect(mHeaders, SIGNAL(activated(KMMessage*)),
	  this, SLOT(slotMsgActivated(KMMessage*)));
  connect( mHeaders, SIGNAL( selectionChanged() ),
           SLOT( startUpdateMessageActionsTimer() ) );
  accel->connectItem(accel->insertItem(SHIFT+Key_Left),
                     mHeaders, SLOT(selectPrevMessage()));
  accel->connectItem(accel->insertItem(SHIFT+Key_Right),
                     mHeaders, SLOT(selectNextMessage()));

  if (!mEncodingStr.isEmpty())
    mCodec = KMMsgBase::codecForName(mEncodingStr);
  else mCodec = 0;

  if (mReaderWindowActive) {
    mMsgView = new KMReaderWin(messageParent, this, actionCollection(), 0 );

    connect(mMsgView, SIGNAL(replaceMsgByUnencryptedVersion()),
        this, SLOT(slotReplaceMsgByUnencryptedVersion()));
    connect(mMsgView, SIGNAL(popupMenu(KMMessage&,const KURL&,const QPoint&)),
        this, SLOT(slotMsgPopup(KMMessage&,const KURL&,const QPoint&)));
    connect(mMsgView, SIGNAL(urlClicked(const KURL&,int)),
        mMsgView, SLOT(slotUrlClicked()));
    connect(mHeaders, SIGNAL(maybeDeleting()),
        mMsgView, SLOT(clearCache()));
    connect(mMsgView, SIGNAL(noDrag()),
        mHeaders, SLOT(slotNoDrag()));
    connect(mMsgView, SIGNAL(statusMsg(const QString&)),
        this, SLOT(statusMsg(const QString&)));
    accel->connectItem(accel->insertItem(Key_Up),
        mMsgView, SLOT(slotScrollUp()));
    accel->connectItem(accel->insertItem(Key_Down),
        mMsgView, SLOT(slotScrollDown()));
    accel->connectItem(accel->insertItem(Key_Prior),
        mMsgView, SLOT(slotScrollPrior()));
    accel->connectItem(accel->insertItem(Key_Next),
        mMsgView, SLOT(slotScrollNext()));
  } else {
    mMsgView = NULL;
  }

  new KAction( i18n("Move Message to Folder"), Key_M, this,
               SLOT(slotMoveMsg()), actionCollection(),
               "move_message_to_folder" );
  new KAction( i18n("Copy Message to Folder"), Key_C, this,
               SLOT(slotCopyMsg()), actionCollection(),
               "copy_message_to_folder" );
  accel->connectItem(accel->insertItem(Key_M),
		     this, SLOT(slotMoveMsg()) );
  accel->connectItem(accel->insertItem(Key_C),
		     this, SLOT(slotCopyMsg()) );

  // create list of folders
  mFolderTree = new KMFolderTree(this, folderParent, "folderTree");

  connect(mFolderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderSelectedUnread(KMFolder*)),
	  this, SLOT(folderSelectedUnread(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderDrop(KMFolder*)),
	  this, SLOT(slotMoveMsgToFolder(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderDropCopy(KMFolder*)),
          this, SLOT(slotCopyMsgToFolder(KMFolder*)));
  connect(mFolderTree, SIGNAL(columnsChanged()),
          this, SLOT(slotFolderTreeColumnsChanged()));

  //Commands not worthy of menu items, but that deserve configurable keybindings
  new KAction(
    i18n("Remove Duplicate Messages"), CTRL+Key_Asterisk, this,
    SLOT(removeDuplicates()), actionCollection(), "remove_duplicate_messages");

  new KAction(
   i18n("Focus on Next Folder"), CTRL+Key_Right, mFolderTree,
   SLOT(incCurrentFolder()), actionCollection(), "inc_current_folder");
  accel->connectItem(accel->insertItem(CTRL+Key_Right),
                     mFolderTree, SLOT(incCurrentFolder()));


  new KAction(
   i18n("Focus on Previous Folder"), CTRL+Key_Left, mFolderTree,
   SLOT(decCurrentFolder()), actionCollection(), "dec_current_folder");
  accel->connectItem(accel->insertItem(CTRL+Key_Left),
                     mFolderTree, SLOT(decCurrentFolder()));

  new KAction(
   i18n("Select Folder with Focus"), CTRL+Key_Space, mFolderTree,
   SLOT(selectCurrentFolder()), actionCollection(), "select_current_folder");
  accel->connectItem(accel->insertItem(CTRL+Key_Space),
                     mFolderTree, SLOT(selectCurrentFolder()));

  connect( kmkernel->outboxFolder(), SIGNAL( msgRemoved(int, QString, QString) ),
           SLOT( startUpdateMessageActionsTimer() ) );
  connect( kmkernel->outboxFolder(), SIGNAL( msgAdded(int) ),
           SLOT( startUpdateMessageActionsTimer() ) );
}


//-----------------------------------------------------------------------------
void KMMainWidget::activatePanners(void)
{
  if (mMsgView) {
    QObject::disconnect( actionCollection()->action( "kmail_copy" ),
        SIGNAL( activated() ),
        mMsgView, SLOT( slotCopySelectedText() ));
  }
  if ( mLongFolderList ) {
    mHeaders->reparent( mPanner2, 0, QPoint( 0, 0 ) );
    if (mMsgView) {
      mMsgView->reparent( mPanner2, 0, QPoint( 0, 0 ) );
      mPanner2->moveToLast( mMsgView );
    }
    mFolderTree->reparent( mPanner1, 0, QPoint( 0, 0 ) );
    mPanner1->moveToLast( mPanner2 );
    mPanner1->setSizes( mPanner1Sep );
    mPanner1->setResizeMode( mFolderTree, QSplitter::KeepSize );
    mPanner2->setSizes( mPanner2Sep );
    mPanner2->setResizeMode( mHeaders, QSplitter::KeepSize );
  } else /* !mLongFolderList */ {
    mFolderTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
    mHeaders->reparent( mPanner2, 0, QPoint( 0, 0 ) );
    mPanner2->moveToLast( mHeaders );
    mPanner1->moveToFirst( mPanner2 );
    if (mMsgView) {
      mMsgView->reparent( mPanner1, 0, QPoint( 0, 0 ) );
      mPanner1->moveToLast( mMsgView );
    }
    mPanner1->setSizes( mPanner1Sep );
    mPanner2->setSizes( mPanner2Sep );
    mPanner1->setResizeMode( mPanner2, QSplitter::KeepSize );
    mPanner2->setResizeMode( mFolderTree, QSplitter::KeepSize );
  }

  if (mMsgView) {
    QObject::connect( actionCollection()->action( "kmail_copy" ),
		    SIGNAL( activated() ),
		    mMsgView, SLOT( slotCopySelectedText() ));
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotSetEncoding()
{
    mEncodingStr = KGlobal::charsets()->encodingForName(mEncoding->currentText()).latin1();
    if (mEncoding->currentItem() == 0) // Auto
    {
      mCodec = 0;
      mEncodingStr = "";
    }
    else
      mCodec = KMMsgBase::codecForName( mEncodingStr );
    if (mMsgView)
      mMsgView->setOverrideCodec(mCodec);
    return;
}

//-----------------------------------------------------------------------------
void KMMainWidget::hide()
{
  QWidget::hide();
}


//-----------------------------------------------------------------------------
void KMMainWidget::show()
{
  if( mPanner1 ) mPanner1->setSizes( mPanner1Sep );
  if( mPanner2 ) mPanner2->setSizes( mPanner2Sep );
  QWidget::show();
}

//-------------------------------------------------------------------------
void KMMainWidget::slotSearch()
{
  if(!mSearchWin)
  {
    mSearchWin = new KMFldSearch(this, "Search", mFolder, false);
    connect(mSearchWin, SIGNAL(destroyed()),
	    this, SLOT(slotSearchClosed()));
  }
  else
  {
    mSearchWin->activateFolder(mFolder);
  }

  mSearchWin->show();
  KWin::activateWindow( mSearchWin->winId() );
}


//-------------------------------------------------------------------------
void KMMainWidget::slotSearchClosed()
{
  mSearchWin = 0;
}


//-------------------------------------------------------------------------
void KMMainWidget::slotFind()
{
  if( mMsgView )
    mMsgView->slotFind();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotHelp()
{
  kapp->invokeHelp();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotNewMailReader()
{
  KMMainWin *d;

  d = new KMMainWin();
  d->show();
  d->resize(d->size());
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotFilter()
{
  kmkernel->filterMgr()->openDialog( this );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotPopFilter()
{
  kmkernel->popFilterMgr()->openDialog( this );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotAddrBook()
{
  KMAddrBookExternal::openAddressBook(this);
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotImport()
{
  KRun::runCommand("kmailcvt");
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotAddFolder()
{
  KMFolderDialog *d;

  d = new KMFolderDialog(0, &(kmkernel->folderMgr()->dir()),
			 this, i18n("Create Folder"));
  if (d->exec()) {
    mFolderTree->reload();
    QListViewItem *qlvi = mFolderTree->indexOfFolder( mFolder );
    if (qlvi) {
      qlvi->setOpen(TRUE);
      mFolderTree->setCurrentItem( qlvi );
    }
  }
  delete d;
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckMail()
{
 kmkernel->acctMgr()->checkMail(true);
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCheckOneAccount(int item)
{
  kmkernel->acctMgr()->intCheckMail(item);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMailChecked(bool newMail, bool sendOnCheck)
{
  if(mSendOnCheck && sendOnCheck)
    slotSendQueued();

  if (!newMail)
    return;

  if(kmkernel->xmlGuiInstance()) {
    KNotifyClient::Instance instance(kmkernel->xmlGuiInstance());
    KNotifyClient::event(0, "new-mail-arrived", i18n("New mail arrived"));
  }
  else
    KNotifyClient::event(0, "new-mail-arrived", i18n("New mail arrived"));
  if (mBeepOnNew) {
    KNotifyClient::beep();
  }
  kapp->dcopClient()->emitDCOPSignal( "unreadCountChanged()", QByteArray() );

  // Todo:
  // scroll mHeaders to show new items if current item would
  // still be visible
  //  mHeaders->showNewMail();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotCompose()
{
  KMComposeWin *win;
  KMMessage* msg = new KMMessage;

  if ( mFolder ) {
      msg->initHeader( mFolder->identity() );
      win = new KMComposeWin(msg, mFolder->identity());
  } else {
      msg->initHeader();
      win = new KMComposeWin(msg);
  }

  win->show();

}


//-----------------------------------------------------------------------------
void KMMainWidget::slotPostToML()
{
  KMComposeWin *win;
  KMMessage* msg = new KMMessage;

  if ( mFolder ) {
      msg->initHeader( mFolder->identity() );

      if (mFolder->isMailingList()) {
          kdDebug(5006)<<QString("mFolder->isMailingList() %1").arg( mFolder->mailingListPostAddress().latin1())<<endl;

          msg->setTo(mFolder->mailingListPostAddress());
      }
      win = new KMComposeWin(msg, mFolder->identity());
      win->setFocusToSubject();
  } else {
      msg->initHeader();
      win = new KMComposeWin(msg);
  }

  win->show();

}


//-----------------------------------------------------------------------------
void KMMainWidget::slotModifyFolder()
{
  if (!mFolderTree) return;
  KMFolderTreeItem *item = static_cast<KMFolderTreeItem*>( mFolderTree->currentItem() );
  if ( item )
    item->properties();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotExpireFolder()
{
  QString     str;
  bool        canBeExpired = true;

  if (!mFolder) return;

  if (!mFolder->isAutoExpire()) {
    canBeExpired = false;
  } else if (mFolder->getUnreadExpireUnits()==expireNever &&
	     mFolder->getReadExpireUnits()==expireNever) {
    canBeExpired = false;
  }

  if (!canBeExpired) {
    str = i18n("This folder does not have any expiry options set");
    KMessageBox::information(this, str);
    return;
  }
  KConfig           *config = KMKernel::config();
  KConfigGroupSaver saver(config, "General");

  if (config->readBoolEntry("warn-before-expire", true)) {
    str = i18n("<qt>Are you sure you want to expire the folder <b>%1</b>?</qt>").arg(mFolder->label());
    if (KMessageBox::warningContinueCancel(this, str, i18n("Expire Folder"),
					   i18n("&Expire"))
	!= KMessageBox::Continue) return;
  }

  mFolder->expireOldMessages();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEmptyFolder()
{
  QString str;
  KMMessage* msg;

  if (!mFolder) return;
  bool isTrash = kmkernel->folderIsTrash(mFolder);

  if (mConfirmEmpty)
  {
    QString title = (isTrash) ? i18n("Empty Trash") : i18n("Move to Trash");
    QString text = (isTrash) ?
      i18n("Are you sure you want to empty the trash folder?") :
      i18n("<qt>Are you sure you want to move all messages from "
           "folder <b>%1</b> to the trash?</qt>").arg(mFolder->label());

    if (KMessageBox::warningContinueCancel(this, text, title, title)
      != KMessageBox::Continue) return;
  }

  if (mFolder->folderType() == KMFolderTypeImap
      || mFolder->folderType() == KMFolderTypeSearch)
  {
    slotMarkAll();
    if (isTrash)
      slotDeleteMsg();
    else
      slotTrashMsg();
    return;
  }

  if (mMsgView)
    mMsgView->clearCache();

  KCursorSaver busy(KBusyPtr::busy());

  // begin of critical part
  // from here to "end..." no signal may change to another mFolder, otherwise
  // the wrong folder will be truncated in expunge (dnaber, 1999-08-29)
  mFolder->open();
  mHeaders->setFolder(0);
  if (mMsgView)
    mMsgView->clear();

  if (mFolder != kmkernel->trashFolder())
  {
    // FIXME: If we run out of disk space mail may be lost rather
    // than moved into the trash -sanders
    while ((msg = mFolder->take(0)) != 0) {
      kmkernel->trashFolder()->addMsg(msg);
      kmkernel->trashFolder()->unGetMsg(kmkernel->trashFolder()->count()-1);
    }
  }

  mFolder->close();
  mFolder->expunge();
  // end of critical
  if (mFolder != kmkernel->trashFolder())
    statusMsg(i18n("Moved all messages to the trash"));

  mHeaders->setFolder(mFolder);
  updateMessageActions();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotRemoveFolder()
{
  QString str;
  QDir dir;

  if (!mFolder) return;
  if (mFolder->isSystemFolder()) return;

  if ( mFolder->folderType() == KMFolderTypeSearch ) {
     str = i18n("<qt>Are you sure you want to delete the search folder "
                "<b>%1</b>? The messages displayed in it will not be deleted "
                "if you do so, as they are stored in a different folder.</qt>")

           .arg(mFolder->label());
  } else {
    if ( mFolder->count() == 0 ) {
      if ( !mFolder->child() || mFolder->child()->isEmpty() ) {
        str = i18n("<qt>Are you sure you want to delete the empty folder "
                   "<b>%1</b>?</qt>")
              .arg(mFolder->label());
      }
      else {
        str = i18n("<qt>Are you sure you want to delete the empty folder "
                   "<b>%1</b> and all its subfolders? Those subfolders "
                   "might not be empty and their  contents will be "
                   "discarded as well.</qt>")
              .arg(mFolder->label());
      }
    } else {
      if ( !mFolder->child() || mFolder->child()->isEmpty() ) {
        str = i18n("<qt>Are you sure you want to delete the folder "
                 "<b>%1</b>, discarding its contents?</qt>")
              .arg(mFolder->label());
      }
      else {
        str = i18n("<qt>Are you sure you want to delete the folder "
                 "<b>%1</b> and all its subfolders, discarding their "
                 "contents?</qt>")
            .arg(mFolder->label());
      }
    }
  }

  if (KMessageBox::warningContinueCancel(this, str, i18n("Delete Folder"),
                                         i18n("&Delete"))
      == KMessageBox::Continue)
  {
    if (mFolder->hasAccounts())
    {
      // this folder has an account, so we need to change that to the inbox
      KMAccount* acct = 0;
      KMAcctFolder* acctFolder = static_cast<KMAcctFolder*>(mFolder);
      for ( acct = acctFolder->account(); acct; acct = acctFolder->nextAccount() )
      {
        acct->setFolder(kmkernel->inboxFolder());
        KMessageBox::information(this,
            i18n("<qt>The destination folder of the account <b>%1</b> was restored to the inbox.</qt>").arg(acct->name()));
      }
    }
    if (mFolder->folderType() == KMFolderTypeImap)
      static_cast<KMFolderImap*>(mFolder)->removeOnServer();
    else if (mFolder->folderType() == KMFolderTypeSearch)
      kmkernel->searchFolderMgr()->remove(mFolder);
    else
      kmkernel->folderMgr()->remove(mFolder);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMarkAllAsRead()
{
  if (!mFolder)
    return;
  mFolder->markUnreadAsRead();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCompactFolder()
{
  int idx = mHeaders->currentItemIndex();
  if (mFolder)
  {
      KCursorSaver busy(KBusyPtr::busy());
      mFolder->compact();
  }
  mHeaders->setCurrentItemByIndex(idx);
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotRefreshFolder()
{
  if (mFolder)
  {
    if (mFolder->folderType() == KMFolderTypeImap)
    {
      KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder);
      imap->getAndCheckFolder();
    }
  }
}

void KMMainWidget::slotInvalidateIMAPFolders() {
  if ( KMessageBox::warningContinueCancel( this,
          i18n("Are you sure you want to refresh the IMAP cache?\n"
	       "This will remove all changes that you have done "
	       "locally to your IMAP folders."),
	  i18n("Refresh IMAP Cache"), i18n("&Refresh") ) == KMessageBox::Continue )
    kmkernel->acctMgr()->invalidateIMAPFolders();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotExpireAll() {
  KConfig    *config = KMKernel::config();
  int        ret = 0;

  KConfigGroupSaver saver(config, "General");

  if (config->readBoolEntry("warn-before-expire", true)) {
    ret = KMessageBox::warningContinueCancel(KMainWindow::memberList->first(),
			 i18n("Are you sure you want to expire all old messages?"),
			 i18n("Expire old Messages?"), i18n("Expire"));
    if (ret != KMessageBox::Continue) {
      return;
    }
  }

  kmkernel->folderMgr()->expireAllFolders();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCompactAll()
{
  KCursorSaver busy(KBusyPtr::busy());
  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folders;
  KMFolder *folder;
  mFolderTree->createFolderList(&strList, &folders);
  for (int i = 0; folders.at(i) != folders.end(); i++)
  {
    folder = *folders.at(i);
    if (!folder || folder->isDir()) continue;
    folder->compact();
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotOverrideHtml()
{
  if( mHtmlPref == mFolderHtmlPref ) {
    int result = KMessageBox::warningContinueCancel( this,
      // the warning text is taken from configuredialog.cpp:
      i18n( "Use of HTML in mail will make you more vulnerable to "
        "\"spam\" and may increase the likelihood that your system will be "
        "compromised by other present and anticipated security exploits." ),
      i18n( "Security Warning" ),
      i18n( "Continue" ),
      "OverrideHtmlWarning", false);
    if( result == KMessageBox::Cancel ) {
      mPreferHtmlAction->setChecked( false );
      return;
    }
  }
  mFolderHtmlPref = !mFolderHtmlPref;
  if (mMsgView) {
    mMsgView->setHtmlOverride(mFolderHtmlPref);
    mMsgView->update( true );
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotOverrideThread()
{
  mFolderThreadPref = !mFolderThreadPref;
  mHeaders->setNestedOverride(mFolderThreadPref);
  mThreadBySubjectAction->setEnabled(mThreadMessagesAction->isChecked());
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToggleSubjectThreading()
{
  mFolderThreadSubjPref = !mFolderThreadSubjPref;
  mHeaders->setSubjectThreading(mFolderThreadSubjPref);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMessageQueuedOrDrafted()
{
  if (!kmkernel->folderIsDraftOrOutbox(mFolder))
      return;
  if (mMsgView)
    mMsgView->update(true);
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardMsg()
{
  KMCommand *command =
    new KMForwardCommand( this, *mHeaders->selectedMsgs(), mFolder->identity() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotForwardAttachedMsg()
{
  KMCommand *command =
    new KMForwardAttachedCommand( this, *mHeaders->selectedMsgs(), mFolder->identity() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotEditMsg()
{
  KMCommand *command = new KMEditMsgCommand( this, mHeaders->currentMsg() );
  command->start();
}





//-----------------------------------------------------------------------------
void KMMainWidget::slotResendMsg()
{
  mHeaders->resendMsg();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotTrashMsg()
{
  mHeaders->deleteMsg();
  updateMessageActions();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotDeleteMsg()
{
  mHeaders->moveMsgToFolder(0);
  updateMessageActions();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotReplyToMsg()
{
  QString text = mMsgView? mMsgView->copyText() : "";
  KMCommand *command = new KMReplyToCommand( this, mHeaders->currentMsg(), text );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotReplyAuthorToMsg()
{
  QString text = mMsgView? mMsgView->copyText() : "";
  KMCommand *command = new KMReplyAuthorCommand( this, mHeaders->currentMsg(), text );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotReplyAllToMsg()
{
  QString text = mMsgView? mMsgView->copyText() : "";
  KMCommand *command = new KMReplyToAllCommand( this, mHeaders->currentMsg(), text );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotRedirectMsg()
{
  KMCommand *command = new KMRedirectCommand( this, mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotBounceMsg()
{
  KMCommand *command = new KMBounceCommand( this, mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotReplyListToMsg()
{

  QString text = mMsgView? mMsgView->copyText() : "";
  KMCommand *command = new KMReplyListCommand( this, mHeaders->currentMsg(),
					       text );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotNoQuoteReplyToMsg()
{
  KMCommand *command = new KMNoQuoteReplyToCommand( this, mHeaders->currentMsg() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSubjectFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;

  KMCommand *command = new KMFilterCommand( "Subject", msg->subject() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMailingListFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;

  KMCommand *command = new KMMailingListFilterCommand( this, msg );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFromFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;

  AddrSpecList al = msg->extractAddrSpecs( "From" );
  if ( al.empty() )
    return;
  KMCommand *command = new KMFilterCommand( "From",  al.front().asString() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;

  KMCommand *command = new KMFilterCommand( "To",  msg->to() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateListFilterAction()
{
  //Proxy the mListFilterAction to update the action text
  QCString name;
  QString value;
  QString lname = KMMLInfo::name( mHeaders->currentMsg(), name, value );
  mListFilterAction->setText( i18n("Filter on Mailing-List...") );
  if ( lname.isNull() )
    mListFilterAction->setEnabled( false );
  else {
    mListFilterAction->setEnabled( true );
    mListFilterAction->setText( i18n( "Filter on Mailing-List %1..." ).arg( lname ) );
  }
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotUndo()
{
    mHeaders->undo();
    updateMessageActions();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToggleUnread()
{
  mFolderTree->toggleColumn(KMFolderTree::unread);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotToggleTotalColumn()
{
  mFolderTree->toggleColumn(KMFolderTree::total, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMoveMsg()
{
  KMFolderSelDlg dlg(this,i18n("Move Message to Folder"));
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  mHeaders->moveMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMoveMsgToFolder( KMFolder *dest)
{
  mHeaders->moveMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCopyMsgToFolder( KMFolder *dest)
{
  mHeaders->copyMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotApplyFilters()
{
  mHeaders->applyFiltersOnMsg();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotEditVacation()
{
  if ( mVacation )
    return;

  mVacation = new Vacation( this );
  if ( mVacation->isUsable() ) {
    connect( mVacation, SIGNAL(result(bool)), mVacation, SLOT(deleteLater()) );
  } else {
    QString msg = i18n("KMail's Out of Office Reply functionality relies on "
                      "server-side filtering. You have not yet configured an "
                      "IMAP server for this.\n"
                      "You can do this on the \"Filtering\" tab of the IMAP "
                      "account configuration.");
    KMessageBox::sorry( this, msg, i18n("No Server-Side Filtering Configured") );

    delete mVacation; // QGuardedPtr sets itself to 0!
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotCopyMsg()
{
  KMFolderSelDlg dlg(this,i18n("Copy Message to Folder"));
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  mHeaders->copyMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotPrintMsg()
{
  bool htmlOverride = mMsgView ? mMsgView->htmlOverride() : false;
  KMCommand *command = new KMPrintCommand( this, mHeaders->currentMsg(),
      htmlOverride );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotConfigChanged()
{
  readConfig();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSaveMsg()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( this,
    *mHeaders->selectedMsgs() );

  if (saveCommand->url().isEmpty())
    delete saveCommand;
  else
    saveCommand->start();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSaveAttachments()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;
  KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand( this,
                                                                        *mHeaders->selectedMsgs() );
  saveCommand->start();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotSendQueued()
{
  kmkernel->msgSender()->sendQueued();
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotViewChange()
{
  if(mBodyPartsMenu->isItemChecked(mBodyPartsMenu->idAt(0)))
  {
    mBodyPartsMenu->setItemChecked(mBodyPartsMenu->idAt(0),FALSE);
    mBodyPartsMenu->setItemChecked(mBodyPartsMenu->idAt(1),TRUE);
  }
  else if(mBodyPartsMenu->isItemChecked(mBodyPartsMenu->idAt(1)))
  {
    mBodyPartsMenu->setItemChecked(mBodyPartsMenu->idAt(1),FALSE);
    mBodyPartsMenu->setItemChecked(mBodyPartsMenu->idAt(0),TRUE);
  }

  //mMsgView->setInline(!mMsgView->isInline());
}


void KMMainWidget::slotFancyHeaders() {
  mMsgView->setHeaderStyleAndStrategy( HeaderStyle::fancy(),
				       HeaderStrategy::rich() );
}

void KMMainWidget::slotBriefHeaders() {
  mMsgView->setHeaderStyleAndStrategy( HeaderStyle::brief(),
				       HeaderStrategy::brief() );
}

void KMMainWidget::slotStandardHeaders() {
  mMsgView->setHeaderStyleAndStrategy( HeaderStyle::plain(),
				       HeaderStrategy::standard());
}

void KMMainWidget::slotLongHeaders() {
  mMsgView->setHeaderStyleAndStrategy( HeaderStyle::plain(),
				       HeaderStrategy::rich() );
}

void KMMainWidget::slotAllHeaders() {
  mMsgView->setHeaderStyleAndStrategy( HeaderStyle::plain(),
				       HeaderStrategy::all() );
}

void KMMainWidget::slotCycleHeaderStyles() {
  const HeaderStrategy * strategy = mMsgView->headerStrategy();
  const HeaderStyle * style = mMsgView->headerStyle();

  const char * actionName = 0;
  if ( style == HeaderStyle::fancy() ) {
    slotBriefHeaders();
    actionName = "view_headers_brief";
  } else if ( style == HeaderStyle::brief() ) {
    slotStandardHeaders();
    actionName = "view_headers_standard";
  } else if ( style == HeaderStyle::plain() ) {
    if ( strategy == HeaderStrategy::standard() ) {
      slotLongHeaders();
      actionName = "view_headers_long";
    } else if ( strategy == HeaderStrategy::rich() ) {
      slotAllHeaders();
      actionName = "view_headers_all";
    } else if ( strategy == HeaderStrategy::all() ) {
      slotFancyHeaders();
      actionName = "view_headers_fancy";
    }
  }

  if ( actionName )
    static_cast<KRadioAction*>( actionCollection()->action( actionName ) )->setChecked( true );
}


void KMMainWidget::slotIconicAttachments() {
  mMsgView->setAttachmentStrategy( AttachmentStrategy::iconic() );
}

void KMMainWidget::slotSmartAttachments() {
  mMsgView->setAttachmentStrategy( AttachmentStrategy::smart() );
}

void KMMainWidget::slotInlineAttachments() {
  mMsgView->setAttachmentStrategy( AttachmentStrategy::inlined() );
}

void KMMainWidget::slotHideAttachments() {
  mMsgView->setAttachmentStrategy( AttachmentStrategy::hidden() );
}

void KMMainWidget::slotCycleAttachmentStrategy() {
  mMsgView->setAttachmentStrategy( mMsgView->attachmentStrategy()->next() );
  KRadioAction * action = actionForAttachmentStrategy( mMsgView->attachmentStrategy() );
  assert( action );
  action->setChecked( true );
}

void KMMainWidget::folderSelected(KMFolder* aFolder)
{
    folderSelected( aFolder, false );
}

KMLittleProgressDlg* KMMainWidget::progressDialog() const
{
    return mLittleProgress;
}

void KMMainWidget::folderSelectedUnread(KMFolder* aFolder)
{
    mHeaders->blockSignals( true );
    folderSelected( aFolder, true );
    QListViewItem *item = mHeaders->firstChild();
    while (item && item->itemAbove())
	item = item->itemAbove();
    mHeaders->setCurrentItem( item );
    mHeaders->nextUnreadMessage(true);
    mHeaders->blockSignals( false );
    mHeaders->highlightMessage( mHeaders->currentItem() );
    slotChangeCaption(mFolderTree->currentItem());
}

//-----------------------------------------------------------------------------
void KMMainWidget::folderSelected(KMFolder* aFolder, bool jumpToUnread)
{
  if( aFolder && mFolder == aFolder )
    return;

  KCursorSaver busy(KBusyPtr::busy());

  if (mMsgView)
    mMsgView->clear(true);

  if( !mFolder ) {
    if (mMsgView) {
      mMsgView->enableMsgDisplay();
      mMsgView->clear(true);
    }
    if( mHeaders )
      mHeaders->show();
  }

  if (mFolder && mFolder->needsCompacting() && (mFolder->folderType() == KMFolderTypeImap))
  {
    KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder);
    if (imap->autoExpunge())
      imap->expungeFolder(imap, TRUE);
  }
  writeFolderConfig();
  if ( mFolder ) {
     disconnect( mFolder, SIGNAL( changed() ),
           this, SLOT( updateMarkAsReadAction() ) );
     disconnect( mFolder, SIGNAL( msgHeaderChanged( KMFolder*, int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
     disconnect( mFolder, SIGNAL( msgAdded( int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
     disconnect( mFolder, SIGNAL( msgRemoved( KMFolder * ) ),
           this, SLOT( updateMarkAsReadAction() ) );

  }
  mFolder = (KMFolder*)aFolder;
  connect( mFolder, SIGNAL( changed() ),
           this, SLOT( updateMarkAsReadAction() ) );
  connect( mFolder, SIGNAL( msgHeaderChanged( KMFolder*, int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
  connect( mFolder, SIGNAL( msgAdded( int ) ),
           this, SLOT( updateMarkAsReadAction() ) );
  connect( mFolder, SIGNAL( msgRemoved(KMFolder *) ),
           this, SLOT( updateMarkAsReadAction() ) );

  readFolderConfig();
  if (mMsgView)
    mMsgView->setHtmlOverride(mFolderHtmlPref);
  mHeaders->setFolder( mFolder, jumpToUnread );
  updateMessageActions();
  updateFolderMenu();
  if (!aFolder)
    slotIntro();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMsgSelected(KMMessage *msg)
{
  if ( msg && msg->parent() && !msg->isComplete() )
  {
    if ( msg->transferInProgress() )
      return;
    mMsgView->clear();
    if ( mJob )
      disconnect( mJob, 0, this, 0 );
    mJob = msg->parent()->createJob( msg, FolderJob::tGetMessage, 0,
          "STRUCTURE", mMsgView->attachmentStrategy() );
    connect(mJob, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(slotUpdateImapMessage(KMMessage*)));
    mJob->start();
  } else {
    mMsgView->setMsg(msg);
  }
  // reset HTML override to the folder setting
  mMsgView->setHtmlOverride(mFolderHtmlPref);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMsgChanged()
{
  mHeaders->msgChanged();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSelectFolder(KMFolder* folder)
{
  QListViewItem* item = mFolderTree->indexOfFolder(folder);
  if (item)
    mFolderTree->doFolderSelected( item );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSelectMessage(KMMessage* msg)
{
  int idx = mFolder->find(msg);
  if (idx != -1) {
    mHeaders->setCurrentMsg(idx);
    if (mMsgView)
      mMsgView->setMsg(msg);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReplaceMsgByUnencryptedVersion()
{
  kdDebug(5006) << "KMMainWidget::slotReplaceMsgByUnencryptedVersion()" << endl;
  KMMessage* oldMsg = mHeaders->currentMsg();
  if( oldMsg ) {
    kdDebug(5006) << "KMMainWidget  -  old message found" << endl;
    if( oldMsg->hasUnencryptedMsg() ) {
      kdDebug(5006) << "KMMainWidget  -  extra unencrypted message found" << endl;
      KMMessage* newMsg = oldMsg->unencryptedMsg();
      // adjust the message id
      {
        QString msgId( oldMsg->msgId() );
        QString prefix("DecryptedMsg.");
        int oldIdx = msgId.find(prefix, 0, false);
        if( -1 == oldIdx ) {
          int leftAngle = msgId.findRev( '<' );
          msgId = msgId.insert( (-1 == leftAngle) ? 0 : ++leftAngle, prefix );
        }
        else {
          // toggle between "DecryptedMsg." and "DeCryptedMsg."
          // to avoid same message id
          QCharRef c = msgId[ oldIdx+2 ];
          if( 'C' == c )
            c = 'c';
          else
            c = 'C';
        }
        newMsg->setMsgId( msgId );
        mMsgView->setIdOfLastViewedMessage( msgId );
      }
      const QString newMsgIdMD5( newMsg->msgIdMD5() );
      // insert the unencrypted message
      kdDebug(5006) << "KMMainWidget  -  copying unencrypted message to same folder" << endl;
      mHeaders->copyMsgToFolder(mFolder, newMsg);
      // delete the encrypted message - this will also delete newMsg
      kdDebug(5006) << "KMMainWidget  -  deleting encrypted message" << endl;
      mHeaders->deleteMsg();
      kdDebug(5006) << "KMMainWidget  -  updating message actions" << endl;
      updateMessageActions();

      // find and select and show the new message
      int idx = mHeaders->currentItemIndex();
      if( -1 != idx ) {
        mHeaders->setCurrentMsg( idx );
        mMsgView->setMsg( mHeaders->currentMsg() );
      } else {
        kdDebug(5006) << "KMMainWidget  -  SORRY, could not store unencrypted message!" << endl;
      }

      kdDebug(5006) << "KMMainWidget  -  done." << endl;
    } else
      kdDebug(5006) << "KMMainWidget  -  NO EXTRA UNENCRYPTED MESSAGE FOUND" << endl;
  } else
    kdDebug(5006) << "KMMainWidget  -  PANIC: NO OLD MESSAGE FOUND" << endl;
}



//-----------------------------------------------------------------------------
void KMMainWidget::slotUpdateImapMessage(KMMessage *msg)
{
  if (msg && ((KMMsgBase*)msg)->isMessage()) {
    // don't update if we have since left the folder
    if ( mFolder &&
       ( mFolder == msg->parent()
      || mFolder->folderType() == KMFolderTypeSearch ) )
    {
      if ( msg->isComplete() )
        msg->cleanupHeader(); // assemble the message, important for signature-check
      mMsgView->setMsg(msg, TRUE);
    } else {
      kdDebug( 5006 ) <<  "KMMainWidget::slotUpdateImapMessage - ignoring update for already left folder" << endl;
    }
  }  else {
    // force an update of the folder
    if ( mFolder && mFolder->folderType() == KMFolderTypeImap )
      static_cast<KMFolderImap*>(mFolder)->getFolder(true);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusNew()
{
  mHeaders->setMsgStatus(KMMsgStatusNew);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusUnread()
{
  mHeaders->setMsgStatus(KMMsgStatusUnread);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusRead()
{
  mHeaders->setMsgStatus(KMMsgStatusRead);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusFlag()
{
  mHeaders->setMsgStatus(KMMsgStatusFlag, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusSpam()
{
  mHeaders->setMsgStatus( KMMsgStatusSpam, true );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusHam()
{
  mHeaders->setMsgStatus( KMMsgStatusHam, true );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusReplied()
{
  mHeaders->setMsgStatus(KMMsgStatusReplied, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusForwarded()
{
  mHeaders->setMsgStatus(KMMsgStatusForwarded, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusQueued()
{
  mHeaders->setMsgStatus(KMMsgStatusQueued, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetMsgStatusSent()
{
  mHeaders->setMsgStatus(KMMsgStatusSent, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusNew()
{
  mHeaders->setThreadStatus(KMMsgStatusNew);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusUnread()
{
  mHeaders->setThreadStatus(KMMsgStatusUnread);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusFlag()
{
  mHeaders->setThreadStatus(KMMsgStatusFlag, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusRead()
{
  mHeaders->setThreadStatus(KMMsgStatusRead);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusReplied()
{
  mHeaders->setThreadStatus(KMMsgStatusReplied, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusForwarded()
{
  mHeaders->setThreadStatus(KMMsgStatusForwarded, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusQueued()
{
  mHeaders->setThreadStatus(KMMsgStatusQueued, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusSent()
{
  mHeaders->setThreadStatus(KMMsgStatusSent, true);
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusWatched()
{
  mHeaders->setThreadStatus(KMMsgStatusWatched, true);
  if (mWatchThreadAction->isChecked()) {
    mIgnoreThreadAction->setChecked(false);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusIgnored()
{
  mHeaders->setThreadStatus(KMMsgStatusIgnored, true);
  if (mIgnoreThreadAction->isChecked()) {
    mWatchThreadAction->setChecked(false);
  }
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusSpam()
{
  mHeaders->setThreadStatus( KMMsgStatusSpam, true );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotSetThreadStatusHam()
{
  mHeaders->setThreadStatus( KMMsgStatusHam, true );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotNextMessage()       { mHeaders->nextMessage(); }
void KMMainWidget::slotNextUnreadMessage()
{
  if ( !mHeaders->nextUnreadMessage() )
    if ( mHeaders->loopOnGotoUnread() == LoopInAllFolders )
      mFolderTree->nextUnreadFolder(true);
}
void KMMainWidget::slotNextImportantMessage() {
  //mHeaders->nextImportantMessage();
}
void KMMainWidget::slotPrevMessage()       { mHeaders->prevMessage(); }
void KMMainWidget::slotPrevUnreadMessage()
{
  if ( !mHeaders->prevUnreadMessage() )
    if ( mHeaders->loopOnGotoUnread() == LoopInAllFolders )
      mFolderTree->prevUnreadFolder();
}
void KMMainWidget::slotPrevImportantMessage() {
  //mHeaders->prevImportantMessage();
}

//-----------------------------------------------------------------------------
//called from headers. Message must not be deleted on close
void KMMainWidget::slotMsgActivated(KMMessage *msg)
{
  if (msg->parent() && !msg->isComplete())
  {
    FolderJob *job = msg->parent()->createJob(msg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(slotMsgActivated(KMMessage*)));
    job->start();
    return;
  }

  if (kmkernel->folderIsDraftOrOutbox(mFolder))
  {
    slotEditMsg();
    return;
  }

  assert( msg != 0 );
  KMReaderMainWin *win = new KMReaderMainWin( mFolderHtmlPref );
  KMMessage *newMessage = new KMMessage();
  newMessage->fromString( msg->asString() );
  newMessage->setStatus( msg->status() );
  newMessage->setParent( msg->parent() );
  win->showMsg( mCodec, newMessage );
  win->resize( 550, 600 );
  win->show();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMarkAll()
{
  mHeaders->selectAll( TRUE );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotMsgPopup(KMMessage&, const KURL &aUrl, const QPoint& aPoint)
{
  KPopupMenu * menu = new KPopupMenu;
  updateMessageMenu();
  mUrlCurrent = aUrl;

  if (!aUrl.isEmpty())
  {
    if (aUrl.protocol() == "mailto")
    {
      // popup on a mailto URL
      mMsgView->mailToComposeAction()->plug( menu );
      if ( mMsgCurrent ) {
	mMsgView->mailToReplyAction()->plug( menu );
	mMsgView->mailToForwardAction()->plug( menu );
        menu->insertSeparator();
      }
      mMsgView->addAddrBookAction()->plug( menu );
      mMsgView->openAddrBookAction()->plug( menu );
      mMsgView->copyAction()->plug( menu );
    } else {
      // popup on a not-mailto URL
      mMsgView->urlOpenAction()->plug( menu );
      mMsgView->urlSaveAsAction()->plug( menu );
      mMsgView->copyURLAction()->plug( menu );
      mMsgView->addBookmarksAction()->plug( menu );
    }
  }
  else
  {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mHeaders->currentMsg()) // no messages
    {
      delete menu;
      return;
    }

    bool out_folder = kmkernel->folderIsDraftOrOutbox(mFolder);
    if ( out_folder ) {
      mEditAction->plug(menu);
    }
    else {
      mReplyAction->plug(menu);
      mReplyAllAction->plug(menu);
      mReplyAuthorAction->plug( menu );
      mReplyListAction->plug( menu );
      mForwardActionMenu->plug(menu);
      mBounceAction->plug(menu);
    }
    menu->insertSeparator();
    if ( !out_folder ) {
      //   mFilterMenu()->plug( menu );
      mStatusMenu->plug( menu );
      mThreadStatusMenu->plug( menu );
    }

    mCopyActionMenu->plug( menu );
    mMoveActionMenu->plug( menu );

    menu->insertSeparator();
    mWatchThreadAction->plug( menu );
    mIgnoreThreadAction->plug( menu );

    menu->insertSeparator();

    // these two only make sense if there is a reader window.
    // I guess. Not sure about view source ;). Till
    if (mMsgView) {
      toggleFixFontAction()->plug(menu);
      viewSourceAction()->plug(menu);
    }

    menu->insertSeparator();
    mPrintAction->plug( menu );
    mSaveAsAction->plug( menu );
    mSaveAttachmentsAction->plug( menu );
    menu->insertSeparator();
    mTrashAction->plug( menu );
    mDeleteAction->plug( menu );
  }
  KAcceleratorManager::manage(menu);
  menu->exec(aPoint, 0);
  delete menu;
}

//-----------------------------------------------------------------------------
void KMMainWidget::getAccountMenu()
{
  QStringList actList;

  mActMenu->clear();
  actList = kmkernel->acctMgr()->getAccounts(false);
  QStringList::Iterator it;
  int id = 0;
  for(it = actList.begin(); it != actList.end() ; ++it, id++)
    mActMenu->insertItem((*it).replace("&", "&&"), id);
}

// little helper function
KRadioAction * KMMainWidget::actionForHeaderStyle( const HeaderStyle * style, const HeaderStrategy * strategy ) {
  const char * actionName = 0;
  if ( style == HeaderStyle::fancy() )
    actionName = "view_headers_fancy";
  else if ( style == HeaderStyle::brief() )
    actionName = "view_headers_brief";
  else if ( style == HeaderStyle::plain() ) {
    if ( strategy == HeaderStrategy::standard() )
      actionName = "view_headers_standard";
    else if ( strategy == HeaderStrategy::rich() )
      actionName = "view_headers_long";
    else if ( strategy == HeaderStrategy::all() )
      actionName = "view_headers_all";
  }
  if ( actionName )
    return static_cast<KRadioAction*>(actionCollection()->action(actionName));
  else
    return 0;
}

KRadioAction * KMMainWidget::actionForAttachmentStrategy( const AttachmentStrategy * as ) {
  const char * actionName = 0;
  if ( as == AttachmentStrategy::iconic() )
    actionName = "view_attachments_as_icons";
  else if ( as == AttachmentStrategy::smart() )
    actionName = "view_attachments_smart";
  else if ( as == AttachmentStrategy::inlined() )
    actionName = "view_attachments_inline";
  else if ( as == AttachmentStrategy::hidden() )
    actionName = "view_attachments_hide";

  if ( actionName )
    return static_cast<KRadioAction*>(actionCollection()->action(actionName));
  else
    return 0;
}


//-----------------------------------------------------------------------------
void KMMainWidget::setupActions()
{
  //----- File Menu
  (void) new KAction( i18n("New &Window"), "window_new", 0,
		      this, SLOT(slotNewMailReader()),
		      actionCollection(), "new_mail_client" );

  mSaveAsAction = new KAction( i18n("Save &As..."), "filesave",
    KStdAccel::shortcut(KStdAccel::Save),
    this, SLOT(slotSaveMsg()), actionCollection(), "file_save_as" );

  (void) new KAction( i18n("&Compact All Folders"), 0,
		      this, SLOT(slotCompactAll()),
		      actionCollection(), "compact_all_folders" );

  (void) new KAction( i18n("&Expire All Folders"), 0,
		      this, SLOT(slotExpireAll()),
		      actionCollection(), "expire_all_folders" );

  (void) new KAction( i18n("&Refresh Local IMAP Cache"), "refresh",
		      this, SLOT(slotInvalidateIMAPFolders()),
		      actionCollection(), "file_invalidate_imap_cache" );

  (void) new KAction( i18n("Empty &Trash"), 0,
		      KMKernel::self(), SLOT(slotEmptyTrash()),
		      actionCollection(), "empty_trash" );

  (void) new KAction( i18n("Check &Mail"), "mail_get", CTRL+Key_L,
		      this, SLOT(slotCheckMail()),
		      actionCollection(), "check_mail" );

  KActionMenu *actActionMenu = new
    KActionMenu( i18n("Check Mail &In"), "mail_get", actionCollection(),
				   	"check_mail_in" );
  actActionMenu->setDelayed(true); //needed for checking "all accounts"

  connect(actActionMenu,SIGNAL(activated()),this,SLOT(slotCheckMail()));

  mActMenu = actActionMenu->popupMenu();
  connect(mActMenu,SIGNAL(activated(int)),this,SLOT(slotCheckOneAccount(int)));
  connect(mActMenu,SIGNAL(aboutToShow()),this,SLOT(getAccountMenu()));

  (void) new KAction( i18n("&Send Queued Messages"), "mail_send", 0, this,
		     SLOT(slotSendQueued()), actionCollection(), "send_queued");
  KAction *act;
  //----- Tools menu
  if (parent()->inherits("KMMainWin")) {
    act =  new KAction( i18n("&Address Book..."), "contents", 0, this,
			SLOT(slotAddrBook()), actionCollection(), "addressbook" );
    if (KStandardDirs::findExe("kaddressbook").isEmpty()) act->setEnabled(false);
  }

  act = new KAction( i18n("&Import Messages..."), "fileopen", 0, this,
		     SLOT(slotImport()), actionCollection(), "import" );
  if (KStandardDirs::findExe("kmailcvt").isEmpty()) act->setEnabled(false);

#if 0 // ### (marc) this isn't ready for prime-time yet - reactivate post 3.2
  (void) new KAction( i18n("Edit \"Out of Office\" Replies..."),
		      "configure", 0, this, SLOT(slotEditVacation()),
		      actionCollection(), "tools_edit_vacation" );
#endif

  //----- Edit Menu
  mTrashAction = new KAction( KGuiItem( i18n("&Move to Trash"), "edittrash",
                                       i18n("Move message to trashcan") ),
                             Key_Delete, this, SLOT(slotTrashMsg()),
                             actionCollection(), "move_to_trash" );

  mDeleteAction = new KAction( i18n("&Delete"), "editdelete", SHIFT+Key_Delete, this,
                              SLOT(slotDeleteMsg()), actionCollection(), "delete" );

  (void) new KAction( i18n("&Find Messages..."), "mail_find", Key_S, this,
		      SLOT(slotSearch()), actionCollection(), "search_messages" );

  mFindInMessageAction = new KAction( i18n("&Find in Message..."), "find", KStdAccel::shortcut(KStdAccel::Find), this,
		      SLOT(slotFind()), actionCollection(), "find_in_messages" );

  (void) new KAction( i18n("Select &All Messages"), KStdAccel::selectAll(), this,
		      SLOT(slotMarkAll()), actionCollection(), "mark_all_messages" );

  (void) new KAction( i18n("Select Message &Text"),
		      CTRL+SHIFT+Key_A, mMsgView,
		      SLOT(selectAll()), actionCollection(), "mark_all_text" );

  //----- Folder Menu
  (void) new KAction( i18n("&New Folder..."), "folder_new", 0, this,
		      SLOT(slotAddFolder()), actionCollection(), "new_folder" );

  mModifyFolderAction = new KAction( i18n("&Properties"), "configure", 0, this,
		      SLOT(slotModifyFolder()), actionCollection(), "modify" );

  mMarkAllAsReadAction = new KAction( i18n("Mark All Messages as &Read"), "goto", 0, this,
		      SLOT(slotMarkAllAsRead()), actionCollection(), "mark_all_as_read" );

  mExpireFolderAction = new KAction(i18n("&Expire"), 0, this, SLOT(slotExpireFolder()),
				   actionCollection(), "expire");

  mCompactFolderAction = new KAction( i18n("&Compact"), 0, this,
		      SLOT(slotCompactFolder()), actionCollection(), "compact" );

  mRefreshFolderAction = new KAction( i18n("Check Mail &in this Folder"), "reload", Key_F5 , this,
                     SLOT(slotRefreshFolder()), actionCollection(), "refresh_folder" );

  mEmptyFolderAction = new KAction( i18n("&Move All Messages to Trash"),
                                   "edittrash", 0, this,
		      SLOT(slotEmptyFolder()), actionCollection(), "empty" );

  mRemoveFolderAction = new KAction( i18n("&Delete Folder"), "editdelete", 0, this,
		      SLOT(slotRemoveFolder()), actionCollection(), "delete_folder" );

  mPreferHtmlAction = new KToggleAction( i18n("Prefer &HTML to Plain Text"), 0, this,
		      SLOT(slotOverrideHtml()), actionCollection(), "prefer_html" );

  mThreadMessagesAction = new KToggleAction( i18n("&Thread Messages"), 0, this,
		      SLOT(slotOverrideThread()), actionCollection(), "thread_messages" );

  mThreadBySubjectAction = new KToggleAction( i18n("Thread Messages also by &Subject"), 0, this,
		      SLOT(slotToggleSubjectThreading()), actionCollection(), "thread_messages_by_subject" );


  //----- Message Menu
  (void) new KAction( i18n("&New Message..."), "mail_new", KStdAccel::shortcut(KStdAccel::New), this,
		      SLOT(slotCompose()), actionCollection(), "new_message" );

  (void) new KAction( i18n("New Message t&o Mailing-List..."), "mail_post_to", 0, this,
		      SLOT(slotPostToML()), actionCollection(), "post_message" );

  mForwardActionMenu = new KActionMenu( i18n("Message->","&Forward"),
					"mail_forward", actionCollection(),
					"message_forward" );
  connect( mForwardActionMenu, SIGNAL(activated()), this,
	   SLOT(slotForwardMsg()) );

  mForwardAttachedAction = new KAction( i18n("Message->Forward->","As &Attachment..."),
				       "mail_forward", Key_F, this,
					SLOT(slotForwardAttachedMsg()), actionCollection(),
					"message_forward_as_attachment" );
  mForwardActionMenu->insert( forwardAttachedAction() );
  mForwardAction = new KAction( i18n("&Inline..."), "mail_forward",
				SHIFT+Key_F, this, SLOT(slotForwardMsg()),
				actionCollection(), "message_forward_inline" );

  mForwardActionMenu->insert( forwardAction() );

  mSendAgainAction = new KAction( i18n("Send A&gain..."), 0, this,
		      SLOT(slotResendMsg()), actionCollection(), "send_again" );

  mReplyActionMenu = new KActionMenu( i18n("Message->","&Reply"),
                                      "mail_reply", actionCollection(),
                                      "message_reply_menu" );
  connect( mReplyActionMenu, SIGNAL(activated()), this,
	   SLOT(slotReplyToMsg()) );

  mReplyAction = new KAction( i18n("&Reply..."), "mail_reply", Key_R, this,
			      SLOT(slotReplyToMsg()), actionCollection(), "reply" );
  mReplyActionMenu->insert( mReplyAction );

  mReplyAuthorAction = new KAction( i18n("Reply to A&uthor..."), "mail_reply",
                                    SHIFT+Key_A, this,
                                    SLOT(slotReplyAuthorToMsg()),
                                    actionCollection(), "reply_author" );
  mReplyActionMenu->insert( mReplyAuthorAction );

  mReplyAllAction = new KAction( i18n("Reply to &All..."), "mail_replyall",
				 Key_A, this, SLOT(slotReplyAllToMsg()),
				 actionCollection(), "reply_all" );
  mReplyActionMenu->insert( mReplyAllAction );

  mReplyListAction = new KAction( i18n("Reply to Mailing-&List..."),
				  "mail_replylist", Key_L, this,
				  SLOT(slotReplyListToMsg()), actionCollection(),
				  "reply_list" );
  mReplyActionMenu->insert( mReplyListAction );

  mRedirectAction = new KAction( i18n("Message->Forward->","&Redirect..."),
				 Key_E, this, SLOT(slotRedirectMsg()),
				 actionCollection(), "message_forward_redirect" );
  mForwardActionMenu->insert( redirectAction() );

  mNoQuoteReplyAction = new KAction( i18n("Reply Without &Quote..."), SHIFT+Key_R,
    this, SLOT(slotNoQuoteReplyToMsg()), actionCollection(), "noquotereply" );

  //---- Bounce action
  mBounceAction = new KAction( i18n("&Bounce..."), 0, this,
			      SLOT(slotBounceMsg()), actionCollection(), "bounce" );

  //----- Create filter actions
  mFilterMenu = new KActionMenu( i18n("&Create Filter"), actionCollection(), "create_filter" );

  mSubjectFilterAction = new KAction( i18n("Filter on &Subject..."), 0, this,
				      SLOT(slotSubjectFilter()),
				      actionCollection(), "subject_filter");
  mFilterMenu->insert( mSubjectFilterAction );

  mFromFilterAction = new KAction( i18n("Filter on &From..."), 0, this,
				   SLOT(slotFromFilter()),
				   actionCollection(), "from_filter");
  mFilterMenu->insert( mFromFilterAction );

  mToFilterAction = new KAction( i18n("Filter on &To..."), 0, this,
				 SLOT(slotToFilter()),
				 actionCollection(), "to_filter");
  mFilterMenu->insert( mToFilterAction );

  mListFilterAction = new KAction( i18n("Filter on Mailing-&List..."), 0, this,
                                   SLOT(slotMailingListFilter()), actionCollection(),
                                   "mlist_filter");
  mFilterMenu->insert( mListFilterAction );

  mPrintAction = KStdAction::print (this, SLOT(slotPrintMsg()), actionCollection());

  //----- Message-Encoding Submenu
  mEncoding = new KSelectAction( i18n( "&Set Encoding" ), "charset", 0, this,
		      SLOT( slotSetEncoding() ), actionCollection(), "encoding" );
  QStringList encodings = KMMsgBase::supportedEncodings(FALSE);
  encodings.prepend( i18n( "Auto" ) );
  mEncoding->setItems( encodings );
  mEncoding->setCurrentItem(0);

  QStringList::Iterator it;
  int i = 0;
  for( it = encodings.begin(); it != encodings.end(); ++it)
  {
    if ( KGlobal::charsets()->encodingForName(*it ) == QString(mEncodingStr) )
    {
      mEncoding->setCurrentItem( i );
      break;
    }
    i++;
  }

  mEditAction = new KAction( i18n("&Edit Message"), "edit", Key_T, this,
                            SLOT(slotEditMsg()), actionCollection(), "edit" );

  //----- "Mark Message" submenu
  mStatusMenu = new KActionMenu ( i18n( "Mar&k Message" ),
                                 actionCollection(), "set_status" );

  mStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &New"), "kmmsgnew",
                                          i18n("Mark selected messages as new")),
                                 0, this, SLOT(slotSetMsgStatusNew()),
                                 actionCollection(), "status_new" ));

  mStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Unread"), "kmmsgunseen",
                                          i18n("Mark selected messages as unread")),
                                 0, this, SLOT(slotSetMsgStatusUnread()),
                                 actionCollection(), "status_unread"));

  mStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Read"), "kmmsgread",
                                          i18n("Mark selected messages as read")),
                                 0, this, SLOT(slotSetMsgStatusRead()),
                                 actionCollection(), "status_read"));

  // -------- Toggle Actions
  mToggleRepliedAction = new KToggleAction(i18n("Mark Message as Re&plied"), "kmmsgreplied",
                                 0, this, SLOT(slotSetMsgStatusReplied()),
                                 actionCollection(), "status_replied");

  mStatusMenu->insert( mToggleRepliedAction );
  mToggleForwardedAction = new KToggleAction(i18n("Mark Message as &Forwarded"), "kmmsgforwarded",
                                 0, this, SLOT(slotSetMsgStatusForwarded()),
                                 actionCollection(), "status_forwarded");
  mStatusMenu->insert( mToggleForwardedAction );

  mToggleQueuedAction = new KToggleAction(i18n("Mark Message as &Queued"), "kmmsgqueued",
                                 0, this, SLOT(slotSetMsgStatusQueued()),
                                 actionCollection(), "status_queued");
  mStatusMenu->insert( mToggleQueuedAction );

  mToggleSentAction = new KToggleAction(i18n("Mark Message as &Sent"), "kmmsgsent",
                                 0, this, SLOT(slotSetMsgStatusSent()),
                                 actionCollection(), "status_sent");
  mStatusMenu->insert( mToggleSentAction );

  mToggleFlagAction = new KToggleAction(i18n("Mark Message as &Important"), "kmmsgflag",
                                 0, this, SLOT(slotSetMsgStatusFlag()),
                                 actionCollection(), "status_flag");
  mStatusMenu->insert( mToggleFlagAction );

  mMarkAsSpamAction = new KAction(i18n("Mark Message as Spa&m"), "mark_as_spam",
                                 0, this, SLOT(slotSetMsgStatusSpam()),
                                 actionCollection(), "status_spam");
  mStatusMenu->insert( mMarkAsSpamAction );

  mMarkAsHamAction = new KAction(i18n("Mark Message as &Ham"), "mark_as_ham",
                                 0, this, SLOT(slotSetMsgStatusHam()),
                                 actionCollection(), "status_ham");
  mStatusMenu->insert( mMarkAsHamAction );

  //----- "Mark Thread" submenu
  mThreadStatusMenu = new KActionMenu ( i18n( "Mark &Thread" ),
                                       actionCollection(), "thread_status" );

  mMarkThreadAsNewAction = new KAction(KGuiItem(i18n("Mark Thread as &New"), "kmmsgnew",
                                               i18n("Mark all messages in the selected thread as new")),
                                               0, this, SLOT(slotSetThreadStatusNew()),
                                               actionCollection(), "thread_new");
  mThreadStatusMenu->insert( mMarkThreadAsNewAction );

  mMarkThreadAsUnreadAction = new KAction(KGuiItem(i18n("Mark Thread as &Unread"), "kmmsgunseen",
                                                i18n("Mark all messages in the selected thread as unread")),
                                                0, this, SLOT(slotSetThreadStatusUnread()),
                                                actionCollection(), "thread_unread");
  mThreadStatusMenu->insert( mMarkThreadAsUnreadAction );

  mMarkThreadAsReadAction = new KAction(KGuiItem(i18n("Mark Thread as &Read"), "kmmsgread",
                                                i18n("Mark all messages in the selected thread as read")),
                                                0, this, SLOT(slotSetThreadStatusRead()),
                                                actionCollection(), "thread_read");
  mThreadStatusMenu->insert( mMarkThreadAsReadAction );

  //----- "Mark Thread" toggle actions
  mToggleThreadRepliedAction = new KToggleAction(i18n("Mark Thread as R&eplied"), "kmmsgreplied",
                                       0, this, SLOT(slotSetThreadStatusReplied()),
                                       actionCollection(), "thread_replied");
  mThreadStatusMenu->insert( mToggleThreadRepliedAction );
  mToggleThreadForwardedAction = new KToggleAction(i18n("Mark Thread as &Forwarded"), "kmmsgforwarded",
                                       0, this, SLOT(slotSetThreadStatusForwarded()),
                                       actionCollection(), "thread_forwarded");
  mThreadStatusMenu->insert( mToggleThreadForwardedAction );
  mToggleThreadQueuedAction = new KToggleAction(i18n("Mark Thread as &Queued"), "kmmsgqueued",
                                       0, this, SLOT(slotSetThreadStatusQueued()),
                                       actionCollection(), "thread_queued");
  mThreadStatusMenu->insert( mToggleThreadQueuedAction );
  mToggleThreadSentAction = new KToggleAction(i18n("Mark Thread as &Sent"), "kmmsgsent",
                                       0, this, SLOT(slotSetThreadStatusSent()),
                                       actionCollection(), "thread_sent");
  mThreadStatusMenu->insert( mToggleThreadSentAction );
  mToggleThreadFlagAction = new KToggleAction(i18n("Mark Thread as &Important"), "kmmsgflag",
                                       0, this, SLOT(slotSetThreadStatusFlag()),
                                       actionCollection(), "thread_flag");
  mThreadStatusMenu->insert( mToggleThreadFlagAction );
  //------- "Watch and ignore thread" actions
  mWatchThreadAction = new KToggleAction(i18n("&Watch Thread"), "kmmsgwatched",
                                       0, this, SLOT(slotSetThreadStatusWatched()),
                                       actionCollection(), "thread_watched");

  mIgnoreThreadAction = new KToggleAction(i18n("&Ignore Thread"), "kmmsgignored",
                                       0, this, SLOT(slotSetThreadStatusIgnored()),
                                       actionCollection(), "thread_ignored");

  //------- "Ham and spam thread" actions
  mMarkThreadAsSpamAction = new KAction(i18n("Mark Thread as S&pam"), "mark_as_spam",
                                       0, this, SLOT(slotSetThreadStatusSpam()),
                                       actionCollection(), "thread_spam");
  mThreadStatusMenu->insert( mMarkThreadAsSpamAction );

  mMarkThreadAsHamAction = new KAction(i18n("Mark Thread as &Ham"), "mark_as_ham",
                                       0, this, SLOT(slotSetThreadStatusHam()),
                                       actionCollection(), "thread_ham");
  mThreadStatusMenu->insert( mMarkThreadAsHamAction );


  mSaveAttachmentsAction = new KAction( i18n("Save A&ttachments..."), "attach",
                                0, this, SLOT(slotSaveAttachments()),
                                actionCollection(), "file_save_attachments" );

  mMoveActionMenu = new KActionMenu( i18n("&Move To" ),
                                    actionCollection(), "move_to" );

  mCopyActionMenu = new KActionMenu( i18n("&Copy To" ),
                                    actionCollection(), "copy_to" );

  mApplyFiltersAction = new KAction( i18n("Appl&y Filters"), "filter",
				    CTRL+Key_J, this,
				    SLOT(slotApplyFilters()),
				    actionCollection(), "apply_filters" );

  mApplyFilterActionsMenu = new KActionMenu( i18n("A&pply Filter Actions" ),
					    actionCollection(),
					    "apply_filter_actions" );

  //----- View Menu
  KRadioAction * raction = 0;

  // "Headers" submenu:
  KActionMenu * headerMenu =
    new KActionMenu( i18n("View->", "&Headers"),
		     actionCollection(), "view_headers" );
  headerMenu->setToolTip( i18n("Choose display style of message headers") );

  if (mMsgView) {
    connect( headerMenu, SIGNAL(activated()), SLOT(slotCycleHeaderStyles()) );

    raction = new KRadioAction( i18n("View->headers->", "&Fancy Headers"), 0, this,
        SLOT(slotFancyHeaders()),
        actionCollection(), "view_headers_fancy" );
    raction->setToolTip( i18n("Show the list of headers in a fancy format") );
    raction->setExclusiveGroup( "view_headers_group" );
    headerMenu->insert( raction );

    raction = new KRadioAction( i18n("View->headers->", "&Brief Headers"), 0, this,
        SLOT(slotBriefHeaders()),
        actionCollection(), "view_headers_brief" );
    raction->setToolTip( i18n("Show brief list of message headers") );
    raction->setExclusiveGroup( "view_headers_group" );
    headerMenu->insert( raction );

    raction = new KRadioAction( i18n("View->headers->", "&Standard Headers"), 0, this,
        SLOT(slotStandardHeaders()),
        actionCollection(), "view_headers_standard" );
    raction->setToolTip( i18n("Show standard list of message headers") );
    raction->setExclusiveGroup( "view_headers_group" );
    headerMenu->insert( raction );

    raction = new KRadioAction( i18n("View->headers->", "&Long Headers"), 0, this,
        SLOT(slotLongHeaders()),
        actionCollection(), "view_headers_long" );
    raction->setToolTip( i18n("Show long list of message headers") );
    raction->setExclusiveGroup( "view_headers_group" );
    headerMenu->insert( raction );

    raction = new KRadioAction( i18n("View->headers->", "&All Headers"), 0, this,
        SLOT(slotAllHeaders()),
        actionCollection(), "view_headers_all" );
    raction->setToolTip( i18n("Show all message headers") );
    raction->setExclusiveGroup( "view_headers_group" );
    headerMenu->insert( raction );

    // check the right one:
    raction = actionForHeaderStyle( mMsgView->headerStyle(), mMsgView->headerStrategy() );
    if ( raction )
      raction->setChecked( true );

    // "Attachments" submenu:
    KActionMenu * attachmentMenu =
      new KActionMenu( i18n("View->", "&Attachments"),
          actionCollection(), "view_attachments" );
    connect( attachmentMenu, SIGNAL(activated()),
        SLOT(slotCycleAttachmentStrategy()) );

    attachmentMenu->setToolTip( i18n("Choose display style of attachments") );

    raction = new KRadioAction( i18n("View->attachments->", "&As Icons"), 0, this,
        SLOT(slotIconicAttachments()),
        actionCollection(), "view_attachments_as_icons" );
    raction->setToolTip( i18n("Show all attachments as icons. Click to see them.") );
    raction->setExclusiveGroup( "view_attachments_group" );
    attachmentMenu->insert( raction );

    raction = new KRadioAction( i18n("View->attachments->", "&Smart"), 0, this,
        SLOT(slotSmartAttachments()),
        actionCollection(), "view_attachments_smart" );
    raction->setToolTip( i18n("Show attachments as suggested by sender.") );
    raction->setExclusiveGroup( "view_attachments_group" );
    attachmentMenu->insert( raction );

    raction = new KRadioAction( i18n("View->attachments->", "&Inline"), 0, this,
        SLOT(slotInlineAttachments()),
        actionCollection(), "view_attachments_inline" );
    raction->setToolTip( i18n("Show all attachments inline (if possible)") );
    raction->setExclusiveGroup( "view_attachments_group" );
    attachmentMenu->insert( raction );

    raction = new KRadioAction( i18n("View->attachments->", "&Hide"), 0, this,
        SLOT(slotHideAttachments()),
        actionCollection(), "view_attachments_hide" );
    raction->setToolTip( i18n("Don't show attachments in the message viewer") );
    raction->setExclusiveGroup( "view_attachments_group" );
    attachmentMenu->insert( raction );

    // check the right one:
    raction = actionForAttachmentStrategy( mMsgView->attachmentStrategy() );
    if ( raction )
      raction->setChecked( true );
  }
  // Unread Submenu
  KActionMenu * unreadMenu =
    new KActionMenu( i18n("View->", "&Unread Count"),
		     actionCollection(), "view_unread" );
  unreadMenu->setToolTip( i18n("Choose how to display the count of unread messages") );

  mUnreadColumnToggle = new KRadioAction( i18n("View->Unread Count", "View in &Separate Column"), 0, this,
			       SLOT(slotToggleUnread()),
			       actionCollection(), "view_unread_column" );
  mUnreadColumnToggle->setExclusiveGroup( "view_unread_group" );
  mUnreadColumnToggle->setChecked( mFolderTree->isUnreadActive() );
  unreadMenu->insert( mUnreadColumnToggle );

  mUnreadTextToggle = new KRadioAction( i18n("View->Unread Count", "View After &Folder Name"), 0, this,
			       SLOT(slotToggleUnread()),
			       actionCollection(), "view_unread_text" );
  mUnreadTextToggle->setExclusiveGroup( "view_unread_group" );
  mUnreadTextToggle->setChecked( !mFolderTree->isUnreadActive() );
  unreadMenu->insert( mUnreadTextToggle );

  // toggle for total column
  mTotalColumnToggle = new KToggleAction( i18n("View->", "&Total Column"), 0, this,
			       SLOT(slotToggleTotalColumn()),
			       actionCollection(), "view_columns_total" );
  mTotalColumnToggle->setToolTip( i18n("Toggle display of column showing the "
                                      "total number of messages in folders.") );
  mTotalColumnToggle->setChecked( mFolderTree->isTotalActive() );

  (void)new KAction( KGuiItem( i18n("View->","&Expand Thread"), QString::null,
			       i18n("Expand the current thread") ),
		     Key_Period, this,
		     SLOT(slotExpandThread()),
		     actionCollection(), "expand_thread" );

  (void)new KAction( KGuiItem( i18n("View->","&Collapse Thread"), QString::null,
			       i18n("Collapse the current thread") ),
		     Key_Comma, this,
		     SLOT(slotCollapseThread()),
		     actionCollection(), "collapse_thread" );

  (void)new KAction( KGuiItem( i18n("View->","Ex&pand All Threads"), QString::null,
			       i18n("Expand all threads in the current folder") ),
		     CTRL+Key_Period, this,
		     SLOT(slotExpandAllThreads()),
		     actionCollection(), "expand_all_threads" );

  (void)new KAction( KGuiItem( i18n("View->","C&ollapse All Threads"), QString::null,
			       i18n("Collapse all threads in the current folder") ),
		     CTRL+Key_Comma, this,
		     SLOT(slotCollapseAllThreads()),
		     actionCollection(), "collapse_all_threads" );


  //----- Go Menu
  new KAction( KGuiItem( i18n("&Next Message"), QString::null,
                         i18n("Go to the next message") ),
                         "N;Right", this, SLOT(slotNextMessage()),
                         actionCollection(), "go_next_message" );

  new KAction( KGuiItem( i18n("Next &Unread Message"),
                         QApplication::reverseLayout() ? "previous" : "next",
                         i18n("Go to the next unread message") ),
                         Key_Plus, this, SLOT(slotNextUnreadMessage()),
                         actionCollection(), "go_next_unread_message" );

  /* ### needs better support from folders:
  new KAction( KGuiItem( i18n("Next &Important Message"), QString::null,
                         i18n("Go to the next important message") ),
                         0, this, SLOT(slotNextImportantMessage()),
                         actionCollection(), "go_next_important_message" );
  */

  new KAction( KGuiItem( i18n("&Previous Message"), QString::null,
                         i18n("Go to the previous message") ),
                         "P;Left", this, SLOT(slotPrevMessage()),
                         actionCollection(), "go_prev_message" );

  new KAction( KGuiItem( i18n("Previous Unread &Message"),
                         QApplication::reverseLayout() ? "next" : "previous",
                         i18n("Go to the previous unread message") ),
                         Key_Minus, this, SLOT(slotPrevUnreadMessage()),
                         actionCollection(), "go_prev_unread_message" );

  /* needs better support from folders:
  new KAction( KGuiItem( i18n("Previous I&mportant Message"), QString::null,
                         i18n("Go to the previous important message") ),
                         0, this, SLOT(slotPrevImportantMessage()),
                         actionCollection(), "go_prev_important_message" );
  */

  new KAction( KGuiItem( i18n("Next Unread &Folder"), QString::null,
                         i18n("Go to the next folder with unread messages") ),
                         CTRL+Key_Plus, this, SLOT(slotNextUnreadFolder()),
                         actionCollection(), "go_next_unread_folder" );

  new KAction( KGuiItem( i18n("Previous Unread F&older"), QString::null,
                         i18n("Go to the previous folder with unread messages") ),
                         CTRL+Key_Minus, this, SLOT(slotPrevUnreadFolder()),
                         actionCollection(), "go_prev_unread_folder" );

  new KAction( KGuiItem( i18n("Go->","Next Unread &Text"), QString::null,
                         i18n("Go to the next unread text"),
                         i18n("Scroll down current message. "
                              "If at end of current message, "
                              "go to next unread message.") ),
                         Key_Space, this, SLOT(slotReadOn()),
                         actionCollection(), "go_next_unread_text" );

  //----- Settings Menu
  (void) new KAction( i18n("Configure &Filters..."), 0, this,
 		      SLOT(slotFilter()), actionCollection(), "filter" );
  (void) new KAction( i18n("Configure &POP Filters..."), 0, this,
 		      SLOT(slotPopFilter()), actionCollection(), "popFilter" );

  (void) new KAction( KGuiItem( i18n("KMail &Introduction"), 0,
				i18n("Display KMail's Welcome Page") ),
		      0, this, SLOT(slotIntro()),
		      actionCollection(), "help_kmail_welcomepage" );

  // ----- Standard Actions
//  KStdAction::configureNotifications(this, SLOT(slotEditNotifications()), actionCollection());
  (void) new KAction( i18n("Configure &Notifications..."),
		      "knotify", 0, this,
 		      SLOT(slotEditNotifications()), actionCollection(),
		      "kmail_configure_notifications" );
//  KStdAction::preferences(this, SLOT(slotSettings()), actionCollection());
  (void) new KAction( i18n("&Configure KMail..."),
		      "configure", 0, kmkernel,
                      SLOT(slotShowConfigurationDialog()), actionCollection(),
                      "kmail_configure_kmail" );

  KStdAction::undo(this, SLOT(slotUndo()), actionCollection(), "kmail_undo");
//  (void) new KAction( i18n("&Undo"), 0, this,
// 		      SLOT(slotUndo()), actionCollection(),
//		      "kmail_undo" );

  KStdAction::copy( messageView(), SLOT(slotCopySelectedText()), actionCollection(), "kmail_copy");
//  (void) new KAction( i18n("&Copy"), CTRL+Key_C, mMsgView,
// 		      SLOT(slotCopySelectedText()), actionCollection(),
//		      "kmail_copy" );

  KStdAction::tipOfDay( this, SLOT( slotShowTip() ), actionCollection() );

  menutimer = new QTimer( this, "menutimer" );
  connect( menutimer, SIGNAL( timeout() ), SLOT( updateMessageActions() ) );
  connect( kmkernel->undoStack(),
           SIGNAL( undoStackChanged() ), this, SLOT( slotUpdateUndo() ));

  initializeFilterActions();
  updateMessageActions();
}

//-----------------------------------------------------------------------------
void KMMainWidget::setupStatusBar()
{
  //we setup the progress dialog here, because its the one widget
  //we want to export to the part.
  KMainWindow *mainWin = dynamic_cast<KMainWindow*>(topLevelWidget());
  KStatusBar *bar =  mainWin ? mainWin->statusBar() : 0;
  mLittleProgress = new KMLittleProgressDlg( bar );

  //mLittleProgress->show();
  connect( KMBroadcastStatus::instance(), SIGNAL(statusProgressEnable( bool )),
           mLittleProgress, SLOT(slotEnable( bool )));
  connect( KMBroadcastStatus::instance(),
           SIGNAL(statusProgressPercent( unsigned long )),
           mLittleProgress,
           SLOT(slotJustPercent( unsigned long )));
  connect( KMBroadcastStatus::instance(),
           SIGNAL(signalUsingSSL( bool )),
           mLittleProgress,
           SLOT(slotSetSSL(bool)) );
  connect( KMBroadcastStatus::instance(), SIGNAL(resetRequested()),
           mLittleProgress, SLOT(slotClean()));
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotEditNotifications()
{
  if(kmkernel->xmlGuiInstance())
    KNotifyDialog::configure(this, 0, kmkernel->xmlGuiInstance()->aboutData());
  else
    KNotifyDialog::configure(this);
}

void KMMainWidget::slotEditKeys()
{
  KKeyDialog::configure( actionCollection(),
			 true /*allow one-letter shortcuts*/
			 );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotReadOn()
{
    if ( !mMsgView )
        return;

    if ( !mMsgView->atBottom() ) {
        mMsgView->slotJumpDown();
        return;
    }
    slotNextUnreadMessage();
}

void KMMainWidget::slotNextUnreadFolder() {
  if ( !mFolderTree ) return;
  mFolderTree->nextUnreadFolder();
}

void KMMainWidget::slotPrevUnreadFolder() {
  if ( !mFolderTree ) return;
  mFolderTree->prevUnreadFolder();
}

void KMMainWidget::slotExpandThread()
{
  mHeaders->slotExpandOrCollapseThread( true ); // expand
}

void KMMainWidget::slotCollapseThread()
{
  mHeaders->slotExpandOrCollapseThread( false ); // collapse
}

void KMMainWidget::slotExpandAllThreads()
{
  mHeaders->slotExpandOrCollapseAllThreads( true ); // expand
}

void KMMainWidget::slotCollapseAllThreads()
{
  mHeaders->slotExpandOrCollapseAllThreads( false ); // collapse
}


//-----------------------------------------------------------------------------
void KMMainWidget::moveSelectedToFolder( int menuId )
{
  if (mMenuToFolder[menuId])
    mHeaders->moveMsgToFolder( mMenuToFolder[menuId] );
}


//-----------------------------------------------------------------------------
void KMMainWidget::copySelectedToFolder(int menuId )
{
  if (mMenuToFolder[menuId])
    mHeaders->copyMsgToFolder( mMenuToFolder[menuId] );
}


//-----------------------------------------------------------------------------
void KMMainWidget::updateMessageMenu()
{
    mMenuToFolder.clear();
    KMMenuCommand::folderToPopupMenu( true, this, &mMenuToFolder, mMoveActionMenu->popupMenu() );
    KMMenuCommand::folderToPopupMenu( false, this, &mMenuToFolder, mCopyActionMenu->popupMenu() );
    updateMessageActions();
}

void KMMainWidget::startUpdateMessageActionsTimer()
{
    menutimer->stop();
    menutimer->start( 20, true );
}

void KMMainWidget::updateMessageActions()
{
    int count = 0;
    QPtrList<QListViewItem> selectedItems;

    if ( mFolder ) {
        for (QListViewItem *item = mHeaders->firstChild(); item; item = item->itemBelow())
            if (item->isSelected() )
	        selectedItems.append(item);
        if ( selectedItems.isEmpty() && mFolder->count() ) // there will always be one in mMsgView
            count = 1;
	else count = selectedItems.count();
    }

    updateListFilterAction();

    bool allSelectedInCommonThread = true;
    if ( count > 1 && mHeaders->isThreaded() ) {
      QListViewItem * curItemParent = mHeaders->currentItem();
      while ( curItemParent->parent() )
	curItemParent = curItemParent->parent();
      for ( QPtrListIterator<QListViewItem> it( selectedItems ) ;
	    it.current() ; ++ it ) {
	QListViewItem * item = *it;
	while ( item->parent() )
	  item = item->parent();
	if ( item != curItemParent ) {
	  allSelectedInCommonThread = false;
	  break;
	}
      }
    }

    bool mass_actions = count >= 1;
    bool thread_actions = mass_actions &&
           allSelectedInCommonThread &&
           mHeaders->isThreaded();
    mStatusMenu->setEnabled( mass_actions );
    mThreadStatusMenu->setEnabled( thread_actions );
    // these need to be handled individually, the user might have them
    // in the toolbar
    mWatchThreadAction->setEnabled( thread_actions );
    mIgnoreThreadAction->setEnabled( thread_actions );
    mMarkThreadAsSpamAction->setEnabled( thread_actions );
    mMarkThreadAsHamAction->setEnabled( thread_actions );
    mMarkThreadAsNewAction->setEnabled( thread_actions );
    mMarkThreadAsReadAction->setEnabled( thread_actions );
    mMarkThreadAsUnreadAction->setEnabled( thread_actions );
    mToggleThreadRepliedAction->setEnabled( thread_actions );
    mToggleThreadForwardedAction->setEnabled( thread_actions );
    mToggleThreadQueuedAction->setEnabled( thread_actions );
    mToggleThreadSentAction->setEnabled( thread_actions );
    mToggleThreadFlagAction->setEnabled( thread_actions );

    if (mFolder && mHeaders && mHeaders->currentMsg()) {
      mToggleRepliedAction->setChecked(mHeaders->currentMsg()->isReplied());
      mToggleForwardedAction->setChecked(mHeaders->currentMsg()->isForwarded());
      mToggleQueuedAction->setChecked(mHeaders->currentMsg()->isQueued());
      mToggleSentAction->setChecked(mHeaders->currentMsg()->isSent());
      mToggleFlagAction->setChecked(mHeaders->currentMsg()->isFlag());
      if (thread_actions) {
        mToggleThreadRepliedAction->setChecked(mHeaders->currentMsg()->isReplied());
        mToggleThreadForwardedAction->setChecked(mHeaders->currentMsg()->isForwarded());
        mToggleThreadQueuedAction->setChecked(mHeaders->currentMsg()->isQueued());
        mToggleThreadSentAction->setChecked(mHeaders->currentMsg()->isSent());
        mToggleThreadFlagAction->setChecked(mHeaders->currentMsg()->isFlag());
        mWatchThreadAction->setChecked( mHeaders->currentMsg()->isWatched());
        mIgnoreThreadAction->setChecked( mHeaders->currentMsg()->isIgnored());
      }
    }

    mMoveActionMenu->setEnabled( mass_actions );
    mCopyActionMenu->setEnabled( mass_actions );
    mTrashAction->setEnabled( mass_actions );
    mDeleteAction->setEnabled( mass_actions );
    mFindInMessageAction->setEnabled( mass_actions );
    mForwardAction->setEnabled( mass_actions );
    mForwardAttachedAction->setEnabled( mass_actions );

    forwardMenu()->setEnabled( mass_actions );

    bool single_actions = count == 1;
    mEditAction->setEnabled( single_actions &&
    kmkernel->folderIsDraftOrOutbox(mFolder));

    filterMenu()->setEnabled( single_actions );
    bounceAction()->setEnabled( single_actions );
    replyAction()->setEnabled( single_actions );
    noQuoteReplyAction()->setEnabled( single_actions );
    replyAuthorAction()->setEnabled( single_actions );
    replyAllAction()->setEnabled( single_actions );
    replyListAction()->setEnabled( single_actions );
    redirectAction()->setEnabled( single_actions );
    printAction()->setEnabled( single_actions );
    if (mMsgView) {
      viewSourceAction()->setEnabled( single_actions );
    }

    mSendAgainAction->setEnabled( single_actions &&
             ( mHeaders->currentMsg() && mHeaders->currentMsg()->isSent() )
          || ( mFolder && kmkernel->folderIsDraftOrOutbox( mFolder ) )
          || ( mFolder && kmkernel->folderIsSentMailFolder( mFolder ) )
             );
    mSaveAsAction->setEnabled( mass_actions );
    bool mails = mFolder && mFolder->count();
    bool enable_goto_unread = mails || (mHeaders->loopOnGotoUnread() == LoopInAllFolders);
    actionCollection()->action( "go_next_message" )->setEnabled( mails );
    actionCollection()->action( "go_next_unread_message" )->setEnabled( enable_goto_unread );
    actionCollection()->action( "go_prev_message" )->setEnabled( mails );
    actionCollection()->action( "go_prev_unread_message" )->setEnabled( enable_goto_unread );
    actionCollection()->action( "send_queued" )->setEnabled( kmkernel->outboxFolder()->count() > 0 );
    if (action( "edit_undo" ))
      action( "edit_undo" )->setEnabled( mHeaders->canUndo() );

    if ( count == 1 ) {
      KMMessage *msg;
      int aIdx;
      if((aIdx = mHeaders->currentItemIndex()) <= -1)
        return;
      if(!(msg = mFolder->getMsg(aIdx)))
        return;

      if (mFolder == kmkernel->outboxFolder())
        mEditAction->setEnabled( !msg->transferInProgress() );
    }

    mApplyFiltersAction->setEnabled(count);
    mApplyFilterActionsMenu->setEnabled(count && (mApplyFilterActionsMenu->popupMenu()->count()>0));
}


//-----------------------------------------------------------------------------
void KMMainWidget::statusMsg(const QString& message)
{
  KMMainWin *mainKMWin = dynamic_cast<KMMainWin*>(topLevelWidget());
  if (mainKMWin)
    return mainKMWin->statusMsg( message );

  KMainWindow *mainWin = dynamic_cast<KMainWindow*>(topLevelWidget());
  if (mainWin && mainWin->statusBar())
    mainWin->statusBar()->message( message );
}


// This needs to be updated more often, so it is in its method.
void KMMainWidget::updateMarkAsReadAction()
{
  mMarkAllAsReadAction->setEnabled( mFolder && (mFolder->countUnread() > 0) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::updateFolderMenu()
{
  mModifyFolderAction->setEnabled( mFolder ? !mFolder->noContent() : false );
  mCompactFolderAction->setEnabled( mFolder ? !mFolder->noContent() : false );
  mRefreshFolderAction->setEnabled( mFolder ? !mFolder->noContent()
                                            && mFolder->folderType()==KMFolderTypeImap
                                            : false );
  mEmptyFolderAction->setEnabled( mFolder ? ( !mFolder->noContent()
                                             && ( mFolder->count() > 0 ) )
                                         : false );
  mEmptyFolderAction->setText( (mFolder && kmkernel->folderIsTrash(mFolder))
    ? i18n("E&mpty Trash") : i18n("&Move All Messages to Trash") );
  mRemoveFolderAction->setEnabled( (mFolder && !mFolder->isSystemFolder()) );
  mExpireFolderAction->setEnabled( mFolder && mFolder->isAutoExpire() );
  updateMarkAsReadAction();
  mPreferHtmlAction->setEnabled( mFolder ? true : false );
  mThreadMessagesAction->setEnabled( mFolder ? true : false );

  mPreferHtmlAction->setChecked( mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref );
  mThreadMessagesAction->setChecked(
      mThreadPref ? !mFolderThreadPref : mFolderThreadPref );
  mThreadBySubjectAction->setEnabled(
      mFolder ? ( mThreadMessagesAction->isChecked()) : false );
  mThreadBySubjectAction->setChecked( mFolderThreadSubjPref );
}


#ifdef MALLOC_DEBUG
static QString fmt(long n) {
  char buf[32];

  if(n > 1024*1024*1024)
    sprintf(buf, "%0.2f GB", ((double)n)/1024.0/1024.0/1024.0);
  else if(n > 1024*1024)
    sprintf(buf, "%0.2f MB", ((double)n)/1024.0/1024.0);
  else if(n > 1024)
    sprintf(buf, "%0.2f KB", ((double)n)/1024.0);
  else
    sprintf(buf, "%ld Byte", n);
  return QString(buf);
}
#endif

void KMMainWidget::slotMemInfo() {
#ifdef MALLOC_DEBUG
  struct mallinfo mi;

  mi = mallinfo();
  QString s = QString("\nMALLOC - Info\n\n"
		      "Number of mmapped regions : %1\n"
		      "Memory allocated in use   : %2\n"
		      "Memory allocated, not used: %3\n"
		      "Memory total allocated    : %4\n"
		      "Max. freeable memory      : %5\n")
    .arg(mi.hblks).arg(fmt(mi.uordblks)).arg(fmt(mi.fordblks))
    .arg(fmt(mi.arena)).arg(fmt(mi.keepcost));
  KMessageBox::information(0, s, "Malloc information", s);
#endif
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotIntro()
{
  if ( !mMsgView ) return;

  mMsgView->clear( true );
  // hide widgets that are in the way:
  if ( mHeaders && mLongFolderList )
    mHeaders->hide();

  mMsgView->displayAboutPage();

  mFolder = 0;
}

void KMMainWidget::slotShowStartupFolder()
{
  if (mFolderTree) {
    // add the folders
    mFolderTree->reload();
    // read the config
    mFolderTree->readConfig();
    // get rid of old-folders
    mFolderTree->cleanupConfigFile();
  }

  connect( kmkernel->filterMgr(), SIGNAL( filterListUpdated() ),
	   this, SLOT( initializeFilterActions() ));

  if (kmkernel->firstStart() || kmkernel->previousVersion() != KMAIL_VERSION) {
    slotIntro();
    return;
  }

  KMFolder* startup = 0;
  if (!mStartupFolder.isEmpty()) {
    // find the startup-folder with this ugly folderMgr switch
    startup = kmkernel->folderMgr()->findIdString(mStartupFolder);
    if (!startup)
      startup = kmkernel->imapFolderMgr()->findIdString(mStartupFolder);
    if (!startup)
      startup = kmkernel->dimapFolderMgr()->findIdString(mStartupFolder);
    if (!startup)
      startup = kmkernel->inboxFolder();
  } else {
    startup = kmkernel->inboxFolder();
  }
  mFolderTree->doFolderSelected(mFolderTree->indexOfFolder(startup));
  mFolderTree->ensureItemVisible(mFolderTree->indexOfFolder(startup));
}

void KMMainWidget::slotShowTipOnStart()
{
  KTipDialog::showTip( this );
}

void KMMainWidget::slotShowTip()
{
  KTipDialog::showTip( this, QString::null, true );
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotChangeCaption(QListViewItem * i)
{
  // set the caption to the current full path
  QStringList names;
  for ( QListViewItem * item = i ; item ; item = item->parent() )
    names.prepend( item->text(0) );
  emit captionChangeRequest( names.join( "/" ) );
}

//-----------------------------------------------------------------------------
void KMMainWidget::removeDuplicates()
{
  if (!mFolder)
    return;
  KMFolder *oFolder = mFolder;
  mHeaders->setFolder(0);
  QMap< QString, QValueList<int> > idMD5s;
  QValueList<int> redundantIds;
  QValueList<int>::Iterator kt;
  mFolder->open();
  for (int i = mFolder->count() - 1; i >= 0; --i) {
    QString id = (*mFolder)[i]->msgIdMD5();
    if ( !id.isEmpty() ) {
      QString subjMD5 = (*mFolder)[i]->strippedSubjectMD5();
      int other = -1;
      if ( idMD5s.contains(id) )
        other = idMD5s[id].first();
      else
        idMD5s[id].append( i );
      if ( other != -1 ) {
        QString otherSubjMD5 = (*mFolder)[other]->strippedSubjectMD5();
        if (otherSubjMD5 == subjMD5)
          idMD5s[id].append( i );
      }
    }
  }
  QMap< QString, QValueList<int> >::Iterator it;
  for ( it = idMD5s.begin(); it != idMD5s.end() ; ++it ) {
    QValueList<int>::Iterator jt;
    bool finished = false;
    for ( jt = (*it).begin(); jt != (*it).end() && !finished; ++jt )
      if (!((*mFolder)[*jt]->isUnread())) {
        (*it).remove( jt );
        (*it).prepend( *jt );
        finished = true;
      }
    for ( jt = (*it).begin(), ++jt; jt != (*it).end(); ++jt )
      redundantIds.append( *jt );
  }
  qHeapSort( redundantIds );
  kt = redundantIds.end();
  int numDuplicates = 0;
  if (kt != redundantIds.begin()) do {
    mFolder->removeMsg( *(--kt) );
    ++numDuplicates;
  }
  while (kt != redundantIds.begin());

  mFolder->close();
  mHeaders->setFolder(oFolder);
  QString msg;
  if ( numDuplicates )
    msg = i18n("Removed %n duplicate message.",
               "Removed %n duplicate messages.", numDuplicates );
    else
      msg = i18n("No duplicate messages found.");
  KMBroadcastStatus::instance()->setStatusMsg( msg );
}


//-----------------------------------------------------------------------------
void KMMainWidget::slotUpdateUndo()
{
    if (actionCollection()->action( "edit_undo" ))
        actionCollection()->action( "edit_undo" )->setEnabled( mHeaders->canUndo() );
}


//-----------------------------------------------------------------------------
void KMMainWidget::initializeFilterActions()
{
  QString filterName, normalizedName;
  KMMetaFilterActionCommand *filterCommand;
  KAction *filterAction;
  mFilterActions.clear();
  mFilterCommands.clear();
  for ( QPtrListIterator<KMFilter> it(*kmkernel->filterMgr()) ;
        it.current() ; ++it )
    if (!(*it)->isEmpty() && (*it)->configureShortcut()) {
      filterName = QString("Filter Action %1").arg((*it)->name());
      normalizedName = filterName.replace(" ", "_");
      if (action(normalizedName.utf8()))
        continue;
      filterCommand = new KMMetaFilterActionCommand(*it, mHeaders, this);
      mFilterCommands.append(filterCommand);
      QString as = i18n("Filter Action %1").arg((*it)->name());
      QString icon = (*it)->icon();
      if ( icon.isEmpty() )
        icon = "gear";
      filterAction = new KAction(as, icon, 0, filterCommand,
                                 SLOT(start()), actionCollection(),
                                 normalizedName.local8Bit());
      mFilterActions.append(filterAction);
    }

  mApplyFilterActionsMenu->popupMenu()->clear();
  plugFilterActions(mApplyFilterActionsMenu->popupMenu());
}


//-----------------------------------------------------------------------------
void KMMainWidget::plugFilterActions(QPopupMenu *menu)
{
  for (QPtrListIterator<KMFilter> it(*kmkernel->filterMgr()); it.current(); ++it)
      if (!(*it)->isEmpty() && (*it)->configureShortcut()) {
	  QString filterName = QString("Filter Action %1").arg((*it)->name());
	  filterName = filterName.replace(" ","_");
	  KAction *filterAction = action(filterName.local8Bit());
	  if (filterAction && menu)
	      filterAction->plug(menu);
      }
}

void KMMainWidget::slotSubscriptionDialog()
{
  if (!mFolder) return;

  ImapAccountBase* account;
  if (mFolder->folderType() == KMFolderTypeImap)
  {
    account = static_cast<KMFolderImap*>(mFolder)->account();
  } else if (mFolder->folderType() == KMFolderTypeCachedImap)
  {
    account = static_cast<KMFolderCachedImap*>(mFolder)->account();
  } else
    return;

  SubscriptionDialog *dialog = new SubscriptionDialog(this,
      i18n("Subscription"),
      account);
  if ( dialog->exec() )
    account->listDirectory();
}

//-----------------------------------------------------------------------------
void KMMainWidget::slotFolderTreeColumnsChanged()
{
  mTotalColumnToggle->setChecked( mFolderTree->isTotalActive() );
  mUnreadColumnToggle->setChecked( mFolderTree->isUnreadActive() );
}

void KMMainWidget::toggleSystray(bool enabled, int mode)
{
  kdDebug(5006) << "setupSystray called" << endl;
  if (enabled && !mSystemTray)
  {
    mSystemTray = new KMSystemTray();
  }
  else if (!enabled && mSystemTray)
  {
    /** Get rid of system tray on user's request */
    kdDebug(5006) << "deleting systray" << endl;
    delete mSystemTray;
    mSystemTray = 0;
  }

  /** Set mode of systemtray.  If mode has changed, tray will handle this */
  if(mSystemTray)
  {
    kdDebug(5006) << "Setting system tray mode" << endl;
    mSystemTray->setMode(mode);
  }
}
