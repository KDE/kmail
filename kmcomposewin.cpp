// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#include <qprinter.h>
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
#endif

#include <assert.h>
#include <drag.h>
#include <kapp.h>
#include <kiconloader.h>
#include <kapp.h>
#include <kmenubar.h>
#include <kmsgbox.h>
#include <kstatusbar.h>
#include <ktablistbox.h>
#include <ktoolbar.h>
#include <kstdaccel.h>
#include <mimelib/mimepp.h>
#include <html.h>
#include <kfiledialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlist.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qregexp.h>
#include <qcursor.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#if defined CHARSETS
#include <kcharsets.h>
#include "charsetsDlg.h"
#endif

#ifdef KRN
/* start added for KRN */
#include "krnsender.h"
extern KLocale *nls;
extern KStdAccel* keys;
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

QString KMComposeWin::mPathAttach = 0;

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
  /* start added for KRN */
  ,mEdtNewsgroups(this,&mMainWidget),mEdtFollowupTo(this,&mMainWidget),
  mLblNewsgroups(&mMainWidget),mLblFollowupTo(&mMainWidget)
  /* end added for KRN */
#endif
{

  mGrid = NULL;
  mAtmListBox = NULL;
  mAtmList.setAutoDelete(TRUE);
  mAutoDeleteMsg = FALSE;
  mPathAttach = 0;
  mEditor = NULL;

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

  connect(&mEdtSubject,SIGNAL(textChanged(const char *)),
	  SLOT(slotUpdWinTitle(const char *)));
  connect(&mBtnTo,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
  connect(&mBtnCc,SIGNAL(clicked()),SLOT(slotAddrBookCc()));
  connect(&mBtnBcc,SIGNAL(clicked()),SLOT(slotAddrBookBcc()));
  connect(&mBtnReplyTo,SIGNAL(clicked()),SLOT(slotAddrBookReplyTo()));
  connect(&mBtnFrom,SIGNAL(clicked()),SLOT(slotAddrBookFrom()));

  mDropZone = new KDNDDropZone(mEditor, DndURL);
  connect(mDropZone, SIGNAL(dropAction(KDNDDropZone *)), 
	  SLOT(slotDropAction()));

  mMainWidget.resize(480,510);
  setView(&mMainWidget, FALSE);
  rethinkFields();

#if defined CHARSETS  
  // As family may change with charset, we must save original settings
  mSavedEditorFont=mEditor->font();
#endif

  mMsg = NULL;
  if (aMsg) setMsg(aMsg);

#ifdef HAS_KSPELL
  mKSpellConfig = new KSpellConfig;
  mKSpell = NULL;
#endif

  if (mEdtTo.isVisible())
    mEdtTo.setFocus();
}


//-----------------------------------------------------------------------------
KMComposeWin::~KMComposeWin()
{
  debug("~KMComposeWin()");

  writeConfig();

  if (mAutoDeleteMsg && mMsg) delete mMsg;
#ifdef HAS_KSPELL
  if (mKSpellConfig) delete KSpellConfig;
  if (mKSpell) delete mKSpell;
#endif
  delete mMenuBar;
  delete mToolBar;
}


//-----------------------------------------------------------------------------
void KMComposeWin::readConfig(void)
{
  KConfig *config = kapp->getConfig();
  QString str;
  int w, h;

  config->setGroup("Composer");
  mAutoSign = (stricmp(config->readEntry("signature","manual"),"auto")==0);
  mShowToolBar = config->readNumEntry("show-toolbar", 1);
  mDefEncoding = config->readEntry("encoding", "base64");
  mShowHeaders = config->readNumEntry("headers", HDR_STANDARD);
  mWordWrap = config->readNumEntry("word-wrap", 1);
  mLineBreak = config->readNumEntry("break-at", 80);
  mBackColor = config->readEntry( "Back-Color","#ffffff");
  mForeColor = config->readEntry( "Fore-Color","#000000");
  mAutoPgpSign = config->readNumEntry("pgp-auto-sign", 0);

  config->setGroup("Fonts");
  mBodyFont = config->readEntry("body-font", "helvetica-medium-r-12");
  if (mEditor) mEditor->setFont(kstrToFont(mBodyFont));

#if defined CHARSETS  
  m7BitAscii = config->readNumEntry("7bit-is-ascii",1);
  mQuoteUnknownCharacters = config->readNumEntry("quote-unknown",0);
  
  str = config->readEntry("default-charset", "");
  if (str.isNull() || str=="default" || !KCharset(str).ok())
      mDefaultCharset="default";
  else
      mDefaultCharset=str;
      
  debug("Default charset: %s", (const char*)mDefaultCharset);
      
  str = config->readEntry("composer-charset", "");
  if (str.isNull() || str=="default" || !KCharset(str).isDisplayable())
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
  KConfig *config = kapp->getConfig();
  QString str(32);

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

  config->writeEntry("Fore-Color",mForeColor);
  config->writeEntry("Back-Color",mBackColor);

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
  mMenuBar = new KMenuBar(this);


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
		   SLOT(slotPrint()), keys->print());
  menu->insertSeparator();
  menu->insertItem(i18n("&New Composer..."),this,
                   SLOT(slotNewComposer()), keys->openNew());
#ifndef KRN
  menu->insertItem(i18n("New Mailreader"), this, 
		   SLOT(slotNewMailReader()));
#endif
  menu->insertSeparator();
  menu->insertItem(i18n("&Close"),this,
		   SLOT(slotClose()), keys->close());
  mMenuBar->insertItem(i18n("&File"),menu);

  
  //---------- Menu: Edit
  menu = new QPopupMenu();
  mMenuBar->insertItem(i18n("&Edit"),menu);
#ifdef BROKEN
  menu->insertItem(i18n("Undo"),this,
		   SLOT(slotUndoEvent()), keys->undo());
  menu->insertSeparator();
#endif //BROKEN
  menu->insertItem(i18n("Cut"), this, SLOT(slotCut()));
  menu->insertItem(i18n("Copy"), this, SLOT(slotCopy()));
  menu->insertItem(i18n("Paste"), this, SLOT(slotPaste()));
  menu->insertItem(i18n("Mark all"),this,
		   SLOT(slotMarkAll()));
  menu->insertSeparator();
  menu->insertItem(i18n("Find..."), this,
		   SLOT(slotFind()), keys->find());
  menu->insertItem(i18n("Replace..."), this,
		   SLOT(slotReplace()), keys->replace());

  //---------- Menu: Options
  menu = new QPopupMenu();
  menu->setCheckable(TRUE);

  mMnuIdUrgent = menu->insertItem(i18n("&Urgent"), this,
				  SLOT(slotToggleUrgent()));
  mMnuIdConfDeliver=menu->insertItem(i18n("&Confirm delivery"), this,
				     SLOT(slotToggleConfirmDelivery()));
  mMnuIdConfRead = menu->insertItem(i18n("&Confirm read"), this,
				    SLOT(slotToggleConfirmRead()));
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

  menu = new QPopupMenu();
  menu->insertItem(i18n("Append S&ignature"), this, 
		   SLOT(slotAppendSignature()));
  menu->insertItem(i18n("&Insert File"), this,
		   SLOT(slotInsertFile()));
  menu->insertItem(i18n("&Attach..."), this, SLOT(slotAttachFile()));
  menu->insertSeparator();
  menu->insertItem(i18n("&Remove"), this, SLOT(slotAttachRemove()));
  menu->insertItem(i18n("&Save..."), this, SLOT(slotAttachSave()));
  menu->insertItem(i18n("Pr&operties..."), 
		   this, SLOT(slotAttachProperties()));
  mMenuBar->insertItem(i18n("&Attach"), menu);

  //---------- Menu: Help
  menu = app->getHelpMenu(TRUE, aboutText);
  mMenuBar->insertSeparator();
  mMenuBar->insertItem(i18n("&Help"), menu);

  setMenu(mMenuBar);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupToolBar(void)
{
  KIconLoader* loader = kapp->getIconLoader();

  mToolBar = new KToolBar(this);

  mToolBar->insertButton(loader->loadIcon("send.xpm"),0,
			SIGNAL(clicked()),this,
			SLOT(slotSend()),TRUE,i18n("Send message"));
  mToolBar->insertSeparator();
  mToolBar->insertButton(loader->loadIcon("filenew.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotNewComposer()), TRUE, 
			i18n("Compose new message"));

  mToolBar->insertButton(loader->loadIcon("filefloppy.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotToDo()), TRUE,
			i18n("Save message to file"));

  mToolBar->insertButton(loader->loadIcon("fileprint.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotPrint()), TRUE,
			i18n("Print message"));
  mToolBar->insertSeparator();
#ifdef BROKEN
  mToolBar->insertButton(loader->loadIcon("reload.xpm"),2,
			SIGNAL(clicked()),this,
			SLOT(slotCopyText()),TRUE,"Undo last change");
#endif
  mToolBar->insertButton(loader->loadIcon("editcut.xpm"),4,
			SIGNAL(clicked()),this,
			SLOT(slotCut()),TRUE,i18n("Cut selection"));
  mToolBar->insertButton(loader->loadIcon("editcopy.xpm"),3,
			SIGNAL(clicked()),this,
			SLOT(slotCopy()),TRUE,i18n("Copy selection"));
  mToolBar->insertButton(loader->loadIcon("editpaste.xpm"),5,
			SIGNAL(clicked()),this,
			SLOT(slotPaste()),TRUE,i18n("Paste clipboard contents"));
  mToolBar->insertSeparator();

  mToolBar->insertButton(loader->loadIcon("attach.xpm"),8,
			 SIGNAL(clicked()),this,
			 SLOT(slotAttachFile()),TRUE,i18n("Attach file"));
  mToolBar->insertButton(loader->loadIcon("openbook.xpm"),7,
			 SIGNAL(clicked()),this,
			 SLOT(slotAddrBook()),TRUE,
			 i18n("Open addressbook..."));
#ifdef HAS_KSPELL
  mToolBar->insertButton(loader->loadIcon("spellcheck.xpm"),7,
			SIGNAL(clicked()),this,
			SLOT(slotSpellcheck()),TRUE,"Spellcheck message");
#endif
  mToolBar->insertSeparator();
  mBtnIdSign = 9;
  mToolBar->insertButton(loader->loadIcon("feather_white.xpm"), mBtnIdSign,
			 TRUE, i18n("sign message"));
  mToolBar->setToggle(mBtnIdSign);
  mToolBar->setButton(mBtnIdSign, mAutoPgpSign);
  mBtnIdEncrypt = 10;
  mToolBar->insertButton(loader->loadIcon("pub_key_red.xpm"), mBtnIdEncrypt,
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
  mStatusBar->setInsertOrder(KStatusBar::RightToLeft);
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
  temp.sprintf("%s: %i", i18n("Line"), (line+1));
  mStatusBar->changeItem(temp,1);
  temp.sprintf("%s: %i", i18n("Column"), (col+1));
  mStatusBar->changeItem(temp,2);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupEditor(void)
{
  QPopupMenu* menu;
  mEditor = new KMEdit(kapp, &mMainWidget, this);
  mEditor->toggleModified(FALSE);
  //mEditor->setFocusPolicy(QWidget::ClickFocus);

  // Word wrapping setup
  mEditor->setWordWrap(mWordWrap);
  if (mWordWrap && (mLineBreak > 0)) 
    mEditor->setFillColumnMode(mLineBreak,TRUE);
  else mEditor->setFillColumnMode(0,FALSE);

  // Font setup
  mEditor->setFont(kstrToFont(mBodyFont));

  // Color setup
  if (mForeColor.isEmpty()) mForeColor = "black";
  if (mBackColor.isEmpty()) mBackColor = "white";

  foreColor.setNamedColor(mForeColor);
  backColor.setNamedColor(mBackColor);

  QPalette myPalette = (mEditor->palette()).copy();
  QColorGroup cgrp  = myPalette.normal();
  QColorGroup ncgrp(foreColor,cgrp.background(),
		    cgrp.light(),cgrp.dark(), cgrp.mid(), foreColor,
		    backColor);
  myPalette.setNormal(ncgrp);
  myPalette.setDisabled(ncgrp);
  myPalette.setActive(ncgrp);

  mEditor->setPalette(myPalette);
  mEditor->setBackgroundColor(backColor);
  

  menu = new QPopupMenu();
#ifdef BROKEN
  menu->insertItem(i18n("Undo"),this,
		   SLOT(slotUndoEvent()), keys->undo());
  menu->insertSeparator();
#endif //BROKEN
  menu->insertItem(i18n("Cut"), this, SLOT(slotCut()));
  menu->insertItem(i18n("Copy"), this, SLOT(slotCopy()));
  menu->insertItem(i18n("Paste"), this, SLOT(slotPaste()));
  menu->insertItem(i18n("Mark all"),this, SLOT(slotMarkAll()));
  menu->insertSeparator();
  menu->insertItem(i18n("Find..."), this, SLOT(slotFind()));
  menu->insertItem(i18n("Replace..."), this, SLOT(slotReplace()));
  mEditor->installRBPopup(menu);
  connect(mEditor,SIGNAL(CursorPositionChanged()),SLOT(updateCursorPosition()));
  updateCursorPosition();
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
  mEdtNewsgroups.setText(mMsg->groups());
  mEdtFollowupTo.setText(mMsg->followup());

  num = mMsg->numBodyParts();
  if (num > 0)
  {
    mMsg->bodyPart(0, &bodyPart);

#if defined CHARSETS    
    mCharset=bodyPart.charset();
    cout<<"Charset: "<<mCharset<<"\n";
    if (mCharset==""){
      mComposeCharset=mDefComposeCharset;
      if (mComposeCharset=="default"
                        && KCharset(mDefaultCharset).isDisplayable())
        mComposeCharset=mDefaultCharset;
    }	
    else if (KCharset(mCharset).isDisplayable()) mComposeCharset=mCharset;
    else mComposeCharset=mDefComposeCharset;
    cout<<"Compose charset: "<<mComposeCharset<<"\n";
    mEditor->setText(convertToLocal(bodyPart.bodyDecoded()));
#else   
    mEditor->setText(bodyPart.bodyDecoded());
#endif
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
                        && KCharset(mDefaultCharset).isDisplayable())
        mComposeCharset=mDefaultCharset;
    }	
    else if (KCharset(mCharset).isDisplayable()) mComposeCharset=mCharset;
    else mComposeCharset=mDefComposeCharset;
    cout<<"Compose charset: "<<mComposeCharset<<"\n";
    mEditor->setText(convertToLocal(mMsg->bodyDecoded()));
  }  
#else  
  else mEditor->setText(mMsg->bodyDecoded());
#endif

  if (mAutoSign && mayAutoSign) slotAppendSignature();
  mEditor->toggleModified(FALSE);
 
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
  mMsg->setFollowup(followupTo());
  mMsg->setGroups(newsgroups());

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
#if defined CHARSETS      
      str=convertToSend(pgpProcessedMsg());
      if (str.isNull()) return FALSE;
      cout<<"Setting charset to: "<<mCharset<<"\n";
      mMsg->setCharset(mCharset);
      mMsg->setBodyEncoded(str);
#else
      str = pgpProcessedMsg();
      if (str.isNull()) return FALSE;
      mMsg->setBodyEncoded(str);
#endif      
    }
    else
    {
      mMsg->setTypeStr("text");
      mMsg->setSubtypeStr("plain");
      mMsg->setCteStr("8bit");
#if defined CHARSETS      
      str=convertToSend(pgpProcessedMsg());
      if (str.isNull()) return FALSE;
      cout<<"Setting charset to: "<<mCharset<<"\n";
      mMsg->setCharset(mCharset);
      mMsg->setBody(str);
#else      
      str = pgpProcessedMsg();
      if (str.isNull()) return FALSE;
      mMsg->setBody(str);
#endif      
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
    str=convertToSend(str);
    cout<<"Setting charset to: "<<mCharset<<"\n";
    mMsg->setCharset(mCharset);
#endif      
    str.truncate(str.length()); // to ensure str.size()==str.length()+1
    bodyPart.setBodyEncoded(str);
    mMsg->addBodyPart(&bodyPart);

    // Since there is at least one more attachment create another bodypart
    for (msgPart=mAtmList.first(); msgPart; msgPart=mAtmList.next())
	mMsg->addBodyPart(msgPart);
  }
  if (!mAutoDeleteMsg) mEditor->toggleModified(FALSE);

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
    rc = KMsgBox::yesNo(0,i18n("KMail Confirm"),
			i18n("Close and discard\nedited message?"));
    if (rc != 1)
    {
      e->ignore();
      return;
    }
  }
  KMComposeWinInherited::closeEvent(e);
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
  
  if (!doSign && !doEncrypt) return mEditor->text();

  pgp->setMessage(mEditor->text());

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

  return 0;
}

 
//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(const QString aUrl)
{
  QString str, name;
  KMMessagePart* msgPart;
  KMMsgPartDlg dlg;
  int i;

  // load the file
  kbp->busy();
  str = kFileToString(aUrl,FALSE);
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
const QString KMComposeWin::msgPartLbxString(const KMMessagePart* msgPart) const
{
  unsigned int len;
  QString lenStr(32);

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
    if (txt.right(1)!=',') txt += ", ";
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
  QString fileName;

  if (mPathAttach.isEmpty()) mPathAttach = QDir::currentDirPath();
  
  KFileDialog fdlg(mPathAttach, "*", this, NULL, TRUE);

  fdlg.setCaption(i18n("Attach File"));
  if (!fdlg.exec()) return;

  mPathAttach = fdlg.dirPath().copy();

  fileName = fdlg.selectedFile();
  if(fileName.isEmpty()) return;

  addAttach(fileName);
  mEditor->toggleModified(TRUE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertFile()
{
  QString fileName, str;
  int col, line;
  
  if (mPathAttach.isEmpty()) mPathAttach = QDir::currentDirPath();

  KFileDialog fdlg(mPathAttach, "*", this, NULL, TRUE);
  fdlg.setCaption(i18n("Include File"));
  if (!fdlg.exec()) return;

  mPathAttach = fdlg.dirPath().copy();

  fileName = fdlg.selectedFile();
  if (fileName.isEmpty()) return;

  str = kFileToString(fileName, TRUE, TRUE);
  if (str.isEmpty()) return;

  mEditor->getCursorPosition(&line, &col);
  mEditor->insertAt(str, line, col);
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
  str = msgPart->bodyDecoded();

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

  fileName = KFileDialog::getSaveFileName(mPathAttach, "*", NULL, pname);
  if (fileName.isEmpty()) return;

  kStringToFile(msgPart->bodyDecoded(), fileName, TRUE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachRemove()
{
  int idx = mAtmListBox->currentItem();
  if (idx >= 0)
  {
    removeAttach(idx);
    mEditor->toggleModified(TRUE);
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
  mEditor->Search();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotReplace()
{
  mEditor->Replace();
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

  QKeyEvent k(Event_KeyPress, Key_C , 0 , ControlButton);
  app->notify(fw, &k);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotPaste()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  QKeyEvent k(Event_KeyPress, Key_V , 0 , ControlButton);
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
void KMComposeWin::slotUpdWinTitle(const char *text)
{
  if (!text || *text=='\0')
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
    str += mEditor->text();
    str += "\n";
    //str.replace(QRegExp("\n"),"\n");
    paint.drawText(30,30,str);
    paint.end();
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotDropAction()
{
  QString  file;
  QStrList *fileList;

  fileList = &mDropZone->getURLList();

  for (file=fileList->first(); !file.isEmpty() ; file=fileList->next())
  {
    file.replace("file:","");
    addAttach(file);
  }
}


//----------------------------------------------------------------------------
void KMComposeWin::doSend(int aSendNow)
{
  bool sentOk;

  kbp->busy();
  //applyChanges();  // is called twice otherwise. Lars
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
    KFileDialog dlg(getenv("HOME"),0,this,0,TRUE,FALSE);
    dlg.setCaption(i18n("Choose Signature File"));
    if (!dlg.exec()) return;
    sigFileName = dlg.selectedFile();
    if (sigFileName.isEmpty()) return;
    sigText = kFileToString(sigFileName, TRUE);
    identity->setSignatureFile(sigFileName);
    identity->writeConfig(true);
  }
  else sigText = identity->signature();

  if (!sigText.isEmpty())
  {
    mEditor->insertLine("--", -1);
    mEditor->insertLine(sigText, -1);
    mEditor->toggleModified(mod);
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotHelp()
{
  app->invokeHTMLHelp("","");
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheck()
{
#ifdef HAS_KSPELL
  if (mKSpell) return;

  mKSpell = new KSpell(this, i18n("KMail: Spellcheck"), this,
		       SLOT(slotSpellcheck2(KSpell*)));
#endif //HAS_KSPELL
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheck2(KSpell*)
{
#ifdef HAS_KSPELL
  if (mKSpell->isOk())
  {
    setReadOnly (TRUE);
    
    connect (mKSpell, SIGNAL (mispelling (char *, QStrList *, long)),
	     this, SLOT (mispelling (char *, QStrList *, long)));
    connect (mKSpell, SIGNAL (corrected (char *, char *, long)),
	     this, SLOT (corrected (char *, char *, long)));
    connect (mKSpell, SIGNAL (done(char *)), 
	     this, SLOT (spellResult (char *)));
    mKSpell->check (text().data());
  }
  else
  {
    warning(i18n("Error starting KSpell. Please make sure you have ISpell properly configured."));
  }
#endif //HAS_KSPELL
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellResult(char *aNewText)
{
  // prevent warning
  (void)aNewText;

#ifdef HAS_KSPELL
  mEditor->setText(aNewText);
  mEditor->setReadOnly(FALSE);
  delete mKSpell;
  mKSpell = NULL; 
#endif //HAS_KSPELL
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellCorrected(char *originalword, 
				      char *newword, long pos)
{
  // prevent warning
  (void)originalword;
  (void)newword;
  (void)pos;

#ifdef HAS_KSPELL
  //we'll reselect the original word in case the user has played with
  //the selection in eframe or the word was auto-replaced

  int line, cnt=0, l;

  if(strcmp(newword,originalword)!=0)
  {
    for (line=0;line<numLines() && cnt<=pos;line++)
      cnt+=lineLength(line)+1;
    line--;

    cnt=pos-cnt+ lineLength(line)+1;

    //remove old word
    setCursorPosition (line, cnt);
    for(l = 0 ; l < (int)strlen(originalword); l++)
      cursorRight(TRUE);
    ///      setCursorPosition (line,
    //       cnt+strlen(originalword),TRUE);
    cut();

    insertAt (newword, line, cnt);
  }

  deselect();
#endif //HAS_KSPELL
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellMispelling(char *word, QStrList *, long pos)
{
  // prevent warning
  (void)word;
  (void)pos;

#ifdef HAS_KSPELL
  int l, cnt=0;

  for (l=0;l<numLines() && cnt<=pos;l++)
    cnt+=strlen (textLine(l))+1;
  l--;

  cnt=pos-cnt+strlen (textLine (l))+1;

  setCursorPosition (l, cnt);

  //According to the Qt docs this could be done more quickly with
  //setCursorPosition (l, cnt+strlen(word), TRUE);
  //but that doesn't seem to work.
  for(l = 0 ; l < (int)strlen(word); l++)
    cursorRight(TRUE);
  
  if (cursorPoint().y()>height()/2)
    mKSpell->moveDlg(10, height()/2 - mKSpell->heightDlg()-15);
  else
    mKSpell->moveDlg(10, height()/2 + 15);

  //  setCursorPosition (line, cnt+strlen(word),TRUE);
#endif //HAS_KSPELL
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotConfigureCharsets()
{
#if defined CHARSETS
   CharsetsDlg *dlg=new CharsetsDlg((const char*)mCharset,
				    (const char*)mComposeCharset,
                                    m7BitAscii,mQuoteUnknownCharacters);
   connect(dlg,SIGNAL( setCharsets(const char *,const char *,bool,bool,bool) )
           ,this,SLOT(slotSetCharsets(const char *,const char *,bool,bool,bool)));
   dlg->show();
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
  if (composer!=mComposeCharset && quote)
     transcodeMessageTo(composer);
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
QString KMComposeWin::convertToLocal(const QString str)
{
  if (m7BitAscii && !is8Bit(str)) return str.copy();
  KCharset destCharset;
  KCharset srcCharset;
  if (mCharset=="")
  {
     if (mDefaultCharset=="default") mCharset=klocale->charset();
     else mCharset=mDefaultCharset;
  }   
  srcCharset=mCharset; 
  if (mComposeCharset=="default") destCharset=klocale->charset();
  else destCharset=mComposeCharset;
  if (srcCharset==destCharset) return str.copy();
  int flags=mQuoteUnknownCharacters?KCharsetConverter::OUTPUT_AMP_SEQUENCES:0;
  KCharsetConverter conv(srcCharset,destCharset,flags);
  KCharsetConversionResult result=conv.convert(str);
  return result.copy();			  
}

//-----------------------------------------------------------------------------
QString KMComposeWin::convertToSend(const QString str)
{
  cout<<"Converting to send...\n";
  if (m7BitAscii && !is8Bit(str))
  { 
    mCharset="us-ascii";
    return str.copy();
  }
  if (mCharset=="")
  {
     if (mDefaultCharset=="default") 
          mCharset=klocale->charset();
     else mCharset=mDefaultCharset;
  }   
  cout<<"mCharset: "<<mCharset<<"\n";
  KCharset srcCharset;
  KCharset destCharset(mCharset);
  if (mComposeCharset=="default") srcCharset=klocale->charset();
  else srcCharset=mComposeCharset;
  cout<<"srcCharset: "<<srcCharset<<"\n";
  if (srcCharset==destCharset) return str.copy();
  int flags=mQuoteUnknownCharacters?KCharsetConverter::INPUT_AMP_SEQUENCES:0;
  KCharsetConverter conv(srcCharset,destCharset,flags);
  KCharsetConversionResult result=conv.convert(str);
  return result.copy();			  
}

//-----------------------------------------------------------------------------
void KMComposeWin::transcodeMessageTo(const QString charset)
{

  cout<<"Transcoding message...\n";
  QString inputStr=mEditor->text();
  KCharset srcCharset;
  KCharset destCharset(charset);
  if (mComposeCharset=="default") srcCharset=klocale->charset();
  else srcCharset=mComposeCharset;
  cout<<"srcCharset: "<<srcCharset<<"\n";
  if (srcCharset==destCharset) return;
  int flags=mQuoteUnknownCharacters?KCharsetConverter::AMP_SEQUENCES:0;
  KCharsetConverter conv(srcCharset,destCharset,flags);
  KCharsetConversionResult result=conv.convert(inputStr);
  mComposeCharset=charset;
  mEditor->setText(result.copy());			  
}


//-----------------------------------------------------------------------------
void KMComposeWin::setEditCharset()
{
  QFont fnt=mSavedEditorFont;
  KCharset kcharset;
  if (mComposeCharset=="default") kcharset=klocale->charset();
  else if (mComposeCharset!="") kcharset=mComposeCharset;
  cout<<"Setting font to: "<<kcharset.name()<<"\n";
  mEditor->setFont(kcharset.setQFont(fnt));
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
  else mEditor->setFocus();
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
  if (e->type() == Event_KeyPress)
  {
    QKeyEvent* k = (QKeyEvent*)e;

    if (k->state()==ControlButton && k->key()==Key_T)
    {
      emit completion();
      cursorAtEnd();
      return TRUE;
    }
  }
  else if (e->type() == Event_MouseButtonPress)
  {
    QMouseEvent* me = (QMouseEvent*)e;
    if (me->button() == MidButton)
    {
      setFocus();
      QKeyEvent ev(Event_KeyPress, Key_V, 0 , ControlButton);
      keyPressEvent(&ev);
      return TRUE;
    }
    else if (me->button() == RightButton)
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
  QKeyEvent ev( Event_KeyPress, Key_End, 0, 0 );
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
    QKeyEvent k(Event_KeyPress, Key_D, 0, ControlButton);
    keyPressEvent(&k);
    QApplication::clipboard()->setText(t);
  }
}

//-----------------------------------------------------------------------------
void KMLineEdit::paste()
{
  QKeyEvent k(Event_KeyPress, Key_V, 0, ControlButton);
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
    mComposer->focusNextPrevEdit(this,TRUE);
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
  s.append("*");
  QRegExp regexp(s.data(), FALSE, TRUE);
  
  n=0;
  
  for (const char *a=adb.first(); a; a=adb.next())
  {
    t.setStr(a);
    if (t.contains(regexp))
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



//=============================================================================
//
//   Class  KMEdit
//
//=============================================================================
KMEdit::KMEdit(KApplication *a,QWidget *parent, KMComposeWin* composer,
	       const char *name, const char *filename):
  KMEditInherited(a, parent, name, filename)
{
  initMetaObject();
  mComposer = composer;
  installEventFilter(this);
}


//-----------------------------------------------------------------------------
KMEdit::~KMEdit()
{
  removeEventFilter(this);
}


//-----------------------------------------------------------------------------
bool KMEdit::eventFilter(QObject*, QEvent* e)
{
  if (e->type() == Event_KeyPress)
  {
    QKeyEvent *k = (QKeyEvent*)e;

    if (k->key()==Key_Tab)
    {
      int col, row;
      getCursorPosition(&row, &col);
      insertAt("	", row, col); // insert tab character '\t'
      return TRUE;
    }
  }
  return FALSE;
}
