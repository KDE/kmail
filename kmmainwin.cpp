#include <qstring.h>
#include <qpixmap.h>
#include <qdir.h>
#include <qfile.h>
#include <qtstream.h>
#include <kapp.h>
#include <kmsgbox.h>
#include "util.h"
#include "kmcomposewin.h"
#include "kmsettings.h"
#include "kmfolderdia.h"
#include "kmaccount.h"
#include "kmmainwin.moc"
#include "mclass.h"
#include "kbusyptr.h"

KBusyPtr* kbp;

KMMainView::KMMainView(QWidget *parent, const char *name) : QWidget(parent,name)
{
        KConfig *config = new KConfig();
        config = KApplication::getKApplication()->getConfig();
        config->setGroup("Settings");
        QString o;
        o = config->readEntry("Reader");
        if( !o.isEmpty() && o.find("separated",0,false) ==0)
                initSeparated();
        else
                initIntegrated();

        currentFolder = new Folder();


}


void KMMainView::initSeparated()
{
        printf("initSeparated()\n");
        vertPanner=new KPanner(this, NULL,KPanner::O_VERTICAL,30);
        connect(vertPanner, SIGNAL(positionChanged()),this,SLOT(pannerHasChanged()));
        folderTree = new KMFolderTree(vertPanner->child0());
	connect(folderTree,SIGNAL(folderSelected(QDir*)),this,SLOT(folderSelected(QDir*)));
        headers = new KMHeaders(vertPanner->child1());
	connect(headers,SIGNAL(messageSelected(Message*)),this,SLOT(messageSelected(Message *)));
        horzPanner = NULL;
        printf("Leaving initSeparated()\n");
        Integrated = 0;

}


void KMMainView::initIntegrated()
{
        printf("Entering initIntegrated()\n");
        horzPanner=new KPanner(this, NULL,KPanner::O_HORIZONTAL,40);
        connect(horzPanner, SIGNAL(positionChanged()),this,SLOT(pannerHasChanged()));
        vertPanner=new KPanner(horzPanner->child0(), NULL,KPanner::O_VERTICAL,30);
        connect(vertPanner, SIGNAL(positionChanged()),this,SLOT(pannerHasChanged()));
        folderTree = new KMFolderTree(vertPanner->child0());
        connect(folderTree,SIGNAL(folderSelected(QDir *)),this,SLOT(folderSelected(QDir *)));
        headers = new KMHeaders(vertPanner->child1());
	connect(headers,SIGNAL(messageSelected(Message*)),this,SLOT(messageSelected(Message*)));
        messageView = new KMReaderView(horzPanner->child1());
        printf("Leaving initIntegrated()\n");
        Integrated =1;

}

void KMMainView::doAddFolder() {
	KMFolderDialog *d=new KMFolderDialog(this);
	d->setCaption("Create Folder");
	if (d->exec()) {
		QDir dir;
		folderTree->cdFolder(&dir);
		dir.mkdir(d->nameEdit->text());
		folderTree->getList();
	}
	delete d;
}

void KMMainView::doCheckMail() {
	unsigned int i,j;
	QDir dir;
	QFile *f;
	QTextStream *g;
	QString s;
	KMAccount *a;
	KMAccountMan *am;
	Message *m;
	int delmsgs;
	Folder download,upload;
	char buf[80];

	am=new KMAccountMan();

	for (i=1;i<folderTree->count();i++) {
		folderTree->cdFolder(&dir,i);
		if (!dir.exists("accounts")) continue;
		f=new QFile(dir.absFilePath("accounts"));
		if (!f->open(IO_ReadWrite)) { delete f; continue; }
		g=new QTextStream(f);

		while (!g->eof()) {

			s=g->readLine();
			strcpy(buf,s);
			printf("account:\t%s\n",buf);

			if ((a=am->findAccount(s))==NULL) continue;
			if (a->config->readEntry("access method")=="maintain remotely") continue;

			delmsgs=a->config->readEntry("access method")=="move messages";

			a->getLocation(&s);
			strcpy(buf,s);
			printf("location:\t%s\n",buf);
			download.open(s);
			if (!dir.exists("contents"))
			  createFolder((const char *)(QString(dir.path())+QString("/contents")));
			upload.open(QString(dir.path())+QString("/contents"));

			for (j=1;j<=download.numMsg();j++) {
				m=download.getMsg(j);
				upload.putMsg(m);
				delete m;
			}

			download.close();
			upload.close();
		}
		delete g;
		delete f;
	}
	delete am;
}

void KMMainView::doCompose() {
	KMComposeWin *d;
	d = new KMComposeWin;
	d->show();
	d->resize(d->size());
}

void KMMainView::doModifyFolder() {
	if ((folderTree->currentItem())<1) return; // make sure something is selected and it's not "/"

	KMFolderDialog *d=new KMFolderDialog(this);
	d->setCaption("Modify Folder");
	d->nameEdit->setText(folderTree->getCurrentItem()->getText());
	if (d->exec()) {
		QDir dir;
		folderTree->cdFolder(&dir);
		dir.cdUp();
		dir.rename(folderTree->getCurrentItem()->getText(),d->nameEdit->text());
		folderTree->getList();
	}
	delete d;
}

