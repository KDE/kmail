// kmsettings.cpp

#include <qdir.h>

#include "kmacctlocal.h"
#include "kmacctmgr.h"
#include "kmacctpop.h"
#include "kmacctseldlg.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmidentity.h"
#include "kmmainwin.h"
#include "kmsender.h"
#include "kmmessage.h"

#include <kapp.h>
#include <klocale.h>
#include <kmsgbox.h>
#include <ktablistbox.h>
#include <qbttngrp.h>
#include <qfiledlg.h>
#include <qframe.h>
#include <qgrpbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbt.h>
#include <qradiobt.h>
#include <qchkbox.h>

//------
#include "kmsettings.moc"


//-----------------------------------------------------------------------------
KMSettings::KMSettings(QWidget *parent, const char *name) :
  QTabDialog(parent, name, TRUE)
{
  initMetaObject();

  setCaption(nls->translate("Settings"));
  resize(500,500);
  setOKButton(nls->translate("Ok"));
  setCancelButton(nls->translate("Cancel"));

  connect(this, SIGNAL(applyButtonPressed()), this, SLOT(doApply()));
  connect(this, SIGNAL(cancelButtonPressed()), this, SLOT(doCancel()));

  createTabIdentity(this);
  createTabNetwork(this);
  createTabComposer(this);
}


//-----------------------------------------------------------------------------
KMSettings::~KMSettings()
{
  accountList->clear();
}


