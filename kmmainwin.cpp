// kmmainwin.cpp
//#define MALLOC_DEBUG 1

#include <kwin.h>

#ifdef MALLOC_DEBUG
#include <malloc.h>
#endif

#undef Unsorted // X headers...
#include <qaccel.h>
#include <qregexp.h>
#include <qmap.h>
#include <qvaluelist.h>
#include <qtextcodec.h>

#include <kopenwith.h>

#include <kmessagebox.h>

#include <kparts/browserextension.h>

#include <kaction.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kcharsets.h>
#include <kmimetype.h>
#include <knotifyclient.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kio/netaccess.h>
#include <kio/job.h>

#include "configuredialog.h"
#include "kmbroadcaststatus.h"
#include "kmfoldermgr.h"
#include "kmfolderdia.h"
#include "kmacctmgr.h"
#include "kbusyptr.h"
#include "kmfoldertree.h"
#include "kmreaderwin.h"
#include "kmfolderimap.h"
#include "kmcomposewin.h"
#include "kmfolderseldlg.h"
#include "kmfiltermgr.h"
#include "kmsender.h"
#include "kmaddrbook.h"
#include "kmversion.h"
#include "kmfldsearch.h"
#include "mailinglist-magic.h"
#include "kmmsgdict.h"
#include "kmacctfolder.h"
#include "kmmimeparttree.h"


#include <assert.h>
#include <kstatusbar.h>
#include <kpopupmenu.h>

#include "kmmainwin.moc"

//-----------------------------------------------------------------------------
KMMainWin::KMMainWin(QWidget *, char *name) :
    KMMainWinInherited(name)
{
  // must be the first line of the constructor:
  searchWin = 0;
  mStartupDone = FALSE;
  mbNewMBVisible = false;
  QListViewItem* idx;
  mIntegrated  = TRUE;
  mFolder = NULL;
  mFolderThreadPref = false;
  mFolderHtmlPref = false;


  mPanner1Sep = new QValueList<int>;
  mPanner2Sep = new QValueList<int>;
  mPanner3Sep = new QValueList<int>;
  *mPanner1Sep << 1 << 1;
  *mPanner2Sep << 1 << 1;
  *mPanner3Sep << 1 << 1;


  setMinimumSize(400, 300);

  mConfigureDialog = 0;

  readPreConfig();
  createWidgets();

  setupMenuBar();
  setupStatusBar();

  applyMainWindowSettings(kapp->config(), "Main Window");
  toolbarAction->setChecked(!toolBar()->isHidden());
  statusbarAction->setChecked(!statusBar()->isHidden());

  readConfig();

  activatePanners();


  if (kernel->firstStart() || kernel->previousVersion() != KMAIL_VERSION)
    idx = mFolderTree->firstChild();
  else
    idx = mFolderTree->indexOfFolder(kernel->inboxFolder());
  if (idx!=0) {
    mFolderTree->doFolderSelected(idx);
  }

  connect(kernel->msgSender(), SIGNAL(statusMsg(const QString&)),
	  SLOT(statusMsg(const QString&)));
  connect(kernel->acctMgr(), SIGNAL( checkedMail(bool, bool)),
          SLOT( slotMailChecked(bool, bool)));

  setCaption( i18n("KDE Mail Client") );

  // must be the last line of the constructor:
  mStartupDone = TRUE;
}


//-----------------------------------------------------------------------------
KMMainWin::~KMMainWin()
{
  if (searchWin)
    searchWin->close();
  writeConfig();
  writeFolderConfig();

  saveMainWindowSettings(kapp->config(), "Main Window");
  kapp->config()->sync();

  delete mHeaders;
  delete mStatusBar;
  delete mFolderTree;


  delete mPanner1Sep;
  delete mPanner2Sep;
  delete mPanner3Sep;
}


//-----------------------------------------------------------------------------
void KMMainWin::readPreConfig(void)
{
  KConfig *config = kapp->config();


  { // area for config group "Geometry"
    KConfigGroupSaver saver(config, "Geometry");
    mWindowLayout = config->readNumEntry( "windowLayout", 0 );
    mShowMIME = config->readBoolEntry( "showMIME", true );
  }

  KConfigGroupSaver saver(config, "General");
  mEncodingStr = config->readEntry("encoding", "").latin1();
}


//-----------------------------------------------------------------------------
void KMMainWin::readFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());
  mFolderThreadPref = config->readBoolEntry( "threadMessagesOverride", false );
  mFolderHtmlPref = config->readBoolEntry( "htmlMailOverride", false );
}


//-----------------------------------------------------------------------------
void KMMainWin::writeFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());
  config->writeEntry( "threadMessagesOverride", mFolderThreadPref );
  config->writeEntry( "htmlMailOverride", mFolderHtmlPref );
}


