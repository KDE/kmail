#include <unistd.h>
#include <qfiledlg.h>
#include <qaccel.h>
#include <qlabel.h>
#include "util.h"
#include "kmmainwin.h"
#include "kmcomposewin.moc"

KMComposeView::KMComposeView(QWidget *parent, const char *name, QString emailAddress) : QWidget(parent,name)
{
	toLEdit = new QLineEdit(this);
	if (emailAddress) toLEdit->setText(emailAddress);
	ccLEdit = new QLineEdit(this);
	subjLEdit = new QLineEdit(this);

	QLabel *label = new QLabel(toLEdit, "&To:", this);
	label->setGeometry(14,10,50,15);

	label = new QLabel(subjLEdit, "&Cc:", this);
	label->setGeometry(14,45,50,20);

	label = new QLabel(ccLEdit, "&Subject:", this);
	label->setGeometry(14,80,50,20);

	editor = new QMultiLineEdit(this);
}

void KMComposeView::undoEvent()
{
}

void KMComposeView::copyText()
{
  editor->copyText();
}

void KMComposeView::cutText()
{
  editor->cut();
}

void KMComposeView::pasteText()
{
  editor->paste();
}

void KMComposeView::markAll()
{
  editor->selectAll();
}

void KMComposeView::saveMail()
{
  QString pwd;
  QString file;
  char buf[511];
  pwd.sprintf("%s",getcwd(buf,sizeof(buf)));
  file = QFileDialog::getSaveFileName(pwd,0,this);
  if(!file.isEmpty())
    {printf("EMail will be saved\n");
    }
  else
    {}
}

void KMComposeView::toDo()
{
	//  KMMainWin::doUnimplemented(); //is private :-(

   KMsgBox::message(this,"Ouch",
			 "Not yet implemented!\n"
			 "We are sorry for the inconvenience :-)",1);
}

void KMComposeView::resizeEvent(QResizeEvent *)
{
  toLEdit->setGeometry(80,10,width()-90,25);
  ccLEdit->setGeometry(80,45,width()-90,25);
  subjLEdit->setGeometry(80,80,width()-90,25);
  editor->setGeometry(10,115,width()-20,height()-125);
}


// This constructor is used if you want to start the Composer from the command line (e.g. kmail markus.wuebben@kde.org)

KMComposeWin::KMComposeWin(QWidget *, const char *name, QString emailAddress) : KTopLevelWidget(name)
{
  setCaption("KMail Composer");

  composeView = new KMComposeView(this,NULL,emailAddress);
  setView(composeView);

  setupMenuBar();

  setupToolBar();

  resize(480, 510);
  windowCount++;
}


/**********************************************************************************************/
void KMComposeWin::setupMenuBar()
{
  menuBar = new KMenuBar(this);

  QPopupMenu *menu = new QPopupMenu();
  menu->insertItem("Send",composeView,SLOT(toDo()));
  menu->insertSeparator();
  menu->insertItem("Save...",composeView,SLOT(saveMail()));
  menu->insertItem("Address Book...",composeView,SLOT(toDo()));
  menu->insertItem("Print...",composeView,SLOT(toDo()));
  menu->insertSeparator();
  menu->insertItem("New Composer",this,SLOT(newComposer()));
  menu->insertItem("New Mailreader",this,SLOT(newMailReader()));
  menu->insertSeparator();
  menu->insertItem("Close",this,SLOT(abort()));
  menu->insertItem("Quit",qApp,SLOT(quit()),ALT + Key_Q);
  menuBar->insertItem("File",menu);

  menu = new QPopupMenu();
  menu->insertItem("Undo",composeView,SLOT(undoEvent()));
  menu->insertSeparator();
  menu->insertItem("Copy",composeView,SLOT(copyText()),CTRL + Key_C);
  menu->insertItem("Cut",composeView,SLOT(cutText()),CTRL + Key_X);
  menu->insertItem("Paste",composeView,SLOT(pasteText()),CTRL + Key_V);
  menu->insertItem("Mark all",composeView,SLOT(markAll()));
  menu->insertSeparator();
  menu->insertItem("Find...",composeView,SLOT(toDo()));
  menuBar->insertItem("Edit",menu);

  menu = new QPopupMenu();
  menu->insertItem("Recipients...",composeView,SLOT(toDo()));
  menu->insertSeparator();
  menu->insertItem("Attach...",composeView,SLOT(toDo()));
  menu->insertSeparator();
  QPopupMenu *menv = new QPopupMenu();
  menv->insertItem("High");
  menv->insertItem("Normal");
  menv->insertItem("Low");
  menu->insertItem("Priority",menv);
  menuBar->insertItem("Message",menu);

  menuBar->insertSeparator();

  menu = new QPopupMenu();
  menu->insertItem("Help",this,SLOT(invokeHelp()),ALT + Key_H);
  menu->insertSeparator();
  menu->insertItem("About",this,SLOT(about()));
  menuBar->insertItem("Help",menu);

  setMenu(menuBar);
}

