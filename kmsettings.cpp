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
#include "kpgp.h"
#include "kfontdialog.h"
#include "kfontutils.h"
#include "kmtopwidget.h"

#include <kapp.h>
#include <kapp.h>
#include <kmsgbox.h>
#include <kfiledialog.h>
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

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL  "/usr/sbin/sendmail"
#endif

//------
#include "kmsettings.moc"


//-----------------------------------------------------------------------------
KMSettings::KMSettings(QWidget *parent, const char *name) :
  QTabDialog(parent, name, TRUE)
{
  initMetaObject();

  setCaption(i18n("Settings"));
  resize(500,600);
  setOKButton(i18n("OK"));
  setCancelButton(i18n("Cancel"));

  connect(this, SIGNAL(applyButtonPressed()), this, SLOT(doApply()));
  connect(this, SIGNAL(cancelButtonPressed()), this, SLOT(doCancel()));

  createTabIdentity(this);
  createTabNetwork(this);
  createTabAppearance(this);
  createTabComposer(this);
  createTabMisc(this);
  createTabPgp(this);
}


//-----------------------------------------------------------------------------
KMSettings::~KMSettings()
{
  debug("~KMSettings");
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
    sel->setFocusPolicy(QWidget::NoFocus);
    sel->setFixedSize(sel->sizeHint().width(), label->height());
    grid->addWidget(sel, gridy, gridx++);
    *detail_return = sel;
  }

  return edit;
}


//-----------------------------------------------------------------------------
// Add a widget with a label and optional detail button ("...")
// The detail button is not created if detail_return is NULL.
// The argument 'label' is the label that will be left of the entry field.
// The whole thing is placed in the grid from row/col to the right.
static void addLabeledWidget(QWidget* parent, QGridLayout* grid,
			     const char* aLabel, QWidget* widg,
			     int gridy, int gridx,
			     QPushButton** detail_return=NULL)
{
  QLabel* label = new QLabel(parent);
  QPushButton* sel;

  label->setText(aLabel);
  label->adjustSize();
  label->resize((int)label->sizeHint().width(),label->sizeHint().height() + 6);
  label->setMinimumSize(label->size());
  grid->addWidget(label, gridy, gridx++);

  widg->setMinimumSize(100, label->height()+2);
  //widg->setMaximumSize(1000, label->height()+2);
  grid->addWidget(widg, gridy, gridx++);

  if (detail_return)
  {
    sel = new QPushButton("...", parent);
    sel->setFocusPolicy(QWidget::NoFocus);
    sel->setFixedSize(sel->sizeHint().width(), label->height());
    grid->addWidget(sel, gridy, gridx++);
    *detail_return = sel;
  }
}


//-----------------------------------------------------------------------------
QPushButton* KMSettings::createPushButton(QWidget* parent, QGridLayout* grid,
					  const char* label, 
					  int gridy, int gridx)
{
  QPushButton* button = new QPushButton(parent, label);
  button->setText(i18n(label));
  button->adjustSize();
  button->setMinimumSize(button->size());
  grid->addWidget(button, gridy, gridx);

  return button;
}


