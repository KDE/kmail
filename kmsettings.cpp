#include <qlabel.h>
#include <qpushbt.h>
#include <qfiledlg.h>
#include <qframe.h>
#include <kmsgbox.h>
#include "util.h"
#include "kmmainwin.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kmglobal.h"

#include "kmsettings.moc"

//-----------------------------------------------------------------------------
KMSettings::KMSettings(QWidget *parent, const char *name) :
  QTabDialog(parent,name,TRUE)
{
  KMAccount* act;
  config=app->getConfig();
  setCaption("Settings");
  resize(570,545);
  setCancelButton();
  setDefaultButton();
  connect(this,SIGNAL(defaultButtonPressed()),this,SLOT(setDefaults()));

  identityTab=new QWidget(this);

  QLabel* label;
  label = new QLabel(identityTab);
  label->setGeometry(30,15,90,25);
  label->setText("Name");
  label->setAlignment(290);

  nameEdit = new QLineEdit(identityTab);
  nameEdit->setGeometry(130,15,410,25);

  label = new QLabel(identityTab);
  label->setGeometry(30,50,90,25);
  label->setText("Organization");
  label->setAlignment(290);

  orgEdit = new QLineEdit(identityTab);
  orgEdit->setGeometry(130,50,410,25);

  label = new QLabel(identityTab);
  label->setGeometry(20,95,100,25);
  label->setText("Email Address");
  label->setAlignment(290);

  emailEdit = new QLineEdit(identityTab);
  emailEdit->setGeometry(130,95,410,25);

  label = new QLabel(identityTab);
  label->setGeometry(10,130,110,25);
  label->setText("Reply-To Address");
  label->setAlignment(290);

  replytoEdit = new QLineEdit(identityTab);
  replytoEdit->setGeometry(130,130,410,25);

  label = new QLabel(identityTab);
  label->setGeometry(30,175,90,25);
  label->setText("Signature File");
  label->setAlignment(290);

  sigEdit = new QLineEdit(identityTab);
  sigEdit->setGeometry(130,175,370,25);

  QPushButton* button;
  button = new QPushButton(identityTab);
  button->setGeometry(505,175,35,25);
  button->setText("...");
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSigFile()));

  config->setGroup("Identity");
  nameEdit->setText(config->readEntry("Name"));
  orgEdit->setText(config->readEntry("Organization"));
  emailEdit->setText(config->readEntry("Email Address"));
  replytoEdit->setText(config->readEntry("Reply-To Address"));
  sigEdit->setText(config->readEntry("Signature File"));

  addTab(identityTab,"Identity");

  networkTab=new QWidget(this);

  outgoingGroup = new QButtonGroup(networkTab);
  outgoingGroup->setGeometry(15,15,525,200);
  outgoingGroup->setTitle("Outgoing Mail");

  sendmailRadio = new QRadioButton(networkTab);
  sendmailRadio->setGeometry(35,35,100,25);
  sendmailRadio->setText("Sendmail");

  label = new QLabel(networkTab);
  label->setGeometry(55,65,70,25);
  label->setText("Location");
  label->setAlignment(290);

  sendmailLocationEdit = new QLineEdit(networkTab);
  sendmailLocationEdit->setGeometry(135,65,350,25);

  button = new QPushButton(networkTab);
  button->setGeometry(495,65,30,25);
  button->setText("...");
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSendmailLocation()));

  smtpRadio = new QRadioButton(networkTab);
  smtpRadio->setGeometry(35,105,60,25);
  smtpRadio->setText("SMTP");

  label = new QLabel(networkTab);
  label->setGeometry(55,135,70,25);
  label->setText("Server");
  label->setAlignment(290);

  smtpServerEdit = new QLineEdit(networkTab);
  smtpServerEdit->setGeometry(135,135,390,25);

  label = new QLabel(networkTab);
  label->setGeometry(25,175,100,25);
  label->setText("Port");
  label->setAlignment(290);

  smtpPortEdit = new QLineEdit(networkTab);
  smtpPortEdit->setGeometry(135,175,70,25);

  outgoingGroup->insert(sendmailRadio);
  outgoingGroup->insert(smtpRadio);

  incomingGroup = new QButtonGroup(networkTab);
  incomingGroup->setGeometry(15,230,525,225);
  incomingGroup->setTitle("Incoming Mail");

  label = new QLabel(networkTab);
  label->setGeometry(35,255,100,25);
  label->setText("Accounts");

  accountList = new QListBox(networkTab);
  accountList->setGeometry(35,280,355,160);
  connect(accountList,SIGNAL(highlighted(int)),this,SLOT(accountSelected(int)));
  connect(accountList,SIGNAL(selected(int)),this,SLOT(modifyAccount(int)));
  for (act=acctMgr->first(); act; act=acctMgr->next())
    accountList->inSort(act->name());

  addButton = new QPushButton(networkTab);
  addButton->setGeometry(405,280,120,40);
  addButton->setText("Add...");
  connect(addButton,SIGNAL(clicked()),this,SLOT(addAccount()));

  modifyButton = new QPushButton(networkTab);
  modifyButton->setGeometry(405,340,120,40);
  modifyButton->setText("Modify...");
  modifyButton->setEnabled(FALSE);
  connect(modifyButton,SIGNAL(clicked()),this,SLOT(modifyAccount2()));

  removeButton = new QPushButton(networkTab);
  removeButton->setGeometry(405,400,120,40);
  removeButton->setText("Remove");
  removeButton->setEnabled(FALSE);
  connect(removeButton,SIGNAL(clicked()),this,SLOT(removeAccount()));

  config->setGroup("Network");
  if (config->readEntry("Outgoing Type")=="Sendmail") sendmailRadio->setChecked(TRUE);
  else if (config->readEntry("Outgoing Type")=="SMTP") smtpRadio->setChecked(TRUE);
  sendmailLocationEdit->setText(config->readEntry("Sendmail Location"));
  smtpServerEdit->setText(config->readEntry("SMTP Server"));
  smtpPortEdit->setText(config->readEntry("SMTP Port"));

  addTab(networkTab,"Network");
}

