// kmmainwin.cpp

#include <qdir.h>

#include "kmfoldermgr.h"
#include "kmsettings.h"
#include "kmfolderdia.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kbusyptr.h"
#include "kmmessage.h"
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

#include <qclipbrd.h>
#include <qaccel.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include <kconfig.h>
#include <kapp.h>
#include <kapp.h>
#include <kiconloader.h>
#include <kstdaccel.h>
#include <knewpanner.h>

#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "kmmainwin.moc"


//-----------------------------------------------------------------------------
KMMainWin::KMMainWin(QWidget *, char *name) :
  KMMainWinInherited(name)
{
  QAccel *accel = new QAccel(this);
  int idx;

  mIntegrated = TRUE;
  mFolder     = NULL;

  setMinimumSize(400, 300);

  mVertPanner  = new KNewPanner(this, "vertPanner", KNewPanner::Horizontal,
				KNewPanner::Absolute);
  mVertPanner->resize(size());
  setView(mVertPanner);

  mHorizPanner = new KNewPanner(mVertPanner,"horizPanner",KNewPanner::Vertical,
				KNewPanner::Absolute);

  mFolderTree  = new KMFolderTree(mHorizPanner, "folderTree");
  connect(mFolderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));

  mHeaders = new KMHeaders(this, mHorizPanner, "headers");
  connect(mHeaders, SIGNAL(selected(KMMessage*)),
	  this, SLOT(slotMsgSelected(KMMessage*)));
  connect(mHeaders, SIGNAL(activated(KMMessage*)),
	  this, SLOT(slotMsgActivated(KMMessage*)));
  accel->connectItem(accel->insertItem(Key_Left),
		     mHeaders, SLOT(prevMessage()));
  accel->connectItem(accel->insertItem(Key_Right), 
		     mHeaders, SLOT(nextMessage()));

  mMsgView = new KMReaderWin(mVertPanner);
  connect(mMsgView, SIGNAL(statusMsg(const char*)),
	  this, SLOT(statusMsg(const char*)));
  connect(mMsgView, SIGNAL(popupMenu(const char*,const QPoint&)),
	  this, SLOT(slotMsgPopup(const char*,const QPoint&)));
  connect(mMsgView, SIGNAL(urlClicked(const char*,int)),
	  this, SLOT(slotUrlClicked(const char*,int)));
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

  readConfig();

  mVertPanner->activate(mHorizPanner, mMsgView);
  mHorizPanner->activate(mFolderTree, mHeaders);

  // now adjust panner positions
  mVertPanner->setAbsSeparatorPos(mVertPannerSep);
  mHorizPanner->setAbsSeparatorPos(mHorizPannerSep);

  setupMenuBar();
  setupToolBar();
  setupStatusBar();

  // set active folder to `inbox' folder
  idx = mFolderTree->indexOfFolder(inboxFolder);
  if (idx>=0) mFolderTree->setCurrentItem(idx);

  connect(msgSender, SIGNAL(statusMsg(const char*)),
	  SLOT(statusMsg(const char*)));
}


//-----------------------------------------------------------------------------
KMMainWin::~KMMainWin()
{
  debug("~KMMainWin()");

  writeConfig();

  if (mHeaders)   delete mHeaders;
  if (mToolBar)   delete mToolBar;
  if (mMenuBar)   delete mMenuBar;
  if (mStatusBar) delete mStatusBar;
}


//-----------------------------------------------------------------------------
void KMMainWin::readConfig(void)
{
  KConfig *config = app->getConfig();
  int w, h;
  QString str;

  config->setGroup("Geometry");
  str = config->readEntry("MainWin", "300,600");
  if (!str.isEmpty() && str.find(',')>=0)
  {
    sscanf(str,"%d,%d",&w,&h);
    resize(w,h);
  }
  str = config->readEntry("Panners", "100,100");
  if ((!str.isEmpty()) && (str.find(',')!=-1))
    sscanf(str,"%d,%d",&mVertPannerSep,&mHorizPannerSep);
  else
    mHorizPannerSep = mVertPannerSep = 100;
  
  config->setGroup("General");
  mSendOnCheck = config->readBoolEntry("SendOnCheck",false);
  

  mMsgView->readConfig();
  mHeaders->readConfig();
}