//-----------------------------------------------------------------------------
void KMSettings::createTabIdentity(QWidget* parent)
{
  QWidget* tab = new QWidget(parent);
  QGridLayout* grid = new QGridLayout(tab, 6, 3, 20, 6);
  QPushButton* button;

  nameEdit = createLabeledEntry(tab, grid, i18n("Name:"), 
				identity->fullName(), 0, 0);
  orgEdit = createLabeledEntry(tab, grid, i18n("Organization:"), 
			       identity->organization(), 1, 0);
  emailEdit = createLabeledEntry(tab, grid, i18n("Email Address:"),
				 identity->emailAddr(), 2, 0);
  replytoEdit = createLabeledEntry(tab, grid, 
				   i18n("Reply-To Address:"),
				   identity->replyToAddr(), 3, 0);
  sigEdit = createLabeledEntry(tab, grid, i18n("Signature File:"),
			       identity->signatureFile(), 4, 0, &button);
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSigFile()));

  grid->setColStretch(0,0);
  grid->setColStretch(1,1);
  grid->setColStretch(2,0);
  grid->setRowStretch(5, 100);

  addTab(tab, i18n("Identity"));
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

  //---- group: sending mail
  bgrp = new QButtonGroup(i18n("Sending Mail"), tab);
  box->addWidget(bgrp);
  grid = new QGridLayout(bgrp, 6, 4, 20, 4);

  sendmailRadio = new QRadioButton(bgrp);
  sendmailRadio->setMinimumSize(sendmailRadio->size());
  sendmailRadio->setText(i18n("Sendmail"));
  bgrp->insert(sendmailRadio);
  grid->addMultiCellWidget(sendmailRadio, 0, 0, 0, 3);

  sendmailLocationEdit = createLabeledEntry(bgrp, grid, 
					    i18n("Location:"),
					    NULL, 1, 1, &button);
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSendmailLocation()));

  smtpRadio = new QRadioButton(bgrp);
  smtpRadio->setText(i18n("SMTP"));
  smtpRadio->setMinimumSize(smtpRadio->size());
  bgrp->insert(smtpRadio);
  grid->addMultiCellWidget(smtpRadio, 2, 2, 0, 3);

  smtpServerEdit = createLabeledEntry(bgrp, grid, i18n("Server:"),
				      NULL, 3, 1);
  smtpPortEdit = createLabeledEntry(bgrp, grid, i18n("Port:"),
				    NULL, 4, 1);
  grid->setColStretch(0,4);
  grid->setColStretch(1,1);
  grid->setColStretch(2,10);
  grid->setColStretch(3,0);
  grid->setRowStretch(5, 100);
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
  grp = new QGroupBox(i18n("Incoming Mail"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 5, 2, 20, 8);

  label = new QLabel(grp);
  label->setText(i18n("Accounts:   (add at least one account!)"));
  label->setMinimumSize(label->size());
  grid->addMultiCellWidget(label, 0, 0, 0, 1);

  accountList = new KTabListBox(grp, "LstAccounts", 3);
  accountList->setColumn(0, i18n("Name"), 150);
  accountList->setColumn(1, i18n("Type"), 60);
  accountList->setColumn(2, i18n("Folder"), 80);
  accountList->setMinimumSize(50, 50);
  connect(accountList,SIGNAL(highlighted(int,int)),
	  this,SLOT(accountSelected(int,int)));
  connect(accountList,SIGNAL(selected(int,int)),
	  this,SLOT(modifyAccount(int,int)));
  grid->addMultiCellWidget(accountList, 1, 4, 0, 0);

  addButton = createPushButton(grp, grid, i18n("Add..."), 1, 1);
  connect(addButton,SIGNAL(clicked()),this,SLOT(addAccount()));

  modifyButton = createPushButton(grp, grid, i18n("Modify..."),2,1);
  connect(modifyButton,SIGNAL(clicked()),this,SLOT(modifyAccount2()));
  modifyButton->setEnabled(FALSE);

  removeButton = createPushButton(grp, grid, i18n("Delete"), 3, 1);
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

  addTab(tab, i18n("Network"));
  box->addStretch(100);
  box->activate();
  tab->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::createTabAppearance(QWidget* parent)
{
  QWidget* tab = new QWidget(parent);
  QBoxLayout* box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 4);
  QGridLayout* grid;
  QGroupBox* grp;
  KConfig* config = app->getConfig();
  QPushButton* btn;
  QLabel* lbl;
  QFont fnt;

  //----- group: fonts
  grp = new QGroupBox(i18n("Fonts"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 5, 4, 20, 4);

  bodyFontLabel = new QLabel(grp);
  addLabeledWidget(grp, grid, i18n("Message Body Font:"), bodyFontLabel, 
		   2, 0, &btn);
  connect(btn,SIGNAL(clicked()),this,SLOT(slotBodyFontSelect()));

  listFontLabel = new QLabel(grp);
  addLabeledWidget(grp, grid, i18n("Message List Font:"), listFontLabel,
		   3, 0, &btn);
  connect(btn,SIGNAL(clicked()),this,SLOT(slotListFontSelect()));

  grid->setColStretch(0,1);
  grid->setColStretch(1,10);
  grid->setColStretch(2,0);
  grid->activate();

  // set values
  config->setGroup("Fonts");

  fnt = kstrToFont(config->readEntry("body-font", "helvetica-medium-r-12"));
  bodyFontLabel->setAutoResize(TRUE);
  bodyFontLabel->setText(kfontToStr(fnt));
  bodyFontLabel->setFont(fnt);

  fnt = kstrToFont(config->readEntry("list-font", "helvetica-medium-r-12"));
  listFontLabel->setAutoResize(TRUE);
  listFontLabel->setText(kfontToStr(fnt));
  listFontLabel->setFont(fnt);

  //----- group: layout
  grp = new QGroupBox(i18n("Layout"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 2, 2, 20, 4);

  longFolderList = new QCheckBox(
	      i18n("Long folder list"), grp);
  longFolderList->adjustSize();
  longFolderList->setMinimumSize(longFolderList->sizeHint());
  grid->addMultiCellWidget(longFolderList, 0, 0, 0, 1);
  grid->activate();

  // set values
  config->setGroup("Geometry");
  longFolderList->setChecked(config->readBoolEntry("longFolderList", false));

  //----- activation
  box->addStretch(100);
  addTab(tab, i18n("Appearance"));
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
  int i;

  //---------- group: phrases
  grp = new QGroupBox(i18n("Phrases"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 7, 3, 20, 4);

  lbl = new QLabel(i18n(
        "The following placeholders are supported in the reply phrases:\n"
	"%D=date, %S=subject, %F=sender, %%=percent sign"), grp);
  lbl->adjustSize();
  lbl->setMinimumSize(100,lbl->size().height());
  grid->setRowStretch(0,10);
  grid->addMultiCellWidget(lbl, 0, 0, 0, 2);

  phraseReplyEdit = createLabeledEntry(grp, grid, 
				       i18n("Reply to sender:"),
				       NULL, 2, 0);
  phraseReplyAllEdit = createLabeledEntry(grp, grid, 
					  i18n("Reply to all:"),
					  NULL, 3, 0);
  phraseForwardEdit = createLabeledEntry(grp, grid, 
					 i18n("Forward:"),
					 NULL, 4, 0);
  indentPrefixEdit = createLabeledEntry(grp, grid, 
					i18n("Indentation:"),
					NULL, 5, 0);
  grid->setColStretch(0,1);
  grid->setColStretch(1,10);
  grid->setColStretch(2,0);
  grid->activate();
  //grp->adjustSize();


  // set the values
  config->setGroup("KMMessage");
  str = config->readEntry("phrase-reply");
  if (str.isEmpty()) str = i18n("On %D, you wrote:");
  phraseReplyEdit->setText(str);

  str = config->readEntry("phrase-reply-all");
  if (str.isEmpty()) str = i18n("On %D, %F wrote:");
  phraseReplyAllEdit->setText(str);

  str = config->readEntry("phrase-forward");
  if (str.isEmpty()) str = i18n("Forwarded Message");
  phraseForwardEdit->setText(str);

  indentPrefixEdit->setText(config->readEntry("indent-prefix", "> "));

  //---------- group appearance
  grp = new QGroupBox(i18n("Appearance"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 8, 3, 20, 4);

  autoAppSignFile=new QCheckBox(
	      i18n("Automatically append signature"), grp);
  autoAppSignFile->adjustSize();
  autoAppSignFile->setMinimumSize(autoAppSignFile->sizeHint());
  grid->addMultiCellWidget(autoAppSignFile, 0, 0, 0, 1);

  pgpAutoSign=new QCheckBox(
	      i18n("Automatically sign messages using PGP"), grp);
  pgpAutoSign->adjustSize();
  pgpAutoSign->setMinimumSize(pgpAutoSign->sizeHint());
  grid->addMultiCellWidget(pgpAutoSign, 1, 1, 0, 1);

  wordWrap = new QCheckBox(i18n("Word wrap at column:"), grp);
  wordWrap->adjustSize();
  wordWrap->setMinimumSize(autoAppSignFile->sizeHint());
  grid->addWidget(wordWrap, 2, 0);

  wrapColumnEdit = new QLineEdit(grp);
  wrapColumnEdit->adjustSize();
  wrapColumnEdit->setMinimumSize(50, wrapColumnEdit->sizeHint().height());
  grid->addWidget(wrapColumnEdit, 2, 1);

  monospFont = new QCheckBox(i18n("Use monospaced font") +
			     QString(" (still broken)"), grp);
  monospFont->adjustSize();
  monospFont->setMinimumSize(monospFont->sizeHint());
  grid->addMultiCellWidget(monospFont, 3, 3, 0, 1);

  grid->setColStretch(0,1);
  grid->setColStretch(1,1);
  grid->setColStretch(2,10);
  grid->activate();

  //---------- group sending
  grp = new QGroupBox(i18n("When sending mail"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 4, 3, 20, 4);
  lbl = new QLabel(i18n("Default sending:"), grp);
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addWidget(lbl, 0, 0);
  sendNow = new QRadioButton(i18n("send now"), grp);
  sendNow->setMinimumSize(sendNow->sizeHint());
  connect(sendNow,SIGNAL(clicked()),SLOT(slotSendNow()));
  grid->addWidget(sendNow, 0, 1);
  sendLater = new QRadioButton(i18n("send later"), grp);
  sendLater->setMinimumSize(sendLater->sizeHint());
  connect(sendLater,SIGNAL(clicked()),SLOT(slotSendLater()));
  grid->addWidget(sendLater, 0, 2);

  lbl = new QLabel(i18n("Send messages:"), grp);
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addWidget(lbl, 1, 0);
  allow8Bit = new QRadioButton(i18n("Allow 8-bit"), grp);
  allow8Bit->setMinimumSize(allow8Bit->sizeHint());
  connect(allow8Bit,SIGNAL(clicked()),SLOT(slotAllow8Bit()));
  grid->addWidget(allow8Bit, 1, 1);
  quotedPrintable = new QRadioButton(i18n(
			  "MIME Compilant (Quoted Printable)"), grp);
  quotedPrintable->setMinimumSize(quotedPrintable->sizeHint());
  connect(quotedPrintable,SIGNAL(clicked()),SLOT(slotQuotedPrintable()));
  grid->addWidget(quotedPrintable, 1, 2);
  grid->activate();

  // set values
  config->setGroup("Composer");
  autoAppSignFile->setChecked(stricmp(config->readEntry("signature"),"auto")==0);
  wordWrap->setChecked(config->readNumEntry("word-wrap",1));
  wrapColumnEdit->setText(config->readEntry("break-at","80"));
  monospFont->setChecked(stricmp(config->readEntry("font","variable"),"fixed")==0);
  pgpAutoSign->setChecked(config->readNumEntry("pgp-auto-sign",0));

  i = msgSender->sendImmediate();
  sendNow->setChecked(i);
  sendLater->setChecked(!i);

  i = msgSender->sendQuotedPrintable();
  allow8Bit->setChecked(!i);
  quotedPrintable->setChecked(i);

  //---------- ére we gø
  box->addStretch(10);
  box->activate();
 
  addTab(tab, i18n("Composer"));
}



//-----------------------------------------------------------------------------
void KMSettings::createTabMisc(QWidget *parent)
{
  QWidget *tab = new QWidget(parent);
  QBoxLayout* box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 4);
  QGridLayout* grid;
  QGroupBox* grp;

  KConfig* config = app->getConfig();
  QString str;

  //---------- group: folders
  grp = new QGroupBox(i18n("Folders"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 2, 3, 20, 4);

  emptyTrashOnExit=new QCheckBox(i18n("empty trash on exit"),grp);
  emptyTrashOnExit->setMinimumSize(emptyTrashOnExit->sizeHint());
  grid->addMultiCellWidget(emptyTrashOnExit, 0, 0, 0, 2);

  sendOnCheck = new QCheckBox(i18n("Send Mail in outbox Folder on Check"),grp);
  sendOnCheck->setMinimumSize(sendOnCheck->sizeHint());
  grid->addMultiCellWidget(sendOnCheck,1,1,0,2);

  grid->activate();

  //---------- set values
  config->setGroup("General");
  emptyTrashOnExit->setChecked(config->readNumEntry("empty-trash-on-exit",0));
  sendOnCheck->setChecked(config->readBoolEntry("sendOnCheck",false));

  //---------- here we go
  box->addStretch(10);
  box->activate();
 
  addTab(tab, i18n("Misc"));
}

// ----------------------------------------------------------------------------
void KMSettings::createTabPgp(QWidget *parent)
{
  pgpConfig = new KpgpConfig(parent);
  addTab(pgpConfig, i18n("PGP"));
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
void KMSettings::slotBodyFontSelect()
{
  QFont font;
  KFontDialog dlg(this, i18n("Message Body Font"), TRUE);

  font = kstrToFont(bodyFontLabel->text());
  dlg.getFont(font);
  bodyFontLabel->setFont(font);
  bodyFontLabel->setText(kfontToStr(font));
  bodyFontLabel->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::slotListFontSelect()
{
  QFont font;
  KFontDialog dlg(this, i18n("Message List Font"), TRUE);

  font = kstrToFont(listFontLabel->text());
  dlg.getFont(font);
  listFontLabel->setFont(font);
  listFontLabel->setText(kfontToStr(font));
  listFontLabel->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::slotAllow8Bit()
{
  allow8Bit->setChecked(TRUE);
  quotedPrintable->setChecked(FALSE);
}


//-----------------------------------------------------------------------------
void KMSettings::slotQuotedPrintable()
{
  allow8Bit->setChecked(FALSE);
  quotedPrintable->setChecked(TRUE);
}


//-----------------------------------------------------------------------------
void KMSettings::slotSendNow()
{
  sendNow->setChecked(TRUE);
  sendLater->setChecked(FALSE);
}


//-----------------------------------------------------------------------------
void KMSettings::slotSendLater()
{
  sendNow->setChecked(FALSE);
  sendLater->setChecked(TRUE);
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
  KMAcctSelDlg acctSel(this, i18n("Select Account"));
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

  acct = acctMgr->create(acctType, i18n("Unnamed"));
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
  KFileDialog dlg("/", "*", this, NULL, TRUE);
  dlg.setCaption(i18n("Choose Sendmail Location"));

  if (dlg.exec()) sendmailLocationEdit->setText(dlg.selectedFile());
}

//-----------------------------------------------------------------------------
void KMSettings::chooseSigFile()
{
  KFileDialog *d=new KFileDialog(QDir::homeDirPath(),"*",this,NULL,TRUE);
  d->setCaption(i18n("Choose Signature File"));
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

  if(!acctMgr->remove(acct))
    return;
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
  sendmailLocationEdit->setText(_PATH_SENDMAIL);
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
  msgSender->setSendImmediate(sendNow->isChecked());
  msgSender->setSendQuotedPrintable(quotedPrintable->isChecked());
  msgSender->writeConfig(FALSE);
  
  //----- incoming mail
  acctMgr->writeConfig(FALSE);

  //----- fonts
  config->setGroup("Fonts");
  config->writeEntry("body-font", bodyFontLabel->text());
  config->writeEntry("list-font", listFontLabel->text());

  //----- layout
  config->setGroup("Geometry");
  config->writeEntry("longFolderList", longFolderList->isChecked());

  //----- composer phrases
  config->setGroup("KMMessage");
  config->writeEntry("phrase-reply", phraseReplyEdit->text());
  config->writeEntry("phrase-reply-all", phraseReplyAllEdit->text());
  config->writeEntry("phrase-forward", phraseForwardEdit->text());
  config->writeEntry("indent-prefix", indentPrefixEdit->text());

  //----- composer appearance
  config->setGroup("Composer");
  config->writeEntry("signature",autoAppSignFile->isChecked()?"auto":"manual");
  config->writeEntry("word-wrap",wordWrap->isChecked());
  config->writeEntry("break-at", atoi(wrapColumnEdit->text()));
  config->writeEntry("font", monospFont->isChecked()?"fixed":"variable");
  config->writeEntry("pgp-auto-sign", pgpAutoSign->isChecked());

  //----- misc
  config->setGroup("General");
  config->writeEntry("empty-trash-on-exit", emptyTrashOnExit->isChecked());
  config->writeEntry("first-start", FALSE);
  config->writeEntry("sendOnCheck",sendOnCheck->isChecked());

  //-----
  config->sync();
  pgpConfig->applySettings();

  folderMgr->contentsChanged();
  KMMessage::readConfig();

  KMTopLevelWidget::forEvery(&KMTopLevelWidget::readConfig);
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
  grid = new QGridLayout(this, 16, 3, 8, 4);
  grid->setColStretch(1, 5);

  lbl = new QLabel(i18n("Type:"), this);
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addWidget(lbl, 0, 0);

  lbl = new QLabel(this);
  grid->addWidget(lbl, 0, 1);

  mEdtName = createLabeledEntry(this, grid, i18n("Name:"),
				mAcct->name(), 1, 0);

  if (acctType == "local")
  {
    lbl->setText(i18n("Local Account"));

    mEdtLocation = createLabeledEntry(this, grid, i18n("Location:"),
				      ((KMAcctLocal*)mAcct)->location(),
				      2, 0, &btnDetail);
    connect(btnDetail,SIGNAL(clicked()), SLOT(chooseLocation()));
  }
  else if (acctType == "pop")
  {
    lbl->setText(i18n("Pop Account"));

    mEdtLogin = createLabeledEntry(this, grid, i18n("Login:"),
				   ((KMAcctPop*)mAcct)->login(), 2, 0);

    mEdtPasswd = createLabeledEntry(this, grid, i18n("Password:"),
				    ((KMAcctPop*)mAcct)->passwd(), 3, 0);
    mEdtPasswd->setEchoMode(QLineEdit::Password);

    mEdtHost = createLabeledEntry(this, grid, i18n("Host:"),
				  ((KMAcctPop*)mAcct)->host(), 4, 0);

    QString tmpStr(32);
    tmpStr.sprintf("%d",((KMAcctPop*)mAcct)->port());
    mEdtPort = createLabeledEntry(this, grid, i18n("Port:"),
				  tmpStr, 5, 0);

    mChkDelete = new QCheckBox(i18n("Delete mail from server"), this);
    mChkDelete->setMinimumSize(mChkDelete->sizeHint());
    mChkDelete->setChecked(!((KMAcctPop*)mAcct)->leaveOnServer());
    grid->addMultiCellWidget(mChkDelete, 6, 6, 1, 2);

    mChkRetrieveAll=new QCheckBox(i18n("Retrieve all mail from server"), this);
    mChkRetrieveAll->setMinimumSize(mChkRetrieveAll->sizeHint());
    mChkRetrieveAll->setChecked(((KMAcctPop*)mAcct)->retrieveAll());
    grid->addMultiCellWidget(mChkRetrieveAll, 7, 7, 1, 2);

  }
  else 
  {
    warning("KMAccountSettings: unsupported account type");
    return;
  }

  mChkInterval = new QCheckBox(i18n("Enable interval Mail checking"), this);
  mChkInterval->setMinimumSize(mChkInterval->sizeHint());
  mChkInterval->setChecked(mAcct->checkInterval() > 0);
  grid->addMultiCellWidget(mChkInterval, 8, 8, 1, 2);

  // label with "Local Account" or "Pop Account" created previously
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());


  lbl = new QLabel(i18n("Store new mail in account:"), this);
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addMultiCellWidget(lbl, 10, 10, 0, 2);

  // combobox of all folders with current account folder selected
  acctFolder = mAcct->folder();
  mFolders = new QComboBox(this);
  mFolders->insertItem(i18n("<none>"));
  for (i=1, folder=(KMFolder*)fdir->first(); folder; 
       folder=(KMFolder*)fdir->next())
  {
    if (folder->isDir()) continue;
    mFolders->insertItem(folder->name());
    if (folder == acctFolder) mFolders->setCurrentItem(i);
    i++;
  }
  mFolders->adjustSize();
  mFolders->setMinimumSize(100, mEdtName->minimumSize().height());
  mFolders->setMaximumSize(500, mEdtName->minimumSize().height());
  grid->addWidget(mFolders, 11, 1);


  // buttons at bottom
  btnBox = new QWidget(this);
  ok = new QPushButton(i18n("OK"), btnBox);
  ok->adjustSize();
  ok->setMinimumSize(ok->sizeHint());
  ok->resize(100, ok->size().height());
  ok->move(10, 5);
  connect(ok, SIGNAL(clicked()), SLOT(accept()));

  cancel = new QPushButton(i18n("Cancel"), btnBox);
  cancel->adjustSize();
  cancel->setMinimumSize(cancel->sizeHint());
  cancel->resize(100, cancel->size().height());
  cancel->move(120, 5);
  connect(cancel, SIGNAL(clicked()), SLOT(reject()));

  btnBox->setMinimumSize(230, ok->size().height()+10);
  btnBox->setMaximumSize(2048, ok->size().height()+10);
  grid->addMultiCellWidget(btnBox, 14, 14, 0, 2);

  resize(350,350);
  grid->activate();
  adjustSize();
  setMinimumSize(size());
}


//-----------------------------------------------------------------------------
void KMAccountSettings::chooseLocation()
{
  static QString sSelLocation("/");
  KFileDialog fdlg(sSelLocation,"*",this,NULL,TRUE);
  fdlg.setCaption(i18n("Choose Location"));

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

  mAcct->setCheckInterval(mChkInterval->isChecked() ? 3 : 0);

  if (acctType == "local")
  {
    ((KMAcctLocal*)mAcct)->setLocation(mEdtLocation->text());
  }

  else if (acctType == "pop")
  {
    ((KMAcctPop*)mAcct)->setHost(mEdtHost->text());
    ((KMAcctPop*)mAcct)->setPort(atoi(mEdtPort->text()));
    ((KMAcctPop*)mAcct)->setLogin(mEdtLogin->text());
    ((KMAcctPop*)mAcct)->setPasswd(mEdtPasswd->text(), true);
    ((KMAcctPop*)mAcct)->setLeaveOnServer(!mChkDelete->isChecked());
    ((KMAcctPop*)mAcct)->setRetrieveAll(mChkRetrieveAll->isChecked());
  }

  acctMgr->writeConfig(TRUE);

  QDialog::accept();
}

