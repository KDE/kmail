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
#include "kmversion.h"
#include "kmsender.h"

#include <qaccel.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include <kconfig.h>
#include <kapp.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kstdaccel.h>
#include <knewpanner.h>

#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "kmmainwin.moc"


static int windowCount = 0;

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
  connect(mMsgView, SIGNAL(popupMenu(const QPoint&)),
	  this, SLOT(slotMsgPopup(const QPoint&)));
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

  readConfig();

  mVertPanner->setAbsSeperatorPos(mVertPannerSep);
  mHorizPanner->setAbsSeperatorPos(mHorizPannerSep);

  mVertPanner->activate(mHorizPanner, mMsgView);
  mHorizPanner->activate(mFolderTree, mHeaders);

  setupMenuBar();
  setupToolBar();
  setupStatusBar();

  windowCount++;

  // set active folder to `inbox' folder
  idx = mFolderTree->indexOfFolder(inboxFolder);
  if (idx>=0) mFolderTree->setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
KMMainWin::~KMMainWin()
{
  if (mToolBar)   delete mToolBar;
  if (mMenuBar)   delete mMenuBar;
  if (mStatusBar) delete mStatusBar;
}


//-----------------------------------------------------------------------------
void KMMainWin::readConfig()
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
}


//-----------------------------------------------------------------------------
void KMMainWin::writeConfig(bool aWithSync)
{
  QString s;
  KConfig *config = app->getConfig();
  QRect r = geometry();

  mMsgView->writeConfig(FALSE);


  config->setGroup("Geometry");

  s.sprintf("%i,%i", r.width(), r.height());
  config->writeEntry("MainWin", s);

  s.sprintf("%i,%i", mVertPanner->seperatorPos(), 
	    mHorizPanner->seperatorPos());
  config->writeEntry("Panners", s);

  if (aWithSync) config->sync();
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
  kapp->processEvents(300);
}


