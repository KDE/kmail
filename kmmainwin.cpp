// kmmainwin.cpp

#include <errno.h>
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
#include <kshortcut.h>
#include "knewpanner.h"

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

#include "kmmainwin.moc"

static int windowCount = 0;


//-----------------------------------------------------------------------------
KMMainWin::KMMainWin(QWidget *, char *name) :
  KMMainWinInherited(name)
{
  mIntegrated = TRUE;
  mFolder     = NULL;

  mVertPanner  = new KNewPanner(this, "vertPanner",
				KNewPanner::Horizontal, KNewPanner::Absolute);

  mHorizPanner = new KNewPanner(mVertPanner,"horizPanner",
				KNewPanner::Vertical, KNewPanner::Absolute);

  mFolderTree  = new KMFolderTree(mHorizPanner, "folderTree");
  connect(mFolderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));

  mHeaders = new KMHeaders(this, mHorizPanner, "headers");
  connect(mHeaders, SIGNAL(messageSelected(KMMessage*)),
	  this, SLOT(messageSelected(KMMessage*)));

  mMsgView = new KMReaderView(mVertPanner);

  setMinimumSize(400, 300);
  resize(400,300);

  mVertPanner->activate(mHorizPanner, mMsgView);
  mHorizPanner->activate(mFolderTree, mHeaders);

  parseConfiguration();
  setView(mVertPanner);

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

  config->setGroup("Settings");
  str = config->readEntry("Inline");
  mMsgView->setInline((str.find("false",0,FALSE) == 0));
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
  mVertPanner->setSeperatorPos(mVertPannerSep);
  mHorizPanner->setSeperatorPos(mHorizPannerSep);
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

  config->setGroup("Settings");
  config->writeEntry("Inline", mMsgView->isInline() ? "true" : "false");

  config->sync();
  e->accept();
  if (!(--windowCount)) qApp->quit();
}


//-----------------------------------------------------------------------------
void KMMainWin::doAbout()
{
  KMsgBox::message(this, nls->translate("About"),
		   "KMail [V0.1.1] by\n\n"
		   "Stefan Taferner <taferner@kde.org>,\n"
		   "Markus Wübben <markus.wuebben@kde.org>\n"
		   "\nbased on the work of\n"
		   "Stephan Meyer <Stephan.Meyer@pobox.com>,\n"
		   "Lynx <lynx@topaz.hknet.com>.\n"
		   "\nThis program is covered by the GPL.", 1);
}


//-----------------------------------------------------------------------------
void KMMainWin::doClose() 
{
  close();
}


//-----------------------------------------------------------------------------
void KMMainWin::doHelp()
{
  app->invokeHTMLHelp("","");
}


//-----------------------------------------------------------------------------
void KMMainWin::doNewMailReader()
{
  KMMainWin *d;

  d = new KMMainWin(NULL);
  d->show();
  d->resize(d->size());
}


//-----------------------------------------------------------------------------
void KMMainWin::doSettings()
{
  KMSettings dlg(this);
  dlg.exec();
}


//-----------------------------------------------------------------------------
void KMMainWin::doUnimplemented()
{
  warning(nls->translate("Sorry, but this feature\nis still missing"));
}


//-----------------------------------------------------------------------------
void KMMainWin::doAddFolder() 
{
  KMFolderDialog dlg(NULL, this);

  dlg.setCaption(nls->translate("New Folder"));
  if (dlg.exec()) mFolderTree->reload();
}


