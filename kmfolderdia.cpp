#include <qstring.h>
#include <qlabel.h>
#include <qdir.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include "kmmainwin.h"
#include "kmfolderdia.moc"

KMFolderDialog::KMFolderDialog(QWidget *parent,const char *name) : QDialog(parent,name,TRUE)
{
	accountMan=new KMAccountMan();

	QLabel *label;
	label = new QLabel(this);
	label->setGeometry(20,20,40,25);
	label->setText("Name");
	label->setAlignment(290);

	nameEdit = new QLineEdit(this);
	nameEdit->setGeometry(70,20,340,25);
	nameEdit->setFocus();

	label = new QLabel(this);
	label->setGeometry(20,70,100,25);
	label->setText("Associated with");

	assocList = new QListBox(this);
	assocList->setGeometry(20,95,160,140);
	connect(assocList,SIGNAL(highlighted(int)),this,SLOT(doAssocHighlighted(int)));
	connect(assocList,SIGNAL(selected(int)),this,SLOT(doAssocSelected(int)));

	label = new QLabel(this);
	label->setGeometry(250,70,100,25);
	label->setText("Accounts");

	accountList = new QListBox(this);
	accountList->setGeometry(250,95,160,140);
	connect(accountList,SIGNAL(highlighted(int)),this,SLOT(doAccountHighlighted(int)));
	connect(accountList,SIGNAL(selected(int)),this,SLOT(doAccountSelected(int)));

	addButton = new QPushButton(this);
	addButton->setGeometry(190,115,50,40);
	addButton->setText("<<");
	addButton->setEnabled(FALSE);
	connect(addButton,SIGNAL(clicked()),this,SLOT(doAdd()));

	removeButton = new QPushButton(this);
	removeButton->setGeometry(190,175,50,40);
	removeButton->setText(">>");
	removeButton->setEnabled(FALSE);
	connect(removeButton,SIGNAL(clicked()),this,SLOT(doRemove()));

	QPushButton *button = new QPushButton(this);
	button->setGeometry(190,260,100,30);
	button->setText("OK");
	connect(button,SIGNAL(clicked()),this,SLOT(doAccept()));

	button = new QPushButton(this);
	button->setGeometry(310,260,100,30);
	button->setText("Cancel");
	connect(button,SIGNAL(clicked()),this,SLOT(reject()));

	resize(430,340);

// this code looks really ugly, but all it does is
// grab the list of accounts for the folder and insert all
// account names into either the assocList or accountList

	QDir dir;
	QFile *f;
	QTextStream *g;
	QString s;
	QStrList l;
	((KMMainView *)parentWidget())->folderTree->cdFolder(&dir);
	if (dir.exists("accounts")) {
		f=new QFile(dir.absFilePath("accounts"));
		f->open(IO_ReadWrite);
		g=new QTextStream(f);
		while (!g->eof()) {
			s=g->readLine();
			if (accountMan->findAccount(s)!=NULL) {
				assocList->inSort(s);
				l.append(s);
			}
		}
		delete g;
		delete f;
	}

	for (unsigned int i=0;i<accountMan->count();i++) {
		s=accountMan->at(i)->name;
		if (l.find(s)==-1)
			accountList->inSort(s);
	}
}

KMFolderDialog::~KMFolderDialog() {
	delete accountMan;
}

void KMFolderDialog::doAccept() {
	// test if account selection is correct
	unsigned int i,a=0,b=0;
	for (i=0;i<assocList->count();i++) {
		if (accountMan->findAccount(QString(assocList->text(i)))->config->readEntry("access method")=="maintain remotely")
		 a++; else b++;
	}
	if (a && b)
	  KMsgBox::message(this,"Error",
	   "You may not select both locally and remotely maintained accounts.",1); else
	    if (a>1)
	      KMsgBox::message(this,"Error",
	       "You may not select more than one remotely maintained account.",1); else
		accept();
}

void KMFolderDialog::doAdd() {
	int i;
	QString s;
	s=accountList->text(i=accountList->currentItem());
	accountList->removeItem(i);
	if (accountList->currentItem()==-1) addButton->setEnabled(FALSE);
	assocList->inSort(s);
}

void KMFolderDialog::doAccountHighlighted(int) {
	addButton->setEnabled(TRUE);
}

void KMFolderDialog::doAccountSelected(int) {
	doAdd();
}

void KMFolderDialog::doAssocHighlighted(int) {
	removeButton->setEnabled(TRUE);
}

void KMFolderDialog::doAssocSelected(int) {
	doRemove();
}

void KMFolderDialog::doRemove() {
	int i;
	QString s;
	s=assocList->text(i=assocList->currentItem());
	assocList->removeItem(i);
	if (assocList->currentItem()==-1) removeButton->setEnabled(FALSE);
	accountList->inSort(s);
}

void KMFolderDialog::accept() {
	QDir dir;
	QFile *f;
	QTextStream *s;
	((KMMainView *)parentWidget())->folderTree->cdFolder(&dir);
	dir.remove("accounts");
	if (assocList->count()) {
		f=new QFile(dir.absFilePath("accounts"));
		f->open(IO_ReadWrite);
		s=new QTextStream(f);
		for (unsigned int i=0;i<assocList->count();i++) {
			(*s)<<assocList->text(i);
			(*s)<<"\n";
		}
		delete s;
		delete f;
	}
	QDialog::accept();
}

