// kmmainwin.cpp
//#define MALLOC_DEBUG 1

#include <kwin.h>
#include <kmfldsearch.h>

#ifdef MALLOC_DEBUG
#include <malloc.h>
#endif

#undef Unsorted // X headers...
#include <qdir.h>
#include <qclipboard.h>
#include <qaccel.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qsplitter.h>
#include <qtimer.h>
#include "mailinglist-magic.h"

#include <kconfig.h>
#include <kapp.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kstdaccel.h>

#include <krun.h>
#include <kopenwith.h>
#include <kpopupmenu.h>

#include <kmenubar.h>
#include <kmessagebox.h>

#include <kparts/browserextension.h>

#include <kaction.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kcharsets.h>
#include <kmimetype.h>
#include <knotifyclient.h>
#include <kdebug.h>

#include "configuredialog.h"
#include "kmbroadcaststatus.h"
#include "kmfoldermgr.h"
#include "kmfolderdia.h"
#include "kmaccount.h"
#include "kmacctimap.h"
#include "kmacctmgr.h"
#include "kbusyptr.h"
#include "kmfoldertree.h"
#include "kmheaders.h"
#include "kmreaderwin.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmcomposewin.h"
#include "kmglobal.h"
#include "kmfolderseldlg.h"
#include "kmfiltermgr.h"
#include "kmsender.h"
#include "kmaddrbookdlg.h"
#include "kmaddrbook.h"
#include "kmversion.h"
#include "kwin.h"

#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <klocale.h>

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
  mHorizPannerSep = new QValueList<int>;
  mVertPannerSep = new QValueList<int>;
  *mHorizPannerSep << 1 << 1;
  *mVertPannerSep << 1 << 1;

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
    mFolderTree->setCurrentItem(idx);
    mFolderTree->setSelected(idx,TRUE);
  }

  connect(kernel->msgSender(), SIGNAL(statusMsg(const QString&)),
	  SLOT(statusMsg(const QString&)));
  connect(kernel->acctMgr(), SIGNAL( checkedMail(bool)),
          SLOT( slotMailChecked(bool)));

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
}