//-----------------------------------------------------------------------------
KMSettings::~KMSettings() {
  delete acctMgr;
}

//-----------------------------------------------------------------------------
void KMSettings::accountSelected(int) {
  modifyButton->setEnabled(TRUE);
  removeButton->setEnabled(TRUE);
}

//-----------------------------------------------------------------------------
void KMSettings::addAccount() {
#ifdef BROKEN
  KMAccount *a=acctMgr->createAccount(".temp");
  KMAccountSettings *d=new KMAccountSettings(this,NULL,a);
  d->setCaption("Create Account");
  if (d->exec()) {
    QString s=a->name;
    acctMgr->renameAccount(QString(".temp"),s);
    // "a" is not longer valid here !!!
    accountList->inSort(s);
  } else acctMgr->removeAccount(QString(".temp"));
  delete d;
#endif
}

//-----------------------------------------------------------------------------
void KMSettings::chooseSendmailLocation() {
  QFileDialog *d=new QFileDialog(".","*",this,NULL,TRUE);
  d->setCaption("Choose Sendmail Location");
  if (d->exec()) sendmailLocationEdit->setText(d->selectedFile());
  delete d;
}

//-----------------------------------------------------------------------------
void KMSettings::chooseSigFile()
{
  QFileDialog *d=new QFileDialog(".","*",this,NULL,TRUE);
  d->setCaption("Choose Signature File");
  if (d->exec()) sigEdit->setText(d->selectedFile());
  delete d;
}

//-----------------------------------------------------------------------------
void KMSettings::modifyAccount(int index) {
  KMAccount *a=acctMgr->find(accountList->text(index));
  KMAccountSettings *d=new KMAccountSettings(this,NULL,a);
  d->setCaption("Modify Account");
  d->exec();
  accountList->removeItem(index);
  accountList->inSort(a->name());
  delete d;
}

//-----------------------------------------------------------------------------
void KMSettings::modifyAccount2() {
  modifyAccount(accountList->currentItem());
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

  account=a;

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