void KMMainView::doRemoveFolder() {
	if ((folderTree->currentItem())<1) return;

	QString s;
	QDir dir;
	s.sprintf("Are you sure you want to remove the folder \"%s\"\n and all of its child folders?",
	 folderTree->getCurrentItem()->getText());
	if ((KMsgBox::yesNo(this,"Confirmation",s))==1) {
		headers->clear();
		if ( horzPanner) messageView->clearCanvas();
		folderTree->cdFolder(&dir);
		removeDirectory(dir.path());
		folderTree->getList();
	}
}

void KMMainView::folderSelected(QDir *d) {
	QString s="";
	s.append(d->path());
	s.append("/contents");
	if ((d->exists(s))==FALSE) {
		headers->clear();
		return;
	}

	Folder *f=new Folder();
	f->open(s);
	currentFolder = f;
	headers->setFolder(f);
	if (horzPanner) messageView->clearCanvas();
}

void KMMainView::doDeleteMessage() {
	headers->toggleDeleteMsg();
}

void KMMainView::doForwardMessage() {
	headers->forwardMsg();
}

void KMMainView::doReplyMessage() {
	headers->replyToMsg();
}

void KMMainView::messageSelected(Message *m) {
	if(horzPanner)
		messageView->parseMessage(m);
	else
		{KMReaderWin *w = new KMReaderWin(0,0,headers->currentItem()+1,currentFolder);
		w->show();
		w->resize(w->size());	
		}
}

void KMMainView::pannerHasChanged() {
	resizeEvent(NULL);
}

void KMMainView::resizeEvent(QResizeEvent *e) {
	QWidget::resizeEvent(e);
	if(horzPanner)
		{horzPanner->setGeometry(0, 0, width(), height());
		vertPanner->resize(horzPanner->child0()->width(),horzPanner->child0()->height());
		folderTree->resize(vertPanner->child0()->width(),vertPanner->child0()->height());
		headers->resize(vertPanner->child1()->width(),vertPanner->child1()->height());
		messageView->resize(horzPanner->child1()->width(), horzPanner->child1()->height());
		}
	else	
	   {vertPanner->resize(width(), height());
	   folderTree->resize(vertPanner->child0()->width(),vertPanner->child0()->height());
	   headers->resize(vertPanner->child1()->width(), vertPanner->child1()->height());
	  }

}

KMMainWin::KMMainWin(QWidget *, char *name) : KTopLevelWidget(name)
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
	QString pixdir = "";   // pics dir code "inspired" by kghostview (thanks)
	char *kdedir = getenv("KDEDIR");
	if (kdedir) pixdir.append(kdedir);
	 else pixdir.append("/usr/local/kde");
	pixdir.append("/lib/pics/toolbar/");

	toolBar = new KToolBar(this);

	QPixmap pixmap;

	pixmap.load(pixdir+"kmnew.xpm");
	toolBar->insertItem(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doCompose()), TRUE, "compose message");

	toolBar->insertSeparator();

	pixmap.load(pixdir+"kmcheckmail.xpm");
	toolBar->insertItem(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doCheckMail()), TRUE, "check for new mail");

	toolBar->insertSeparator();

	pixmap.load(pixdir+"kmreply.xpm");
	toolBar->insertItem(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doReplyMessage()), TRUE, "reply to message");

	pixmap.load(pixdir+"kmforward.xpm");
	toolBar->insertItem(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doForwardMessage()), TRUE, "forward message");

	pixmap.load(pixdir+"kmdel.xpm");
	toolBar->insertItem(pixmap, 0,
			    SIGNAL(clicked()), mainView,
			    SLOT(doDeleteMessage()), TRUE, "delete message");

	pixmap.load(pixdir+"kmsave.xpm");
	toolBar->insertItem(pixmap, 0,
			    SIGNAL(clicked()), this,
			    SLOT(doUnimplemented()), TRUE, "save message to file");

	pixmap.load(pixdir+"kmprint.xpm");
	toolBar->insertItem(pixmap, 0,
			    SIGNAL(clicked()), this,
			    SLOT(doUnimplemented()), TRUE, "print message");

	toolBar->insertSeparator();

	pixmap.load(pixdir+"help.xpm");
	toolBar->insertItem(pixmap, 0,
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
	KMsgBox::message(this,"Ouch",
	 "Not yet implemented!\n"
	 "We are sorry for the inconvenience :)",1);
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

main(int argc, char *argv[])
{
	KApplication *a;
	KMMainWin *k;

	initCC();
	a = new KApplication(argc, argv, "kmail");
	k = new KMMainWin();
	kbp = new KBusyPtr(a);
	k->show();
	k->resize(k->size()); // necessary despite resize in constructor
	a->exec();
	delete a;
}