void KMComposeWin::setupToolBar()
{
	QString pixdir = "";   // pics dir code "inspired" by kghostview (thanks)
	char *kdedir = getenv("KDEDIR");
	if (kdedir) pixdir.append(kdedir);
	 else pixdir.append("/usr/local/kde");
	pixdir.append("/lib/pics/toolbar/");

	toolBar = new KToolBar(this);

	QPixmap pixmap;

	pixmap.load(pixdir+"kmnew.xpm");
	toolBar->insertItem(pixmap,0,SIGNAL(clicked()),composeView,SLOT(toDo()),TRUE,"New Composer");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"kmsend.xpm");
	toolBar->insertItem(pixmap,0,SIGNAL(clicked()),composeView,SLOT(toDo()),TRUE,"Send");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"reload.xpm");
	toolBar->insertItem(pixmap,2,SIGNAL(clicked()),composeView,SLOT(copyText()),TRUE,"Undo");
	pixmap.load(pixdir+"editcopy.xpm");
	toolBar->insertItem(pixmap,3,SIGNAL(clicked()),composeView,SLOT(copyText()),TRUE,"Copy");
	pixmap.load(pixdir+"editcut.xpm");
	toolBar->insertItem(pixmap,4,SIGNAL(clicked()),composeView,SLOT(cutText()),TRUE,"Cut");
	pixmap.load(pixdir+"editpaste.xpm");
	toolBar->insertItem(pixmap,5,SIGNAL(clicked()),composeView,SLOT(pasteText()),TRUE,"Paste");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"thumb_up.xpm");
	toolBar->insertItem(pixmap,6,SIGNAL(clicked()),composeView,SLOT(toDo()),TRUE,"Recipients");
	pixmap.load(pixdir+"kmaddressbook.xpm");
	toolBar->insertItem(pixmap,7,SIGNAL(clicked()),composeView,SLOT(toDo()),TRUE,"Addressbook");
	pixmap.load(pixdir+"kmattach.xpm");
	toolBar->insertItem(pixmap,8,SIGNAL(clicked()),composeView,SLOT(toDo()),TRUE,"Attach");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"kmsave.xpm");
	toolBar->insertItem(pixmap,11,SIGNAL(clicked()),composeView,SLOT(saveMail()),TRUE,"Save");
	pixmap.load(pixdir+"kmprint.xpm");
	toolBar->insertItem(pixmap,12,SIGNAL(clicked()),composeView,SLOT(toDo()),TRUE,"Print");
	pixmap.load(pixdir+"help.xpm");
	toolBar->insertItem(pixmap,13,SIGNAL(clicked()),this,SLOT(invokeHelp()),TRUE,"Help");

	addToolBar(toolBar);
}


void KMComposeWin::abort()
{
  close();
}

void KMComposeWin::about()
{
	KMsgBox::message(this,"About",
	 "kmail [ALPHA]\n\n"
	 "Yat-Nam Lo <lynx@topaz.hknet.com>\n"
	 "Stephan Meyer <Stephan.Meyer@munich.netsurf.de>\n"
	 "Stefan Taferner <taferner@alpin.or.at>\n"
	 "Markus Wübben <markus.wuebben@kde.org>\n\n"
	 "This program is covered by the GPL.",1);
}

void KMComposeWin::invokeHelp()
{

  KApplication::getKApplication()->invokeHTMLHelp("","");

}

void KMComposeWin::newComposer()
{
	KMComposeWin *d;
	d = new KMComposeWin(NULL);
	d->show();
	d->resize(d->size());
}

void KMComposeWin::newMailReader()
{
	KMMainWin *d;
	d = new KMMainWin(NULL);
	d->show();
	d->resize(d->size());
}

void KMComposeWin::toDo()
{
	//  KMMainWin::doUnimplemented(); //is private :-(

   KMsgBox::message(this,"Ouch",
			 "Not yet implemented!\n"
			 "We are sorry for the inconvenience :-)",1);
}

void KMComposeWin::closeEvent(QCloseEvent *e)
{
	KTopLevelWidget::closeEvent(e);
	delete this;
	if (!(--windowCount)) qApp->quit();
}

