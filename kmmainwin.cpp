// kmmainwin.cpp
//#define MALLOC_DEBUG 1
#define IDENTITY_UOIDs

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
#include <qheader.h>
#include <qguardedptr.h>

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
#include <ktip.h>
#include <kdeversion.h>
#if KDE_VERSION >= 306
#include <knotifydialog.h>
#endif

#include "kmbroadcaststatus.h"
#include "kmfoldermgr.h"
#include "kmfolderdia.h"
#include "kmacctmgr.h"
#include "kbusyptr.h"
#include "kmcommands.h"
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
#include "kmundostack.h"
#include "vacation.h"
using KMail::Vacation;

#include <assert.h>
#include <kstatusbar.h>
#include <kpopupmenu.h>
#include <kprogress.h>

#include "kmmainwin.moc"

//-----------------------------------------------------------------------------
KMMainWin::KMMainWin(QWidget *) :
    KMMainWinInherited("kmail-mainwindow")
{
  // must be the first line of the constructor:
  searchWin = 0;
  mStartupDone = FALSE;
  mIntegrated  = TRUE;
  mFolder = 0;
  mFolderThreadPref = false;
  mFolderHtmlPref = false;
  mCountJobs = 0;

  mPanner1Sep << 1 << 1;
  mPanner2Sep << 1 << 1 << 1;
  mPanner3Sep << 1 << 1;


  setMinimumSize(400, 300);

  readPreConfig();
  createWidgets();

  setupMenuBar();
  setupStatusBar();

  applyMainWindowSettings(KMKernel::config(), "Main Window");
  toolbarAction->setChecked(!toolBar()->isHidden());
  statusbarAction->setChecked(!statusBar()->isHidden());

  readConfig();

  activatePanners();


  if (kernel->firstStart() || kernel->previousVersion() != KMAIL_VERSION)
    slotIntro();
  else
  {
    KMFolder* startup = 0;
    if (!mStartupFolder.isEmpty())
    {
      // find the startup-folder with this ugly folderMgr switch
      startup = kernel->folderMgr()->findIdString(mStartupFolder);
      if (!startup)
        startup = kernel->imapFolderMgr()->findIdString(mStartupFolder);
      if (!startup)
        startup = kernel->inboxFolder();
    } else {
      startup = kernel->inboxFolder();
    }
    mFolderTree->doFolderSelected(mFolderTree->indexOfFolder(startup));
  }

  connect(kernel->msgSender(), SIGNAL(statusMsg(const QString&)),
	  SLOT(statusMsg(const QString&)));
  connect(kernel->acctMgr(), SIGNAL( checkedMail(bool, bool)),
          SLOT( slotMailChecked(bool, bool)));

  // display the full path to the folder in the caption
  connect(mFolderTree, SIGNAL(currentChanged(QListViewItem*)),
      this, SLOT(slotChangeCaption(QListViewItem*)));

  if ( kernel->firstInstance() )
    QTimer::singleShot( 200, this, SLOT(slotShowTipOnStart()) );

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

  saveMainWindowSettings(KMKernel::config(), "Main Window");
  KMKernel::config()->sync();

  delete mHeaders;
  delete mFolderTree;
}


//-----------------------------------------------------------------------------
void KMMainWin::readPreConfig(void)
{
  KConfig *config = KMKernel::config();


  { // area for config group "Geometry"
    KConfigGroupSaver saver(config, "Geometry");
    mWindowLayout = config->readNumEntry( "windowLayout", 1 );
    mShowMIMETreeMode = config->readNumEntry( "showMIME", 1 );
  }

  KConfigGroupSaver saver(config, "General");
  mEncodingStr = config->readEntry("encoding", "").latin1();
}


//-----------------------------------------------------------------------------
void KMMainWin::readFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());
  mFolderThreadPref = config->readBoolEntry( "threadMessagesOverride", false );
  mFolderHtmlPref = config->readBoolEntry( "htmlMailOverride", false );
}


//-----------------------------------------------------------------------------
void KMMainWin::writeFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Folder-" + mFolder->idString());
  config->writeEntry( "threadMessagesOverride", mFolderThreadPref );
  config->writeEntry( "htmlMailOverride", mFolderHtmlPref );
}


