// kmmainwin.cpp

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

#include "kmcomposewin.h"
#include "kmsettings.h"
#include "kmfolderdia.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kbusyptr.h"
#include "kmfolder.h"
#include "kmglobal.h"
#include "kmmessage.h"
#include "kmmainview.h"

#include "kmmainwin.moc"

static int windowCount = 0;


//-----------------------------------------------------------------------------
KMMainWin::KMMainWin(QWidget *, char *name) :
  KMMainWinInherited(name)
{
  parseConfiguration();
  mainView = new KMMainView(this);

  setView(mainView);

  setupMenuBar();
  setupToolBar();
  setupStatusBar();

  setMinimumSize(400, 300);
  windowCount++;
}


//-----------------------------------------------------------------------------
KMMainWin::~KMMainWin()
{
  if (toolBar) delete toolBar;
  if (menuBar) delete menuBar;
}


//-----------------------------------------------------------------------------
void KMMainWin::parseConfiguration()
{
  KConfig *config;
  QString s;
  config=KApplication::getKApplication()->getConfig();

  int x=0,y=0,w=400,h=300;
  config->setGroup("Geometry");
  s=config->readEntry("Main");
  if ((!s.isEmpty()) && (s.find(',')!=-1)) {
    sscanf(s,"%d,%d,%d,%d",&x,&y,&w,&h);
    setGeometry(x,y,w,h);
  }

 config->setGroup("Settings");
  s = config->readEntry("Inline");
  if((!s.isEmpty() && s.find("false",0,false)) == 0)
	showInline = false;
  else
	showInline  = true;

}


