// kmmainwin.cpp

#include <qstring.h>
#include <qpixmap.h>
#include <qdir.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include <Kconfig.h>
#include <kapp.h>

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

void KMMainWin::setupMenuBar()
{
	QPopupMenu *fileMenu = new QPopupMenu();
	fileMenu->insertItem("New Composer", mainView, SLOT(doCompose()), Key_C);
	fileMenu->insertItem("New Mailreader", this, SLOT(doNewMailReader()));
	fileMenu->insertSeparator();
	fileMenu->insertItem("&Settings...", this, SLOT(doSettings()));
	fileMenu->insertSeparator();
	fileMenu->insertItem("&Close", this, SLOT(doClose()));
	fileMenu->insertItem("&Quit", qApp, SLOT(quit()), ALT + Key_Q);

	QPopupMenu *messageMenu = new QPopupMenu();
	messageMenu->insertItem("&Next", this, SLOT(doUnimplemented()), Key_N);
	messageMenu->insertItem("&Previous", this, SLOT(doUnimplemented()), Key_P);
	messageMenu->insertSeparator();
	messageMenu->insertItem("Toggle All &Headers", this, SLOT(doUnimplemented()), Key_H);
	messageMenu->insertSeparator();
	messageMenu->insertItem("&Reply...", this, SLOT(doUnimplemented()), Key_R);
	messageMenu->insertItem("&Forward...", this, SLOT(doUnimplemented()), Key_F);
	messageMenu->insertSeparator();
	messageMenu->insertItem("&Move...", this, SLOT(doUnimplemented()), Key_M);
	messageMenu->insertItem("&Copy...", this, SLOT(doUnimplemented()), Key_S);
	messageMenu->insertItem("&Delete", this, SLOT(doUnimplemented()), Key_D);
	messageMenu->insertItem("&Undelete", this, SLOT(doUnimplemented()), Key_U);
	messageMenu->insertSeparator();
	messageMenu->insertItem("&Export...", this, SLOT(doUnimplemented()), Key_E);
	messageMenu->insertItem("Pr&int...", this, SLOT(doUnimplemented()), CTRL + Key_P);

	QPopupMenu *folderMenu = new QPopupMenu();
	folderMenu->insertItem("&Create...", mainView, SLOT(doAddFolder()), ALT + Key_C);
	folderMenu->insertItem("&Modify...", mainView, SLOT(doModifyFolder()), ALT + Key_M);
	folderMenu->insertItem("&Remove", mainView, SLOT(doRemoveFolder()), ALT + Key_R);

	QPopupMenu *helpMenu = new QPopupMenu();
	helpMenu->insertItem("&Help", this, SLOT(doHelp()), ALT + Key_H),
	helpMenu->insertSeparator();
	helpMenu->insertItem("&About", this, SLOT(doAbout()));

	menuBar  = new KMenuBar(this);
	menuBar->insertItem("File", fileMenu);
	menuBar->insertItem("Message", messageMenu);
	menuBar->insertItem("Folder", folderMenu);
	menuBar->insertItem("Help", helpMenu);

	setMenu(menuBar);
}

void KMMainWin::setupToolBar()
{
	QString pixdir = kapp->kdedir();
	pixdir.append("/lib/pics/toolbar/");

	toolBar = new KToolBar(this);

	QPixmap pixmap;

	pixmap.load(pixdir+"kmnew.xpm");
	toolBar->insertButton(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doCompose()), TRUE, "compose message");

	toolBar->insertSeparator();

	pixmap.load(pixdir+"kmcheckmail.xpm");
	toolBar->insertButton(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doCheckMail()), TRUE, "check for new mail");

	toolBar->insertSeparator();

	pixmap.load(pixdir+"kmreply.xpm");
	toolBar->insertButton(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doReplyMessage()), TRUE, "reply to message");

	pixmap.load(pixdir+"kmforward.xpm");
	toolBar->insertButton(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doForwardMessage()), TRUE, "forward message");

	pixmap.load(pixdir+"kmdel.xpm");
	toolBar->insertButton(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doDeleteMessage()), TRUE, "delete message");

	pixmap.load(pixdir+"kmsave.xpm");
	toolBar->insertButton(pixmap, 0,
			    SIGNAL(clicked()), this,
			    SLOT(doUnimplemented()), TRUE, "save message to file");

	pixmap.load(pixdir+"kmprint.xpm");
	toolBar->insertButton(pixmap, 0,
			    SIGNAL(clicked()), this,
			    SLOT(doUnimplemented()), TRUE, "print message");

	toolBar->insertSeparator();

	pixmap.load(pixdir+"help.xpm");
	toolBar->insertButton(pixmap, 0,
			    SIGNAL(clicked()), this,
			    SLOT(doHelp()), TRUE, "Help");

	addToolBar(toolBar);
}

void KMMainWin::setupStatusBar()
{
	statusBar = new KStatusBar(this);

	statusBar->enable(KStatusBar::Show);
	setStatusBar(statusBar);
}

void KMMainWin::doAbout()
{
	KMsgBox::message(this,"About",
	 "kmail [ALPHA]\n\n"
	 "Yat-Nam Lo <lynx@topaz.hknet.com>\n"
	 "Stephan Meyer <Stephan.Meyer@munich.netsurf.de>\n"
	 "Stefan Taferner <taferner@alpin.or.at>\n"
	 "Markus Wübben <markus.wuebben@kde.org>\n\n"
	 "This program is covered by the GPL.",1);
}


void KMMainWin::doClose() {
	close();
}

void KMMainWin::doHelp()
{
	KApplication::getKApplication()->invokeHTMLHelp("","");
}

void KMMainWin::doNewMailReader()
{
	KMMainWin *d;
	d = new KMMainWin(NULL);
	d->show();
	d->resize(d->size());
}

void KMMainWin::doSettings()
{
	KMSettings *d=new KMSettings(this);
	d->exec();
	delete d;
}

void KMMainWin::doUnimplemented()
{
	KMsgBox::message(this,"Oops",
	 "Not yet implemented!\n"
	 "We are sorry for the inconvenience.",1);
}

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

void KMMainWin::show(void)
{
	KMMainWinInherited::show();
	resize(size());
}
