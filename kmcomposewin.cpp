// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

#include "kmcomposewin.h"
#include "keditcl.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmsender.h"
#include "kmidentity.h"
#include "kfileio.h"
#include "kbusyptr.h"
#include "kmmsgpartdlg.h"
#include "kpgp.h"

#include <assert.h>
#include <drag.h>
#include <kapp.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmsgbox.h>
#include <kstatusbar.h>
#include <ktablistbox.h>
#include <ktoolbar.h>
#include <kstdaccel.h>
#include <mimelib/mimepp.h>
#include <html.h>
#include <qfiledlg.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlist.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qprinter.h>
#include <qregexp.h>
#include <qcursor.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef KRN
/* start added for KRN */
#include "krnsender.h"
extern KLocale *nls;
extern KStdAccel* keys;
extern KApplication *app;
extern KBusyPtr *kbp;
extern KRNSender *msgSender;
extern KMIdentity *identity;
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

//-----------------------------------------------------------------------------
KMComposeWin::KMComposeWin(KMMessage *aMsg) : KMComposeWinInherited(),
  mMainWidget(this), 
  mEdtFrom(&mMainWidget), mEdtReplyTo(&mMainWidget), mEdtTo(&mMainWidget),
  mEdtCc(&mMainWidget), mEdtBcc(&mMainWidget), mEdtSubject(&mMainWidget),
  mLblFrom(&mMainWidget), mLblReplyTo(&mMainWidget), mLblTo(&mMainWidget),
  mLblCc(&mMainWidget), mLblBcc(&mMainWidget), mLblSubject(&mMainWidget)
    /* start added for KRN */
    ,mEdtNewsgroups(&mMainWidget),mEdtFollowupTo(&mMainWidget),
    mLblNewsgroups(&mMainWidget),mLblFollowupTo(&mMainWidget)
    /* end added for KRN */
{

  mGrid = NULL;
  mAtmListBox = NULL;
  mAtmList.setAutoDelete(TRUE);
  mAutoDeleteMsg = FALSE;

  setCaption(nls->translate("KMail Composer"));
  setMinimumSize(200,200);

  mAtmListBox = new KTabListBox(&mMainWidget, NULL, 5);
  mAtmListBox->setColumn(0, nls->translate("F"),16, KTabListBox::PixmapColumn);
  mAtmListBox->setColumn(1, nls->translate("Name"), 200);
  mAtmListBox->setColumn(2, nls->translate("Size"), 80);
  mAtmListBox->setColumn(3, nls->translate("Encoding"), 120);
  mAtmListBox->setColumn(4, nls->translate("Type"), 150);
  connect(mAtmListBox,SIGNAL(popupMenu(int,int)),
	  SLOT(slotAttachPopupMenu(int,int)));

  readConfig();

  setupEditor();
  setupMenuBar();
  setupToolBar();
  setupStatusBar();

  if(!mShowToolBar) enableToolBar(KToolBar::Hide);	

  connect(&mEdtSubject,SIGNAL(textChanged(const char *)),
	  SLOT(slotUpdWinTitle(const char *)));

  mDropZone = new KDNDDropZone(mEditor, DndURL);
  connect(mDropZone, SIGNAL(dropAction(KDNDDropZone *)), 
	  SLOT(slotDropAction()));

  mMainWidget.resize(480,510);
  setView(&mMainWidget, FALSE);
  rethinkFields();

  mMsg = NULL;
  if (aMsg) { 
    setMsg(aMsg);
    if(mEditor->text().isEmpty())
      mEditor->toggleModified(FALSE);
  }

  if ( mAutoSign ) 
    slotAppendSignature();

}


//-----------------------------------------------------------------------------
KMComposeWin::~KMComposeWin()
{
  if (mAutoDeleteMsg && mMsg) delete mMsg;
}


