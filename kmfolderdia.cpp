// kmfolderdia.cpp

#include <qstring.h>
#include <qlabel.h>
#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <kapp.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlistbox.h>

#include "kmmainwin.h"
#include "kmglobal.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmfoldermgr.h"

#include <assert.h>
#include <klocale.h>

#include "kmfolderdia.moc"


//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog(KMFolder* aFolder, QWidget *parent,
			       const char *name) :
  KMFolderDialogInherited(parent, name, TRUE)
{
  KMAccount* acct;
  QLabel *label;
  QString type;

  folder = (KMAcctFolder*)aFolder;

  label = new QLabel(this);
  label->setGeometry(20,20,60,25);
  label->setText(i18n("Name:"));
  label->setAlignment(290);

  nameEdit = new QLineEdit(this);
  nameEdit->setGeometry(90,20,320,25);
  nameEdit->setFocus();
  nameEdit->setText(folder ? folder->name() : i18n("unnamed"));

  if (folder)
  {
    type = folder->type();
    if (!type.isEmpty() && type!="plain")
      nameEdit->setEnabled(false);
  }

  label = new QLabel(this);
  label->setText(i18n("Associated with"));
  label->adjustSize();
  label->move(20,74);

  assocList = new QListBox(this);
  assocList->setGeometry(20,95,160,140);
  connect(assocList,SIGNAL(highlighted(int)),this,SLOT(doAssocHighlighted(int)));
  connect(assocList,SIGNAL(selected(int)),this,SLOT(doAssocSelected(int)));

  label = new QLabel(this);
  label->setText(i18n("Unassociated Accounts"));
  label->adjustSize();
  label->move(250,74);

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
  button->setText(i18n("OK"));
  connect(button,SIGNAL(clicked()),this,SLOT(doAccept()));

  if (type=="Out" || type=="St")
    button->setEnabled(false);

  button = new QPushButton(this);
  button->setGeometry(310,260,100,30);
  button->setText(i18n("Cancel"));
  connect(button,SIGNAL(clicked()),this,SLOT(reject()));

  resize(430,340);
  setFixedSize(430,340);

  if (folder)
  {
    // Grab the list of accounts associated with the given folder.
    for (acct=folder->account(); acct; acct=folder->nextAccount())
    {
      assocList->inSort(acct->name());
    }
  }

  // insert list of available accounts that are not associated with
  // any account
  for (acct=acctMgr->first(); acct; acct=acctMgr->next())
  {
    if (!acct->folder())
      accountList->inSort(acct->name());
  }
}


//-----------------------------------------------------------------------------
void KMFolderDialog::doAccept()
{
  QString acctName;
  KMAccount* acct;
  unsigned int i;
  QString fldName, oldFldName;

  if (folder) oldFldName = folder->name();
  if (*nameEdit->text()) fldName = nameEdit->text();
  else fldName = oldFldName;
  if (fldName.isEmpty()) fldName = i18n("unnamed");

  if (!folder) folder = (KMAcctFolder*)folderMgr->createFolder(fldName);
  else if (oldFldName != fldName)
  {
    folder->rename(fldName);
    folderMgr->contentsChanged();
  }

  if (folder)
  {
    folder->clearAccountList();

    for (i=0; i<assocList->count(); i++)
    {
      acctName = assocList->text(i);
      if (!(acct = acctMgr->find(acctName))) continue;
      folder->addAccount(acct);
    }
  }

  KMFolderDialogInherited::accept();
}


//-----------------------------------------------------------------------------
void KMFolderDialog::doAdd()
{
  int i;
  QString s;
  s=accountList->text(i=accountList->currentItem());
  accountList->removeItem(i);
  if (accountList->currentItem()==-1) addButton->setEnabled(FALSE);
  assocList->inSort(s);
}


//-----------------------------------------------------------------------------
void KMFolderDialog::doAccountHighlighted(int)
{
  addButton->setEnabled(TRUE);
}


//-----------------------------------------------------------------------------
void KMFolderDialog::doAccountSelected(int)
{
  doAdd();
}


//-----------------------------------------------------------------------------
void KMFolderDialog::doAssocHighlighted(int)
{
  removeButton->setEnabled(TRUE);
}


//-----------------------------------------------------------------------------
void KMFolderDialog::doAssocSelected(int)
{
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