//-----------------------------------------------------------------------------
void KMMainWin::readConfig(void)
{
  KConfig *config = kapp->config();


  bool oldWindowLayout = 0;
  bool oldShowMIME = true;

  QString str;
  QSize siz;

  if (mStartupDone)
  {
    writeConfig();


    oldWindowLayout = mWindowLayout;
    oldShowMIME = mShowMIME;

    readPreConfig();
    mHeaders->refreshNestedState();


    if(oldWindowLayout != mWindowLayout ||
       oldShowMIME != mShowMIME )
    {
      hide();
      // delete all panners
      delete mPanner1; // will always delete the others
      createWidgets();
    }

  }

  { // area for config group "Reader"
    KConfigGroupSaver saver(config, "Reader");
    mHtmlPref = config->readBoolEntry( "htmlMail", false );
  }

  { // area for config group "Geometry"
    KConfigGroupSaver saver(config, "Geometry");
    mThreadPref = config->readBoolEntry( "nestedMessages", false );
    QSize defaultSize(600,600);
    siz = config->readSizeEntry("MainWin", &defaultSize);
    if (!siz.isEmpty())
      resize(siz);


    // the default sizes are dependent on the actual layout
    switch( mWindowLayout ) {
    case 0:
        // the default value for the FolderPaneWidth should be about
        // 160 but the  default is set to 0 to enable the workaround
        // (see below)
        (*mPanner1Sep)[0] = config->readNumEntry( "FolderPaneWidth", 0 );
        (*mPanner1Sep)[1] = config->readNumEntry( "HeaderPaneWidth", 600-160 );
        (*mPanner2Sep)[0] = config->readNumEntry( "HeaderPaneHeight", 200 );
        (*mPanner2Sep)[1] = config->readNumEntry( "MimePaneHeight", 100 );
        (*mPanner2Sep)[2] = config->readNumEntry( "MessagePaneHeight", 300 );
        break;
    case 1:
        (*mPanner1Sep)[0] = config->readNumEntry( "FolderPaneWidth", 0 );
        (*mPanner1Sep)[1] = config->readNumEntry( "HeaderPaneWidth", 600-160 );
        (*mPanner2Sep)[0] = config->readNumEntry( "FolderPaneHeight", 400 );
        (*mPanner2Sep)[1] = config->readNumEntry( "MimePaneHeight", 200 );
        (*mPanner3Sep)[0] = config->readNumEntry( "HeaderPaneHeight", 200 );
        (*mPanner3Sep)[1] = config->readNumEntry( "MessagePaneHeight", 400 );
        break;
    case 2:
        (*mPanner1Sep)[0] = config->readNumEntry( "FolderPaneHeight", 300 );
        (*mPanner1Sep)[1] = config->readNumEntry( "MessagePaneHeight", 300 );
        (*mPanner2Sep)[0] = config->readNumEntry( "FolderPaneWidth", 160 );
        (*mPanner2Sep)[1] = config->readNumEntry( "HeaderPaneWidth", 600-160 );
        (*mPanner3Sep)[0] = config->readNumEntry( "HeaderPaneHeight", 150 );
        (*mPanner3Sep)[1] = config->readNumEntry( "MimePaneHeight", 150 );
        break;
    case 3:
        (*mPanner1Sep)[0] = config->readNumEntry( "FolderPaneHeight", 200 );
        (*mPanner1Sep)[1] = config->readNumEntry( "MimePaneHeight", 100 );
        (*mPanner1Sep)[2] = config->readNumEntry( "MessagePaneHeight", 300 );
        (*mPanner2Sep)[0] = config->readNumEntry( "FolderPaneWidth", 160 );
        (*mPanner2Sep)[1] = config->readNumEntry( "HeaderPaneWidth", 600-160 );
        break;
    }

    // workaround to support the old buggy way of saving the dimensions of the panes
    if ((*mPanner1Sep)[0] == 0) {
      defaultSize = QSize(300,130);
      siz = config->readSizeEntry("Panners", &defaultSize);
      if (siz.isEmpty())
        siz = QSize(100,100); // why not defaultSize?
      (*mPanner2Sep)[0] = siz.width();
      (*mPanner1Sep)[0] = siz.height();
      (*mPanner2Sep)[1] = height() - siz.width();
      (*mPanner1Sep)[1] = width() - siz.height();
    }

  }

  mMsgView->readConfig();
  slotSetEncoding();
  mHeaders->readConfig();
  mFolderTree->readConfig();

  { // area for config group "General"
    KConfigGroupSaver saver(config, "General");
    mSendOnCheck = config->readBoolEntry("sendOnCheck",false);
    mBeepOnNew = config->readBoolEntry("beep-on-mail", false);
    mBoxOnNew = config->readBoolEntry("msgbox-on-mail", false);
    mExecOnNew = config->readBoolEntry("exec-on-mail", false);
    mNewMailCmd = config->readEntry("exec-on-mail-cmd", "");
    mConfirmEmpty = config->readBoolEntry("confirm-before-empty", true);
  }

  // Load crypto plugins
  mCryptPlugList.loadFromConfig( config );

  // Re-activate panners
  if (mStartupDone)
  {

    if (oldWindowLayout != mWindowLayout || oldShowMIME != mShowMIME )
      activatePanners();

    //    kernel->kbp()->busy(); //Crashes KMail
    mFolderTree->reload();
    QListViewItem *qlvi = mFolderTree->indexOfFolder(mFolder);
    if (qlvi!=0) {
      mFolderTree->setCurrentItem(qlvi);
      mFolderTree->setSelected(qlvi,TRUE);
    }


    // sanders - New code
    mHeaders->setFolder(mFolder, true);
    int aIdx = mHeaders->currentItemIndex();
    if (aIdx != -1)
      mMsgView->setMsg( mFolder->getMsg(aIdx), true );
    else
      mMsgView->setMsg( 0, true );
    updateMessageActions();
    show();
    // sanders - Maybe this fixes a bug?

    /* Old code
    mMsgView->setMsg( mMsgView->msg(), TRUE );
    mHeaders->setFolder(mFolder);
    //    kernel->kbp()->idle(); //For symmetry
    show();
    */
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::writeConfig(void)
{
  QString s;
  KConfig *config = kapp->config();


  QRect r = geometry();


  mMsgView->writeConfig();
  mFolderTree->writeConfig();

  { // area for config group "Geometry"
    KConfigGroupSaver saver(config, "Geometry");

    config->writeEntry("MainWin", r.size());

    // Save the dimensions of the folder, header and message pane;
    // this is dependent on the layout style.
    switch( mWindowLayout ) {
    case 0:
        config->writeEntry( "FolderPaneWidth", mPanner1->sizes()[0] );
        config->writeEntry( "HeaderPaneWidth", mPanner1->sizes()[1] );
        config->writeEntry( "HeaderPaneHeight", mPanner2->sizes()[0] );
        config->writeEntry( "MimePaneHeight", mPanner2->sizes()[1] );
        config->writeEntry( "MessagePaneHeight", mPanner2->sizes()[2] );
        break;
    case 1:
        config->writeEntry( "FolderPaneWidth", mPanner1->sizes()[0] );
        config->writeEntry( "HeaderPaneWidth", mPanner1->sizes()[1] );
        config->writeEntry( "FolderPaneHeight", mPanner2->sizes()[0] );
        config->writeEntry( "MimePaneHeight", mPanner2->sizes()[1] );
        config->writeEntry( "HeaderPaneHeight", mPanner3->sizes()[0] );
        config->writeEntry( "MessagePaneHeight", mPanner3->sizes()[1] );
        break;
    case 2:
        config->writeEntry( "FolderPaneHeight", mPanner1->sizes()[0] );
        config->writeEntry( "MessagePaneHeight", mPanner1->sizes()[1] );
        config->writeEntry( "FolderPaneWidth", mPanner2->sizes()[0] );
        config->writeEntry( "HeaderPaneWidth", mPanner2->sizes()[1] );
        config->writeEntry( "HeaderPaneHeight", mPanner3->sizes()[0] );
        config->writeEntry( "MimePaneHeight", mPanner3->sizes()[1] );
        break;
    case 3:
        config->writeEntry( "FolderPaneHeight", mPanner1->sizes()[0] );
        config->writeEntry( "MimePaneHeight", mPanner1->sizes()[1] );
        config->writeEntry( "MessagePaneHeight", mPanner1->sizes()[2] );
        config->writeEntry( "FolderPaneWidth", mPanner2->sizes()[0] );
        config->writeEntry( "HeaderPaneWidth", mPanner2->sizes()[1] );
        break;
    }
  }


  KConfigGroupSaver saver(config, "General");
  config->writeEntry("encoding", QString(mEncodingStr));
}


//-----------------------------------------------------------------------------
void KMMainWin::createWidgets(void)
{
    QAccel *accel = new QAccel(this, "createWidgets()");

  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Geometry");

  // Create the splitters according to the layout settings
  QWidget *headerParent = 0, *folderParent = 0,
            *mimeParent = 0, *messageParent = 0;
  switch( mWindowLayout ) {
  case 0:
      mPanner1 = new QSplitter( Qt::Horizontal, this, "panner 1" );
      mPanner1->setOpaqueResize( true );
      mPanner2 = new QSplitter( Qt::Vertical, mPanner1, "panner 2" );
      mPanner2->setOpaqueResize( true );
      mPanner3 = 0;
      headerParent = mPanner2;
      folderParent = mPanner1;
      mimeParent = mPanner2;
      messageParent = mPanner2;
      break;
  case 1:
      mPanner1 = new QSplitter( Qt::Horizontal, this, "panner 1" );
      mPanner1->setOpaqueResize( true );
      mPanner2 = new QSplitter( Qt::Vertical, mPanner1, "panner 2" );
      mPanner2->setOpaqueResize( true );
      mPanner3 = new QSplitter( Qt::Vertical, mPanner1, "panner 3" );
      mPanner3->setOpaqueResize( true );
      headerParent = mPanner3;
      folderParent = mPanner2;
      mimeParent = mPanner2;
      messageParent = mPanner3;
      break;
  case 2:
      mPanner1 = new QSplitter( Qt::Vertical, this, "panner 1" );
      mPanner1->setOpaqueResize( true );
      mPanner2 = new QSplitter( Qt::Horizontal, mPanner1, "panner 2" );
      mPanner2->setOpaqueResize( true );
      mPanner3 = new QSplitter( Qt::Vertical, mPanner2, "panner 3" );
      mPanner3->setOpaqueResize( true );
      headerParent = mPanner3;
      folderParent = mPanner2;
      mimeParent = mPanner3;
      messageParent = mPanner1;
      break;
  case 3:
      mPanner1 = new QSplitter( Qt::Vertical, this, "panner 1" );
      mPanner1->setOpaqueResize( true );
      mPanner2 = new QSplitter( Qt::Horizontal, mPanner1, "panner 2" );
      mPanner2->setOpaqueResize( true );
      mPanner3 = 0;
      headerParent = mPanner2;
      folderParent = mPanner2;
      mimeParent = mPanner1;
      messageParent = mPanner1;
      break;
  }

  if( mPanner1 ) mPanner1->dumpObjectTree();
  if( mPanner2 ) mPanner2->dumpObjectTree();
  if( mPanner3 ) mPanner3->dumpObjectTree();


  // BUG -sanders these accelerators stop working after switching
  // between long/short folder layout
  // Probably need to disconnect them first.

  // create list of messages
    headerParent->dumpObjectTree();
    mHeaders = new KMHeaders(this, headerParent, "headers");
  connect(mHeaders, SIGNAL(selected(KMMessage*)),
	  this, SLOT(slotMsgSelected(KMMessage*)));
  connect(mHeaders, SIGNAL(activated(KMMessage*)),
	  this, SLOT(slotMsgActivated(KMMessage*)));
  connect( mHeaders, SIGNAL( selectionChanged() ),
           SLOT( startUpdateMessageActionsTimer() ) );
  accel->connectItem(accel->insertItem(Key_Left),
		     mHeaders, SLOT(prevMessage()));
  accel->connectItem(accel->insertItem(Key_Right),
  		     mHeaders, SLOT(nextMessage()));
  accel->connectItem(accel->insertItem(SHIFT+Key_Left),
                     mHeaders, SLOT(selectPrevMessage()));
  accel->connectItem(accel->insertItem(SHIFT+Key_Right),
                     mHeaders, SLOT(selectNextMessage()));

  if (!mEncodingStr.isEmpty())
    mCodec = KMMsgBase::codecForName(mEncodingStr);
  else mCodec = 0;


  mMsgView = new KMReaderWin(&mCryptPlugList, 0, messageParent);

  connect(mMsgView, SIGNAL(statusMsg(const QString&)),
	  this, SLOT(htmlStatusMsg(const QString&)));
  connect(mMsgView, SIGNAL(popupMenu(KMMessage&,const KURL&,const QPoint&)),
	  this, SLOT(slotMsgPopup(KMMessage&,const KURL&,const QPoint&)));
  connect(mMsgView, SIGNAL(urlClicked(const KURL&,int)),
	  this, SLOT(slotUrlClicked(const KURL&,int)));
  connect(mMsgView, SIGNAL(showAtmMsg(KMMessage *)),
	  this, SLOT(slotAtmMsg(KMMessage *)));
  connect(mHeaders, SIGNAL(maybeDeleting()),
	  mMsgView, SLOT(clearCache()));
  connect(mMsgView, SIGNAL(noDrag()),
          mHeaders, SLOT(slotNoDrag()));
  accel->connectItem(accel->insertItem(Key_Up),
		     mMsgView, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Key_Down),
		     mMsgView, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Key_Prior),
		     mMsgView, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Key_Next),
		     mMsgView, SLOT(slotScrollNext()));

  new KAction( i18n("Move Message to Folder"), Key_M, this,
               SLOT(slotMoveMsg()), actionCollection(),
               "move_message_to_folder" );
  new KAction( i18n("Copy Message to Folder"), Key_C, this,
               SLOT(slotCopyMsg()), actionCollection(),
               "copy_message_to_folder" );
  new KAction( i18n("Delete Message"), Key_Delete, this,
               SLOT(slotDeleteMsg()), actionCollection(),
               "delete_message" );

  // create list of folders
  mFolderTree  = new KMFolderTree(&mCryptPlugList,
                                  folderParent, "folderTree");

  connect(mFolderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderSelectedUnread(KMFolder*)),
	  this, SLOT(folderSelectedUnread(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderDrop(KMFolder*)),
	  this, SLOT(slotMoveMsgToFolder(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderDropCopy(KMFolder*)),
          this, SLOT(slotCopyMsgToFolder(KMFolder*)));

  // create a mime part tree and store it's pointer in the reader win
  mMimePartTree = new KMMimePartTree( mMsgView, mimeParent, "mMimePartTree" );
  mMsgView->setMimePartTree( mMimePartTree );
  if( mShowMIME )
      mMimePartTree->show();
  else
      mMimePartTree->hide();

  //Commands not worthy of menu items, but that deserve configurable keybindings
  new KAction(
    i18n("Next folder with unread messages"), CTRL+Key_Plus, mFolderTree,
    SLOT(nextUnreadFolder()), actionCollection(), "next_unread_folder");

  new KAction(
   i18n("Previous folder with unread messages"), CTRL+Key_Minus, mFolderTree,
   SLOT(prevUnreadFolder()), actionCollection(), "prev_unread_folder");

  new KAction(
   i18n("Focus on next folder"), CTRL+Key_Right, mFolderTree,
   SLOT(incCurrentFolder()), actionCollection(), "inc_current_folder");

  new KAction(
   i18n("Focus on previous folder"), CTRL+Key_Left, mFolderTree,
   SLOT(decCurrentFolder()), actionCollection(), "dec_current_folder");

  new KAction(
   i18n("Select folder with focus"), CTRL+Key_Space, mFolderTree,
   SLOT(selectCurrentFolder()), actionCollection(), "select_current_folder");

  new KAction( i18n("Expand Thread"), Key_Period, this,
		   SLOT(slotExpandThread()),
		   actionCollection(), "expand_thread" );

  new KAction( i18n("Collapse Thread"), Key_Comma, this,
		   SLOT(slotCollapseThread()),
		   actionCollection(), "collapse_thread" );

  new KAction( i18n("Expand All Threads"), CTRL+Key_Period, this,
		   SLOT(slotExpandAllThreads()),
		   actionCollection(), "expand_all_threads" );

  new KAction( i18n("Collapse All Threads"), CTRL+Key_Comma, this,
		   SLOT(slotCollapseAllThreads()),
		   actionCollection(), "collapse_all_threads" );

  new KAction( i18n( "Move to the Next Unread Text" ),
               Key_Space, this,  SLOT( slotReadOn() ),
               actionCollection(), "read_on" );

  connect( kernel->outboxFolder(), SIGNAL( msgRemoved(int, QString) ),
           SLOT( startUpdateMessageActionsTimer() ) );
  connect( kernel->outboxFolder(), SIGNAL( msgAdded(int) ),
           SLOT( startUpdateMessageActionsTimer() ) );
}