//-----------------------------------------------------------------------------
void KMComposeWin::readConfig(void)
{
  KConfig *config = kapp->getConfig();
  QString str;
  int w, h;

  config->setGroup("Composer");
  mAutoSign = (stricmp(config->readEntry("signature"),"auto")==0);
  cout << "auto:" << mAutoSign << endl;
  mShowToolBar = config->readNumEntry("show-toolbar", 1);
  mSendImmediate = config->readNumEntry("send-immediate", -1);
  mDefEncoding = config->readEntry("encoding", "base64");
  mShowHeaders = config->readNumEntry("headers", HDR_STANDARD);
  mWordWrap = config->readNumEntry("word-wrap", 1);
  mLineBreak = config->readNumEntry("break-at", 80);
  mBackColor = config->readEntry( "Back-Color","#ffffff");
  mForeColor = config->readEntry( "Fore-Color","#000000");
  mAutoPgpSign = config->readNumEntry("pgp-auto-sign", 0);

  config->setGroup("Geometry");
  str = config->readEntry("composer", "480 510");
  sscanf(str.data(), "%d %d", &w, &h);
  if (w<200) w=200;
  if (h<200) h=200;
  resize(w, h);
}	


//-----------------------------------------------------------------------------
void KMComposeWin::writeConfig(bool aWithSync)
{
  KConfig *config = kapp->getConfig();
  QString str(32);

  config->setGroup("Composer");
  config->writeEntry("signature", mAutoSign?"auto":"manual");
  config->writeEntry("show-toolbar", mShowToolBar);
  config->writeEntry("send-immediate", mSendImmediate);
  config->writeEntry("encoding", mDefEncoding);
  config->writeEntry("headers", mShowHeaders);

  config->writeEntry("Fore-Color",mForeColor);
  config->writeEntry("Back-Color",mBackColor);

  config->setGroup("Geometry");
  str.sprintf("%d %d", width(), height());
  config->writeEntry("composer", str);

  if (aWithSync) config->sync();
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

  row = 0;
  rethinkHeaderLine(showHeaders,HDR_FROM, row, nls->translate("&From:"),
		    &mLblFrom, &mEdtFrom);
  rethinkHeaderLine(showHeaders,HDR_REPLY_TO,row,nls->translate("&Reply to:"),
		    &mLblReplyTo, &mEdtReplyTo);
  rethinkHeaderLine(showHeaders,HDR_TO, row, nls->translate("&To:"),
		    &mLblTo, &mEdtTo);
  rethinkHeaderLine(showHeaders,HDR_CC, row, nls->translate("&Cc:"),
		    &mLblCc, &mEdtCc);
  rethinkHeaderLine(showHeaders,HDR_BCC, row, nls->translate("&Bcc:"),
		    &mLblBcc, &mEdtBcc);
  rethinkHeaderLine(showHeaders,HDR_SUBJECT, row, nls->translate("&Subject:"),
		    &mLblSubject, &mEdtSubject);
  rethinkHeaderLine(showHeaders,HDR_NEWSGROUPS, row, nls->translate("&Newsgroups:"),
		    &mLblNewsgroups, &mEdtNewsgroups);
  rethinkHeaderLine(showHeaders,HDR_FOLLOWUP_TO, row, nls->translate("&Followup-To:"),
		    &mLblFollowupTo, &mEdtFollowupTo);
  assert(row<=mNumHeaders);

  mGrid->addMultiCellWidget(mEditor, row, mNumHeaders, 0, 2);
  mGrid->addMultiCellWidget(mAtmListBox, mNumHeaders+1, mNumHeaders+1, 0, 2);

  if (mAtmList.count() > 0) mAtmListBox->show();
  else mAtmListBox->hide();

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
    aEdt->setFocusPolicy(QWidget::StrongFocus);

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
    aEdt->setFocusPolicy(QWidget::NoFocus);
    if (aBtn) aBtn->hide();
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupMenuBar(void)
{
  QPopupMenu *menu;
  mMenuBar = new KMenuBar(this);


  //---------- Menu: File
  menu = new QPopupMenu();
  menu->insertItem(nls->translate("Send"),this, SLOT(slotSend()));
  menu->insertItem(nls->translate("Send &later"),this, SLOT(slotSendLater()));
  menu->insertSeparator();
  menu->insertItem(nls->translate("Address &Book..."),this,
		   SLOT(slotToDo()), ALT+Key_B);
  menu->insertItem(nls->translate("&Print..."),this, 
		   SLOT(slotPrint()), keys->print());
  menu->insertSeparator();
  menu->insertItem(nls->translate("New &Composer"),this,
		   SLOT(slotNewComposer()), keys->openNew());
  menu->insertSeparator();
  menu->insertItem(nls->translate("&Close"),this,
		   SLOT(slotClose()), keys->close());
  mMenuBar->insertItem(nls->translate("&File"),menu);

  
  //---------- Menu: Edit
  menu = new QPopupMenu();
  mMenuBar->insertItem(nls->translate("&Edit"),menu);
#ifdef BROKEN
  menu->insertItem(nls->translate("Undo"),this,
		   SLOT(slotUndoEvent()), keys->undo());
  menu->insertSeparator();
  menu->insertItem(nls->translate("Cut"), mEditor,
		   SLOT(cutText()), keys->cut());
  menu->insertItem(nls->translate("Copy"), mEditor,
		   SLOT(copyText()), keys->copy());
  menu->insertItem(nls->translate("Paste"), mEditor,
		   SLOT(pasteText()), keys->paste());
  menu->insertItem(nls->translate("Mark all"),this,
		   SLOT(slotMarkAll()), CTRL + Key_A);
  menu->insertSeparator();
  menu->insertItem(nls->translate("Find..."), mEditor,
		   SLOT(search()), keys->find());

  menu = new QPopupMenu();
  menu->insertItem(nls->translate("Recip&ients..."),this,
		    SLOT(slotToDo()));
  menu->insertItem(nls->translate("Insert &File"), this,
		    SLOT(slotInsertFile()));
  menu->insertSeparator();

  subMenu = new QPopupMenu();
  subMenu->insertItem(nls->translate("High"));
  subMenu->insertItem(nls->translate("Normal"));
  subMenu->insertItem(nls->translate("Low"));
  menu->insertItem(nls->translate("Priority"),subMenu);
  mMenuBar->insertItem(nls->translate("&Message"),menu);
#endif //BROKEN

  //---------- Menu: View
  menu = new QPopupMenu();
  mMnuView = menu;
  menu->setCheckable(TRUE);
  connect(menu, SIGNAL(activated(int)), 
	  this, SLOT(slotMenuViewActivated(int)));

  menu->insertItem(nls->translate("&All Fields"), 0);
  menu->insertSeparator();
  menu->insertItem(nls->translate("&From"), HDR_FROM);
  menu->insertItem(nls->translate("&Reply to"), HDR_REPLY_TO);
  menu->insertItem(nls->translate("&To"), HDR_TO);
  menu->insertItem(nls->translate("&Cc"), HDR_CC);
  menu->insertItem(nls->translate("&Bcc"), HDR_BCC);
  menu->insertItem(nls->translate("&Subject"), HDR_SUBJECT);
  menu->insertItem(nls->translate("&Newsgroups"), HDR_NEWSGROUPS); // for KRN
  menu->insertItem(nls->translate("&Followup-To"), HDR_FOLLOWUP_TO); // for KRN
  mMenuBar->insertItem(nls->translate("&View"), menu);

  menu = new QPopupMenu();
  menu->insertItem(nls->translate("Append S&ignature"), this, 
		   SLOT(slotAppendSignature()));
  menu->insertItem(nls->translate("&Insert File"), this,
		   SLOT(slotInsertFile()));
  menu->insertItem(nls->translate("&Attach..."), this, SLOT(slotAttachFile()));
  menu->insertSeparator();
  menu->insertItem(nls->translate("&Remove"), this, SLOT(slotAttachRemove()));
  menu->insertItem(nls->translate("&Save..."), this, SLOT(slotAttachSave()));
  menu->insertItem(nls->translate("Pr&operties..."), 
		   this, SLOT(slotAttachProperties()));
  mMenuBar->insertItem(nls->translate("&Attach"), menu);

  //---------- Menu: Help
  menu = app->getHelpMenu(TRUE, aboutText);
  mMenuBar->insertSeparator();
  mMenuBar->insertItem(nls->translate("&Help"), menu);

  setMenu(mMenuBar);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupToolBar(void)
{
  KIconLoader* loader = kapp->getIconLoader();

  mToolBar = new KToolBar(this);

  mToolBar->insertButton(loader->loadIcon("send.xpm"),0,
			SIGNAL(clicked()),this,
			SLOT(slotSend()),TRUE,"Send message");
  mToolBar->insertSeparator();
  mToolBar->insertButton(loader->loadIcon("filenew.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotNewComposer()), TRUE, 
			nls->translate("Compose new message"));

  mToolBar->insertButton(loader->loadIcon("filefloppy.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotToDo()), TRUE,
			nls->translate("Save message to file"));

  mToolBar->insertButton(loader->loadIcon("fileprint.xpm"), 0, 
			SIGNAL(clicked()), this,
			SLOT(slotPrint()), TRUE,
			nls->translate("Print message"));
  mToolBar->insertSeparator();
#ifdef BROKEN
  mToolBar->insertButton(loader->loadIcon("reload.xpm"),2,
			SIGNAL(clicked()),this,
			SLOT(slotCopyText()),TRUE,"Undo last change");
  mToolBar->insertButton(loader->loadIcon("editcopy.xpm"),3,
			SIGNAL(clicked()),this,
			SLOT(slotCopyText()),TRUE,"Copy selection");
  mToolBar->insertButton(loader->loadIcon("editcut.xpm"),4,
			SIGNAL(clicked()),this,
			SLOT(slotCutText()),TRUE,"Cut selection");
  mToolBar->insertButton(loader->loadIcon("editpaste.xpm"),5,
			SIGNAL(clicked()),this,
			SLOT(slotPasteText()),TRUE,"Paste selection");
  mToolBar->insertSeparator();
#endif //BROKEN
  mToolBar->insertButton(loader->loadIcon("attach.xpm"),8,
			SIGNAL(clicked()),this,
			SLOT(slotAttachFile()),TRUE,"Attach file");
  mToolBar->insertButton(loader->loadIcon("openbook.xpm"),7,
			SIGNAL(clicked()),this,
			SLOT(slotToDo()),TRUE,"Open addressbook");
  mToolBar->insertSeparator();
  mBtnIdSign = 9;
  mToolBar->insertButton(loader->loadIcon("feather_white.xpm"), mBtnIdSign,
			 TRUE, nls->translate("sign message"));
  mToolBar->setToggle(mBtnIdSign);
  mToolBar->setButton(mBtnIdSign,mShowToolBar);
  mBtnIdEncrypt = 10;
  mToolBar->insertButton(loader->loadIcon("pub_key_red.xpm"), mBtnIdEncrypt,
			 TRUE, nls->translate("encrypt message"));
  mToolBar->setToggle(mBtnIdEncrypt);

  addToolBar(mToolBar);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupStatusBar(void)
{
  mStatusBar = new KStatusBar(this);
  mStatusBar->insertItem("Status:",0);
  setStatusBar(mStatusBar);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupEditor(void)
{
  //QPopupMenu* popup;
  mEditor = new KEdit(kapp, &mMainWidget);
  mEditor->toggleModified(FALSE);

  // Word wrapping setup
  if(mWordWrap) 
    mEditor->setWordWrap(TRUE);
  else {
    mEditor->setWordWrap(FALSE);
    mEditor->setFillColumnMode(0,FALSE);    
  }
  if(mWordWrap && (mLineBreak > 0)) 
    mEditor->setFillColumnMode(mLineBreak,TRUE);


  // Font setup


  // Color setup
  if( mForeColor.isEmpty())
    mForeColor = "black";

  if( mBackColor.isEmpty())
    mBackColor = "white";

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
  

#ifdef BROKEN
  popup = new QPopupMenu();
  popup->insertItem (klocale->translate("Send now"), 
		     this,SLOT(slotSendNow()));
  popup->insertItem(klocale->translate("Send later"), 
		    this, SLOT(slotSendLater()));
  popup->insertSeparator(-1);
  popup->insertItem(klocale->translate("Cut"), 
		    mEditor, SLOT(cutText()));
  popup->insertItem(klocale->translate("Copy"), 
		    mEditor, SLOT(copyText()));
  popup->insertItem(klocale->translate("Paste"),
		    mEditor, SLOT(pasteText()));
  popup->insertItem(klocale->translate("Mark All"), 
		    mEditor, SLOT(markAll()));
  popup->insertSeparator(-1);
  popup->insertItem(klocale->translate("Font"), 
		    mEditor, SLOT(selectFont()));
  mEditor->installRBPopup(popup);
#endif //BROKEN
}


//-----------------------------------------------------------------------------
void KMComposeWin::setMsg(KMMessage* newMsg)
{
  KMMessagePart bodyPart, *msgPart;
  int i, num;

  assert(newMsg!=NULL);
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
    mEditor->setText(bodyPart.body());
    mEditor->insertLine("\n", -1);

    for(i=1; i<num; i++)
    {
      msgPart = new KMMessagePart;
      mMsg->bodyPart(i, msgPart);
      addAttach(msgPart);
    }
  }
  else mEditor->setText(mMsg->body());
  mEditor->toggleModified(FALSE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::applyChanges(void)
{
  QString str, atmntStr;
  QString temp;
  KMMessagePart bodyPart, *msgPart;

  assert(mMsg!=NULL);

  if (!to().isEmpty()) mMsg->setTo(to());
  if (!from().isEmpty()) mMsg->setFrom(from());
  if (!cc().isEmpty()) mMsg->setCc(cc());
  if (!subject().isEmpty()) mMsg->setSubject(subject());
  if (!replyTo().isEmpty()) mMsg->setReplyTo(replyTo());
  if (!bcc().isEmpty()) mMsg->setBcc(bcc());
  if (!followupTo().isEmpty()) mMsg->setFollowup(followupTo());
  if (!newsgroups().isEmpty()) mMsg->setGroups(newsgroups());

  if(mAtmList.count() <= 0)
  {
    // If there are no attachments in the list waiting it is a simple 
    // text message.
    mMsg->setBody(pgpProcessedMsg());
  }
  else 
  { 
    mMsg->deleteBodyParts();

    // create informative header for those that have no mime-capable
    // email client
    mMsg->setBody("This message is in MIME format.\n\n");

    // create bodyPart for editor text.
    bodyPart.setCteStr("7bit"); 
    bodyPart.setTypeStr("text");
    bodyPart.setSubtypeStr("plain");
    bodyPart.setBody(pgpProcessedMsg());
    mMsg->addBodyPart(&bodyPart);

    // Since there is at least one more attachment create another bodypart
    for (msgPart=mAtmList.first(); msgPart; msgPart=mAtmList.next())
	mMsg->addBodyPart(msgPart);
  }

  mMsg->setAutomaticFields();
  if (!mAutoDeleteMsg) mEditor->toggleModified(FALSE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::closeEvent(QCloseEvent* e)
{
  if(mEditor->isModified())
    if((KMsgBox::yesNo(0,nls->translate("KMail Confirm"),
		       nls->translate("Close and discard\nedited message?")) == 2))
      return;
  writeConfig();
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
	// check if we have a public key for the receiver
	if(!pgp->havePublicKey(receiver))
	{
	  kbp->idle();
	  warning(nls->translate("public key for %s not found.\n"
				 "This person will not be able to " 
				 "decrypt the message."),
		  (const char *)receiver);
	  kbp->busy();
	} 
	else 
	{
	  debug("encrypting for %s",(const char *)receiver);
	  persons.append(receiver);
	}
      }
      lastindex = index;
    }
    while (lastindex > 0);

    if(pgp->encryptFor(persons, doSign))
      return pgp->message();
  }

  // in case of an error we end up here
  warning(nls->translate("Error during PGP:") + QString("\n") + 
	  pgp->lastErrorMsg());

  return mEditor->text();
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
  str = kFileToString(aUrl,TRUE);
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
  msgPart->setEncodedBody(str);
  msgPart->magicSetType();

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
}


//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(KMMessagePart* msgPart)
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
const QString KMComposeWin::msgPartLbxString(KMMessagePart* msgPart) const
{
  unsigned int len;
  QString lenStr(32);

  assert(msgPart != NULL);

  len = msgPart->body().size()-1;
  if (len > 9999) lenStr.sprintf("%uK", len>>10);
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
void KMComposeWin::slotAttachFile()
{
  // Create File Dialog and return selected file
  // We will not care about any permissions, existence or whatsoever in 
  // this function.
  QString fileName;
  QFileDialog fdlg(".","*",this,NULL,TRUE);

  fdlg.setCaption(nls->translate("Attach File"));
  if (!fdlg.exec()) return;

  fileName = fdlg.selectedFile();		
  if(fileName.isEmpty()) return;

  addAttach(fileName);
}


void KMComposeWin::slotInsertFile()
{
  // Create File Dialog and return selected file

  QString fileName;
  QString strCopy;
  char temp[256];
  int col, line;
  QFileDialog fdlg(".","*",this,NULL,TRUE);

  fdlg.setCaption(nls->translate("Attach File"));
  if (!fdlg.exec()) return;

  fileName = fdlg.selectedFile();		
  if(fileName.isEmpty()) return;

  cout << fileName << endl;
  QFile *f = new QFile(fileName);

  if(!f->open(IO_ReadOnly))
    return;

  f->at(0);
  while(!f->atEnd()) {
    f->readLine(temp,255);
    strCopy.append(temp);
  }

  mEditor->getCursorPosition(&line,&col);
  mEditor->insertAt(strCopy, line ,col);
  f->close();
}  

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachPopupMenu(int index, int)
{
  QPopupMenu *menu = new QPopupMenu;

  mAtmListBox->setCurrentItem(index);

  menu->insertItem(nls->translate("View..."), this, SLOT(slotAttachView()));
  menu->insertItem(nls->translate("Save..."), this, SLOT(slotAttachSave()));
  menu->insertItem(nls->translate("Properties..."), 
		   this, SLOT(slotAttachProperties()));
  menu->insertItem(nls->translate("Remove"), this, SLOT(slotAttachRemove()));
  menu->insertSeparator();
  menu->insertItem(nls->translate("Attach..."), this, SLOT(slotAttachFile()));
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
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachSave()
{
  KMMessagePart* msgPart;
  QString fileName;

  int idx = mAtmListBox->currentItem();
  if (idx < 0) return;

  msgPart = mAtmList.at(idx);
  fileName = QFileDialog::getSaveFileName(".", "*", NULL, msgPart->name());
  kStringToFile(msgPart->bodyDecoded(), fileName, TRUE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachRemove()
{
  int idx = mAtmListBox->currentItem();
  if (idx >= 0) removeAttach(idx);
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
void KMComposeWin::slotCut()
{
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotCopy()
{
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotPaste()
{
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotClose()
{
  // we should check here if there were any changes...
  close();
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
void KMComposeWin::slotToDo()
{
  warning(nls->translate("Sorry, but this feature\nis still missing"));
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdWinTitle(const char *text)
{
  setCaption(text);
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
    str += nls->translate("To:");
    str += " \n";
    str += to();
    str += "\n";
    str += nls->translate("Subject:");
    str += " \n";
    str += subject();
    str += nls->translate("Date:");
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
void KMComposeWin::slotSend()
{
  bool sentOk;

  kbp->busy();
  applyChanges();
  sentOk = msgSender->send(mMsg);
  kbp->idle();

  if (sentOk)
  {
    mAutoDeleteMsg = FALSE;
    close();
  }
  else warning("Failed to send message.");
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSendLater()
{
  kbp->busy();
  applyChanges();
  if(msgSender->send(mMsg,FALSE))
  {
    mAutoDeleteMsg = FALSE;
    close();
  }
  kbp->idle();
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
    QFileDialog dlg;
    dlg.setCaption(nls->translate("Choose Signature File"));
    if (!dlg.exec()) return;
    sigFileName = dlg.selectedFile();
    if (sigFileName.isEmpty()) return;
    sigText = kFileToString(sigFileName);
  }
  else sigText = identity->signature();

  if (!sigText.isEmpty())
  {
    mEditor->insertLine(sigText, -1);
    mEditor->toggleModified(mod);
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotHelp()
{
  app->invokeHTMLHelp("","");
}

//Class KMLineEdit ------------------------------------------------------------

KMLineEdit::KMLineEdit(QWidget *parent = NULL, const char *name = NULL)
  :QLineEdit(parent,name)
{
}

//-----------------------------------------------------------------------------

void KMLineEdit::mousePressEvent(QMouseEvent *e)
{
  if(e->button() == MidButton) {
    QKeyEvent k( Event_KeyPress, Key_V, 0 , ControlButton);
    keyPressEvent(&k);
    return;
  }
  else if(e->button() == RightButton){
    QPopupMenu *p = new QPopupMenu;
    p->insertItem("Copy",this,SLOT(copy()));
    p->insertItem("Cut",this,SLOT(cut()));
    p->insertItem("Paste",this,SLOT(paste()));
    p->insertItem("Mark all",this,SLOT(markAll()));
    p->popup(QCursor::pos());
    }
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
    if(hasMarkedText()) {
    copy();
    // Delete text is missing
    }
}

//-----------------------------------------------------------------------------
void KMLineEdit::paste()
{
  QKeyEvent k( Event_KeyPress, Key_V , 0 , ControlButton);
  keyPressEvent(&k);
}

//-----------------------------------------------------------------------------
void KMLineEdit::markAll()
{
  selectAll();
}







