// kmmainwin.cpp

#include <qdir.h>

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

#include <qclipboard.h>
#include <qaccel.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qmessagebox.h>
#include <kconfig.h>
#include <kapp.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kstdaccel.h>
#include <qsplitter.h>
#include <krun.h>
#include <kmenubar.h>
#include <qtimer.h>

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

  idx = mFolderTree->indexOfFolder(inboxFolder);
  if (idx!=0) {
    mFolderTree->setCurrentItem(idx);
    mFolderTree->setSelected(idx,TRUE);
    mHeaders->workAroundQListViewLimitation();
  }
  
  connect(msgSender, SIGNAL(statusMsg(const QString&)),
	  SLOT(statusMsg(const QString&)));
  connect(acctMgr, SIGNAL( newMail(KMAccount *)),
          SLOT( slotNewMail(KMAccount *)));

  // must be the last line of the constructor:
  mStartupDone = TRUE;
}


//-----------------------------------------------------------------------------
KMMainWin::~KMMainWin()
{
  writeConfig();
  
  if (mHeaders)    delete mHeaders;
  if (mToolBar)    delete mToolBar;
  if (mMenuBar)    delete mMenuBar;
  if (mStatusBar)  delete mStatusBar;
  if (mFolderTree) delete mFolderTree;
}


//-----------------------------------------------------------------------------
void KMMainWin::readPreConfig(void)
{
  KConfig *config = app->config();
  QString str;

  config->setGroup("Geometry");
  mLongFolderList = config->readBoolEntry("longFolderList", false);
}