//-----------------------------------------------------------------------------
void KMMainWin::setupMenuBar()
{
  //----- File Menu
  QPopupMenu *fileMenu = new QPopupMenu();
  fileMenu->insertItem(nls->translate("New Composer"), mainView, 
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
  messageMenu->insertItem(nls->translate("&Next"), mainView, 
			  SLOT(doNextMsg()), Key_N);
  messageMenu->insertItem(nls->translate("&Previous"), mainView, 
			  SLOT(doPreviousMsg()), Key_P);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("Toggle All &Headers"), this, 
			  SLOT(doUnimplemented()), Key_H);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Reply..."), mainView, 
			  SLOT(doReplyMessage()), Key_R);
  messageMenu->insertItem(nls->translate("Reply &All..."),mainView,
			  SLOT(doReplyAllToMessage()), Key_A);
  messageMenu->insertItem(nls->translate("&Forward..."), mainView, 
			  SLOT(doForwardMessage()), Key_F);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Move..."), this, 
			  SLOT(doUnimplemented()), Key_M);
  messageMenu->insertItem(nls->translate("&Copy..."), this, 
			  SLOT(doUnimplemented()), Key_S);
  messageMenu->insertItem(nls->translate("&Delete"), this, 
			  SLOT(doUnimplemented()), Key_D);
  messageMenu->insertItem(nls->translate("&Undelete"), this,
			  SLOT(doUnimplemented()), Key_U);
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
  messageMenu->insertItem(nls->translate("Pr&int..."), mainView, 
			  SLOT(doPrintMessage()), keys->print());

  //----- Folder Menu
  QPopupMenu *folderMenu = new QPopupMenu();
  folderMenu->insertItem(nls->translate("&Create..."), mainView, 
			 SLOT(doAddFolder()));
  folderMenu->insertItem(nls->translate("&Modify..."), mainView, 
			 SLOT(doModifyFolder()));
  folderMenu->insertItem(nls->translate("&Remove"), mainView, 
			 SLOT(doRemoveFolder()));
  folderMenu->insertItem(nls->translate("&Empty"), mainView, 
			 SLOT(doEmptyFolder()));

  //----- Help Menu
  QPopupMenu *helpMenu = new QPopupMenu();
  helpMenu->insertItem(nls->translate("&Help"), this, SLOT(doHelp()), 
		       keys->help());
  helpMenu->insertSeparator();
  helpMenu->insertItem(nls->translate("&About"), this, SLOT(doAbout()));

  //----- Menubar
  menuBar  = new KMenuBar(this);
  menuBar->insertItem(nls->translate("File"), fileMenu);
  menuBar->insertItem(nls->translate("&Edit"), editMenu);
  menuBar->insertItem(nls->translate("&Message"), messageMenu);
  menuBar->insertItem(nls->translate("F&older"), folderMenu);
  menuBar->insertSeparator();
  menuBar->insertItem(nls->translate("&Help"), helpMenu);

  setMenu(menuBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupToolBar()
{
  KIconLoader* loader = kapp->getIconLoader();

  toolBar = new KToolBar(this);

  toolBar->insertButton(loader->loadIcon("filenew.xpm"), 0, 
			SIGNAL(clicked()), mainView,
			SLOT(doCompose()), TRUE, 
			nls->translate("Compose new message"));

  toolBar->insertButton(loader->loadIcon("filefloppy.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(doUnimplemented()), TRUE,
			nls->translate("Save message to file"));

  toolBar->insertButton(loader->loadIcon("fileprint.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(doUnimplemented()), TRUE,
			nls->translate("Print message"));

  toolBar->insertSeparator();

  toolBar->insertButton(loader->loadIcon("checkmail.xpm"), 0, 
			SIGNAL(clicked()), mainView,
			SLOT(doCheckMail()), TRUE,
			nls->translate("Get new mail"));
  toolBar->insertSeparator();

  toolBar->insertButton(loader->loadIcon("filereply.xpm"), 0, 
			SIGNAL(clicked()), mainView, 
			SLOT(doReplyMessage()), TRUE,
			nls->translate("Reply to author"));

  toolBar->insertButton(loader->loadIcon("filereplyall.xpm"), 0, 
			SIGNAL(clicked()), mainView,
			SLOT(doReplyMessage()), TRUE,
			nls->translate("Reply to all recipients"));

  toolBar->insertButton(loader->loadIcon("fileforward.xpm"), 0, 
			SIGNAL(clicked()), mainView,
			SLOT(doForwardMessage()), TRUE,
			nls->translate("Forward message"));

  toolBar->insertButton(loader->loadIcon("filedel2.xpm"), 0, 
			SIGNAL(clicked()), mainView,
			SLOT(doDeleteMessage()), TRUE,
			nls->translate("Delete message"));

  addToolBar(toolBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupStatusBar()
{
  statusBar = new KStatusBar(this);

  statusBar->enable(KStatusBar::Show);
  setStatusBar(statusBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::doAbout()
{
  KMsgBox::message(this,nls->translate("About"),
		   "KMail [V0.1] by\n\n"
		   "Stephan Meyer <Stephan.Meyer@pobox.com>,\n"
		   "Stefan Taferner <taferner@kde.org>,\n"
		   "Markus Wübben <markus.wuebben@kde.org>,\n" 
		   "Lynx <lynx@topaz.hknet.com>,\n"
		   "\nThis program is covered by the GPL.",1);
}


//-----------------------------------------------------------------------------
void KMMainWin::doClose() {
  close();
}


//-----------------------------------------------------------------------------
void KMMainWin::doHelp()
{
  KApplication::getKApplication()->invokeHTMLHelp("","");
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
  KMSettings *d=new KMSettings(this);
  d->exec();
  delete d;
}


//-----------------------------------------------------------------------------
void KMMainWin::doUnimplemented()
{
  warning(nls->translate("Sorry, but this feature\nis still missing"));
}


//-----------------------------------------------------------------------------
void KMMainWin::closeEvent(QCloseEvent *e)
{
  KTopLevelWidget::closeEvent(e);
  QString s;
  KConfig *config;
  QRect r=geometry();
  s.sprintf("%i,%i,%i,%i",r.x(),r.y(),r.width(),r.height());
  config=KApplication::getKApplication()->getConfig();
  config->setGroup("Geometry");
  config->writeEntry("Main",s);

  config->setGroup("Settings");
  if(mainView->isInline())
    {printf("isInline %i\n",mainView->isInline()); 
    config->writeEntry("Inline","true");}
  else
    config->writeEntry("Inline","false");
  config->sync();
  delete this;
  if (!(--windowCount)) qApp->quit();
}

void KMMainWin::doViewChange()
{
  if(bodyParts->isItemChecked(bodyParts->idAt(0)))
    {bodyParts->setItemChecked(bodyParts->idAt(0),FALSE);
    bodyParts->setItemChecked(bodyParts->idAt(1),TRUE);
    }
  else
    if(bodyParts->isItemChecked(bodyParts->idAt(1)))
      {bodyParts->setItemChecked(bodyParts->idAt(1),FALSE);
      bodyParts->setItemChecked(bodyParts->idAt(0),TRUE);
      }
  mainView->slotViewChange();
}

//-----------------------------------------------------------------------------
void KMMainWin::show(void)
{
  KMMainWinInherited::show();
  resize(size());
}








