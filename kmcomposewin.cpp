// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#include <qdir.h>
#include <qprinter.h>
#include <qcombobox.h>

#include "kmcomposewin.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmsender.h"
#include "kmidentity.h"
#include "kfileio.h"
#include "kbusyptr.h"
#include "kmmsgpartdlg.h"
#include "kpgp.h"
#include "kmaddrbookdlg.h"
#include "kmaddrbook.h"
#include "kfontutils.h"

#ifndef KRN
#include "kmmainwin.h"
#include "kmsettings.h"
#include "kfileio.h"
#endif

#include <assert.h>
#include <kapp.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <ktablistbox.h>
#include <ktoolbar.h>
#include <kstdaccel.h>
#include <mimelib/mimepp.h>
#include <kfiledialog.h>
#include <kwm.h>
#include <kglobal.h>
#include <kmessagebox.h>

#include <kspell.h>

#include <qtabdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlist.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qregexp.h>
#include <qcursor.h>
#include <qmessagebox.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <klocale.h>
#include <ktempfile.h>

#if defined CHARSETS
#include <kcharsets.h>
#include "charsetsDlg.h"
#endif

#ifdef KRN
/* start added for KRN */
#include "krnsender.h"
extern KApplication *app;
extern KBusyPtr *kbp;
extern KRNSender *msgSender;
extern KMIdentity *identity;
extern KMAddrBook *addrBook;
typedef QList<QWidget> WindowList;
WindowList* windowList=new WindowList;
#define aboutText "KRN"
/* end added for KRN */
#else
#include "kmglobal.h"
#include "kmmainwin.h"
#endif

#include "kmcomposewin.moc"


#define HDR_FROM     0x01
#define HDR_REPLY_TO 0x02
#define HDR_TO       0x04
#define HDR_CC       0x08
#define HDR_BCC      0x10
#define HDR_SUBJECT  0x20
#define HDR_NEWSGROUPS  0x40
#define HDR_FOLLOWUP_TO 0x80
#define HDR_ALL      0xff

#ifdef KRN
#define HDR_STANDARD (HDR_SUBJECT|HDR_NEWSGROUPS)
#else
#define HDR_STANDARD (HDR_SUBJECT|HDR_TO|HDR_CC)
#endif

QString KMComposeWin::mPathAttach = QString::null;

//-----------------------------------------------------------------------------
KMComposeWin::KMComposeWin(KMMessage *aMsg) : KMComposeWinInherited(),
  mMainWidget(this),
  mEdtFrom(this,&mMainWidget), mEdtReplyTo(this,&mMainWidget),
  mEdtTo(this,&mMainWidget),  mEdtCc(this,&mMainWidget),
  mEdtBcc(this,&mMainWidget), mEdtSubject(this,&mMainWidget, "subjectLine"),
  mLblFrom(&mMainWidget), mLblReplyTo(&mMainWidget), mLblTo(&mMainWidget),
  mLblCc(&mMainWidget), mLblBcc(&mMainWidget), mLblSubject(&mMainWidget),
  mBtnTo("...",&mMainWidget), mBtnCc("...",&mMainWidget),
  mBtnBcc("...",&mMainWidget),  mBtnFrom("...",&mMainWidget),
  mBtnReplyTo("...",&mMainWidget)

#ifdef KRN
  ,mEdtNewsgroups(this,&mMainWidget),mEdtFollowupTo(this,&mMainWidget),
  mLblNewsgroups(&mMainWidget),mLblFollowupTo(&mMainWidget)
#endif
{
    setWFlags( WType_TopLevel | WStyle_Dialog );

  mGrid = NULL;
  mAtmListBox = NULL;
  mAtmList.setAutoDelete(TRUE);
  mAutoDeleteMsg = FALSE;
  mPathAttach = QString::null;
  mEditor = NULL;

  mSpellCheckInProgress=FALSE;

  setCaption(i18n("KMail Composer"));
  setMinimumSize(200,200);

  mBtnTo.setFocusPolicy(QWidget::NoFocus);
  mBtnCc.setFocusPolicy(QWidget::NoFocus);
  mBtnBcc.setFocusPolicy(QWidget::NoFocus);
  mBtnFrom.setFocusPolicy(QWidget::NoFocus);
  mBtnReplyTo.setFocusPolicy(QWidget::NoFocus);

  mAtmListBox = new KTabListBox(&mMainWidget, NULL, 5);
  mAtmListBox->setFocusPolicy(QWidget::NoFocus);
  mAtmListBox->setColumn(0, i18n("F"),16, KTabListBox::PixmapColumn);
  mAtmListBox->setColumn(1, i18n("Name"), 200);
  mAtmListBox->setColumn(2, i18n("Size"), 80);
  mAtmListBox->setColumn(3, i18n("Encoding"), 120);
  mAtmListBox->setColumn(4, i18n("Type"), 150);
  connect(mAtmListBox,SIGNAL(popupMenu(int,int)),
	  SLOT(slotAttachPopupMenu(int,int)));

  readConfig();

  setupStatusBar();
  setupEditor();
  setupMenuBar();
  setupToolBar();

  if(!mShowToolBar) enableToolBar(KToolBar::Hide);	

  connect(&mEdtSubject,SIGNAL(textChanged(const QString&)),
	  SLOT(slotUpdWinTitle(const QString&)));
  connect(&mBtnTo,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
  connect(&mBtnCc,SIGNAL(clicked()),SLOT(slotAddrBookCc()));
  connect(&mBtnBcc,SIGNAL(clicked()),SLOT(slotAddrBookBcc()));
  connect(&mBtnReplyTo,SIGNAL(clicked()),SLOT(slotAddrBookReplyTo()));
  connect(&mBtnFrom,SIGNAL(clicked()),SLOT(slotAddrBookFrom()));

  mMainWidget.resize(480,510);
  setView(&mMainWidget, FALSE);
  rethinkFields();

#ifndef KRN
  if (useExtEditor) {
    mEditor->setExternalEditor(true);
    mEditor->setExternalEditorPath(mExtEditor);
  }
#endif

#if defined CHARSETS
  // As family may change with charset, we must save original settings
  mSavedEditorFont=mEditor->font();
#endif

  mMsg = NULL;
  if (aMsg)
    setMsg(aMsg);

  mEdtTo.setFocus();
}


//-----------------------------------------------------------------------------
KMComposeWin::~KMComposeWin()
{
  writeConfig();

  if (mAutoDeleteMsg && mMsg) delete mMsg;
#if 0
  delete mDropZone;
#endif
  delete mMenuBar;
  delete mToolBar;
}


//-----------------------------------------------------------------------------
void KMComposeWin::readConfig(void)
{
  KConfig *config = kapp->config();
  QString str;
  int w, h;

  config->setGroup("Composer");
  mAutoSign = config->readEntry("signature","manual") == "auto";
  mShowToolBar = config->readNumEntry("show-toolbar", 1);
  mDefEncoding = config->readEntry("encoding", "base64");
  mShowHeaders = config->readNumEntry("headers", HDR_STANDARD);
  mWordWrap = config->readNumEntry("word-wrap", 1);
  mLineBreak = config->readNumEntry("break-at", 78);
  if ((mLineBreak == 0) || (mLineBreak > 78))
    mLineBreak = 78;
  if (mLineBreak < 60)
    mLineBreak = 60;
  mAutoPgpSign = config->readNumEntry("pgp-auto-sign", 0);
#ifndef KRN
  mConfirmSend = config->readBoolEntry("confirm-before-send", false);
#endif

  config->setGroup("Reader");
  QColor c1=QColor(app->palette().normal().text());
  QColor c4=QColor(app->palette().normal().base());

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    foreColor = config->readColorEntry("ForegroundColor",&c1);
    backColor = config->readColorEntry("BackgroundColor",&c4);
  }
  else {
    foreColor = c1;
    backColor = c4;
  }

  // Color setup
  mPalette = palette().copy();
  QColorGroup cgrp  = mPalette.normal();
  QColorGroup ncgrp(foreColor,cgrp.background(),
		    cgrp.light(),cgrp.dark(), cgrp.mid(), foreColor,
		    backColor);
  mPalette.setNormal(ncgrp);
  mPalette.setDisabled(ncgrp);
  mPalette.setActive(ncgrp);

  mEdtFrom.setPalette(mPalette);
  mEdtReplyTo.setPalette(mPalette);
  mEdtTo.setPalette(mPalette);
  mEdtCc.setPalette(mPalette);
  mEdtBcc.setPalette(mPalette);
  mEdtSubject.setPalette(mPalette);

#ifndef KRN
  config->setGroup("General");
  mExtEditor = config->readEntry("external-editor", DEFAULT_EDITOR_STR);
  useExtEditor = config->readBoolEntry("use-external-editor", FALSE);

  int headerCount = config->readNumEntry("mime-header-count", 0);
  mCustHeaders.clear();
  mCustHeaders.setAutoDelete(true);
  for (int i = 0; i < headerCount; i++) {
    QString thisGroup;
    _StringPair *thisItem = new _StringPair;
    thisGroup.sprintf("Mime #%d", i);
    config->setGroup(thisGroup);
    thisItem->name = config->readEntry("name", "");
    if ((thisItem->name).length() > 0) {
      thisItem->value = config->readEntry("value", "");
      mCustHeaders.append(thisItem);
    } else {
      delete thisItem;
    }
  }
#endif

  config->setGroup("Fonts");
  if (!config->readBoolEntry("defaultFonts",TRUE)) {
    QString mBodyFontStr;
    mBodyFontStr = config->readEntry("body-font", "helvetica-medium-r-12");
    mBodyFont = kstrToFont(mBodyFontStr);
  }
  else
    mBodyFont = KGlobal::generalFont();
  if (mEditor) mEditor->setFont(mBodyFont);

#if defined CHARSETS
  m7BitAscii = config->readNumEntry("7bit-is-ascii",1);
  mQuoteUnknownCharacters = config->readNumEntry("quote-unknown",0);

  str = config->readEntry("default-charset", "");
  if (str.isNull() || str=="default" || !KGlobal::charsets()->isAvailable(str))
      mDefaultCharset="default";
  else
      mDefaultCharset=str;

  debug("Default charset: %s", (const char*)mDefaultCharset);

  str = config->readEntry("composer-charset", "");
  if (str.isNull() || str=="default" || !KGlobal::charsets()->isAvailable(str))
      mDefComposeCharset="default";
  else
      mDefComposeCharset=str;

  debug("Default composer charset: %s", (const char*)mDefComposeCharset);
#endif

  config->setGroup("Geometry");
  str = config->readEntry("composer", "480 510");
  sscanf(str.data(), "%d %d", &w, &h);
  if (w<200) w=200;
  if (h<200) h=200;
  resize(w, h);
}	


