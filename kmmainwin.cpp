// kmmainwin.cpp

#include <errno.h>
#include <assert.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qdir.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include <kconfig.h>
#include <kapp.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kstdaccel.h>
#include <knewpanner.h>
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

#include "kmmainwin.moc"

static int windowCount = 0;


//-----------------------------------------------------------------------------
KMMainWin::KMMainWin(QWidget *, char *name) :
  KMMainWinInherited(name)
{
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

  mMsgView = new KMReaderWin(mVertPanner);
  connect(mMsgView, SIGNAL(statusMsg(const char*)),
	  this, SLOT(statusMsg(const char*)));

  parseConfiguration();

  mVertPanner->setAbsSeperatorPos(mVertPannerSep);
  mHorizPanner->setAbsSeperatorPos(mHorizPannerSep);

  mVertPanner->activate(mHorizPanner, mMsgView);
  mHorizPanner->activate(mFolderTree, mHeaders);

  setupMenuBar();
  setupToolBar();
  setupStatusBar();

  windowCount++;

  folderSelected(inboxFolder);
}


//-----------------------------------------------------------------------------
KMMainWin::~KMMainWin()
{
  if (mToolBar)   delete mToolBar;
  if (mMenuBar)   delete mMenuBar;
  if (mStatusBar) delete mStatusBar;
}