//-----------------------------------------------------------------------------
void KMMainWin::writeConfig(void)
{
  QString s(32);
  KConfig *config = app->getConfig();
  QRect r = geometry();

  mMsgView->writeConfig();

  config->setGroup("Geometry");

  s.sprintf("%i,%i", r.width(), r.height());
  config->writeEntry("MainWin", s);

  s.sprintf("%i,%i", mVertPanner->separatorPos(), 
	    mHorizPanner->separatorPos());
  config->writeEntry("Panners", s);
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
void KMMainWin::statusMsg(const char* aText)
{
  mStatusBar->changeItem(aText, mMessageStatusId);
  kapp->flushX();
  kapp->processEvents(100);
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
  KMFolderDialog dlg(NULL, this);

  dlg.setCaption(i18n("New Folder"));
  if (dlg.exec()) mFolderTree->reload();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCheckMail() 
{
  bool rc;


 if(checkingMail) 
 {
    KMsgBox::message(0,i18n("KMail error"),
		     i18n("Already checking for mail!"));
    return;
  }
    
 checkingMail = TRUE;
 
 kbp->busy();
 rc = acctMgr->checkMail();
 kbp->idle();
 
 if (!rc) statusMsg(i18n("No new mail available"));
 
 if(mSendOnCheck) slotSendQueued();
 checkingMail = FALSE;
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMenuActivated()
{
  getAccountMenu();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCheckOneAccount(int item)
{
  bool rc = FALSE;


  if(checkingMail)
  {
    KMsgBox::message(0,i18n("KMail error"),
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
  if (mFolder->isSystemFolder())
  {
    warning(i18n("Cannot modify a\nsystem folder."));
    return;
  }

  d = new KMFolderDialog((KMFolder*)mFolder, this);
  d->setCaption(i18n("Modify Folder"));
  if (d->exec()) mFolderTree->reload();
  delete d;
}


//-----------------------------------------------------------------------------
void KMMainWin::slotEmptyFolder()
{
  QString str(256);
  KMMessage* msg;

  if (!mFolder) return;

#ifdef NOT_REQUIRED_ANYMORE
  str.sprintf(i18n("Are you sure you want to discard the\n"
		   "contents of the folder \"%s\" ?"),
	      (const char*)mFolder->label());
  if ((KMsgBox::yesNo(this,i18n("Confirmation"),str))==1)
  {
#endif
    kbp->busy();
    mFolder->open();
    mHeaders->setFolder(NULL);
    mMsgView->clear();

    if (mFolder != trashFolder)
    {
      while ((msg = mFolder->take(0)) != NULL)
	trashFolder->addMsg(msg);
      statusMsg(i18n("Moved all messages into trash"));
    }
    mFolder->close();
    mFolder->expunge();
    mHeaders->setFolder(mFolder);
    kbp->idle();

//}
}


//-----------------------------------------------------------------------------
void KMMainWin::slotRemoveFolder()
{
  QString str(256);
  QDir dir;

  if (!mFolder) return;
  if (mFolder->isSystemFolder())
  {
    warning(i18n("Cannot remove a\nsystem folder."));
    return;
  }

  str.sprintf(i18n("Are you sure you want to remove the folder\n"
			     "\"%s\", discarding it's contents ?"),
			     (const char*)mFolder->label());
  if ((KMsgBox::yesNo(this,i18n("Confirmation"),str, KMsgBox::DB_SECOND))==1)
  {
    mHeaders->setFolder(NULL);
    mMsgView->clear();
    folderMgr->remove(mFolder);
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCompactFolder()
{
  int idx = mHeaders->currentItem();
  if (mFolder)
  {
    kbp->busy();
    mFolder->compact();
    kbp->idle();
  }
  mHeaders->setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotPrintMsg()
{ 
  if(mHeaders->currentItem() >= 0)
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
      KMsgBox::message(0,i18n("KMail notification!"),
		       i18n("Only messages in the outbox folder can be edited!"));
      return;
    }
    
  
  if((aIdx = mHeaders->currentItem()) <= -1)
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
  if(mHeaders->currentItem() == -1)
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
  if (!mMsgView) return;
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
  cout << "Entering folderSelected\n";
  if(!aFolder)
    {
      debug("KMMainWin::folderSelected(): aFolder == NULL");
      return;
    }
    
  kbp->busy();
  mFolder = (KMFolder*)aFolder;
  mMsgView->clear();
  mHeaders->setFolder(mFolder);

  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgSelected(KMMessage *msg)
{
  //assert(msg != NULL);
  if(msg == NULL)
    return;
  mMsgView->setMsg(msg);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSetMsgStatus(int id)
{
  mHeaders->setMsgStatus((KMMsgStatus)id);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgActivated(KMMessage *msg)
{
  KMReaderWin *win;

  assert(msg != NULL);

  win = new KMReaderWin;
  win->setCaption(msg->subject());
  win->resize(550,600);
  win->setMsg(msg);
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

  int i;
  for(i = 0; i < mHeaders->numRows(); i++) 
    mHeaders->markItem(i);

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
    system("kfmclient openURL \""+QString(aUrl)+"\"");
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
  mMsgView->slotUrlOpen(mUrlCurrent,0);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgPopup(const char* aUrl, const QPoint& aPoint)
{
  QPopupMenu* menu = new QPopupMenu;

  mUrlCurrent = aUrl;
  mUrlCurrent.detach();

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
		       SLOT(slotCompose()), keys->openNew());
  fileMenu->insertItem(i18n("New Mailreader"), this, 
		       SLOT(slotNewMailReader()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(i18n("Save As..."), this,
		       SLOT(slotSaveMsg()), keys->save());
  fileMenu->insertItem(i18n("Print..."), this,
		       SLOT(slotPrintMsg()), keys->print());
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
		       SLOT(slotClose()), keys->close());
  fileMenu->insertItem(i18n("&Quit"), this,
		       SLOT(quit()), keys->quit());

  //----- Edit Menu
  QPopupMenu *editMenu = new QPopupMenu();
  editMenu->insertItem(i18n("&Copy"), this, SLOT(slotCopyText()),
		       keys->copy());
  editMenu->insertSeparator();
#ifdef BROKEN
  editMenu->insertItem(i18n("&Find..."), this, 
		       SLOT(slotUnimplemented()), keys->find());
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
  QPopupMenu *messageMenu = new QPopupMenu;
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
  messageMenu->insertItem(i18n("&Move..."), this, 
			  SLOT(slotMoveMsg()), Key_M);
  messageMenu->insertItem(i18n("&Copy..."), this, 
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
  mViewMenu->insertItem(i18n("Iconic Attachments"));
  mViewMenu->insertItem(i18n("Smart Attachments"));
  mViewMenu->insertItem(i18n("Inlined Attachments"));
  mViewMenu->setItemChecked((int)mMsgView->headerStyle(), TRUE);
  mViewMenu->setItemChecked((int)mMsgView->attachmentStyle()+5, TRUE);

  //----- Help Menu
  QPopupMenu *helpMenu = kapp->getHelpMenu(TRUE, aboutText);

  //----- Menubar
  mMenuBar  = new KMenuBar(this);
  mMenuBar->insertItem(i18n("&File"), fileMenu);
  mMenuBar->insertItem(i18n("&Edit"), editMenu);
  mMenuBar->insertItem(i18n("F&older"), folderMenu);
  mMenuBar->insertItem(i18n("&Message"), messageMenu);
  mMenuBar->insertItem(i18n("&View"), mViewMenu);
  mMenuBar->insertSeparator();
  mMenuBar->insertItem(i18n("&Help"), helpMenu);

  setMenu(mMenuBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupToolBar()
{
  KIconLoader* loader = kapp->getIconLoader();

  mToolBar = new KToolBar(this);

  mToolBar->insertButton(loader->loadIcon("filenew.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotCompose()), TRUE, 
			i18n("Compose new message"));

  mToolBar->insertButton(loader->loadIcon("filefloppy.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotSaveMsg()), TRUE,
			i18n("Save message to file"));

  mToolBar->insertButton(loader->loadIcon("fileprint.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotPrintMsg()), TRUE,
			i18n("Print message"));

  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("checkmail.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotCheckMail()), TRUE,
			i18n("Get new mail"));
  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("filereply.xpm"), 0, 
			SIGNAL(clicked()), this, 
			SLOT(slotReplyToMsg()), TRUE,
			i18n("Reply to author"));

  mToolBar->insertButton(loader->loadIcon("filereplyall.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotReplyAllToMsg()), TRUE,
			i18n("Reply to all recipients"));

  mToolBar->insertButton(loader->loadIcon("fileforward.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotForwardMsg()), TRUE,
			i18n("Forward message"));

  mToolBar->insertButton(loader->loadIcon("filedel2.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotDeleteMsg()), TRUE,
			i18n("Delete message"));

  mToolBar->insertSeparator();
  mToolBar->insertButton(loader->loadIcon("openbook.xpm"), 0, 
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
  //if((KMsgBox::yesNo(0,"KMail Confirm","Do you really want to quit?") ==2))
  //  return;
  qApp->quit();
}