//-----------------------------------------------------------------------------
void KMMainWin::doCheckMail() 
{
  kbp->busy();
  acctMgr->checkMail();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::doCompose()
{
  KMComposeWin *d;
  d = new KMComposeWin;
  d->show();
}


//-----------------------------------------------------------------------------
void KMMainWin::doModifyFolder()
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
void KMMainWin::doEmptyFolder()
{
  kbp->busy();
  mFolder->expunge();
  mHeaders->setFolder(mFolder);
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::doRemoveFolder()
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
#ifdef BROKEN
    mHeaders->clear();
    if ( horzPanner) messageView->clearCanvas();
    mFolderTree->cdFolder(&dir);
    removeDirectory(dir.path());
    mFolderTree->getList();
#else
    warning("Removing of folders is\nstill under construction.");
#endif
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::doPrintMsg()
{ 
  if(mHeaders->currentItem() >= 0)
    mMsgView->printMail();
}


//-----------------------------------------------------------------------------
void KMMainWin::doReplyToMsg()
{ 
  mHeaders->replyToMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::doReplyAllToMsg()
{ 
  mHeaders->replyAllToMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::doForwardMsg()
{ 
  mHeaders->forwardMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::doDeleteMsg()
{ 
  mHeaders->deleteMsg();
}


//-----------------------------------------------------------------------------
void KMMainWin::doViewChange()
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

  // from KMMainView::slotViewChange():
  mMsgView->setInline(!mMsgView->isInline());
  mMsgView->updateDisplay();  
}


//-----------------------------------------------------------------------------
void KMMainWin::folderSelected(KMFolder* aFolder)
{
  kbp->busy();

  mFolder = (KMFolder*)aFolder;
  mHeaders->setFolder(mFolder);
  mMsgView->clearCanvas();

  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainWin::messageSelected(KMMessage *msg)
{
  KMReaderWin *win;

  if(mIntegrated) mMsgView->parseMessage(msg);
  else
  {
    win = new KMReaderWin(0,0,mHeaders->currentItem()+1,mFolder);
    win->show();
  }
}


//-----------------------------------------------------------------------------
void KMMainWin::setupMenuBar()
{
  //----- File Menu
  QPopupMenu *fileMenu = new QPopupMenu();
  fileMenu->insertItem(nls->translate("New Composer"), this, 
		       SLOT(doCompose()), keys->openNew());
  fileMenu->insertItem(nls->translate("New Mailreader"), this, 
		       SLOT(doNewMailReader()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("&Settings..."), this, 
		       SLOT(doSettings()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("&Close"), this, 
		       SLOT(doClose()), keys->close());
  fileMenu->insertItem(nls->translate("&Quit"), qApp,
		       SLOT(quit()), keys->quit());

  //----- Edit Menu
  QPopupMenu *editMenu = new QPopupMenu();
  editMenu->insertItem(nls->translate("&Undo"), this, SLOT(doUnimplemented()),
		       keys->undo());
  editMenu->insertSeparator();
  editMenu->insertItem(nls->translate("&Cut"), this, SLOT(doUnimplemented()),
		       keys->cut());
  editMenu->insertItem(nls->translate("&Copy"), this, SLOT(doUnimplemented()),
		       keys->copy());
  editMenu->insertItem(nls->translate("&Paste"),this, SLOT(doUnimplemented()),
		       keys->paste());
  editMenu->insertSeparator();
  editMenu->insertItem(nls->translate("&Find..."), this, 
		       SLOT(doUnimplemented()), keys->find());

  //----- Message Menu
  QPopupMenu *messageMenu = new QPopupMenu();
#ifdef BROKEN
  messageMenu->insertItem(nls->translate("&Next"), mainView, 
			  SLOT(doNextMsg()), Key_N);
  messageMenu->insertItem(nls->translate("&Previous"), mainView, 
			  SLOT(doPreviousMsg()), Key_P);
  messageMenu->insertSeparator();
#endif //BROKEN
  messageMenu->insertItem(nls->translate("Toggle All &Headers"), this, 
			  SLOT(doUnimplemented()), Key_H);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Reply..."), this,
			  SLOT(doReplyToMsg()), Key_R);
  messageMenu->insertItem(nls->translate("Reply &All..."), this,
			  SLOT(doReplyAllToMsg()), Key_A);
  messageMenu->insertItem(nls->translate("&Forward..."), this, 
			  SLOT(doForwardMsg()), Key_F);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Move..."), this, 
			  SLOT(doUnimplemented()), Key_M);
  messageMenu->insertItem(nls->translate("&Copy..."), this, 
			  SLOT(doUnimplemented()), Key_S);
  messageMenu->insertItem(nls->translate("&Delete"), this, 
			  SLOT(doDeleteMsg()), Key_D);
#ifdef BROKEN
  messageMenu->insertItem(nls->translate("&Undelete"), this,
			  SLOT(doUnimplemented()), Key_U);
#endif
  messageMenu->insertSeparator();

  bodyParts = new QPopupMenu();
  bodyParts->setCheckable(TRUE);
  bodyParts->insertItem(nls->translate("Inline"), this,
			SLOT(doViewChange()));
  bodyParts->insertItem(nls->translate("Separate"), this,
			SLOT(doViewChange()));

  printf("mainwin %i\n",showInline);
  if(showInline)
    bodyParts->setItemChecked(bodyParts->idAt(0),TRUE);
  else
    bodyParts->setItemChecked(bodyParts->idAt(1),TRUE);

  messageMenu->insertItem(nls->translate("View Body Parts..."),bodyParts);

  messageMenu->insertItem(nls->translate("&Export..."), this, 
			  SLOT(doUnimplemented()), Key_E);
  messageMenu->insertItem(nls->translate("Pr&int..."), this,
			  SLOT(doPrintMsg()), keys->print());

  //----- Folder Menu
  QPopupMenu *folderMenu = new QPopupMenu();
  folderMenu->insertItem(nls->translate("&Create..."), this, 
			 SLOT(doAddFolder()));
  folderMenu->insertItem(nls->translate("&Modify..."), this, 
			 SLOT(doModifyFolder()));
  folderMenu->insertItem(nls->translate("&Remove"), this, 
			 SLOT(doRemoveFolder()));
  folderMenu->insertItem(nls->translate("&Empty"), this, 
			 SLOT(doEmptyFolder()));

  //----- Help Menu
  QPopupMenu *helpMenu = new QPopupMenu();
  helpMenu->insertItem(nls->translate("&Help"), this, SLOT(doHelp()), 
		       keys->help());
  helpMenu->insertSeparator();
  helpMenu->insertItem(nls->translate("&About"), this, SLOT(doAbout()));

  //----- Menubar
  mMenuBar  = new KMenuBar(this);
  mMenuBar->insertItem(nls->translate("File"), fileMenu);
  mMenuBar->insertItem(nls->translate("&Edit"), editMenu);
  mMenuBar->insertItem(nls->translate("&Message"), messageMenu);
  mMenuBar->insertItem(nls->translate("F&older"), folderMenu);
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
			SLOT(doCompose()), TRUE, 
			nls->translate("Compose new message"));

  mToolBar->insertButton(loader->loadIcon("filefloppy.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(doUnimplemented()), TRUE,
			nls->translate("Save message to file"));

  mToolBar->insertButton(loader->loadIcon("fileprint.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(doUnimplemented()), TRUE,
			nls->translate("Print message"));

  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("checkmail.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(doCheckMail()), TRUE,
			nls->translate("Get new mail"));
  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("filereply.xpm"), 0, 
			SIGNAL(clicked()), this, 
			SLOT(doReplyToMsg()), TRUE,
			nls->translate("Reply to author"));

  mToolBar->insertButton(loader->loadIcon("filereplyall.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(doReplyAllToMsg()), TRUE,
			nls->translate("Reply to all recipients"));

  mToolBar->insertButton(loader->loadIcon("fileforward.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(doForwardMsg()), TRUE,
			nls->translate("Forward message"));

  mToolBar->insertButton(loader->loadIcon("filedel2.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(doDeleteMsg()), TRUE,
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