//-----------------------------------------------------------------------------
void KMMainWin::readConfig(void)
{
  KConfig *config = KMKernel::config();


  int oldWindowLayout = 1;
  int oldShowMIMETreeMode = 1;

  QString str;
  QSize siz;

  if (mStartupDone)
  {
    writeConfig();


    oldWindowLayout = mWindowLayout;
    oldShowMIMETreeMode = mShowMIMETreeMode;

    readPreConfig();
    mHeaders->refreshNestedState();


    if(oldWindowLayout != mWindowLayout ||
       oldShowMIMETreeMode != mShowMIMETreeMode )
    {
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
  toggleFixFontAction->setChecked( readerConfig.readBoolEntry( "useFixedFont",
                                                               false ) );

  { // area for config group "Geometry"
    KConfigGroupSaver saver(config, "Geometry");
    mThreadPref = config->readBoolEntry( "nestedMessages", false );
    // size of the mainwin
    QSize defaultSize(750, 560);
    siz = config->readSizeEntry("MainWin", &defaultSize);
    if (!siz.isEmpty())
      resize(siz);
    // default width of the foldertree
    uint folderpanewidth = 250;

    // the default sizes are dependent on the actual layout
    switch( mWindowLayout ) {
    case 0:
        mPanner1Sep[0] = config->readNumEntry( "FolderPaneWidth", folderpanewidth );
        mPanner1Sep[1] = config->readNumEntry( "HeaderPaneWidth", width()-folderpanewidth );
        mPanner2Sep[0] = config->readNumEntry( "HeaderPaneHeight", 180 );
        mPanner2Sep[1] = config->readNumEntry( "MimePaneHeight", 100 );
        mPanner2Sep[2] = config->readNumEntry( "MessagePaneHeight", 280 );
        break;
    case 1:
        mPanner1Sep[0] = config->readNumEntry( "FolderPaneWidth", folderpanewidth );
        mPanner1Sep[1] = config->readNumEntry( "HeaderPaneWidth", width()-folderpanewidth );
        mPanner2Sep[0] = config->readNumEntry( "HeaderPaneHeight", 180 );
        mPanner2Sep[1] = config->readNumEntry( "MessagePaneHeight", 280 );
        mPanner2Sep[2] = config->readNumEntry( "MimePaneHeight", 100 );
        break;
    case 2:
        mPanner1Sep[0] = config->readNumEntry( "FolderPaneWidth", folderpanewidth );
        mPanner1Sep[1] = config->readNumEntry( "HeaderPaneWidth", width()-folderpanewidth );
        mPanner2Sep[0] = config->readNumEntry( "FolderPaneHeight", 380 );
        mPanner2Sep[1] = config->readNumEntry( "MimePaneHeight", 180 );
        mPanner3Sep[0] = config->readNumEntry( "HeaderPaneHeight", 180 );
        mPanner3Sep[1] = config->readNumEntry( "MessagePaneHeight", 380 );
        break;
    case 3:
        mPanner1Sep[0] = config->readNumEntry( "FolderPaneHeight", 280 );
        mPanner1Sep[1] = config->readNumEntry( "MessagePaneHeight", 280 );
        mPanner2Sep[0] = config->readNumEntry( "FolderPaneWidth", folderpanewidth );
        mPanner2Sep[1] = config->readNumEntry( "HeaderPaneWidth", width()-folderpanewidth );
        mPanner3Sep[0] = config->readNumEntry( "HeaderPaneHeight", 140 );
        mPanner3Sep[1] = config->readNumEntry( "MimePaneHeight", 140 );
        break;
    case 4:
        mPanner1Sep[0] = config->readNumEntry( "FolderPaneHeight", 180 );
        mPanner1Sep[1] = config->readNumEntry( "MimePaneHeight", 100 );
        mPanner1Sep[2] = config->readNumEntry( "MessagePaneHeight", 280 );
        mPanner2Sep[0] = config->readNumEntry( "FolderPaneWidth", folderpanewidth );
        mPanner2Sep[1] = config->readNumEntry( "HeaderPaneWidth", width()-folderpanewidth );
        break;
    }

    if (!mStartupDone ||
        oldWindowLayout != mWindowLayout ||
        oldShowMIMETreeMode != mShowMIMETreeMode )
    {
      /** unread / total columns
       * as we have some dependencies in this widget
       * it's better to manage these here */
      // The columns are shown by default.
      int unreadColumn = config->readNumEntry("UnreadColumn", 1);
      int totalColumn = config->readNumEntry("TotalColumn", 2);

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

  mMsgView->readConfig();
  slotSetEncoding();
  mHeaders->readConfig();
  mHeaders->restoreLayout(KMKernel::config(), "Header-Geometry");
  mFolderTree->readConfig();

  { // area for config group "General"
    KConfigGroupSaver saver(config, "General");
    mSendOnCheck = config->readBoolEntry("sendOnCheck",false);
    mBeepOnNew = config->readBoolEntry("beep-on-mail", false);
    mConfirmEmpty = config->readBoolEntry("confirm-before-empty", true);
    // startup-Folder, defaults to system-inbox
    mStartupFolder = config->readEntry("startupFolder", kernel->inboxFolder()->idString());
  }

  // Re-activate panners
  if (mStartupDone)
  {

    if (oldWindowLayout != mWindowLayout ||
        oldShowMIMETreeMode != mShowMIMETreeMode )
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
      mMsgView->clear( true );
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
  KConfig *config = KMKernel::config();


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
        config->writeEntry( "HeaderPaneHeight", mPanner2->sizes()[0] );
        config->writeEntry( "MessagePaneHeight", mPanner2->sizes()[1] );
        config->writeEntry( "MimePaneHeight", mPanner2->sizes()[2] );
        break;
    case 2:
        config->writeEntry( "FolderPaneWidth", mPanner1->sizes()[0] );
        config->writeEntry( "HeaderPaneWidth", mPanner1->sizes()[1] );
        config->writeEntry( "FolderPaneHeight", mPanner2->sizes()[0] );
        config->writeEntry( "MimePaneHeight", mPanner2->sizes()[1] );
        config->writeEntry( "HeaderPaneHeight", mPanner3->sizes()[0] );
        config->writeEntry( "MessagePaneHeight", mPanner3->sizes()[1] );
        break;
    case 3:
        config->writeEntry( "FolderPaneHeight", mPanner1->sizes()[0] );
        config->writeEntry( "MessagePaneHeight", mPanner1->sizes()[1] );
        config->writeEntry( "FolderPaneWidth", mPanner2->sizes()[0] );
        config->writeEntry( "HeaderPaneWidth", mPanner2->sizes()[1] );
        config->writeEntry( "HeaderPaneHeight", mPanner3->sizes()[0] );
        config->writeEntry( "MimePaneHeight", mPanner3->sizes()[1] );
        break;
    case 4:
        config->writeEntry( "FolderPaneHeight", mPanner1->sizes()[0] );
        config->writeEntry( "MimePaneHeight", mPanner1->sizes()[1] );
        config->writeEntry( "MessagePaneHeight", mPanner1->sizes()[2] );
        config->writeEntry( "FolderPaneWidth", mPanner2->sizes()[0] );
        config->writeEntry( "HeaderPaneWidth", mPanner2->sizes()[1] );
        break;
    }

    // save the state of the unread/total-columns
    config->writeEntry("UnreadColumn", mFolderTree->unreadIndex());
    config->writeEntry("TotalColumn", mFolderTree->totalIndex());
  }


  KConfigGroupSaver saver(config, "General");
  config->writeEntry("encoding", QString(mEncodingStr));
  // TODO: change that via GUI in 3.2 (cb)
  config->writeEntry("startupFolder", mStartupFolder);
}


//-----------------------------------------------------------------------------
void KMMainWin::createWidgets(void)
{
    QAccel *accel = new QAccel(this, "createWidgets()");

  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Geometry");

  // Create the splitters according to the layout settings
  QWidget *headerParent = 0, *folderParent = 0,
            *mimeParent = 0, *messageParent = 0;
  switch( mWindowLayout ) {
  case 0:
  case 1:
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
  case 2:
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
  case 3:
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
  case 4:
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
  accel->connectItem(accel->insertItem(SHIFT+Key_Left),
                     mHeaders, SLOT(selectPrevMessage()));
  accel->connectItem(accel->insertItem(SHIFT+Key_Right),
                     mHeaders, SLOT(selectNextMessage()));

  if (!mEncodingStr.isEmpty())
    mCodec = KMMsgBase::codecForName(mEncodingStr);
  else mCodec = 0;


  mMsgView = new KMReaderWin(0, &mShowMIMETreeMode, messageParent);

  connect(mMsgView, SIGNAL(replaceMsgByUnencryptedVersion()),
	  this, SLOT(slotReplaceMsgByUnencryptedVersion()));
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

  // create list of folders
  mFolderTree  = new KMFolderTree(folderParent, "folderTree");

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

  //Commands not worthy of menu items, but that deserve configurable keybindings
  new KAction(
   i18n("Focus on Next Folder"), CTRL+Key_Right, mFolderTree,
   SLOT(incCurrentFolder()), actionCollection(), "inc_current_folder");

  new KAction(
   i18n("Focus on Previous Folder"), CTRL+Key_Left, mFolderTree,
   SLOT(decCurrentFolder()), actionCollection(), "dec_current_folder");

  new KAction(
   i18n("Select Folder with Focus"), CTRL+Key_Space, mFolderTree,
   SLOT(selectCurrentFolder()), actionCollection(), "select_current_folder");

  connect( kernel->outboxFolder(), SIGNAL( msgRemoved(int, QString) ),
           SLOT( startUpdateMessageActionsTimer() ) );
  connect( kernel->outboxFolder(), SIGNAL( msgAdded(int) ),
           SLOT( startUpdateMessageActionsTimer() ) );
}


//-----------------------------------------------------------------------------
void KMMainWin::activatePanners(void)
{
  // glue everything together
    switch( mWindowLayout ) {
    case 0:
    case 1:
        mHeaders->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mMimePartTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mMsgView->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        if( mWindowLayout )
          mPanner2->moveToLast( mMimePartTree );
        else
          mPanner2->moveToLast( mMsgView );
        mFolderTree->reparent( mPanner1, 0, QPoint( 0, 0 ) );
        mPanner1->moveToLast( mPanner2 );
        mPanner1->setSizes( mPanner1Sep );
        mPanner1->setResizeMode( mFolderTree, QSplitter::KeepSize );
        mPanner2->setSizes( mPanner2Sep );
        mPanner2->setResizeMode( mHeaders, QSplitter::KeepSize );
        mPanner2->setResizeMode( mMimePartTree, QSplitter::KeepSize );
        break;
    case 2:
        mHeaders->reparent( mPanner3, 0, QPoint( 0, 0 ) );
        mMsgView->reparent( mPanner3, 0, QPoint( 0, 0 ) );
        mPanner3->moveToLast( mMsgView );
        mFolderTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mMimePartTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mPanner2->moveToLast( mMimePartTree );
        mPanner1->setSizes( mPanner1Sep );
        mPanner2->setSizes( mPanner2Sep );
        mPanner3->setSizes( mPanner2Sep );
        mPanner2->setResizeMode( mMimePartTree, QSplitter::KeepSize );
        mPanner3->setResizeMode( mHeaders, QSplitter::KeepSize );
        break;
    case 3:
        mFolderTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mPanner2->moveToFirst( mFolderTree );
        mHeaders->reparent( mPanner3, 0, QPoint( 0, 0 ) );
        mMimePartTree->reparent( mPanner3, 0, QPoint( 0, 0 ) );
        mPanner3->moveToLast( mMimePartTree );
        mMsgView->reparent( mPanner1, 0, QPoint( 0, 0 ) );
        mPanner1->moveToLast( mMsgView );
        mPanner1->setSizes( mPanner1Sep );
        mPanner2->setSizes( mPanner2Sep );
        mPanner3->setSizes( mPanner2Sep );
        mPanner1->setResizeMode( mPanner2, QSplitter::KeepSize );
        mPanner2->setResizeMode( mFolderTree, QSplitter::KeepSize );
        mPanner3->setResizeMode( mMimePartTree, QSplitter::KeepSize );
        break;
    case 4:
        mFolderTree->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mHeaders->reparent( mPanner2, 0, QPoint( 0, 0 ) );
        mPanner2->moveToLast( mHeaders );
        mMimePartTree->reparent( mPanner1, 0, QPoint( 0, 0 ) );
        mPanner1->moveToFirst( mPanner2 );
        mMsgView->reparent( mPanner1, 0, QPoint( 0, 0 ) );
        mPanner1->moveToLast( mMsgView );
        mPanner1->setSizes( mPanner1Sep );
        mPanner2->setSizes( mPanner2Sep );
        mPanner1->setResizeMode( mPanner2, QSplitter::KeepSize );
        mPanner1->setResizeMode( mMimePartTree, QSplitter::KeepSize );
        mPanner2->setResizeMode( mFolderTree, QSplitter::KeepSize );
        break;
    }

    if( 1 < mShowMIMETreeMode )
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
  int statusWidth = statusBar()->width() - littleProgress->width()
    - fontMetrics().maxWidth();

  while (!text.isEmpty() && fontMetrics().width( text ) >= statusWidth)
    text.truncate( text.length() - 1);

  // ### FIXME: We should disable richtext/HTML (to avoid possible denial of service attacks),
  // but this code would double the size of the satus bar if the user hovers
  // over an <foo@bar.com>-style email address :-(
//  text.replace(QRegExp("&"), "&amp;");
//  text.replace(QRegExp("<"), "&lt;");
//  text.replace(QRegExp(">"), "&gt;");

  statusBar()->changeItem(text, mMessageStatusId);
}


//-----------------------------------------------------------------------------
void KMMainWin::hide()
{
  KMMainWinInherited::hide();
}


//-----------------------------------------------------------------------------
void KMMainWin::show()
{
  if( mPanner1 ) mPanner1->setSizes( mPanner1Sep );
  if( mPanner2 ) mPanner2->setSizes( mPanner2Sep );
  if( mPanner3 ) mPanner3->setSizes( mPanner3Sep );
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

  d = new KMMainWin(0);
  d->show();
  d->resize(d->size());
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
    if ( mFolder && mFolder->needsRepainting() )
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

  KNotifyClient::event("new-mail-arrived", i18n("New mail arrived"));
  if (mBeepOnNew) {
    KNotifyClient::beep();
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

      win = new KMComposeWin(msg, mFolder->identity());
  } else {
      msg->initHeader();
      win = new KMComposeWin(msg);
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
      win = new KMComposeWin(msg, mFolder->identity());
  } else {
      msg->initHeader();
      win = new KMComposeWin(msg);
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
  KConfig           *config = KMKernel::config();
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
    bool isTrash = kernel->folderIsTrash(mFolder);
    QString title = (isTrash) ? i18n("Empty Trash") : i18n("Move to Trash");
    QString text = (isTrash) ?
      i18n("Are you sure you want to empty the trash folder?") :
      i18n("Are you sure you want to move all messages from "
           "folder \"%1\" to the trash?").arg(mFolder->label());

    if (KMessageBox::warningContinueCancel(this, text, title, title)
      != KMessageBox::Continue) return;
  }

  if (mFolder->protocol() == "imap")
  {
    slotMarkAll();
    slotTrashMsg();
    return;
  }

  mMsgView->clearCache();

  kernel->kbp()->busy();

  // begin of critical part
  // from here to "end..." no signal may change to another mFolder, otherwise
  // the wrong folder will be truncated in expunge (dnaber, 1999-08-29)
  mFolder->open();
  mHeaders->setFolder(0);
  mMsgView->clear();

  if (mFolder != kernel->trashFolder())
  {
    // FIXME: If we run out of disk space mail may be lost rather
    // than moved into the trash -sanders
    while ((msg = mFolder->take(0)) != 0) {
      kernel->trashFolder()->addMsg(msg);
      kernel->trashFolder()->unGetMsg(kernel->trashFolder()->count()-1);
    }
  }

  mFolder->close();
  mFolder->expunge();
  // end of critical
  if (mFolder != kernel->trashFolder())
    statusMsg(i18n("Moved all messages to the trash"));

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

  str = i18n("Are you sure you want to delete the folder "
	     "\"%1\" and all its subfolders, discarding their contents?")
			     .arg(mFolder->label());

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
        acct->setFolder(kernel->inboxFolder());
        KMessageBox::information(this,
            i18n("The destination folder of the account '%1' was restored to the inbox.").arg(acct->name()));
      }
    }
    if (mFolder->protocol() == "imap")
      static_cast<KMFolderImap*>(mFolder)->removeOnServer();
    else if (mFolder->protocol() == "cachedimap")
      kernel->imapFolderMgr()->remove(mFolder);
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
  KConfig    *config = KMKernel::config();
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
  mMsgView->update( true );
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
  if (!mHeaders->currentMsg())
    return;
  KMCommand *command = new KMPrintCommand( this, mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotReplyToMsg()
{
  if (!mHeaders->currentMsg())
    return;

  KMCommand *command = new KMReplyToCommand( this, mHeaders->currentMsg(),
                                            mMsgView->copyText() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotNoQuoteReplyToMsg()
{
  if (!mHeaders->currentMsg())
    return;

  KMCommand *command = new KMNoQuoteReplyToCommand( this,
                                                    mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotReplyAllToMsg()
{
  if (!mHeaders->currentMsg())
	return;
  KMCommand *command = new KMReplyToAllCommand( this,  mHeaders->currentMsg(),
                                               mMsgView->copyText() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotReplyListToMsg()
{
  if (!mHeaders->currentMsg())
	return;
  KMCommand *command = new KMReplyListCommand( this, mHeaders->currentMsg(),
                                              mMsgView->copyText() );
  command->start();
}


void KMMainWin::slotForward() {
  // ### FIXME: remember the last used subaction and use that
  // currently, we hard-code "as attachment".
  slotForwardAttachedMsg();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotForwardMsg()
{
  if (!mHeaders->selectedMsgs())
    return;

  KMCommand *command = new KMForwardCommand( this, *mHeaders->selectedMsgs(),
                                             mFolder );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotForwardAttachedMsg()
{
  if (!mHeaders->selectedMsgs())
    return;

  KMCommand *command = new KMForwardAttachedCommand( this,
    *mHeaders->selectedMsgs(), mFolder );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotRedirectMsg()
{
  if (!mHeaders->currentMsg())
    return;

  KMCommand *command = new KMRedirectCommand( this, mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotBounceMsg()
{
  if (!mHeaders->currentMsg())
    return;

  KMCommand *command = new KMBounceCommand( this, mHeaders->currentMsg() );
  command->start();
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
  if (!mHeaders->currentMsg())
    return;

  KMCommand *command = new KMEditMsgCommand( this, mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotResendMsg()
{
  mHeaders->resendMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotTrashMsg()
{
  mHeaders->deleteMsg();
  updateMessageActions();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotDeleteMsg()
{
  mHeaders->moveMsgToFolder(0);
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
  if (!mHeaders->currentMsg())
    return;
  KMCommand *command = new KMShowMsgSrcCommand( this, mHeaders->currentMsg(),
    mCodec, mMsgView->isfixedFont() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotToggleFixedFont()
{
  KMCommand *command = new KMToggleFixedCommand( mMsgView );
  command->start();
}

//-----------------------------------------------------------------------------
void KMMainWin::slotToggleUnreadColumn()
{
  mFolderTree->toggleColumn(KMFolderTree::unread);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotToggleTotalColumn()
{
  mFolderTree->toggleColumn(KMFolderTree::total, true);
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

void KMMainWin::slotEditVacation() {
  if ( mVacation )
    return;

  mVacation = new Vacation( this );
  if ( mVacation->isUsable() )
    connect( mVacation, SIGNAL(result(bool)), mVacation, SLOT(deleteLater()) );
  else {
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
  KMMessage *msg = mHeaders->currentMsg();
  if (!msg)
    return;
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( this,
    *mHeaders->selectedMsgs(), mFolder );

  if (saveCommand->url().isEmpty())
    delete saveCommand;
  else
    saveCommand->start();
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
  else {
    style = KMReaderWin::HeaderStyle((int)style+1);
    mMsgView->setHeaderStyle( KMReaderWin::HeaderStyle(style) );
  }
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

void KMMainWin::slotHideAttachments() {
  mMsgView->setAttachmentStyle( KMReaderWin::HideAttmnt );
}

void KMMainWin::slotCycleAttachmentStyles() {
  KMReaderWin::AttachmentStyle style = mMsgView->attachmentStyle();
  if ( style == KMReaderWin::HideAttmnt ) // last, go to top again:
    mMsgView->setAttachmentStyle( KMReaderWin::IconicAttmnt );
  else {
    style = KMReaderWin::AttachmentStyle((int)style+1);
    mMsgView->setAttachmentStyle( KMReaderWin::AttachmentStyle(style) );
  }
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

  mMsgView->clear(true);
  if( !aFolder || aFolder->noContent() ||
      aFolder->count() == 0 )
  {
    if( mMimePartTree && (mShowMIMETreeMode != 2) )
      mMimePartTree->hide();
  } else {
    if( mMimePartTree && (1 < mShowMIMETreeMode) )
      mMimePartTree->show();
  }
  if( !mFolder ) {
    mMsgView->enableMsgDisplay();
    mMsgView->clear(true);
    if( mHeaders )
      mHeaders->show();
  }

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
  if (msg && curMsgSerNum != msg->getMsgSerNum())
    mHeaders->setCurrentMsg(index);

  return msg;
}

//-----------------------------------------------------------------------------
void KMMainWin::slotMsgSelected(KMMessage *msg)
{
  if (msg && msg->parent() && (msg->parent()->protocol() == "imap") &&
      !msg->isComplete())
  {
    mMsgView->clear();
    KMImapJob *job = new KMImapJob(msg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(slotUpdateImapMessage(KMMessage*)));
  } else {
    mMsgView->setMsg(msg);
  }
  // reset HTML override to the folder setting
  mMsgView->setHtmlOverride(mFolderHtmlPref);
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
void KMMainWin::slotReplaceMsgByUnencryptedVersion()
{
  kdDebug(5006) << "KMMainWin::slotReplaceMsgByUnencryptedVersion()" << endl;
  KMMessage* oldMsg = mHeaders->getMsg(-1);
  if( oldMsg ) {
    kdDebug(5006) << "KMMainWin  -  old message found" << endl;
    if( oldMsg->hasUnencryptedMsg() ) {
      kdDebug(5006) << "KMMainWin  -  extra unencrypted message found" << endl;
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
      kdDebug(5006) << "KMMainWin  -  copying unencrypted message to same folder" << endl;
      mHeaders->copyMsgToFolder(mFolder, -1, newMsg);
      // delete the encrypted message - this will also delete newMsg
      kdDebug(5006) << "KMMainWin  -  deleting encrypted message" << endl;
      mHeaders->deleteMsg();
      kdDebug(5006) << "KMMainWin  -  updating message actions" << endl;
      updateMessageActions();

      // find and select and show the new message
      int idx = mFolder->find( newMsgIdMD5 );
      if( -1 != idx ) {
        mHeaders->setCurrentMsg( idx );
        mMsgView->setMsg( mHeaders->currentMsg() );
      } else {
        kdDebug(5006) << "KMMainWin  -  SORRY, could not store unencrypted message!" << endl;
      }

      kdDebug(5006) << "KMMainWin  -  done." << endl;
    } else
      kdDebug(5006) << "KMMainWin  -  NO EXTRA UNENCRYPTED MESSAGE FOUND" << endl;
  } else
    kdDebug(5006) << "KMMainWin  -  PANIC: NO OLD MESSAGE FOUND" << endl;
}



//-----------------------------------------------------------------------------
void KMMainWin::slotUpdateImapMessage(KMMessage *msg)
{
  if (msg && ((KMMsgBase*)msg)->isMessage())
    mMsgView->setMsg(msg, TRUE);
  else // force an update of the folder
    static_cast<KMFolderImap*>(mFolder)->getFolder(true);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSubjectFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (msg) {
    KMCommand *command = new KMFilterCommand( "Subject", msg->subject() );
    command->start();
  }
}

void KMMainWin::slotMailingListFilter()
{
  if (mHeaders->currentMsg()) {
    KMCommand *command = new KMMailingListFilterCommand( this,
      mHeaders->currentMsg() );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMMainWin::slotFromFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (msg) {
    KMCommand *command = new KMFilterCommand( "From",  msg->from() );
    command->start();
  }
}

//-----------------------------------------------------------------------------
void KMMainWin::slotToFilter()
{
  KMMessage *msg = mHeaders->currentMsg();
  if (msg) {
    KMCommand *command = new KMFilterCommand( "To",  msg->to() );
    command->start();
  }
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
void KMMainWin::slotSetMsgStatusForwarded()
{
  mHeaders->setMsgStatus(KMMsgStatusForwarded);
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
void KMMainWin::slotSetThreadStatusForwarded()
{
  mHeaders->setThreadStatus(KMMsgStatusForwarded);
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
void KMMainWin::slotNextImportantMessage() {
  //mHeaders->nextImportantMessage();
}
void KMMainWin::slotPrevMessage()       { mHeaders->prevMessage(); }
void KMMainWin::slotPrevUnreadMessage() { mHeaders->prevUnreadMessage(); }
void KMMainWin::slotPrevImportantMessage() {
  //mHeaders->prevImportantMessage();
}

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

  assert(msg != 0);
  KMReaderWin *win;

  win = new KMReaderWin;
  win->setShowCompleteMessage(true);
  win->setAutoDelete(true);
  win->setHtmlOverride(mFolderHtmlPref);
  KMMessage *newMessage = new KMMessage();
  newMessage->fromString(msg->asString());
  newMessage->setStatus(msg->status());
  showMsg(win, newMessage);
}


//called from reader win. message must be deleted on close
void KMMainWin::slotAtmMsg(KMMessage *msg)
{
  KMReaderWin *win;
  assert(msg != 0);
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
  KMCommand *command = new KMUrlClickedCommand( aUrl, mFolder, mMsgView,
                                               mFolderHtmlPref, this );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoCompose()
{
  KMCommand *command = new KMMailtoComposeCommand( mUrlCurrent,
                                                  mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoReply()
{
  KMCommand *command = new KMMailtoReplyCommand( this, mUrlCurrent,
    mHeaders->currentMsg(), mMsgView->copyText() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoForward()
{
  KMCommand *command = new KMMailtoForwardCommand( this, mUrlCurrent,
                                                  mHeaders->currentMsg() );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoAddAddrBook()
{
  KMCommand *command = new KMMailtoAddAddrBookCommand( mUrlCurrent, this );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoOpenAddrBook()
{
  KMCommand *command = new KMMailtoOpenAddrBookCommand( mUrlCurrent, this );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlCopy()
{
  KMCommand *command = new KMUrlCopyCommand( mUrlCurrent, this );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlOpen()
{
  KMCommand *command = new KMUrlOpenCommand( mUrlCurrent, mMsgView );
  command->start();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlSave()
{
  KMCommand *command = new KMUrlSaveCommand( mUrlCurrent, this );
  command->start();
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
      menu->insertItem(i18n("Add to Address Book"), this,
		       SLOT(slotMailtoAddAddrBook()));
      menu->insertItem(i18n("Open in Address Book"), this,
		       SLOT(slotMailtoOpenAddrBook()));
      menu->insertItem(i18n("Copy to Clipboard"), this,
		       SLOT(slotUrlCopy()));
    }
    else
    {
      // popup on a not-mailto URL
      menu->insertItem(i18n("Open URL"), this,
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
	 action("message_forward")->plug(menu);
         bounceAction->plug(menu);
     }
     menu->insertSeparator();
     if ( !out_folder ) {
         filterMenu->plug( menu );
         statusMenu->plug( menu );
	 threadStatusMenu->plug( menu );
     }

     copyActionMenu->plug( menu );
     moveActionMenu->plug( menu );

     menu->insertSeparator();
     toggleFixFontAction->plug(menu);
     viewSourceAction->plug(menu);

     menu->insertSeparator();
     printAction->plug(menu);
     saveAsAction->plug(menu);
     menu->insertSeparator();
     trashAction->plug(menu);
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
  case KMReaderWin::HideAttmnt:
    actionName = "view_attachments_hide"; break;
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
  (void) new KAction( i18n("New &Window"), "window_new", 0,
		      this, SLOT(slotNewMailReader()),
		      actionCollection(), "new_mail_client" );

  saveAsAction = new KAction( i18n("Save &As..."), "filesave",
    KStdAccel::shortcut(KStdAccel::Save),
    this, SLOT(slotSaveMsg()), actionCollection(), "file_save_as" );

  printAction = KStdAction::print (this, SLOT(slotPrintMsg()), actionCollection());

  (void) new KAction( i18n("&Compact All Folders"), 0,
		      this, SLOT(slotCompactAll()),
		      actionCollection(), "compact_all_folders" );

  (void) new KAction( i18n("&Expire All Folders"), 0,
		      this, SLOT(slotExpireAll()),
		      actionCollection(), "expire_all_folders" );

  (void) new KAction( i18n("Check &Mail"), "mail_get", CTRL+Key_L,
		      this, SLOT(slotCheckMail()),
		      actionCollection(), "check_mail" );

  KActionMenu *actActionMenu = new
    KActionMenu( i18n("Check Mail &In"), "mail_get", actionCollection(),
				   	"check_mail_in" );
  actActionMenu->setDelayed(true); //needed for checking "all accounts"

  connect(actActionMenu,SIGNAL(activated()),this,SLOT(slotCheckMail()));

  actMenu = actActionMenu->popupMenu();
  connect(actMenu,SIGNAL(activated(int)),this,SLOT(slotCheckOneAccount(int)));
  connect(actMenu,SIGNAL(aboutToShow()),this,SLOT(getAccountMenu()));

  (void) new KAction( i18n("&Send Queued Messages"), "mail_send", 0, this,
		     SLOT(slotSendQueued()), actionCollection(), "send_queued");

  KStdAction::quit( this, SLOT(slotQuit()), actionCollection());

  //----- Tools menu
  (void) new KAction( i18n("&Address Book"), "contents", 0, this,
		      SLOT(slotAddrBook()), actionCollection(), "addressbook" );

  (void) new KAction( i18n("&Import..."), "fileopen", 0, this,
		      SLOT(slotImport()), actionCollection(), "import" );

  (void) new KAction( i18n("Edit \"Out of Office\" Replies..."),
		      "configure", 0, this, SLOT(slotEditVacation()),
		      actionCollection(), "tools_edit_vacation" );

  //----- Edit Menu
  KStdAction::undo( this, SLOT(slotUndo()), actionCollection(), "edit_undo");

  KStdAction::copy( this, SLOT(slotCopyText()), actionCollection(), "copy");

  trashAction = new KAction( KGuiItem( i18n("&Move to Trash"), "edittrash",
                                       i18n("Move message to trashcan") ),
                             "D;Delete", this, SLOT(slotTrashMsg()),
                             actionCollection(), "move_to_trash" );

  deleteAction = new KAction( i18n("&Delete"), "editdelete", SHIFT+Key_Delete, this,
                              SLOT(slotDeleteMsg()), actionCollection(), "delete_message" );

  (void) new KAction( i18n("&Find Messages..."), "mail_find", Key_S, this,
		      SLOT(slotSearch()), actionCollection(), "search_messages" );

  (void) new KAction( i18n("&Find in Message..."), "find", KStdAccel::shortcut(KStdAccel::Find), this,
		      SLOT(slotFind()), actionCollection(), "find_in_messages" );

  (void) new KAction( i18n("Select &All Messages"), Key_K, this,
		      SLOT(slotMarkAll()), actionCollection(), "mark_all_messages" );

  (void) new KAction( i18n("Select Message &Text"), KStdAccel::shortcut(KStdAccel::SelectAll), this,
		      SLOT(slotSelectText()), actionCollection(), "mark_all_text" );

  //----- Folder Menu
  (void) new KAction( i18n("&New Folder..."), "folder_new", 0, this,
		      SLOT(slotAddFolder()), actionCollection(), "new_folder" );

  modifyFolderAction = new KAction( i18n("&Properties..."), "configure", 0, this,
		      SLOT(slotModifyFolder()), actionCollection(), "modify" );

  markAllAsReadAction = new KAction( i18n("Mark All Messages as &Read"), "goto", 0, this,
		      SLOT(slotMarkAllAsRead()), actionCollection(), "mark_all_as_read" );

  expireFolderAction = new KAction(i18n("&Expire"), 0, this, SLOT(slotExpireFolder()),
				   actionCollection(), "expire");

  compactFolderAction = new KAction( i18n("&Compact"), 0, this,
		      SLOT(slotCompactFolder()), actionCollection(), "compact" );

  emptyFolderAction = new KAction( i18n("&Move All Messages to Trash"),
                                   "edittrash", 0, this,
		      SLOT(slotEmptyFolder()), actionCollection(), "empty" );

  removeFolderAction = new KAction( i18n("&Delete Folder"), "editdelete", 0, this,
		      SLOT(slotRemoveFolder()), actionCollection(), "delete_folder" );

  preferHtmlAction = new KToggleAction( i18n("Prefer &HTML to Plain Text"), 0, this,
		      SLOT(slotOverrideHtml()), actionCollection(), "prefer_html" );

  threadMessagesAction = new KToggleAction( i18n("&Thread Messages"), 0, this,
		      SLOT(slotOverrideThread()), actionCollection(), "thread_messages" );

  //----- Message Menu
  (void) new KAction( i18n("&New Message..."), "mail_new", KStdAccel::shortcut(KStdAccel::New), this,
		      SLOT(slotCompose()), actionCollection(), "new_message" );

  (void) new KAction( i18n("New Message t&o Mailing-List..."), "mail_post_to", 0, this,
		      SLOT(slotPostToML()), actionCollection(), "post_message" );

  replyAction = new KAction( i18n("&Reply..."), "mail_reply", Key_R, this,
		      SLOT(slotReplyToMsg()), actionCollection(), "reply" );

  noQuoteReplyAction = new KAction( i18n("Reply Without &Quote..."), SHIFT+Key_R,
    this, SLOT(slotNoQuoteReplyToMsg()), actionCollection(), "noquotereply" );

  replyAllAction = new KAction( i18n("Reply to &All..."), "mail_replyall",
     Key_A, this, SLOT(slotReplyAllToMsg()), actionCollection(), "reply_all" );

  replyListAction = new KAction( i18n("Reply to Mailing-&List..."), "mail_replylist",
     Key_L, this, SLOT(slotReplyListToMsg()), actionCollection(), "reply_list" );

  KActionMenu * forwardMenu =
    new KActionMenu( i18n("Message->","&Forward"), "mail_forward",
		     actionCollection(), "message_forward" );
  connect( forwardMenu, SIGNAL(activated()), this, SLOT(slotForward()) );

  forwardAction = new KAction( i18n("&Inline..."), "mail_forward", SHIFT+Key_F, this,
			       SLOT(slotForwardMsg()),
			       actionCollection(), "message_forward_inline" );
  forwardMenu->insert( forwardAction );

  forwardAttachedAction = new KAction( i18n("Message->Forward->","As &Attachment..."),
				       "mail_forward", Key_F, this,
				       SLOT(slotForwardAttachedMsg()),
				       actionCollection(), "message_forward_as_attachment" );
  forwardMenu->insert( forwardAttachedAction );

  redirectAction = new KAction( i18n("Message->Forward->","&Redirect..."),
				Key_E, this, SLOT(slotRedirectMsg()),
				actionCollection(), "message_forward_redirect" );
  forwardMenu->insert( redirectAction );

  bounceAction = new KAction( i18n("&Bounce..."), 0, this,
		      SLOT(slotBounceMsg()), actionCollection(), "bounce" );

  sendAgainAction = new KAction( i18n("Send A&gain..."), 0, this,
		      SLOT(slotResendMsg()), actionCollection(), "send_again" );

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

  editAction = new KAction( i18n("&Edit Message"), "edit", Key_T, this,
                            SLOT(slotEditMsg()), actionCollection(), "edit" );

  //----- Create filter submenu
  filterMenu = new KActionMenu( i18n("&Create Filter"), actionCollection(), "create_filter" );

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

  //----- "Mark Message" submenu
  statusMenu = new KActionMenu ( i18n( "Mar&k Message" ),
                                 actionCollection(), "set_status" );

  statusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &New"), "kmmsgnew",
                                          i18n("Mark selected messages as new")),
                                 0, this, SLOT(slotSetMsgStatusNew()),
                                 actionCollection(), "status_new" ));

  statusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Unread"), "kmmsgunseen",
                                          i18n("Mark selected messages as unread")),
                                 0, this, SLOT(slotSetMsgStatusUnread()),
                                 actionCollection(), "status_unread"));

  statusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Read"), "kmmsgold",
                                          i18n("Mark selected messages as read")),
                                 0, this, SLOT(slotSetMsgStatusRead()),
                                 actionCollection(), "status_read"));

  statusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as R&eplied"), "kmmsgreplied",
                                          i18n("Mark selected messages as replied")),
                                 0, this, SLOT(slotSetMsgStatusReplied()),
                                 actionCollection(), "status_replied"));

  statusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Forwarded"), "kmmsgforwarded",
                                          i18n("Mark selected messages as forwarded")),
                                 0, this, SLOT(slotSetMsgStatusForwarded()),
                                 actionCollection(), "status_forwarded"));

  statusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Queued"), "kmmsgqueued",
                                          i18n("Mark selected messages as queued")),
                                 0, this, SLOT(slotSetMsgStatusQueued()),
                                 actionCollection(), "status_queued"));

  statusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Sent"), "kmmsgsent",
                                          i18n("Mark selected messages as sent")),
                                 0, this, SLOT(slotSetMsgStatusSent()),
                                 actionCollection(), "status_sent"));

  statusMenu->insert(new KAction(KGuiItem(i18n("Mark Message as &Important"), "kmmsgflag",
                                          i18n("Mark selected messages as important")),
                                 0, this, SLOT(slotSetMsgStatusFlag()),
                                 actionCollection(), "status_flag"));

  //----- "Mark Thread" submenu
  threadStatusMenu = new KActionMenu ( i18n( "Mark &Thread" ),
                                       actionCollection(), "thread_status" );

  threadStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Thread as &New"), "kmmsgnew",
                                                i18n("Mark all messages in the selected thread as new")),
                                       0, this, SLOT(slotSetThreadStatusNew()),
                                       actionCollection(), "thread_new"));

  threadStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Thread as &Unread"), "kmmsgunseen",
                                                i18n("Mark all messages in the selected thread as unread")),
                                       0, this, SLOT(slotSetThreadStatusUnread()),
                                       actionCollection(), "thread_unread"));

  threadStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Thread as &Read"), "kmmsgold",
                                                i18n("Mark all messages in the selected thread as read")),
                                       0, this, SLOT(slotSetThreadStatusRead()),
                                       actionCollection(), "thread_read"));

  threadStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Thread as R&eplied"), "kmmsgreplied",
                                                i18n("Mark all messages in the selected thread as replied")),
                                       0, this, SLOT(slotSetThreadStatusReplied()),
                                       actionCollection(), "thread_replied"));

  threadStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Thread as &Forwarded"), "kmmsgforwarded",
                                                i18n("Mark all messages in the selected thread as forwarded")),
                                       0, this, SLOT(slotSetThreadStatusForwarded()),
                                       actionCollection(), "thread_forwarded"));

  threadStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Thread as &Queued"), "kmmsgqueued",
                                                i18n("Mark all messages in the selected thread as queued")),
                                       0, this, SLOT(slotSetThreadStatusQueued()),
                                       actionCollection(), "thread_queued"));

  threadStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Thread as &Sent"), "kmmsgsent",
                                                i18n("Mark all messages in the selected thread as sent")),
                                       0, this, SLOT(slotSetThreadStatusSent()),
                                       actionCollection(), "thread_sent"));

  threadStatusMenu->insert(new KAction(KGuiItem(i18n("Mark Thread as &Important"), "kmmsgflag",
                                                i18n("Mark all messages in the selected thread as important")),
                                       0, this, SLOT(slotSetThreadStatusFlag()),
                                       actionCollection(), "thread_flag"));



  moveActionMenu = new KActionMenu( i18n("&Move To" ),
                                    actionCollection(), "move_to" );

  copyActionMenu = new KActionMenu( i18n("&Copy To" ),
                                    actionCollection(), "copy_to" );

  (void) new KAction( i18n("Appl&y Filters"), "filter", CTRL+Key_J, this,
		      SLOT(slotApplyFilters()), actionCollection(), "apply_filters" );


  //----- View Menu
  KRadioAction * raction = 0;

  // "Headers" submenu:
  KActionMenu * headerMenu =
    new KActionMenu( i18n("View->", "&Headers"),
		     actionCollection(), "view_headers" );
  headerMenu->setToolTip( i18n("Choose display style of message headers") );

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
  raction = actionForHeaderStyle( mMsgView->headerStyle() );
  if ( raction )
    raction->setChecked( true );

  // "Attachments" submenu:
  KActionMenu * attachmentMenu =
    new KActionMenu( i18n("View->", "&Attachments"),
		     actionCollection(), "view_attachments" );
  connect( attachmentMenu, SIGNAL(activated()),
	   SLOT(slotCycleAttachmentStyles()) );

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
  raction = actionForAttachmentStyle( mMsgView->attachmentStyle() );
  if ( raction )
    raction->setChecked( true );

  unreadColumnToggle = new KToggleAction( i18n("View->", "&Unread Column"), 0, this,
			       SLOT(slotToggleUnreadColumn()),
			       actionCollection(), "view_columns_unread" );
  unreadColumnToggle->setToolTip( i18n("Toggle display of column showing the "
                                       "number of unread messages in folders.") );
  unreadColumnToggle->setChecked( mFolderTree->isUnreadActive() );

  totalColumnToggle = new KToggleAction( i18n("View->", "&Total Column"), 0, this,
			       SLOT(slotToggleTotalColumn()),
			       actionCollection(), "view_columns_total" );
  totalColumnToggle->setToolTip( i18n("Toggle display of column showing the "
                                      "total number of messages in folders.") );
  totalColumnToggle->setChecked( mFolderTree->isTotalActive() );

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

  toggleFixFontAction = new KToggleAction( i18n("Use Fi&xed Font"),
			Key_X, this, SLOT(slotToggleFixedFont()),
			actionCollection(), "toggle_fixedfont" );

  viewSourceAction = new KAction( i18n("&View Source"), Key_V, this,
		      SLOT(slotShowMsgSrc()), actionCollection(), "view_source" );


  //----- Go Menu
  new KAction( KGuiItem( i18n("&Next Message"), QString::null,
                         i18n("Go to the next message") ),
                         "N;Right", this, SLOT(slotNextMessage()),
                         actionCollection(), "go_next_message" );

  new KAction( KGuiItem( i18n("Next &Unread Message"), "next",
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

  new KAction( KGuiItem( i18n("Previous Unread &Message"), "previous",
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
  toolbarAction = KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
    actionCollection());
  statusbarAction = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
    actionCollection());


  KStdAction::keyBindings(this, SLOT(slotEditKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
#if KDE_VERSION >= 306
  KStdAction::configureNotifications(this, SLOT(slotEditNotifications()), actionCollection());
#endif
  KStdAction::preferences(kernel, SLOT(slotShowConfigurationDialog()), actionCollection());
#if KDE_VERSION >= 305 // KDE 3.1
  KStdAction::tipOfDay( this, SLOT( slotShowTip() ), actionCollection() );
#else
  (void) new KAction( KGuiItem( i18n("Tip of the &Day"), "idea",
				i18n("Show \"Tip of the Day\"") ),
		      0, this, SLOT(slotShowTip()),
		      actionCollection(), "help_show_tip" );
#endif


  (void) new KAction( i18n("Configure &Filters..."), 0, this,
 		      SLOT(slotFilter()), actionCollection(), "filter" );
  (void) new KAction( i18n("Configure &POP Filters..."), 0, this,
 		      SLOT(slotPopFilter()), actionCollection(), "popFilter" );

  (void) new KAction( KGuiItem( i18n("KMail &Introduction"), 0,
				i18n("Display KMail's Welcome Page") ),
		      0, this, SLOT(slotIntro()),
		      actionCollection(), "help_kmail_welcomepage" );

  createGUI( "kmmainwin.rc", false );

  connect( guiFactory()->container("folder", this),
	   SIGNAL( aboutToShow() ), this, SLOT( updateFolderMenu() ));

  connect( guiFactory()->container("message", this),
	   SIGNAL( aboutToShow() ), this, SLOT( updateMessageMenu() ));
  // contains "Create Filter" actions.
  connect( guiFactory()->container("tools", this),
	   SIGNAL( aboutToShow() ), this, SLOT( updateMessageMenu() ));
  // contains "View source" action.
  connect( guiFactory()->container("view", this),
	   SIGNAL( aboutToShow() ), this, SLOT( updateMessageMenu() ));


  conserveMemory();

  menutimer = new QTimer( this, "menutimer" );
  connect( menutimer, SIGNAL( timeout() ), SLOT( updateMessageActions() ) );
  connect( kernel->undoStack(),
      SIGNAL( undoStackChanged() ), this, SLOT( slotUpdateUndo() ));  

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
  saveMainWindowSettings(KMKernel::config(), "MainWindow");
  KEditToolbar dlg(actionCollection(), "kmmainwin.rc");

  connect( &dlg, SIGNAL(newToolbarConfig()),
	   SLOT(slotUpdateToolbars()) );

  dlg.exec();
}

void KMMainWin::slotEditNotifications()
{
#if KDE_VERSION >= 306
  KNotifyDialog::configure(this);
#endif
}

void KMMainWin::slotUpdateToolbars()
{
  createGUI("kmmainwin.rc");
  applyMainWindowSettings(KMKernel::config(), "MainWindow");
  toolbarAction->setChecked(!toolBar()->isHidden());
}

void KMMainWin::slotEditKeys()
{
  KKeyDialog::configure( actionCollection()
#if KDE_VERSION >= 306
			 , true /*allow on-letter shortcuts*/
#endif
			 );
}

//-----------------------------------------------------------------------------
void KMMainWin::setupStatusBar()
{
  littleProgress = new KMLittleProgressDlg( statusBar() );

  statusBar()->addWidget( littleProgress, 0 , true );
  mMessageStatusId = 1;
  statusBar()->insertItem(i18n(" Initializing..."), 1, 1 );
  statusBar()->setItemAlignment( 1, AlignLeft | AlignVCenter );
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

void KMMainWin::slotNextUnreadFolder() {
  if ( !mFolderTree ) return;
  mFolderTree->nextUnreadFolder();
}

void KMMainWin::slotPrevUnreadFolder() {
  if ( !mFolderTree ) return;
  mFolderTree->prevUnreadFolder();
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
void KMMainWin::updateMessageMenu()
{
    mMenuToFolder.clear();
    KMMenuCommand::folderToPopupMenu( true, this, &mMenuToFolder,
      moveActionMenu->popupMenu() );
    KMMenuCommand::folderToPopupMenu( false, this, &mMenuToFolder,
      copyActionMenu->popupMenu() );
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
    trashAction->setEnabled( mass_actions );
    deleteAction->setEnabled( mass_actions );
    action( "message_forward" )->setEnabled( mass_actions );
    forwardAction->setEnabled( mass_actions );
    forwardAttachedAction->setEnabled( mass_actions );

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

    bool mails = mFolder && mFolder->count();
    action( "go_next_message" )->setEnabled( mails );
    action( "go_next_unread_message" )->setEnabled( mails );
    action( "go_prev_message" )->setEnabled( mails );
    action( "go_prev_unread_message" )->setEnabled( mails );

    action( "send_queued" )->setEnabled( kernel->outboxFolder()->count() > 0 );
    action( "edit_undo" )->setEnabled( mHeaders->canUndo() );

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
}

//-----------------------------------------------------------------------------
void KMMainWin::slotUpdateUndo()
{
  action( "edit_undo" )->setEnabled( mHeaders->canUndo() );
}


//-----------------------------------------------------------------------------
void KMMainWin::updateFolderMenu()
{
  modifyFolderAction->setEnabled( mFolder ? !mFolder->noContent() : false );
  compactFolderAction->setEnabled( mFolder ? !mFolder->noContent() : false );
  emptyFolderAction->setEnabled( mFolder ? !mFolder->noContent() : false );
  emptyFolderAction->setText( (mFolder && kernel->folderIsTrash(mFolder))
    ? i18n("&Empty Trash") : i18n("&Move All Messages to Trash") );
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
  KConfig *config = KMKernel::config();

  // Make sure this is the last window.
  KMainWindow   *kmWin = 0;
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


//-----------------------------------------------------------------------------
void KMMainWin::slotIntro() {
  if ( !mFolderTree || !mMsgView ) return;

  if ( !mFolderTree->firstChild() ) return;
  if ( mFolderTree->currentItem() != mFolderTree->firstChild() ) // don't loop
    mFolderTree->doFolderSelected( mFolderTree->firstChild() );

  mMsgView->clear( true );
  // hide widgets that are in the way:
  if ( mHeaders && mWindowLayout < 3 )
    mHeaders->hide();
  if ( mMimePartTree && mShowMIMETreeMode > 0 &&
       mWindowLayout != 2 && mWindowLayout != 3 )
    mMimePartTree->hide();

  mMsgView->displayAboutPage();
}

void KMMainWin::slotShowTipOnStart() {
  KTipDialog::showTip( this );
}

void KMMainWin::slotShowTip() {
  KTipDialog::showTip( this, QString::null, true );
}

//-----------------------------------------------------------------------------
void KMMainWin::slotChangeCaption(QListViewItem * i)
{
  // set the caption to the current full path
  QStringList names;
  for ( QListViewItem * item = i ; item ; item = item->parent() )
    names.prepend( item->text(0) );
  setCaption( names.join("/") );
}

