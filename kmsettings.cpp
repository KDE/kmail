// kmsettings.cpp
#include <qdir.h>
#include <qlabel.h>
#include <qpushbt.h>
#include <qfiledlg.h>
#include <qframe.h>
#include <kmsgbox.h>
#include <qlayout.h>
#include <qgrpbox.h>
#include <qlayout.h>
#include "util.h"
#include "kmmainwin.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmacctfolder.h"
#include "kmglobal.h"
#include "kmsender.h"
#include "ktablistbox.h"
#include <klocale.h>
//------
#include "kmsettings.moc"

//-----------------------------------------------------------------------------
KMSettings::KMSettings(QWidget *parent, const char *name) :
  QTabDialog(parent, name, TRUE)
{
  initMetaObject();

  config=app->getConfig();
  setCaption(nls->translate("Settings"));
  resize(500,600);
  setCancelButton();
  setDefaultButton();
  connect(this,SIGNAL(defaultButtonPressed()),this,SLOT(setDefaults()));

  createTabIdentity(this);
  createTabNetwork(this);
}


//-----------------------------------------------------------------------------
KMSettings::~KMSettings()
{
  delete acctMgr;
}


//-----------------------------------------------------------------------------
QLineEdit* KMSettings::createLabeledEntry(QWidget* parent, QGridLayout* grid,
					  const char* aLabel,
					  const char* aText, 
					  int gridy, int gridx,
					  QPushButton** detail_return)
{
  QLabel* label = new QLabel(parent);
  QLineEdit* edit = new QLineEdit(parent);
  QPushButton* sel;

  label->setText(nls->translate(aLabel));
  label->adjustSize();
  label->resize((int)label->sizeHint().width(),label->sizeHint().height() + 6);
  label->setMinimumSize(label->size());
  grid->addWidget(label, gridy, gridx++);

  if (aText) edit->setText(aText);
  edit->setMinimumSize(100, label->height()+2);
  edit->setMaximumSize(1000, label->height()+2);
  grid->addWidget(edit, gridy, gridx++);

  if (detail_return)
  {
    sel = new QPushButton("...", parent);
    sel->setFixedSize(sel->sizeHint().width(), label->height());
    grid->addWidget(sel, gridy, gridx++);
    *detail_return = sel;
  }

  return edit;
}


//-----------------------------------------------------------------------------
QPushButton* KMSettings::createPushButton(QWidget* parent, QGridLayout* grid,
					  const char* label, 
					  int gridy, int gridx)
{
  QPushButton* button = new QPushButton(parent, label);
  button->setText(nls->translate(label));
  button->adjustSize();
  button->setMinimumSize(button->size());
  grid->addWidget(button, gridy, gridx);

  return button;
}


//-----------------------------------------------------------------------------
void KMSettings::createTabIdentity(QWidget* parent)
{
  QWidget* tab = new QWidget(parent);
  QGridLayout* grid = new QGridLayout(tab, 8, 3, 20, 6);
  QPushButton* button;

  nameEdit = createLabeledEntry(tab, grid, "Name", NULL, 0, 0);
  orgEdit = createLabeledEntry(tab, grid, "Organization", NULL, 1, 0);
  emailEdit = createLabeledEntry(tab, grid, "Email Address", NULL, 2, 0);
  replytoEdit = createLabeledEntry(tab, grid, "Reply-To Address", NULL, 3, 0);

  sigEdit = createLabeledEntry(tab, grid, "Signature File", NULL, 4, 0, 
			       &button);
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSigFile()));

  config->setGroup("Identity");
  nameEdit->setText(config->readEntry("Name"));
  orgEdit->setText(config->readEntry("Organization"));
  emailEdit->setText(config->readEntry("Email Address"));
  replytoEdit->setText(config->readEntry("Reply-To Address"));
  sigEdit->setText(config->readEntry("Signature File"));

  grid->setColStretch(0,0);
  grid->setColStretch(1,1);
  grid->setColStretch(2,0);

  identityTab = tab;
  addTab(tab, nls->translate("Identity"));
  grid->activate();
}


