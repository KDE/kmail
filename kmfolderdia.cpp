#include <qstring.h>
#include <qlabel.h>
#include <qdir.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include "kmmainwin.h"
#include "kmglobal.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctfolder.h"

#include <assert.h>

#include "kmfolderdia.moc"

KMFolderDialog::KMFolderDialog(KMAcctFolder* aFolder, QWidget *parent,
			       const char *name) :
  QDialog(parent,name,TRUE)
{
	KMAccount* act;

	assert(aFolder != NULL);
	folder = aFolder;

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

	// grab the list of accounts associated with the folder
	for (act=folder->account(); act; act=folder->nextAccount())
	{
		assocList->inSort(act->name());
	}

	// insert list of available accounts
	for (act=acctMgr->first(); act; act=acctMgr->next())
	{
		accountList->inSort(act->name());
	}
}

KMFolderDialog::~KMFolderDialog() {
	delete acctMgr;
}

void KMFolderDialog::doAccept() {

#ifdef BROKEN
	// test if account selection is correct
	unsigned int i,a=0,b=0;
	for (i=0;i<assocList->count();i++) {
		if (acctMgr->find(QString(assocList->text(i)))->config->readEntry("access method")=="maintain remotely")
		 a++; else b++;
	}
	if (a && b)
	  KMsgBox::message(this,"Error",
	   "You may not select both locally and remotely maintained accounts.",1); else
	    if (a>1)
	      KMsgBox::message(this,"Error",
	       "You may not select more than one remotely maintained account.",1); else
#endif //BROKEN

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

//-----------------------------------------------------------------------------
void KMFolderDialog::doRemove()
{
	int i;
	QString s;
	s=assocList->text(i=assocList->currentItem());
	assocList->removeItem(i);
	if (assocList->currentItem()==-1) removeButton->setEnabled(FALSE);
	accountList->inSort(s);
}

//-----------------------------------------------------------------------------
void KMFolderDialog::accept()
{
	QString acctName;
	unsigned int i;
	KMAccount* act;

	folder->clearAccountList();

	for (i=0; i<assocList->count(); i++)
	{
		acctName = assocList->text(i);
		if (!(act = acctMgr->find(acctName))) continue;
		folder->addAccount(act);
	}

	QDialog::accept();
}