//-----------------------------------------------------------------------------
// Create an input field with a label and optional detail button ("...")
// The detail button is not created if detail_return is NULL.
// The argument 'label' is the label that will be left of the entry field.
// The argument 'text' is the text that will show up in the entry field.
// The whole thing is placed in the grid from row/col to the right.
static QLineEdit* createLabeledEntry(QWidget* parent, QGridLayout* grid,
				     const char* aLabel,
				     const char* aText, 
				     int gridy, int gridx,
				     QPushButton** detail_return=NULL)
{
  QLabel* label = new QLabel(parent);
  QLineEdit* edit = new QLineEdit(parent);
  QPushButton* sel;

  label->setText(aLabel);
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

  nameEdit = createLabeledEntry(tab, grid, nls->translate("Name:"), 
				identity->fullName(), 0, 0);
  orgEdit = createLabeledEntry(tab, grid, nls->translate("Organization:"), 
			       identity->organization(), 1, 0);
  emailEdit = createLabeledEntry(tab, grid, nls->translate("Email Address:"),
				 identity->emailAddr(), 2, 0);
  replytoEdit = createLabeledEntry(tab, grid, 
				   nls->translate("Reply-To Address:"),
				   identity->replyToAddr(), 3, 0);
  sigEdit = createLabeledEntry(tab, grid, nls->translate("Signature File:"),
			       identity->signatureFile(), 4, 0, &button);
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSigFile()));

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

  sendmailLocationEdit = createLabeledEntry(bgrp, grid, 
					    nls->translate("Location:"),
					    NULL, 1, 1, &button);
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSendmailLocation()));

  smtpRadio = new QRadioButton(bgrp);
  smtpRadio->setText(nls->translate("SMTP"));
  smtpRadio->setMinimumSize(smtpRadio->size());
  bgrp->insert(smtpRadio);
  grid->addMultiCellWidget(smtpRadio, 2, 2, 0, 3);

  smtpServerEdit = createLabeledEntry(bgrp, grid, nls->translate("Server:"),
				      NULL, 3, 1);
  smtpPortEdit = createLabeledEntry(bgrp, grid, nls->translate("Port:"),
				    NULL, 4, 1);
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
  QString tmp;
  smtpPortEdit->setText(tmp.setNum(msgSender->smtpPort()));


  //---- group: incoming mail
  grp = new QGroupBox(nls->translate("Incoming Mail"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 5, 2, 20, 8);

  label = new QLabel(grp);
  label->setText(nls->translate("Accounts:   (add at least one account!)"));
  label->setMinimumSize(label->size());
  grid->addMultiCellWidget(label, 0, 0, 0, 1);

  accountList = new KTabListBox(grp, "LstAccounts", 3);
  accountList->setColumn(0, nls->translate("Name"), 150);
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
  modifyButton->setEnabled(FALSE);

  removeButton = createPushButton(grp, grid, nls->translate("Delete"), 3, 1);
  connect(removeButton,SIGNAL(clicked()),this,SLOT(removeAccount()));
  removeButton->setEnabled(FALSE);

  grid->setColStretch(0, 10);
  grid->setColStretch(1, 0);

  grid->setRowStretch(1, 2);
  grid->setRowStretch(4, 2);
  grid->activate();
  grp->adjustSize();

  accountList->clear();
  for (act=acctMgr->first(); act; act=acctMgr->next())
    accountList->insertItem(tabNetworkAcctStr(act), -1);

  addTab(tab, nls->translate("Network"));
  box->addStretch(100);
  box->activate();
  tab->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::createTabComposer(QWidget *parent)
{
  QWidget *tab = new QWidget(parent);
  QBoxLayout* box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 4);
  QGridLayout* grid;
  QGroupBox* grp;
  QLabel* lbl;
  KConfig* config = app->getConfig();
  QString str;

  //---------- group: phrases
  grp = new QGroupBox(nls->translate("Phrases"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 7, 3, 20, 4);

  lbl = new QLabel(nls->translate(
        "The following placeholders are supported in the reply phrases:\n"
	"%D=date, %S=subject, %F=sender, %%=percent sign"), grp);
  lbl->adjustSize();
  lbl->setMinimumSize(100,lbl->size().height());
  grid->setRowStretch(0,10);
  grid->addMultiCellWidget(lbl, 0, 0, 0, 2);

  phraseReplyEdit = createLabeledEntry(grp, grid, 
				       nls->translate("Reply to sender:"),
				       NULL, 2, 0);
  phraseReplyAllEdit = createLabeledEntry(grp, grid, 
					  nls->translate("Reply to all:"),
					  NULL, 3, 0);
  phraseForwardEdit = createLabeledEntry(grp, grid, 
					 nls->translate("Forward:"),
					 NULL, 4, 0);
  indentPrefixEdit = createLabeledEntry(grp, grid, 
					nls->translate("Indentation:"),
					NULL, 5, 0);
  grid->setColStretch(0,1);
  grid->setColStretch(1,10);
  grid->setColStretch(2,0);
  grid->activate();
  //grp->adjustSize();


  // set the values
  config->setGroup("KMMessage");
  str = config->readEntry("phrase-reply");
  if (str.isEmpty()) str = nls->translate("On %D, you wrote:");
  phraseReplyEdit->setText(str);

  str = config->readEntry("phrase-reply-all");
  if (str.isEmpty()) str = nls->translate("On %D, %F wrote:");
  phraseReplyAllEdit->setText(str);

  str = config->readEntry("phrase-forward");
  if (str.isEmpty()) str = nls->translate("Forwarded Message");
  phraseForwardEdit->setText(str);

  indentPrefixEdit->setText(config->readEntry("indent-prefix", "> "));

  //---------- group appearance
  grp = new QGroupBox(nls->translate("Appearance"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 7, 3, 20, 4);

  autoSign = new QCheckBox(nls->translate("Automatically append signature"),
			   grp);
  autoSign->adjustSize();
  autoSign->setMinimumSize(autoSign->sizeHint());
  grid->addMultiCellWidget(autoSign, 0, 0, 0, 1);

  wordWrap = new QCheckBox(nls->translate("Word wrap at column:"), grp);
  wordWrap->adjustSize();
  wordWrap->setMinimumSize(autoSign->sizeHint());
  grid->addWidget(wordWrap, 1, 0);

  wrapColumnEdit = new QLineEdit(grp);
  wrapColumnEdit->adjustSize();
  wrapColumnEdit->setMinimumSize(50, wrapColumnEdit->sizeHint().height());
  grid->addWidget(wrapColumnEdit, 1, 1);

  monospFont = new QCheckBox(nls->translate("Use monospaced font"), grp);
  monospFont->adjustSize();
  monospFont->setMinimumSize(monospFont->sizeHint());
  grid->addMultiCellWidget(monospFont, 2, 2, 0, 1);

  grid->setColStretch(0,1);
  grid->setColStretch(1,1);
  grid->setColStretch(2,10);
  grid->activate();

  config->setGroup("Composer");
  autoSign->setChecked(stricmp(config->readEntry("signature"),"auto")==0);
  wordWrap->setChecked(config->readNumEntry("word-wrap",1));
  wrapColumnEdit->setText(config->readEntry("break-at","80"));
  monospFont->setChecked(stricmp(config->readEntry("font","variable"),"fixed")==0);

  //---------- ére we gø
  box->addStretch(10);
  box->activate();
 
  addTab(tab, nls->translate("Composer"));
}


//-----------------------------------------------------------------------------
const QString KMSettings::tabNetworkAcctStr(const KMAccount* act) const
{
  QString str;
  
  str = "";
  str += act->name();
  str += "\n";
  str += act->type();
  str += "\n";
  if (act->folder()) str += act->folder()->name();

  return str;
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
  KMAcctSelDlg acctSel(this, nls->translate("Select Account"));
  KMAccount* acct;
  KMAccountSettings* acctSettings;
  const char* acctType = NULL;

  if (!acctSel.exec()) return;

  switch(acctSel.selected())
  {
  case 0:
    acctType = "local";
    break;
  case 1:
    acctType = "pop";
    break;
  default:
    fatal("KMSettings: unsupported account type selected");
  }

  acct = acctMgr->create(acctType, nls->translate("Unnamed"));
  assert(acct != NULL);

  acct->init(); // fill the account fields with good default values

  acctSettings = new KMAccountSettings(this, NULL, acct);

  if (acctSettings->exec())
    accountList->insertItem(tabNetworkAcctStr(acct), -1);
  else
    acctMgr->remove(acct);

  delete acctSettings;
}

//-----------------------------------------------------------------------------
void KMSettings::chooseSendmailLocation()
{
  QFileDialog dlg("/", "*", this, NULL, TRUE);
  dlg.setCaption(nls->translate("Choose Sendmail Location"));

  if (dlg.exec()) sendmailLocationEdit->setText(dlg.selectedFile());
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
  KMAccount* acct;
  KMAccountSettings* d;

  if (index < 0) return;

  acct = acctMgr->find(accountList->text(index,0));
  assert(acct != NULL);

  d = new KMAccountSettings(this, NULL, acct);
  d->exec();
  delete d;

  acct->stateChanged();
  accountList->changeItem(tabNetworkAcctStr(acct), index);
  accountList->setCurrentItem(index);
}

//-----------------------------------------------------------------------------
void KMSettings::modifyAccount2()
{
  modifyAccount(accountList->currentItem(),-1);
}

//-----------------------------------------------------------------------------
void KMSettings::removeAccount()
{
  QString acctName;
  KMAccount* acct;
  int idx = accountList->currentItem();

  acctName = accountList->text(idx, 0);
  acct = acctMgr->find(acctName);
  if (!acct) return;

  acctMgr->remove(acct);
  accountList->removeItem(idx);
  if (!accountList->count())
  {
    modifyButton->setEnabled(FALSE);
    removeButton->setEnabled(FALSE);
  }
  if (idx >= (int)accountList->count()) idx--;
  if (idx >= 0) accountList->setCurrentItem(idx);

  accountList->update();
}

//-----------------------------------------------------------------------------
void KMSettings::setDefaults()
{
  sigEdit->setText(QString(QDir::home().path())+"/.signature");
  sendmailRadio->setChecked(TRUE);
  sendmailLocationEdit->setText("/usr/sbin/sendmail");
  smtpRadio->setChecked(FALSE);
  smtpPortEdit->setText("25");
}

//-----------------------------------------------------------------------------
void KMSettings::doCancel()
{
  identity->readConfig();
  msgSender->readConfig();
  acctMgr->readConfig();
}

//-----------------------------------------------------------------------------
void KMSettings::doApply()
{
  KConfig* config = app->getConfig();

  //----- identity
  identity->setFullName(nameEdit->text());
  identity->setOrganization(orgEdit->text());
  identity->setEmailAddr(emailEdit->text());
  identity->setReplyToAddr(replytoEdit->text());
  identity->setSignatureFile(sigEdit->text());
  identity->writeConfig(FALSE);
  
  //----- sending mail
  if (sendmailRadio->isChecked())
    msgSender->setMethod(KMSender::smMail);
  else
    msgSender->setMethod(KMSender::smSMTP);
  
  msgSender->setMailer(sendmailLocationEdit->text());
  msgSender->setSmtpHost(smtpServerEdit->text());
  msgSender->setSmtpPort(atoi(smtpPortEdit->text()));
  msgSender->writeConfig(FALSE);
  
  //----- incoming mail
  acctMgr->writeConfig(FALSE);

  //----- composer phrases
  config->setGroup("KMMessage");
  config->writeEntry("phrase-reply", phraseReplyEdit->text());
  config->writeEntry("phrase-reply-all", phraseReplyAllEdit->text());
  config->writeEntry("phrase-forward", phraseForwardEdit->text());
  config->writeEntry("indent-prefix", indentPrefixEdit->text());

  //----- composer appearance
  config->setGroup("Composer");
  config->writeEntry("signature", autoSign->isChecked()?"auto":"manual");
  config->writeEntry("word-wrap", wordWrap->isChecked());
  config->writeEntry("break-at", atoi(wrapColumnEdit->text()));
  config->writeEntry("font", monospFont->isChecked()?"fixed":"variable");

  //-----
  app->getConfig()->sync();

  folderMgr->contentsChanged();
  KMMessage::readConfig();
}


//=============================================================================
//=============================================================================
KMAccountSettings::KMAccountSettings(QWidget *parent, const char *name,
				     KMAccount *aAcct):
  QDialog(parent,name,TRUE)
{
  QGridLayout *grid;
  QPushButton *btnDetail, *ok, *cancel;
  QString acctType;
  QWidget *btnBox;
  QLabel *lbl;
  KMFolder *folder, *acctFolder;
  KMFolderDir* fdir = (KMFolderDir*)&folderMgr->dir();
  int i;

  initMetaObject();

  assert(aAcct != NULL);

  mAcct=aAcct;
  setCaption(name);

  acctType = mAcct->type();

  setCaption("Configure Account");
  grid = new QGridLayout(this, 12, 3, 8, 4);
  grid->setColStretch(1, 5);

  lbl = new QLabel(nls->translate("Type:"), this);
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addWidget(lbl, 0, 0);

  lbl = new QLabel(this);
  grid->addWidget(lbl, 0, 1);

  mEdtName = createLabeledEntry(this, grid, nls->translate("Name:"),
				mAcct->name(), 1, 0);

  if (acctType == "local")
  {
    lbl->setText(nls->translate("Local Account"));

    mEdtLocation = createLabeledEntry(this, grid, nls->translate("Location:"),
				      ((KMAcctLocal*)mAcct)->location(),
				      2, 0, &btnDetail);
    connect(btnDetail,SIGNAL(clicked()), SLOT(chooseLocation()));
  }
  else if (acctType == "pop")
  {
    lbl->setText(nls->translate("Pop Account"));

    mEdtLogin = createLabeledEntry(this, grid, nls->translate("Login:"),
				   ((KMAcctPop*)mAcct)->login(), 2, 0);

    mEdtPasswd = createLabeledEntry(this, grid, nls->translate("Password:"),
				    ((KMAcctPop*)mAcct)->passwd(), 3, 0);
    mEdtPasswd->setEchoMode(QLineEdit::Password);

    mEdtHost = createLabeledEntry(this, grid, nls->translate("Host:"),
				  ((KMAcctPop*)mAcct)->host(), 4, 0);

    QString tmpStr(32);
    tmpStr.sprintf("%d",((KMAcctPop*)mAcct)->port());
    mEdtPort = createLabeledEntry(this, grid, nls->translate("Port:"),
				  tmpStr, 5, 0);

    chk = new QCheckBox(nls->translate("Delete mail from server"), this);
    chk->setChecked(!((KMAcctPop*)mAcct)->leaveOnServer());
    grid->addMultiCellWidget(chk, 6, 6, 1, 2);

  }
  else fatal("KMAccountSettings: unsupported account type");

  // label with "Local Account" or "Pop Account" created previously
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());


  lbl = new QLabel(nls->translate("Store new mail in account:"), this);
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addMultiCellWidget(lbl, 7, 7, 0, 2);

  // combobox of all folders with current account folder selected
  acctFolder = mAcct->folder();
  mFolders = new QComboBox(this);
  mFolders->insertItem(nls->translate("<none>"));
  for (i=1, folder=(KMFolder*)fdir->first(); folder; folder=(KMFolder*)fdir->next())
  {
    if (folder->isDir()) continue;
    mFolders->insertItem(folder->name());
    if (folder == acctFolder) mFolders->setCurrentItem(i);
    i++;
  }
  mFolders->adjustSize();
  mFolders->setMinimumSize(100, mEdtName->minimumSize().height());
  mFolders->setMaximumSize(500, mEdtName->minimumSize().height());
  grid->addWidget(mFolders, 8, 1);

  // buttons at bottom
  btnBox = new QWidget(this);
  ok = new QPushButton(nls->translate("Ok"), btnBox);
  ok->adjustSize();
  ok->setMinimumSize(ok->sizeHint());
  ok->resize(100, ok->size().height());
  ok->move(10, 5);
  connect(ok, SIGNAL(clicked()), SLOT(accept()));

  cancel = new QPushButton(nls->translate("Cancel"), btnBox);
  cancel->adjustSize();
  cancel->setMinimumSize(cancel->sizeHint());
  cancel->resize(100, cancel->size().height());
  cancel->move(120, 5);
  connect(cancel, SIGNAL(clicked()), SLOT(reject()));

  btnBox->setMinimumSize(230, ok->size().height()+10);
  btnBox->setMaximumSize(2048, ok->size().height()+10);
  grid->addMultiCellWidget(btnBox, 10, 10, 0, 2);

  resize(350,310);
  grid->activate();
  adjustSize();
  setMinimumSize(size());
}


//-----------------------------------------------------------------------------
void KMAccountSettings::chooseLocation()
{
  static QString sSelLocation("/");
  QFileDialog fdlg(sSelLocation,"*",this,NULL,TRUE);
  fdlg.setCaption(nls->translate("Choose Location"));

  if (fdlg.exec()) mEdtLocation->setText(fdlg.selectedFile());
  sSelLocation = fdlg.selectedFile().copy();
}

//-----------------------------------------------------------------------------
void KMAccountSettings::accept()
{
  QString acctType = mAcct->type();
  KMFolder* fld;
  int id;

  if (mEdtName->text() != mAcct->name())
  {
    mAcct->setName(mEdtName->text());
  }

  id = mFolders->currentItem();
  if (id > 0) fld = folderMgr->find(mFolders->currentText());
  else fld = NULL;
  mAcct->setFolder((KMFolder*)fld);

  if (acctType == "local")
  {
    ((KMAcctLocal*)mAcct)->setLocation(mEdtLocation->text());

    // Wainting for GUI
    //((KMAcctLocal*)mAcct)->setTimerRequested(false);
  }

  else if (acctType == "pop")
  {
    ((KMAcctPop*)mAcct)->setHost(mEdtHost->text());
    ((KMAcctPop*)mAcct)->setPort(atoi(mEdtPort->text()));
    ((KMAcctPop*)mAcct)->setLogin(mEdtLogin->text());
    ((KMAcctPop*)mAcct)->setPasswd(mEdtPasswd->text(), true);
    ((KMAcctPop*)mAcct)->setLeaveOnServer(!chk->isChecked());

    // Waiting for GUI
    //((KMAcctPop*)mAcct)->setTimerRequested(false);
  }

  acctMgr->writeConfig(TRUE);

  QDialog::accept();
}