//-----------------------------------------------------------------------------
void KMSettings::createTabNetwork(QWidget* parent)
{
  QWidget* tab = new QWidget(parent);
  QBoxLayout* box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 4);
  QGridLayout* grid;
  QGroupBox*   grp;
  QButtonGroup*   bgrp;
  QPushButton* button;
  QLabel* label;
  KMAccount* act;
  QString str;

  networkTab = tab;

  //---- group: sending mail
  bgrp = new QButtonGroup(nls->translate("Sending Mail"), tab);
  box->addWidget(bgrp);
  grid = new QGridLayout(bgrp, 5, 4, 20, 4);

  sendmailRadio = new QRadioButton(bgrp);
  sendmailRadio->setMinimumSize(sendmailRadio->size());
  sendmailRadio->setText(nls->translate("Sendmail"));
  bgrp->insert(sendmailRadio);
  grid->addMultiCellWidget(sendmailRadio, 0, 0, 0, 3);

  sendmailLocationEdit = createLabeledEntry(bgrp, grid, "Location", NULL, 
					    1, 1, &button);
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSendmailLocation()));

  smtpRadio = new QRadioButton(bgrp);
  smtpRadio->setText(nls->translate("SMTP"));
  smtpRadio->setMinimumSize(smtpRadio->size());
  bgrp->insert(smtpRadio);
  grid->addMultiCellWidget(smtpRadio, 2, 2, 0, 3);

  smtpServerEdit = createLabeledEntry(bgrp, grid, "Server", NULL, 3, 1);
  smtpPortEdit = createLabeledEntry(bgrp, grid, "Port", NULL, 4, 1);
  grid->setColStretch(0,4);
  grid->setColStretch(1,1);
  grid->setColStretch(2,10);
  grid->setColStretch(3,0);
  grid->activate();
  bgrp->adjustSize();

  if (msgSender->method()==KMSender::smMail) 
    sendmailRadio->setChecked(TRUE);
  else if (msgSender->method()==KMSender::smSMTP)
    smtpRadio->setChecked(TRUE);

  sendmailLocationEdit->setText(msgSender->mailer());
  smtpServerEdit->setText(msgSender->smtpHost());
  smtpPortEdit->setText(QString(msgSender->smtpPort()));


  //---- group: incoming mail
  grp = new QGroupBox(nls->translate("Incoming Mail"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 5, 2, 20, 8);

  label = new QLabel(grp);
  label->setText(nls->translate("Accounts"));
  label->setMinimumSize(label->size());
  grid->addMultiCellWidget(label, 0, 0, 0, 1);

  accountList = new KTabListBox(grp, "LstAccounts", 3);
  accountList->setColumn(0, nls->translate("Name"), 80);
  accountList->setColumn(1, nls->translate("Type"), 60);
  accountList->setColumn(2, nls->translate("Folder"), 80);
  accountList->setMinimumSize(50, 50);
  connect(accountList,SIGNAL(highlighted(int,int)),
	  this,SLOT(accountSelected(int,int)));
  connect(accountList,SIGNAL(selected(int,int)),
	  this,SLOT(modifyAccount(int,int)));
  grid->addMultiCellWidget(accountList, 1, 4, 0, 0);

  addButton = createPushButton(grp, grid, nls->translate("Add..."), 1, 1);
  connect(addButton,SIGNAL(clicked()),this,SLOT(addAccount()));

  modifyButton = createPushButton(grp, grid, nls->translate("Modify..."),2,1);
  connect(modifyButton,SIGNAL(clicked()),this,SLOT(modifyAccount2()));

  removeButton = createPushButton(grp, grid, nls->translate("Delete"), 3, 1);
  connect(removeButton,SIGNAL(clicked()),this,SLOT(removeAccount()));

  grid->setColStretch(0, 10);
  grid->setColStretch(1, 0);
  grid->setRowStretch(4, 10);
  grid->activate();
  grp->adjustSize();

  accountList->clear();
  for (act=acctMgr->first(); act; act=acctMgr->next())
    tabNetworkAddAcct(accountList, act);

  addTab(tab, nls->translate("Network"));
  box->addStretch(100);
  box->activate();
  tab->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::tabNetworkAddAcct(KTabListBox* actList, KMAccount* act, 
				   int idx)
{
  QString str;
  
  str = "";
  str += act->name();
  str += "\n";
  str += act->type();
  str += "\n";
  if (act->folder()) str += act->folder()->name();

  actList->insertItem(str, idx);
}


//-----------------------------------------------------------------------------
void KMSettings::accountSelected(int,int)
{
  modifyButton->setEnabled(TRUE);
  removeButton->setEnabled(TRUE);
}

//-----------------------------------------------------------------------------
void KMSettings::addAccount()
{
#ifdef BROKEN
  KMAccount act = acctMgr->

  KMAccount *a=acctMgr->createAccount(".temp");
  KMAccountSettings *d=new KMAccountSettings(this,NULL,a);
  d->setCaption("Create Account");
  if (d->exec())
  {
    QString s=a->name;
    acctMgr->renameAccount(QString(".temp"),s);
    // `a' is not longer valid here !!!
    accountList->inSort(s);
  }
  else acctMgr->removeAccount(QString(".temp"));

  delete d;
#endif
}

//-----------------------------------------------------------------------------
void KMSettings::chooseSendmailLocation()
{
  QFileDialog *d=new QFileDialog(".","*",this,NULL,TRUE);
  d->setCaption(nls->translate("Choose Sendmail Location"));
  if (d->exec()) sendmailLocationEdit->setText(d->selectedFile());
  delete d;
}

//-----------------------------------------------------------------------------
void KMSettings::chooseSigFile()
{
  QFileDialog *d=new QFileDialog(QDir::homeDirPath(),"*",this,NULL,TRUE);
  d->setCaption(nls->translate("Choose Signature File"));
  if (d->exec()) sigEdit->setText(d->selectedFile());
  delete d;
}

//-----------------------------------------------------------------------------
void KMSettings::modifyAccount(int index,int)
{
  KMAccount* act = acctMgr->find(accountList->text(index,0));
  KMAccountSettings* d;

  assert(act != NULL);

  d = new KMAccountSettings(this, NULL, act);
  d->exec();
  delete d;

  accountList->removeItem(index);
  tabNetworkAddAcct(accountList, act, index);
}

//-----------------------------------------------------------------------------
void KMSettings::modifyAccount2() {
  modifyAccount(accountList->currentItem(),-1);
}

//-----------------------------------------------------------------------------
void KMSettings::removeAccount() {
  QString s,t;
  s=accountList->text(accountList->currentItem());
  t="Are you sure you want to remove the account \"";
  t+=s; t+="\" ?";
  if ((KMsgBox::yesNo(this,"Confirmation",t))==1) {
    acctMgr->remove(acctMgr->find(s));
    accountList->removeItem(accountList->currentItem());
    if (!accountList->count()) {
      modifyButton->setEnabled(FALSE);
      removeButton->setEnabled(FALSE);
    }
  }
}

//-----------------------------------------------------------------------------
void KMSettings::setDefaults()
{
  QString s="";
  s.append(QDir::home().path());
  sigEdit->setText(s+"/.signature");
  sendmailRadio->setChecked(TRUE);
  sendmailLocationEdit->setText("/usr/sbin/sendmail");
  smtpRadio->setChecked(FALSE);
  smtpPortEdit->setText("25");
}

//-----------------------------------------------------------------------------
void KMSettings::done(int r)
{
  QTabDialog::done(r);
  if (r) {
    config->setGroup("Identity");
    config->writeEntry("Name",nameEdit->text());
    config->writeEntry("Organization",orgEdit->text());
    config->writeEntry("Email Address",emailEdit->text());
    config->writeEntry("Reply-To Address",replytoEdit->text());
    config->writeEntry("Signature File",sigEdit->text());
    config->setGroup("Network");
    if (sendmailRadio->isChecked()) config->writeEntry("Outgoing Type","Sendmail");
    else if (smtpRadio->isChecked()) config->writeEntry("Outgoing Type","SMTP");
    config->writeEntry("Sendmail Location",sendmailLocationEdit->text());
    config->writeEntry("SMTP Server",smtpServerEdit->text());
    config->writeEntry("SMTP Port",smtpPortEdit->text());
  }
}


//=============================================================================
//=============================================================================
KMAccountSettings::KMAccountSettings(QWidget *parent, const char *name,
				     KMAccount *a):
  QDialog(parent,name,TRUE)
{
  QString s;
  int i;

  initMetaObject();

  account=a;

  setCaption(nls->translate("Modify Account"));

  QLabel *label;
  label = new QLabel(this);
  label->setGeometry(25,20,75,25);
  label->setText("Name");
  label->setAlignment(290);

  nameEdit = new QLineEdit(this);
  nameEdit->setGeometry(110,20,220,25);
  nameEdit->setFocus();
  if (a->name()!=".temp") nameEdit->setText(a->name());

  label = new QLabel(this);
  label->setGeometry(20,55,80,25);
  label->setText("Access via");
  label->setAlignment(290);

  typeList = new QComboBox(FALSE,this);
  typeList->setGeometry(110,55,100,25);
  typeList->insertItem("Local Inbox");
  typeList->insertItem("IMAP");
  typeList->insertItem("POP3");
  connect(typeList,SIGNAL(activated(int)),this,SLOT(typeSelected(int)));

  QFrame *frame;
  frame = new QFrame(this);
  frame->setGeometry(425,0,15,305);
  frame->setFrameStyle(QFrame::VLine | QFrame::Sunken);

  QPushButton *button;
  button = new QPushButton(this);
  button->setGeometry(445,15,100,30);
  button->setText("OK");
  connect(button,SIGNAL(clicked()),this,SLOT(accept()));

  button = new QPushButton(this);
  button->setGeometry(445,60,100,30);
  button->setText("Cancel");
  connect(button,SIGNAL(clicked()),this,SLOT(reject()));

  local = new QWidget(this);
  local->setGeometry(0,90,430,215);

  label = new QLabel(local);
  label->setGeometry(10,20,50,25);
  label->setText("Location");
  label->setAlignment(290);

  locationEdit = new QLineEdit(local);
  locationEdit->setGeometry(70,20,315,25);
  locationEdit->setFocus();
  locationEdit->setText(a->config()->readEntry("location"));

  button=new QPushButton(local);
  button->setGeometry(390,20,30,25);
  button->setText("...");
  connect(button,SIGNAL(clicked()),this,SLOT(chooseLocation()));

  remote = new QWidget(this);
  remote->setGeometry(0,90,430,215);

  label = new QLabel(remote);
  label->setGeometry(10,15,60,25);
  label->setText("Host");
  label->setAlignment(290);

  hostEdit = new QLineEdit(remote);
  hostEdit->setGeometry(80,15,335,25);
  hostEdit->setFocus();
  hostEdit->setText(account->config()->readEntry("host"));

  label = new QLabel(remote);
  label->setGeometry(10,50,60,25);
  label->setText("Port");
  label->setAlignment(290);

  portEdit = new QLineEdit(remote);
  portEdit->setGeometry(80,50,70,25);
  portEdit->setText(account->config()->readEntry("port"));

  label = new QLabel(remote);
  label->setGeometry(20,95,50,25);
  label->setText("Mailbox");
  label->setAlignment(290);

  mailboxEdit = new QLineEdit(remote);
  mailboxEdit->setGeometry(80,95,165,25);
  mailboxEdit->setText(account->config()->readEntry("mailbox"));

  label = new QLabel(remote);
  label->setGeometry(10,140,60,25);
  label->setText("Login");
  label->setAlignment(290);

  loginEdit = new QLineEdit(remote);
  loginEdit->setGeometry(80,140,165,25);
  loginEdit->setText(account->config()->readEntry("login"));

  label = new QLabel(remote);
  label->setGeometry(10,175,60,25);
  label->setText("Password");
  label->setAlignment(290);

  passEdit = new QLineEdit(remote);
  passEdit->setGeometry(80,175,165,25);
  passEdit->setEchoMode(QLineEdit::Password);
  passEdit->setText(account->config()->readEntry("password"));

  QButtonGroup *buttonGroup = new QButtonGroup(remote);
  buttonGroup->setGeometry(265,90,150,105);
  buttonGroup->setTitle("access method");

  accessMethod1 = new QRadioButton(remote);
  accessMethod1->setGeometry(275,110,120,25);
  accessMethod1->setText("maintain remotely");
  buttonGroup->insert(accessMethod1);

  accessMethod2 = new QRadioButton(remote);
  accessMethod2->setGeometry(275,135,120,25);
  accessMethod2->setText("move messages");
  buttonGroup->insert(accessMethod2);

  accessMethod3 = new QRadioButton(remote);
  accessMethod3->setGeometry(275,160,120,25);
  accessMethod3->setText("copy messages");
  buttonGroup->insert(accessMethod3);

  if (account->config()->readEntry("access method")=="maintain remotely")
    accessMethod1->setChecked(TRUE); else
      if (account->config()->readEntry("access method")=="move messages")
	accessMethod2->setChecked(TRUE); else
	  if (account->config()->readEntry("access method")=="copy messages")
	    accessMethod3->setChecked(TRUE); else
	      accessMethod2->setChecked(TRUE);

  resize(555,305);

  s=a->config()->readEntry("type");
  if (s=="inbox") i=0; else
    if (s=="imap") i=1; else
      if (s=="pop3") i=2; else i=0;
  typeList->setCurrentItem(i);
  changeType(i);
}

//-----------------------------------------------------------------------------
void KMAccountSettings::changeType(int index) {
  switch (index) {
  case 0 :	remote->hide();
    local->show();
    break;
  case 1 :	local->hide();
    remote->show();
    mailboxEdit->setEnabled(TRUE);
    if (strcmp(portEdit->text(),"")==0) portEdit->setText("143");
    break;
  case 2 :	local->hide();
    remote->show();
    mailboxEdit->setEnabled(FALSE);
    if (strcmp(portEdit->text(),"")==0) portEdit->setText("110");
    break;
  }
}

//-----------------------------------------------------------------------------
void KMAccountSettings::chooseLocation() {
  QFileDialog *d=new QFileDialog(".","*",this,NULL,TRUE);
  d->setCaption("Choose Location");
  if (d->exec()) locationEdit->setText(d->selectedFile());
  delete d;
}

//-----------------------------------------------------------------------------
void KMAccountSettings::typeSelected(int index) {
  changeType(index);
}

//-----------------------------------------------------------------------------
void KMAccountSettings::accept() {
  account->setName(nameEdit->text());
  switch (typeList->currentItem()) {
  case 0 :	account->config()->writeEntry("type","inbox");
    break;
  case 1 :	account->config()->writeEntry("type","imap");
    break;
  case 2 :	account->config()->writeEntry("type","pop3");
    break;
  }
  account->config()->writeEntry("location",locationEdit->text());
  account->config()->writeEntry("host",hostEdit->text());
  account->config()->writeEntry("port",portEdit->text());
  account->config()->writeEntry("mailbox",mailboxEdit->text());
  account->config()->writeEntry("login",loginEdit->text());
  account->config()->writeEntry("password",passEdit->text());
  if (accessMethod1->isChecked())
    account->config()->writeEntry("access method","maintain remotely"); else
      if (accessMethod2->isChecked())
	account->config()->writeEntry("access method","move messages"); else
	  if (accessMethod3->isChecked())
	    account->config()->writeEntry("access method","copy messages");

  QDialog::accept();
}