//-----------------------------------------------------------------------------
void KMMainWin::readConfig(void)
{
  KConfig *config = app->config();
  bool oldLongFolderList=false;
  int w, h;
  QString str;

  if (mStartupDone)
  {
    writeConfig();
    oldLongFolderList = mLongFolderList;
    readPreConfig();
    if (oldLongFolderList != mLongFolderList)
    {
      hide();
      if (mHorizPanner->parent()==this) delete mHorizPanner;
      else delete mVertPanner;
      createWidgets();
    }
  }

  config->setGroup("Geometry");
  str = config->readEntry("MainWin", "300,600");
  if (!str.isEmpty() && str.find(',')>=0)
  {
    sscanf(str,"%d,%d",&w,&h);
    resize(w,h);
  }
  str = config->readEntry("Panners", "100,100");
  if ((!str.isEmpty()) && (str.find(',')!=-1))
    sscanf(str,"%d,%d",&((*mHorizPannerSep)[0]),&((*mVertPannerSep)[0]));
  else
    (*mHorizPannerSep)[0] = (*mVertPannerSep)[0] = 100;
  (*mHorizPannerSep)[1] = h - (*mHorizPannerSep)[0];
  (*mVertPannerSep)[1] = w - (*mVertPannerSep)[0];
  mSendOnCheck = config->readBoolEntry("SendOnCheck",false);

  mMsgView->readConfig();
  mHeaders->readConfig();
  mFolderTree->readConfig();

  // Re-activate panners
  if (mStartupDone)
  {
    if (oldLongFolderList != mLongFolderList)
      activatePanners();
    kbp->busy();
    mFolderTree->reload();
    QListViewItem *qlvi = mFolderTree->indexOfFolder(mFolder);
    if (qlvi!=0) {
      mFolderTree->setCurrentItem(qlvi);
      mFolderTree->setSelected(qlvi,TRUE);
    }
    mMsgView->setMsg( mMsgView->msg(), TRUE );
    mHeaders->setFolder(mFolder);
    kbp->idle();
    show();
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::writeConfig(void)
{
  QString s;
  KConfig *config = app->config();
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
  connect(mMsgView, SIGNAL(popupMenu(const char*,const QPoint&)),
	  this, SLOT(slotMsgPopup(const char*,const QPoint&)));
  connect(mMsgView, SIGNAL(urlClicked(const char*,int)),
	  this, SLOT(slotUrlClicked(const char*,int)));
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
}


//-----------------------------------------------------------------------------
int KMMainWin::statusBarAddItem(const char* aText)
{
  return mStatusBar->insertItem(aText, -1);
}


//-----------------------------------------------------------------------------
void KMMainWin::statusBarChangeItem(int aId, const char* aText)
{
  mStatusBar->changeItem(aText, aId);
}


//-----------------------------------------------------------------------------
void KMMainWin::statusMsg(const QString& aText)
{
  mStatusBar->changeItem(aText, mMessageStatusId);
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


//-----------------------------------------------------------------------------
void KMMainWin::slotHelp()
{
  app->invokeHTMLHelp("","");
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
  // markus: we write the Config here cause otherwise the
  // geometry will be set to the value in the config.
  // Problem arises when we change the geometry during the
  // session are press the OK button in the settings. Then we
  // lose the current geometry! Not anymore ;-)
  writeConfig();
  KMSettings dlg(this);
  dlg.exec();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotFilter()
{
  filterMgr->openDialog();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotAddrBook()
{
  KMAddrBookEditDlg dlg(addrBook);
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

  d = new KMFolderDialog(0, &(folderMgr->dir()), 
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
  bool rc;


 if(checkingMail) 
 {
    QMessageBox::information(0,i18n("KMail error"),
		     i18n("Already checking for mail!"));
    return;
  }
    
 checkingMail = TRUE;
 
 kbp->busy();
 rc = acctMgr->checkMail(FALSE);
 kbp->idle();
 
 if (!rc) statusMsg(i18n("No new mail available"));
 
 if(mSendOnCheck) slotSendQueued();
 checkingMail = FALSE;
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
  bool rc = FALSE;


  if(checkingMail)
  {
    QMessageBox::message(0,i18n("KMail error"),
		     i18n("Already checking for mail!"));
    return;
  }
    
  checkingMail = TRUE;
   
  kbp->busy();
  rc = acctMgr->intCheckMail(item);
  kbp->idle();
  
  if (!rc) warning(i18n("No new mail available"));
  if(mSendOnCheck)
    slotSendQueued();

  checkingMail = FALSE; 

}

void KMMainWin::slotNewMail(KMAccount *) {
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

  kbp->busy();

  // begin of critical part
  // from here to "end..." no signal may change to another mFolder, otherwise
  // the wrong folder will be truncated in expunge (dnaber, 1999-08-29)
  mFolder->open();
  mHeaders->setFolder(NULL);
  mMsgView->clear();

  if (mFolder != trashFolder)
  {
    // FIXME: If we run out of disk space mail may be lost rather
    // than moved into the trash -sanders
    while ((msg = mFolder->take(0)) != NULL)
      trashFolder->addMsg(msg);
  }

  mFolder->close();
  mFolder->expunge();
  // end of critical
  if (mFolder != trashFolder)
    statusMsg(i18n("Moved all messages into trash"));

  mHeaders->setFolder(mFolder);
  kbp->idle();
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
			     "\"%1\", discarding it's contents ?")
			     .arg(mFolder->label());
  if ((QMessageBox::information(this,i18n("Confirmation"),str, 
				i18n( "Yes" ), i18n( "No" ), 0, 0, 1))==0)
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
    folderMgr->remove(folderToDelete);
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCompactFolder()
{
  int idx = mHeaders->currentItemIndex();
  if (mFolder)
  {
    kbp->busy();
    mFolder->compact();
    kbp->idle();
  }
  mHeaders->setCurrentItemByIndex(idx);
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
void KMMainWin::slotEditMsg() 
{
  KMMessage *msg;
  int aIdx;
  
  if(mFolder != outboxFolder) 
    {
      QMessageBox::information(0,i18n("KMail notification!"),
		       i18n("Only messages in the outbox folder can be edited!"));
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
void KMMainWin::slotShowMsgSrc()
{ 
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg) msg->viewSource(i18n("Message as Plain Text"));
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMoveMsg()
{ 
  KMFolderSelDlg dlg(i18n("Select Folder"));
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
  KMFolderSelDlg dlg(i18n("Select Folder"));
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
  if (msgSender->sendQueued())
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
}


//-----------------------------------------------------------------------------
void KMMainWin::folderSelected(KMFolder* aFolder)
{
  if (mFolder == aFolder)
    return;

  kbp->busy();
  mFolder = (KMFolder*)aFolder;
  mMsgView->clear();
  mHeaders->setFolder(mFolder);
  kbp->idle();
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

  connect(win, SIGNAL(statusMsg(const char*)),
          this, SLOT(statusMsg(const char*)));
  connect(win, SIGNAL(popupMenu(const char*,const QPoint&)),
          this, SLOT(slotMsgPopup(const char*,const QPoint&)));
  connect(win, SIGNAL(urlClicked(const char*,int)),
          this, SLOT(slotUrlClicked(const char*,int)));

  QAccel *accel = new QAccel(win);
  accel->connectItem(accel->insertItem(Key_Up),
                     win, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Key_Down),
                     win, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Key_Prior),
                     win, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Key_Next),
                     win, SLOT(slotScrollNext()));
  win->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCopyText()
{
  QString temp;
  temp = mMsgView->copyText();
  app->clipboard()->setText(temp);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotMarkAll() {
  QListViewItem *item;
  for (item = mHeaders->firstChild(); item; item = item->nextSibling())
    mHeaders->setSelected( item, TRUE );
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUrlClicked(const char* aUrl, int)
{
  KMComposeWin *win;
  KMMessage* msg;

  if (!strnicmp(aUrl, "mailto:", 7))
  {
    msg = new KMMessage;
    msg->initHeader();
    msg->setTo(aUrl+7);

    win = new KMComposeWin(msg);
    win->show();
  }
  else if (!strnicmp(aUrl, "http:", 5) || !strnicmp(aUrl, "ftp:", 4) ||
	   !strnicmp(aUrl, "file:", 5))
  {
    statusMsg(i18n("Opening URL..."));
    // -- David : replacement for KFM::openURL
    (void) new KRun(aUrl);
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
  if (mUrlCurrent.isEmpty()) return;
  addrBook->insert(mUrlCurrent.mid(7,255));
  statusMsg(i18n("Address added to addressbook."));
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
  mMsgView->slotUrlOpen(mUrlCurrent, QString::null, 0);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgPopup(const char* aUrl, const QPoint& aPoint)
{
  QPopupMenu* menu = new QPopupMenu;

  mUrlCurrent = aUrl;
  

  if (aUrl)
  {
    if (strnicmp(aUrl,"mailto:",7)==0)
    {
      // popup on a mailto URL
      menu = new QPopupMenu();
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
    menu->insertSeparator();
    menu->insertItem(i18n("&Move..."), this, 
		     SLOT(slotMoveMsg()), Key_M);
    menu->insertItem(i18n("&Copy..."), this, 
		     SLOT(slotCopyText()), Key_S);
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
  actList = acctMgr->getAccounts();
  QString tmp;
  for(tmp = actList.first(); tmp ; tmp = actList.next())
    actMenu->insertItem(tmp);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupMenuBar()
{
  //----- File Menu
  fileMenu = new QPopupMenu();
  fileMenu->insertItem(i18n("New Composer"), this, 
		       SLOT(slotCompose()), KStdAccel::openNew());
  fileMenu->insertItem(i18n("New Mailreader"), this, 
		       SLOT(slotNewMailReader()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("Save As..."), this,
		       SLOT(slotSaveMsg()), KStdAccel::save());
  fileMenu->insertItem(i18n("Print..."), this,
		       SLOT(slotPrintMsg()), KStdAccel::print());
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("Compact all folders"), folderMgr,
                        SLOT(compactAll()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("Check Mail..."), this,
		       SLOT(slotCheckMail()), CTRL+Key_L);
  actMenu = new QPopupMenu();

  getAccountMenu();

  connect(actMenu,SIGNAL(activated(int)),this,SLOT(slotCheckOneAccount(int)));
  connect(fileMenu,SIGNAL(highlighted(int)),this,SLOT(slotMenuActivated()));

  fileMenu->insertItem(i18n("Check Mail in..."),actMenu);

  fileMenu->insertItem(i18n("Send Queued"), this,
		       SLOT(slotSendQueued()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("&Settings..."), this, 
		       SLOT(slotSettings()));
  fileMenu->insertItem(i18n("&Addressbook..."), this, 
		       SLOT(slotAddrBook()));
  fileMenu->insertItem(i18n("&Filter..."), this, 
		       SLOT(slotFilter()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("&Close"), this, 
		       SLOT(slotClose()), KStdAccel::close());
  fileMenu->insertItem(i18n("&Quit"), this,
		       SLOT(quit()), KStdAccel::quit());

  //----- Edit Menu
  QPopupMenu *editMenu = new QPopupMenu();
  editMenu->insertItem(i18n("&Copy"), this, SLOT(slotCopyText()),
		       KStdAccel::copy());
  editMenu->insertSeparator();
#ifdef BROKEN
  editMenu->insertItem(i18n("&Find..."), this, 
		       SLOT(slotUnimplemented()), KStdAccel::find());
#endif
  //----- Folder Menu
  QPopupMenu *folderMenu = new QPopupMenu();
  folderMenu->insertItem(i18n("&Create..."), this, 
			 SLOT(slotAddFolder()));
  folderMenu->insertItem(i18n("&Modify..."), this, 
			 SLOT(slotModifyFolder()));
  folderMenu->insertItem(i18n("C&ompact"), this, 
			 SLOT(slotCompactFolder()));
  folderMenu->insertSeparator();
  folderMenu->insertItem(i18n("&Empty"), this, 
			 SLOT(slotEmptyFolder()));
  folderMenu->insertItem(i18n("&Remove"), this, 
			 SLOT(slotRemoveFolder()));

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
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("Edi&t..."),this,
			  SLOT(slotEditMsg()), Key_T);
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("&Set Status"), msgStatusMenu);
  messageMenu->insertSeparator();
  messageMenu->insertItem(i18n("Mar&k all"), this, 
			  SLOT(slotMarkAll()), Key_K);
  moveId = messageMenu->insertItem(i18n("&Move..."), this, 
			  SLOT(slotMoveMsg()), Key_M);
  copyId = messageMenu->insertItem(i18n("&Copy..."), this, 
			  SLOT(slotCopyMsg()), Key_S);
  messageMenu->insertItem(i18n("&Delete"), this, 
			  SLOT(slotDeleteMsg()), Key_D);
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
  mMenuBar->insertItem(i18n("F&older"), folderMenu);
  mMenuBar->insertItem(i18n("&Message"), messageMenu);
  mMenuBar->insertItem(i18n("&View"), mViewMenu);
  mMenuBar->insertSeparator();
  mMenuBar->insertItem(i18n("&Help"), mHelpMenu);

  setMenu(mMenuBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupToolBar()
{
  KIconLoader* loader = KGlobal::iconLoader();

  mToolBar = new KToolBar(this);

  mToolBar->insertButton(loader->loadIcon("filenew"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotCompose()), TRUE, 
			i18n("Compose new message"));

  mToolBar->insertButton(loader->loadIcon("filefloppy"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotSaveMsg()), TRUE,
			i18n("Save message to file"));

  mToolBar->insertButton(loader->loadIcon("fileprint"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotPrintMsg()), TRUE,
			i18n("Print message"));

  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("checkmail"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotCheckMail()), TRUE,
			i18n("Get new mail"));
  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("filereply"), 0, 
			SIGNAL(clicked()), this, 
			SLOT(slotReplyToMsg()), TRUE,
			i18n("Reply to author"));

  mToolBar->insertButton(loader->loadIcon("filereplyall"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotReplyAllToMsg()), TRUE,
			i18n("Reply to all recipients"));

  mToolBar->insertButton(loader->loadIcon("fileforward"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotForwardMsg()), TRUE,
			i18n("Forward message"));

  mToolBar->insertButton(loader->loadIcon("filedel2"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotDeleteMsg()), TRUE,
			i18n("Delete message"));

  mToolBar->insertSeparator();
  mToolBar->insertButton(loader->loadIcon("openbook"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotAddrBook()), TRUE,
			i18n("Open addressbook..."));


  addToolBar(mToolBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupStatusBar()
{
  mStatusBar = new KStatusBar(this);

  mMessageStatusId = statusBarAddItem(i18n("Initializing..."));
  mStatusBar->enable(KStatusBar::Show);
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
  
  KMFolderDir *dir = &folderMgr->dir();
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

