// kmmainwin.cpp

#include <qstring.h>
#include <qpixmap.h>
#include <qdir.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include <Kconfig.h>
#include <kapp.h>
#include <klocale.h>

#include "util.h"
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


//-----------------------------------------------------------------------------
KMMainWin::KMMainWin(QWidget *, char *name) :
  KMMainWinInherited(name)
{
  mainView = new KMMainView(this);

  setView(mainView);

  parseConfiguration();

  setupMenuBar();
  setupToolBar();
  setupStatusBar();

  setMinimumSize(400, 300);
  windowCount++;
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
}


//-----------------------------------------------------------------------------
void KMMainWin::setupMenuBar()
{
  //----- File Menu
  QPopupMenu *fileMenu = new QPopupMenu();
  fileMenu->insertItem(nls->translate("New Composer"), mainView, 
		       SLOT(doCompose()), Key_C);
  fileMenu->insertItem(nls->translate("New Mailreader"), this, 
		       SLOT(doNewMailReader()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("&Settings..."), this, 
		       SLOT(doSettings()));
  fileMenu->insertSeparator();
  fileMenu->insertItem(nls->translate("&Close"), this, SLOT(doClose()));
  fileMenu->insertItem(nls->translate("&Quit"), qApp,
		       SLOT(quit()), ALT + Key_Q);

  //----- Edit Menu
  QPopupMenu *editMenu = new QPopupMenu();
  editMenu->insertItem(nls->translate("&Undo"), this, SLOT(doUnimplemented()),
		       CTRL + Key_Z);
  editMenu->insertItem(nls->translate("&Cut"), this, SLOT(doUnimplemented()),
		       CTRL + Key_X);
  editMenu->insertItem(nls->translate("&Copy"), this, SLOT(doUnimplemented()),
		       CTRL + Key_C);
  editMenu->insertItem(nls->translate("&Paste"),this, SLOT(doUnimplemented()),
		       CTRL + Key_V);
  editMenu->insertSeparator();
  editMenu->insertItem(nls->translate("&Find..."), this, 
		       SLOT(doUnimplemented()), CTRL + Key_F);

  //----- Message Menu
  QPopupMenu *messageMenu = new QPopupMenu();
  messageMenu->insertItem(nls->translate("&Next"), this, 
			  SLOT(doUnimplemented()), Key_N);
  messageMenu->insertItem(nls->translate("&Previous"), this, 
			  SLOT(doUnimplemented()), Key_P);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("Toggle All &Headers"), this, 
			  SLOT(doUnimplemented()), Key_H);
  messageMenu->insertSeparator();
  messageMenu->insertItem(nls->translate("&Reply..."), this, 
			  SLOT(doUnimplemented()), Key_R);
  messageMenu->insertItem(nls->translate("&Forward..."), this, 
			  SLOT(doUnimplemented()), Key_F);
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
  messageMenu->insertItem(nls->translate("&Export..."), this, 
			  SLOT(doUnimplemented()), Key_E);
  messageMenu->insertItem(nls->translate("Pr&int..."), this, 
			  SLOT(doUnimplemented()), CTRL + Key_P);

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
  helpMenu->insertItem(nls->translate("&Help"), this, SLOT(doHelp()), Key_F1);
  helpMenu->insertSeparator();
  helpMenu->insertItem(nls->translate("&About"), this, SLOT(doAbout()));

  //----- Menubar
  menuBar  = new KMenuBar(this);
  menuBar->insertItem(nls->translate("File"), fileMenu);
  menuBar->insertItem(nls->translate("&Edit"), editMenu);
  menuBar->insertItem(nls->translate("&Message"), messageMenu);
  menuBar->insertItem(nls->translate("F&older"), folderMenu);
  menuBar->insertItem(nls->translate("&Help"), helpMenu);

  setMenu(menuBar);
}


//-----------------------------------------------------------------------------
void KMMainWin::setupToolBar()
{
  QString pixdir = kapp->kdedir();
  pixdir.append("/lib/pics/toolbar/");

  toolBar = new KToolBar(this);

  QPixmap pixmap;

  pixmap.load(pixdir+"kmnew.xpm");
  toolBar->insertButton(pixmap, 0, SIGNAL(clicked()), mainView,
			SLOT(doCompose()), TRUE, 
			nls->translate("compose message"));

  toolBar->insertSeparator();

  pixmap.load(pixdir+"kmcheckmail.xpm");
  toolBar->insertButton(pixmap, 0, SIGNAL(clicked()), mainView,
			SLOT(doCheckMail()), TRUE,
			nls->translate("check for new mail"));

  toolBar->insertSeparator();

  pixmap.load(pixdir+"kmreply.xpm");
  toolBar->insertButton(pixmap, 0, SIGNAL(clicked()), mainView,
			SLOT(doReplyMessage()), TRUE,
			nls->translate("reply to message"));

  pixmap.load(pixdir+"kmforward.xpm");
  toolBar->insertButton(pixmap, 0, SIGNAL(clicked()), mainView,
			SLOT(doForwardMessage()), TRUE,
			nls->translate("forward message"));

  pixmap.load(pixdir+"kmdel.xpm");
  toolBar->insertButton(pixmap, 0, SIGNAL(clicked()), mainView,
			SLOT(doDeleteMessage()), TRUE,
			nls->translate("delete message"));

  pixmap.load(pixdir+"kmsave.xpm");
  toolBar->insertButton(pixmap, 0, SIGNAL(clicked()), this,
			SLOT(doUnimplemented()), TRUE,
			nls->translate("save message to file"));

  pixmap.load(pixdir+"kmprint.xpm");
  toolBar->insertButton(pixmap, 0, SIGNAL(clicked()), this,
			SLOT(doUnimplemented()), TRUE,
			nls->translate("print message"));

  toolBar->insertSeparator();

  pixmap.load(pixdir+"help.xpm");
  toolBar->insertButton(pixmap, 0, SIGNAL(clicked()), this,
			SLOT(doHelp()), TRUE, nls->translate("Help"));

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
		   "Stefan Taferner <taferner@alpin.or.at>,\n"
		   "Markus Wübben <markus.wuebben@kde.org>\n\n" 
		   "based on the work of:\n"
		   "Lynx <lynx@topaz.hknet.com>,\n"
		   "Stephan Meyer <Stephan.Meyer@pobox.com>,\n"
		   "and the above authors.\n"
		   "This program is covered by the GPL.",1);
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
  delete this;
  if (!(--windowCount)) qApp->quit();
}


//-----------------------------------------------------------------------------
void KMMainWin::show(void)
{
  KMMainWinInherited::show();
  resize(size());
}