//-----------------------------------------------------------------------------
void KMMainWin::readPreConfig(void)
{
  KConfig *config = kapp->config();
  QString str;

  { // area for config group "Geometry"
    KConfigGroupSaver saver(config, "Geometry");
    mLongFolderList = config->readBoolEntry("longFolderList", false);
  }

  KConfigGroupSaver saver(config, "General");
  mEncodingStr = config->readEntry("encoding", "");
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
  bool oldLongFolderList=false;
  QString str;
  QSize siz;

  if (mStartupDone)
  {
    writeConfig();
    oldLongFolderList = mLongFolderList;
    readPreConfig();
    mHeaders->refreshNestedState();
    if (oldLongFolderList != mLongFolderList)
    {
      hide();
      if (mHorizPanner->parent()==this) delete mHorizPanner;
      else delete mVertPanner;
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
    defaultSize = QSize(300,130);
    siz = config->readSizeEntry("Panners", &defaultSize);
    if (siz.isEmpty())
      siz = QSize(100,100); // why not defaultSize?
    (*mHorizPannerSep)[0] = siz.width();
    (*mVertPannerSep)[0] = siz.height();
    (*mHorizPannerSep)[1] = height() - siz.width();
    (*mVertPannerSep)[1] = width() - siz.height();
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

  // Re-activate panners
  if (mStartupDone)
  {
    if (oldLongFolderList != mLongFolderList)
      activatePanners();
    //    kernel->kbp()->busy(); //Crashes KMail
    mFolderTree->reload();
    QListViewItem *qlvi = mFolderTree->indexOfFolder(mFolder);
    if (qlvi!=0) {
      mFolderTree->setCurrentItem(qlvi);
      mFolderTree->setSelected(qlvi,TRUE);
    }

    // sanders - New code
    mHeaders->setFolder(mFolder);
    int aIdx = mHeaders->currentItemIndex();
    if (aIdx != -1)
      mMsgView->setMsg( mFolder->getMsg(aIdx), true );
    else
      mMsgView->setMsg( 0, true );
    updateMessageMenu();
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

    // Get those panner sizes right!
    config->writeEntry("Panners",
        QSize( mHorizPanner->sizes()[0] * r.height() /
	         (mHorizPanner->sizes()[0] + mHorizPanner->sizes()[1]),
	       mVertPanner->sizes()[0] * r.width() /
	         (mVertPanner->sizes()[0] + mVertPanner->sizes()[1])
	       ) );
  }

  KConfigGroupSaver saver(config, "General");
  config->writeEntry("encoding", mEncodingStr);
}


//-----------------------------------------------------------------------------
void KMMainWin::createWidgets(void)
{
  QSplitter *pnrMsgView, *pnrMsgList, *pnrFldList;
  QAccel *accel = new QAccel(this);

  // create panners
  if (mLongFolderList)
  {
    mVertPanner  = new QSplitter(Qt::Horizontal, this, "vertPanner" );
    mHorizPanner = new QSplitter(Qt::Vertical, mVertPanner, "horizPanner" );
    pnrFldList = mHorizPanner;
    pnrMsgView = mVertPanner;
    pnrMsgList = mVertPanner;
  }
  else
  {
    mHorizPanner = new QSplitter( Qt::Vertical, this, "horizPanner" );
    mVertPanner  = new QSplitter( Qt::Horizontal, mHorizPanner, "vertPanner" );
    pnrMsgView = mVertPanner;
    pnrMsgList = mHorizPanner;
    pnrFldList = mHorizPanner;
  }
  mVertPanner->setOpaqueResize(true);
  mHorizPanner->setOpaqueResize(true);

  // BUG -sanders these accelerators stop working after switching
  // between long/short folder layout
  // Probably need to disconnect them first.

  // create list of messages
  mHeaders = new KMHeaders(this, pnrMsgList, "headers");
  connect(mHeaders, SIGNAL(selected(KMMessage*)),
	  this, SLOT(slotMsgSelected(KMMessage*)));
  connect(mHeaders, SIGNAL(activated(KMMessage*)),
	  this, SLOT(slotMsgActivated(KMMessage*)));
  connect( mHeaders, SIGNAL( selectionChanged() ),
           SLOT( slotUpdateMessageMenu() ) );
  accel->connectItem(accel->insertItem(Key_Left),
		     mHeaders, SLOT(prevMessage()));
  accel->connectItem(accel->insertItem(Key_Right),
		     mHeaders, SLOT(nextMessage()));

  if (!mEncodingStr.isEmpty())
    mCodec = KMMsgBase::codecForName(mEncodingStr);
  else mCodec = 0;

  // create HTML reader widget
  mMsgView = new KMReaderWin(pnrMsgView);
  connect(mMsgView, SIGNAL(statusMsg(const QString&)),
	  this, SLOT(statusMsg(const QString&)));
  connect(mMsgView, SIGNAL(popupMenu(const KURL&,const QPoint&)),
	  this, SLOT(slotMsgPopup(const KURL&,const QPoint&)));
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
  accel->connectItem(accel->insertItem(Key_M),
                     this, SLOT(slotMoveMsg()));
  accel->connectItem(accel->insertItem(Key_C),
                     this, SLOT(slotCopyMsg()));
  accel->connectItem(accel->insertItem(Key_Delete),
		     this, SLOT(slotDeleteMsg()));

  // create list of folders
  mFolderTree  = new KMFolderTree(pnrFldList, "folderTree");
  connect(mFolderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderSelectedUnread(KMFolder*)),
	  this, SLOT(folderSelectedUnread(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderDrop(KMFolder*)),
	  this, SLOT(slotMoveMsgToFolder(KMFolder*)));
  connect(mFolderTree, SIGNAL(folderDropCopy(KMFolder*)),
          this, SLOT(slotCopyMsgToFolder(KMFolder*)));

  //Commands not worthy of menu items, but that deserve configurable keybindings
  KAction *nextUnreadFolderAction = new KAction(
    i18n("Next folder with unread messages"), CTRL+Key_Plus, mFolderTree,
    SLOT(nextUnreadFolder()), actionCollection(), "next_unread_folder");
  nextUnreadFolderAction->plugAccel( this->accel() );

  KAction *prevUnreadFolderAction = new KAction(
   i18n("Previous folder with unread messages"), CTRL+Key_Minus, mFolderTree,
   SLOT(prevUnreadFolder()), actionCollection(), "prev_unread_folder");
  prevUnreadFolderAction->plugAccel( this->accel() );

  KAction *nextFolderAction = new KAction(
   i18n("Focus on next folder"), CTRL+Key_Right, mFolderTree,
   SLOT(incCurrentFolder()), actionCollection(), "inc_current_folder");
  nextFolderAction->plugAccel( this->accel() );

  KAction *prevFolderAction = new KAction(
   i18n("Focus on previous folder"), CTRL+Key_Left, mFolderTree,
   SLOT(decCurrentFolder()), actionCollection(), "dec_current_folder");
  prevFolderAction->plugAccel( this->accel() );

  KAction *selectCurrentFolderAction = new KAction(
   i18n("Select folder with focus"), CTRL+Key_Space, mFolderTree,
   SLOT(selectCurrentFolder()), actionCollection(), "select_current_folder");
  selectCurrentFolderAction->plugAccel( this->accel() );

  KAction *readOnAction = new KAction( i18n( "Move to the next unread text" ),
                                       Key_Space, this,  SLOT( slotReadOn() ),
                                       actionCollection(), "read_on" );
  readOnAction->plugAccel( this->accel() );
  connect( kernel->outboxFolder(), SIGNAL( msgRemoved(int, QString) ),
           SLOT( slotUpdateMessageMenu() ) );
  connect( kernel->outboxFolder(), SIGNAL( msgAdded(int) ),
           SLOT( slotUpdateMessageMenu() ) );
}


//-----------------------------------------------------------------------------
void KMMainWin::activatePanners(void)
{
  // glue everything together
  if (mLongFolderList)
  {
    mHeaders->reparent( mHorizPanner, 0, QPoint( 0, 0 ) );
    mMsgView->reparent( mHorizPanner, 0, QPoint( 0, 0 ) );
    mFolderTree->reparent( mVertPanner, 0, QPoint( 0, 0 ) );
    mVertPanner->moveToFirst( mFolderTree );
    setCentralWidget(mVertPanner);
  }
  else
  {
    mFolderTree->reparent( mVertPanner, 0, QPoint( 0, 0 ) );
    mHeaders->reparent( mVertPanner, 0, QPoint( 0, 0 ) );
    mMsgView->reparent( mHorizPanner, 0, QPoint( 0, 0 ) );
    setCentralWidget(mHorizPanner);
  }
  mHorizPanner->setSizes( *mHorizPannerSep );
  mVertPanner->setSizes( *mVertPannerSep );

  mVertPanner->setResizeMode( mFolderTree, QSplitter::KeepSize);
  if( mLongFolderList )
  {
    mHorizPanner->setResizeMode( mHeaders, QSplitter::KeepSize);
  }
  else
  {
    mHorizPanner->setResizeMode( mVertPanner, QSplitter::KeepSize);
  }
}

//-----------------------------------------------------------------------------
void KMMainWin::slotSetEncoding()
{
    mEncodingStr = KGlobal::charsets()->encodingForName(mEncoding->currentText());
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
void KMMainWin::statusMsg(const QString& aText)
{
  QString text = " " + aText + " ";
  int statusWidth = mStatusBar->width() - littleProgress->width()
    - fontMetrics().maxWidth();

  while (!text.isEmpty() && fontMetrics().width( text ) >= statusWidth)
    text.truncate( text.length() - 1);

  mStatusBar->changeItem( text, mMessageStatusId);
}


//-----------------------------------------------------------------------------
void KMMainWin::hide()
{
  KMMainWinInherited::hide();
}


//-----------------------------------------------------------------------------
void KMMainWin::show()
{
  mHorizPanner->setSizes( *mHorizPannerSep );
  mVertPanner->setSizes( *mVertPannerSep );
  KMMainWinInherited::show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotClose()
{
  close(TRUE);
}

//-------------------------------------------------------------------------
void KMMainWin::slotSearch()
{
  if(!searchWin) {
    searchWin = new KMFldSearch(this, "Search", mFolder, false);
    connect(searchWin, SIGNAL(destroyed()),
	    this, SLOT(slotSearchClosed()));
  }

  searchWin->show();
  KWin::setActiveWindow(searchWin->winId());
}


//-------------------------------------------------------------------------
void KMMainWin::slotSearchClosed() {
  if(searchWin)
    searchWin = 0;
}


//-------------------------------------------------------------------------
void KMMainWin::slotFind() {
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
      mConfigureDialog = new ConfigureDialog( this, "configure", false );
  }
  mConfigureDialog->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotFilter()
{
  kernel->filterMgr()->openDialog( this );
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
  }
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

void KMMainWin::slotMailChecked(bool newMail) {
  if(mSendOnCheck)
    slotSendQueued();

  if (!newMail)
    return;

  if (mBeepOnNew) {
    KNotifyClient::beep();
  }

  // FIXME: change system() to a KProcess
  if (mExecOnNew) {
    if (mNewMailCmd.length() > 0)
      system((const char *)mNewMailCmd);
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

      if (mFolder->isMailingList()) {
          kdDebug()<<QString("mFolder->isMailingList() %1").arg( mFolder->mailingListPostAddress().latin1())<<endl;;

          msg->setTo(mFolder->mailingListPostAddress());
      }
      win = new KMComposeWin(msg, mFolder->identity());
  } else
      win = new KMComposeWin(msg);

  win->show();

}


//-----------------------------------------------------------------------------
void KMMainWin::slotModifyFolder()
{
  KMFolderDialog *d;

  if (!mFolder) return;
  d = new KMFolderDialog((KMFolder*)mFolder, mFolder->parent(),
			 this, i18n("Properties of folder %1").arg( mFolder->label() ) );
  if (d->exec()) {
    mFolderTree->reload();
    QListViewItem *qlvi = mFolderTree->indexOfFolder( mFolder );
    if (qlvi) {
      qlvi->setOpen(TRUE);
      mFolderTree->setCurrentItem( qlvi );
      mHeaders->msgChanged();
    }
  }
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
                                          i18n("Empty folder"), i18n("&Empty") )
       !=KMessageBox::Continue) return;
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
  updateMessageMenu();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotRemoveFolder()
{
  QString str;
  QDir dir;

  if (!mFolder) return;
  if (mFolder->isSystemFolder() || strcmp(mFolder->type(),"plain")!=0)
  {
    kdDebug() << "Cannot remove a\nsystem folder." << endl;
    return;
  }

  str = i18n("Are you sure you want to remove the folder\n"
	     "\"%1\" and all subfolders, discarding their contents?")
			     .arg(mFolder->label());

  if (KMessageBox::warningContinueCancel(this, str,
                              i18n("Remove folder"), i18n("&Remove") )
      ==
      KMessageBox::Continue)
  {
    KMFolder *folderToDelete = mFolder;
    QListViewItem *qlviCur = mFolderTree->currentItem();
    QListViewItem *qlvi = qlviCur->itemAbove();
    if (!qlvi)
      qlvi = mFolderTree->currentItem()->itemBelow();
    mHeaders->setFolder(0);
    mMsgView->clear();
    mFolderTree->setCurrentItem( qlvi );
    mFolderTree->setSelected( qlvi, TRUE );
    delete qlviCur;
    kernel->folderMgr()->remove(folderToDelete);
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCompactFolder()
{
  int idx = mHeaders->currentItemIndex();
  if (mFolder)
  {
    if (mFolder->account())
      mFolder->account()->expungeFolder(mFolder);
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
    if (folder->account())
      folder->account()->expungeFolder(folder);
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
      i18n( "Use of HTML in mail will make you more vulnerable to\n"
        "\"spam\" and may increase the likelihood that your system will be\n"
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
  if((mFolder != kernel->outboxFolder()) && (mFolder != kernel->draftsFolder()))
      return;
  mMsgView->update(true);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotEditMsg()
{
  if (mFolder != kernel->outboxFolder() && mFolder != kernel->draftsFolder())
    return;
  KMMessage *msg;
  int aIdx;

  if((aIdx = mHeaders->currentItemIndex()) <= -1)
    return;
  if(!(msg = mHeaders->getMsg(aIdx)))
    return;
  mFolder->removeMsg(msg);
  mHeaders->setSelected(mHeaders->currentItem(), TRUE);
  mHeaders->highlightMessage(mHeaders->currentItem(), true);

  KMComposeWin *win = new KMComposeWin;
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
}

//-----------------------------------------------------------------------------
void KMMainWin::slotUndo()
{
    mHeaders->undo();
    updateMessageMenu();
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
       QString cset = msg->charset();
       if (!cset.isEmpty())
         codec = KMMsgBase::codecForName(cset);
    }
    msg->viewSource(i18n("Message as Plain Text"), codec);
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMoveMsg()
{
  KMFolderSelDlg dlg(i18n("Move Message to Folder"));
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
  KMFolderSelDlg dlg(i18n("Copy Message to Folder"));
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


//-----------------------------------------------------------------------------
void KMMainWin::slotSetHeaderStyle(int id)
{
  if(id <= 5)
  {
    mViewMenu->setItemChecked((int)mMsgView->headerStyle(), FALSE);
    mMsgView->setHeaderStyle((KMReaderWin::HeaderStyle)id);
    mViewMenu->setItemChecked(id, TRUE);
  }
  else
  {
    mViewMenu->setItemChecked((int)mMsgView->attachmentStyle()+5, FALSE);
    mViewMenu->setItemChecked(id, TRUE);
    mMsgView->setAttachmentStyle(id-5);
  }
  readConfig(); // added this so _all_ the other widgets get this information
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
    mHeaders->nextUnreadMessage();
    mHeaders->blockSignals( false );
    mHeaders->highlightMessage( mHeaders->currentItem() );
}

//-----------------------------------------------------------------------------
void KMMainWin::folderSelected(KMFolder* aFolder, bool jumpToUnread)
{
  if (mFolder == aFolder && aFolder)
    return;

  kernel->kbp()->busy();
  if (!aFolder && mFolderTree->currentItem() == mFolderTree->firstChild())
  {
    mMsgView->setMsg(0,TRUE);
    if (mLongFolderList) mHeaders->hide();
    mMsgView->displayAboutPage();
  }
  else if (!mFolder)
  {
    mMsgView->enableMsgDisplay();
    mMsgView->setMsg(0,TRUE);
    mHeaders->show();
  }
  if (mFolder && mFolder->account() && mFolder->account()->autoExpunge()
    && mFolder->needsCompacting())
      mFolder->account()->expungeFolder(mFolder);
  writeFolderConfig();
  mFolder = (KMFolder*)aFolder;
  readFolderConfig();
  mMsgView->setHtmlOverride(mFolderHtmlPref);
  mHeaders->setFolder( mFolder, jumpToUnread );
  updateMessageMenu();
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgSelected(KMMessage *msg)
{
  mMsgView->setMsg(msg);
  if (msg && msg->parent() && msg->parent()->account())
  {
    KMImapJob *job = new KMImapJob(msg);
    connect(job, SIGNAL(messageRetrieved(KMMessage*)),
            SLOT(slotUpdateImapMessage(KMMessage*)));
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
    kernel->filterMgr()->createFilter( "Subject", msg->headerField( "Subject" ));
}

void KMMainWin::slotMailingListFilter()
{
    KMMessage* msg = mHeaders->getMsg(-1);
    if (msg) {
        QString name, value;
        if ( !detect_list( msg, name, value ).isNull() )
            kernel->filterMgr()->createFilter( name, value );
    }
}

//-----------------------------------------------------------------------------
void KMMainWin::slotFromFilter()
{
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg)
    kernel->filterMgr()->createFilter( "From", msg->headerField( "From" ));
}

//-----------------------------------------------------------------------------
void KMMainWin::slotToFilter()
{
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg)
    kernel->filterMgr()->createFilter( "To", msg->headerField( "To" ));
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
//called from heders. Message must not be deleted on close
void KMMainWin::slotMsgActivated(KMMessage *msg)
{
  if (mFolder == kernel->outboxFolder() || mFolder == kernel->draftsFolder())
  {
    slotEditMsg(); return;
  }

  assert(msg != NULL);
  KMReaderWin *win;

  win = new KMReaderWin;
  win->setAutoDelete(true);
  KMMessage *newMessage = new KMMessage();
  newMessage->fromString(msg->asString());
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
  connect(win, SIGNAL(popupMenu(const KURL&,const QPoint&)),
          this, SLOT(slotMsgPopup(const KURL&,const QPoint&)));
  connect(win, SIGNAL(urlClicked(const KURL&,int)),
          this, SLOT(slotUrlClicked(const KURL&,int)));
  connect(win, SIGNAL(showAtmMsg(KMMessage *)),
	  this, SLOT(slotAtmMsg(KMMessage *)));

  QAccel *accel = new QAccel(win);
  accel->connectItem(accel->insertItem(Key_Up),
                     win, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Key_Down),
                     win, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Key_Prior),
                     win, SLOT(slotScrollPrior()));
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
void KMMainWin::slotMarkAll() {
    QListViewItem *item;
    for (item = mHeaders->firstChild(); item; item = item->itemBelow())
        mHeaders->setSelected( item, TRUE );
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
      else if (queryPart.left(6) == "?cc=")
	msg->setCc( KURL::decode_string(queryPart.mid(4)) );
    }

    win = new KMComposeWin(msg,id);
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
    // -- David : replacement for KFM::openURL
    if ( !KOpenWithHandler::exists() )
      (void) new KFileOpenWithHandler();
    (void) new KRun( aUrl );
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoCompose()
{
  KMComposeWin *win;
  KMMessage *msg = new KMMessage;
  QString id = "";

  if ( mFolder )
    id = mFolder->identity();
  msg->initHeader(id);
  msg->setTo(mUrlCurrent.path());

  win = new KMComposeWin(msg,id);
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoReply()
{
  KMComposeWin *win;
  KMMessage *msg, *rmsg;
  QString id;

  if (!(msg = mHeaders->getMsg(-1))) return;
  id = msg->headerField( "X-KMail-Identity" );
  if ( id.isEmpty() )
    id = mFolder->identity();
  rmsg = msg->createReply(FALSE, FALSE, mMsgView->copyText());
  rmsg->setTo(mUrlCurrent.path());

  win = new KMComposeWin(rmsg,id);
  win->setCharset(msg->codec()->name(), TRUE);
  win->setReplyFocus();
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoForward()
{
  KMComposeWin *win;
  KMMessage *msg, *fmsg;

  if (!(msg = mHeaders->getMsg(-1))) return;
  fmsg = msg->createForward();
  fmsg->setTo(mUrlCurrent.path());

  win = new KMComposeWin(fmsg);
  win->setCharset(msg->codec()->name(), TRUE);
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
    clip->setText(mUrlCurrent.path());
    statusMsg(i18n("Address copied to clipboard."));
  }
  else
  {
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
void KMMainWin::slotMsgPopup(const KURL &aUrl, const QPoint& aPoint)
{
  KPopupMenu* menu = new KPopupMenu;
  updateMessageMenu();

  mUrlCurrent = aUrl;

  if (!aUrl.isEmpty())
  {
    if (aUrl.protocol() == "mailto")
    {
      // popup on a mailto URL
      menu->insertItem(i18n("Send to..."), this,
		       SLOT(slotMailtoCompose()));
      menu->insertItem(i18n("Send reply to..."), this,
		       SLOT(slotMailtoReply()));
      menu->insertItem(i18n("Forward to..."), this,
		       SLOT(slotMailtoForward()));
      menu->insertSeparator();
      menu->insertItem(i18n("Add to addressbook"), this,
		       SLOT(slotMailtoAddAddrBook()));
      menu->insertItem(i18n("Copy to clipboard"), this,
		       SLOT(slotUrlCopy()));
      menu->popup(aPoint,0);
    }
    else
    {
      // popup on a not-mailto URL
      menu->insertItem(i18n("Open URL..."), this,
		       SLOT(slotUrlOpen()));
      menu->insertItem(i18n("Copy to clipboard"), this,
		       SLOT(slotUrlCopy()));
      menu->popup(aPoint,0);
    }
  }
  else
  {
    // popup somewhere else (i.e., not a URL) on the message

     if (!mFolder) // no messages
     {
         return;
     }
     else  {

         bool out_folder = (mFolder == kernel->outboxFolder()) ||
                (mFolder == kernel->draftsFolder());
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
           }

           moveActionMenu->plug( menu );
           copyActionMenu->plug( menu );

           menu->insertSeparator();
           printAction->plug(menu);
           saveAsAction->plug(menu);
           menu->insertSeparator();
           deleteAction->plug(menu);
           menu->popup(aPoint, 0);
      }
  }
}

//-----------------------------------------------------------------------------
void KMMainWin::getAccountMenu()
{
  QStringList actList;

  actMenu->clear();
  actList = kernel->acctMgr()->getAccounts(true);
  QStringList::Iterator it;
  int id = 0;
  for(it = actList.begin(); it != actList.end() ; ++it, id++)
    actMenu->insertItem(*it, id);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupMenuBar()
{
  //----- File Menu
  (void) new KAction( i18n("&New Mail Client..."), "window_new", 0, this, SLOT(slotNewMailReader()),
		      actionCollection(), "new_mail_client" );

  saveAsAction = new KAction( i18n("Save &As..."), "filesave",
    KStdAccel::key(KStdAccel::Save),
    this, SLOT(slotSaveMsg()), actionCollection(), "save_as" );

  printAction = KStdAction::print (this, SLOT(slotPrintMsg()), actionCollection());

  (void) new KAction( i18n("Compact all &folders"), 0,
		      this, SLOT(slotCompactAll()),
		      actionCollection(), "compact_all_folders" );

  (void) new KAction( i18n("Check &Mail"), "mail_get", CTRL+Key_L,
		      this, SLOT(slotCheckMail()),
		      actionCollection(), "check_mail" );

  KActionMenu *actActionMenu = new
    KActionMenu( i18n("Check Mail in"), "mail_get", actionCollection(),
				   	"check_mail_in" );
  actActionMenu->setDelayed(true); //needed for checking "all accounts"

  connect(actActionMenu,SIGNAL(activated()),this,SLOT(slotCheckMail()));

  actMenu = actActionMenu->popupMenu();
  connect(actMenu,SIGNAL(activated(int)),this,SLOT(slotCheckOneAccount(int)));
  connect(actMenu,SIGNAL(aboutToShow()),this,SLOT(getAccountMenu()));

  (void) new KAction( i18n("&Send Queued"), 0, this,
		     SLOT(slotSendQueued()), actionCollection(), "send_queued");

  (void) new KAction( i18n("Address &Book..."), "contents", 0, this,
		      SLOT(slotAddrBook()), actionCollection(), "addressbook" );

  (void) new KAction( i18n("&Import..."), "fileopen", 0, this,
		      SLOT(slotImport()), actionCollection(), "import" );

  KStdAction::close( this, SLOT(slotClose()), actionCollection());
  //KStdAction::quit( this, SLOT(quit()), actionCollection());

  //----- Edit Menu
  KStdAction::undo( this, SLOT(slotUndo()), actionCollection(), "edit_undo");

  (void) new KAction( i18n("&Copy text"), KStdAccel::key(KStdAccel::Copy), this,
		      SLOT(slotCopyText()), actionCollection(), "copy_text" );

  deleteAction = new KAction( i18n("&Delete"), "editdelete", Key_D, this,
		      SLOT(slotDeleteMsg()), actionCollection(), "delete" );

  (void) new KAction( i18n("&Search messages..."), "find", Key_S, this,
		      SLOT(slotSearch()), actionCollection(), "search_messages" );

  (void) new KAction( i18n("&Find in message..."), KStdAccel::key(KStdAccel::Find), this,
		      SLOT(slotFind()), actionCollection(), "find_in_messages" );

  (void) new KAction( i18n("Sele&ct all messages"), Key_K, this,
		      SLOT(slotMarkAll()), actionCollection(), "mark_all_messages" );

  (void) new KAction( i18n("Select message &text"), KStdAccel::key(KStdAccel::SelectAll), this,
		      SLOT(slotSelectText()), actionCollection(), "mark_all_text" );

  //----- Folder Menu
  (void) new KAction( i18n("&Create..."), "folder_new", 0, this,
		      SLOT(slotAddFolder()), actionCollection(), "create" );

  modifyFolderAction = new KAction( i18n("&Properties..."), 0, this,
		      SLOT(slotModifyFolder()), actionCollection(), "modify" );

  (void) new KAction( i18n("C&ompact"), 0, this,
		      SLOT(slotCompactFolder()), actionCollection(), "compact" );

  (void) new KAction( i18n("&Empty"), 0, this,
		      SLOT(slotEmptyFolder()), actionCollection(), "empty" );

  removeFolderAction = new KAction( i18n("&Remove"), 0, this,
		      SLOT(slotRemoveFolder()), actionCollection(), "remove" );

  preferHtmlAction = new KToggleAction( i18n("&Prefer HTML to plain text"), 0, this,
		      SLOT(slotOverrideHtml()), actionCollection(), "prefer_html" );

  threadMessagesAction = new KToggleAction( i18n("&Thread messages"), 0, this,
		      SLOT(slotOverrideThread()), actionCollection(), "thread_messages" );

  //----- Message Menu
  (void) new KAction( i18n("New Message..."), "filenew", KStdAccel::key(KStdAccel::New), this,
		      SLOT(slotCompose()), actionCollection(), "new_message" );

  (void) new KAction( i18n("&Next"), Key_N, mHeaders,
		      SLOT(nextMessage()), actionCollection(), "next" );

  (void) new KAction( i18n("Next unread"), "next", Key_Plus, mHeaders,
		      SLOT(nextUnreadMessage()), actionCollection(), "next_unread" );

  (void) new KAction( i18n("&Previous"), Key_P, mHeaders,
		      SLOT(prevMessage()), actionCollection(), "previous" );

  (void) new KAction( i18n("Previous unread"), "previous", Key_Minus, mHeaders,
		      SLOT(prevUnreadMessage()), actionCollection(), "previous_unread" );

  replyAction = new KAction( i18n("&Reply..."), "mail_reply", Key_R, this,
		      SLOT(slotReplyToMsg()), actionCollection(), "reply" );

  noQuoteReplyAction = new KAction( i18n("Reply &without quote..."), ALT+Key_R,
    this, SLOT(slotNoQuoteReplyToMsg()), actionCollection(), "noquotereply" );

  replyAllAction = new KAction( i18n("Reply &All..."), "mail_replyall",
     Key_A, this, SLOT(slotReplyAllToMsg()), actionCollection(), "reply_all" );

  replyListAction = new KAction( i18n("Reply &List..."),
     Key_L, this, SLOT(slotReplyListToMsg()), actionCollection(), "reply_list" );

  forwardAction = new KAction( i18n("&Forward..."), "mail_forward", Key_F, this,
		      SLOT(slotForwardMsg()), actionCollection(), "forward" );

  forwardAttachedAction = new KAction( i18n("&Forward as attachment"), "mail_forward_attached", SHIFT+Key_F, this,
		      SLOT(slotForwardAttachedMsg()), actionCollection(), "forward_attached" );

  redirectAction = new KAction( i18n("R&edirect..."), Key_E, this,
		      SLOT(slotRedirectMsg()), actionCollection(), "redirect" );

  bounceAction = new KAction( i18n("&Bounce..."), 0, this,
		      SLOT(slotBounceMsg()), actionCollection(), "bounce" );

  sendAgainAction = new KAction( i18n("Send a&gain..."), 0, this,
		      SLOT(slotResendMsg()), actionCollection(), "send_again" );

  //----- Message-Encoding Submenu
  mEncoding = new KSelectAction( i18n( "Set &Encoding" ), 0, this, SLOT( slotSetEncoding() ), actionCollection(), "encoding" );
  QStringList encodings = KGlobal::charsets()->descriptiveEncodingNames();
  encodings.prepend( i18n( "Auto" ) );
  mEncoding->setItems( encodings );
  mEncoding->setCurrentItem(0);

  QStringList::Iterator it;
  int i = 0;
  for( it = encodings.begin(); it != encodings.end(); ++it) {
      if ( KGlobal::charsets()->encodingForName(*it ) == mEncodingStr ) {
	  mEncoding->setCurrentItem( i );
	  break;
      }
      i++;
  }

  editAction = new KAction( i18n("Edi&t"), Key_T, this,
		      SLOT(slotEditMsg()), actionCollection(), "edit" );

  //----- Create filter submenu
  filterMenu = new KActionMenu( i18n("&Create Filter"), actionCollection(), "create_filter" );

  KAction *subjectFilterAction = new KAction( i18n("Filter on Subject..."), 0, this,
                                              SLOT(slotSubjectFilter()),
                                              actionCollection(), "subject_filter");
  filterMenu->insert( subjectFilterAction );

  KAction *fromFilterAction = new KAction( i18n("Filter on From..."), 0, this,
                                           SLOT(slotFromFilter()),
                                           actionCollection(), "from_filter");
  filterMenu->insert( fromFilterAction );

  KAction *toFilterAction = new KAction( i18n("Filter on To..."), 0, this,
                                         SLOT(slotToFilter()),
                                         actionCollection(), "to_filter");
  filterMenu->insert( toFilterAction );

  mlistFilterAction = new KAction( i18n("Filter on Mailing-List..."), 0, this,
                                   SLOT(slotMailingListFilter()), actionCollection(),
                                   "mlist_filter");
  filterMenu->insert( mlistFilterAction );

  statusMenu = new KActionMenu ( i18n( "Set Status" ), actionCollection(), "set_status" );

  //----- Set status submenu
  KAction *newAction= new KAction( i18n("&New"), "kmmsgnew", 0, this,
                                   SLOT(slotSetMsgStatusNew()),
                                   actionCollection(), "status_new");
  statusMenu->insert( newAction );

  KAction *unreadAction=new KAction( i18n("&Unread"), "kmmsgunseen", 0, this,
                                     SLOT(slotSetMsgStatusUnread()),
                                     actionCollection(), "status_unread");
  statusMenu->insert( unreadAction );

  KAction *readAction=new KAction( i18n("&Read"), "kmmsgold", 0, this,
                                   SLOT(slotSetMsgStatusRead()), actionCollection(),
                                   "status_read");
  statusMenu->insert( readAction );

  KAction *repliedAction= new KAction( i18n("R&eplied"), "kmmsgreplied", 0, this,
                              SLOT(slotSetMsgStatusReplied()),
                              actionCollection(), "status_replied");
  statusMenu->insert( repliedAction );

  KAction *queueAction=new KAction( i18n("&Queued"), "kmmsgqueued", 0, this,
                                    SLOT(slotSetMsgStatusQueued()),
                                    actionCollection(), "status_queued");
  statusMenu->insert( queueAction );

  KAction *sentAction=new KAction( i18n("&Sent"), "kmmsgsent", 0, this,
                                   SLOT(slotSetMsgStatusSent()),
                                   actionCollection(), "status_sent");
  statusMenu->insert( sentAction );

  KAction *flagAction=new KAction( i18n("&Flagged"), "kmmsgflag", 0, this,
                                   SLOT(slotSetMsgStatusFlag()),
                                   actionCollection(), "status_flag");
  statusMenu->insert( flagAction );

  moveActionMenu = new KActionMenu( i18n("&Move to" ),
                                    actionCollection(), "move_to" );

  copyActionMenu = new KActionMenu( i18n("&Copy to" ),
                                    actionCollection(), "copy_to" );

  (void) new KAction( i18n("Apply filters"), "filter", CTRL+Key_J, this,
		      SLOT(slotApplyFilters()), actionCollection(), "apply_filters" );

  (void) new KAction( i18n("View Source..."), 0, this,
		      SLOT(slotShowMsgSrc()), actionCollection(), "view_source" );

  //----- View Menu
  KActionMenu *viewMenuAction = new
    KActionMenu( i18n("things to show", "&View"), actionCollection(), "view" );

  mViewMenu = viewMenuAction->popupMenu();
  mViewMenu->setCheckable(TRUE);
  connect(mViewMenu,SIGNAL(activated(int)),SLOT(slotSetHeaderStyle(int)));
  mViewMenu->insertItem(i18n("&Brief Headers"), KMReaderWin::HdrBrief);
  mViewMenu->insertItem(i18n("&Fancy Headers"), KMReaderWin::HdrFancy);
  mViewMenu->insertItem(i18n("&Standard Headers"), KMReaderWin::HdrStandard);
  mViewMenu->insertItem(i18n("&Long Headers"), KMReaderWin::HdrLong);
  mViewMenu->insertItem(i18n("&All Headers"), KMReaderWin::HdrAll);
  mViewMenu->insertSeparator();
  mViewMenu->insertItem(i18n("&Iconic Attachments"),
		       KMReaderWin::HdrAll + KMReaderWin::IconicAttmnt);
  mViewMenu->insertItem(i18n("Sma&rt Attachments"),
		       KMReaderWin::HdrAll + KMReaderWin::SmartAttmnt);
  mViewMenu->insertItem(i18n("I&nlined Attachments"),
		       KMReaderWin::HdrAll + KMReaderWin::InlineAttmnt);
  mViewMenu->setItemChecked((int)mMsgView->headerStyle(), TRUE);
  mViewMenu->setItemChecked((int)mMsgView->attachmentStyle()+5, TRUE);

  //----- Settings Menu
  toolbarAction = KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
    actionCollection());
  statusbarAction = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
    actionCollection());
  KStdAction::keyBindings(this, SLOT(slotEditKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
  KStdAction::preferences(this, SLOT(slotSettings()), actionCollection());


  (void) new KAction( i18n("Configure F&ilters..."), 0, this,
 		      SLOT(slotFilter()), actionCollection(), "filter" );

  createGUI( "kmmainwin.rc", false );

  QObject::connect( guiFactory()->container("folder", this),
		    SIGNAL( aboutToShow() ), this, SLOT( updateFolderMenu() ));

  QObject::connect( guiFactory()->container("message", this),
		    SIGNAL( aboutToShow() ), this, SLOT( updateMessageMenu() ));

  conserveMemory();

  menutimer = new QTimer( this, "menutimer" );
  connect( menutimer, SIGNAL( timeout() ), SLOT( updateMessageMenu() ) );

  updateMessageMenu();

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
  KEditToolbar dlg(actionCollection(), "kmmainwin.rc");

  if (dlg.exec() == true)
  {
    createGUI("kmmainwin.rc");
    toolbarAction->setChecked(!toolBar()->isHidden());
  }
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

void KMMainWin::quit()
{
  qApp->quit();
}

void KMMainWin::slotReadOn()
{
    if ( !mMsgView )
        return;

    if ( !mMsgView->atBottom() ) {
        mMsgView->slotJumpDown();
        return;
    }
    int i = mHeaders->findUnread(true, -1, false, true);
    if ( i < 0 ) // let's try from start, what gives?
        i = mHeaders->findUnread(true, 0, false, true);
    if ( i >= 0 ) {
         mHeaders->setCurrentMsg(i);
         QTimer::singleShot( 100, mHeaders, SLOT( ensureCurrentItemVisible() ) );
         return;
    }
    mFolderTree->nextUnreadFolder( true );
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
    while ( menu->count() ) {
        delete menu->findItem( menu->idAt( 0 ) )->popup();
        menu->removeItemAt( 0 );
    }

    menu->clear();

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
          menuId = menu->insertItem(i18n("Move to this folder"));
      else
          menuId = menu->insertItem(i18n("Copy to this folder"));
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
void KMMainWin::slotUpdateMessageMenu()
{
    menutimer->stop();
    menutimer->start( 20, true );
}

void KMMainWin::updateMessageMenu()
{
    kdDebug() << "updateMessageMenu\n";

    mMenuToFolder.clear();
    folderToPopupMenu( 0, true, this, &mMenuToFolder, moveActionMenu->popupMenu() );
    folderToPopupMenu( 0, false, this, &mMenuToFolder, copyActionMenu->popupMenu() );

    int count = 0;

    if ( mFolder ) {
        for (QListViewItem *item = mHeaders->firstChild(); item; item = item->itemBelow())
            if (item->isSelected() )
                count++;
        if ( !count && mFolder->count() ) // there will always be one in mMsgView
            count = 1;
    }

    mlistFilterAction->setText( i18n("Filter on Mailing-List...") );

    bool mass_actions = count >= 1;
    statusMenu->setEnabled( mass_actions );
    moveActionMenu->setEnabled( mass_actions );
    copyActionMenu->setEnabled( mass_actions );
    deleteAction->setEnabled( mass_actions );
    forwardAction->setEnabled( mass_actions );
    forwardAttachedAction->setEnabled( mass_actions );
    action( "apply_filters" )->setEnabled( mass_actions );

    bool single_actions = count == 1;
    filterMenu->setEnabled( single_actions );
    editAction->setEnabled( single_actions && 
      (mFolder == kernel->outboxFolder() || mFolder == kernel->draftsFolder()));
    bounceAction->setEnabled( single_actions );
    replyAction->setEnabled( single_actions );
    noQuoteReplyAction->setEnabled( single_actions );
    replyAllAction->setEnabled( single_actions );
    replyListAction->setEnabled( single_actions );
    redirectAction->setEnabled( single_actions );
    sendAgainAction->setEnabled( single_actions );
    printAction->setEnabled( single_actions );
    saveAsAction->setEnabled( mass_actions );
    action( "view_source" )->setEnabled( single_actions );

    if ( count == 1 ) {
        KMMessage *msg;
        int aIdx;
        if((aIdx = mHeaders->currentItemIndex()) <= -1)
           return;
        if(!(msg = mHeaders->getMsg(aIdx)))
            return;

        QString name, value;
        QString lname = detect_list( msg, name, value );
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
  modifyFolderAction->setEnabled( mFolder ? !mFolder->isSystemFolder()
    : false );
  removeFolderAction->setEnabled( mFolder ? !mFolder->isSystemFolder()
    : false );
  preferHtmlAction->setEnabled( mFolder ? true : false );
  threadMessagesAction->setEnabled( true );
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