//-----------------------------------------------------------------------------
void KMComposeWin::writeConfig(void)
{
  KConfig *config = kapp->config();
  QString str;

  config->setGroup("Composer");
  config->writeEntry("signature", mAutoSign?"auto":"manual");
  config->writeEntry("show-toolbar", mShowToolBar);
  config->writeEntry("encoding", mDefEncoding);
  config->writeEntry("headers", mShowHeaders);
#if defined CHARSETS
  config->writeEntry("7bit-is-ascii",m7BitAscii);
  config->writeEntry("quote-unknown",mQuoteUnknownCharacters);
  config->writeEntry("default-charset",mDefaultCharset);
  config->writeEntry("composer-charset",mDefComposeCharset);
#endif

  config->setGroup("Geometry");
  str.sprintf("%d %d", width(), height());
  config->writeEntry("composer", str);
}


//-----------------------------------------------------------------------------
void KMComposeWin::deadLetter(void)
{
  FILE* fh;
  char fname[128];
  const char* msgStr;

  if (!mMsg) return;

  // This method is called when KMail crashed, so we better use as
  // basic functions as possible here.
  applyChanges();
  msgStr = mMsg->asString();

  sprintf(fname,"%s/dead.letter",getenv("HOME"));
  fh = fopen(fname,"a");
  if (fh)
  {
    fwrite("From ???@??? Mon Jan 01 00:00:00 1997\n", 19, 1, fh);
    fwrite(msgStr, strlen(msgStr), 1, fh);
    fwrite("\n", 1, 1, fh);
    fclose(fh);
    fprintf(stderr,"appending message to ~/dead.letter\n");
  }
  else perror("cannot open ~/dead.letter");
}


//-----------------------------------------------------------------------------
void KMComposeWin::setAutoDelete(bool f)
{
  mAutoDeleteMsg = f;
}


//-----------------------------------------------------------------------------
void KMComposeWin::rethinkFields(void)
{
  int mask, row, numRows;
  long showHeaders;

  if (mShowHeaders < 0)
  {
    showHeaders = HDR_ALL;
    mMnuView->setItemChecked(0, TRUE);
  }
  else
  {
    showHeaders = mShowHeaders;
    mMnuView->setItemChecked(0, FALSE);
  }

  for (mask=1,mNumHeaders=0; mask<=showHeaders; mask<<=1)
    if ((showHeaders&mask) != 0) mNumHeaders++;

  numRows = mNumHeaders + 2;

  if (mGrid) delete mGrid;
  mGrid = new QGridLayout(&mMainWidget, numRows, 3, 4, 4);
  mGrid->setColStretch(0, 1);
  mGrid->setColStretch(1, 100);
  mGrid->setColStretch(2, 1);
  mGrid->setRowStretch(mNumHeaders, 100);

  mEdtList.clear();
  row = 0;
  rethinkHeaderLine(showHeaders,HDR_FROM, row, i18n("&From:"),
		    &mLblFrom, &mEdtFrom, &mBtnFrom);
  rethinkHeaderLine(showHeaders,HDR_REPLY_TO,row,i18n("&Reply to:"),
		    &mLblReplyTo, &mEdtReplyTo, &mBtnReplyTo);
  rethinkHeaderLine(showHeaders,HDR_TO, row, i18n("&To:"),
		    &mLblTo, &mEdtTo, &mBtnTo);
  rethinkHeaderLine(showHeaders,HDR_CC, row, i18n("&Cc:"),
		    &mLblCc, &mEdtCc, &mBtnCc);
  rethinkHeaderLine(showHeaders,HDR_BCC, row, i18n("&Bcc:"),
		    &mLblBcc, &mEdtBcc, &mBtnBcc);
  rethinkHeaderLine(showHeaders,HDR_SUBJECT, row, i18n("&Subject:"),
		    &mLblSubject, &mEdtSubject);
#ifdef KRN
  rethinkHeaderLine(showHeaders,HDR_NEWSGROUPS, row, i18n("&Newsgroups:"),
		    &mLblNewsgroups, &mEdtNewsgroups);
  rethinkHeaderLine(showHeaders,HDR_FOLLOWUP_TO, row, i18n("&Followup-To:"),
		    &mLblFollowupTo, &mEdtFollowupTo);
#endif
  assert(row<=mNumHeaders);

  mGrid->addMultiCellWidget(mEditor, row, mNumHeaders, 0, 2);
  mGrid->addMultiCellWidget(mAtmListBox, mNumHeaders+1, mNumHeaders+1, 0, 2);

  if (mAtmList.count() > 0) mAtmListBox->show();
  else mAtmListBox->hide();
  resize(this->size());
  repaint();

  mGrid->activate();
}