void KMMainWin::activatePanners(void)
{
  // glue everything together
    switch( mWindowLayout ) {
    case 0:
        mHeaders->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mMimePartTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mMsgView->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mFolderTree->reparent( mPanner1, 0, QPoint( 0, 0 ) );
        mPanner1->moveToLast( mPanner2 );
        mPanner1->setSizes( *mPanner1Sep );
        mPanner1->setResizeMode( mFolderTree, QSplitter::KeepSize );
        mPanner2->setSizes( *mPanner2Sep );
        mPanner2->setResizeMode( mHeaders, QSplitter::KeepSize );
        mPanner2->setResizeMode( mMimePartTree, QSplitter::KeepSize );
        break;
    case 1:
        mHeaders->reparent( mPanner3, 0, QPoint( 0, 0 ) );
        mMsgView->reparent( mPanner3, 0, QPoint( 0, 0 ) );
        mPanner3->moveToLast( mMsgView );
        mFolderTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mMimePartTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mPanner2->moveToLast( mMimePartTree );
        mPanner1->setSizes( *mPanner1Sep );
        mPanner2->setSizes( *mPanner2Sep );
        mPanner3->setSizes( *mPanner2Sep );
        mPanner2->setResizeMode( mMimePartTree, QSplitter::KeepSize );
        mPanner3->setResizeMode( mHeaders, QSplitter::KeepSize );
        break;
    case 2:
        mFolderTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mPanner2->moveToFirst( mFolderTree );
        mHeaders->reparent( mPanner3, 0, QPoint( 0, 0 ) );
        mMimePartTree->reparent( mPanner3, 0, QPoint( 0, 0 ) );
        mPanner3->moveToLast( mMimePartTree );
        mMsgView->reparent( mPanner1, 0, QPoint( 0, 0 ) );
        mPanner1->moveToLast( mMsgView );
        mPanner1->setSizes( *mPanner1Sep );
        mPanner2->setSizes( *mPanner2Sep );
        mPanner3->setSizes( *mPanner2Sep );
        mPanner1->setResizeMode( mPanner2, QSplitter::KeepSize );
        mPanner2->setResizeMode( mFolderTree, QSplitter::KeepSize );
        mPanner3->setResizeMode( mMimePartTree, QSplitter::KeepSize );
        break;
    case 3:
        mFolderTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mHeaders->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mPanner2->moveToLast( mHeaders );
        mMimePartTree->reparent( mPanner1, 0, QPoint( 0, 0 ) );
        mPanner1->moveToFirst( mPanner2 );
        mMsgView->reparent( mPanner1, 0, QPoint( 0, 0 ) );
        mPanner1->moveToLast( mMsgView );
        mPanner1->setSizes( *mPanner1Sep );
        mPanner2->setSizes( *mPanner2Sep );
        mPanner1->setResizeMode( mPanner2, QSplitter::KeepSize );
        mPanner1->setResizeMode( mMimePartTree, QSplitter::KeepSize );
        mPanner2->setResizeMode( mFolderTree, QSplitter::KeepSize );
        break;
    }

    if( mShowMIME )
        mMimePartTree->show();
    else
        mMimePartTree->hide();

    setCentralWidget( mPanner1 );
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSetEncoding()
{
    mEncodingStr = KGlobal::charsets()->encodingForName(mEncoding->currentText()).latin1();
    if (mEncoding->currentItem() == 0) // Auto
    {
      mCodec = 0;
      mEncodingStr = "";
    }
    else
      mCodec = KMMsgBase::codecForName( mEncodingStr );
    mMsgView->setCodec(mCodec);
    return;
}

//-----------------------------------------------------------------------------
void KMMainWin::htmlStatusMsg(const QString &aText)
{
  if (aText.isEmpty()) displayStatusMsg(mLastStatusMsg);
  else displayStatusMsg(aText);
}

//-----------------------------------------------------------------------------
void KMMainWin::statusMsg(const QString& aText)
{
  mLastStatusMsg = aText;
  displayStatusMsg(aText);
}

//-----------------------------------------------------------------------------
void KMMainWin::displayStatusMsg(const QString& aText)
{
  QString text = " " + aText + " ";
  int statusWidth = mStatusBar->width() - littleProgress->width()
    - fontMetrics().maxWidth();

  while (!text.isEmpty() && fontMetrics().width( text ) >= statusWidth)
    text.truncate( text.length() - 1);

  // ### FIXME: We should disable richtext/HTML (to avoid possible denial of service attacks),
  // but this code would double the size of the satus bar if the user hovers
  // over an <foo@bar.com>-style email address :-(
//  text.replace(QRegExp("&"), "&amp;");
//  text.replace(QRegExp("<"), "&lt;");
//  text.replace(QRegExp(">"), "&gt;");

  mStatusBar->changeItem(text, mMessageStatusId);
}


//-----------------------------------------------------------------------------
void KMMainWin::hide()
{
  KMMainWinInherited::hide();
}


//-----------------------------------------------------------------------------
void KMMainWin::show()
{
  if( mPanner1 ) mPanner1->setSizes( *mPanner1Sep );
  if( mPanner2 ) mPanner2->setSizes( *mPanner2Sep );
  if( mPanner3 ) mPanner3->setSizes( *mPanner3Sep );
  KMMainWinInherited::show();
}


//-------------------------------------------------------------------------
void KMMainWin::slotSearch()
{
  if(!searchWin)
  {
    searchWin = new KMFldSearch(this, "Search", mFolder, false);
    connect(searchWin, SIGNAL(destroyed()),
	    this, SLOT(slotSearchClosed()));
  }
  else
  {
    searchWin->activateFolder(mFolder);
  }

  searchWin->show();
  KWin::setActiveWindow(searchWin->winId());
}


//-------------------------------------------------------------------------
void KMMainWin::slotSearchClosed()
{
  searchWin = 0;
}


//-------------------------------------------------------------------------
void KMMainWin::slotFind()
{
  if( mMsgView )
    mMsgView->slotFind();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotHelp()
{
  kapp->invokeHelp();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotNewMailReader()
{
  KMMainWin *d;

  d = new KMMainWin(NULL);
  d->show();
  d->resize(d->size());
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSettings()
{
  if( mConfigureDialog == 0 )
  {
      mConfigureDialog = new ConfigureDialog( &mCryptPlugList,
                                              this, "configure", false );
  }
  mConfigureDialog->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotFilter()
{
  kernel->filterMgr()->openDialog( this );
}


//-----------------------------------------------------------------------------
void KMMainWin::slotPopFilter()
{
  kernel->popFilterMgr()->openDialog( this );
}


//-----------------------------------------------------------------------------
void KMMainWin::slotAddrBook()
{
  KMAddrBookExternal::launch(this);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotImport()
{
  KRun::runCommand("kmailcvt");
}


//-----------------------------------------------------------------------------
void KMMainWin::slotAddFolder()
{
  KMFolderDialog *d;

  d = new KMFolderDialog(0, &(kernel->folderMgr()->dir()),
			 this, i18n("Create Folder"));
  if (d->exec()) {
    mFolderTree->reload();
    QListViewItem *qlvi = mFolderTree->indexOfFolder( mFolder );
    if (qlvi) {
      qlvi->setOpen(TRUE);
      mFolderTree->setCurrentItem( qlvi );
    }
    if ( mFolder->needsRepainting() )
      mFolderTree->delayedUpdate();
  }
  delete d;
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCheckMail()
{
 if(kernel->checkingMail())
 {
    KMessageBox::information(this,
		     i18n("Your mail is already being checked."));
    return;
  }

 kernel->setCheckingMail(true);

 kernel->acctMgr()->checkMail(true);

 kernel->setCheckingMail(false);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCheckOneAccount(int item)
{
  if(kernel->checkingMail())
  {
    KMessageBox::information(this,
		     i18n("Your mail is already being checked."));
    return;
  }

  kernel->setCheckingMail(true);

  //  kbp->busy();
  kernel->acctMgr()->intCheckMail(item);
  // kbp->idle();

  kernel->setCheckingMail(false);
}

void KMMainWin::slotMailChecked(bool newMail, bool sendOnCheck)
{
  if(mSendOnCheck && sendOnCheck)
    slotSendQueued();

  if (!newMail)
    return;

  if (mBeepOnNew) {
    KNotifyClient::beep();
  }

  if (mExecOnNew) {
    if (!mNewMailCmd.isEmpty()) {
      KShellProcess p;
      p << mNewMailCmd;
      p.start(KProcess::DontCare);
    }
  }

  if (mBoxOnNew && !mbNewMBVisible) {
    mbNewMBVisible = true;
    KMessageBox::information(this, QString(i18n("You have new mail!")),
                                   QString(i18n("New Mail")));
    mbNewMBVisible = false;
  }

  // Todo:
  // scroll mHeaders to show new items if current item would
  // still be visible
  //  mHeaders->showNewMail();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCompose()
{
  KMComposeWin *win;
  KMMessage* msg = new KMMessage;

  if ( mFolder ) {
      msg->initHeader( mFolder->identity() );

      win = new KMComposeWin(&mCryptPlugList, msg, mFolder->identity());
  } else {
      msg->initHeader();
      win = new KMComposeWin(&mCryptPlugList, msg);
  }

  win->show();

}


//-----------------------------------------------------------------------------
void KMMainWin::slotPostToML()
{
  KMComposeWin *win;
  KMMessage* msg = new KMMessage;

  if ( mFolder ) {
      msg->initHeader( mFolder->identity() );

      if (mFolder->isMailingList()) {
          kdDebug(5006)<<QString("mFolder->isMailingList() %1").arg( mFolder->mailingListPostAddress().latin1())<<endl;;

          msg->setTo(mFolder->mailingListPostAddress());
      }
      win = new KMComposeWin(&mCryptPlugList, msg, mFolder->identity());
  } else {
      msg->initHeader();
      win = new KMComposeWin(&mCryptPlugList, msg);
  }

  win->show();

}


//-----------------------------------------------------------------------------
void KMMainWin::slotModifyFolder()
{
  KMFolderDialog *d;

  if (!mFolder) return;
  d = new KMFolderDialog((KMFolder*)mFolder, mFolder->parent(),
			 this, i18n("Properties of Folder %1").arg( mFolder->label() ) );
  if (d->exec() &&
      ((mFolder->protocol() != "imap") || mFolder->needsRepainting() ) ) {
    mFolderTree->reload();
    QListViewItem *qlvi = mFolderTree->indexOfFolder( mFolder );
    if (qlvi) {
      qlvi->setOpen(TRUE);
      mFolderTree->setCurrentItem( qlvi );
      mHeaders->msgChanged();
    }
    if ( mFolder->needsRepainting() )
      mFolderTree->delayedUpdate();
  }
  delete d;
}

//-----------------------------------------------------------------------------
void KMMainWin::slotExpireFolder()
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
  KConfig           *config = kapp->config();
  KConfigGroupSaver saver(config, "General");

  if (config->readBoolEntry("warn-before-expire")) {
    str = i18n("Are you sure you want to expire this folder \"%1\"?").arg(mFolder->label());
    if (KMessageBox::warningContinueCancel(this, str, i18n("Expire Folder"),
					   i18n("&Expire"))
	!= KMessageBox::Continue) return;
  }

  mFolder->expireOldMessages();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotEmptyFolder()
{
  QString str;
  KMMessage* msg;

  if (!mFolder) return;

  if (mConfirmEmpty)
  {
    str = i18n("Are you sure you want to empty the folder \"%1\"?").arg(mFolder->label());

    if (KMessageBox::warningContinueCancel(this, str,
                                          i18n("Empty Folder"), i18n("&Empty") )
       !=KMessageBox::Continue) return;
  }

  if (mFolder->protocol() == "imap")
  {
    slotMarkAll();
    slotDeleteMsg();
    return;
  }

  mMsgView->clearCache();

  kernel->kbp()->busy();

  // begin of critical part
  // from here to "end..." no signal may change to another mFolder, otherwise
  // the wrong folder will be truncated in expunge (dnaber, 1999-08-29)
  mFolder->open();
  mHeaders->setFolder(NULL);
  mMsgView->clear();

  if (mFolder != kernel->trashFolder())
  {
    // FIXME: If we run out of disk space mail may be lost rather
    // than moved into the trash -sanders
    while ((msg = mFolder->take(0)) != NULL) {
      kernel->trashFolder()->addMsg(msg);
      kernel->trashFolder()->unGetMsg(kernel->trashFolder()->count()-1);
    }
  }

  mFolder->close();
  mFolder->expunge();
  // end of critical
  if (mFolder != kernel->trashFolder())
    statusMsg(i18n("Moved all messages into trash"));

  mHeaders->setFolder(mFolder);
  kernel->kbp()->idle();
  updateMessageActions();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotRemoveFolder()
{
  QString str;
  QDir dir;

  if (!mFolder) return;
  if (mFolder->isSystemFolder()) return;

  str = i18n("Are you sure you want to remove the folder "
	     "\"%1\" and all subfolders, discarding their contents?")
			     .arg(mFolder->label());

  if (KMessageBox::warningYesNo(this, str, i18n("Remove Folder"),
				i18n("&Remove"), KStdGuiItem::cancel() )
      == KMessageBox::Yes)
  {
    if (mFolder->hasAccounts())
    {
      // this folder has an account, so we need to change that to the inbox
      KMAccount* acct = NULL;
      KMAcctFolder* acctFolder = static_cast<KMAcctFolder*>(mFolder);
      for ( acct = acctFolder->account(); acct; acct = acctFolder->nextAccount() )
      {
        acct->setFolder(kernel->inboxFolder());
        KMessageBox::information(this,
            i18n("The destination folder of the account '%1' was restored to the inbox.").arg(acct->name()));
      }
    }
    if (mFolder->protocol() == "imap")
      static_cast<KMFolderImap*>(mFolder)->removeOnServer();
    else
      kernel->folderMgr()->remove(mFolder);
  }
}

//-----------------------------------------------------------------------------
void KMMainWin::slotMarkAllAsRead()
{
  if (!mFolder)
    return;
  mFolder->markUnreadAsRead();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotCompactFolder()
{
  int idx = mHeaders->currentItemIndex();
  if (mFolder)
  {
    if (mFolder->protocol() == "imap")
    {
      KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder);
      imap->expungeFolder(imap, FALSE);
    }
    else
    {
      kernel->kbp()->busy();
      mFolder->compact();
      kernel->kbp()->idle();
    }
  }
  mHeaders->setCurrentItemByIndex(idx);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotExpireAll() {
  KConfig    *config = kapp->config();
  int        ret = 0;

  KConfigGroupSaver saver(config, "General");

  if (config->readBoolEntry("warn-before-expire")) {
    ret = KMessageBox::warningContinueCancel(KMainWindow::memberList->first(),
			 i18n("Are you sure you want to expire all old messages?"),
			 i18n("Expire old Messages?"), i18n("Expire"));
    if (ret != KMessageBox::Continue) {
      return;
    }
  }

  kernel->folderMgr()->expireAllFolders();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotCompactAll()
{
  kernel->kbp()->busy();
  QStringList strList;
  QValueList<QGuardedPtr<KMFolder> > folders;
  KMFolder *folder;
  mFolderTree->createFolderList(&strList, &folders);
  for (int i = 0; folders.at(i) != folders.end(); i++)
  {
    folder = *folders.at(i);
    if (!folder || folder->isDir()) continue;
    if (folder->protocol() == "imap")
    {
      KMFolderImap *imap = static_cast<KMFolderImap*>(folder);
      imap->expungeFolder(imap, TRUE);
    }
    else
      folder->compact();
  }
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotOverrideHtml()
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
      return;
    }
  }
  mFolderHtmlPref = !mFolderHtmlPref;
  mMsgView->setHtmlOverride(mFolderHtmlPref);
  mMsgView->setMsg( mMsgView->msg(), TRUE );
}

//-----------------------------------------------------------------------------
void KMMainWin::slotOverrideThread()
{
  mFolderThreadPref = !mFolderThreadPref;
  mHeaders->setNestedOverride(mFolderThreadPref);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotPrintMsg()
{
  if(mHeaders->currentItemIndex() >= 0)
    mMsgView->printMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotReplyToMsg()
{
  mHeaders->replyToMsg(mMsgView->copyText());
}

//-----------------------------------------------------------------------------
void KMMainWin::slotNoQuoteReplyToMsg()
{
  mHeaders->noQuoteReplyToMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotReplyAllToMsg()
{
  mHeaders->replyAllToMsg(mMsgView->copyText());
}


//-----------------------------------------------------------------------------
void KMMainWin::slotReplyListToMsg()
{
  mHeaders->replyListToMsg(mMsgView->copyText());
}


//-----------------------------------------------------------------------------
void KMMainWin::slotForwardMsg()
{
  mHeaders->forwardMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotForwardAttachedMsg()
{
  mHeaders->forwardAttachedMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotRedirectMsg()
{
  mHeaders->redirectMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotBounceMsg()
{
  mHeaders->bounceMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMessageQueuedOrDrafted()
{
  if (!kernel->folderIsDraftOrOutbox(mFolder))
      return;
  mMsgView->update(true);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotEditMsg()
{
  KMMessage *msg;
  int aIdx;

  if((aIdx = mHeaders->currentItemIndex()) <= -1)
    return;
  if(!(msg = mHeaders->getMsg(aIdx)))
    return;

  slotEditMsg(msg);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotEditMsg(KMMessage* msg)
{
  if (!kernel->folderIsDraftOrOutbox(mFolder))
    return;

  if ( !msg->isComplete() && mFolder->protocol() == "imap" )
  {
    // transfer the message first
    kdDebug(5006) << "slotEditMsg: transfer message" << endl;
    if (msg->transferInProgress()) return;
    msg->setTransferInProgress(TRUE);
    KMImapJob *job = new KMImapJob(msg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(slotEditMsg(KMMessage*)));
    return;
  }

  mFolder->removeMsg(msg);
  mHeaders->setSelected(mHeaders->currentItem(), TRUE);
  mHeaders->highlightMessage(mHeaders->currentItem(), true);

  KMComposeWin *win = new KMComposeWin(&mCryptPlugList);
  QObject::connect( win, SIGNAL( messageQueuedOrDrafted()),
		    this, SLOT( slotMessageQueuedOrDrafted()) );
  win->setMsg(msg,FALSE, TRUE);
  win->setFolder(mFolder);
  win->show();
}



//-----------------------------------------------------------------------------
void KMMainWin::slotResendMsg()
{
  mHeaders->resendMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotDeleteMsg()
{
  mHeaders->deleteMsg();
  updateMessageActions();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotUndo()
{
    mHeaders->undo();
    updateMessageActions();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotShowMsgSrc()
{
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg)
  {
    QTextCodec *codec = mCodec;
    if (!codec) //this is Auto setting
    {
       QCString cset = msg->charset();
       if (!cset.isEmpty())
         codec = KMMsgBase::codecForName(cset);
    }
    msg->viewSource(i18n("Message as Plain Text"), codec,
		    mMsgView->isfixedFont());
  }
}

//-----------------------------------------------------------------------------
void KMMainWin::slotToggleFixedFont()
{
  mMsgView->slotToggleFixedFont();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMoveMsg()
{
  KMFolderSelDlg dlg(this,i18n("Move Message to Folder"));
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  mHeaders->moveMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotMoveMsgToFolder( KMFolder *dest)
{
  mHeaders->moveMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotCopyMsgToFolder( KMFolder *dest)
{
  mHeaders->copyMsgToFolder(dest);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotApplyFilters()
{
  mHeaders->applyFiltersOnMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCopyMsg()
{
  KMFolderSelDlg dlg(this,i18n("Copy Message to Folder"));
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  mHeaders->copyMsgToFolder(dest);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSaveMsg()
{
  if(mHeaders->currentItemIndex() == -1)
    return;
  mHeaders->saveMsg(-1);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSendQueued()
{
  kernel->msgSender()->sendQueued();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotViewChange()
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


void KMMainWin::slotBriefHeaders() {
  mMsgView->setHeaderStyle( KMReaderWin::HdrBrief );
}

void KMMainWin::slotFancyHeaders() {
  mMsgView->setHeaderStyle( KMReaderWin::HdrFancy );
}

void KMMainWin::slotStandardHeaders() {
  mMsgView->setHeaderStyle( KMReaderWin::HdrStandard );
}

void KMMainWin::slotLongHeaders() {
  mMsgView->setHeaderStyle( KMReaderWin::HdrLong );
}

void KMMainWin::slotAllHeaders() {
  mMsgView->setHeaderStyle( KMReaderWin::HdrAll );
}

void KMMainWin::slotCycleHeaderStyles() {
  KMReaderWin::HeaderStyle style = mMsgView->headerStyle();
  if ( style == KMReaderWin::HdrAll ) // last, go to top again:
    mMsgView->setHeaderStyle( KMReaderWin::HdrFancy );
  else
    mMsgView->setHeaderStyle( KMReaderWin::HeaderStyle(++int(style)) );
  KRadioAction * action = actionForHeaderStyle( mMsgView->headerStyle() );
  assert( action );
  action->setChecked( true );
}


void KMMainWin::slotIconicAttachments() {
  mMsgView->setAttachmentStyle( KMReaderWin::IconicAttmnt );
}

void KMMainWin::slotSmartAttachments() {
  mMsgView->setAttachmentStyle( KMReaderWin::SmartAttmnt );
}

void KMMainWin::slotInlineAttachments() {
  mMsgView->setAttachmentStyle( KMReaderWin::InlineAttmnt );
}

void KMMainWin::slotCycleAttachmentStyles() {
  KMReaderWin::AttachmentStyle style = mMsgView->attachmentStyle();
  if ( style == KMReaderWin::InlineAttmnt ) // last, go to top again:
    mMsgView->setAttachmentStyle( KMReaderWin::IconicAttmnt );
  else
    mMsgView->setAttachmentStyle( KMReaderWin::AttachmentStyle(++int(style)) );
  KRadioAction * action = actionForAttachmentStyle( mMsgView->attachmentStyle() );
  assert( action );
  action->setChecked( true );
}

void KMMainWin::folderSelected(KMFolder* aFolder)
{
    folderSelected( aFolder, false );
}

void KMMainWin::folderSelectedUnread(KMFolder* aFolder)
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
}

//-----------------------------------------------------------------------------
void KMMainWin::folderSelected(KMFolder* aFolder, bool jumpToUnread)
{
  if( aFolder && mFolder == aFolder )
    return;

  kernel->kbp()->busy();

  if( !aFolder && mFolderTree->currentItem() == mFolderTree->firstChild() ) {
    mMsgView->setMsg( 0, TRUE );
    if( mHeaders )
      mHeaders->hide();
    if( mShowMIME && mMimePartTree )
        mMimePartTree->hide();
    mMsgView->displayAboutPage();
  } else if( !mFolder ) {
    mMsgView->enableMsgDisplay();
    mMsgView->setMsg( 0, TRUE );
    if( mHeaders )
      mHeaders->show();
    if( mShowMIME && mMimePartTree )
      mMimePartTree->show();
  } else
    mMsgView->setMsg( 0, FALSE );

  if (mFolder && mFolder->needsCompacting() && (mFolder->protocol() == "imap"))
  {
    KMFolderImap *imap = static_cast<KMFolderImap*>(mFolder);
    if (imap->autoExpunge())
      imap->expungeFolder(imap, TRUE);
  }
  writeFolderConfig();
  mFolder = (KMFolder*)aFolder;
  readFolderConfig();
  mMsgView->setHtmlOverride(mFolderHtmlPref);
  mHeaders->setFolder( mFolder, jumpToUnread );
  updateMessageActions();
  kernel->kbp()->idle();
}

//-----------------------------------------------------------------------------
KMMessage *KMMainWin::jumpToMessage(KMMessage *aMsg)
{
  KMFolder *folder;
  int index;

  kernel->msgDict()->getLocation(aMsg, &folder, &index);
  if (!folder)
    return 0;

  // setting current folder only if we actually have to
  if (folder != mFolder) {
    folderSelected(folder, false);
    slotSelectFolder(folder);
  }

  KMMsgBase *msgBase = folder->getMsg(index);
  KMMessage *msg = static_cast<KMMessage *>(msgBase);

  // setting current message only if we actually have to
  unsigned long curMsgSerNum = 0;
  if (mHeaders->currentMsg())
    curMsgSerNum = mHeaders->currentMsg()->getMsgSerNum();
  if (curMsgSerNum != msg->getMsgSerNum())
    mHeaders->setCurrentMsg(index);

  return msg;
}

//-----------------------------------------------------------------------------
void KMMainWin::slotMsgSelected(KMMessage *msg)
{
  if (msg && msg->parent() && (msg->parent()->protocol() == "imap"))
  {
    mMsgView->setMsg(NULL);
    KMImapJob *job = new KMImapJob(msg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(slotUpdateImapMessage(KMMessage*)));
  } else {
    mMsgView->setMsg(msg);
  }
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSelectFolder(KMFolder* folder)
{
  QListViewItem* item = mFolderTree->indexOfFolder(folder);
  if (item)
    mFolderTree->setCurrentItem( item );
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSelectMessage(KMMessage* msg)
{
  int idx = mFolder->find(msg);
  if (idx != -1) {
    mHeaders->setCurrentMsg(idx);
    mMsgView->setMsg(msg);
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUpdateImapMessage(KMMessage *msg)
{
  if (((KMMsgBase*)msg)->isMessage()) mMsgView->setMsg(msg, TRUE);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSubjectFilter()
{
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg)
    kernel->filterMgr()->createFilter( "Subject", msg->subject());
}

void KMMainWin::slotMailingListFilter()
{
    KMMessage* msg = mHeaders->getMsg(-1);
    if (msg) {
        QCString name;
        QString value;
        if ( !KMMLInfo::name( msg, name, value ).isNull() )
            kernel->filterMgr()->createFilter( name, value );
    }
}

//-----------------------------------------------------------------------------
void KMMainWin::slotFromFilter()
{
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg)
    kernel->filterMgr()->createFilter( "From", msg->from());
}

//-----------------------------------------------------------------------------
void KMMainWin::slotToFilter()
{
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg)
    kernel->filterMgr()->createFilter( "To", msg->to());
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatusNew()
{
  mHeaders->setMsgStatus(KMMsgStatusNew);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatusUnread()
{
  mHeaders->setMsgStatus(KMMsgStatusUnread);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatusFlag()
{
  mHeaders->setMsgStatus(KMMsgStatusFlag);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatusRead()
{
  mHeaders->setMsgStatus(KMMsgStatusRead);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatusReplied()
{
  mHeaders->setMsgStatus(KMMsgStatusReplied);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatusQueued()
{
  mHeaders->setMsgStatus(KMMsgStatusQueued);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatusSent()
{
  mHeaders->setMsgStatus(KMMsgStatusSent);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetThreadStatusNew()
{
  mHeaders->setThreadStatus(KMMsgStatusNew);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetThreadStatusUnread()
{
  mHeaders->setThreadStatus(KMMsgStatusUnread);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetThreadStatusFlag()
{
  mHeaders->setThreadStatus(KMMsgStatusFlag);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetThreadStatusRead()
{
  mHeaders->setThreadStatus(KMMsgStatusRead);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetThreadStatusReplied()
{
  mHeaders->setThreadStatus(KMMsgStatusReplied);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetThreadStatusQueued()
{
  mHeaders->setThreadStatus(KMMsgStatusQueued);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetThreadStatusSent()
{
  mHeaders->setThreadStatus(KMMsgStatusSent);
}



//-----------------------------------------------------------------------------
void KMMainWin::slotNextMessage()       { mHeaders->nextMessage(); }
void KMMainWin::slotNextUnreadMessage() { mHeaders->nextUnreadMessage(); }
void KMMainWin::slotPrevMessage()       { mHeaders->prevMessage(); }
void KMMainWin::slotPrevUnreadMessage() { mHeaders->prevUnreadMessage(); }

//-----------------------------------------------------------------------------
//called from headers. Message must not be deleted on close
void KMMainWin::slotMsgActivated(KMMessage *msg)
{
  if (!msg->isComplete() && mFolder->protocol() == "imap")
  {
    KMImapJob *job = new KMImapJob(msg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(slotMsgActivated(KMMessage*)));
    return;
  }

  if (kernel->folderIsDraftOrOutbox(mFolder))
  {
    slotEditMsg();
		return;
  }

  assert(msg != NULL);
  KMReaderWin *win;

  win = new KMReaderWin;
  win->setAutoDelete(true);
  win->setHtmlOverride(mFolderHtmlPref);
  KMMessage *newMessage = new KMMessage();
  newMessage->fromString(msg->asString());
  newMessage->setStatus(msg->status());
  // the following line makes sure the message has a unique serial number
  // and that it is in the message dictionary
  newMessage->setMsgSerNum(msg->getMsgSerNum());
  showMsg(win, newMessage);
}


//called from reader win. message must be deleted on close
void KMMainWin::slotAtmMsg(KMMessage *msg)
{
  KMReaderWin *win;
  assert(msg != NULL);
  win = new KMReaderWin;
  win->setAutoDelete(true); //delete on end
  showMsg(win, msg);
}


void KMMainWin::showMsg(KMReaderWin *win, KMMessage *msg)
{
  KWin::setIcons(win->winId(), kapp->icon(), kapp->miniIcon());
  win->setCodec(mCodec);
  win->setMsg(msg, true); // hack to work around strange QTimer bug
  win->resize(550,600);

  connect(win, SIGNAL(statusMsg(const QString&)),
          this, SLOT(statusMsg(const QString&)));
  connect(win, SIGNAL(popupMenu(KMMessage&,const KURL&,const QPoint&)),
          this, SLOT(slotMsgPopup(KMMessage&,const KURL&,const QPoint&)));
  connect(win, SIGNAL(urlClicked(const KURL&,int)),
          this, SLOT(slotUrlClicked(const KURL&,int)));
  connect(win, SIGNAL(showAtmMsg(KMMessage *)),
	  this, SLOT(slotAtmMsg(KMMessage *)));

  QAccel *accel = new QAccel(win, "showMsg()");
  accel->connectItem(accel->insertItem(Key_Up),
                     win, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Key_Down),
                     win, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Key_Prior),
                     win, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Key_Next),
                     win, SLOT(slotScrollNext()));
  //accel->connectItem(accel->insertItem(Key_S),
  //                   win, SLOT(slotCopyMsg()));
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCopyText()
{
  QString temp;
  temp = mMsgView->copyText();
  kapp->clipboard()->setText(temp);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotMarkAll()
{
  for (QListViewItemIterator it(mHeaders); it.current(); it++)
    mHeaders->setSelected( it.current(), TRUE );
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSelectText() {
  mMsgView->selectAll();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotUrlClicked(const KURL &aUrl, int)
{
  KMComposeWin *win;
  KMMessage* msg;

  if (aUrl.protocol() == "mailto")
  {
    QString id = "";
    if ( mFolder )
      id = mFolder->identity();

    msg = new KMMessage;
    msg->initHeader(id);
    msg->setCharset("utf-8");
    msg->setTo(aUrl.path());
    QString query=aUrl.query();
    while (!query.isEmpty()) {
      QString queryPart;
      int secondQuery = query.find('?',1);
      if (secondQuery != -1)
	queryPart = query.left(secondQuery);
      else
	queryPart = query;
      query = query.mid(queryPart.length());

      if (queryPart.left(9) == "?subject=")
	msg->setSubject( KURL::decode_string(queryPart.mid(9)) );
      else if (queryPart.left(6) == "?body=")
	// It is correct to convert to latin1() as URL should not contain
	// anything except ascii.
	msg->setBody( KURL::decode_string(queryPart.mid(6)).latin1() );
      else if (queryPart.left(4) == "?cc=")
	msg->setCc( KURL::decode_string(queryPart.mid(4)) );
    }

    win = new KMComposeWin(&mCryptPlugList, msg, id);
    win->setCharset("", TRUE);
    win->show();
  }
  else if ((aUrl.protocol() == "http") || (aUrl.protocol() == "https") ||
	   (aUrl.protocol() ==  "ftp") || (aUrl.protocol() == "file") ||
           (aUrl.protocol() == "help"))
  {
    statusMsg(i18n("Opening URL..."));
    KMimeType::Ptr mime = KMimeType::findByURL( aUrl );
    if (mime->name() == "application/x-desktop" ||
        mime->name() == "application/x-executable" ||
        mime->name() == "application/x-shellscript" )
    {
      if (KMessageBox::warningYesNo( 0, i18n( "Do you really want to execute"
        " '%1' ? " ).arg( aUrl.prettyURL() ) ) != KMessageBox::Yes) return;
    }
    (void) new KRun( aUrl );
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoCompose()
{
  KMComposeWin *win;
  KMMessage *msg = new KMMessage;
  QString id = "";

  if ( mMsgCurrent )
    id = mMsgCurrent->parent()->identity();
  else
    if ( mFolder )
      id = mFolder->identity();
  msg->initHeader(id);
  msg->setCharset("utf-8");
  msg->setTo(mUrlCurrent.path());

  win = new KMComposeWin(&mCryptPlugList, msg, id);
  win->setCharset("", TRUE);
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoReply()
{
  KMComposeWin *win;
  KMMessage *msg, *rmsg;
  QString id;

  /* if (!(msg = mHeaders->getMsg(-1))) return; */
  msg = mMsgCurrent;
  rmsg = msg->createReply(FALSE, FALSE, mMsgView->copyText());
  rmsg->setTo(mUrlCurrent.path());

  win = new KMComposeWin(&mCryptPlugList, rmsg, id);
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->setReplyFocus();
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoForward()
{
  KMComposeWin *win;
  KMMessage *msg, *fmsg;

  /* if (!(msg = mHeaders->getMsg(-1))) return; */
  msg = mMsgCurrent;
  fmsg = msg->createForward();
  fmsg->setTo(mUrlCurrent.path());

  win = new KMComposeWin(&mCryptPlugList, fmsg);
  win->setCharset(msg->codec()->mimeName(), TRUE);
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoAddAddrBook()
{
  KMAddrBookExternal::addEmail(mUrlCurrent.path(), this);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlCopy()
{
  QClipboard* clip = QApplication::clipboard();

  if (mUrlCurrent.protocol() == "mailto")
  {
    // put the url into the mouse selection and the clipboard
    clip->setSelectionMode(true);
    clip->setText(mUrlCurrent.path());
    clip->setSelectionMode(false);
    clip->setText(mUrlCurrent.path());
    statusMsg(i18n("Address copied to clipboard."));
  }
  else
  {
    // put the url into the mouse selection and the clipboard
    clip->setSelectionMode(true);
    clip->setText(mUrlCurrent.url());
    clip->setSelectionMode(false);
    clip->setText(mUrlCurrent.url());
    statusMsg(i18n("URL copied to clipboard."));
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlOpen()
{
  if (mUrlCurrent.isEmpty()) return;
  //  mMsgView->slotUrlOpen(mUrlCurrent, QString::null, 0);
  mMsgView->slotUrlOpen( mUrlCurrent, KParts::URLArgs() );
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlSave()
{
  if (mUrlCurrent.isEmpty()) return;
  KURL saveUrl = KFileDialog::getSaveURL(mUrlCurrent.fileName(), QString::null,
    this);
  if (saveUrl.isEmpty()) return;
  if (KIO::NetAccess::exists(saveUrl))
  {
    if (KMessageBox::warningContinueCancel(0,
        i18n("File %1 exists.\nDo you want to replace it?")
        .arg(saveUrl.prettyURL()), i18n("Save to File"), i18n("&Replace"))
        != KMessageBox::Continue)
      return;
  }
  KIO::Job *job = KIO::file_copy(mUrlCurrent, saveUrl, -1, true);
  connect(job, SIGNAL(result(KIO::Job*)), SLOT(slotUrlSaveResult(KIO::Job*)));
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlSaveResult(KIO::Job *job)
{
  if (job->error()) job->showErrorDialog();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgPopup(KMMessage &aMsg, const KURL &aUrl, const QPoint& aPoint)
{
  KPopupMenu * menu = new KPopupMenu;
  updateMessageMenu();

  mMsgCurrent = jumpToMessage(&aMsg);
  mUrlCurrent = aUrl;

  if (!aUrl.isEmpty())
  {
    if (aUrl.protocol() == "mailto")
    {
      // popup on a mailto URL
      menu->insertItem(i18n("Send To..."), this,
                       SLOT(slotMailtoCompose()));
      if ( mMsgCurrent ) {
        menu->insertItem(i18n("Send Reply To..."), this,
                         SLOT(slotMailtoReply()));
        menu->insertItem(i18n("Forward To..."), this,
                         SLOT(slotMailtoForward()));
        menu->insertSeparator();
      }
      menu->insertItem(i18n("Add to Addressbook..."), this,
		       SLOT(slotMailtoAddAddrBook()));
      menu->insertItem(i18n("Copy to Clipboard"), this,
		       SLOT(slotUrlCopy()));
    }
    else
    {
      // popup on a not-mailto URL
      menu->insertItem(i18n("Open URL..."), this,
		       SLOT(slotUrlOpen()));
      menu->insertItem(i18n("Save Link As..."), this,
                       SLOT(slotUrlSave()));
      menu->insertItem(i18n("Copy to Clipboard"), this,
		       SLOT(slotUrlCopy()));
    }
  }
  else
  {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mMsgCurrent) // no messages
    {
         delete menu;
         return;
     }

     bool out_folder = kernel->folderIsDraftOrOutbox(mFolder);
     if ( out_folder )
         editAction->plug(menu);
     else {
         replyAction->plug(menu);
         replyAllAction->plug(menu);
         forwardAction->plug(menu);
         forwardAttachedAction->plug(menu);
         redirectAction->plug(menu);
         bounceAction->plug(menu);
     }
     menu->insertSeparator();
     if ( !out_folder ) {
         filterMenu->plug( menu );
         statusMenu->plug( menu );
	 threadStatusMenu->plug( menu );
     }

     moveActionMenu->plug( menu );
     copyActionMenu->plug( menu );

     menu->insertSeparator();
     toggleFixFontAction->plug(menu);
     viewSourceAction->plug(menu);

     menu->insertSeparator();
     printAction->plug(menu);
     saveAsAction->plug(menu);
     menu->insertSeparator();
     deleteAction->plug(menu);
  }
  menu->exec(aPoint, 0);
  delete menu;
}

//-----------------------------------------------------------------------------
void KMMainWin::getAccountMenu()
{
  QStringList actList;

  actMenu->clear();
  actList = kernel->acctMgr()->getAccounts(false);
  QStringList::Iterator it;
  int id = 0;
  for(it = actList.begin(); it != actList.end() ; ++it, id++)
    actMenu->insertItem((*it).replace(QRegExp("&"),"&&"), id);
}

// little helper function
KRadioAction * KMMainWin::actionForHeaderStyle( int style ) {
  const char * actionName = 0;
  switch( style ) {
  case KMReaderWin::HdrFancy:
    actionName = "view_headers_fancy"; break;
  case KMReaderWin::HdrBrief:
    actionName = "view_headers_brief"; break;
  case KMReaderWin::HdrStandard:
    actionName = "view_headers_standard"; break;
  case KMReaderWin::HdrLong:
    actionName = "view_headers_long"; break;
  case KMReaderWin::HdrAll:
    actionName = "view_headers_all"; break;
  }
  if ( actionName )
    return static_cast<KRadioAction*>(actionCollection()->action(actionName));
  else
    return 0;
}

KRadioAction * KMMainWin::actionForAttachmentStyle( int style ) {
  const char * actionName = 0;
  switch ( style ) {
  case KMReaderWin::IconicAttmnt:
    actionName = "view_attachments_as_icons"; break;
  case KMReaderWin::SmartAttmnt:
    actionName = "view_attachments_smart"; break;
  case KMReaderWin::InlineAttmnt:
    actionName = "view_attachments_inline"; break;
  }
  if ( actionName )
    return static_cast<KRadioAction*>(actionCollection()->action(actionName));
  else
    return 0;
}


//-----------------------------------------------------------------------------
void KMMainWin::setupMenuBar()
{
  //----- File Menu
  (void) new KAction( i18n("&New Mail Client..."), "window_new", 0,
		      this, SLOT(slotNewMailReader()),
		      actionCollection(), "new_mail_client" );

  saveAsAction = new KAction( i18n("Save &As..."), "filesave",
    KStdAccel::shortcut(KStdAccel::Save),
    this, SLOT(slotSaveMsg()), actionCollection(), "save_as" );

  printAction = KStdAction::print (this, SLOT(slotPrintMsg()), actionCollection());

  (void) new KAction( i18n("Compact All &Folders"), 0,
		      this, SLOT(slotCompactAll()),
		      actionCollection(), "compact_all_folders" );

  (void) new KAction( i18n("&Expire All Folders"), 0,
		      this, SLOT(slotExpireAll()),
		      actionCollection(), "expire_all_folders" );

  (void) new KAction( i18n("Check &Mail"), "mail_get", CTRL+Key_L,
		      this, SLOT(slotCheckMail()),
		      actionCollection(), "check_mail" );

  KActionMenu *actActionMenu = new
    KActionMenu( i18n("Chec&k Mail In"), "mail_get", actionCollection(),
				   	"check_mail_in" );
  actActionMenu->setDelayed(true); //needed for checking "all accounts"

  connect(actActionMenu,SIGNAL(activated()),this,SLOT(slotCheckMail()));

  actMenu = actActionMenu->popupMenu();
  connect(actMenu,SIGNAL(activated(int)),this,SLOT(slotCheckOneAccount(int)));
  connect(actMenu,SIGNAL(aboutToShow()),this,SLOT(getAccountMenu()));

  (void) new KAction( i18n("&Send Queued"), "mail_send", 0, this,
		     SLOT(slotSendQueued()), actionCollection(), "send_queued");

  (void) new KAction( i18n("Address &Book..."), "contents", 0, this,
		      SLOT(slotAddrBook()), actionCollection(), "addressbook" );

  (void) new KAction( i18n("&Import..."), "fileopen", 0, this,
		      SLOT(slotImport()), actionCollection(), "import" );

  KStdAction::quit( this, SLOT(slotQuit()), actionCollection());

  //----- Edit Menu
  KStdAction::undo( this, SLOT(slotUndo()), actionCollection(), "edit_undo");

  (void) new KAction( i18n("&Copy Text"), KStdAccel::shortcut(KStdAccel::Copy), this,
		      SLOT(slotCopyText()), actionCollection(), "copy_text" );

  deleteAction = new KAction( i18n("&Delete"), "editdelete", Key_D, this,
		      SLOT(slotDeleteMsg()), actionCollection(), "delete" );

  (void) new KAction( i18n("&Search Messages..."), "find", Key_S, this,
		      SLOT(slotSearch()), actionCollection(), "search_messages" );

  (void) new KAction( i18n("&Find in Message..."), KStdAccel::shortcut(KStdAccel::Find), this,
		      SLOT(slotFind()), actionCollection(), "find_in_messages" );

  (void) new KAction( i18n("Select &All Messages"), Key_K, this,
		      SLOT(slotMarkAll()), actionCollection(), "mark_all_messages" );

  (void) new KAction( i18n("Select Message &Text"), KStdAccel::shortcut(KStdAccel::SelectAll), this,
		      SLOT(slotSelectText()), actionCollection(), "mark_all_text" );

  //----- Folder Menu
  (void) new KAction( i18n("&Create..."), "folder_new", 0, this,
		      SLOT(slotAddFolder()), actionCollection(), "create" );

  modifyFolderAction = new KAction( i18n("&Properties..."), 0, this,
		      SLOT(slotModifyFolder()), actionCollection(), "modify" );

  markAllAsReadAction = new KAction( i18n("&Mark All Messages as Read"), "goto", 0, this,
		      SLOT(slotMarkAllAsRead()), actionCollection(), "mark_all_as_read" );

  expireFolderAction = new KAction(i18n("E&xpire"), 0, this, SLOT(slotExpireFolder()),
				   actionCollection(), "expire");

  compactFolderAction = new KAction( i18n("C&ompact"), 0, this,
		      SLOT(slotCompactFolder()), actionCollection(), "compact" );

  emptyFolderAction = new KAction( i18n("&Empty"), 0, this,
		      SLOT(slotEmptyFolder()), actionCollection(), "empty" );

  removeFolderAction = new KAction( i18n("&Remove"), 0, this,
		      SLOT(slotRemoveFolder()), actionCollection(), "remove" );

  preferHtmlAction = new KToggleAction( i18n("Pre&fer HTML to Plain Text"), 0, this,
		      SLOT(slotOverrideHtml()), actionCollection(), "prefer_html" );

  threadMessagesAction = new KToggleAction( i18n("&Thread Messages"), 0, this,
		      SLOT(slotOverrideThread()), actionCollection(), "thread_messages" );

  //----- Message Menu
  (void) new KAction( i18n("&New Message..."), "filenew", KStdAccel::shortcut(KStdAccel::New), this,
		      SLOT(slotCompose()), actionCollection(), "new_message" );

  (void) new KAction( i18n("P&ost to Mailing-List..."), 0, this,
		      SLOT(slotPostToML()), actionCollection(), "post_message" );

  (void) new KAction( i18n("next message","Ne&xt"), Key_N, this,
		      SLOT(slotNextMessage()), actionCollection(), "next" );

  (void) new KAction( i18n("Next &Unread"), "next", Key_Plus, this,
		      SLOT(slotNextUnreadMessage()), actionCollection(), "next_unread" );

  (void) new KAction( i18n("previous message","&Previous"), Key_P, this,
		      SLOT(slotPrevMessage()), actionCollection(), "previous" );

  (void) new KAction( i18n("Previou&s Unread"), "previous", Key_Minus, this,
		      SLOT(slotPrevUnreadMessage()), actionCollection(), "previous_unread" );

  replyAction = new KAction( i18n("&Reply..."), "mail_reply", Key_R, this,
		      SLOT(slotReplyToMsg()), actionCollection(), "reply" );

  noQuoteReplyAction = new KAction( i18n("Reply Without &Quote..."), ALT+Key_R,
    this, SLOT(slotNoQuoteReplyToMsg()), actionCollection(), "noquotereply" );

  replyAllAction = new KAction( i18n("Reply &All..."), "mail_replyall",
     Key_A, this, SLOT(slotReplyAllToMsg()), actionCollection(), "reply_all" );

  replyListAction = new KAction( i18n("Reply &List..."), "mail_replylist",
     Key_L, this, SLOT(slotReplyListToMsg()), actionCollection(), "reply_list" );

  forwardAction = new KAction( i18n("&Forward..."), "mail_forward", Key_F, this,
		      SLOT(slotForwardMsg()), actionCollection(), "forward" );

  forwardAttachedAction = new KAction( i18n("For&ward as Attachment"), SHIFT+Key_F, this,
		      SLOT(slotForwardAttachedMsg()), actionCollection(), "forward_attached" );

  redirectAction = new KAction( i18n("Re&direct..."), Key_E, this,
		      SLOT(slotRedirectMsg()), actionCollection(), "redirect" );

  bounceAction = new KAction( i18n("&Bounce..."), 0, this,
		      SLOT(slotBounceMsg()), actionCollection(), "bounce" );

  sendAgainAction = new KAction( i18n("Send A&gain..."), 0, this,
		      SLOT(slotResendMsg()), actionCollection(), "send_again" );

  //----- Message-Encoding Submenu
  mEncoding = new KSelectAction( i18n( "Set &Encoding" ), "charset", 0, this,
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

  editAction = new KAction( i18n("Edi&t"), Key_T, this,
		      SLOT(slotEditMsg()), actionCollection(), "edit" );

  //----- Create filter submenu
  filterMenu = new KActionMenu( i18n("Create F&ilter"), actionCollection(), "create_filter" );

  KAction *subjectFilterAction = new KAction( i18n("Filter on &Subject..."), 0, this,
                                              SLOT(slotSubjectFilter()),
                                              actionCollection(), "subject_filter");
  filterMenu->insert( subjectFilterAction );

  KAction *fromFilterAction = new KAction( i18n("Filter on &From..."), 0, this,
                                           SLOT(slotFromFilter()),
                                           actionCollection(), "from_filter");
  filterMenu->insert( fromFilterAction );

  KAction *toFilterAction = new KAction( i18n("Filter on &To..."), 0, this,
                                         SLOT(slotToFilter()),
                                         actionCollection(), "to_filter");
  filterMenu->insert( toFilterAction );

  mlistFilterAction = new KAction( i18n("Filter on Mailing-&List..."), 0, this,
                                   SLOT(slotMailingListFilter()), actionCollection(),
                                   "mlist_filter");
  filterMenu->insert( mlistFilterAction );

  KAction *action;
  QString msg;
  //----- "Mark Message" submenu
  statusMenu = new KActionMenu ( i18n( "Mar&k Message" ),
				 actionCollection(), "set_status" );
  action = new KAction( i18n("&New"), "kmmsgnew", 0, this,
			SLOT(slotSetMsgStatusNew()),
			actionCollection(), "status_new" );
  msg = i18n("Mark current message as new");
  action->setToolTip( msg );
  statusMenu->insert( action );

  action = new KAction( i18n("&Unread"), "kmmsgunseen", 0, this,
			SLOT(slotSetMsgStatusUnread()),
			actionCollection(), "status_unread");
  msg = i18n("Mark current message as unread");
  action->setToolTip( msg );
  statusMenu->insert( action );

  action = new KAction( i18n("&Read"), "kmmsgold", 0, this,
			SLOT(slotSetMsgStatusRead()),
			actionCollection(), "status_read");
  msg = i18n("Mark current message as read");
  action->setToolTip( msg );
  statusMenu->insert( action );

  action = new KAction( i18n("R&eplied"), "kmmsgreplied", 0, this,
			SLOT(slotSetMsgStatusReplied()),
			actionCollection(), "status_replied");
  msg = i18n("Mark current message as replied");
  action->setToolTip( msg );
  statusMenu->insert( action );

  action = new KAction( i18n("&Queued"), "kmmsgqueued", 0, this,
			SLOT(slotSetMsgStatusQueued()),
			actionCollection(), "status_queued");
  action->setToolTip( i18n("Mark current message as queued") );
  statusMenu->insert( action );

  action = new KAction( i18n("&Sent"), "kmmsgsent", 0, this,
			SLOT(slotSetMsgStatusSent()),
			actionCollection(), "status_sent");
  msg = i18n("Mark current message as sent");
  action->setToolTip( msg );
  statusMenu->insert( action );

  action = new KAction( i18n("&Important"), "kmmsgflag", 0, this,
			SLOT(slotSetMsgStatusFlag()),
			actionCollection(), "status_flag");
  msg = i18n("Mark current message as important");
  action->setToolTip( msg );
  statusMenu->insert( action );

  //----- "Mark Thread" submenu
  threadStatusMenu = new KActionMenu ( i18n( "Mark T&hread" ),
				       actionCollection(), "thread_status" );
  action = new KAction( i18n("&New"), "kmmsgnew", 0, this,
			SLOT(slotSetThreadStatusNew()),
			actionCollection(), "thread_new");
  msg = i18n("Mark current thread as new");
  action->setToolTip( msg );
  threadStatusMenu->insert( action );

  action = new KAction( i18n("&Unread"), "kmmsgunseen", 0, this,
			SLOT(slotSetThreadStatusUnread()),
			actionCollection(), "thread_unread");
  msg = i18n("Mark current thread as unread");
  action->setToolTip( msg );
  threadStatusMenu->insert( action );

  action = new KAction( i18n("&Read"), "kmmsgold", 0, this,
			SLOT(slotSetThreadStatusRead()),
			actionCollection(), "thread_read");
  msg = i18n("Mark current thread as read");
  action->setToolTip( msg );
  threadStatusMenu->insert( action );

  action = new KAction( i18n("R&eplied"), "kmmsgreplied", 0, this,
			SLOT(slotSetThreadStatusReplied()),
			actionCollection(), "thread_replied");
  msg = i18n("Mark current thread as replied");
  action->setToolTip( msg );
  threadStatusMenu->insert( action );

  action = new KAction( i18n("&Queued"), "kmmsgqueued", 0, this,
			SLOT(slotSetThreadStatusQueued()),
			actionCollection(), "thread_queued");
  msg = i18n("Mark current thread as queued");
  action->setToolTip( msg );
  threadStatusMenu->insert( action );

  action = new KAction( i18n("&Sent"), "kmmsgsent", 0, this,
			SLOT(slotSetThreadStatusSent()),
			actionCollection(), "thread_sent");
  msg = i18n("Mark current thread as sent");
  action->setToolTip( msg );
  threadStatusMenu->insert( action );

  action = new KAction( i18n("&Important"), "kmmsgflag", 0, this,
			SLOT(slotSetThreadStatusFlag()),
			actionCollection(), "thread_flag");
  msg = i18n("Mark current thread as important");
  action->setToolTip( msg );
  threadStatusMenu->insert( action );


  moveActionMenu = new KActionMenu( i18n("&Move To" ),
                                    actionCollection(), "move_to" );

  copyActionMenu = new KActionMenu( i18n("&Copy To" ),
                                    actionCollection(), "copy_to" );

  (void) new KAction( i18n("Appl&y Filters"), "filter", CTRL+Key_J, this,
		      SLOT(slotApplyFilters()), actionCollection(), "apply_filters" );

  viewSourceAction = new KAction( i18n("&View Source..."), Key_V, this,
		      SLOT(slotShowMsgSrc()), actionCollection(), "view_source" );


  //----- View Menu
  KRadioAction * raction = 0;

  // "Headers" submenu:
  KActionMenu * headerMenu =
    new KActionMenu( i18n("View->", "&Headers"),
		     actionCollection(), "view_headers" );
  msg = i18n("Choose display style of message headers");
  headerMenu->setToolTip( msg );

  connect( headerMenu, SIGNAL(activated()), SLOT(slotCycleHeaderStyles()) );

  raction = new KRadioAction( i18n("View->headers->", "&Fancy"), 0, this,
			      SLOT(slotFancyHeaders()),
			      actionCollection(), "view_headers_fancy" );
  msg = i18n("Show the list of headers in a fancy format");
  raction->setToolTip( msg );
  raction->setExclusiveGroup( "view_headers_group" );
  headerMenu->insert( raction );

  raction = new KRadioAction( i18n("View->headers->", "&Brief"), 0, this,
			      SLOT(slotBriefHeaders()),
			      actionCollection(), "view_headers_brief" );
  msg = i18n("Show brief list of message headers");
  raction->setToolTip( msg );
  raction->setExclusiveGroup( "view_headers_group" );
  headerMenu->insert( raction );

  raction = new KRadioAction( i18n("View->headers->", "&Standard"), 0, this,
			      SLOT(slotStandardHeaders()),
			      actionCollection(), "view_headers_standard" );
  msg = i18n("Show standard list of message headers");
  raction->setToolTip( msg );
  raction->setExclusiveGroup( "view_headers_group" );
  headerMenu->insert( raction );

  raction = new KRadioAction( i18n("View->headers->", "&Long"), 0, this,
			      SLOT(slotLongHeaders()),
			      actionCollection(), "view_headers_long" );
  msg = i18n("Show long list of message headers");
  raction->setToolTip( msg );
  raction->setExclusiveGroup( "view_headers_group" );
  headerMenu->insert( raction );

  raction = new KRadioAction( i18n("View->headers->", "&All"), 0, this,
			      SLOT(slotAllHeaders()),
			      actionCollection(), "view_headers_all" );
  msg = i18n("Show all message headers");
  raction->setToolTip( msg );
  raction->setExclusiveGroup( "view_headers_group" );
  headerMenu->insert( raction );

  // check the right one:
  raction = actionForHeaderStyle( mMsgView->headerStyle() ); 
  if ( raction )
    raction->setChecked( true );

  // "Attachments" submenu:
  KActionMenu * attachmentMenu =
    new KActionMenu( i18n("View->", "&Attachments"),
		     actionCollection(), "view_attachments" );
  connect( attachmentMenu, SIGNAL(activated()),
	   SLOT(slotCycleAttachmentStyles()) );

  msg = i18n("Choose display style of attachments");
  attachmentMenu->setToolTip( msg );

  raction = new KRadioAction( i18n("View->attachments->", "&As Icons"), 0, this,
			      SLOT(slotIconicAttachments()),
			      actionCollection(), "view_attachments_as_icons" );
  msg = i18n("Show all attachments as icons. Click to see them.");
  raction->setToolTip( msg );
  raction->setExclusiveGroup( "view_attachments_group" );
  attachmentMenu->insert( raction );

  raction = new KRadioAction( i18n("View->attachments->", "&Smart"), 0, this,
			      SLOT(slotSmartAttachments()),
			      actionCollection(), "view_attachments_smart" );
  msg = i18n("Show attachments as suggested by sender.");
  raction->setToolTip( msg );
  raction->setExclusiveGroup( "view_attachments_group" );
  attachmentMenu->insert( raction );

  raction = new KRadioAction( i18n("View->attachments->", "&Inline"), 0, this,
			      SLOT(slotInlineAttachments()),
			      actionCollection(), "view_attachments_inline" );
  msg = i18n("Show all attachments inline (if possible)");
  raction->setToolTip( msg );
  raction->setExclusiveGroup( "view_attachments_group" );
  attachmentMenu->insert( raction );

  // check the right one:
  raction = actionForAttachmentStyle( mMsgView->attachmentStyle() );
  if ( raction )
    raction->setChecked( true );

  toggleFixFontAction = new KToggleAction( i18n("Fixed Font &Widths"),
			0, this, SLOT(slotToggleFixedFont()),
			actionCollection(), "toggle_fixedfont" );

  //----- Settings Menu
  toolbarAction = KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
    actionCollection());
  statusbarAction = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
    actionCollection());


  KStdAction::keyBindings(this, SLOT(slotEditKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
  KStdAction::preferences(this, SLOT(slotSettings()), actionCollection());


  (void) new KAction( i18n("Configure &Filters..."), 0, this,
 		      SLOT(slotFilter()), actionCollection(), "filter" );
  (void) new KAction( i18n("Configure &Pop Filters..."), 0, this,
 		      SLOT(slotPopFilter()), actionCollection(), "popFilter" );

  createGUI( "kmmainwin.rc", false );

  QObject::connect( guiFactory()->container("folder", this),
		    SIGNAL( aboutToShow() ), this, SLOT( updateFolderMenu() ));

  QObject::connect( guiFactory()->container("message", this),
		    SIGNAL( aboutToShow() ), this, SLOT( updateMessageMenu() ));


  conserveMemory();

  menutimer = new QTimer( this, "menutimer" );
  connect( menutimer, SIGNAL( timeout() ), SLOT( updateMessageActions() ) );

  updateMessageActions();

}


//-----------------------------------------------------------------------------

void KMMainWin::slotToggleToolBar()
{
  if(toolBar("mainToolBar")->isVisible())
    toolBar("mainToolBar")->hide();
  else
    toolBar("mainToolBar")->show();
}

void KMMainWin::slotToggleStatusBar()
{
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
}


void KMMainWin::slotEditToolbars()
{
  saveMainWindowSettings(kapp->config(), "MainWindow");
  KEditToolbar dlg(actionCollection(), "kmmainwin.rc");

  connect( &dlg, SIGNAL(newToolbarConfig()),
	   SLOT(slotUpdateToolbars()) );

  dlg.exec();
}

void KMMainWin::slotUpdateToolbars()
{
  createGUI("kmmainwin.rc");
  applyMainWindowSettings(kapp->config(), "MainWindow");
  toolbarAction->setChecked(!toolBar()->isHidden());
}

void KMMainWin::slotEditKeys()
{
  KKeyDialog::configureKeys(actionCollection(), xmlFile(), true, this);
}

//-----------------------------------------------------------------------------
void KMMainWin::setupStatusBar()
{
  mStatusBar = new KStatusBar(this);

  littleProgress = new KMLittleProgressDlg( mStatusBar );

  mStatusBar->addWidget( littleProgress, 0 , true );
  mMessageStatusId = 1;
  mStatusBar->insertItem(i18n(" Initializing..."), 1, 1 );
  mStatusBar->setItemAlignment( 1, AlignLeft | AlignVCenter );
  littleProgress->show();
  connect( KMBroadcastStatus::instance(), SIGNAL(statusProgressEnable( bool )),
	   littleProgress, SLOT(slotEnable( bool )));
  connect( KMBroadcastStatus::instance(),
	   SIGNAL(statusProgressPercent( unsigned long )),
	   littleProgress,
	   SLOT(slotJustPercent( unsigned long )));
  connect( KMBroadcastStatus::instance(), SIGNAL(resetRequested()),
	   littleProgress, SLOT(slotClean()));
  connect( KMBroadcastStatus::instance(), SIGNAL(statusMsg( const QString& )),
	   this, SLOT(statusMsg( const QString& )));
  setStatusBar(mStatusBar);
}

void KMMainWin::slotQuit()
{
    close();
}

void KMMainWin::slotReadOn()
{
    if ( !mMsgView )
        return;

    if ( !mMsgView->atBottom() ) {
        mMsgView->slotJumpDown();
        return;
    }
    int i = mHeaders->findUnread(true, -1, false, false);
    if ( i < 0 ) // let's try from start, what gives?
        i = mHeaders->findUnread(true, 0, false, true);
    if ( i >= 0 ) {
         mHeaders->setCurrentMsg(i);
         QTimer::singleShot( 100, mHeaders, SLOT( ensureCurrentItemVisible() ) );
         return;
    }
    mFolderTree->nextUnreadFolder( true );
}

void KMMainWin::slotExpandThread()
{
  mHeaders->slotExpandOrCollapseThread( true ); // expand
}

void KMMainWin::slotCollapseThread()
{
  mHeaders->slotExpandOrCollapseThread( false ); // collapse
}

void KMMainWin::slotExpandAllThreads()
{
  mHeaders->slotExpandOrCollapseAllThreads( true ); // expand
}

void KMMainWin::slotCollapseAllThreads()
{
  mHeaders->slotExpandOrCollapseAllThreads( false ); // collapse
}


//-----------------------------------------------------------------------------
void KMMainWin::moveSelectedToFolder( int menuId )
{
  if (mMenuToFolder[menuId])
    mHeaders->moveMsgToFolder( mMenuToFolder[menuId] );
}


//-----------------------------------------------------------------------------
void KMMainWin::copySelectedToFolder(int menuId )
{
  if (mMenuToFolder[menuId])
    mHeaders->copyMsgToFolder( mMenuToFolder[menuId] );
}


//-----------------------------------------------------------------------------
QPopupMenu* KMMainWin::folderToPopupMenu(KMFolderTreeItem* fti,
					 bool move,
					 QObject *receiver,
					 KMMenuToFolder *aMenuToFolder,
					 QPopupMenu *menu )
{
  while ( menu->count() )
  {
    QPopupMenu *popup = menu->findItem( menu->idAt( 0 ) )->popup();
    if (popup)
      delete popup;
    else
      menu->removeItemAt( 0 );
  }

  if (!fti) fti = static_cast<KMFolderTreeItem*>(mFolderTree->firstChild());
  if (move)
  {
    disconnect(menu, SIGNAL(activated(int)), receiver,
           SLOT(moveSelectedToFolder(int)));
    connect(menu, SIGNAL(activated(int)), receiver,
             SLOT(moveSelectedToFolder(int)));
  } else {
    disconnect(menu, SIGNAL(activated(int)), receiver,
           SLOT(copySelectedToFolder(int)));
    connect(menu, SIGNAL(activated(int)), receiver,
             SLOT(copySelectedToFolder(int)));
  }

  if (fti->folder && !fti->folder->isDir())
  {
      int menuId;
      if (move)
          menuId = menu->insertItem(i18n("Move to this Folder"));
      else
          menuId = menu->insertItem(i18n("Copy to this Folder"));
      aMenuToFolder->insert( menuId, fti->folder );
      menu->insertSeparator();
  }
  fti = static_cast<KMFolderTreeItem*>(fti->firstChild());
  while (fti)
  {
    if (fti->folder)
    {
      QString label = fti->text(0);
      label.replace(QRegExp("&"),QString("&&"));
      if (fti->firstChild())
      {
        QPopupMenu *subMenu = folderToPopupMenu(fti, move, receiver,
                                                aMenuToFolder,
                                                new QPopupMenu(menu, "subMenu"));
        menu->insertItem(label, subMenu);
      } else
      if (!fti->folder->isDir())
      {
        int menuId = menu->insertItem(label);
        aMenuToFolder->insert( menuId, fti->folder );
      }
    }
    fti = static_cast<KMFolderTreeItem*>(fti->nextSibling());
  }
  return menu;
}


//-----------------------------------------------------------------------------
void KMMainWin::updateMessageMenu()
{
    mMenuToFolder.clear();
    folderToPopupMenu( 0, true, this, &mMenuToFolder, moveActionMenu->popupMenu() );
    folderToPopupMenu( 0, false, this, &mMenuToFolder, copyActionMenu->popupMenu() );
    updateMessageActions();
}

void KMMainWin::startUpdateMessageActionsTimer()
{
    menutimer->stop();
    menutimer->start( 20, true );
}

void KMMainWin::updateMessageActions()
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

    mlistFilterAction->setText( i18n("Filter on Mailing-List...") );

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
    statusMenu->setEnabled( mass_actions );
    threadStatusMenu->setEnabled( mass_actions &&
				  allSelectedInCommonThread &&
				  mHeaders->isThreaded() );
    moveActionMenu->setEnabled( mass_actions );
    copyActionMenu->setEnabled( mass_actions );
    deleteAction->setEnabled( mass_actions );
    // FIXME: Temporarily disable forwarding of multiple IMAP messages because
    //        it doesn't work correctly
    if( mFolder && mFolder->protocol() != "imap" )
    {
      forwardAction->setEnabled( mass_actions );
      forwardAttachedAction->setEnabled( mass_actions );
    }
    else
    {
      forwardAction->setEnabled( count == 1 );
      forwardAttachedAction->setEnabled( count == 1 );
    }
    action( "apply_filters" )->setEnabled( mass_actions );

    bool single_actions = count == 1;
    filterMenu->setEnabled( single_actions );
    editAction->setEnabled( single_actions &&
      kernel->folderIsDraftOrOutbox(mFolder));
    bounceAction->setEnabled( single_actions );
    replyAction->setEnabled( single_actions );
    noQuoteReplyAction->setEnabled( single_actions );
    replyAllAction->setEnabled( single_actions );
    replyListAction->setEnabled( single_actions );
    redirectAction->setEnabled( single_actions );
    sendAgainAction->setEnabled( single_actions );
    printAction->setEnabled( single_actions );
    saveAsAction->setEnabled( mass_actions );
    viewSourceAction->setEnabled( single_actions );

    if ( count == 1 ) {
        KMMessage *msg;
        int aIdx;
        if((aIdx = mHeaders->currentItemIndex()) <= -1)
           return;
        if(!(msg = mHeaders->getMsg(aIdx)))
            return;

        QCString name;
        QString value;
        QString lname = KMMLInfo::name( msg, name, value );
        if ( lname.isNull() )
            mlistFilterAction->setEnabled( false );
        else {
            mlistFilterAction->setEnabled( true );
            mlistFilterAction->setText( i18n( "Filter on Mailing-List %1..." ).arg( lname ) );
        }
    }

    bool mails = mFolder && mFolder->count();
    action( "next" )->setEnabled( mails );
    action( "next_unread" )->setEnabled( mails );
    action( "previous" )->setEnabled( mails );
    action( "previous_unread" )->setEnabled( mails );

    action( "send_queued" )->setEnabled( kernel->outboxFolder()->count() > 0 );
    action( "edit_undo" )->setEnabled( mHeaders->canUndo() );
}


//-----------------------------------------------------------------------------
void KMMainWin::updateFolderMenu()
{
  modifyFolderAction->setEnabled( mFolder ? !mFolder->noContent() : false );
  compactFolderAction->setEnabled( mFolder ? !mFolder->noContent() : false );
  emptyFolderAction->setEnabled( mFolder ? !mFolder->noContent() : false );
  removeFolderAction->setEnabled( (mFolder && !mFolder->isSystemFolder()) );
  expireFolderAction->setEnabled( mFolder && mFolder->protocol() != "imap"
    && mFolder->isAutoExpire() );
  markAllAsReadAction->setEnabled( mFolder && (mFolder->countUnread() > 0) );
  preferHtmlAction->setEnabled( mFolder ? true : false );
  threadMessagesAction->setEnabled( mFolder ? true : false );

  preferHtmlAction->setChecked( mHtmlPref ? !mFolderHtmlPref : mFolderHtmlPref );
  threadMessagesAction->setChecked( mThreadPref ? !mFolderThreadPref : mFolderThreadPref );
}


#ifdef MALLOC_DEBUG
QString fmt(long n) {
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

void KMMainWin::slotMemInfo() {
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
bool KMMainWin::queryClose() {
  int      ret = 0;
  QString  str = i18n("Expire old messages from all folders? "
		      "Expired messages are permanently deleted.");
  KConfig *config = kapp->config();

  // Make sure this is the last window.
  KMainWindow   *kmWin = NULL;
  int           num = 0;

  kernel->setCanExpire(false);
  for (kmWin = KMainWindow::memberList->first(); kmWin;
       kmWin = KMainWindow::memberList->next()) {
    if (kmWin->isA("KMMainWin")) {
      num++;
    }
  }
  // If this isn't the last open window, don't do anything.
  if (num > 1) {
    return true;
  }

  KConfigGroupSaver saver(config, "General");
  if (config->readNumEntry("when-to-expire", 0) != expireAtExit) {
    return true;
  }

  if (config->readBoolEntry("warn-before-expire")) {
    ret = KMessageBox::warningContinueCancel(KMainWindow::memberList->first(),
			 str, i18n("Expire old Messages?"), i18n("Expire"));
    if (ret == KMessageBox::Continue) {
      kernel->setCanExpire(true);
    }
  }

  return true;
}
