// kmmainwin.cpp
//#define MALLOC_DEBUG 1

#include <kwin.h>
#include <kmfldsearch.h>

#ifdef MALLOC_DEBUG
#include <qmessagebox.h>
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

#include "configuredialog.h"
#include "kmbroadcaststatus.h"
#include "kmfoldermgr.h"
#include "kmsettings.h"
#include "kmfolderdia.h"
#include "kmaccount.h"
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
#include "kwm.h"

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
  QListViewItem* idx;
  mIntegrated  = TRUE;
  mFolder = NULL;
  mHorizPannerSep = new QValueList<int>;
  mVertPannerSep = new QValueList<int>;
  *mHorizPannerSep << 1 << 1;
  *mVertPannerSep << 1 << 1;

  setMinimumSize(400, 300);

  readPreConfig();
  createWidgets();
  readConfig();
  activatePanners();

  setupMenuBar();
  setupToolBar();
  setupStatusBar();

  idx = mFolderTree->indexOfFolder(kernel->inboxFolder());
  if (idx!=0) {
    mFolderTree->setCurrentItem(idx);
    mFolderTree->setSelected(idx,TRUE);
  }

  connect(kernel->msgSender(), SIGNAL(statusMsg(const QString&)),
	  SLOT(statusMsg(const QString&)));
  connect(kernel->acctMgr(), SIGNAL( newMail()),
          SLOT( slotNewMail()));

  setCaption( i18n("KDE Mail Client") );

  // must be the last line of the constructor:
  mStartupDone = TRUE;
}


//-----------------------------------------------------------------------------
KMMainWin::~KMMainWin()
{
  writeConfig();      
  writeFolderConfig();
      
  if (mHeaders)    delete mHeaders;
  if (mToolBar)    delete mToolBar;
  if (mMenuBar)    delete mMenuBar;
  if (mStatusBar)  delete mStatusBar;
  if (mFolderTree) delete mFolderTree;
}


//-----------------------------------------------------------------------------
void KMMainWin::readPreConfig(void)
{
  KConfig *config = kapp->config();
  QString str;

  config->setGroup("Geometry");
  mLongFolderList = config->readBoolEntry("longFolderList", false);
}


//-----------------------------------------------------------------------------
void KMMainWin::readFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = kapp->config();
  int pathLen = mFolder->path().length() - kernel->folderMgr()->basePath().length();
  QString path = mFolder->path().right( pathLen );

  if (!path.isEmpty())
    path = path.right( path.length() - 1 ) + "/";
  config->setGroup("Folder-" + path + mFolder->name());
  mFolderThreadPref = config->readBoolEntry( "threadMessagesOverride", false );
  mFolderHtmlPref = config->readBoolEntry( "htmlMailOverride", false );
}


//-----------------------------------------------------------------------------
void KMMainWin::writeFolderConfig(void)
{
  if (!mFolder)
    return;

  KConfig *config = kapp->config();
  int pathLen = mFolder->path().length() - kernel->folderMgr()->basePath().length();
  QString path = mFolder->path().right( pathLen );

  if (!path.isEmpty())
    path = path.right( path.length() - 1 ) + "/";
  config->setGroup("Folder-" + path + mFolder->name());
  config->writeEntry( "threadMessagesOverride", mFolderThreadPref );
  config->writeEntry( "htmlMailOverride", mFolderHtmlPref );
}