//-----------------------------------------------------------------------------
void KMMainWin::parseConfiguration()
{
  int x, y, w, h;
  KConfig *config = app->getConfig();
  QString str;

  config->setGroup("Geometry");
  str = config->readEntry("Main", "20,20,300,600");
  if ((!str.isEmpty()) && (str.find(',')!=-1))
  {
    sscanf(str,"%d,%d,%d,%d",&x,&y,&w,&h);
    setGeometry(x,y,w,h);
  }
  str = config->readEntry("Panners", "100,100");
  if ((!str.isEmpty()) && (str.find(',')!=-1))
    sscanf(str,"%d,%d",&mVertPannerSep,&mHorizPannerSep);
  else
    mHorizPannerSep = mVertPannerSep = 100;
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
void KMMainWin::show(void)
{
  KMMainWinInherited::show();
  resize(size());
}


//-----------------------------------------------------------------------------
void KMMainWin::closeEvent(QCloseEvent *e)
{
  QString s;
  KConfig *config = app->getConfig();
  QRect r = geometry();

  KTopLevelWidget::closeEvent(e);

  config->setGroup("Geometry");

  s.sprintf("%i,%i,%i,%i",r.x()-6,r.y()-24,r.width(),r.height());
  config->writeEntry("Main", s);

  s.sprintf("%i,%i", mVertPanner->seperatorPos(), 
	    mHorizPanner->seperatorPos());
  config->writeEntry("Panners", s);

  config->sync();
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

  str.sprintf(nls->translate("Are you sure you want to remove the folder \""
			     "%s\"\nand all of its child folders?"),
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
  if (mFolder) mFolder->compact();
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
void KMMainWin::slotViewChange()
{
  if(bodyParts->isItemChecked(bodyParts->idAt(0)))
  {
    bodyParts->setItemChecked(bodyParts->idAt(0),FALSE);
    bodyParts->setItemChecked(bodyParts->idAt(1),TRUE);
  }
  else if(bodyParts->isItemChecked(bodyParts->idAt(1)))
  {
    bodyParts->setItemChecked(bodyParts->idAt(1),FALSE);
    bodyParts->setItemChecked(bodyParts->idAt(0),TRUE);
  }

  //mMsgView->setInline(!mMsgView->isInline());
}


//-----------------------------------------------------------------------------
void KMMainWin::slotSetHeaderStyle(int id)
{
  if (!mMsgView) return;

  mViewMenu->setItemChecked((int)mMsgView->headerStyle(), FALSE);
  mMsgView->setHeaderStyle((KMReaderWin::HeaderStyle)id);
  mViewMenu->setItemChecked(id, TRUE);
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
void KMMainWin::slotMsgActivated(KMMessage *msg)
{
  KMReaderWin *win;

  assert(msg != NULL);
  debug("KMMainWin::slotMsgActivated() called");
  win = new KMReaderWin;
  win->setCaption(msg->subject());
  win->resize(400,600);
  win->setMsg(msg);
  win->show();
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
  fileMenu->insertItem(nls->translate("&Settings..."), this, 
		       SLOT(slotSettings()));
  fileMenu->insertItem(nls->translate("&Filter..."), this, 
		       SLOT(slotFilter()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("&Close"), this, 
		       SLOT(slotClose()), keys->close());
  fileMenu->insertItem(nls->translate("&Quit"), qApp,
		       SLOT(quit()), keys->quit());

  //----- Edit Menu
  QPopupMenu *editMenu = new QPopupMenu();
  editMenu->insertItem(nls->translate("&Undo"), this, SLOT(slotUnimplemented()),
		       keys->undo());
  editMenu->insertSeparator();
  editMenu->insertItem(nls->translate("&Cut"), this, SLOT(slotUnimplemented()),
		       keys->cut());
  editMenu->insertItem(nls->translate("&Copy"), this, SLOT(slotUnimplemented()),
		       keys->copy());
  editMenu->insertItem(nls->translate("&Paste"),this, SLOT(slotUnimplemented()),
		       keys->paste());
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

  //----- Message Menu
  QPopupMenu *messageMenu = new QPopupMenu();
#ifdef BROKEN
  messageMenu->insertItem(nls->translate("&Next"), mainView, 
			  SLOT(slotNextMsg()), Key_N);
  messageMenu->insertItem(nls->translate("&Previous"), mainView, 
			  SLOT(slotPreviousMsg()), Key_P);
  messageMenu->insertSeparator();
#endif //BROKEN
  messageMenu->insertItem(nls->translate("Toggle All &Headers"), this, 
			  SLOT(slotUnimplemented()), Key_H);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Reply..."), this,
			  SLOT(slotReplyToMsg()), Key_R);
  messageMenu->insertItem(nls->translate("Reply &All..."), this,
			  SLOT(slotReplyAllToMsg()), Key_A);
  messageMenu->insertItem(nls->translate("&Forward..."), this, 
			  SLOT(slotForwardMsg()), Key_F);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Move..."), this, 
			  SLOT(slotMoveMsg()), Key_M);
  messageMenu->insertItem(nls->translate("&Copy..."), this, 
			  SLOT(slotUnimplemented()), Key_S);
  messageMenu->insertItem(nls->translate("&Delete"), this, 
			  SLOT(slotDeleteMsg()), Key_D);
#ifdef BROKEN
  messageMenu->insertItem(nls->translate("&Undelete"), this,
			  SLOT(slotUnimplemented()), Key_U);
#endif
  messageMenu->insertSeparator();

  bodyParts = new QPopupMenu();
  bodyParts->setCheckable(TRUE);
  bodyParts->insertItem(nls->translate("Inline"), this,
			SLOT(slotViewChange()));
  bodyParts->insertItem(nls->translate("Separate"), this,
			SLOT(slotViewChange()));

  printf("mainwin %i\n",showInline);
  if(showInline)
    bodyParts->setItemChecked(bodyParts->idAt(0),TRUE);
  else
    bodyParts->setItemChecked(bodyParts->idAt(1),TRUE);

  messageMenu->insertItem(nls->translate("View Body Parts..."),bodyParts);

  messageMenu->insertItem(nls->translate("&Export..."), this, 
			  SLOT(slotUnimplemented()), Key_E);
  messageMenu->insertItem(nls->translate("Pr&int..."), this,
			  SLOT(slotPrintMsg()), keys->print());
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

  //----- Help Menu
  QPopupMenu *helpMenu = kapp->getHelpMenu(TRUE, aboutText);

  //----- Menubar
  mMenuBar  = new KMenuBar(this);
  mMenuBar->insertItem(nls->translate("File"), fileMenu);
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