//-----------------------------------------------------------------------------
void KMComposeWin::rethinkHeaderLine(int aValue, int aMask, int& aRow,
				     const QString aLabelStr, QLabel* aLbl,
				     QLineEdit* aEdt, QPushButton* aBtn)
{
  if (aValue & aMask)
  {
    mMnuView->setItemChecked(aMask, TRUE);
    aLbl->setText(aLabelStr);
    aLbl->adjustSize();
    aLbl->resize((int)aLbl->sizeHint().width(),aLbl->sizeHint().height() + 6);
    aLbl->setMinimumSize(aLbl->size());
    aLbl->show();
    aLbl->setBuddy(aEdt);
    mGrid->addWidget(aLbl, aRow, 0);

    aEdt->setBackgroundColor( backColor );
    aEdt->show();
    aEdt->setMinimumSize(100, aLbl->height()+2);
    aEdt->setMaximumSize(1000, aLbl->height()+2);
    //aEdt->setFocusPolicy(QWidget::ClickFocus);
    //aEdt->setFocusPolicy(QWidget::StrongFocus);
    mEdtList.append(aEdt);

    if (aBtn)
    {
      mGrid->addWidget(aEdt, aRow, 1);
      mGrid->addWidget(aBtn, aRow, 2);
      aBtn->setFixedSize(aBtn->sizeHint().width(), aLbl->height());
      aBtn->show();
    }
    else mGrid->addMultiCellWidget(aEdt, aRow, aRow, 1, 2);
    aRow++;
  }
  else
  {
    mMnuView->setItemChecked(aMask, FALSE);
    aLbl->hide();
    aEdt->hide();
    // aEdt->setFocusPolicy(QWidget::NoFocus);
    if (aBtn) aBtn->hide();
  }

  mMnuView->setItemEnabled(aMask, (aValue!=HDR_ALL));
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupMenuBar(void)
{
  QPopupMenu *menu;
  mMenuBar = menuBar();// new KMenuBar(this);


  //---------- Menu: File
  menu = new QPopupMenu();
  menu->insertItem(i18n("&Send"),this, SLOT(slotSend()),
		   CTRL+Key_Return);
  if (msgSender->sendImmediate())
    menu->insertItem(i18n("S&end later"),this,SLOT(slotSendLater()));
  else
    menu->insertItem(i18n("S&end now"),this,SLOT(slotSendNow()));
  menu->insertSeparator();
  menu->insertItem(i18n("&Insert File..."), this,
		    SLOT(slotInsertFile()));

  menu->insertSeparator();
  menu->insertItem(i18n("&Addressbook..."),this,
		   SLOT(slotAddrBook()));
  menu->insertItem(i18n("&Print..."),this,
		   SLOT(slotPrint()), KStdAccel::key(KStdAccel::Print));
  menu->insertSeparator();
  menu->insertItem(i18n("&New Composer..."),this,
           SLOT(slotNewComposer()), KStdAccel::key(KStdAccel::New));
#ifndef KRN
  menu->insertItem(i18n("New Mailreader"), this,
		   SLOT(slotNewMailReader()));
#endif
  menu->insertSeparator();
  menu->insertItem(i18n("&Close"),this,
		   SLOT(slotClose()), KStdAccel::key(KStdAccel::Close));
  mMenuBar->insertItem(i18n("&File"),menu);


  //---------- Menu: Edit
  menu = new QPopupMenu();
#ifdef BROKEN
  menu->insertItem(i18n("Undo"),this,
		   SLOT(slotUndoEvent()), KStdAccel::key(KStdAccel::Undo));
  menu->insertSeparator();
#endif //BROKEN
  menu->insertItem(i18n("Und&o"),mEditor,
		   SLOT(undo()), KStdAccel::key(KStdAccel::Undo));
  menu->insertItem(i18n("Re&do"),mEditor,
				   SLOT(redo()), KStdAccel::key(KStdAccel::Redo));
  menu->insertSeparator();
  menu->insertItem(i18n("C&ut"), this, SLOT(slotCut()), KStdAccel::key(KStdAccel::Cut));
  menu->insertItem(i18n("&Copy"), this, SLOT(slotCopy()), KStdAccel::key(KStdAccel::Copy));
  menu->insertItem(i18n("&Paste"), this, SLOT(slotPaste()), KStdAccel::key(KStdAccel::Paste));
  menu->insertItem(i18n("Select &All"),this,
		   SLOT(slotMarkAll()));
  menu->insertSeparator();
  menu->insertItem(i18n("&Find..."), this,
		   SLOT(slotFind()), KStdAccel::key(KStdAccel::Find));
  menu->insertItem(i18n("&Replace..."), this,
		   SLOT(slotReplace()), KStdAccel::key(KStdAccel::Replace));
  menu->insertSeparator();
  menu->insertItem(i18n("&Spellcheck..."), this,
		   SLOT(slotSpellcheck()));
  menu->insertItem(i18n("Cl&ean Spaces"), this,
		   SLOT(slotCleanSpace()));
  mMenuBar->insertItem(i18n("&Edit"),menu);

  //---------- Menu: Options
  menu = new QPopupMenu();
  menu->setCheckable(TRUE);

  mMnuIdUrgent = menu->insertItem(i18n("&Urgent"), this,
				  SLOT(slotToggleUrgent()));
  mMnuIdConfDeliver=menu->insertItem(i18n("&Confirm delivery"), this,
				     SLOT(slotToggleConfirmDelivery()));
  mMnuIdConfRead = menu->insertItem(i18n("&Confirm read"), this,
				    SLOT(slotToggleConfirmRead()));
  menu->insertSeparator();
  menu->insertItem(i18n("&Spellchecker..."), this,
		   SLOT (slotSpellcheckConfig()));
#if defined CHARSETS				
  mMnuIdConfRead = menu->insertItem(i18n("&Charsets..."), this,
				    SLOT(slotConfigureCharsets()));
#endif				
  mMenuBar->insertItem(i18n("&Options"),menu);
  mMnuOptions = menu;

  //---------- Menu: View
  menu = new QPopupMenu();
  mMnuView = menu;
  menu->setCheckable(TRUE);
  connect(menu, SIGNAL(activated(int)),
	  this, SLOT(slotMenuViewActivated(int)));

  menu->insertItem(i18n("&All Fields"), 0);
  menu->insertSeparator();
  menu->insertItem(i18n("&From"), HDR_FROM);
  menu->insertItem(i18n("&Reply to"), HDR_REPLY_TO);
  menu->insertItem(i18n("&To"), HDR_TO);
  menu->insertItem(i18n("&Cc"), HDR_CC);
  menu->insertItem(i18n("&Bcc"), HDR_BCC);
  menu->insertItem(i18n("&Subject"), HDR_SUBJECT);
#ifdef KRN
  menu->insertItem(i18n("&Newsgroups"), HDR_NEWSGROUPS); // for KRN
  menu->insertItem(i18n("&Followup-To"), HDR_FOLLOWUP_TO); // for KRN
#endif
  mMenuBar->insertItem(i18n("&View"), menu);

  //---------- Menu: Attach
  menu = new QPopupMenu();
  menu->insertItem(i18n("Append S&ignature"), this,
		   SLOT(slotAppendSignature()));
  menu->insertItem(i18n("&Insert File"), this,
		   SLOT(slotInsertFile()));

  menu->insertItem(i18n("&Attach..."), this, SLOT(slotAttachFile()));
  menu->insertSeparator();
  int id= menu->insertItem(i18n("Attach &Public Key"), this,
		    SLOT(slotInsertPublicKey()));
  int id1=menu->insertItem(i18n("Attach My &Public Key"), this,
		    SLOT(slotInsertMyPublicKey()));

  if(!Kpgp::getKpgp()->havePGP())
  {
      menu->setItemEnabled(id, false);
      menu->setItemEnabled(id1, false);
  }

  menu->insertSeparator();
  menu->insertItem(i18n("&Remove"), this, SLOT(slotAttachRemove()));
  menu->insertItem(i18n("&Save..."), this, SLOT(slotAttachSave()));
  menu->insertItem(i18n("Pr&operties..."),
		   this, SLOT(slotAttachProperties()));
  mMenuBar->insertItem(i18n("&Attach"), menu);

  //---------- Menu: Help
  menu = helpMenu(aboutText);
  mMenuBar->insertSeparator();
  mMenuBar->insertItem(i18n("&Help"), menu);

  //setMenu(mMenuBar);
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupToolBar(void)
{
  mToolBar = new KToolBar(this);

  mToolBar->insertButton(BarIcon("send"),0,
			SIGNAL(clicked()),this,
			SLOT(slotSend()),TRUE,i18n("Send message"));
  mToolBar->insertSeparator();
  mToolBar->insertButton(BarIcon("filenew"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotNewComposer()), TRUE,
			i18n("Compose new message"));

  if (msgSender->sendImmediate())
  mToolBar->insertButton(BarIcon("filefloppy"), 0,
			SIGNAL(clicked()), this,
			 SLOT(slotSendLater()), TRUE,
			 i18n("send later")); //grr translations!!!

  mToolBar->insertButton(BarIcon("fileprint"), 0,
			SIGNAL(clicked()), this,
			SLOT(slotPrint()), TRUE,
			i18n("Print message"));
  mToolBar->insertSeparator();
#ifdef BROKEN
  mToolBar->insertButton(BarIcon("reload"),2,
			SIGNAL(clicked()),this,
			SLOT(slotCopyText()),TRUE,"Undo last change");
#endif
  mToolBar->insertButton(BarIcon("editcut"),4,
			SIGNAL(clicked()),this,
			SLOT(slotCut()),TRUE,i18n("Cut selection"));
  mToolBar->insertButton(BarIcon("editcopy"),3,
			SIGNAL(clicked()),this,
			SLOT(slotCopy()),TRUE,i18n("Copy selection"));
  mToolBar->insertButton(BarIcon("editpaste"),5,
			SIGNAL(clicked()),this,
			SLOT(slotPaste()),TRUE,i18n("Paste clipboard contents"));
  mToolBar->insertSeparator();

  mToolBar->insertButton(BarIcon("attach"),8,
			 SIGNAL(clicked()),this,
			 SLOT(slotAttachFile()),TRUE,i18n("Attach file"));
  mToolBar->insertButton(BarIcon("openbook"),7,
			 SIGNAL(clicked()),this,
			 SLOT(slotAddrBook()),TRUE,
			 i18n("Open addressbook..."));
  mToolBar->insertButton(BarIcon("spellcheck"),7,
			SIGNAL(clicked()),this,
			SLOT(slotSpellcheck()),TRUE,"Spellcheck message");
  mToolBar->insertSeparator();
  mBtnIdSign = 9;
  mToolBar->insertButton(BarIcon("feather_white"), mBtnIdSign,
			 TRUE, i18n("sign message"));
  mToolBar->setToggle(mBtnIdSign);
  mToolBar->setButton(mBtnIdSign, mAutoPgpSign);
  mBtnIdEncrypt = 10;
  mToolBar->insertButton(BarIcon("pub_key_red"), mBtnIdEncrypt,
			 TRUE, i18n("encrypt message"));
  mToolBar->setToggle(mBtnIdEncrypt);
  // these buttons should only be enabled, if pgp is actually installed
  if(!Kpgp::getKpgp()->havePGP())
  {
    mToolBar->setItemEnabled(mBtnIdSign, false);
    mToolBar->setItemEnabled(mBtnIdEncrypt, false);
  }
  addToolBar(mToolBar);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupStatusBar(void)
{
  mStatusBar = new KStatusBar(this);
#warning rwilliams: statusbar
  //mStatusBar->setInsertOrder(KStatusBar::RightToLeft);
  mStatusBar->insertItem(QString(i18n("Column"))+":     ",2);
  mStatusBar->insertItem(QString(i18n("Line"))+":     ",1);
  mStatusBar->insertItem("  ",0);
  setStatusBar(mStatusBar);
}


//-----------------------------------------------------------------------------
void KMComposeWin::updateCursorPosition()
{
  int col,line;
  QString temp;
  line = mEditor->currentLine();
  col = mEditor->currentColumn();
  temp = QString("%1: %2").arg(i18n("Line")).arg(line+1);
  mStatusBar->changeItem(temp,1);
  temp = QString("%1: %2").arg(i18n("Column")).arg(col+1);
  mStatusBar->changeItem(temp,2);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupEditor(void)
{
  QPopupMenu* menu;
  mEditor = new KMEdit(&mMainWidget, this);
  mEditor->setModified(FALSE);
  //mEditor->setFocusPolicy(QWidget::ClickFocus);

  if (mWordWrap)
  {
    mEditor->setWordWrap( QMultiLineEdit::FixedColumnWidth );
    mEditor->setWrapColumnOrWidth(mLineBreak);
  }
  else
  {
    mEditor->setWordWrap( QMultiLineEdit::NoWrap );
  }

  // Font setup
  mEditor->setFont(mBodyFont);

  // Color setup
  mEditor->setPalette(mPalette);
  mEditor->setBackgroundColor(backColor);

  menu = new QPopupMenu();
  //#ifdef BROKEN
  menu->insertItem(i18n("Undo"),mEditor,
		   SLOT(undo()), KStdAccel::key(KStdAccel::Undo));
  menu->insertItem(i18n("Redo"),mEditor,
		   SLOT(redo()), KStdAccel::key(KStdAccel::Redo));
  menu->insertSeparator();
  //#endif //BROKEN
  menu->insertItem(i18n("Cut"), this, SLOT(slotCut()));
  menu->insertItem(i18n("Copy"), this, SLOT(slotCopy()));
  menu->insertItem(i18n("Paste"), this, SLOT(slotPaste()));
  menu->insertItem(i18n("Mark all"),this, SLOT(slotMarkAll()));
  menu->insertSeparator();
  menu->insertItem(i18n("Find..."), this, SLOT(slotFind()));
  menu->insertItem(i18n("Replace..."), this, SLOT(slotReplace()));
  mEditor->installRBPopup(menu);
    updateCursorPosition();
connect(mEditor,SIGNAL(CursorPositionChanged()),SLOT(updateCursorPosition()));
}

//-----------------------------------------------------------------------------
void KMComposeWin::verifyWordWrapLengthIsAdequate(const QString &body)
{
  int maxLineLength = 0;
  int curPos;
  int oldPos = 0;
  if (mEditor->QMultiLineEdit::wordWrap() == QMultiLineEdit::FixedColumnWidth) {
    for (curPos = 0; curPos < (int)body.length(); ++curPos)
	if (body[curPos] == '\n') {
	  if ((curPos - oldPos) > maxLineLength)
	    maxLineLength = curPos - oldPos;
	  oldPos = curPos;
	}
    if ((curPos - oldPos) > maxLineLength)
      maxLineLength = curPos - oldPos;
    if (mEditor->wrapColumnOrWidth() < maxLineLength) // column
      mEditor->setWrapColumnOrWidth(maxLineLength);
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::setMsg(KMMessage* newMsg, bool mayAutoSign)
{
  KMMessagePart bodyPart, *msgPart;
  int i, num;

  //assert(newMsg!=NULL);
  if(!newMsg)
    {
      debug("KMComposeWin::setMsg() : newMsg == NULL!\n");
      return;
    }
  mMsg = newMsg;

  mEdtTo.setText(mMsg->to());
  mEdtFrom.setText(mMsg->from());
  mEdtCc.setText(mMsg->cc());
  mEdtSubject.setText(mMsg->subject());
  mEdtReplyTo.setText(mMsg->replyTo());
  mEdtBcc.setText(mMsg->bcc());
#ifdef KRN
  mEdtNewsgroups.setText(mMsg->groups());
  mEdtFollowupTo.setText(mMsg->followup());
#endif

  num = mMsg->numBodyParts();
  if (num > 0)
  {
    QString bodyDecoded;
    mMsg->bodyPart(0, &bodyPart);

#if defined CHARSETS
    mCharset=bodyPart.charset();
    cout<<"Charset: "<<mCharset<<"\n";
    if (mCharset==""){
      mComposeCharset=mDefComposeCharset;
      if (mComposeCharset=="default"
                        && KGlobal::charsets()->isAvailable(mDefaultCharset))
        mComposeCharset=mDefaultCharset;
    }	
    else if (KGlobal::charsets()->isAvailable(mCharset))
      mComposeCharset=mCharset;
    else mComposeCharset=mDefComposeCharset;
    cout<<"Compose charset: "<<mComposeCharset<<"\n";

    // FIXME: We might need a QTextCodec here
#endif
    bodyDecoded = QString(bodyPart.bodyDecoded());
    verifyWordWrapLengthIsAdequate(bodyDecoded);
    mEditor->setText(bodyDecoded);
    mEditor->insertLine("\n", -1);

    for(i=1; i<num; i++)
    {
      msgPart = new KMMessagePart;
      mMsg->bodyPart(i, msgPart);
      addAttach(msgPart);
    }
  }
#if defined CHARSETS
  else{
    mCharset=mMsg->charset();
    cout<<"mCharset: "<<mCharset<<"\n";
    if (mCharset==""){
      mComposeCharset=mDefComposeCharset;
      if (mComposeCharset=="default"
                        && KGlobal::charsets()->isAvailable(mDefaultCharset))
        mComposeCharset=mDefaultCharset;
    }	
    else if (KGlobal::charsets()->isAvailable(mCharset))
      mComposeCharset=mCharset;
    else mComposeCharset=mDefComposeCharset;
    cout<<"Compose charset: "<<mComposeCharset<<"\n";
    // FIXME: QTextCodec needed?
    Editor->setText(QString(mMsg->bodyDecoded()));
  }
#else
  else {
    QString bodyDecoded = QString(mMsg->bodyDecoded());
    verifyWordWrapLengthIsAdequate(bodyDecoded);
    mEditor->setText(bodyDecoded);
  }
#endif

  if (mAutoSign && mayAutoSign) slotAppendSignature();
  mEditor->setModified(FALSE);

#if defined CHARSETS
  setEditCharset();
#endif
}


//-----------------------------------------------------------------------------
bool KMComposeWin::applyChanges(void)
{
  QString str, atmntStr;
  QString temp, replyAddr;
  KMMessagePart bodyPart, *msgPart;

  //assert(mMsg!=NULL);
  if(!mMsg)
  {
    debug("KMComposeWin::applyChanges() : mMsg == NULL!\n");
    return FALSE;
  }

  mMsg->setTo(to());
  mMsg->setFrom(from());
  mMsg->setCc(cc());
  mMsg->setSubject(subject());
  mMsg->setReplyTo(replyTo());
  mMsg->setBcc(bcc());
#ifdef KRN
  mMsg->setFollowup(followupTo());
  mMsg->setGroups(newsgroups());
#endif

  if (!replyTo().isEmpty()) replyAddr = replyTo();
  else replyAddr = from();

  if (mMnuOptions->isItemChecked(mMnuIdConfDeliver))
    mMsg->setHeaderField("Return-Receipt-To", replyAddr);

  if (mMnuOptions->isItemChecked(mMnuIdConfRead))
    mMsg->setHeaderField("X-Chameleon-Return-To", replyAddr);

  if (mMnuOptions->isItemChecked(mMnuIdUrgent))
  {
    mMsg->setHeaderField("X-PRIORITY", "2 (High)");
    mMsg->setHeaderField("Priority", "urgent");
  }

#ifndef KRN
  _StringPair *pCH;
  for (pCH  = mCustHeaders.first();
       pCH != NULL;
       pCH  = mCustHeaders.next()) {
    mMsg->setHeaderField(pCH->name, pCH->value);
  }

#endif

  if(mAtmList.count() <= 0)
  {
    mMsg->setAutomaticFields(FALSE);

    // If there are no attachments in the list waiting it is a simple
    // text message.
    if (msgSender->sendQuotedPrintable())
    {
      mMsg->setTypeStr("text");
      mMsg->setSubtypeStr("plain");
      mMsg->setCteStr("quoted-printable");
      str = pgpProcessedMsg();
      if (str.isNull()) return FALSE;
#if defined CHARSETS
      convertToSend(str);
      cout<<"Setting charset to: "<<mCharset<<"\n";
      mMsg->setCharset(mCharset);
#endif
      mMsg->setBodyEncoded(str);
    }
    else
    {
      mMsg->setTypeStr("text");
      mMsg->setSubtypeStr("plain");
      mMsg->setCteStr("8bit");
      str = pgpProcessedMsg();
      if (str.isNull()) return FALSE;
#if defined CHARSETS
      convertToSend(str);
      cout<<"Setting charset to: "<<mCharset<<"\n";
      mMsg->setCharset(mCharset);
#endif
      mMsg->setBody(str);
    }
  }
  else
  {
    mMsg->deleteBodyParts();
    mMsg->setAutomaticFields(TRUE);

    // create informative header for those that have no mime-capable
    // email client
    mMsg->setBody("This message is in MIME format.\n\n");

    // create bodyPart for editor text.
    if (msgSender->sendQuotedPrintable())
         bodyPart.setCteStr("quoted-printable");
    else bodyPart.setCteStr("8bit");
    bodyPart.setTypeStr("text");
    bodyPart.setSubtypeStr("plain");
    str = pgpProcessedMsg();
    if (str.isNull()) return FALSE;
#if defined CHARSETS
    convertToSend(str);
    cout<<"Setting charset to: "<<mCharset<<"\n";
    mMsg->setCharset(mCharset);
#endif
    str.truncate(str.length()); // to ensure str.size()==str.length()+1
    bodyPart.setBodyEncoded(QCString(str.ascii()));
    mMsg->addBodyPart(&bodyPart);

    // Since there is at least one more attachment create another bodypart
    for (msgPart=mAtmList.first(); msgPart; msgPart=mAtmList.next())
	mMsg->addBodyPart(msgPart);
  }
  if (!mAutoDeleteMsg) mEditor->setModified(FALSE);

  // remove fields that contain no data (e.g. an empty Cc: or Bcc:)
  mMsg->cleanupHeader();
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMComposeWin::closeEvent(QCloseEvent* e)
{
    int rc;

  if(mEditor->isModified())
  {
    rc = KMessageBox::warningContinueCancel(this,
           i18n("Close and discard\nedited message?"),
           i18n("Close message"), i18n("&Discard"));
    if (rc == KMessageBox::Cancel)
    {
       e->ignore();
       return;
    }
  }
  delete this; // ugh
  //KMComposeWinInherited::closeEvent(e);
}


//-----------------------------------------------------------------------------
const QString KMComposeWin::pgpProcessedMsg(void)
{
  Kpgp *pgp = Kpgp::getKpgp();
  bool doSign = mToolBar->isButtonOn(mBtnIdSign);
  bool doEncrypt = mToolBar->isButtonOn(mBtnIdEncrypt);
  QString _to, receiver;
  int index, lastindex;
  QStrList persons;

  if (!doSign && !doEncrypt) return mEditor->brokenText();

  pgp->setMessage(mEditor->brokenText());

  if (!doEncrypt)
  {
    if(pgp->sign()) return pgp->message();
  }
  else
  {
    // encrypting
    _to = to().copy();
    if(!cc().isEmpty()) _to += "," + cc();
    if(!bcc().isEmpty()) _to += "," + bcc();
    lastindex = -1;
    do
    {
      index = _to.find(",",lastindex+1);
      receiver = _to.mid(lastindex+1, index<0 ? 255 : index-lastindex-1);
      if (!receiver.isEmpty())
      {
	persons.append(receiver);
      }
      lastindex = index;
    }
    while (lastindex > 0);

    if(pgp->encryptFor(persons, doSign))
      return pgp->message();
  }

  // in case of an error we end up here
  //warning(i18n("Error during PGP:") + QString("\n") +
  //	  pgp->lastErrorMsg());

  return QString::null;
}


//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(const QString aUrl)
{
  QString name;
  QByteArray str;
  KMMessagePart* msgPart;
  KMMsgPartDlg dlg;
  int i;

  // load the file
  kbp->busy();
  str = kFileToBytes(aUrl,FALSE);
  if (str.isNull())
  {
    kbp->idle();
    return;
  }

  // create message part
  i = aUrl.findRev('/');
  name = (i>=0 ? aUrl.mid(i+1, 256) : aUrl);
  msgPart = new KMMessagePart;
  msgPart->setName(name);
  msgPart->setCteStr(mDefEncoding);
  msgPart->setBodyEncoded(str);
  //  msgPart->setBodyEncoded(QCString(str.ascii()));
  msgPart->magicSetType();
  msgPart->setContentDisposition("attachment; filename=\""+name+"\"");

  // show properties dialog
  kbp->idle();
  dlg.setMsgPart(msgPart);
  if (!dlg.exec())
  {
    delete msgPart;
    return;
  }

  // add the new attachment to the list
  addAttach(msgPart);
  rethinkFields(); //work around initial-size bug in Qt-1.32
}


//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(const KMMessagePart* msgPart)
{
  mAtmList.append(msgPart);

  // show the attachment listbox if it does not up to now
  if (mAtmList.count()==1)
  {
    mGrid->setRowStretch(mNumHeaders+1, 1);
    mAtmListBox->setMinimumSize(100, 80);
    mAtmListBox->show();
    resize(size());
  }

  // add a line in the attachment listbox
  mAtmListBox->insertItem(msgPartLbxString(msgPart));
}


//-----------------------------------------------------------------------------
const QString KMComposeWin::msgPartLbxString(const KMMessagePart* msgPart) const {
  unsigned int len;
  QString lenStr;

  assert(msgPart != NULL);

  len = msgPart->size();
  if (len > 9999) lenStr.sprintf("%uK", (len>>10));
  else lenStr.sprintf("%u", len);

  return (" \n" + msgPart->name() + "\n" + lenStr + "\n" +
	  msgPart->contentTransferEncodingStr() + "\n" +
	  msgPart->typeStr() + "/" + msgPart->subtypeStr());
}


//-----------------------------------------------------------------------------
void KMComposeWin::removeAttach(const QString aUrl)
{
  int idx;
  KMMessagePart* msgPart;

  for(idx=0,msgPart=mAtmList.first(); msgPart;
      msgPart=mAtmList.next(),idx++) {
    if (msgPart->name() == aUrl) {
      removeAttach(idx);
      return;
    }
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::removeAttach(int idx)
{
  mAtmList.remove(idx);
  mAtmListBox->removeItem(idx);

  if (mAtmList.count()<=0)
  {
    mAtmListBox->hide();
    mGrid->setRowStretch(mNumHeaders+1, 0);
    mAtmListBox->setMinimumSize(0, 0);
    resize(size());
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::addrBookSelInto(KMLineEdit* aLineEdit)
{
  KMAddrBookSelDlg dlg(addrBook);
  QString txt;

  //assert(aLineEdit!=NULL);
  if(!aLineEdit)
    {
      debug("KMComposeWin::addrBookSelInto() : aLineEdit == NULL\n");
      return;
    }
  if (dlg.exec()==QDialog::Rejected) return;
  txt = QString(aLineEdit->text()).stripWhiteSpace();
  if (!txt.isEmpty())
  {
    if (txt.right(1).at(0)!=',') txt += ", ";
    else txt += ' ';
  }
  aLineEdit->setText(txt + dlg.address());
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotToggleConfirmDelivery()
{
  mMnuOptions->setItemChecked(mMnuIdConfDeliver,
			      !mMnuOptions->isItemChecked(mMnuIdConfDeliver));
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotToggleConfirmRead()
{
  mMnuOptions->setItemChecked(mMnuIdConfRead,
			      !mMnuOptions->isItemChecked(mMnuIdConfRead));
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotToggleUrgent()
{
  mMnuOptions->setItemChecked(mMnuIdUrgent,
			      !mMnuOptions->isItemChecked(mMnuIdUrgent));
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBook()
{
  KMAddrBookEditDlg dlg(addrBook);
  dlg.exec();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookFrom()
{
  addrBookSelInto(&mEdtFrom);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookReplyTo()
{
  addrBookSelInto(&mEdtReplyTo);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookTo()
{
  addrBookSelInto(&mEdtTo);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookCc()
{
  addrBookSelInto(&mEdtCc);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookBcc()
{
  addrBookSelInto(&mEdtBcc);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFile()
{
  // Create File Dialog and return selected file
  // We will not care about any permissions, existence or whatsoever in
  // this function.
  if (mPathAttach.isEmpty()) mPathAttach = QDir::currentDirPath();

  KFileDialog fdlg(mPathAttach, "*", this, NULL, TRUE);

  fdlg.setCaption(i18n("Attach File"));
  if (!fdlg.exec()) return;

  KURL u = fdlg.selectedURL();

  mPathAttach = u.directory();

  if(u.filename().isEmpty()) return;
  addAttach(u.path());
  mEditor->setModified(TRUE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertFile()
{
  QString str;
  int col, line;

  if (mPathAttach.isEmpty()) mPathAttach = QDir::currentDirPath();

  KFileDialog fdlg(mPathAttach, "*", this, NULL, TRUE);
  fdlg.setCaption(i18n("Include File"));
  if (!fdlg.exec()) return;

  KURL u = fdlg.selectedURL();

  mPathAttach = u.directory();

  if (u.filename().isEmpty()) return;

  if( !u.isLocalFile() )
  {
    KMessageBox::sorry( 0L, i18n( "Only local files are supported." ) );
    return;
  }

  str = kFileToString(u.path(), TRUE, TRUE);
  if (str.isEmpty()) return;

  mEditor->getCursorPosition(&line, &col);
  mEditor->insertAt(QCString(str), line, col);
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertMyPublicKey()
{
  QString str, name;
  KMMessagePart* msgPart;

  // load the file
  kbp->busy();
  str=Kpgp::getKpgp()->getAsciiPublicKey(mMsg->from());
  if (str.isNull())
  {
    kbp->idle();
    return;
  }

  // create message part
  msgPart = new KMMessagePart;
  msgPart->setName(i18n("my pgp key"));
  msgPart->setCteStr(mDefEncoding);
  msgPart->setTypeStr("application");
  msgPart->setSubtypeStr("pgp-keys");
  msgPart->setBodyEncoded(QCString(str.ascii()));
  msgPart->setContentDisposition("attachment; filename=public_key.asc");

  // add the new attachment to the list
  addAttach(msgPart);
  rethinkFields(); //work around initial-size bug in Qt-1.32

  kbp->idle();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertPublicKey()
{
  QString str, name;
  KMMessagePart* msgPart;
  Kpgp *pgp;

  pgp=Kpgp::getKpgp();

  str=pgp->getAsciiPublicKey(
         KpgpKey::getKeyName(this, pgp->keys()));

  // create message part
  msgPart = new KMMessagePart;
  msgPart->setName(i18n("pgp key"));
  msgPart->setCteStr(mDefEncoding);
  msgPart->setTypeStr("application");
  msgPart->setSubtypeStr("pgp-keys");
  msgPart->setBodyEncoded(QCString(str.ascii()));
  msgPart->setContentDisposition("attachment; filename=public_key.asc");

  // add the new attachment to the list
  addAttach(msgPart);
  rethinkFields(); //work around initial-size bug in Qt-1.32

}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachPopupMenu(int index, int)
{
  QPopupMenu *menu = new QPopupMenu;

  mAtmListBox->setCurrentItem(index);

  menu->insertItem(i18n("View..."), this, SLOT(slotAttachView()));
  menu->insertItem(i18n("Save..."), this, SLOT(slotAttachSave()));
  menu->insertItem(i18n("Properties..."),
		   this, SLOT(slotAttachProperties()));
  menu->insertItem(i18n("Remove"), this, SLOT(slotAttachRemove()));
  menu->insertSeparator();
  menu->insertItem(i18n("Attach..."), this, SLOT(slotAttachFile()));
  menu->popup(QCursor::pos());
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachProperties()
{
  KMMsgPartDlg dlg;
  KMMessagePart* msgPart;
  int idx = mAtmListBox->currentItem();

  if (idx < 0) return;

  msgPart = mAtmList.at(idx);
  dlg.setMsgPart(msgPart);
  if (dlg.exec())
  {
    // values may have changed, so recreate the listbox line
    mAtmListBox->changeItem(msgPartLbxString(msgPart), idx);
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachView()
{
  QString str, pname;
  KMMessagePart* msgPart;
  QMultiLineEdit* edt = new QMultiLineEdit;

  int idx = mAtmListBox->currentItem();
  if (idx < 0) return;

  msgPart = mAtmList.at(idx);
  pname = msgPart->name();
  if (pname.isEmpty()) pname=msgPart->contentDescription();
  if (pname.isEmpty()) pname="unnamed";

  kbp->busy();
  str = QCString(msgPart->bodyDecoded());

  edt->setCaption(i18n("View Attachment: ") + pname);
  edt->insertLine(str);
  edt->setReadOnly(TRUE);
  edt->show();

  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachSave()
{
  KMMessagePart* msgPart;
  QString fileName, pname;

  int idx = mAtmListBox->currentItem();
  if (idx < 0) return;

  msgPart = mAtmList.at(idx);
  pname = msgPart->name();
  if (pname.isEmpty()) pname="unnamed";

  if (mPathAttach.isEmpty()) mPathAttach = QDir::currentDirPath();

  KURL url = KFileDialog::getSaveURL(mPathAttach, "*", NULL, pname);

  if( url.isEmpty() )
    return;

  if( !url.isLocalFile() )
  {
    KMessageBox::sorry( 0L, i18n( "Only local files supported yet." ) );
    return;
  }

  fileName = url.path();

  kByteArrayToFile(msgPart->bodyDecoded(), fileName, TRUE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachRemove()
{
  int idx = mAtmListBox->currentItem();
  if (idx >= 0)
  {
    removeAttach(idx);
    mEditor->setModified(TRUE);
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotToggleToolBar()
{
  enableToolBar(KToolBar::Toggle);
  mShowToolBar = !mShowToolBar;
  repaint();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotMenuViewActivated(int id)
{
  if (mMnuView->isItemChecked(id))
  {
    // hide header
    if (id > 0) mShowHeaders = mShowHeaders & ~id;
    else mShowHeaders = abs(mShowHeaders);
  }
  else
  {
    // show header
    if (id > 0) mShowHeaders |= id;
    else mShowHeaders = -abs(mShowHeaders);
  }
  rethinkFields();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotFind()
{
  mEditor->search();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotReplace()
{
  mEditor->replace();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotCut()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (fw->inherits("KEdit"))
    ((QMultiLineEdit*)fw)->cut();
  else if (fw->inherits("KMLineEdit"))
    ((KMLineEdit*)fw)->cut();
  else debug("wrong focus widget");
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotCopy()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

#ifdef KeyPress
#undef KeyPress
#endif

  QKeyEvent k(QEvent::KeyPress, Key_C , 0 , ControlButton);
  app->notify(fw, &k);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotPaste()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

#ifdef KeyPress
#undef KeyPress
#endif

  QKeyEvent k(QEvent::KeyPress, Key_V , 0 , ControlButton);
  app->notify(fw, &k);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotMarkAll()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (fw->inherits("QLineEdit"))
      ((QLineEdit*)fw)->selectAll();
  else if (fw->inherits("QMultiLineEdit"))
    ((QMultiLineEdit*)fw)->selectAll();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotClose()
{
  close(FALSE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotNewComposer()
{
  KMComposeWin* win;
  KMMessage* msg = new KMMessage;

  msg->initHeader();
  win = new KMComposeWin(msg);
  win->show();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotNewMailReader()
{

#ifndef KRN
  KMMainWin *d;

  d = new KMMainWin(NULL);
  d->show();
  d->resize(d->size());
#endif

}


//-----------------------------------------------------------------------------
void KMComposeWin::slotToDo()
{
  warning(i18n("Sorry, but this feature\nis still missing"));
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdWinTitle(const QString& text)
{
  if (text.isEmpty())
       setCaption("("+QString(i18n("unnamed"))+")");
  else setCaption(text);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotPrint()
{
  QPrinter printer;
  QPainter paint;
  QString  str;

  if (printer.setup(this))
  {
    paint.begin(&printer);
    str += i18n("To:");
    str += " \n";
    str += to();
    str += "\n";
    str += i18n("Subject:");
    str += " \n";
    str += subject();
    str += i18n("Date:");
    str += " \n\n";
    str += mEditor->brokenText();
    str += "\n";
    //str.replace(QRegExp("\n"),"\n");
    paint.drawText(30,30,str);
    paint.end();
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotDropAction()
{
#if 0
  QString  file;
  QStrList *fileList;

  fileList = &mDropZone->getURLList();

  for (file=fileList->first(); !file.isEmpty() ; file=fileList->next())
  {
    file.replace(QRegExp("file:"),"");
    addAttach(file);
  }
#endif
}


//----------------------------------------------------------------------------
void KMComposeWin::doSend(int aSendNow)
{
  bool sentOk;

  kbp->busy();
  //applyChanges();  // is called twice otherwise. Lars
  mMsg->setDateToday();
  sentOk = (applyChanges() && msgSender->send(mMsg, aSendNow));
  kbp->idle();

  if (sentOk)
  {
    mAutoDeleteMsg = FALSE;
    close();
  }
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSend()
{
#ifndef KRN
  if (mConfirmSend) {
    switch(QMessageBox::information(&mMainWidget,
                                    i18n("Send Confirmation"),
                                    i18n("About to send email..."),
                                    i18n("Send &Now"),
                                    i18n("Send &Later"),
                                    i18n("&Cancel"),
                                    0,
                                    2)) {
    case 0:        // send now
        doSend(TRUE);
      break;
    case 1:        // send later
        doSend(FALSE);
      break;
    case 2:        // cancel
      break;
    default:
      ;    // whoa something weird happened here!
    }
    return;
  }
#endif

  doSend();
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSendLater()
{
  doSend(FALSE);
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSendNow()
{
  doSend(TRUE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAppendSignature()
{
  QString sigFileName = identity->signatureFile();
  QString sigText;
  bool mod = mEditor->isModified();

  if (sigFileName.isEmpty())
  {
    // open a file dialog and let the user choose manually
#warning KFileDialog misses localfiles only flag.
//    KFileDialog dlg(getenv("HOME"),QString::null,this,0,TRUE,FALSE);
    KFileDialog dlg(getenv("HOME"),QString::null,this,0, TRUE);

    dlg.setCaption(i18n("Choose Signature File"));

    if( !dlg.exec() )
      return;

    KURL url = dlg.selectedURL();

    if( url.isEmpty() )
      return;

    if( !url.isLocalFile() )
    {
      KMessageBox::sorry( 0L, i18n( "Only local files are supported." ) );
      return;
    }

    sigFileName = url.path();
    sigText = kFileToString(sigFileName, TRUE);
    identity->setSignatureFile(sigFileName);
    identity->writeConfig(true);
  }
  else sigText = identity->signature();

  if (!sigText.isEmpty())
  {
    mEditor->insertLine("-- ", -1);
    mEditor->insertLine(sigText, -1);
    mEditor->setModified(mod);
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotHelp()
{
  app->invokeHTMLHelp("","");
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCleanSpace()
{
  if (!mEditor) return;

  mEditor->cleanWhiteSpace();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheck()
{
  if (mSpellCheckInProgress) return;

  mSpellCheckInProgress=TRUE;
  /*
    connect (mEditor, SIGNAL (spellcheck_progress (unsigned)),
    this, SLOT (spell_progress (unsigned)));
    */

  if(mEditor){
    connect (mEditor, SIGNAL (spellcheck_done()),
	     this, SLOT (slotSpellcheckDone ()));
    mEditor->spellcheck();
  }
}
//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheckDone()
{
  debug( "spell check complete" );
  mSpellCheckInProgress=FALSE;
  mStatusBar->changeItem(i18n("Spellcheck complete."),0);

}

//-----------------------------------------------------------------------------
void KMComposeWin::slotConfigureCharsets()
{
#if defined CHARSETS
   CharsetsDlg *dlg=new CharsetsDlg((const char*)mCharset,
				    (const char*)mComposeCharset,
                                    m7BitAscii,mQuoteUnknownCharacters);
   connect(dlg,SIGNAL( setCharsets(const char *,const char *,bool,bool,bool) )
           ,this,SLOT(slotSetCharsets(const char *,const char    dlg->show();
*,bool,bool,bool)));
   delete dlg;	
#endif
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSetCharsets(const char *message,const char *composer,
                                   bool ascii,bool quote,bool def)
{
  // prevent warning
  (void)message;
  (void)composer;
  (void)ascii;
  (void)quote;
  (void)def;

#if defined CHARSETS
  mCharset=message;
  m7BitAscii=ascii;
  mComposeCharset=composer;
  mQuoteUnknownCharacters=quote;
  if (def)
  {
    mDefaultCharset=message;
    mDefComposeCharset=composer;
  }
  setEditCharset();
#endif
}


#if defined CHARSETS
//-----------------------------------------------------------------------------
bool KMComposeWin::is8Bit(const QString str)
{
  const char *ptr=str;
  while(*ptr)
  {
    if ( (*ptr)&0x80 ) return TRUE;
    else if ( (*ptr)=='&' && ptr[1]=='#' )
    {
      int code=atoi(ptr+2);
      if (code>0x7f) return TRUE;
    }
    ptr++;
  }
  return FALSE;
}

//-----------------------------------------------------------------------------
void KMComposeWin::convertToSend(const QString str)
{
  cout<<"Converting to send...\n";
  if (m7BitAscii && !is8Bit(str))
  {
    mCharset="us-ascii";
    return;
  }
  if (mCharset=="")
  {
     if (mDefaultCharset=="default")
          mCharset=KGlobal::charsets()->charsetForLocale();
     else mCharset=mDefaultCharset;
  }
  cout<<"mCharset: "<<mCharset<<"\n";
}

//-----------------------------------------------------------------------------
void KMComposeWin::setEditCharset()
{
  QFont fnt=mSavedEditorFont;
  if (mComposeCharset=="default")
    mComposeCharset=KGlobal::charsets()->charsetForLocale();
  cout<<"Setting font to: "<<mComposeCharset<<"\n";
  KGlobal::charsets()->setQFont(fnt, mComposeCharset);
  mEditor->setFont(fnt);
}
#endif //CHARSETS


//-----------------------------------------------------------------------------
void KMComposeWin::focusNextPrevEdit(const QLineEdit* aCur, bool aNext)
{
  QLineEdit* cur;

  if (!aCur)
  {
    cur=mEdtList.last();
  }
  else
  {
    for (cur=mEdtList.first(); aCur!=cur && cur; cur=mEdtList.next())
      ;
    if (!cur) return;
    if (aNext) cur = mEdtList.next();
    else cur = mEdtList.prev();
  }
  if (cur) cur->setFocus();
  else if (aNext) mEditor->setFocus(); //Key up from first doea nothing (sven)
}



//=============================================================================
//
//   Class  KMLineEdit
//
//=============================================================================
KMLineEdit::KMLineEdit(KMComposeWin* composer, QWidget *parent,
		       const char *name): KMLineEditInherited(parent,name)
{
  mComposer = composer;

  installEventFilter(this);

  connect (this, SIGNAL(completion()), this, SLOT(slotCompletion()));
}


//-----------------------------------------------------------------------------
KMLineEdit::~KMLineEdit()
{
  removeEventFilter(this);
}


//-----------------------------------------------------------------------------
bool KMLineEdit::eventFilter(QObject*, QEvent* e)
{
#ifdef KeyPress
#undef KeyPress
#endif

  if (e->type() == QEvent::KeyPress)
  {
    QKeyEvent* k = (QKeyEvent*)e;

    if (k->state()==ControlButton && k->key()==Key_T)
    {
      emit completion();
      cursorAtEnd();
      return TRUE;
    }
    // ---sven's Return is same Tab and arrow key navigation start ---
    if (k->key() == Key_Enter || k->key() == Key_Return)
    {
      mComposer->focusNextPrevEdit(this,TRUE);
      return TRUE;
    }
    if (k->key() == Key_Right)
    {
      if ((int)strlen(text()) == cursorPosition()) // at End?
      {
        emit completion();
        cursorAtEnd();
        return TRUE;
      }
      return FALSE;
    }
    if (k->key() == Key_Up)
    {
      mComposer->focusNextPrevEdit(this,FALSE); // Go up
      return TRUE;
    }
    if (k->key() == Key_Down)
    {
      mComposer->focusNextPrevEdit(this,TRUE); // Go down
      return TRUE;
    }
    // ---sven's Return is same Tab and arrow key navigation end ---

  }
  else if (e->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = (QMouseEvent*)e;
    if (me->button() == RightButton)
    {
      QPopupMenu* p = new QPopupMenu;
      p->insertItem(i18n("Cut"),this,SLOT(cut()));
      p->insertItem(i18n("Copy"),this,SLOT(copy()));
      p->insertItem(i18n("Paste"),this,SLOT(paste()));
      p->insertItem(i18n("Mark all"),this,SLOT(markAll()));
      setFocus();
      p->popup(QCursor::pos());
      return TRUE;
    }
  }
  return FALSE;
}


//-----------------------------------------------------------------------------
void KMLineEdit::cursorAtEnd()
{
  QKeyEvent ev( QEvent::KeyPress, Key_End, 0, 0 );
  QLineEdit::keyPressEvent( &ev );
}


//-----------------------------------------------------------------------------
void KMLineEdit::copy()
{
  QString t = markedText();
  QApplication::clipboard()->setText(t);
}

//-----------------------------------------------------------------------------
void KMLineEdit::cut()
{
  if(hasMarkedText())
  {
    QString t = markedText();
    QKeyEvent k(QEvent::KeyPress, Key_D, 0, ControlButton);
    keyPressEvent(&k);
    QApplication::clipboard()->setText(t);
  }
}

//-----------------------------------------------------------------------------
void KMLineEdit::paste()
{
  QKeyEvent k(QEvent::KeyPress, Key_V, 0, ControlButton);
  keyPressEvent(&k);
}

//-----------------------------------------------------------------------------
void KMLineEdit::markAll()
{
  selectAll();
}

//-----------------------------------------------------------------------------
void KMLineEdit::slotCompletion()
{
  QString t;
  QString Name(name());

  if (Name == "subjectLine")
  {
    //mComposer->focusNextPrevEdit(this,TRUE); //IMHO, it is useless now (sven)

    return;
  }

  QPopupMenu pop;
  int n;

  KMAddrBook adb;
  adb.readConfig();

  if(adb.load() == IO_FatalError)
    return;

  QString s(text());
  QString prevAddr;
  n = s.findRev(',');
  if (n>=0)
  {
    prevAddr = s.left(n+1) + ' ';
    s = s.mid(n+1,255).stripWhiteSpace();
  }
  //s.append("*");
  //QRegExp regexp(s.data(), FALSE, TRUE);

  n=0;
  for (const char *a=adb.first(); a; a=adb.next())
  {
    //t.setStr(a);
    //if (t.contains(regexp))
    if (QString(a).find(s,0,false) >= 0)
    {
      pop.insertItem(a);
      n++;
    }
  }

  if (n > 1)
  {
    int id;
    pop.popup(parentWidget()->mapToGlobal(QPoint(x(), y()+height())));
    pop.setActiveItem(pop.idAt(0));
    id = pop.exec();

    if (id!=-1)
    {
      setText(prevAddr + pop.text(id));
      //mComposer->focusNextPrevEdit(this,TRUE);
    }
  }
  else if (n==1)
  {
    setText(prevAddr + pop.text(pop.idAt(0)));
    //mComposer->focusNextPrevEdit(this,TRUE);
  }

  setFocus();
  cursorAtEnd();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheckConfig()
{

  KWM kwm;
  QTabDialog qtd (this, "tabdialog", true);
  KSpellConfig mKSpellConfig (&qtd);

  qtd.addTab (&mKSpellConfig, "Spellchecker");
  qtd.setCancelButton ();

  kwm.setMiniIcon (qtd.winId(), kapp->miniIcon());

  if (qtd.exec())
    mKSpellConfig.writeGlobalSettings();
}

//=============================================================================
//
//   Class  KMEdit
//
//=============================================================================
KMEdit::KMEdit(QWidget *parent, KMComposeWin* composer,
	       const char *name):
  KMEditInherited(parent, name)
{
  initMetaObject();
  mComposer = composer;
  installEventFilter(this);

#ifndef KRN
  extEditor = false;     // the default is to use ourself
#endif

  mKSpell = NULL;
}


//-----------------------------------------------------------------------------
KMEdit::~KMEdit()
{

  removeEventFilter(this);

  if (mKSpell) delete mKSpell;

}


//-----------------------------------------------------------------------------
QString KMEdit::brokenText() const
{
    QString temp;

    for (int i = 0; i < numLines(); ++i) {
      temp += *getString(i);
      if (i + 1 < numLines())
	temp += '\n';
    }

    return temp;
}

//-----------------------------------------------------------------------------
bool KMEdit::eventFilter(QObject*, QEvent* e)
{
  if (e->type() == QEvent::KeyPress)
  {
    QKeyEvent *k = (QKeyEvent*)e;

#ifndef KRN
    if (extEditor) {
      if (k->key() == Key_Up)
      {
        mComposer->focusNextPrevEdit(0, false); //take me up
        return TRUE;
      }

      QRegExp repFn("\\%f");
      QString sysLine = mExtEditor;
      KTempFile tmpFile;
      QString tmpName = tmpFile.name();

      tmpFile.setAutoDelete(true);

      fprintf(tmpFile.fstream(), "%s", (const char *)text());

      tmpFile.close();
      // replace %f in the system line
      sysLine.replace(repFn, tmpName);
      system((const char *)sysLine);

      setAutoUpdate(false);
      clear();

      // read data back in from file
      insertLine(kFileToString(tmpName, TRUE, FALSE), -1);
//       QFile mailFile(tmpName);

//       while (!mailFile.atEnd()) {
//          QString oneLine;
//          mailFile.readLine(oneLine, 1024);
//          insertLine(oneLine, -1);
//       }

//       setModified(true);
//       mailFile.close();

      setAutoUpdate(true);
      repaint();
      return TRUE;
    } else {
#endif
    if (k->key()==Key_Tab)
    {
      int col, row;
      getCursorPosition(&row, &col);
      insertAt("	", row, col); // insert tab character '\t'
      emit CursorPositionChanged();
      return TRUE;
    }
    // ---sven's Arrow key navigation start ---
    // Key Up in first line takes you to Subject line.
    if (k->key() == Key_Up && currentLine() == 0)
    {
      mComposer->focusNextPrevEdit(0, false); //take me up
      return TRUE;
    }
    // ---sven's Arrow key navigation end ---
#ifndef KRN
    }
#endif
  }
  return FALSE;
}

//-----------------------------------------------------------------------------
void KMEdit::spellcheck()
{
  mKSpell = new KSpell(this, i18n("Spellcheck - KMail"), this,
		       SLOT(slotSpellcheck2(KSpell*)));
  connect (mKSpell, SIGNAL( death()),
          this, SLOT (slotSpellDone()));
  connect (mKSpell, SIGNAL (misspelling (QString, QStringList *, unsigned)),
          this, SLOT (misspelling (QString, QStringList *, unsigned)));
  connect (mKSpell, SIGNAL (corrected (QString, QString, unsigned)),
          this, SLOT (corrected (QString, QString, unsigned)));
  connect (mKSpell, SIGNAL (done(const char *)),
          this, SLOT (slotSpellResult (const char *)));
}


//-----------------------------------------------------------------------------
void KMEdit::slotSpellcheck2(KSpell*)
{
  spellcheck_start();
  //we're going to want to ignore quoted-message lines...
  mKSpell->check(text());
}

//-----------------------------------------------------------------------------
void KMEdit::slotSpellResult(const char *aNewText)
{
  spellcheck_stop();
  if (mKSpell->dlgResult() == 0)
  {
     setText(aNewText);
  }
  mKSpell->cleanUp();
}

//-----------------------------------------------------------------------------
void KMEdit::slotSpellDone()
{
  KSpell::spellStatus status = mKSpell->status();
  delete mKSpell;
  mKSpell = 0;
  if (status == KSpell::Error)
  {
     KMessageBox::sorry(this, i18n("ISpell could not be started.\n Please make sure you have ISpell properly configured and in your PATH."));
  }
  else if (status == KSpell::Crashed)
  {
     spellcheck_stop();
     KMessageBox::sorry(this, i18n("ISpell seems to have crashed."));
  }
  else
  {
     emit spellcheck_done();
  }
}