//-----------------------------------------------------------------------------
void KMMainWin::readConfig(void)
{
  KConfig *config = kapp->config();
  bool oldLongFolderList=false;
  int w, h;
  QString str;

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

  config->setGroup("Reader");
  mHtmlPref = config->readBoolEntry( "htmlMail", true );

  config->setGroup("Geometry");
  mThreadPref = config->readBoolEntry( "nestedMessages", false );
  str = config->readEntry("MainWin", "600,600");
  if (!str.isEmpty() && str.find(',')>=0)
  {
    sscanf(str,"%d,%d",&w,&h);
    resize(w,h);
  }
  str = config->readEntry("Panners", "300,130");
  if ((!str.isEmpty()) && (str.find(',')!=-1))
    sscanf(str,"%d,%d",&((*mHorizPannerSep)[0]),&((*mVertPannerSep)[0]));
  else
    (*mHorizPannerSep)[0] = (*mVertPannerSep)[0] = 100;
  (*mHorizPannerSep)[1] = h - (*mHorizPannerSep)[0];
  (*mVertPannerSep)[1] = w - (*mVertPannerSep)[0];

  mMsgView->readConfig();
  mHeaders->readConfig();
  mFolderTree->readConfig();

  config->setGroup("General");
  mSendOnCheck = config->readBoolEntry("sendOnCheck",false);
  mBeepOnNew = config->readBoolEntry("beep-on-mail", false);
  mBoxOnNew = config->readBoolEntry("msgbox-on-mail", false);
  mExecOnNew = config->readBoolEntry("exec-on-mail", false);
  mNewMailCmd = config->readEntry("exec-on-mail-cmd", "");

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
    mMsgView->setMsg( mMsgView->msg(), TRUE );
    mHeaders->setFolder(mFolder);
    //    kernel->kbp()->idle(); //For symmetry
    show();
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

  config->setGroup("Geometry");

  s.sprintf("%i,%i", r.width(), r.height());
  config->writeEntry("MainWin", s);

  // Get those panner sizes right!
  s.sprintf("%i,%i",
	    (mHorizPanner->sizes()[0] * r.height() ) /
	    (mHorizPanner->sizes()[0] + mHorizPanner->sizes()[1]),
	    ( mVertPanner->sizes()[0] * r.width() ) /
	    (mVertPanner->sizes()[0] + mVertPanner->sizes()[1])
	    );
  config->writeEntry("Panners", s);
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

  // BUG -sanders these accelerators stop working after switching
  // between long/short folder layout
  // Probably need to disconnect them first.

  // create list of messages
  mHeaders = new KMHeaders(this, pnrMsgList, "headers");
  connect(mHeaders, SIGNAL(selected(KMMessage*)),
	  this, SLOT(slotMsgSelected(KMMessage*)));
  connect(mHeaders, SIGNAL(activated(KMMessage*)),
	  this, SLOT(slotMsgActivated(KMMessage*)));
  accel->connectItem(accel->insertItem(Key_Left),
		     mHeaders, SLOT(prevMessage()));
  accel->connectItem(accel->insertItem(Key_Right),
		     mHeaders, SLOT(nextMessage()));

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
  connect(mFolderTree, SIGNAL(folderDrop(KMFolder*)),
	  this, SLOT(slotMoveMsgToFolder(KMFolder*)));
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
    setView(mVertPanner);
  }
  else
  {
    mFolderTree->reparent( mVertPanner, 0, QPoint( 0, 0 ) );
    mHeaders->reparent( mVertPanner, 0, QPoint( 0, 0 ) );
    mMsgView->reparent( mHorizPanner, 0, QPoint( 0, 0 ) );
    setView(mHorizPanner);
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
void KMMainWin::statusMsg(const QString& aText)
{
  mStatusBar->changeItem(" " + aText + " ", mMessageStatusId);
  /* Just causes to much trouble with event driven repainting.
  kapp->flushX();
  kapp->processEvents(100);
  */
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
void KMMainWin::slotSearch() {
  if(!searchWin) {
    searchWin = new KMFldSearch(this, "Search", false);
    connect(searchWin, SIGNAL(destroyed()),
	    this, SLOT(slotsearchClosed()));
  } 

  searchWin->show();
  KWin::setActiveWindow(searchWin->winId());
}

//-------------------------------------------------------------------------
void KMMainWin::slotSearchClosed() {
  if(searchWin)
    searchWin = 0;
}


//-----------------------------------------------------------------------------
void KMMainWin::slotHelp()
{
  kapp->invokeHTMLHelp("","");
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
  //
  // 2000-03-12 Espen Sand
  // New Settings Dialog
  //
  static ConfigureDialog *dialog = 0;
  if( dialog == 0 )
  {
    dialog = new ConfigureDialog( this, "configure", false );
  }
  dialog->show();
}


void KMMainWin::slotOldSettings()
{
  // markus: we write the Config here cause otherwise the
  // geometry will be set to the value in the config.
  // Problem arises when we change the geometry during the
  // session are press the OK button in the settings. Then we
  // lose the current geometry! Not anymore ;-)
  //  writeConfig();
  //  KMSettings dlg(this);
  //  dlg.exec();
}













//-----------------------------------------------------------------------------
void KMMainWin::slotFilter()
{
  kernel->filterMgr()->openDialog();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotAddrBook()
{
  KMAddrBookEditDlg dlg( kernel->addrBook(), this );
  dlg.exec();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUnimplemented()
{
  warning(i18n("Sorry, but this feature\nis still missing"));
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

 if(mSendOnCheck)
   slotSendQueued();
 kernel->setCheckingMail(false);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMenuActivated()
{
  if ( !actMenu->isVisible() )
      getAccountMenu();
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

  if(mSendOnCheck)
    slotSendQueued();

  kernel->setCheckingMail(false);
}

void KMMainWin::slotNewMail() {

  if (mBeepOnNew) {
    KApplication::beep();
  }

  if (mExecOnNew) {
    if (mNewMailCmd.length() > 0)
      system((const char *)mNewMailCmd);
  }

  if (mBoxOnNew) {
    KMessageBox::information(this, QString(i18n("You have new mail!")),
                                   QString(i18n("New Mail")));
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
  msg->initHeader();

  win = new KMComposeWin(msg);
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotModifyFolder()
{
  KMFolderDialog *d;

  if (!mFolder) return;
  d = new KMFolderDialog((KMFolder*)mFolder, mFolder->parent(),
			 this, i18n("Modify Folder") );
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
void KMMainWin::slotEmptyFolder()
{
  QString str;
  KMMessage* msg;

  if (!mFolder) return;

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
}


//-----------------------------------------------------------------------------
void KMMainWin::slotRemoveFolder()
{
  QString str;
  QDir dir;

  if (!mFolder) return;
  if (mFolder->isSystemFolder() || strcmp(mFolder->type(),"plain")!=0)
  {
    warning(i18n("Cannot remove a\nsystem folder."));
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
    kernel->kbp()->busy();
    mFolder->compact();
    kernel->kbp()->idle();
  }
  mHeaders->setCurrentItemByIndex(idx);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotOverrideHtml()
{
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
  mHeaders->replyToMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotReplyAllToMsg()
{
  mHeaders->replyAllToMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotForwardMsg()
{
  mHeaders->forwardMsg();
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
void KMMainWin::slotEditMsg()
{
  KMMessage *msg;
  int aIdx;

  if(mFolder != kernel->outboxFolder())
  {
      KMessageBox::sorry(0,
         i18n("Sorry, only messages in the outbox folder can be edited."));
      return;
  }


  if((aIdx = mHeaders->currentItemIndex()) <= -1)
    return;
  if(!(msg = mHeaders->getMsg(aIdx)))
    return;

  KMComposeWin *win = new KMComposeWin;
  win->setMsg(msg,FALSE);
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
}

//-----------------------------------------------------------------------------
void KMMainWin::slotShowMsgSrc()
{
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg) msg->viewSource(i18n("Message as Plain Text"));
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMoveMsg()
{
  KMFolderSelDlg dlg(i18n("Move Message - Select Folder"));
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
void KMMainWin::slotApplyFilters()
{
  mHeaders->applyFiltersOnMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCopyMsg()
{
  KMFolderSelDlg dlg(i18n("Copy Message - Select Folder"));
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
  if (kernel->msgSender()->sendQueued())
    statusMsg(i18n("Queued messages successfully sent."));
  else
    statusMsg(i18n("Failed to send (some) queued messages."));
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


//-----------------------------------------------------------------------------
void KMMainWin::folderSelected(KMFolder* aFolder)
{
  if (mFolder == aFolder)
    return;

  kernel->kbp()->busy();
  writeFolderConfig();
  mFolder = (KMFolder*)aFolder;
  readFolderConfig();
  mMsgView->setHtmlOverride(mFolderHtmlPref);
  mHeaders->setFolder(mFolder);
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgSelected(KMMessage *msg)
{
  mMsgView->setMsg(msg);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatus(int id)
{
  mHeaders->setMsgStatus((KMMsgStatus)id);
}


//-----------------------------------------------------------------------------
//called from heders. Message must not be deleted on close
void KMMainWin::slotMsgActivated(KMMessage *msg)
{
  KMReaderWin *win;
  assert(msg != NULL);

  win = new KMReaderWin;
  showMsg(win, msg);
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
  KWM::setMiniIcon(win->winId(), kapp->miniIcon());
  win->setCaption(msg->subject());

  win->setMsg(msg);
  win->resize(550,600);

  connect(win, SIGNAL(statusMsg(const QString&)),
          this, SLOT(statusMsg(const QString&)));
  connect(win, SIGNAL(popupMenu(const KURL&,const QPoint&)),
          this, SLOT(slotMsgPopup(const KURL&,const QPoint&)));
  connect(win, SIGNAL(urlClicked(const KURL&,int)),
          this, SLOT(slotUrlClicked(const KURL&,int)));

  QAccel *accel = new QAccel(win);
  accel->connectItem(accel->insertItem(Key_Up),
                     win, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Key_Down),
                     win, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Key_Prior),
                     win, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Key_S),
                     win, SLOT(slotCopyMessage()));
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
void KMMainWin::slotUrlClicked(const KURL &aUrl, int)
{
  KMComposeWin *win;
  KMMessage* msg;

  if (aUrl.protocol() == "mailto")
  {
    msg = new KMMessage;
    msg->initHeader();
    msg->setTo(aUrl.path());
    QString query=aUrl.query();
    if (query.left(8) == "subject=")
    {
       msg->setSubject( query.mid(8) ); // TODO: Decode subject!
    }

    win = new KMComposeWin(msg);
    win->show();
  }
  else if ((aUrl.protocol() == "http") || (aUrl.protocol() ==  "ftp") ||
           (aUrl.protocol() == "file"))
  {
    statusMsg(i18n("Opening URL..."));
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

  msg->initHeader();
  msg->setTo(mUrlCurrent.mid(7,255));

  win = new KMComposeWin(msg);
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoReply()
{
  KMComposeWin *win;
  KMMessage *msg;

  if (!(msg = mHeaders->getMsg(-1))) return;
  msg = msg->createReply(FALSE);
  msg->setTo(mUrlCurrent.mid(7,255));

  win = new KMComposeWin(msg);
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoForward()
{
  KMComposeWin *win;
  KMMessage *msg;

  if (!(msg = mHeaders->getMsg(-1))) return;
  msg = msg->createForward();
  msg->setTo(mUrlCurrent.mid(7,255));

  win = new KMComposeWin(msg);
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMailtoAddAddrBook()
{
#warning mario: had to disable this for compile
//    if (mUrlCurrent.isEmpty()) return;
//    kernel->addrBook()->insert(mUrlCurrent.mid(7,255));
//    statusMsg(i18n("Address added to addressbook."));
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlCopy()
{
  QClipboard* clip = QApplication::clipboard();

  if (strnicmp(mUrlCurrent,"mailto:",7)==0)
  {
    clip->setText(mUrlCurrent.mid(7,255));
    statusMsg(i18n("Address copied to clipboard."));
  }
  else
  {
    clip->setText(mUrlCurrent);
    statusMsg(i18n("URL copied to clipboard."));
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlOpen()
{
  if (mUrlCurrent.isEmpty()) return;
  //  mMsgView->slotUrlOpen(mUrlCurrent, QString::null, 0);
  mMsgView->slotUrlOpen( KURL( mUrlCurrent ), KParts::URLArgs() );
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgPopup(const KURL &aUrl, const QPoint& aPoint)
{
  KPopupMenu* menu = new KPopupMenu;

  mUrlCurrent = aUrl.url();

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
    // popup somewhere else on the document
    menu->insertItem(i18n("&Reply..."), this,
		     SLOT(slotReplyToMsg()));
    menu->insertItem(i18n("Reply &All..."), this,
		     SLOT(slotReplyAllToMsg()));
    menu->insertItem(i18n("&Forward..."), this,
		     SLOT(slotForwardMsg()), Key_F);
    menu->insertItem(i18n("R&edirect..."), this,
		     SLOT(slotRedirectMsg()));
    menu->insertSeparator();
    menu->insertItem(i18n("&Move..."), this,
		     SLOT(slotMoveMsg()), Key_M);
    menu->insertItem(i18n("&Copy..."), this,
		     SLOT(slotCopyMsg()), Key_C);
    menu->insertSeparator();
    menu->insertItem(i18n("&Delete"), this,
		     SLOT(slotDeleteMsg()), Key_D);
    menu->popup(aPoint, 0);
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::getAccountMenu()
{
  QStrList actList;

  actMenu->clear();
  actList = kernel->acctMgr()->getAccounts();
  QString tmp;
  int id = 0;
  for(tmp = actList.first(); tmp ; tmp = actList.next(), id++)
    actMenu->insertItem(tmp, id);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupMenuBar()
{
  //----- File Menu
  fileMenu = new QPopupMenu();
  fileMenu->insertItem(i18n("&New Mailreader"), this,
		       SLOT(slotNewMailReader()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("Save &As..."), this,
			   SLOT(slotSaveMsg()), KStdAccel::key(KStdAccel::Save));
  fileMenu->insertItem(i18n("&Print..."), this,
			   SLOT(slotPrintMsg()), KStdAccel::key(KStdAccel::Print));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("Compact all &folders"), kernel->folderMgr(),
                        SLOT(compactAll()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("Check &Mail"), this,
		       SLOT(slotCheckMail()), CTRL+Key_L);
  actMenu = new QPopupMenu();

  getAccountMenu();

  connect(actMenu,SIGNAL(activated(int)),this,SLOT(slotCheckOneAccount(int)));
  connect(fileMenu,SIGNAL(highlighted(int)),this,SLOT(slotMenuActivated()));

  fileMenu->insertItem(i18n("Check Mail in..."),actMenu);

  fileMenu->insertItem(i18n("Send &Queued"), this,
		       SLOT(slotSendQueued()));
  fileMenu->insertSeparator();
  fileMenu->insertItem("Settings...", this,
		       SLOT(slotSettings()));
  //fileMenu->insertItem(i18n("&Settings (old dialog)..."), this,
  //		       SLOT(slotOldSettings()));
  fileMenu->insertItem(i18n("&Addressbook..."), this,
		       SLOT(slotAddrBook()));
  fileMenu->insertItem(i18n("F&ilter..."), this,
		       SLOT(slotFilter()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("&Close"), this,
			   SLOT(slotClose()), KStdAccel::key(KStdAccel::Close));
  fileMenu->insertItem(i18n("&Quit"), this,
			   SLOT(quit()), KStdAccel::key(KStdAccel::Quit));

  //----- Edit Menu
  QPopupMenu *editMenu = new QPopupMenu();
  editMenu->insertItem(i18n("&Copy"), this, SLOT(slotCopyText()),
			   KStdAccel::key(KStdAccel::Copy));
  editMenu->insertSeparator();
#ifdef BROKEN
  editMenu->insertItem(i18n("&Find..."), this,
			   SLOT(slotUnimplemented()), KStdAccel::key(KStdAccel::Find));
#endif
  //----- Folder Menu
  mFolderMenu = new QPopupMenu();
  mFolderMenu->insertItem(i18n("&Create..."), this,
			 SLOT(slotAddFolder()));
  mFolderMenu->insertItem(i18n("&Modify..."), this,
			 SLOT(slotModifyFolder()));
  mFolderMenu->insertItem(i18n("C&ompact"), this,
			 SLOT(slotCompactFolder()));
  mFolderMenu->insertSeparator();
  mFolderMenu->insertItem(i18n("&Empty"), this,
			 SLOT(slotEmptyFolder()));
  mFolderMenu->insertItem(i18n("&Remove"), this,
			 SLOT(slotRemoveFolder()));
  mFolderMenu->insertSeparator();
  htmlId = mFolderMenu->insertItem("", this,
				  SLOT(slotOverrideHtml()));
  threadId = mFolderMenu->insertItem("", this,
				    SLOT(slotOverrideThread()));
  QObject::connect( mFolderMenu, SIGNAL( aboutToShow() ), 
		    this, SLOT( updateFolderMenu() ));

  //----- Message-Status Submenu
  QPopupMenu *msgStatusMenu = new QPopupMenu;
  connect(msgStatusMenu, SIGNAL(activated(int)), this,
	  SLOT(slotSetMsgStatus(int)));
  msgStatusMenu->insertItem(i18n("New"), (int)KMMsgStatusNew);
  msgStatusMenu->insertItem(i18n("Unread"), (int)KMMsgStatusUnread);
  msgStatusMenu->insertItem(i18n("Read"), (int)KMMsgStatusOld);
  msgStatusMenu->insertItem(i18n("Replied"), (int)KMMsgStatusReplied);
  msgStatusMenu->insertItem(i18n("Queued"), (int)KMMsgStatusQueued);
  msgStatusMenu->insertItem(i18n("Sent"), (int)KMMsgStatusSent);

  //----- Message Menu
  messageMenu = new QPopupMenu;
  QObject::connect( messageMenu, SIGNAL( aboutToShow() ),
		    this, SLOT( updateMessageMenu() ));

  messageMenu->insertItem(i18n("New &Message"), this,
			  SLOT(slotCompose()), KStdAccel::key(KStdAccel::New));
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("&Search"), this,
			  SLOT(slotSearch()), Key_S);
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("&Next"), mHeaders,
			  SLOT(nextMessage()), Key_N);
  messageMenu->insertItem(i18n("Next unread"), mHeaders,
			  SLOT(nextUnreadMessage()), Key_Plus);
  messageMenu->insertItem(i18n("&Previous"), mHeaders,
			  SLOT(prevMessage()), Key_P);
  messageMenu->insertItem(i18n("Previous unread"), mHeaders,
			  SLOT(prevUnreadMessage()), Key_Minus);
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("&Reply..."), this,
			  SLOT(slotReplyToMsg()), Key_R);
  messageMenu->insertItem(i18n("Reply &All..."), this,
			  SLOT(slotReplyAllToMsg()), Key_A);
  messageMenu->insertItem(i18n("&Forward..."), this,
			  SLOT(slotForwardMsg()), Key_F);
  messageMenu->insertItem(i18n("R&edirect..."), this,
			  SLOT(slotRedirectMsg()), Key_E);
  messageMenu->insertItem(i18n("&Bounce..."), this,
			  SLOT(slotBounceMsg()));
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("Edi&t..."),this,
			  SLOT(slotEditMsg()), Key_T);
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("&Set Status"), msgStatusMenu);
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("Mar&k all"), this,
			  SLOT(slotMarkAll()), Key_K);
  moveId = messageMenu->insertItem(i18n("&Move..."), this,
			  SLOT(slotMoveMsg()));
  copyId = messageMenu->insertItem(i18n("&Copy..."), this,
			  SLOT(slotCopyMsg()));
  messageMenu->insertItem(i18n("&Delete"), this,
			  SLOT(slotDeleteMsg()), Key_D);
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("&Undo"), this,
			  SLOT(slotUndo()), KStdAccel::key(KStdAccel::Undo));
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("Send again..."), this,
			  SLOT(slotResendMsg()));
  messageMenu->insertItem(i18n("Apply filters"), this,
			  SLOT(slotApplyFilters()), CTRL+Key_J);
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("View Source..."), this,
			  SLOT(slotShowMsgSrc()));

  //----- View Menu
  mViewMenu = new QPopupMenu();
  mViewMenu->setCheckable(TRUE);
  connect(mViewMenu,SIGNAL(activated(int)),SLOT(slotSetHeaderStyle(int)));
  mViewMenu->insertItem(i18n("&Brief Headers"),
		       KMReaderWin::HdrBrief);
  mViewMenu->insertItem(i18n("&Fancy Headers"),
		       KMReaderWin::HdrFancy);
  mViewMenu->insertItem(i18n("&Standard Headers"),
		       KMReaderWin::HdrStandard);
  mViewMenu->insertItem(i18n("&Long Headers"),
		       KMReaderWin::HdrLong);
  mViewMenu->insertItem(i18n("&All Headers"),
		       KMReaderWin::HdrAll);
  mViewMenu->insertSeparator();
  mViewMenu->insertItem(i18n("Iconic Attachments"),
		       KMReaderWin::HdrAll + KMReaderWin::IconicAttmnt);
  mViewMenu->insertItem(i18n("Smart Attachments"),
		       KMReaderWin::HdrAll + KMReaderWin::SmartAttmnt);
  mViewMenu->insertItem(i18n("Inlined Attachments"),
		       KMReaderWin::HdrAll + KMReaderWin::InlineAttmnt);
  mViewMenu->setItemChecked((int)mMsgView->headerStyle(), TRUE);
  mViewMenu->setItemChecked((int)mMsgView->attachmentStyle()+5, TRUE);

  //----- Help Menu
  QPopupMenu *mHelpMenu = helpMenu(aboutText);

  //----- Menubar
  mMenuBar  = new KMenuBar(this);
  mMenuBar->insertItem(i18n("&File"), fileMenu);
  mMenuBar->insertItem(i18n("&Edit"), editMenu);
  mMenuBar->insertItem(i18n("F&older"), mFolderMenu);
  mMenuBar->insertItem(i18n("&Message"), messageMenu);
  mMenuBar->insertItem(i18n("&View"), mViewMenu);
  mMenuBar->insertSeparator();
  mMenuBar->insertItem(i18n("&Help"), mHelpMenu);

  setMenu(mMenuBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupToolBar()
{
  mToolBar = new KToolBar(this);

#ifdef MALLOC_DEBUG
  mToolBar->insertButton(BarIcon("filenew"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotMemInfo()), TRUE,
			i18n("Malloc info"));
#endif

  mToolBar->insertButton(BarIcon("filenew"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotCompose()), TRUE,
			i18n("Compose new message"));

  mToolBar->insertButton(BarIcon("filesave"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotSaveMsg()), TRUE,
			i18n("Save message to file"));

  mToolBar->insertButton(BarIcon("fileprint"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotPrintMsg()), TRUE,
			i18n("Print message"));

  mToolBar->insertSeparator();

  mToolBar->insertButton(UserIcon("checkmail"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotCheckMail()), TRUE,
			i18n("Get new mail"));
  mToolBar->insertSeparator();

  mToolBar->insertButton(UserIcon("filereply"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotReplyToMsg()), TRUE,
			i18n("Reply to author"));

  mToolBar->insertButton(UserIcon("filereplyall"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotReplyAllToMsg()), TRUE,
			i18n("Reply to all recipients"));

  mToolBar->insertButton(UserIcon("fileforward"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotForwardMsg()), TRUE,
			i18n("Forward message"));

  mToolBar->insertButton(UserIcon("filedel2"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotDeleteMsg()), TRUE,
			i18n("Delete message"));

  mToolBar->insertSeparator();
  mToolBar->insertButton(BarIcon("openbook"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotAddrBook()), TRUE,
			i18n("Open addressbook..."));


  addToolBar(mToolBar);
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
QPopupMenu* KMMainWin::folderToPopupMenu(KMFolderDir* aFolderDir,
					 bool move,
					 QObject *receiver,
					 KMMenuToFolder *aMenuToFolder)
{
  KMFolderNode *folderNode;
  KMFolder* folder;

  QPopupMenu *msgMoveMenu = new QPopupMenu;
  if (move)
    connect(msgMoveMenu, SIGNAL(activated(int)), receiver,
	    SLOT(moveSelectedToFolder(int)));
  else
    connect(msgMoveMenu, SIGNAL(activated(int)), receiver,
	    SLOT(copySelectedToFolder(int)));
  for (folderNode = aFolderDir->first();
       folderNode != NULL;
       folderNode = aFolderDir->next())
    if (!folderNode->isDir()) {
      folder = static_cast<KMFolder*>(folderNode);

      int menuId = msgMoveMenu->insertItem(folder->label());
      aMenuToFolder->insert( menuId, folder );

      KMFolderDir *child = folder->child();
      if (child && child->first()) {
	QPopupMenu *subMenu = folderToPopupMenu( child, move, receiver,
						 aMenuToFolder );
	// add an item to the top of the submenu somehow subMenu
	msgMoveMenu->insertItem(i18n(folder->label() + " child"), subMenu);
      }
    }
  return msgMoveMenu;
}


//-----------------------------------------------------------------------------
void KMMainWin::updateMessageMenu()
{
  int moveIndex = messageMenu->indexOf( moveId );
  int copyIndex = messageMenu->indexOf( copyId );
  messageMenu->removeItem( moveId );
  messageMenu->removeItem( copyId );

  KMFolderDir *dir = &kernel->folderMgr()->dir();
  mMenuToFolder.clear();
  QPopupMenu *msgMoveMenu;
  msgMoveMenu =  folderToPopupMenu( dir, TRUE, this, &mMenuToFolder );
  QPopupMenu *msgCopyMenu;
  msgCopyMenu = folderToPopupMenu( dir, FALSE, this, &mMenuToFolder );
  moveId = messageMenu->insertItem(i18n("&Move to"), msgMoveMenu, -1,
				   moveIndex );
  copyId = messageMenu->insertItem(i18n("&Copy to"), msgCopyMenu, -1,
				   copyIndex );
}


void KMMainWin::updateFolderMenu()
{
  mFolderMenu->setItemEnabled(htmlId, mFolder ? true : false);
  mFolderMenu->setItemEnabled(threadId, mFolder ? true : false);

  if (mHtmlPref && mFolderHtmlPref)
    mFolderMenu->changeItem(htmlId, i18n( "Prefer HTML to plain text (default)" ));
  else if (mHtmlPref && !mFolderHtmlPref)
    mFolderMenu->changeItem(htmlId, i18n( "Prefer plain text to HTML (override default)" ));
  else if (!mHtmlPref && mFolderHtmlPref)
    mFolderMenu->changeItem(htmlId, i18n( "Prefer plain text to HTML (default)" ));
  else if (!mHtmlPref && !mFolderHtmlPref)
    mFolderMenu->changeItem(htmlId, i18n( "Prefer HTML to plain text (override default)" ));

  if (mThreadPref && mFolderThreadPref)
    mFolderMenu->changeItem(threadId, i18n( "Thread messages (default)" ));
  else if (mThreadPref && !mFolderThreadPref)
    mFolderMenu->changeItem(threadId, i18n( "Don't thread messages (override default)" ));
  else if (!mThreadPref && mFolderThreadPref)
    mFolderMenu->changeItem(threadId, i18n( "Don't thread messages (default)" ));
  else if (!mThreadPref && !mFolderThreadPref)
    mFolderMenu->changeItem(threadId, i18n( "Thread messages (override default)" ));
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
  QMessageBox::information(0, "Malloc information", s);
#endif
}