//-----------------------------------------------------------------------------
void KMMainWin::closeEvent(QCloseEvent *e)
{
  KMMainWinInherited::closeEvent(e);
  writeConfig(FALSE);

  e->accept();
  if (!(--windowCount)) qApp->quit();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotClose() 
{
  close();
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
  KMSettings dlg(this);
  dlg.exec();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotFilter()
{
  filterMgr->openDialog();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotUnimplemented()
{
  warning(nls->translate("Sorry, but this feature\nis still missing"));
}


//-----------------------------------------------------------------------------
void KMMainWin::slotAddFolder() 
{
  KMFolderDialog dlg(NULL, this);

  dlg.setCaption(nls->translate("New Folder"));
  if (dlg.exec()) mFolderTree->reload();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCheckMail() 
{
  kbp->busy();
  acctMgr->checkMail();
  kbp->idle();
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
    warning(nls->translate("Cannot modify a\nsystem folder."));
    return;
  }

  d = new KMFolderDialog((KMFolder*)mFolder, this);
  d->setCaption(nls->translate("Modify Folder"));
  if (d->exec()) mFolderTree->reload();
  delete d;
}


//-----------------------------------------------------------------------------
void KMMainWin::slotEmptyFolder()
{
  kbp->busy();
  mFolder->expunge();
  mHeaders->setFolder(mFolder);
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotRemoveFolder()
{
  QString str;
  QDir dir;

  if (!mFolder) return;
  if (mFolder->isSystemFolder())
  {
    warning(nls->translate("Cannot remove a\nsystem folder."));
    return;
  }

  str.sprintf(nls->translate("Are you sure you want to remove the folder\n"
			     "\"%s\", discarding it's contents ?"),
			     (const char*)mFolder->label());
  if ((KMsgBox::yesNo(this,nls->translate("Confirmation"),str))==1)
  {
    mHeaders->setFolder(NULL);
    folderMgr->remove(mFolder);
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotCompactFolder()
{
  int idx = mHeaders->currentItem();
  if (mFolder) mFolder->compact();
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
void KMMainWin::slotDeleteMsg()
{ 
  mHeaders->deleteMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotShowMsgSrc()
{ 
  KMMessage* msg = mHeaders->getMsg(-1);
  if (msg) msg->viewSource(nls->translate("Message as Plain Text"));
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMoveMsg()
{ 
  KMFolderSelDlg dlg(nls->translate("Select Folder"));
  KMFolder* dest;

  if (!dlg.exec()) return;
  if (!(dest = dlg.folder())) return;

  mHeaders->moveMsgToFolder(dest);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSaveMsg()
{
  mHeaders->saveMsg(-1);
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSendQueued()
{
  if (msgSender->sendQueued())
    statusMsg(nls->translate("Queued messages successfully sent."));
  else
    statusMsg(nls->translate("Failed to send (some) queued messages."));
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
    {mViewMenu->setItemChecked((int)mMsgView->headerStyle(), FALSE);
    mMsgView->setHeaderStyle((KMReaderWin::HeaderStyle)id);
    mViewMenu->setItemChecked(id, TRUE);
    return;
    }
  if(id > 5)
    {mViewMenu->setItemChecked((int)mMsgView->attachmentStyle()+5, FALSE);
    mMsgView->setAttachmentStyle(id-5);
    mViewMenu->setItemChecked(id, TRUE);
    return;
    }
}


//-----------------------------------------------------------------------------
void KMMainWin::folderSelected(KMFolder* aFolder)
{
  kbp->busy();

  mFolder = (KMFolder*)aFolder;
  mHeaders->setFolder(mFolder);
  mMsgView->clear();

  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgSelected(KMMessage *msg)
{
  assert(msg != NULL);
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
    system("kfmclient openURL \""+QString(aUrl)+"\"");
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotMsgPopup(const QPoint& aPoint)
{
  QPopupMenu* menu = new QPopupMenu;

  menu->insertItem(nls->translate("&Reply..."), this, SLOT(slotReplyToMsg()));
  menu->insertItem(nls->translate("Reply &All..."), this, 
		   SLOT(slotReplyAllToMsg()));
  menu->insertItem(nls->translate("&Forward..."), this, 
		   SLOT(slotForwardMsg()), Key_F);
  menu->insertSeparator();
  menu->insertItem(nls->translate("&Move..."), this, 
		   SLOT(slotMoveMsg()), Key_M);
  menu->insertItem(nls->translate("&Copy..."), this, 
			  SLOT(slotCopyText()), Key_S);
  menu->insertItem(nls->translate("&Delete"), this, 
			  SLOT(slotDeleteMsg()), Key_D);
  menu->popup(aPoint, 0);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupMenuBar()
{
  //----- File Menu
  QPopupMenu *fileMenu = new QPopupMenu();
  fileMenu->insertItem(nls->translate("New Composer"), this, 
		       SLOT(slotCompose()), keys->openNew());
  fileMenu->insertItem(nls->translate("New Mailreader"), this, 
		       SLOT(slotNewMailReader()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("Save As..."), this,
		       SLOT(slotSaveMsg()), keys->save());
  fileMenu->insertItem(nls->translate("Print..."), this,
		       SLOT(slotPrintMsg()), keys->print());
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("Check Mail..."), this,
		       SLOT(slotCheckMail()));
  fileMenu->insertItem(nls->translate("Send Queued"), this,
		       SLOT(slotSendQueued()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("&Settings..."), this, 
		       SLOT(slotSettings()));
  fileMenu->insertItem(nls->translate("&Filter..."), this, 
		       SLOT(slotFilter()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("&Close"), this, 
		       SLOT(slotClose()), keys->close());
  fileMenu->insertItem(nls->translate("&Quit"), this,
		       SLOT(quit()), keys->quit());

  //----- Edit Menu
  QPopupMenu *editMenu = new QPopupMenu();
#ifdef BROKEN
  editMenu->insertItem(nls->translate("&Undo"), this, SLOT(slotUnimplemented()),
		       keys->undo());
  editMenu->insertSeparator();
  editMenu->insertItem(nls->translate("&Cut"), this, SLOT(slotUnimplemented()),
		       keys->cut());
#endif
  editMenu->insertItem(nls->translate("&Copy"), this, SLOT(slotCopyText()),
		       keys->copy());
  editMenu->insertSeparator();
  editMenu->insertItem(nls->translate("&Find..."), this, 
		       SLOT(slotUnimplemented()), keys->find());

  //----- Folder Menu
  QPopupMenu *folderMenu = new QPopupMenu();
  folderMenu->insertItem(nls->translate("&Create..."), this, 
			 SLOT(slotAddFolder()));
  folderMenu->insertItem(nls->translate("&Modify..."), this, 
			 SLOT(slotModifyFolder()));
  folderMenu->insertItem(nls->translate("C&ompact"), this, 
			 SLOT(slotCompactFolder()));
  folderMenu->insertItem(nls->translate("&Empty"), this, 
			 SLOT(slotEmptyFolder()));
  folderMenu->insertItem(nls->translate("&Remove"), this, 
			 SLOT(slotRemoveFolder()));

  //----- Message-Status Submenu
  QPopupMenu *msgStatusMenu = new QPopupMenu;
  connect(msgStatusMenu, SIGNAL(activated(int)), this, 
	  SLOT(slotSetMsgStatus(int)));
  msgStatusMenu->insertItem(nls->translate("New"), (int)KMMsgStatusNew);
  msgStatusMenu->insertItem(nls->translate("Unread"), (int)KMMsgStatusUnread);
  msgStatusMenu->insertItem(nls->translate("Read"), (int)KMMsgStatusOld);
  msgStatusMenu->insertItem(nls->translate("Replied"), (int)KMMsgStatusReplied);
  msgStatusMenu->insertItem(nls->translate("Queued"), (int)KMMsgStatusQueued);
  msgStatusMenu->insertItem(nls->translate("Sent"), (int)KMMsgStatusSent);

  //----- Message Menu
  QPopupMenu *messageMenu = new QPopupMenu;
  messageMenu->insertItem(nls->translate("&Next"), mHeaders, 
			  SLOT(nextMessage()), Key_N);
  messageMenu->insertItem(nls->translate("&Previous"), mHeaders, 
			  SLOT(prevMessage()), Key_P);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Reply..."), this,
			  SLOT(slotReplyToMsg()), Key_R);
  messageMenu->insertItem(nls->translate("Reply &All..."), this,
			  SLOT(slotReplyAllToMsg()), Key_A);
  messageMenu->insertItem(nls->translate("&Forward..."), this, 
			  SLOT(slotForwardMsg()), Key_F);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Set Status"), msgStatusMenu);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Move..."), this, 
			  SLOT(slotMoveMsg()), Key_M);
  messageMenu->insertItem(nls->translate("&Copy..."), this, 
			  SLOT(slotUnimplemented()), Key_S);
  messageMenu->insertItem(nls->translate("&Delete"), this, 
			  SLOT(slotDeleteMsg()), Key_D);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("View Source..."), this,
			  SLOT(slotShowMsgSrc()));

  //----- View Menu
  mViewMenu = new QPopupMenu();
  mViewMenu->setCheckable(TRUE);
  connect(mViewMenu,SIGNAL(activated(int)),SLOT(slotSetHeaderStyle(int)));
  mViewMenu->insertItem(nls->translate("&Brief Headers"),
		       KMReaderWin::HdrBrief);
  mViewMenu->insertItem(nls->translate("&Fancy Headers"),
		       KMReaderWin::HdrFancy);
  mViewMenu->insertItem(nls->translate("&Standard Headers"),
		       KMReaderWin::HdrStandard);
  mViewMenu->insertItem(nls->translate("&Long Headers"),
		       KMReaderWin::HdrLong);
  mViewMenu->insertItem(nls->translate("&All Headers"),
		       KMReaderWin::HdrAll);
  mViewMenu->insertSeparator();
  mViewMenu->insertItem(nls->translate("Iconic Attachments"));
  mViewMenu->insertItem(nls->translate("Smart Attachments"));
  mViewMenu->insertItem(nls->translate("Inlined Attachments"));
  mViewMenu->setItemChecked((int)mMsgView->headerStyle(), TRUE);
  mViewMenu->setItemChecked((int)mMsgView->attachmentStyle()+5, TRUE);

  //----- Help Menu
  QPopupMenu *helpMenu = kapp->getHelpMenu(TRUE, aboutText);

  //----- Menubar
  mMenuBar  = new KMenuBar(this);
  mMenuBar->insertItem(nls->translate("&File"), fileMenu);
  mMenuBar->insertItem(nls->translate("&Edit"), editMenu);
  mMenuBar->insertItem(nls->translate("F&older"), folderMenu);
  mMenuBar->insertItem(nls->translate("&Message"), messageMenu);
  mMenuBar->insertItem(nls->translate("&View"), mViewMenu);
  mMenuBar->insertSeparator();
  mMenuBar->insertItem(nls->translate("&Help"), helpMenu);

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
			nls->translate("Compose new message"));

  mToolBar->insertButton(loader->loadIcon("filefloppy.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotUnimplemented()), TRUE,
			nls->translate("Save message to file"));

  mToolBar->insertButton(loader->loadIcon("fileprint.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotPrintMsg()), TRUE,
			nls->translate("Print message"));

  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("checkmail.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotCheckMail()), TRUE,
			nls->translate("Get new mail"));
  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("filereply.xpm"), 0, 
			SIGNAL(clicked()), this, 
			SLOT(slotReplyToMsg()), TRUE,
			nls->translate("Reply to author"));

  mToolBar->insertButton(loader->loadIcon("filereplyall.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotReplyAllToMsg()), TRUE,
			nls->translate("Reply to all recipients"));

  mToolBar->insertButton(loader->loadIcon("fileforward.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotForwardMsg()), TRUE,
			nls->translate("Forward message"));

  mToolBar->insertButton(loader->loadIcon("filedel2.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotDeleteMsg()), TRUE,
			nls->translate("Delete message"));

  addToolBar(mToolBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupStatusBar()
{
  mStatusBar = new KStatusBar(this);

  mMessageStatusId = statusBarAddItem(nls->translate("Initializing..."));
  mStatusBar->enable(KStatusBar::Show);
  setStatusBar(mStatusBar);
}

void KMMainWin::quit()
{
  //if((KMsgBox::yesNo(0,"KMail Confirm","Do you really want to quit?") ==2))
  //  return;
  qApp->quit();
}

