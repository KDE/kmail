// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

// keep this in sync with the define in configuredialog.h
#define DEFAULT_EDITOR_STR "kate %f"

//#define STRICT_RULES_OF_GERMAN_GOVERNMENT_01

#undef GrayScale
#undef Color
#include <config.h>
#include <qtooltip.h>
#include <qtextcodec.h>
#include <qheader.h>

#include "kmmessage.h"
#include "kmsender.h"
#include "kmkernel.h"
#include "identitymanager.h"
#include "identitycombo.h"
#include "kmidentity.h"
#include "kfileio.h"
#include "kbusyptr.h"
#include "kmmsgpartdlg.h"
#include <kpgp.h>
#include <kpgpblock.h>
#include "kmaddrbookdlg.h"
#include "kmaddrbook.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmfoldercombobox.h"
#include "kmtransport.h"

#include <kaction.h>
#include <kcharsets.h>
#include <kcompletionbox.h>
#include <kcursor.h>
#include <kcombobox.h>
#include <kservicetype.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kdebug.h>
#include <kdeversion.h>

#include "kmmainwin.h"
#include "kmreaderwin.h"

#include <assert.h>
#include <mimelib/mimepp.h>
#include <kfiledialog.h>
#include <kwin.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <kurldrag.h>

#include <kspell.h>
#include <kspelldlg.h>
#include "spellingfilter.h"

#include <qtabdialog.h>
#include <qregexp.h>
#include <qbuffer.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ktempfile.h>
#include <fcntl.h>

#include "kmrecentaddr.h"
#include <klocale.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <qpopupmenu.h>

#include "cryptplugwrapperlist.h"
#include "klistboxdialog.h"

#include "kmcomposewin.moc"



//-----------------------------------------------------------------------------
KMComposeWin::KMComposeWin( CryptPlugWrapperList * cryptPlugList,
                            KMMessage *aMsg, uint id )
  : KMTopLevelWidget(), MailComposerIface(),
    mId( id ), mCryptPlugList( cryptPlugList )

{
  // make sure we have a valid CryptPlugList
  mTmpPlugList = !mCryptPlugList;
  if( mTmpPlugList ) {
    mCryptPlugList = new CryptPlugWrapperList();
    KConfig *config = KGlobal::config();
    mCryptPlugList->loadFromConfig( config );
  }

  mMainWidget = new QWidget(this);

  mIdentity = new IdentityCombo(mMainWidget);
  mFcc = new KMFolderComboBox(mMainWidget);
  mFcc->showOutboxFolder( FALSE );
  mTransport = new QComboBox(true, mMainWidget);
  mEdtFrom = new KMLineEdit(this,false,mMainWidget);
  mEdtReplyTo = new KMLineEdit(this,false,mMainWidget);
  mEdtTo = new KMLineEdit(this,true,mMainWidget);
  mEdtCc = new KMLineEdit(this,true,mMainWidget);
  mEdtBcc = new KMLineEdit(this,true,mMainWidget);
  mEdtSubject = new KMLineEdit(this,false,mMainWidget, "subjectLine");
  mLblIdentity = new QLabel(mMainWidget);
  mLblFcc = new QLabel(mMainWidget);
  mLblTransport = new QLabel(mMainWidget);
  mLblFrom = new QLabel(mMainWidget);
  mLblReplyTo = new QLabel(mMainWidget);
  mLblTo = new QLabel(mMainWidget);
  mLblCc = new QLabel(mMainWidget);
  mLblBcc = new QLabel(mMainWidget);
  mLblSubject = new QLabel(mMainWidget);
  mBtnIdentity = new QCheckBox(i18n("Sticky"),mMainWidget);
  mBtnFcc = new QCheckBox(i18n("Sticky"),mMainWidget);
  mBtnTransport = new QCheckBox(i18n("Sticky"),mMainWidget);
  mBtnTo = new QPushButton("...",mMainWidget);
  mBtnCc = new QPushButton("...",mMainWidget);
  mBtnBcc = new QPushButton("...",mMainWidget);
  mBtnFrom = new QPushButton("...",mMainWidget);
  mBtnReplyTo = new QPushButton("...",mMainWidget);

  //setWFlags( WType_TopLevel | WStyle_Dialog );
  mDone = false;
  mGrid = NULL;
  mAtmListBox = NULL;
  mAtmList.setAutoDelete(TRUE);
  mAtmTempList.setAutoDelete(TRUE);
  mAtmModified = FALSE;
  mAutoDeleteMsg = FALSE;
  mFolder = NULL;
  bAutoCharset = TRUE;
  fixedFontAction = NULL;
  mEditor = new KMEdit(mMainWidget, this);
  mEditor->setTextFormat(Qt::PlainText);
  disableBreaking = false;
  QString tip = i18n("Select email address(es)");
  QToolTip::add( mBtnTo, tip );
  QToolTip::add( mBtnCc, tip );
  QToolTip::add( mBtnReplyTo, tip );

  mSpellCheckInProgress=FALSE;

  setCaption( i18n("Composer") );
  setMinimumSize(200,200);

  mBtnIdentity->setFocusPolicy(QWidget::NoFocus);
  mBtnFcc->setFocusPolicy(QWidget::NoFocus);
  mBtnTransport->setFocusPolicy(QWidget::NoFocus);
  mBtnTo->setFocusPolicy(QWidget::NoFocus);
  mBtnCc->setFocusPolicy(QWidget::NoFocus);
  mBtnBcc->setFocusPolicy(QWidget::NoFocus);
  mBtnFrom->setFocusPolicy(QWidget::NoFocus);
  mBtnReplyTo->setFocusPolicy(QWidget::NoFocus);

  mAtmListBox = new QListView(mMainWidget, "mAtmListBox");
  mAtmListBox->setFocusPolicy(QWidget::NoFocus);
  mAtmListBox->addColumn(i18n("Name"), 200);
  mAtmListBox->addColumn(i18n("Size"), 80);
  mAtmListBox->addColumn(i18n("Encoding"), 120);
  int atmColType = mAtmListBox->addColumn(i18n("Type"), 120);
  mAtmListBox->header()->setStretchEnabled(true, atmColType); // Stretch "Type".
  mAtmCryptoColWidth = 80;
  mAtmColEncrypt= mAtmListBox->addColumn(i18n("Encrypt"),mAtmCryptoColWidth);
  mAtmColSign   = mAtmListBox->addColumn(i18n("Sign"),   mAtmCryptoColWidth);
  mAtmListBox->setAllColumnsShowFocus(true);

  connect(mAtmListBox,
	  SIGNAL(doubleClicked(QListViewItem *)),
	  SLOT(slotAttachProperties()));
  connect(mAtmListBox,
	  SIGNAL(rightButtonPressed(QListViewItem *, const QPoint &, int)),
	  SLOT(slotAttachPopupMenu(QListViewItem *, const QPoint &, int)));
  mAttachMenu = 0;

  readConfig();
  setupStatusBar();
  setupEditor();
  setupActions();
  applyMainWindowSettings(kapp->config(), "Composer");
  toolbarAction->setChecked(!toolBar()->isHidden());
  statusbarAction->setChecked(!statusBar()->isHidden());

  connect(mEdtSubject,SIGNAL(textChanged(const QString&)),
	  SLOT(slotUpdWinTitle(const QString&)));
  connect(mBtnTo,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
  connect(mBtnCc,SIGNAL(clicked()),SLOT(slotAddrBookCc()));
  connect(mBtnBcc,SIGNAL(clicked()),SLOT(slotAddrBookBcc()));
  connect(mBtnReplyTo,SIGNAL(clicked()),SLOT(slotAddrBookReplyTo()));
  connect(mBtnFrom,SIGNAL(clicked()),SLOT(slotAddrBookFrom()));
  connect(mIdentity,SIGNAL(identityChanged(uint)),
	  SLOT(slotIdentityChanged(uint)));

  connect(mEdtTo,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
          SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
  connect(mEdtCc,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
          SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
  connect(mEdtBcc,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
          SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
  connect(mEdtReplyTo,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
          SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
  connect(mEdtFrom,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
          SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
	connect(kernel->folderMgr(),SIGNAL(removed(KMFolder*)),
					SLOT(slotFolderRemoved(KMFolder*)));
	connect(kernel->imapFolderMgr(),SIGNAL(removed(KMFolder*)),
					SLOT(slotFolderRemoved(KMFolder*)));

  connect (mEditor, SIGNAL (spellcheck_done(int)),
    this, SLOT (slotSpellcheckDone (int)));

  mMainWidget->resize(480,510);
  setCentralWidget(mMainWidget);
  rethinkFields();

  if (useExtEditor) {
    mEditor->setExternalEditor(true);
    mEditor->setExternalEditorPath(mExtEditor);
  }

  mMsg = NULL;
  bccMsgList.setAutoDelete( false );
  if (aMsg)
    setMsg(aMsg);

  mEdtTo->setFocus();
  mErrorProcessingStructuringInfo =
    i18n("Structuring information returned by the Crypto Plug-In "
         "could not be processed correctly, the Plug-In might be damaged.\n"
         "PLEASE CONTACT YOUR SYSTEM ADMINISTRATOR");
  mErrorNoCryptPlugAndNoBuildIn =
    i18n("<p>No active Crypto Plug-In was found and the built-in OpenPGP code "
         "did not run successfully.</p>"
         "<p>You can do two things to change this:</p>"
         "<ul><li><em>either</em> activate a Plug-In using the "
         "Settings->Configure KMail->Plug-In dialog.</li>"
         "<li><em>or</em> specify traditional OpenPGP settings on the same dialog's "
         "Identity->Advanced tab.</li></ul>");
  mDone = true;
}


//-----------------------------------------------------------------------------
KMComposeWin::~KMComposeWin()
{
  writeConfig();
  if (mFolder && mMsg)
  {
    mAutoDeleteMsg = FALSE;
    mFolder->addMsg(mMsg);
    emit messageQueuedOrDrafted();
  }
  if (mAutoDeleteMsg && mMsg) delete mMsg;
  QMap<KIO::Job*, atmLoadData>::Iterator it = mapAtmLoadData.begin();
  while ( it != mapAtmLoadData.end() )
  {
    KIO::Job *job = it.key();
    mapAtmLoadData.remove( it );
    job->kill();
    it = mapAtmLoadData.begin();
  }
  bccMsgList.clear();
  if( mTmpPlugList )
    delete mCryptPlugList;
}

//-----------------------------------------------------------------------------
void KMComposeWin::send(int how)
{
  switch (how) {
    case 1:
      slotSendNow();
      break;
    default:
    case 0:
      // TODO: find out, what the default send method is and send it this way
    case 2:
      slotSendLater();
      break;
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttachment(KURL url,QString /*comment*/)
{
  addAttach(url);
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttachment(const QString &name,
                                 const QCString &/*cte*/,
                                 const QByteArray &data,
                                 const QCString &type,
                                 const QCString &subType,
                                 const QCString &paramAttr,
                                 const QString &paramValue,
                                 const QCString &contDisp)
{
  if (!data.isEmpty()) {
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName(name);
    QValueList<int> dummy;
    msgPart->setBodyAndGuessCte(data, dummy,
				kernel->msgSender()->sendQuotedPrintable());
    msgPart->setTypeStr(type);
    msgPart->setSubtypeStr(subType);
    msgPart->setParameter(paramAttr,paramValue);
    msgPart->setContentDisposition(contDisp);
    addAttach(msgPart);
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::setBody(QString body)
{
  mEditor->setText(body);
}

//-----------------------------------------------------------------------------
bool KMComposeWin::event(QEvent *e)
{
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
  }
  return KMTopLevelWidget::event(e);
}


//-----------------------------------------------------------------------------
void KMComposeWin::readColorConfig(void)
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Reader");
  QColor c1=QColor(kapp->palette().active().text());
  QColor c4=QColor(kapp->palette().active().base());

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    foreColor = config->readColorEntry("ForegroundColor",&c1);
    backColor = config->readColorEntry("BackgroundColor",&c4);
  }
  else {
    foreColor = c1;
    backColor = c4;
  }

  // Color setup
  mPalette = kapp->palette();
  QColorGroup cgrp  = mPalette.active();
  cgrp.setColor( QColorGroup::Base, backColor);
  cgrp.setColor( QColorGroup::Text, foreColor);
  mPalette.setDisabled(cgrp);
  mPalette.setActive(cgrp);
  mPalette.setInactive(cgrp);

  mEdtTo->setPalette(mPalette);
  mEdtFrom->setPalette(mPalette);
  mEdtCc->setPalette(mPalette);
  mEdtSubject->setPalette(mPalette);
  mEdtReplyTo->setPalette(mPalette);
  mEdtBcc->setPalette(mPalette);
  mTransport->setPalette(mPalette);
  mEditor->setPalette(mPalette);
  mFcc->setPalette(mPalette);
}

//-----------------------------------------------------------------------------
void KMComposeWin::readConfig(void)
{
  KConfig *config = kapp->config();
  QCString str;
  //  int w, h,
  int maxTransportItems;

  KConfigGroupSaver saver(config, "Composer");

  mDefCharset = KMMessage::defaultCharset();
  mForceReplyCharset = config->readBoolEntry("force-reply-charset", false );
  mAutoSign = config->readEntry("signature","auto") == "auto";
  mShowHeaders = config->readNumEntry("headers", HDR_STANDARD);
  mWordWrap = config->readBoolEntry("word-wrap", true);
  mLineBreak = config->readNumEntry("break-at", 78);
  mBtnIdentity->setChecked(config->readBoolEntry("sticky-identity", false));
  if (mBtnIdentity->isChecked())
    mId = config->readUnsignedNumEntry("previous-identity", mId );
  mBtnFcc->setChecked(config->readBoolEntry("sticky-fcc", false));
  QString previousFcc = kernel->sentFolder()->idString();
  if (mBtnFcc->isChecked())
    previousFcc = config->readEntry("previous-fcc", previousFcc );
  mBtnTransport->setChecked(config->readBoolEntry("sticky-transport", false));
  mTransportHistory = config->readListEntry("transport-history");
  QString currentTransport = config->readEntry("current-transport");
  maxTransportItems = config->readNumEntry("max-transport-items",10);

  if ((mLineBreak == 0) || (mLineBreak > 78))
    mLineBreak = 78;
  if (mLineBreak < 30)
    mLineBreak = 30;
  mAutoPgpSign = config->readBoolEntry("pgp-auto-sign", false);
  mAutoPgpEncrypt = config->readBoolEntry("pgp-auto-encrypt", false);
  mConfirmSend = config->readBoolEntry("confirm-before-send", false);

  int mode = config->readNumEntry("Completion Mode",
                                  KGlobalSettings::completionMode() );
  mEdtFrom->setCompletionMode( (KGlobalSettings::Completion) mode );
  mEdtReplyTo->setCompletionMode( (KGlobalSettings::Completion) mode );
  mEdtTo->setCompletionMode( (KGlobalSettings::Completion) mode );
  mEdtCc->setCompletionMode( (KGlobalSettings::Completion) mode );
  mEdtBcc->setCompletionMode( (KGlobalSettings::Completion) mode );

  readColorConfig();

  { // area for config group "General"
    KConfigGroupSaver saver(config, "General");
    mExtEditor = config->readEntry("external-editor", DEFAULT_EDITOR_STR);
    useExtEditor = config->readBoolEntry("use-external-editor", FALSE);

    int headerCount = config->readNumEntry("mime-header-count", 0);
    mCustHeaders.clear();
    mCustHeaders.setAutoDelete(true);
    for (int i = 0; i < headerCount; i++) {
      QString thisGroup;
      _StringPair *thisItem = new _StringPair;
      thisGroup.sprintf("Mime #%d", i);
      KConfigGroupSaver saver(config, thisGroup);
      thisItem->name = config->readEntry("name", "");
      if ((thisItem->name).length() > 0) {
        thisItem->value = config->readEntry("value", "");
        mCustHeaders.append(thisItem);
      } else {
        delete thisItem;
      }
    }
  }

  { // area fo config group "Fonts"
    KConfigGroupSaver saver(config, "Fonts");
    mBodyFont = KGlobalSettings::generalFont();
    mFixedFont = KGlobalSettings::fixedFont();
    if (!config->readBoolEntry("defaultFonts",TRUE)) {
      mBodyFont = config->readFontEntry("composer-font", &mBodyFont);
      mFixedFont = config->readFontEntry("fixed-font", &mFixedFont);
    }
    slotUpdateFont();
    mEdtFrom->setFont(mBodyFont);
    mEdtReplyTo->setFont(mBodyFont);
    mEdtTo->setFont(mBodyFont);
    mEdtCc->setFont(mBodyFont);
    mEdtBcc->setFont(mBodyFont);
    mEdtSubject->setFont(mBodyFont);
  }

  { // area fo config group "Fonts"
    KConfigGroupSaver saver(config, "Geometry");
    QSize defaultSize(480,510);
    QSize siz = config->readSizeEntry("composer", &defaultSize);
    if (siz.width() < 200) siz.setWidth(200);
    if (siz.height() < 200) siz.setHeight(200);
    resize(siz);
  }

  mIdentity->setCurrentIdentity( mId );

  mTransport->insertStringList( KMTransportInfo::availableTransports() );
  while (mTransportHistory.count() > (uint)maxTransportItems)
    mTransportHistory.remove( mTransportHistory.last() );
  mTransport->insertStringList( mTransportHistory );
  if (mBtnTransport->isChecked() && !currentTransport.isEmpty())
  {
    for (int i = 0; i < mTransport->count(); i++)
      if (mTransport->text(i) == currentTransport)
        mTransport->setCurrentItem(i);
    mTransport->setEditText( currentTransport );
  }

  if ( !mBtnFcc->isChecked() )
  {
      kdDebug(5006) << "KMComposeWin::readConfig. " << mIdentity->currentIdentityName() << endl;
      const KMIdentity & ident =
        kernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );
      kdDebug(5006) << "KMComposeWin::readConfig: identity.fcc()='"
                    << ident.fcc() << "'" << endl;
      if ( ident.fcc().isEmpty() )
        previousFcc = kernel->sentFolder()->idString();
      else
        previousFcc = ident.fcc();
      kdDebug(5006) << "KMComposeWin::readConfig: previousFcc="
                << previousFcc <<  endl;
  }

  setFcc( previousFcc );
}

//-----------------------------------------------------------------------------
void KMComposeWin::writeConfig(void)
{
  KConfig *config = kapp->config();
  QString str;

  {
    KConfigGroupSaver saver(config, "Composer");
    config->writeEntry("signature", mAutoSign?"auto":"manual");
    config->writeEntry("headers", mShowHeaders);
    config->writeEntry("sticky-transport", mBtnTransport->isChecked());
    config->writeEntry("sticky-identity", mBtnIdentity->isChecked());
    config->writeEntry("sticky-fcc", mBtnFcc->isChecked());
    config->writeEntry("previous-identity", mIdentity->currentIdentity() );
    config->writeEntry("current-transport", mTransport->currentText());
    config->writeEntry("previous-fcc", mFcc->getFolder()->idString() );
    mTransportHistory.remove(mTransport->currentText());
    if (KMTransportInfo::availableTransports().findIndex(mTransport
      ->currentText()) == -1)
        mTransportHistory.prepend(mTransport->currentText());
    config->writeEntry("transport-history", mTransportHistory );
  }

  {
    KConfigGroupSaver saver(config, "Geometry");
    config->writeEntry("composer", size());

    saveMainWindowSettings(config, "Composer");
    config->sync();
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::deadLetter(void)
{
  if (!mMsg) return;

  // This method is called when KMail crashed, so we better use as
  // basic functions as possible here.
  applyChanges();
  QCString msgStr = mMsg->asString();
  QCString fname = getenv("HOME");
  fname += "/dead.letter";
  // Security: the file is created in the user's home directory, which
  // might be readable by other users. So the file only gets read/write
  // permissions for the user himself. Note that we create the file with
  // correct permissions, we do not set them after creating the file!
  // (dnaber, 2000-02-27):
  int fd = open(fname, O_CREAT|O_APPEND|O_WRONLY, S_IWRITE|S_IREAD);
  if (fd != -1)
  {
    QCString startStr = "From " + mMsg->fromEmail() + " " + mMsg->dateShortStr() + "\n";
    ::write(fd, startStr, startStr.length());
    ::write(fd, msgStr, msgStr.length());
    ::write(fd, "\n", 1);
    ::close(fd);
    fprintf(stderr,"appending message to ~/dead.letter\n");
  }
  else perror("cannot open ~/dead.letter for saving the current message");
}



//-----------------------------------------------------------------------------
void KMComposeWin::slotView(void)
{
  if (!mDone)
    return; // otherwise called from rethinkFields during the construction
            // which is not the intended behavior
  int id;

  //This sucks awfully, but no, I cannot get an activated(int id) from
  // actionContainer()
  if (!sender()->isA("KToggleAction"))
    return;
  KToggleAction *act = (KToggleAction *) sender();

  if (act == allFieldsAction)
    id = 0;
  else if (act == identityAction)
    id = HDR_IDENTITY;
  else if (act == transportAction)
    id = HDR_TRANSPORT;
  else if (act == fromAction)
    id = HDR_FROM;
  else if (act == replyToAction)
    id = HDR_REPLY_TO;
  else if (act == toAction)
    id = HDR_TO;
  else if (act == ccAction)
    id = HDR_CC;
  else  if (act == bccAction)
    id = HDR_BCC;
  else if (act == subjectAction)
    id = HDR_SUBJECT;
  else if (act == fccAction)
    id = HDR_FCC;
  else
   {
     id = 0;
     kdDebug(5006) << "Something is wrong (Oh, yeah?)" << endl;
     return;
   }

  // sanders There's a bug here this logic doesn't work if no
  // fields are shown and then show all fields is selected.
  // Instead of all fields being shown none are.
  if (!act->isChecked())
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
  rethinkFields(true);

}

void KMComposeWin::rethinkFields(bool fromSlot)
{
  //This sucks even more but again no ids. sorry (sven)
  int mask, row, numRows;
  long showHeaders;

  if (mShowHeaders < 0)
    showHeaders = HDR_ALL;
  else
    showHeaders = mShowHeaders;

  for (mask=1,mNumHeaders=0; mask<=showHeaders; mask<<=1)
    if ((showHeaders&mask) != 0) mNumHeaders++;

  numRows = mNumHeaders + 2;

  if (mGrid) delete mGrid;
  mGrid = new QGridLayout(mMainWidget, numRows, 3, 4, 4);
  mGrid->setColStretch(0, 1);
  mGrid->setColStretch(1, 100);
  mGrid->setColStretch(2, 1);
  mGrid->setRowStretch(mNumHeaders, 100);

  mEdtList.clear();
  row = 0;
  kdDebug(5006) << "KMComposeWin::rethinkFields" << endl;
  if (!fromSlot) allFieldsAction->setChecked(showHeaders==HDR_ALL);

  if (!fromSlot) identityAction->setChecked(abs(mShowHeaders)&HDR_IDENTITY);
  rethinkHeaderLine(showHeaders,HDR_IDENTITY, row, i18n("&Identity:"),
		    mLblIdentity, mIdentity, mBtnIdentity);
  if (!fromSlot) fccAction->setChecked(abs(mShowHeaders)&HDR_FCC);
  rethinkHeaderLine(showHeaders,HDR_FCC, row, i18n("Sent-Mail fol&der:"),
		    mLblFcc, mFcc, mBtnFcc);
  if (!fromSlot) transportAction->setChecked(abs(mShowHeaders)&HDR_TRANSPORT);
  rethinkHeaderLine(showHeaders,HDR_TRANSPORT, row, i18n("Mai&l transport:"),
		    mLblTransport, mTransport, mBtnTransport);
  if (!fromSlot) fromAction->setChecked(abs(mShowHeaders)&HDR_FROM);
  rethinkHeaderLine(showHeaders,HDR_FROM, row, i18n("&From:"),
		    mLblFrom, mEdtFrom, mBtnFrom);
  if (!fromSlot) replyToAction->setChecked(abs(mShowHeaders)&HDR_REPLY_TO);
  rethinkHeaderLine(showHeaders,HDR_REPLY_TO,row,i18n("&Reply to:"),
		    mLblReplyTo, mEdtReplyTo, mBtnReplyTo);
  if (!fromSlot) toAction->setChecked(abs(mShowHeaders)&HDR_TO);
  rethinkHeaderLine(showHeaders,HDR_TO, row, i18n("&To:"),
		    mLblTo, mEdtTo, mBtnTo);
  if (!fromSlot) ccAction->setChecked(abs(mShowHeaders)&HDR_CC);
  rethinkHeaderLine(showHeaders,HDR_CC, row, i18n("&CC:"),
		    mLblCc, mEdtCc, mBtnCc);
  if (!fromSlot) bccAction->setChecked(abs(mShowHeaders)&HDR_BCC);
  rethinkHeaderLine(showHeaders,HDR_BCC, row, i18n("&BCC:"),
		    mLblBcc, mEdtBcc, mBtnBcc);
  if (!fromSlot) subjectAction->setChecked(abs(mShowHeaders)&HDR_SUBJECT);
  rethinkHeaderLine(showHeaders,HDR_SUBJECT, row, i18n("S&ubject:"),
		    mLblSubject, mEdtSubject);
  assert(row<=mNumHeaders);

  mGrid->addMultiCellWidget(mEditor, row, mNumHeaders, 0, 2);
  mGrid->addMultiCellWidget(mAtmListBox, mNumHeaders+1, mNumHeaders+1, 0, 2);

  if (mAtmList.count() > 0)
    mAtmListBox->show();
  else
    mAtmListBox->hide();
  resize(this->size());
  repaint();

  mGrid->activate();

  enableAttachActions();
  identityAction->setEnabled(!allFieldsAction->isChecked());
  transportAction->setEnabled(!allFieldsAction->isChecked());
  fromAction->setEnabled(!allFieldsAction->isChecked());
  replyToAction->setEnabled(!allFieldsAction->isChecked());
  toAction->setEnabled(!allFieldsAction->isChecked());
  ccAction->setEnabled(!allFieldsAction->isChecked());
  bccAction->setEnabled(!allFieldsAction->isChecked());
  fccAction->setEnabled(!allFieldsAction->isChecked());
  subjectAction->setEnabled(!allFieldsAction->isChecked());
}


//-----------------------------------------------------------------------------
void KMComposeWin::rethinkHeaderLine(int aValue, int aMask, int& aRow,
				     const QString &aLabelStr, QLabel* aLbl,
				     QLineEdit* aEdt, QPushButton* aBtn)
{
  if (aValue & aMask)
  {
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
    mEdtList.append(aEdt);

    mGrid->addWidget(aEdt, aRow, 1);
    if (aBtn)
    {
      mGrid->addWidget(aBtn, aRow, 2);
      aBtn->setFixedSize(aBtn->sizeHint().width(), aLbl->height());
      aBtn->show();
    }
    aRow++;
  }
  else
  {
    aLbl->hide();
    aEdt->hide();
    if (aBtn) aBtn->hide();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::rethinkHeaderLine(int aValue, int aMask, int& aRow,
				     const QString &aLabelStr, QLabel* aLbl,
				     QComboBox* aCbx, QCheckBox* aChk)
{
  if (aValue & aMask)
  {
    aLbl->setText(aLabelStr);
    aLbl->adjustSize();
    aLbl->resize((int)aLbl->sizeHint().width(),aLbl->sizeHint().height() + 6);
    aLbl->setMinimumSize(aLbl->size());
    aLbl->show();
    aLbl->setBuddy(aCbx);
    mGrid->addWidget(aLbl, aRow, 0);

    //    aCbx->setBackgroundColor( backColor );
    aCbx->show();
    aCbx->setMinimumSize(100, aLbl->height()+2);

    mGrid->addWidget(aCbx, aRow, 1);
    mGrid->addWidget(aChk, aRow, 2);
    aChk->setFixedSize(aChk->sizeHint().width(), aLbl->height());
    aChk->show();
    aRow++;
  }
  else
  {
    aLbl->hide();
    aCbx->hide();
    aChk->hide();
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupActions(void)
{
  if (kernel->msgSender()->sendImmediate()) //default == send now?
  {
    //default = send now, alternative = queue
    (void) new KAction (i18n("&Send"), "mail_send", CTRL+Key_Return,
                        this, SLOT(slotSendNow()), actionCollection(),
                        "send_default");
    (void) new KAction (i18n("&Queue"), "queue", 0,
                        this, SLOT(slotSendLater()),
                        actionCollection(), "send_alternative");
  }
  else //no, default = send later
  {
    //default = queue, alternative = send now
    (void) new KAction (i18n("&Queue"), "queue",
                        CTRL+Key_Return,
                        this, SLOT(slotSendLater()), actionCollection(),
                        "send_default");
    (void) new KAction (i18n("S&end Now"), "mail_send", 0,
                        this, SLOT(slotSendNow()),
                        actionCollection(), "send_alternative");
  }

  (void) new KAction (i18n("Save in &Drafts Folder"), 0,
		      this, SLOT(slotSaveDraft()),
		      actionCollection(), "save_in_drafts");
  (void) new KAction (i18n("&Insert File..."), "fileopen", 0,
                      this,  SLOT(slotInsertFile()),
                      actionCollection(), "insert_file");
  (void) new KAction (i18n("Address &Book..."), "contents",0,
                      this, SLOT(slotAddrBook()),
                      actionCollection(), "addresbook");
  (void) new KAction (i18n("&New Composer..."), "filenew",
                      KStdAccel::shortcut(KStdAccel::New),
                      this, SLOT(slotNewComposer()),
                      actionCollection(), "new_composer");

  //KStdAction::save(this, SLOT(), actionCollection(), "save_message");
  KStdAction::print (this, SLOT(slotPrint()), actionCollection());
  KStdAction::close (this, SLOT(slotClose()), actionCollection());

  KStdAction::undo (this, SLOT(slotUndo()), actionCollection());
  KStdAction::redo (this, SLOT(slotRedo()), actionCollection());
  KStdAction::cut (this, SLOT(slotCut()), actionCollection());
  KStdAction::copy (this, SLOT(slotCopy()), actionCollection());
  KStdAction::paste (this, SLOT(slotPaste()), actionCollection());
  KStdAction::selectAll (this, SLOT(slotMarkAll()), actionCollection());

  KStdAction::find (this, SLOT(slotFind()), actionCollection());
  KStdAction::replace (this, SLOT(slotReplace()), actionCollection());
  KStdAction::spelling (this, SLOT(slotSpellcheck()), actionCollection(), "spellcheck");

  (void) new KAction (i18n("Cl&ean Spaces"), 0, this, SLOT(slotCleanSpace()),
                      actionCollection(), "clean_spaces");

  fixedFontAction = new KToggleAction( i18n("Use Fi&xed Font"), 0, this,
                      SLOT(slotUpdateFont()), actionCollection(), "toggle_fixedfont" );

  //these are checkable!!!
  urgentAction = new KToggleAction (i18n("&Urgent"), 0,
                                    actionCollection(),
                                    "urgent");
  confirmDeliveryAction =  new KToggleAction (i18n("&Confirm Delivery"), 0,
                                              actionCollection(),
                                              "confirm_delivery");
  confirmReadAction = new KToggleAction (i18n("Confirm &Read"), 0,
                                         actionCollection(), "confirm_read");
  //----- Message-Encoding Submenu
  encodingAction = new KSelectAction( i18n( "Set &Encoding" ), "charset",
				      0, this, SLOT(slotSetCharset() ),
				      actionCollection(), "charsets" );
  wordWrapAction = new KToggleAction (i18n("&Wordwrap"), 0,
		      actionCollection(), "wordwrap");
  wordWrapAction->setChecked(mWordWrap);
  connect(wordWrapAction, SIGNAL(toggled(bool)), SLOT(slotWordWrapToggled(bool)));

  QStringList encodings = KMMsgBase::supportedEncodings(TRUE);
  encodings.prepend( i18n("Auto-detect"));
  encodingAction->setItems( encodings );
  encodingAction->setCurrentItem( -1 );

  //these are checkable!!!
  allFieldsAction = new KToggleAction (i18n("&All Fields"), 0, this,
                                       SLOT(slotView()),
                                       actionCollection(), "show_all_fields");
  identityAction = new KToggleAction (i18n("&Identity"), 0, this,
				      SLOT(slotView()),
				      actionCollection(), "show_identity");
  fccAction = new KToggleAction (i18n("Sent-Mail F&older"), 0, this,
                                 SLOT(slotView()),
                                 actionCollection(), "show_fcc");
  transportAction = new KToggleAction (i18n("&Mail Transport"), 0, this,
				      SLOT(slotView()),
				      actionCollection(), "show_transport");
  fromAction = new KToggleAction (i18n("&From"), 0, this,
                                  SLOT(slotView()),
                                  actionCollection(), "show_from");
  replyToAction = new KToggleAction (i18n("&Reply To"), 0, this,
                                     SLOT(slotView()),
                                     actionCollection(), "show_reply_to");
  toAction = new KToggleAction (i18n("&To"), 0, this,
                                SLOT(slotView()),
                                actionCollection(), "show_to");
  ccAction = new KToggleAction (i18n("&CC"), 0, this,
                                SLOT(slotView()),
                                actionCollection(), "show_cc");
  bccAction = new KToggleAction (i18n("&BCC"), 0, this,
                                 SLOT(slotView()),
                                 actionCollection(), "show_bcc");
  subjectAction = new KToggleAction (i18n("&Subject"), 0, this,
                                     SLOT(slotView()),
                                     actionCollection(), "show_subject");
  //end of checkable

  (void) new KAction (i18n("Append S&ignature"), 0, this,
                      SLOT(slotAppendSignature()),
                      actionCollection(), "append_signature");
  attachPK  = new KAction (i18n("Attach &Public Key..."), 0, this,
                           SLOT(slotInsertPublicKey()),
                           actionCollection(), "attach_public_key");
  attachMPK = new KAction (i18n("Attach &My Public Key"), 0, this,
                           SLOT(slotInsertMyPublicKey()),
                           actionCollection(), "attach_my_public_key");
  (void) new KAction (i18n("&Attach File..."), "attach",
                      0, this, SLOT(slotAttachFile()),
                      actionCollection(), "attach");
  attachRemoveAction = new KAction (i18n("&Remove"), 0, this,
                      SLOT(slotAttachRemove()),
                      actionCollection(), "remove");
  attachSaveAction = new KAction (i18n("&Save..."), "filesave",0,
                      this, SLOT(slotAttachSave()),
                      actionCollection(), "attach_save");
  attachPropertiesAction = new KAction (i18n("Pr&operties..."), 0, this,
                      SLOT(slotAttachProperties()),
                      actionCollection(), "attach_properties");

  toolbarAction = KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
    actionCollection());
  statusbarAction = KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
    actionCollection());
  KStdAction::keyBindings(this, SLOT(slotEditKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
  (void) new KAction (i18n("&Spellchecker..."), 0, this, SLOT(slotSpellcheckConfig()),
                      actionCollection(), "setup_spellchecker");

  encryptAction = new KToggleAction (i18n("&Encrypt Message"),
                                     "decrypted", 0,
                                     actionCollection(), "encrypt_message");
  signAction = new KToggleAction (i18n("&Sign Message"),
                                  "signature", 0,
                                  actionCollection(), "sign_message");

  selectCryptoAction = new KAction (i18n("&Select Crypto..."),
                                    0, //KStdAccel::shortcut( .. ),
                                    this,
                                    SLOT(slotSelectCrypto()),
                                    actionCollection(), "select_crypto");

  // get PGP user id for the chosen identity
  const KMIdentity & ident =
    kernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  QString pgpUserId = ident.pgpIdentity();

  selectCryptoAction->setEnabled(true);

  CryptPlugWrapper* cryptPlug = mCryptPlugList ? mCryptPlugList->active() : 0;

  mLastEncryptActionState =
    ( cryptPlug && EncryptEmail_EncryptAll == cryptPlug->encryptEmail() );
  mLastSignActionState =
    (    (!cryptPlug && mAutoPgpSign)
      || ( cryptPlug && SignEmail_SignAll == cryptPlug->signEmail()) );

  if(!cryptPlug && !Kpgp::Module::getKpgp()->usePGP())
  {
    attachPK->setEnabled(false);
    attachMPK->setEnabled(false);
    encryptAction->setEnabled(false);
    encryptAction->setChecked(false);
    signAction->setEnabled(false);
    signAction->setChecked(false);
  }
  else if (!cryptPlug && pgpUserId.isEmpty())
  {
    attachMPK->setEnabled(false);
    encryptAction->setEnabled(false);
    encryptAction->setChecked(false);
    signAction->setEnabled(false);
    signAction->setChecked(false);
  }
  else
  {
    encryptAction->setChecked( mLastEncryptActionState );
    signAction->setChecked(    mLastSignActionState    );
    slotEncryptToggled( mLastEncryptActionState );
    slotSignToggled(    mLastSignActionState    );
  }

  connect(encryptAction, SIGNAL(toggled(bool)),
                         SLOT(slotEncryptToggled( bool )));
  connect(signAction,    SIGNAL(toggled(bool)),
                         SLOT(slotSignToggled(    bool )));

  createGUI("kmcomposerui.rc");
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupStatusBar(void)
{
  statusBar()->insertItem("", 0, 1);
  statusBar()->setItemAlignment(0, AlignLeft | AlignVCenter);

  statusBar()->insertItem(i18n(" Column: %1 ").arg("     "),2,0,true);
  statusBar()->insertItem(i18n(" Line: %1 ").arg("     "),1,0,true);
}


//-----------------------------------------------------------------------------
void KMComposeWin::updateCursorPosition()
{
  int col,line;
  QString temp;
  line = mEditor->currentLine();
  col = mEditor->currentColumn();
  temp = i18n(" Line: %1 ").arg(line+1);
  statusBar()->changeItem(temp,1);
  temp = i18n(" Column: %1 ").arg(col+1);
  statusBar()->changeItem(temp,2);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupEditor(void)
{
  //QPopupMenu* menu;
  mEditor->setModified(FALSE);
  QFontMetrics fm(mBodyFont);
  mEditor->setTabStopWidth(fm.width(QChar(' ')) * 8);
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
  slotUpdateFont();

  /* installRBPopup() is broken in kdelibs, we should wait for
	  the new klibtextedit (dnaber, 2002-01-01)
  menu = new QPopupMenu(this);
  //#ifdef BROKEN
  menu->insertItem(i18n("Undo"),mEditor,
		   SLOT(undo()), KStdAccel::shortcut(KStdAccel::Undo));
  menu->insertItem(i18n("Redo"),mEditor,
		   SLOT(redo()), KStdAccel::shortcut(KStdAccel::Redo));
  menu->insertSeparator();
  //#endif //BROKEN
  menu->insertItem(i18n("Cut"), this, SLOT(slotCut()));
  menu->insertItem(i18n("Copy"), this, SLOT(slotCopy()));
  menu->insertItem(i18n("Paste"), this, SLOT(slotPaste()));
  menu->insertItem(i18n("Mark All"),this, SLOT(slotMarkAll()));
  menu->insertSeparator();
  menu->insertItem(i18n("Find..."), this, SLOT(slotFind()));
  menu->insertItem(i18n("Replace..."), this, SLOT(slotReplace()));
  menu->insertSeparator();
  menu->insertItem(i18n("Fixed font widths"), this, SLOT(slotUpdateFont()));
  mEditor->installRBPopup(menu);
  */
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
void KMComposeWin::decryptOrStripOffCleartextSignature( QCString& body )
{
  Kpgp::Module* pgp = Kpgp::Module::getKpgp();
  assert(pgp != NULL);

  QPtrList<Kpgp::Block> pgpBlocks;
  QStrList nonPgpBlocks;
  if( Kpgp::Module::prepareMessageForDecryption( body,
                                                 pgpBlocks, nonPgpBlocks ) )
  {
    // Only decrypt/strip off the signature if there is only one OpenPGP
    // block in the message
    if( pgpBlocks.count() == 1 )
    {
      Kpgp::Block* block = pgpBlocks.first();
      if( ( block->type() == Kpgp::PgpMessageBlock ) ||
          ( block->type() == Kpgp::ClearsignedBlock ) )
      {
        if( block->type() == Kpgp::PgpMessageBlock )
          // try to decrypt this OpenPGP block
          block->decrypt();
        else
          // strip off the signature
          block->verify();

        body = nonPgpBlocks.first()
             + block->text()
             + nonPgpBlocks.last();
      }
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::setMsg(KMMessage* newMsg, bool mayAutoSign, bool allowDecryption)
{
  KMMessagePart bodyPart, *msgPart;
  int i, num;

  //assert(newMsg!=NULL);
  if(!newMsg)
    {
      kdDebug(5006) << "KMComposeWin::setMsg() : newMsg == NULL!\n" << endl;
      return;
    }
  mMsg = newMsg;

  mEdtTo->setText(mMsg->to());
  mEdtFrom->setText(mMsg->from());
  mEdtCc->setText(mMsg->cc());
  mEdtSubject->setText(mMsg->subject());
  mEdtReplyTo->setText(mMsg->replyTo());
  mEdtBcc->setText(mMsg->bcc());

  if (!mBtnIdentity->isChecked() && !newMsg->headerField("X-KMail-Identity").isEmpty())
    mId = newMsg->headerField("X-KMail-Identity").stripWhiteSpace().toUInt();

  // anyone knows why slotIdentityChanged() is only to be called on
  // mBtnIdentity->isChecked()?
  if ( !mBtnIdentity->isChecked() )
    mIdentity->blockSignals( true );
  mIdentity->setCurrentIdentity( mId );
  if ( !mBtnIdentity->isChecked() )
    mIdentity->blockSignals( false );

  const KMIdentity & ident =
    kernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );

  mOldSigText = ident.signatureText();

  // get PGP user id for the currently selected identity
  QString pgpUserId = ident.pgpIdentity();

  CryptPlugWrapper* cryptPlug = mCryptPlugList ? mCryptPlugList->active() : 0;

  if( cryptPlug || Kpgp::Module::getKpgp()->usePGP() )
  {
    if( !cryptPlug && pgpUserId.isEmpty() )
    {
      attachMPK->setEnabled(false);
      encryptAction->setEnabled(false);
      encryptAction->setChecked(false);
      signAction->setEnabled(false);
      signAction->setChecked(false);
    }
    else
    {
      attachMPK->setEnabled(true);
      encryptAction->setEnabled(true);
      encryptAction->setChecked(mLastEncryptActionState);
      signAction->setEnabled(true);
      signAction->setChecked(mLastSignActionState);
    }
  }

  QString transport = newMsg->headerField("X-KMail-Transport");
  if (!mBtnTransport->isChecked() && !transport.isEmpty())
  {
    for (int i = 0; i < mTransport->count(); i++)
      if (mTransport->text(i) == transport)
        mTransport->setCurrentItem(i);
    mTransport->setEditText( transport );
  }

  if (!mBtnFcc->isChecked() && !mMsg->fcc().isEmpty())
    setFcc(mMsg->fcc());

  num = mMsg->numBodyParts();

  if (num > 0)
  {
    QCString bodyDecoded;
    mMsg->bodyPart(0, &bodyPart);

    int firstAttachment = (bodyPart.typeStr().lower() == "text") ? 1 : 0;
    if (firstAttachment)
    {
      mCharset = bodyPart.charset();
      if ((mCharset=="") || (mCharset == "default"))
        mCharset = mDefCharset;

      bodyDecoded = bodyPart.bodyDecoded();

      if( allowDecryption )
        decryptOrStripOffCleartextSignature( bodyDecoded );

      // As nobody seems to know the purpose of the following line and
      // as it breaks word wrapping of long lines if drafts with attachments
      // are opened for editting in the composer (cf. Bug#41102) I comment it
      // out. Ingo, 2002-04-21
      //verifyWordWrapLengthIsAdequate(bodyDecoded);

      QTextCodec *codec = KMMsgBase::codecForName(mCharset);
      if (codec)
        mEditor->setText(codec->toUnicode(bodyDecoded));
      else
        mEditor->setText(QString::fromLocal8Bit(bodyDecoded));
      mEditor->insertLine("\n", -1);
    } else mEditor->setText("");
    for(i=firstAttachment; i<num; i++)
    {
      msgPart = new KMMessagePart;
      mMsg->bodyPart(i, msgPart);
      addAttach(msgPart);
    }
  } else{
    mCharset=mMsg->charset();
    if ((mCharset=="") || (mCharset == "default"))
      mCharset = mDefCharset;

    QCString bodyDecoded = mMsg->bodyDecoded();

    if( allowDecryption )
      decryptOrStripOffCleartextSignature( bodyDecoded );

    QTextCodec *codec = KMMsgBase::codecForName(mCharset);
    if (codec) {
      mEditor->setText(codec->toUnicode(bodyDecoded));
    } else
      mEditor->setText(QString::fromLocal8Bit(bodyDecoded));
  }

  setCharset(mCharset);

  if( mAutoSign && mayAutoSign )
  {
    //
    // Espen 2000-05-16
    // Delay the signature appending. It may start a fileseletor.
    // Not user friendy if this modal fileseletor opens before the
    // composer.
    //
    QTimer::singleShot( 0, this, SLOT(slotAppendSignature()) );
  }
  mEditor->setModified(FALSE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setFcc( const QString &idString )
{
  // check if the sent-mail folder still exists
  KMFolder *folder = kernel->folderMgr()->findIdString( idString );
  if ( !folder )
    folder = kernel->imapFolderMgr()->findIdString( idString );
  if ( folder )
    mFcc->setFolder( idString );
  else
  {
    KMessageBox::sorry( this,
                        i18n("The sent-mail folder of the current identity "
                             "doesn't exist. Therefore the default sent-mail "
                             "folder will be used.") );
    mFcc->setFolder( kernel->sentFolder() );
  }
}


//-----------------------------------------------------------------------------
bool KMComposeWin::queryClose ()
{
  int rc;

  if(mEditor->isModified() || mEdtFrom->edited() || mEdtReplyTo->edited() ||
     mEdtTo->edited() || mEdtCc->edited() || mEdtBcc->edited() ||
     mEdtSubject->edited() || mAtmModified ||
     (mTransport->lineEdit() && mTransport->lineEdit()->edited()))
  {
    rc = KMessageBox::warningYesNoCancel(this,
           i18n("Do you want to discard the message or save it for later?"),
           i18n("Discard or Save Message"),
           i18n("&Save as Draft"),
	   // i18n fix by mok: avoid clash with common_texts (breaks translation)
	   i18n("discard message", "&Discard") );
    if (rc == KMessageBox::Cancel)
      return false;
    else if (rc == KMessageBox::Yes)
      return slotSaveDraft();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool KMComposeWin::applyChanges(void)
{
  QString str, atmntStr;
  QString temp, replyAddr;

  //assert(mMsg!=NULL);
  if(!mMsg)
  {
    kdDebug(5006) << "KMComposeWin::applyChanges() : mMsg == NULL!\n" << endl;
    return FALSE;
  }

  bccMsgList.clear();

  if (bAutoCharset) {
    QCString charset = KMMsgBase::autoDetectCharset(mCharset, KMMessage::preferredCharsets(), mEditor->text());
    if (charset.isEmpty())
    {
      KMessageBox::sorry(this,
           i18n("No suitable encoding could be found for your message.\n"
                "Please set an encoding using the 'Options' menu."));
      return false;
    }
    mCharset = charset;
  }
  mMsg->setCharset(mCharset);

  mMsg->setTo(to());
  mMsg->setFrom(from());
  mMsg->setCc(cc());
  mMsg->setSubject(subject());
  mMsg->setReplyTo(replyTo());
  mMsg->setBcc(bcc());

  const KMIdentity & id
    = kernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );
  kdDebug(5006) << "KMComposeWin::applyChanges: " << mFcc->currentText() << "=="
            << id.fcc() << "?" << endl;

  KMFolder *f = mFcc->getFolder();
  assert( f != 0 );
  if ( f->idString() == id.fcc() )
    mMsg->removeHeaderField("X-KMail-Fcc");
  else
    mMsg->setFcc( f->idString() );

  // set the correct drafts folder
  mMsg->setDrafts( id.drafts() );

  if (id.isDefault())
    mMsg->removeHeaderField("X-KMail-Identity");
  else mMsg->setHeaderField("X-KMail-Identity", QString().setNum( id.uoid() ));

  if (!replyTo().isEmpty()) replyAddr = replyTo();
  else replyAddr = from();

  if (confirmDeliveryAction->isChecked())
    mMsg->setHeaderField("Return-Receipt-To", replyAddr);

  if (confirmReadAction->isChecked())
    mMsg->setHeaderField("Disposition-Notification-To", replyAddr);

  if (urgentAction->isChecked())
  {
    mMsg->setHeaderField("X-PRIORITY", "2 (High)");
    mMsg->setHeaderField("Priority", "urgent");
  }

  _StringPair *pCH;
  for (pCH  = mCustHeaders.first();
       pCH != NULL;
       pCH  = mCustHeaders.next()) {
    mMsg->setHeaderField(KMMsgBase::toUsAscii(pCH->name), pCH->value);
  }

  bool doSign    = signAction->isChecked();
  bool doEncrypt = encryptAction->isChecked();

  // get PGP user id for the chosen identity
  const KMIdentity & ident =
    kernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  QCString pgpUserId = ident.pgpIdentity();

  CryptPlugWrapper* cryptPlug = mCryptPlugList ? mCryptPlugList->active() : 0;

  Kpgp::Module *pgp = Kpgp::Module::getKpgp();

  // check settings of composer buttons *and* attachment check boxes
  bool doSignCompletely    = doSign;
  bool doEncryptCompletely = doEncrypt;
  if( cryptPlug && (0 < mAtmList.count() ) ) {
    int idx=0;
    KMMessagePart *attachPart;
    for( attachPart = mAtmList.first();
         attachPart;
         attachPart=mAtmList.next(), ++idx ) {
      if( !encryptFlagOfAttachment( idx ) )
        doEncryptCompletely = false;
      if( !signFlagOfAttachment(    idx ) )
        doSignCompletely = false;
    }
  }

  bool bOk = true;

  if( !doSignCompletely ) {
    if( cryptPlug ) {
      // note: only ask for signing if "Warn me" flag is set! (khz)
      if( cryptPlug->warnSendUnsigned() ) {
        int ret =
        KMessageBox::warningYesNoCancel( this,
          QString( "<qt><b>"
            + i18n("Warning:")
            + "</b><br>"
            + QString(
                (doSign && !doSignCompletely)
              ? i18n("You specified not to sign some parts of this message, but"
                     " you wanted to be warned not to send unsigned messages!")
              : i18n("You specified not to sign this message, but"
                     " you wanted to be warned not to send unsigned messages!") )
            + "<br>&nbsp;<br><b>"
            + i18n("Sign all parts of this message?")
            + "</b></qt>" ) );
        if( ret == KMessageBox::Cancel )
          bOk = false;
        else if( ret == KMessageBox::Yes ) {
          doSign = true;
          doSignCompletely = true;
        }
      }
    } else {
      // ask if the message should be encrypted via old build-in pgp code
      // pending (who ever wants to implement it)
    }
  }

  if( bOk && !doEncryptCompletely ) {
    if( cryptPlug ) {
      // note: only ask for encrypting if "Warn me" flag is set! (khz)
      if( cryptPlug->warnSendUnencrypted() ) {
        int ret =
        KMessageBox::warningYesNoCancel( this,
          QString( "<qt><b>"
            + i18n("Warning:")
            + "</b><br>"
            + QString(
                (doEncrypt && !doEncryptCompletely)
              ? i18n("You specified not to encrypt some parts of this message, but"
                     " you wanted to be warned not to send unencrypted messages!")
              : i18n("You specified not to encrypt this message, but"
                     " you wanted to be warned not to send unencrypted messages!") )
            + "<br>&nbsp;<br><b>"
            + i18n("Encrypt all parts of this message?")
            + "</b></qt>" ) );
        if( ret == KMessageBox::Cancel )
          bOk = false;
        else if( ret == KMessageBox::Yes ) {
          doEncrypt = true;
          doEncryptCompletely = true;
        }
      }

      /*
      note: Processing the cryptPlug->encryptEmail() flag here would
            be absolutely wrong: this is used for specifying
                if messages should be encrypted 'in general'.
            --> This sets the initial state of a freshly started Composer.
            --> This does *not* mean overriding user setting made while
                editing in that composer window!         (khz, 2002/06/26)
      */

    } else if( mAutoPgpEncrypt && !pgpUserId.isEmpty() ) {
      // determine the complete list of recipients
      QString _to = to().simplifyWhiteSpace();
      if( !cc().isEmpty() ) {
        if( !_to.endsWith(",") )
          _to += ",";
        _to += cc().simplifyWhiteSpace();
      }
      if( !mMsg->bcc().isEmpty() ) {
        if( !_to.endsWith(",") )
          _to += ",";
        _to += mMsg->bcc().simplifyWhiteSpace();
      }
      // check if the message should be encrypted via old build-in pgp code
      QStringList allRecipients = KMMessage::splitEmailAddrList(_to);
      int status = pgp->encryptionPossible( allRecipients );
      if( status == 1 )
        doEncrypt = true;
      else if( status == 2 )
      { // the user wants to be asked or has to be asked
        kernel->kbp()->idle();
        int ret = KMessageBox::questionYesNo( this,
                                      i18n("Should this message be encrypted?") );
        kernel->kbp()->busy();
        doEncrypt = ( KMessageBox::Yes == ret );
      }
      else if( status == -1 )
      { // warn the user that there are conflicting encryption preferences
        int ret =
          KMessageBox::warningYesNoCancel( this,
                                      i18n("There are conflicting encryption "
                                      "preferences!\n\n"
                                      "Should this message be encrypted?") );
        if( ret == KMessageBox::Cancel )
          bOk = false;
        doEncrypt = ( ret == KMessageBox::Yes );
      }
    }
  }

  // This c-string (init empty here) is set by *first* testing of expiring
  // signature certificate and stops us from repeatedly asking same questions.
  QCString signCertFingerprint;

  // note: Must create extra message *before* calling compose on mMsg.
  KMMessage* extraMessage = new KMMessage( *mMsg );

  if( bOk )
    bOk = composeMessage( cryptPlug, pgpUserId,
                          *mMsg, doSign, doEncrypt, false,
                          signCertFingerprint );

  if( bOk ) {
    bool saveSentSignatures = cryptPlug ? cryptPlug->saveSentSignatures()
                                        : true;
    bool saveMessagesEncrypted = cryptPlug ? cryptPlug->saveMessagesEncrypted()
                                        : true;

    kdDebug(5006) << "\n\n" << endl;
    kdDebug(5006) << "KMComposeWin::applyChanges(void)  -  Send encrypted=" << doEncrypt << "  Store encrypted=" << saveMessagesEncrypted << endl;
// note: The following define is specified on top of this file. To compile
//       a less strict version of KMail just comment it out there above.
#ifdef STRICT_RULES_OF_GERMAN_GOVERNMENT_01
    // Hack to make sure the S/MIME CryptPlugs follows the strict requirement
    // of german government:
    // --> Encrypted messages *must* be stored in unencrypted form after sending.
    //     ( "Abspeichern ausgegangener Nachrichten in entschluesselter Form" )
    // --> Signed messages *must* be stored including the signature after sending.
    //     ( "Aufpraegen der Signatur" )
    // So we provide the user with a non-deactivateble warning and let her/him
    // choose to obey the rules or to ignore them explicitely.
    if(    cryptPlug
        && ( 0 <= cryptPlug->libName().find( "smime",   0, false ) )
        && (    ( doEncrypt && saveMessagesEncrypted )
            || ( doSign    && ! saveSentSignatures    ) ) ){

      QString headTxt =
        i18n("Warning: Your S/MIME Plug-in configuration is unsafe.");
      QString sigTxt =
        i18n("Signatures should be stored with the message, leaving them out is not allowed.");
      QString encrTxt =
        i18n("Encrypted messages should be stored in *unencrypted* form, local saving in encrypted form is not allowed.");
      QString footTxt =
        i18n("Please correct the wrong settings in KMail's Plug-in configuration pages as soon as possible.");
      QString question =
        i18n("Store message in the recommended way?");





            saveSentSignatures    = true;
      /*
      if( (doSign && !saveSentSignatures) && (doEncrypt && saveMessagesEncrypted) ) {
        if( KMessageBox::Yes == KMessageBox::warningYesNo(this, "<qt><b>" + headTxt + "</b><br>" + sigTxt + "<br>" + encrTxt + "<br>&nbsp;<br>" + footTxt + "<br>&nbsp;<br><b>" + question + "</b></qt>") ) {
            saveSentSignatures    = true;
            saveMessagesEncrypted = false;
        }
      } else if( doSign && !saveSentSignatures ) {
        if( KMessageBox::Yes == KMessageBox::warningYesNo(this, "<qt><b>" + headTxt + "</b><br>" + sigTxt  + "<br>&nbsp;<br>" + footTxt + "<br>&nbsp;<br><b>" + question + "</b></qt>") ) {
            saveSentSignatures = true;
        }
      } else */if( doEncrypt && saveMessagesEncrypted ) {
        if( KMessageBox::Yes == KMessageBox::warningYesNo(this, "<qt><b>" + headTxt + "</b><br>" + encrTxt + "<br>&nbsp;<br>" + footTxt + "<br>&nbsp;<br><b>" + question + "</b></qt>") ) {
            saveMessagesEncrypted = false;
        }
      }
    }
    kdDebug(5006) << "KMComposeWin::applyChanges(void)  -  Send encrypted=" << doEncrypt << "  Store encrypted=" << saveMessagesEncrypted << endl;
#endif

    if(    ( doEncrypt && ! saveMessagesEncrypted )
        || ( doSign    && ! saveSentSignatures    ) ){
      if( cryptPlug ){
        for( KMAtmListViewItem* entry = (KMAtmListViewItem*)mAtmItemList.first();
             entry;
             entry = (KMAtmListViewItem*)mAtmItemList.next() ){
          entry->setEncrypt( saveMessagesEncrypted );
          entry->setSign(    saveSentSignatures );
        }
      }
      bOk = composeMessage( cryptPlug, pgpUserId,
                            *extraMessage,
                            doSign    && saveSentSignatures,
                            doEncrypt && saveMessagesEncrypted,
                            true,
                            signCertFingerprint );
kdDebug(5006) << "KMComposeWin::applyChanges(void)  -  Store message in decrypted form." << endl;
      extraMessage->cleanupHeader();
      mMsg->setUnencryptedMsg( extraMessage );
    }
  }

  if( bOk ) {
    if (!mAutoDeleteMsg) mEditor->setModified(FALSE);
    mEdtFrom->setEdited(FALSE);
    mEdtReplyTo->setEdited(FALSE);
    mEdtTo->setEdited(FALSE);
    mEdtCc->setEdited(FALSE);
    mEdtBcc->setEdited(FALSE);
    mEdtSubject->setEdited(FALSE);
    if (mTransport->lineEdit())
      mTransport->lineEdit()->setEdited(FALSE);
    mAtmModified = FALSE;

    // remove fields that contain no data (e.g. an empty Cc: or Bcc:)
    mMsg->cleanupHeader();
/*
kdDebug(5006) << "\n\n\nKMComposeWin::applyChanges():\n1.: |||" << mMsg->asString() << "|||\n\n"
 << "\n\n\nKMComposeWin::applyChanges():\n1.: |||" << mMsg->asSendableString() << "|||\n\n" << endl;
*/
  }
  return bOk;
}


bool KMComposeWin::composeMessage( CryptPlugWrapper* cryptPlug,
                                   QCString pgpUserId,
                                   KMMessage& theMessage,
                                   bool doSign,
                                   bool doEncrypt,
                                   bool ignoreBcc,
                                   QCString& signCertFingerprint )
{
  bool bOk = true;
  // create informative header for those that have no mime-capable
  // email client
  theMessage.setBody( "This message is in MIME format." );

  // preprocess the body text
  QCString body = breakLinesAndApplyCodec();

  qDebug( "***body = %s", body.data() );

  if (body.isNull()) return FALSE;

  if (body.isEmpty()) body = "\n"; // don't crash

  // set the main headers
  theMessage.deleteBodyParts();
  theMessage.removeHeaderField("Content-Type");
  theMessage.removeHeaderField("Content-Transfer-Encoding");
  theMessage.setAutomaticFields(TRUE); // == multipart/mixed

  // this is our *final* body part
  KMMessagePart newBodyPart;


  // this is the boundary depth of the surrounding MIME part
  int previousBoundaryLevel = 0;


  // create temporary bodyPart for editor text
  // (and for all attachments, if mail is to be singed and/or encrypted)
  bool earlyAddAttachments =
    cryptPlug && (0 < mAtmList.count()) && (doSign || doEncrypt);

  bool allAttachmentsAreInBody = earlyAddAttachments ? true : false;

  // test whether there ARE attachments that can be included into the body
  if( earlyAddAttachments ) {
    bool someOk = false;
    int idx;
    KMMessagePart *attachPart;
    for( idx=0, attachPart = mAtmList.first();
        attachPart;
        attachPart=mAtmList.next(),
        ++idx )
      if(    doEncrypt == encryptFlagOfAttachment( idx )
          && doSign    == signFlagOfAttachment(    idx ) )
        someOk = true;
      else
        allAttachmentsAreInBody = false;
    if( !allAttachmentsAreInBody && !someOk )
      earlyAddAttachments = false;
  }

  KMMessagePart oldBodyPart;
  oldBodyPart.setTypeStr(   earlyAddAttachments ? "multipart" : "text" );
  oldBodyPart.setSubtypeStr(earlyAddAttachments ? "mixed"     : "plain");
  // NOTE: Here *signing* has higher priority than encrypting
  //       since when signing the oldBodyPart's contents are
  //       used for signing *first*.
  if( doSign || doEncrypt )
    oldBodyPart.setContentDescription( doSign
                                      ? (cryptPlug ? "signed data" : "clearsigned data")
                                      : "encrypted data" );
  oldBodyPart.setContentDisposition( "inline" );

  QCString boundaryCStr;

  bool isQP = kernel->msgSender()->sendQuotedPrintable();

  if( earlyAddAttachments ) {
    // calculate a boundary string
    ++previousBoundaryLevel;
    DwMediaType tmpCT;
    tmpCT.CreateBoundary( previousBoundaryLevel );
    boundaryCStr = tmpCT.Boundary().c_str();
    // add the normal body text
    KMMessagePart innerBodyPart;
    innerBodyPart.setTypeStr(   "text" );
    innerBodyPart.setSubtypeStr("plain");
    innerBodyPart.setContentDescription( "body text" );
    innerBodyPart.setContentDisposition( "inline" );
    innerBodyPart.setContentTransferEncodingStr( isQP ? "quoted-printable" : "8bit" );
    innerBodyPart.setCharset(mCharset);
    innerBodyPart.setBodyEncoded( body );
    DwBodyPart* innerDwPart = theMessage.createDWBodyPart( &innerBodyPart );
    innerDwPart->Assemble();
    body  = "--";
    body +=     boundaryCStr;
    body +=                 "\n";
    body += innerDwPart->AsString().c_str();
    delete innerDwPart;
    // add all matching Attachments
    // NOTE: This code will be changed when KMime is complete.
    int idx;
    KMMessagePart *attachPart;
    for( idx=0, attachPart = mAtmList.first();
        attachPart;
        attachPart=mAtmList.next(),
        ++idx ) {
      if( !cryptPlug
          || (    doEncrypt == encryptFlagOfAttachment( idx )
              && doSign    == signFlagOfAttachment(    idx ) ) ){
        innerDwPart = theMessage.createDWBodyPart( attachPart );
        innerDwPart->Assemble();
        body += "\n--";
        body +=       boundaryCStr;
        body +=                   "\n";
        body += innerDwPart->AsString().c_str();
        delete innerDwPart;
      }
    }
    body += "\n--";
    body +=       boundaryCStr;
    body +=                   "--\n";
  }
  else
  {
    oldBodyPart.setContentTransferEncodingStr( isQP ? "quoted-printable" : "8bit" );
    oldBodyPart.setCharset(mCharset);
  }
  // create S/MIME body part for signing and/or encrypting
  oldBodyPart.setBodyEncoded( body );

  QCString encodedBody; // only needed if signing and/or encrypting

  if( doSign || doEncrypt ) {
    if( cryptPlug ) {
      // get string representation of body part (including the attachments)
      DwBodyPart* dwPart = theMessage.createDWBodyPart( &oldBodyPart );
      dwPart->Assemble();
      encodedBody = dwPart->AsString().c_str();
      delete dwPart;

      // manually add a boundary definition to the Content-Type header
      if( !boundaryCStr.isEmpty() ) {
        int boundPos = encodedBody.find( '\n' );
        if( -1 < boundPos ) {
          // insert new "boundary" parameter
          QCString bStr( ";\n  boundary=\"" );
          bStr += boundaryCStr;
          bStr += "\"";
          encodedBody.insert( boundPos, bStr );
        }
      }

      // kdDebug(5006) << "\n\n\n******* a) encodedBody = \"" << encodedBody << "\"******\n\n" << endl;

      if( (0 <= cryptPlug->libName().find( "smime",   0, false )) ||
          (0 <= cryptPlug->libName().find( "openpgp", 0, false )) ) {
        // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
        // according to RfC 2633, 3.1.1 Canonicalization
        kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
        encodedBody = KMMessage::lf2crlf( encodedBody );
        kdDebug(5006) << "                                                       done." << endl;
        // kdDebug(5006) << "\n\n\n******* b) encodedBody = \"" << encodedBody << "\"******\n\n" << endl;
      }
    } else {
      encodedBody = body;
    }
  }

  if( doSign ) {
    if( cryptPlug ) {
      StructuringInfoWrapper structuring( cryptPlug );

      // kdDebug(5006) << "\n\n\n******* c) encodedBody = \"" << encodedBody << "\"******\n\n" << endl;

      QByteArray signature = pgpSignedMsg( encodedBody,
                                           structuring,
                                           signCertFingerprint );
      kdDebug(5006) << "                           size of signature: " << signature.count() << "\n" << endl;
      bOk = !signature.isEmpty();
      if( bOk ) {
        bOk = processStructuringInfo( QString::fromUtf8( cryptPlug->bugURL() ),
                                      previousBoundaryLevel + doEncrypt ? 3 : 2,
                                      oldBodyPart.contentDescription(),
                                      oldBodyPart.typeStr(),
                                      oldBodyPart.subtypeStr(),
                                      oldBodyPart.contentDisposition(),
                                      oldBodyPart.contentTransferEncodingStr(),
                                      encodedBody,
                                      "signature",
                                      signature,
                                      structuring,
                                      newBodyPart );
        if( bOk ) {
          if( newBodyPart.name().isEmpty() )
            newBodyPart.setName("signed message part");
          newBodyPart.setCharset( oldBodyPart.charset() );
        } else
          KMessageBox::sorry(this, mErrorProcessingStructuringInfo );
      }
    }
    else if ( !doEncrypt ) {
      // we try calling the *old* build-in code for OpenPGP clearsigning
      Kpgp::Block block;
      block.setText( encodedBody );

      // clearsign the message
      bOk = block.clearsign( pgpUserId, mCharset );

      if( bOk ) {
        newBodyPart.setType(                       oldBodyPart.type() );
        newBodyPart.setSubtype(                    oldBodyPart.subtype() );
        newBodyPart.setCharset(                    oldBodyPart.charset() );
        newBodyPart.setContentTransferEncodingStr( oldBodyPart.contentTransferEncodingStr() );
        newBodyPart.setContentDescription(         oldBodyPart.contentDescription() );
        newBodyPart.setContentDisposition(         oldBodyPart.contentDisposition() );
        newBodyPart.setBodyEncoded( block.text() );
      }
      else
        KMessageBox::sorry(this, i18n("<qt><p>Signing not done.</p>%1</qt>")
			   .arg( mErrorNoCryptPlugAndNoBuildIn ));
    }
  }

  if( bOk ) {
    // determine the list of public recipients
    QString _to = to().simplifyWhiteSpace();
    if( !cc().isEmpty() ) {
      if( !_to.endsWith(",") )
        _to += ",";
      _to += cc().simplifyWhiteSpace();
    }
    QStringList recipientsWithoutBcc = KMMessage::splitEmailAddrList(_to);

    // run encrypting(s) for Bcc recipient(s)
    if( doEncrypt && !ignoreBcc && !theMessage.bcc().isEmpty() ) {
      QStringList bccRecips = KMMessage::splitEmailAddrList( theMessage.bcc() );
      for( QStringList::ConstIterator it = bccRecips.begin();
          it != bccRecips.end();
          ++it ) {
        QStringList tmpRecips( recipientsWithoutBcc );
        tmpRecips << *it;
        //kdDebug(5006) << "###BEFORE \"" << theMessage.asString() << "\""<< endl;
        KMMessage* yetAnotherMessageForBCC = new KMMessage( theMessage );
        KMMessagePart tmpNewBodyPart = newBodyPart;
        bOk = encryptMessage( yetAnotherMessageForBCC,
                              tmpRecips,
                              doSign, doEncrypt, cryptPlug, encodedBody,
                              previousBoundaryLevel,
                              oldBodyPart,
                              earlyAddAttachments, allAttachmentsAreInBody,
                              tmpNewBodyPart,
                              signCertFingerprint );
        if( bOk ){
          yetAnotherMessageForBCC->setHeaderField( "X-KMail-Recipients", *it );
          bccMsgList.append( yetAnotherMessageForBCC );
          //kdDebug(5006) << "###BCC AFTER \"" << theMessage.asString() << "\""<<endl;
        }
      }
      theMessage.setHeaderField( "X-KMail-Recipients", recipientsWithoutBcc.join(",") );
    }

    // run encrypting for public recipient(s)
    if( bOk )
      bOk = encryptMessage( &theMessage,
                            recipientsWithoutBcc,
                            doSign, doEncrypt, cryptPlug, encodedBody,
                            previousBoundaryLevel,
                            oldBodyPart,
                            earlyAddAttachments, allAttachmentsAreInBody,
                            newBodyPart,
                            signCertFingerprint );
    //        kdDebug(5006) << "###AFTER ENCRYPTION\"" << theMessage.asString() << "\""<<endl;
  }
  return bOk;
}


bool KMComposeWin::queryExit ()
{
  return true;
}

bool KMComposeWin::encryptMessage( KMMessage* msg,
                                   const QStringList& recipients,
                                   bool doSign,
                                   bool doEncrypt,
                                   CryptPlugWrapper* cryptPlug,
                                   const QCString& encodedBody,
                                   int previousBoundaryLevel,
                                   const KMMessagePart& oldBodyPart,
                                   bool earlyAddAttachments,
                                   bool allAttachmentsAreInBody,
                                   KMMessagePart newBodyPart,
                                   QCString& signCertFingerprint )
{
  if(!msg)
  {
    kdDebug(5006) << "KMComposeWin::encryptMessage() : msg == NULL!\n" << endl;
    return FALSE;
  }

  // This c-string (init empty here) is set by *first* testing of expiring
  // encryption certificate: stops us from repeatedly asking same questions.
  QCString encryptCertFingerprints;

  bool bOk = true;
  // encrypt message
  if( doEncrypt ) {
    QCString innerContent;
    if( doSign && cryptPlug ) {
      DwBodyPart* dwPart = msg->createDWBodyPart( &newBodyPart );
      dwPart->Assemble();
      innerContent = dwPart->AsString().c_str();
      delete dwPart;
    } else
      innerContent = encodedBody;

    // now do the encrypting:
    {
      if( cryptPlug ) {
        if( (0 <= cryptPlug->libName().find( "smime",   0, false )) ||
            (0 <= cryptPlug->libName().find( "openpgp", 0, false )) ) {
          // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
          // according to RfC 2633, 3.1.1 Canonicalization
          kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
          innerContent = KMMessage::lf2crlf( innerContent );
          kdDebug(5006) << "                                                       done." << endl;
        }

        StructuringInfoWrapper structuring( cryptPlug );

        QByteArray encryptedBody = pgpEncryptedMsg( innerContent,
                                                    recipients,
                                                    structuring,
                                                    encryptCertFingerprints );

        bOk = ! (encryptedBody.isNull() || encryptedBody.isEmpty());

        if( bOk ) {
          bOk = processStructuringInfo( QString::fromUtf8( cryptPlug->bugURL() ),
                                        previousBoundaryLevel + doEncrypt ? 2 : 1,
                                        newBodyPart.contentDescription(),
                                        newBodyPart.typeStr(),
                                        newBodyPart.subtypeStr(),
                                        newBodyPart.contentDisposition(),
                                        newBodyPart.contentTransferEncodingStr(),
                                        innerContent,
                                        "encrypted data",
                                        encryptedBody,
                                        structuring,
                                        newBodyPart );
          if( bOk ) {
            if( newBodyPart.name().isEmpty() )
              newBodyPart.setName("encrypted message part");
          } else
            KMessageBox::sorry(this, mErrorProcessingStructuringInfo);
        } else
          KMessageBox::sorry(this,
          i18n( "<qt><b>This message could not be encrypted!</b><br>&nbsp;<br>"
                "The Crypto Plug-in %1<br>"
                "did not return an encoded text block.<br>&nbsp;<br>"
                "Recipient's public key was not found or is untrusted.</qt>").arg(cryptPlug->libName()));
      } else {
        // we try calling the *old* build-in code for OpenPGP encrypting
        Kpgp::Block block;
        block.setText( innerContent );

        // get PGP user id for the chosen identity
        const KMIdentity & ident =
          kernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
        QCString pgpUserId = ident.pgpIdentity();

        // encrypt the message
        bOk = block.encrypt( recipients, pgpUserId, doSign, mCharset );

        if( bOk ) {
          newBodyPart.setBodyEncodedBinary( block.text() );
          if( newBodyPart.name().isEmpty() )
            newBodyPart.setName("encrypted message part");
        }
        else
          KMessageBox::sorry(this,
            i18n("<qt><p>Encrypting not done.</p>%1</qt>").arg( mErrorNoCryptPlugAndNoBuildIn ));
      }
    }
  }

  // process the attachments that are not included into the body
  if( bOk ) {
    const KMMessagePart& ourFineBodyPart( (doSign || doEncrypt)
                                          ? newBodyPart
                                          : oldBodyPart );
    if(    mAtmList.count()
        && ( !earlyAddAttachments || !allAttachmentsAreInBody ) ) {
      // set the content type header
      msg->headers().ContentType().FromString( "Multipart/Mixed" );
kdDebug(5006) << "KMComposeWin::encryptMessage() : set top level Content-Type to Multipart/Mixed" << endl;
//      msg->setBody( "This message is in MIME format.\n"
//                    "Since your mail reader does not understand this format,\n"
//                    "some or all parts of this message may not be legible." );
      // add our Body Part
      msg->addBodyPart( &ourFineBodyPart );

      // add Attachments
      // create additional bodyparts for the attachments (if any)
      int idx;
      KMMessagePart newAttachPart;
      KMMessagePart *attachPart;
      for( idx=0, attachPart = mAtmList.first();
           attachPart;
           attachPart = mAtmList.next(), ++idx ) {
kdDebug(5006) << "                                 processing " << idx << ". attachment" << endl;

        bool cryptFlagsDifferent = cryptPlug
                        ? (    (encryptFlagOfAttachment( idx ) != doEncrypt)
                            || (signFlagOfAttachment(    idx ) != doSign) )
                        : false;
        bool encryptThisNow = cryptFlagsDifferent ? encryptFlagOfAttachment( idx ) : false;
        bool signThisNow    = cryptFlagsDifferent ? signFlagOfAttachment(    idx ) : false;

        if( cryptFlagsDifferent || !earlyAddAttachments ) {

          if( encryptThisNow || signThisNow ) {

            KMMessagePart& rEncryptMessagePart( *attachPart );

            // prepare the attachment's content
            DwBodyPart* innerDwPart = msg->createDWBodyPart( attachPart );
            innerDwPart->Assemble();
            QCString encodedAttachment = innerDwPart->AsString().c_str();
            delete innerDwPart;

            if( (0 <= cryptPlug->libName().find( "smime",   0, false )) ||
                (0 <= cryptPlug->libName().find( "openpgp", 0, false )) ) {
              // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
              // according to RfC 2633, 3.1.1 Canonicalization
              kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
              encodedAttachment = KMMessage::lf2crlf( encodedAttachment );
              kdDebug(5006) << "                                                       done." << endl;
            }

            // sign this attachment
            if( signThisNow ) {
kdDebug(5006) << "                                 sign " << idx << ". attachment separately" << endl;
              StructuringInfoWrapper structuring( cryptPlug );

              QByteArray signature = pgpSignedMsg( encodedAttachment,
                                                   structuring,
                                                   signCertFingerprint );
              bOk = !signature.isEmpty();
              if( bOk ) {
                bOk = processStructuringInfo( QString::fromUtf8( cryptPlug->bugURL() ),
                                              previousBoundaryLevel + 10 + idx,
                                              attachPart->contentDescription(),
                                              attachPart->typeStr(),
                                              attachPart->subtypeStr(),
                                              attachPart->contentDisposition(),
                                              attachPart->contentTransferEncodingStr(),
                                              encodedAttachment,
                                              "signature",
                                              signature,
                                              structuring,
                                              newAttachPart );
                if( bOk ) {
                  if( newAttachPart.name().isEmpty() )
                    newAttachPart.setName("signed attachment");
                  if( encryptThisNow ) {
                    rEncryptMessagePart = newAttachPart;
                    DwBodyPart* dwPart = msg->createDWBodyPart( &newAttachPart );
                    dwPart->Assemble();
                    encodedAttachment = dwPart->AsString().c_str();
                    delete dwPart;
                  }
                } else
                    KMessageBox::sorry(this, mErrorProcessingStructuringInfo );
              } else {
                // quit the attachments' loop
                break;
              }
            }
            if( encryptThisNow ) {
kdDebug(5006) << "                                 encrypt " << idx << ". attachment separately" << endl;
              StructuringInfoWrapper structuring( cryptPlug );
              QByteArray encryptedBody = pgpEncryptedMsg( encodedAttachment,
                                                          recipients,
                                                          structuring,
                                                          encryptCertFingerprints );

              bOk = ! (encryptedBody.isNull() || encryptedBody.isEmpty());

              if( bOk ) {
                bOk = processStructuringInfo( QString::fromUtf8( cryptPlug->bugURL() ),
                                              previousBoundaryLevel + 11 + idx,
                                              rEncryptMessagePart.contentDescription(),
                                              rEncryptMessagePart.typeStr(),
                                              rEncryptMessagePart.subtypeStr(),
                                              rEncryptMessagePart.contentDisposition(),
                                              rEncryptMessagePart.contentTransferEncodingStr(),
                                              encodedAttachment,
                                              "encrypted data",
                                              encryptedBody,
                                              structuring,
                                              newAttachPart );
                if( bOk ) {
                  if( newAttachPart.name().isEmpty() ) {
                    newAttachPart.setName("encrypted attachment");
                  }
                } else
                  KMessageBox::sorry(this, mErrorProcessingStructuringInfo);
              }
            }
            msg->addBodyPart( &newAttachPart );
          } else
            msg->addBodyPart( attachPart );

kdDebug(5006) << "                                 added " << idx << ". attachment to this Multipart/Mixed" << endl;
        } else {
kdDebug(5006) << "                                 " << idx << ". attachment was part of the BODY allready" << endl;
        }
      }
    } else {
      if( ourFineBodyPart.originalContentTypeStr() ) {
        msg->headers().ContentType().FromString( ourFineBodyPart.originalContentTypeStr() );
        msg->headers().Parse();
kdDebug(5006) << "KMComposeWin::encryptMessage() : set top level Content-Type from originalContentTypeStr()" << endl;
      } else {
        msg->headers().ContentType().FromString( ourFineBodyPart.typeStr() + "/" + ourFineBodyPart.subtypeStr() );
kdDebug(5006) << "KMComposeWin::encryptMessage() : set top level Content-Type from typeStr()/subtypeStr()" << endl;
      }
      msg->setCharset( ourFineBodyPart.charset() );
      msg->setHeaderField( "Content-Transfer-Encoding",
                            ourFineBodyPart.contentTransferEncodingStr() );
      msg->setHeaderField( "Content-Description",
                            ourFineBodyPart.contentDescription() );
      msg->setHeaderField( "Content-Disposition",
                            ourFineBodyPart.contentDisposition() );
kdDebug(5006) << "KMComposeWin::encryptMessage() : top level headers and body adjusted" << endl;
      // set body content
      msg->setBody( ourFineBodyPart.body() );
    }

  }
  return bOk;
}

//-----------------------------------------------------------------------------
bool KMComposeWin::processStructuringInfo( const QString   bugURL,
                                           uint            boundaryLevel,
                                           const QString   contentDescClear,
                                           const QCString  contentTypeClear,
                                           const QCString  contentSubtypeClear,
                                           const QCString  contentDispClear,
                                           const QCString  contentTEncClear,
                                           const QCString& clearCStr,
                                           const QString   contentDescCiph,
                                           const QByteArray& ciphertext,
                                           const StructuringInfoWrapper& structuring,
                                           KMMessagePart&  resultingPart )
{
#ifdef DEBUG
  QString ds( "||| entering KMComposeWin::processStructuringInfo()" );
  qDebug( ds.utf8() );
#endif
  //assert(mMsg!=NULL);
  if(!mMsg)
  {
    kdDebug(5006) << "KMComposeWin::processStructuringInfo() : mMsg == NULL!\n" << endl;
    return FALSE;
  }

  bool bOk = true;

  if( structuring.data.makeMimeObject ) {

    QCString mainHeader;

    if(    structuring.data.contentTypeMain
        && 0 < strlen( structuring.data.contentTypeMain ) ) {
      mainHeader = "Content-Type: ";
      mainHeader += structuring.data.contentTypeMain;
    } else {
      mainHeader = "Content-Type: ";
      if( structuring.data.makeMultiMime )
        mainHeader += "text/plain";
      else {
        mainHeader += contentTypeClear;
        mainHeader += '/';
        mainHeader += contentSubtypeClear;
      }
    }

    QCString boundaryCStr;  // storing boundary string data
    // add "boundary" parameter

    if( structuring.data.makeMultiMime ) {

      // calculate boundary value
      DwMediaType tmpCT;
      tmpCT.CreateBoundary( boundaryLevel );
      boundaryCStr = tmpCT.Boundary().c_str();
      // remove old "boundary" parameter
      int boundA = mainHeader.find("boundary=", 0,false);
      int boundZ;
      if( -1 < boundA ) {
        // take into account a leading ";  " string
        while(    0 < boundA
               && ' ' == mainHeader[ boundA-1 ] )
            --boundA;
        if(    0 < boundA
            && ';' == mainHeader[ boundA-1 ] )
            --boundA;
        boundZ = mainHeader.find(';', boundA+1);
        if( -1 == boundZ )
          mainHeader.truncate( boundA );
        else
          mainHeader.remove( boundA, (1 + boundZ - boundA) );
      }
      // insert new "boundary" parameter
      QCString bStr( ";boundary=\"" );
      bStr += boundaryCStr;
      bStr += "\"";
      mainHeader += bStr;
    }

    if(    structuring.data.contentTypeMain
        && 0 < strlen( structuring.data.contentTypeMain ) ) {

      if(    structuring.data.contentDispMain
          && 0 < strlen( structuring.data.contentDispMain ) ) {
        mainHeader += "\nContent-Disposition: ";
        mainHeader += structuring.data.contentDispMain;
      }
      if(    structuring.data.contentTEncMain
          && 0 < strlen( structuring.data.contentTEncMain ) ) {

        mainHeader += "\nContent-Transfer-Encoding: ";
        mainHeader += structuring.data.contentTEncMain;
      }

    } else {
      if( 0 < contentDescClear.length() ) {
        mainHeader += "\nContent-Description: ";
        mainHeader += contentDescClear.utf8();
      }
      if( 0 < contentDispClear.length() ) {
        mainHeader += "\nContent-Disposition: ";
        mainHeader += contentDispClear;
      }
      if( 0 < contentTEncClear.length() ) {
        mainHeader += "\nContent-Transfer-Encoding: ";
        mainHeader += contentTEncClear;
      }
    }


    DwString mainDwStr;
    mainDwStr = mainHeader;
    DwBodyPart mainDwPa( mainDwStr, 0 );
    mainDwPa.Parse();
    KMMessage::bodyPart(&mainDwPa, &resultingPart);
/*
qDebug("***************************************");
qDebug("***************************************");
qDebug("***************************************");
qDebug(mainHeader);
qDebug("***************************************");
qDebug("***************************************");
qDebug("***************************************");
qDebug(resultingPart.additionalCTypeParamStr());
qDebug("***************************************");
qDebug("***************************************");
qDebug("***************************************");
*/
    if( ! structuring.data.makeMultiMime ) {

      if( structuring.data.includeCleartext ) {
        QCString bodyText( clearCStr );
        bodyText += '\n';
        bodyText += ciphertext;
        resultingPart.setBodyEncoded( bodyText );
      } else
        resultingPart.setBodyEncodedBinary( ciphertext );

    } else { //  OF  if( ! structuring.data.makeMultiMime )

      QCString versCStr, codeCStr;

      // Build the encapsulated MIME parts.

/*
      if( structuring.data.includeCleartext ) {
        // Build a MIME part holding the cleartext.
        // using the original cleartext's headers and by
        // taking it's original body text.
        KMMessagePart clearKmPa;
        clearKmPa.setContentDescription(         contentDescClear    );
        clearKmPa.setTypeStr(                    contentTypeClear    );
        clearKmPa.setSubtypeStr(                 contentSubtypeClear );
        clearKmPa.setContentDisposition(         contentDispClear    );
        clearKmPa.setContentTransferEncodingStr( contentTEncClear    );
        // store string representation of the cleartext headers
        DwBodyPart* tmpDwPa = mMsg->createDWBodyPart( &clearKmPa );
        tmpDwPa->Headers().SetModified();
        tmpDwPa->Headers().Assemble();
        clearCStr = tmpDwPa->Headers().AsString().c_str();
        delete tmpDwPa;
        // store string representation of encoded cleartext
        clearKmPa.setBodyEncoded( cleartext );
        clearCStr += clearKmPa.body();
      }
*/

      // Build a MIME part holding the version information
      // taking the body contents returned in
      // structuring.data.bodyTextVersion.
      if(    structuring.data.contentTypeVersion
          && 0 < strlen( structuring.data.contentTypeVersion ) ) {

        DwString versStr( "Content-Type: " );
        versStr += structuring.data.contentTypeVersion;

        versStr += "\nContent-Description: ";
        versStr += "version code";

        if(    structuring.data.contentDispVersion
            && 0 < strlen( structuring.data.contentDispVersion ) ) {
          versStr += "\nContent-Disposition: ";
          versStr += structuring.data.contentDispVersion;
        }
        if(    structuring.data.contentTEncVersion
            && 0 < strlen( structuring.data.contentTEncVersion ) ) {
          versStr += "\nContent-Transfer-Encoding: ";
          versStr += structuring.data.contentTEncVersion;
        }

        DwBodyPart versDwPa( versStr, 0 );
        versDwPa.Parse();
        KMMessagePart versKmPa;
        KMMessage::bodyPart(&versDwPa, &versKmPa);
        versKmPa.setBodyEncoded( structuring.data.bodyTextVersion );
        // store string representation of the cleartext headers
        versCStr = versDwPa.Headers().AsString().c_str();
        // store string representation of encoded cleartext
        versCStr += "\n\n";
        versCStr += versKmPa.body();
      }

      // Build a MIME part holding the code information
      // taking the body contents returned in ciphertext.
      if(    structuring.data.contentTypeCode
          && 0 < strlen( structuring.data.contentTypeCode ) ) {

        DwString codeStr( "Content-Type: " );
        codeStr += structuring.data.contentTypeCode;
        if(    structuring.data.contentTEncCode
            && 0 < strlen( structuring.data.contentTEncCode ) ) {
	  codeStr += "\nContent-Transfer-Encoding: ";
          codeStr += structuring.data.contentTEncCode;
	//} else {
        //  codeStr += "\nContent-Transfer-Encoding: ";
	//  codeStr += "base64";
	}
        if( !contentDescCiph.isEmpty() ) {
          codeStr += "\nContent-Description: ";
          codeStr += contentDescCiph.utf8();
        }
        if(    structuring.data.contentDispCode
            && 0 < strlen( structuring.data.contentDispCode ) ) {
          codeStr += "\nContent-Disposition: ";
          codeStr += structuring.data.contentDispCode;
        }

        DwBodyPart codeDwPa( codeStr, 0 );
        codeDwPa.Parse();
        KMMessagePart codeKmPa;
        KMMessage::bodyPart(&codeDwPa, &codeKmPa);
        //if(    structuring.data.contentTEncCode
        //    && 0 < strlen( structuring.data.contentTEncCode ) ) {
        //  codeKmPa.setCteStr( structuring.data.contentTEncCode );
        //} else {
	//  codeKmPa.setCteStr("base64");
        //}
        codeKmPa.setBodyEncodedBinary( ciphertext );
        // store string representation of the cleartext headers
        codeCStr = codeDwPa.Headers().AsString().c_str();
        // store string representation of encoded cleartext
        codeCStr += "\n\n";
        codeCStr += codeKmPa.body();
qDebug("***************************************");
qDebug("***************************************");
qDebug( codeCStr );
qDebug("***************************************");
qDebug("***************************************");
      } else {

        // Plugin error!
        KMessageBox::sorry(this,
            i18n("Error: Cryptography plugin returned\n"
                 "       \" structuring.makeMultiMime \"\n"
                 "but did NOT specify a Content-Type header\n"
                 "for the ciphertext that was generated.\n\n"
                 "Please report this bug.\n( ")
                 + bugURL + " )" );
        bOk = false;
      }

      QCString mainStr;

      mainStr  = "--";
      mainStr +=       boundaryCStr;

      if( structuring.data.includeCleartext && (0 < clearCStr.length()) ) {
        mainStr += "\n";
        mainStr +=     clearCStr;
        mainStr += "\n--";
        mainStr +=       boundaryCStr;
      }
      if( 0 < versCStr.length() ) {
        mainStr += "\n";
        mainStr +=     versCStr;
        mainStr += "\n\n--";
        mainStr +=       boundaryCStr;
      }
      if( 0 < codeCStr.length() ) {
        mainStr += "\n";
        mainStr +=     codeCStr;
        // add the closing boundary string
        mainStr += "\n--";
        mainStr +=       boundaryCStr;
      }
      mainStr +=                     "--\n";

      resultingPart.setBodyEncoded( mainStr );

    } //  OF  if( ! structuring.data.makeMultiMime ) .. else

    /*
    resultingData += mainHeader;
    resultingData += '\n';
    resultingData += mainKmPa.body();
    */

  } else { //  OF  if( structuring.data.makeMimeObject )

    // Build a plain message body
    // based on the values returned in structInf.
    // Note: We do _not_ insert line breaks between the parts since
    //       it is the plugin job to provide us with ready-to-use
    //       texts containing all neccessary line breaks.
    resultingPart.setContentDescription(         contentDescClear    );
    resultingPart.setTypeStr(                    contentTypeClear    );
    resultingPart.setSubtypeStr(                 contentSubtypeClear );
    resultingPart.setContentDisposition(         contentDispClear    );
    resultingPart.setContentTransferEncodingStr( contentTEncClear    );
    QCString resultingBody;

    if(    structuring.data.flatTextPrefix
        && strlen( structuring.data.flatTextPrefix ) )
      resultingBody += structuring.data.flatTextPrefix;
    if( structuring.data.includeCleartext ) {
      if( !clearCStr.isEmpty() )
        resultingBody += clearCStr;
      if(    structuring.data.flatTextSeparator
          && strlen( structuring.data.flatTextSeparator ) )
        resultingBody += structuring.data.flatTextSeparator;
    }
    if(    ciphertext
        && strlen( ciphertext ) )
      resultingBody += *ciphertext;
    else {
        // Plugin error!
        KMessageBox::sorry(this,
            i18n("Error: Cryptography plugin did not return"
                 " any encoded data."
                 "\nPlease report this bug:"
                 "\n" ) + bugURL );
        bOk = false;
    }
    if(    structuring.data.flatTextPostfix
        && strlen( structuring.data.flatTextPostfix ) )
      resultingBody += structuring.data.flatTextPostfix;

    resultingPart.setBodyEncoded( resultingBody );

  } //  OF  if( structuring.data.makeMimeObject ) .. else

  // No need to free the memory that was allocated for the ciphertext
  // since this memory is freed by it's QCString destructor.

  // Neither do we free the memory that was allocated
  // for our structuring info data's char* members since we are using
  // not the pure cryptplug's StructuringInfo struct
  // but the convenient CryptPlugWrapper's StructuringInfoWrapper class.

#ifdef DEBUG
  ds = "||| leaving KMComposeWin::processStructuringInfo()\n||| returning: ";
  ds += bOk ? "TRUE" : "FALSE";
  ds += "\"\n\n";
  qDebug( ds.utf8() );
#endif

  return bOk;
}

//-----------------------------------------------------------------------------
QCString KMComposeWin::breakLinesAndApplyCodec()
{
  QString text;
  QCString cText;

  if (disableBreaking)
      text = mEditor->text();
  else
      text = mEditor->brokenText();

  text.truncate(text.length()); // to ensure text.size()==text.length()+1

  {
    // Provide a local scope for newText.
    QString newText;
    QTextCodec *codec = KMMsgBase::codecForName(mCharset);

    if (mCharset == "us-ascii") {
      cText = KMMsgBase::toUsAscii(text);
      newText = QString::fromLatin1(cText);
    } else if (codec == NULL) {
      kdDebug(5006) << "Something is wrong and I can not get a codec." << endl;
      cText = text.local8Bit();
      newText = QString::fromLocal8Bit(cText);
    } else {
      cText = codec->fromUnicode(text);
      newText = codec->toUnicode(cText);
    }
    if (cText.isNull()) cText = "";

    if (!text.isEmpty() && (newText != text))
    {
      QString oldText = mEditor->text();
      mEditor->setText(newText);
      kernel->kbp()->idle();
      bool anyway = (KMessageBox::warningYesNo(0L,
      i18n("Not all characters fit into the chosen"
      " encoding.\nSend the message anyway?"),
      i18n("Some characters will be lost"),
      i18n("Yes"), i18n("No, let me change the encoding") ) == KMessageBox::Yes);
      if (!anyway)
      {
        mEditor->setText(oldText);
        return QCString();
      }
    }
  }

  return cText;
}


//-----------------------------------------------------------------------------
QByteArray KMComposeWin::pgpSignedMsg( QCString cText,
                                       StructuringInfoWrapper& structuring,
                                       QCString& signCertFingerprint )
{
  QByteArray signature;

  // we call the cryptplug for signing
  CryptPlugWrapper* cryptPlug = mCryptPlugList->active();
  if( cryptPlug ) {
    kdDebug(5006) << "\nKMComposeWin::pgpSignedMsg calling CRYPTPLUG "
                  << cryptPlug->libName() << endl;

    bool bSign = true;

    if( signCertFingerprint.isEmpty() ){
        int certSize = 0;
        QByteArray certificate;
        QString selectedCert;
        KListBoxDialog dialog( selectedCert, "", i18n( "&Select certificate:") );
        dialog.resize( 700, 200 );

        QCString signer = from().utf8();
        signer.replace(QRegExp("\\x0001"), " ");

        kdDebug(5006) << "\n\nRetrieving keys for: " << from() << endl;
        char* certificatePtr = 0;
        bool findCertsOk = cryptPlug->findCertificates(
                                            &(*signer),
                                            &certificatePtr,
                                            &certSize,
                                            true )
                          && (0 < certSize);
        kdDebug(5006) << "keys retrieved ok: " << findCertsOk << endl;

        bool useDialog = false;
        if( findCertsOk ) {
            kdDebug(5006) << "findCertificates() returned " << certificatePtr << endl;
            certificate.assign( certificatePtr, certSize );

            // fill selection dialog listbox
            dialog.entriesLB->clear();
            int iA = 0;
            int iZ = 0;
            while( iZ < certSize ) {
                if( (certificate[iZ] == '\1') || (certificate[iZ] == '\0') ) {
                    char c = certificate[iZ];
                    if( (c == '\1') && !useDialog ) {
                        // set up selection dialog
                        useDialog = true;
                        QString caption( i18n( "Select Certificate" ));
                        caption += " [";
                        caption += from();
                        caption += "]";
                        dialog.setCaption( caption );
                    }
                    certificate[iZ] = '\0';
                    QString s = QString::fromUtf8( &certificate[iA] );
                    certificate[iZ] = c;
                    if( useDialog )
                        dialog.entriesLB->insertItem( s );
                    else
                        selectedCert = s;
                    ++iZ;
                    iA = iZ;
                }
                ++iZ;
            }

            // run selection dialog and retrieve user choice
            // OR take the single entry (if only one was found)
            if( useDialog ) {
                dialog.entriesLB->setFocus();
                dialog.entriesLB->setSelected( 0, true );
                bSign = (dialog.exec() == QDialog::Accepted);
            }

            if (bSign) {
                signCertFingerprint = selectedCert.utf8();
                signCertFingerprint.remove( 0, signCertFingerprint.findRev( '(' )+1 );
                signCertFingerprint.truncate( signCertFingerprint.length()-1 );
                kdDebug(5006) << "\n\n                      Signer: " << from()
                              <<   "\nFingerprint of signature key: " << QString( signCertFingerprint ) << "\n\n" << endl;
                if( signCertFingerprint.isEmpty() )
                    bSign = false;
            }
        }


/* ----------------------------- */
#ifdef DEBUG
        QString ds( "\n\nBEFORE calling cryptplug:" );
        ds += "\nstructuring.contentTypeMain:   \"";
        ds += structuring.data.contentTypeMain;
        ds += "\"";
        ds += "\nstructuring.contentTypeVersion:\"";
        ds += structuring.data.contentTypeVersion;
        ds += "\"";
        ds += "\nstructuring.contentTypeCode:   \"";
        ds += structuring.data.contentTypeCode;
        ds += "\"";
        ds += "\nstructuring.flatTextPrefix:    \"";
        ds += structuring.data.flatTextPrefix;
        ds += "\"";
        ds += "\nstructuring.flatTextSeparator: \"";
        ds += structuring.data.flatTextSeparator;
        ds += "\"";
        ds += "\nstructuring.flatTextPostfix:   \"";
        ds += structuring.data.flatTextPostfix;
        ds += "\"";
        kdDebug(5006) << ds.utf8();
#endif

        // Check for expiry of the signer, CA, and Root certificate.
        // Only do the expiry check if the plugin has this feature
        // and if there in *no* fingerprint in signCertFingerprint already.
        if( cryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) ){
            int sigDaysLeft = cryptPlug->signatureCertificateDaysLeftToExpiry( signCertFingerprint );
            if( cryptPlug->signatureCertificateExpiryNearWarning() &&
                sigDaysLeft <
                cryptPlug->signatureCertificateExpiryNearInterval() ) {
                int ret = KMessageBox::warningYesNo( this,
                                                    i18n( "<qt>The certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer.<br>Do you still want to use this signature?</qt>" ).arg( sigDaysLeft ),
                                                    i18n( "Certificate Warning" ) );
                if( ret == KMessageBox::No )
                    bSign = false;
            }

            if( bSign ) {
                int rootDaysLeft = cryptPlug->rootCertificateDaysLeftToExpiry( signCertFingerprint );
                if( cryptPlug->rootCertificateExpiryNearWarning() &&
                    rootDaysLeft <
                    cryptPlug->rootCertificateExpiryNearInterval() ) {
                    int ret = KMessageBox::warningYesNo( this,
                                                        i18n( "<qt>The root certificate of the certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer.<br>Do you still want to use this signature?</qt>" ).arg( rootDaysLeft ),
                                                        i18n( "Certificate Warning" ) );
                    if( ret == KMessageBox::No )
                        bSign = false;
                }
            }


            if( bSign ) {
                int caDaysLeft = cryptPlug->caCertificateDaysLeftToExpiry( signCertFingerprint );
                if( cryptPlug->caCertificateExpiryNearWarning() &&
                    caDaysLeft <
                    cryptPlug->caCertificateExpiryNearInterval() ) {
                    int ret = KMessageBox::warningYesNo( this,
                                                        i18n( "<qt>The CA certificate of the certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer.<br>Do you still want to use this signature?</qt>" ).arg( caDaysLeft ),
                                                        i18n( "Certificate Warning" ) );
                    if( ret == KMessageBox::No )
                        bSign = false;
                }
            }
        }
        // Check whether the sender address of the signer is contained in
        // the certificate, but only do this if the plugin has this feature.
        if( cryptPlug->hasFeature( Feature_WarnSignEmailNotInCertificate ) ) {
            if( bSign && cryptPlug->warnNoCertificate() &&
                !cryptPlug->isEmailInCertificate( QString( KMMessage::getEmailAddr( from() ) ).utf8(), signCertFingerprint ) )  {
                int ret = KMessageBox::warningYesNo( this,
                                                    i18n( "<qt>The certificate does not contain your sender email address.<br>This means that it is not possible for the recipients to check whether the email really came from you.<br>Do you still want to use this signature?</qt>" ),
                                                    i18n( "Certificate Warning" ) );
                if( ret == KMessageBox::No )
                    bSign = false;
            }
        }
    } // if( signCertFingerprint.isEmpty() )


    // Finally sign the message, but only if the plugin has this feature.
    if( cryptPlug->hasFeature( Feature_SignMessages ) ) {
        size_t cipherLen;

        const char* cleartext = cText;
        char* ciphertext  = 0;

//#define KHZ_TEST
#ifdef KHZ_TEST
        QFile fileS( "testdat_sign.input" );
        if( fileS.open( IO_WriteOnly ) ) {
            QDataStream ds( &fileS );
            ds.writeRawBytes( cleartext, strlen( cleartext ) );
            fileS.close();
        }
#endif
        if ( bSign ){
            int   errId = 0;
            char* errTxt = 0;
            if ( cryptPlug->signMessage( cleartext,
                                         &ciphertext, &cipherLen,
                                         signCertFingerprint,
                                         structuring,
                                         &errId,
                                         &errTxt ) ){
#ifdef KHZ_TEST
                QFile fileD( "testdat_sign.output" );
                if( fileD.open( IO_WriteOnly ) ) {
                    QDataStream ds( &fileD );
                    ds.writeRawBytes( ciphertext, cipherLen );
                    fileD.close();
                }
#endif
#ifdef DEBUG
                QString ds( "\nAFTER calling cryptplug:" );
                ds += "\nstructuring.contentTypeMain:   \"";
                ds += structuring.data.contentTypeMain;
                ds += "\"";
                ds += "\nstructuring.contentTypeVersion:\"";
                ds += structuring.data.contentTypeVersion;
                ds += "\"";
                ds += "\nstructuring.contentTypeCode:   \"";
                ds += structuring.data.contentTypeCode;
                ds += "\"";
                ds += "\nstructuring.flatTextPrefix:    \"";
                ds += structuring.data.flatTextPrefix;
                ds += "\"";
                ds += "\nstructuring.flatTextSeparator: \"";
                ds += structuring.data.flatTextSeparator;
                ds += "\"";
                ds += "\nstructuring.flatTextPostfix:   \"";
                ds += structuring.data.flatTextPostfix;
                ds += "\"";
                ds += "\n\nresulting signature bloc:\n\"";
                ds += ciphertext;
                ds += "\"\n\n";
                ds += "signature length: ";
                ds += cipherLen;
                ds += "\n\n";
                kdDebug(5006) << ds.utf8();
#endif
                signature.assign( ciphertext, cipherLen );
            } else {
                QString error("#");
                error += QString::number( errId );
                error += "  :  ";
                if( errTxt )
                  error += errTxt;
                else
                  error += i18n("[unknown error]");
                KMessageBox::sorry(this,
                  i18n("<b>This message could not be signed!</b><br>&nbsp;<br>"
                      "The Crypto Plug-In %1<br>"
                      "reported the following details:<br>&nbsp;<br>&nbsp; &nbsp; <i>%2</i><br>&nbsp;<br>"
                      "Your configuration might be invalid or the Plug-In damaged.<br>&nbsp;<br>"
                      "<b>PLEASE CONTACT YOUR SYSTEM ADMINISTRATOR</b>").arg(cryptPlug->libName()).arg( error ) );
            }
            // we do NOT call a "delete ciphertext" !
            // since "signature" will take care for it (is a QByteArray)
            delete errTxt;
        }
    }

    // PENDING(khz,kalle) Warn if there was no signature? (because of
    // a problem or because the plugin does not allow signing?

/* ----------------------------- */


    kdDebug(5006) << "\nKMComposeWin::pgpSignedMsg returning from CRYPTPLUG.\n" << endl;
  } else
      KMessageBox::sorry(this,
          i18n("No active Crypto Plug-In could be found.\n"
                "Please activate a Plug-In using the 'Settings/Configure KMail / Plug-In' dialog."));
  return signature;
}


//-----------------------------------------------------------------------------
QByteArray KMComposeWin::pgpEncryptedMsg( QCString cText, const QStringList& recipients,
                                          StructuringInfoWrapper& structuring,
                                          QCString& encryptCertFingerprints )
{
  QByteArray encoding;

  // we call the cryptplug
  CryptPlugWrapper* cryptPlug = mCryptPlugList->active();
  if( cryptPlug ) {
    kdDebug(5006) << "\nKMComposeWin::pgpEncryptedMsg: going to call CRYPTPLUG "
                  << cryptPlug->libName() << endl;


    bool bEncrypt = true;
    // Check for CRL expiry, but only if the plugin has this
    // feature.
    if( encryptCertFingerprints.isEmpty() &&
        cryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) &&
        cryptPlug->hasFeature( Feature_EncryptionCRLs ) ) {
        int crlDaysLeft = cryptPlug->encryptionCRLsDaysLeftToExpiry();
        if( cryptPlug->encryptionUseCRLs() &&
            cryptPlug->encryptionCRLExpiryNearWarning() &&
            crlDaysLeft <
            cryptPlug->encryptionCRLNearExpiryInterval() ) {
            int ret = KMessageBox::warningYesNo( this,
                                                 i18n( "The certification revocation lists that are used for checking the validity of the certificate you want to use for encrypting expire in %1 days.\n\nDo you still want to encrypt this message?" ).arg( crlDaysLeft ),
                                                 i18n( "Certificate Warning" ) );
            if( ret == KMessageBox::No )
                bEncrypt = false;
        }
    }

    // PENDING(khz,kalle) Warn if no encryption?

    if( !bEncrypt ) return encoding;



    const char* cleartext  = cText;
    const char* ciphertext = 0;


    if( encryptCertFingerprints.isEmpty() ){

      QString selectedCert;
      KListBoxDialog dialog( selectedCert, "", i18n( "&Select certificate:" ) );
      dialog.resize( 700, 200 );
      bool useDialog;
      int certSize = 0;
      QByteArray certificateList;

      for( QStringList::ConstIterator it = recipients.begin(); it != recipients.end(); ++it ) {
        QCString addressee = (*it).utf8();
        addressee.replace(QRegExp("\\x0001"), " ");
        kdDebug(5006) << "\n\n1st try:  Retrieving keys for: " << *it << endl;


        bool askForDifferentSearchString = false;
        do {

          certSize = 0;
          char* certificatePtr = 0;
          bool findCertsOk;
          if( askForDifferentSearchString )
            findCertsOk = false;
          else {
            findCertsOk = cryptPlug->findCertificates( &(*addressee),
                                                      &certificatePtr,
                                                      &certSize,
                                                      false )
                          && (0 < certSize);
            kdDebug(5006) << "         keys retrieved successfully: " << findCertsOk << "\n" << endl;
            qDebug( "findCertificates() 1st try returned %s", certificatePtr );
            if( findCertsOk )
              certificateList.assign( certificatePtr, certSize );
          }
          while( !findCertsOk ) {
            bool bOk = false;
            addressee = KLineEditDlg::getText(
                          askForDifferentSearchString
                          ? i18n("Look for other certificates")
                          : i18n("No certificate found"),
                          i18n("Enter different address for recipient %1 "
                              "or enter \" * \" to see all certificates:").arg(*it),
                          addressee, &bOk, this ).stripWhiteSpace().utf8();
            askForDifferentSearchString = false;
            if( bOk ) {
              addressee = addressee.simplifyWhiteSpace();
              if( ("\"*\"" == addressee) ||
                  ("\" *\"" == addressee) ||
                  ("\"* \"" == addressee) ||
                  ("\" * \"" == addressee))  // You never know what users type.  :-)
                addressee = "*";
              kdDebug(5006) << "\n\nnext try: Retrieving keys for: " << addressee << endl;
              certSize = 0;
              char* certificatePtr = 0;
              findCertsOk = cryptPlug->findCertificates(
                                            &(*addressee),
                                            &certificatePtr,
                                            &certSize,
                                            false )
                            && (0 < certSize);
              kdDebug(5006) << "         keys retrieved successfully: " << findCertsOk << "\n" << endl;
              qDebug( "findCertificates() 2nd try returned %s", certificatePtr );
              if( findCertsOk )
                certificateList.assign( certificatePtr, certSize );
            } else {
              bEncrypt = false;
              break;
            }
          }
          if( bEncrypt && findCertsOk ) {

            // fill selection dialog listbox
            dialog.entriesLB->clear();
            // show dialog even if only one entry to allow specifying of
            // another search string _instead_of_ the recipients address
            bool bAllwaysShowDialog = true;

            useDialog = false;
            int iA = 0;
            int iZ = 0;
            while( iZ < certSize ) {
              if( (certificateList.at(iZ) == '\1') || (certificateList.at(iZ) == '\0') ) {
                kdDebug(5006) << "iA=" << iA << " iZ=" << iZ << endl;
                char c = certificateList.at(iZ);
                if( (bAllwaysShowDialog || (c == '\1')) && !useDialog ) {
                  // set up selection dialog
                  useDialog = true;
                  QString caption( i18n( "Select certificate for encryption" ));
                  caption += " [";
                  caption += *it;
                  caption += "]";
                  dialog.setCaption( caption );
                  dialog.setLabelAbove(
                    i18n( "&Select certificate for recipient %1:" )
                    .arg( *it ) );
                }
                certificateList.at(iZ) = '\0';
                QString s = QString::fromUtf8( &certificateList.at(iA) );
                certificateList.at(iZ) = c;
                if( useDialog )
                  dialog.entriesLB->insertItem( s );
                else
                  selectedCert = s;
                ++iZ;
                iA = iZ;
              }
              ++iZ;
            }
            // run selection dialog and retrieve user choice
            // OR take the single entry (if only one was found)
            if( useDialog ) {
              dialog.setCommentBelow(
                i18n("(Certificates matching address \"%1\", "
                     "press [Cancel] to use different address for recipient %2.)")
                .arg(addressee)
                .arg(*it) );
              dialog.entriesLB->setFocus();
              dialog.entriesLB->setSelected( 0, true );
              askForDifferentSearchString = (dialog.exec() != QDialog::Accepted);
            }
          }
        } while ( askForDifferentSearchString );


        if( bEncrypt ) {
          QCString certFingerprint = selectedCert.utf8();
          certFingerprint.remove( 0, certFingerprint.findRev( '(' )+1 );
          certFingerprint.truncate( certFingerprint.length()-1 );
          kdDebug(5006) << "\n\n                    Recipient: " << *it
                    <<   "\nFingerprint of encryption key: " << QString( certFingerprint ) << "\n\n" << endl;

          // Check for expiry of various certificates, but only if the
          // plugin supports this.
          if( cryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) ) {
              QString captionWarn( i18n( "Certificate Warning" ) );
              captionWarn += " [";
              captionWarn += *it;
              captionWarn += "]";
              if( bEncrypt ) {
                  int encRecvDaysLeft = cryptPlug->receiverCertificateDaysLeftToExpiry( certFingerprint );
                  if( cryptPlug->receiverCertificateExpiryNearWarning() &&
                      encRecvDaysLeft <
                      cryptPlug->receiverCertificateExpiryNearWarningInterval() ) {
                      int ret = KMessageBox::warningYesNo( this,
                                                          i18n( "<qt>The certificate of the recipient you want to use expires in %1 days.<br>This means that after this period, the recipient will not be able to read your message any longer.\n\nDo you still want to use this certificate?</qt>" ).arg( encRecvDaysLeft ),
                                                          captionWarn );
                      if( ret == KMessageBox::No )
                          bEncrypt = false;
                  }
              }

              if( bEncrypt ) {
                  int certInChainDaysLeft = cryptPlug->certificateInChainDaysLeftToExpiry( certFingerprint );
                  if( cryptPlug->certificateInChainExpiryNearWarning() &&
                      certInChainDaysLeft <
                      cryptPlug->certificateInChainExpiryNearWarningInterval() ) {
                      int ret = KMessageBox::warningYesNo( this,
                                                          i18n( "One of the certificates in the chain of the certificate of the recipient you want to send this message to expires in %1 days.\nThis means that after this period, the recipient will not be able to read your message any longer.\n\nDo you still want to use this certificate?" ).arg( certInChainDaysLeft ),
                                                          captionWarn );
                      if( ret == KMessageBox::No )
                          bEncrypt = false;
                  }
              }

              /*  The following test is not neccessary, since we _got_ the certificate
                  by looking for all certificates of our addressee - so it _must_ be valid
                  for the respective address!

                  // Check whether the receiver address is contained in
                  // the certificate.
                  if( bEncrypt && cryptPlug->receiverEmailAddressNotInCertificateWarning() &&
                  !cryptPlug->isEmailInCertificate( QString( KMMessage::getEmailAddr( recipient ) ).utf8(),
                  certFingerprint ) )  {
                  int ret = KMessageBox::warningYesNo( this,
                  i18n( "The certificate does not contain the email address of the sender.\nThis means that it will not be possible for the recipient to read this message.\n\nDo you still want to use this certificate?" ),
                  captionWarn );
                  if( ret == KMessageBox::No )
                  bEncrypt = false;
                  }
              */
          }

          if( bEncrypt ) {
            if( !encryptCertFingerprints.isEmpty() )
              encryptCertFingerprints += '\1';
            encryptCertFingerprints += certFingerprint;
          }
          else
            break;
        }

        if( !bEncrypt )  break;

      }

    } // if( encryptCertFingerprints.isEmpty() )


    // Actually do the encryption, if the plugin supports this
    size_t cipherLen;
    if ( bEncrypt ) {
      int errId = 0;
      char* errTxt = 0;
      if( cryptPlug->hasFeature( Feature_EncryptMessages ) &&
          cryptPlug->encryptMessage( cleartext,
                                     &ciphertext, &cipherLen,
                                     encryptCertFingerprints,
                                     structuring,
                                     &errId,
                                     &errTxt )
          && ciphertext )
        encoding.assign( ciphertext, cipherLen );
      else {
        bEncrypt = false;
        QString error("#");
        error += QString::number( errId );
        error += "  :  ";
        if( errTxt )
          error += errTxt;
        else
          error += i18n("[unknown error]");
        KMessageBox::sorry(this,
          i18n("<b>This message could not be encrypted!</b><br>&nbsp;<br>"
              "The Crypto Plug-In %1<br>"
              "reported the following details:<br>&nbsp;<br>&nbsp; &nbsp; <i>%2</i><br>&nbsp;<br>"
              "Your configuration might be invalid or the Plug-In damaged.<br>&nbsp;<br>"
              "<b>PLEASE CONTACT YOUR SYSTEM ADMINISTRATOR</b>").arg(cryptPlug->libName()).arg( error ) );
      }
      delete errTxt;
    }

    // we do NOT delete the "ciphertext" !
    // bacause "encoding" will take care for it (is a QByteArray)

    kdDebug(5006) << "\nKMComposeWin::pgpEncryptedMsg: returning from CRYPTPLUG.\n" << endl;

  } else
      KMessageBox::sorry(this,
          i18n("No active Crypto Plug-In could be found.\n"
                "Please activate a Plug-In using the 'Settings/Configure KMail / Plug-In' dialog."));

  return encoding;
}






/*
     // old HEAD code:

//-----------------------------------------------------------------------------
QCString KMComposeWin::pgpProcessedMsg(void)
{
  Kpgp::Module *pgp = Kpgp::Module::getKpgp();
  Kpgp::Block block;
  bool doSign = signAction->isChecked();
  bool doEncrypt = encryptAction->isChecked();
  QString text;
  QCString cText;

  if (disableBreaking)
      text = mEditor->text();
  else
      text = mEditor->brokenText();

  text.truncate(text.length()); // to ensure text.size()==text.length()+1

  {
    // Provide a local scope for newText.
    QString newText;
    QTextCodec *codec = KMMsgBase::codecForName(mCharset);

    if (mCharset == "us-ascii") {
      cText = KMMsgBase::toUsAscii(text);
      newText = QString::fromLatin1(cText);
    } else if (codec == NULL) {
      kdDebug(5006) << "Something is wrong and I can not get a codec." << endl;
      cText = text.local8Bit();
      newText = QString::fromLocal8Bit(cText);
    } else {
      cText = codec->fromUnicode(text);
      newText = codec->toUnicode(cText);
    }
    if (cText.isNull()) cText = "";

    if (!text.isEmpty() && (newText != text))
    {
      QString oldText = mEditor->text();
      mEditor->setText(newText);
      kernel->kbp()->idle();
      bool anyway = (KMessageBox::warningYesNo(0L,
      i18n("Not all characters fit into the chosen"
      " encoding.\nSend the message anyway?"),
      i18n("Some Characters Will Be Lost"),
      i18n("Yes"), i18n("No, Let Me Change the Encoding") ) == KMessageBox::Yes);
      if (!anyway)
      {
        mEditor->setText(oldText);
        return QCString();
      }
    }
  }

  // determine the list of recipients
  QString _to = to();
  if(!cc().isEmpty()) _to += "," + cc();
  if(!bcc().isEmpty()) _to += "," + bcc();

  QStringList recipients = KMMessage::splitEmailAddrList(_to);

  if( mAutoPgpEncrypt && !doEncrypt )
  { // check if the message should be encrypted
    int status = pgp->encryptionPossible( recipients );
    if( status == 1 )
      doEncrypt = true;
    else if( status == 2 )
    { // the user wants to be asked or has to be asked
      kernel->kbp()->idle();
      int ret =
        KMessageBox::questionYesNo( this,
                                    i18n("Should this message be encrypted?") );
      kernel->kbp()->busy();
      doEncrypt = ( ret == KMessageBox::Yes );
    }
    else if( status == -1 )
    { // warn the user that there are conflicting encryption preferences
      kernel->kbp()->idle();
      int ret =
        KMessageBox::warningYesNoCancel( this,
                                         i18n("There are conflicting encryption "
                                         "preferences!\n"
                                         "Should this message be encrypted?") );
      kernel->kbp()->busy();
      if( ret == KMessageBox::Cancel )
        return QCString();
      doEncrypt = ( ret == KMessageBox::Yes );
    }
  }

  if (!doSign && !doEncrypt) return cText;

  block.setText( cText );

  // get PGP user id for the chosen identity
  QCString pgpUserId =
    kernel->identityManager()->identityForUoidOrDefault( mId ).pgpIdentity();

  if (!doEncrypt)
  { // clearsign the message
    if( block.clearsign( pgpUserId, mCharset ) )
      return block.text();
  }
  else
  { // encrypt the message
    if( block.encrypt( recipients, pgpUserId, doSign, mCharset ) )
      return block.text();
  }

  // in case of an error we end up here
  //qWarning(i18n("Error during PGP:") + QString("\n") +
  //	  pgp->lastErrorMsg());

  return QCString();
}
*/

//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(const KURL aUrl)
{
  KIO::Job *job = KIO::get(aUrl);
  atmLoadData ld;
  ld.url = aUrl;
  ld.data = QByteArray();
  ld.insert = false;
  ld.mimeType = KMimeType::findByURL(aUrl, 0, aUrl.isLocalFile())
    ->name().latin1();
  mapAtmLoadData.insert(job, ld);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotAttachFileResult(KIO::Job *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotAttachFileData(KIO::Job *, const QByteArray &)));
}


//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(const KMMessagePart* msgPart)
{
  mAtmList.append(msgPart);

  // show the attachment listbox if it does not up to now
  if (mAtmList.count()==1)
  {
    mGrid->setRowStretch(mNumHeaders+1, 50);
    mAtmListBox->setMinimumSize(100, 80);
    mAtmListBox->setMaximumHeight( 100 );
    mAtmListBox->show();
    resize(size());
    enableAttachActions();
  }

  // add a line in the attachment listbox
  KMAtmListViewItem *lvi = new KMAtmListViewItem( mAtmListBox );
  msgPartToItem(msgPart, lvi);
  mAtmItemList.append(lvi);
}


//-----------------------------------------------------------------------------
void KMComposeWin::enableAttachActions()
{
  bool enable = mAtmList.count() > 0;
  attachRemoveAction->setEnabled(enable);
  attachSaveAction->setEnabled(enable);
  attachPropertiesAction->setEnabled(enable);
}


//-----------------------------------------------------------------------------

QString KMComposeWin::prettyMimeType( const QString& type )
{
  QString t = type.lower();
  KServiceType::Ptr st = KServiceType::serviceType( t );
  return st ? st->comment() : t;
}

void KMComposeWin::msgPartToItem(const KMMessagePart* msgPart,
                                 KMAtmListViewItem *lvi)
{
  assert(msgPart != NULL);

  if (!msgPart->fileName().isEmpty())
    lvi->setText(0, msgPart->fileName());
  else
    lvi->setText(0, msgPart->name());
  lvi->setText(1, KIO::convertSize( msgPart->decodedSize()));
  lvi->setText(2, msgPart->contentTransferEncodingStr());
  lvi->setText(3, prettyMimeType(msgPart->typeStr() + "/" + msgPart->subtypeStr()));
  if( mCryptPlugList && mCryptPlugList->active() ) {
    mAtmListBox->setColumnWidth( mAtmColEncrypt, mAtmCryptoColWidth );
    mAtmListBox->setColumnWidth( mAtmColSign,    mAtmCryptoColWidth );
    lvi->enableCryptoCBs( true );
    lvi->setEncrypt( encryptAction->isChecked() );
    lvi->setSign(    signAction->isChecked() );
  } else {
    mAtmListBox->setColumnWidth( mAtmColEncrypt, 0 );
    mAtmListBox->setColumnWidth( mAtmColSign,    0 );
    lvi->enableCryptoCBs( false );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::removeAttach(const QString &aUrl)
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
  mAtmModified = TRUE;
  mAtmList.remove(idx);
  delete mAtmItemList.take(idx);

  if (mAtmList.count()<=0)
  {
    mAtmListBox->hide();
    mGrid->setRowStretch(mNumHeaders+1, 0);
    mAtmListBox->setMinimumSize(0, 0);
    resize(size());
    enableAttachActions();
  }
}


//-----------------------------------------------------------------------------
bool KMComposeWin::encryptFlagOfAttachment(int idx)
{
  return (int)(mAtmItemList.count()) > idx
    ? ((KMAtmListViewItem*)(mAtmItemList.at( idx )))->isEncrypt()
    : false;
}


//-----------------------------------------------------------------------------
bool KMComposeWin::signFlagOfAttachment(int idx)
{
  return (int)(mAtmItemList.count()) > idx
    ? ((KMAtmListViewItem*)(mAtmItemList.at( idx )))->isSign()
    : false;
}


//-----------------------------------------------------------------------------
void KMComposeWin::addrBookSelInto(KMLineEdit* aLineEdit)
{
  KMAddrBookSelDlg dlg(this);
  QString txt;

  //assert(aLineEdit!=NULL);
  if(!aLineEdit)
    {
      kdDebug(5006) << "KMComposeWin::addrBookSelInto() : aLineEdit == NULL\n" << endl;
      return;
    }
  if (dlg.exec()==QDialog::Rejected) return;
  txt = aLineEdit->text().stripWhiteSpace();
  if (!txt.isEmpty())
    {
      if (txt.right(1).at(0)!=',') txt += ", ";
      else txt += ' ';
    }
  aLineEdit->setText(txt + dlg.address());
}


//-----------------------------------------------------------------------------
void KMComposeWin::setCharset(const QCString& aCharset, bool forceDefault)
{
  if ((forceDefault && mForceReplyCharset) || aCharset.isEmpty())
    mCharset = mDefCharset;
  else
    mCharset = aCharset.lower();

  if ((mCharset=="") || (mCharset == "default"))
     mCharset = mDefCharset;

  if (bAutoCharset)
  {
    encodingAction->setCurrentItem( 0 );
    return;
  }

  QStringList encodings = encodingAction->items();
  int i = 0;
  bool charsetFound = FALSE;
  for ( QStringList::Iterator it = encodings.begin(); it != encodings.end();
     ++it, i++ )
  {
    if (i > 0 && ((mCharset == "us-ascii" && i == 1) ||
     (i != 1 && KGlobal::charsets()->codecForName(
      KGlobal::charsets()->encodingForName(*it))
      == KGlobal::charsets()->codecForName(mCharset))))
    {
      encodingAction->setCurrentItem( i );
      slotSetCharset();
      charsetFound = TRUE;
      break;
    }
  }
  if (!aCharset.isEmpty() && !charsetFound) setCharset("", TRUE);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBook()
{
  KMAddrBookExternal::launch(this);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookFrom()
{
  addrBookSelInto(mEdtFrom);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookReplyTo()
{
  addrBookSelInto(mEdtReplyTo);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookTo()
{
  addrBookSelInto(mEdtTo);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookCc()
{
  addrBookSelInto(mEdtCc);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookBcc()
{
  addrBookSelInto(mEdtBcc);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFile()
{
  // Create File Dialog and return selected file(s)
  // We will not care about any permissions, existence or whatsoever in
  // this function.

  KURL::List files = KFileDialog::getOpenURLs(QString::null, QString::null,
	this, i18n("Attach File"));
  for (KURL::List::Iterator it = files.begin(); it != files.end(); ++it)
    addAttach(*it);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFileData(KIO::Job *job, const QByteArray &data)
{
  QMap<KIO::Job*, atmLoadData>::Iterator it = mapAtmLoadData.find(job);
  assert(it != mapAtmLoadData.end());
  QBuffer buff((*it).data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(data.data(), data.size());
  buff.close();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFileResult(KIO::Job *job)
{
  QMap<KIO::Job*, atmLoadData>::Iterator it = mapAtmLoadData.find(job);
  assert(it != mapAtmLoadData.end());
  if (job->error())
  {
    mapAtmLoadData.remove(it);
    job->showErrorDialog();
    return;
  }
  if ((*it).insert)
  {
    int col, line;
    mEditor->getCursorPosition(&line, &col);
    (*it).data.resize((*it).data.size() + 1);
    (*it).data[(*it).data.size() - 1] = '\0';
    QTextCodec *codec = KGlobal::charsets()->codecForName((*it).encoding);
    if (codec)
      mEditor->insertAt(codec->toUnicode((*it).data), line, col);
    else
      mEditor->insertAt(QString::fromLocal8Bit((*it).data), line, col);
    mapAtmLoadData.remove(it);
    return;
  }
  QString name;
  QString urlStr = (*it).url.prettyURL();
  KMMessagePart* msgPart;
  int i;

  kernel->kbp()->busy();
  i = urlStr.findRev('/');
  name = (i>=0 ? urlStr.mid(i+1, 256) : urlStr);

  QCString encoding = KMMessage::autoDetectCharset(mCharset,
    KMMessage::preferredCharsets(), name);
  if (encoding.isEmpty()) encoding = "utf-8";
  QCString encName = KMMsgBase::encodeRFC2231String(name, encoding);
  bool RFC2231encoded = name != QString(encName);

  // create message part
  msgPart = new KMMessagePart;
  msgPart->setName(name);
  QValueList<int> allowedCTEs;
  msgPart->setBodyAndGuessCte((*it).data, allowedCTEs,
			      !kernel->msgSender()->sendQuotedPrintable());
  kdDebug(5006) << "autodetected cte: " << msgPart->cteStr() << endl;
  int slash = (*it).mimeType.find("/");
  if (slash == -1) slash = (*it).mimeType.length();
  msgPart->setTypeStr((*it).mimeType.left(slash));
  msgPart->setSubtypeStr((*it).mimeType.mid(slash+1));
  msgPart->setContentDisposition(QCString("attachment; filename")
    + ((RFC2231encoded) ? "*" : "") +  "=\"" + encName + "\"");

  mapAtmLoadData.remove(it);

  kernel->kbp()->idle();
  msgPart->setCharset(mCharset);

  // show message part dialog, if not configured away (default):
  KConfigGroup composer(kapp->config(), "Composer");
  if (!composer.hasKey("showMessagePartDialogOnAttach"))
    // make it visible in the config file:
    composer.writeEntry("showMessagePartDialogOnAttach", false);
  if (composer.readBoolEntry("showMessagePartDialogOnAttach", false)) {
    KMMsgPartDialogCompat dlg;
    int encodings = 0;
    for ( QValueListConstIterator<int> it = allowedCTEs.begin() ;
	  it != allowedCTEs.end() ; ++it )
      switch ( *it ) {
      case DwMime::kCteBase64: encodings |= KMMsgPartDialog::Base64; break;
      case DwMime::kCteQp: encodings |= KMMsgPartDialog::QuotedPrintable; break;
      case DwMime::kCte7bit: encodings |= KMMsgPartDialog::SevenBit; break;
      case DwMime::kCte8bit: encodings |= KMMsgPartDialog::EightBit; break;
      default: ;
      }
    dlg.setShownEncodings( encodings );
    dlg.setMsgPart(msgPart);
    if (!dlg.exec()) {
      delete msgPart;
      return;
    }
  }
  mAtmModified = TRUE;
  if (msgPart->typeStr().lower() != "text") msgPart->setCharset(QCString());

  // add the new attachment to the list
  addAttach(msgPart);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertFile()
{
  KFileDialog fdlg(QString::null, QString::null, this, NULL, TRUE);
  fdlg.setCaption(i18n("Insert File"));
  fdlg.toolBar()->insertCombo(KMMsgBase::supportedEncodings(FALSE), 4711,
    false, NULL, NULL, NULL);
  KComboBox *combo = fdlg.toolBar()->getCombo(4711);
  for (int i = 0; i < combo->count(); i++)
    if (KGlobal::charsets()->codecForName(KGlobal::charsets()->
      encodingForName(combo->text(i)))
      == QTextCodec::codecForLocale()) combo->setCurrentItem(i);
  if (!fdlg.exec()) return;

  KURL u = fdlg.selectedURL();

  if (u.fileName().isEmpty()) return;

  KIO::Job *job = KIO::get(u);
  atmLoadData ld;
  ld.url = u;
  ld.data = QByteArray();
  ld.insert = true;
  ld.encoding = KGlobal::charsets()->encodingForName(
    combo->currentText()).latin1();
  mapAtmLoadData.insert(job, ld);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotAttachFileResult(KIO::Job *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotAttachFileData(KIO::Job *, const QByteArray &)));
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSetCharset()
{
  if (encodingAction->currentItem() == 0)
  {
    bAutoCharset = true;
    return;
  }
  bAutoCharset = false;

  mCharset = KGlobal::charsets()->encodingForName( encodingAction->
    currentText() ).latin1();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertMyPublicKey()
{
  KMMessagePart* msgPart;

  kernel->kbp()->busy();

  // get PGP user id for the chosen identity
  QCString pgpUserId =
    kernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() ).pgpIdentity();

  QCString armoredKey = Kpgp::Module::getKpgp()->getAsciiPublicKey(pgpUserId);
  if (armoredKey.isEmpty())
  {
    kernel->kbp()->idle();
    KMessageBox::sorry( 0L, i18n("Couldn't get your public key for\n%1.")
      .arg(Kpgp::Module::getKpgp()->user()) );
    return;
  }

  // create message part
  msgPart = new KMMessagePart;
  msgPart->setName(i18n("my pgp key"));
  msgPart->setTypeStr("application");
  msgPart->setSubtypeStr("pgp-keys");
  QValueList<int> dummy;
  msgPart->setBodyAndGuessCte(armoredKey, dummy, false);
  msgPart->setContentDisposition("attachment; filename=public_key.asc");

  // add the new attachment to the list
  addAttach(msgPart);
  rethinkFields(); //work around initial-size bug in Qt-1.32

  kernel->kbp()->idle();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertPublicKey()
{
  QCString keyID;
  KMMessagePart* msgPart;
  Kpgp::Module *pgp;

  if ( !(pgp = Kpgp::Module::getKpgp()) )
    return;

  keyID = pgp->selectPublicKey( i18n("Attach Public OpenPGP Key"),
                                i18n("Select the public key which should "
                                     "be attached.") );

  if (keyID.isEmpty())
    return;

  QCString armoredKey = pgp->getAsciiPublicKey(keyID);
  if (!armoredKey.isEmpty()) {
    // create message part
    msgPart = new KMMessagePart;
    msgPart->setName(i18n("PGP key 0x%1").arg(keyID));
    msgPart->setTypeStr("application");
    msgPart->setSubtypeStr("pgp-keys");
    QValueList<int> dummy;
    msgPart->setBodyAndGuessCte(armoredKey, dummy, false);
    msgPart->setContentDisposition("attachment; filename=0x" + keyID + ".asc");

    // add the new attachment to the list
    addAttach(msgPart);
    rethinkFields(); //work around initial-size bug in Qt-1.32
  } else {
    KMessageBox::sorry( 0L, i18n( "Could not attach public key, perhaps its format is invalid "
    	"(e.g. key contains umlaut with wrong encoding)." ) );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachPopupMenu(QListViewItem *, const QPoint &, int)
{
  if (!mAttachMenu)
  {
     mAttachMenu = new QPopupMenu(this);

     mAttachMenu->insertItem(i18n("View..."), this, SLOT(slotAttachView()));
     mAttachMenu->insertItem(i18n("Save..."), this, SLOT(slotAttachSave()));
     mAttachMenu->insertItem(i18n("Properties..."),
		   this, SLOT(slotAttachProperties()));
     mAttachMenu->insertItem(i18n("Remove"), this, SLOT(slotAttachRemove()));
     mAttachMenu->insertSeparator();
     mAttachMenu->insertItem(i18n("Attach..."), this, SLOT(slotAttachFile()));
  }
  mAttachMenu->popup(QCursor::pos());
}

//-----------------------------------------------------------------------------
int KMComposeWin::currentAttachmentNum()
{
  int idx = -1;
  QPtrListIterator<QListViewItem> it(mAtmItemList);
  for ( int i = 0; it.current(); ++it, ++i )
    if (*it == mAtmListBox->currentItem()) {
      idx = i;
      break;
    }

  return idx;
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachProperties()
{
  CryptPlugWrapper* cryptPlug = mCryptPlugList ? mCryptPlugList->active() : 0;

  int idx = currentAttachmentNum();

  if (idx < 0) return;

  KMMessagePart* msgPart = mAtmList.at(idx);
  msgPart->setCharset(mCharset);

  KMMsgPartDialogCompat dlg;
  dlg.setMsgPart(msgPart);
  KMAtmListViewItem* listItem = (KMAtmListViewItem*)(mAtmItemList.at(idx));
  if( cryptPlug && listItem ) {
    dlg.setCanSign(    true );
    dlg.setCanEncrypt( true );
    dlg.setSigned(    listItem->isSign()    );
    dlg.setEncrypted( listItem->isEncrypt() );
  } else {
    dlg.setCanSign(    false );
    dlg.setCanEncrypt( false );
  }
  if (dlg.exec())
  {
    mAtmModified = TRUE;
    // values may have changed, so recreate the listbox line
    if( listItem ) {
      msgPartToItem(msgPart, listItem);
      if( cryptPlug ) {
        listItem->setSign(    dlg.isSigned()    );
        listItem->setEncrypt( dlg.isEncrypted() );
      }
    }
  }
  if (msgPart->typeStr().lower() != "text") msgPart->setCharset(QCString());
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachView()
{
  QString str, pname;
  KMMessagePart* msgPart;
  int idx = currentAttachmentNum();

  if (idx < 0) return;

  msgPart = mAtmList.at(idx);
  pname = msgPart->name();
  if (pname.isEmpty()) pname=msgPart->contentDescription();
  if (pname.isEmpty()) pname="unnamed";

  KTempFile* atmTempFile = new KTempFile();
  mAtmTempList.append( atmTempFile );
  atmTempFile->setAutoDelete( true );
  kByteArrayToFile(msgPart->bodyDecodedBinary(), atmTempFile->name(), false, false,
    false);
  KMReaderWin::atmView(NULL, msgPart, false, atmTempFile->name(), pname,
    KMMsgBase::codecForName(mCharset));
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachSave()
{
  KMMessagePart* msgPart;
  QString fileName, pname;
  int idx = currentAttachmentNum();

  if (idx < 0) return;

  msgPart = mAtmList.at(idx);
  pname = msgPart->name();
  if (pname.isEmpty()) pname="unnamed";

  KURL url = KFileDialog::getSaveURL(QString::null, QString::null, 0, i18n("Save Attachment As"));

  if( url.isEmpty() )
    return;

  kernel->byteArrayToRemoteFile(msgPart->bodyDecodedBinary(), url);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachRemove()
{
  int idx = currentAttachmentNum();

  if (idx >= 0)
  {
    removeAttach(idx);
    mEditor->setModified(TRUE);
  }
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
void KMComposeWin::slotUpdateFont()
{
  mEditor->setFont( fixedFontAction && (fixedFontAction->isChecked())
    ? mFixedFont : mBodyFont );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotUndo()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (fw->inherits("KEdit"))
    ((QMultiLineEdit*)fw)->undo();
  else if (fw->inherits("KMLineEdit"))
    ((KMLineEdit*)fw)->undo();
}

void KMComposeWin::slotRedo()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (fw->inherits("KEdit"))
    ((QMultiLineEdit*)fw)->redo();

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
  else kdDebug(5006) << "wrong focus widget" << endl;
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
  kapp->notify(fw, &k);
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
  kapp->notify(fw, &k);
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
  win = new KMComposeWin(mCryptPlugList, msg);
  win->show();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotNewMailReader()
{
  KMMainWin *kmmwin = new KMMainWin(NULL);
  kmmwin->show();
  //d->resize(d->size());
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdWinTitle(const QString& text)
{
  if (text.isEmpty())
       setCaption("("+QString(i18n("unnamed"))+")");
  else setCaption(text);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotEncryptToggled(bool on)
{
  if ( on )
    encryptAction->setIcon("encrypted");
  else
    encryptAction->setIcon("decrypted");
  if( mCryptPlugList && mCryptPlugList->active() )
    for( KMAtmListViewItem* entry = (KMAtmListViewItem*)mAtmItemList.first();
         entry;
         entry = (KMAtmListViewItem*)mAtmItemList.next() )
      entry->setEncrypt( on );
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSignToggled(bool on)
{
  if( mCryptPlugList && mCryptPlugList->active() )
    for( KMAtmListViewItem* entry = (KMAtmListViewItem*)mAtmItemList.first();
         entry;
         entry = (KMAtmListViewItem*)mAtmItemList.next() )
      entry->setSign( on );
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotWordWrapToggled(bool on)
{
  if (on)
  {
    mEditor->setWordWrap( QMultiLineEdit::FixedColumnWidth );
    mEditor->setWrapColumnOrWidth(mLineBreak);
  }
  else
  {
    mEditor->setWordWrap( QMultiLineEdit::NoWrap );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotPrint()
{
  KMReaderWin rw;
  applyChanges();
  rw.setMsg(mMsg, true);
  rw.printMsg();
}


//----------------------------------------------------------------------------
bool KMComposeWin::doSend(int aSendNow, bool saveInDrafts)
{
  if (!saveInDrafts)
  {
     if (to().isEmpty())
     {
        mEdtTo->setFocus();
        KMessageBox::information(0,i18n("You must specify at least one receiver in the To: field."));
        return false;
     }

     if (subject().isEmpty())
     {
        mEdtSubject->setFocus();
        int rc = KMessageBox::questionYesNo(0, i18n("You did not specify a subject. Send message anyway?"),
    		i18n("No Subject Specified"), i18n("Yes"), i18n("No, Let Me Specify the Subject"), "no_subject_specified" );
        if( rc == KMessageBox::No )
        {
           return false;
        }
     }
  }

  kernel->kbp()->busy();
  mMsg->setDateToday();

  // If a user sets up their outgoing messages preferences wrong and then
  // sends mail that gets 'stuck' in their outbox, they should be able to
  // rectify the problem by editing their outgoing preferences and
  // resending.
  // Hence this following conditional
  QString hf = mMsg->headerField("X-KMail-Transport");
  if ((mTransport->currentText() != mTransport->text(0)) ||
      (!hf.isEmpty() && (hf != mTransport->text(0))))
    mMsg->setHeaderField("X-KMail-Transport", mTransport->currentText());

  disableBreaking = saveInDrafts;

  bccMsgList.clear();
  bool sentOk = applyChanges();

  disableBreaking = false;

  if (!sentOk)
  {
     kernel->kbp()->idle();
     return false;
  }

  if (saveInDrafts)
  {
    KMFolder* draftsFolder = 0, *imapDraftsFolder = 0;
    // get the draftsFolder
    if ( !mMsg->drafts().isEmpty() )
    {
      draftsFolder = kernel->folderMgr()->findIdString( mMsg->drafts() );
      if ( draftsFolder == 0 )
	imapDraftsFolder = kernel->imapFolderMgr()->findIdString( mMsg->drafts() );
    }
    if (imapDraftsFolder && imapDraftsFolder->noContent())
      imapDraftsFolder = NULL;

    if ( draftsFolder == 0 ) {
      draftsFolder = kernel->draftsFolder();
    } else {
      draftsFolder->open();
    }
    kdDebug(5006) << "saveindrafts: drafts=" << draftsFolder->name() << endl;
    if (imapDraftsFolder)
      kdDebug(5006) << "saveindrafts: imapdrafts="
		    << imapDraftsFolder->name() << endl;

    sentOk = !(draftsFolder->addMsg(mMsg));
    if (imapDraftsFolder)
    {
      // move the message to the imap-folder and highlight it
      imapDraftsFolder->moveMsg(mMsg);
      (static_cast<KMFolderImap*>(imapDraftsFolder))->getFolder();
    }

  } else {
    mMsg->setTo( KabcBridge::expandDistributionLists( to() ));
    mMsg->setCc( KabcBridge::expandDistributionLists( cc() ));
    if( !mMsg->bcc().isEmpty() )
      mMsg->setBcc( KabcBridge::expandDistributionLists( mMsg->bcc() ));
    QString recips = mMsg->headerField( "X-KMail-Recipients" );
    if( !recips.isEmpty() ) {
      mMsg->setHeaderField( "X-KMail-Recipients", KabcBridge::expandDistributionLists( recips ) );
    }
    mMsg->cleanupHeader();
    sentOk = kernel->msgSender()->send(mMsg, aSendNow);
    KMMessage* msg;
    for( msg = bccMsgList.first(); msg; msg = bccMsgList.next() ) {
      msg->setTo( KabcBridge::expandDistributionLists( to() ));
      msg->setCc( KabcBridge::expandDistributionLists( cc() ));
      msg->setBcc( KabcBridge::expandDistributionLists( bcc() ));
      QString recips = msg->headerField( "X-KMail-Recipients" );
      if( !recips.isEmpty() ) {
        msg->setHeaderField( "X-KMail-Recipients", KabcBridge::expandDistributionLists( recips ) );
      }
      msg->cleanupHeader();
      sentOk &= kernel->msgSender()->send(msg, aSendNow);
    }
  }

  kernel->kbp()->idle();

  if (!sentOk)
     return false;

  if (saveInDrafts || !aSendNow)
      emit messageQueuedOrDrafted();

  KMRecentAddresses::self()->add( bcc() );
  KMRecentAddresses::self()->add( cc() );
  KMRecentAddresses::self()->add( to() );

  mAutoDeleteMsg = FALSE;
  mFolder = NULL;
  close();
  return true;
}



//----------------------------------------------------------------------------
void KMComposeWin::slotSendLater()
{
  doSend(FALSE);
}


//----------------------------------------------------------------------------
bool KMComposeWin::slotSaveDraft()
{
  return doSend(FALSE, TRUE);
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSendNow()
{
  if (mConfirmSend) {
    switch(KMessageBox::warningYesNoCancel(mMainWidget,
                                    i18n("About to send email..."),
                                    i18n("Send Confirmation"),
                                    i18n("Send &Now"),
                                    i18n("Send &Later"))) {
    case KMessageBox::Yes:        // send now
        doSend(TRUE);
      break;
    case KMessageBox::No:        // send later
        doSend(FALSE);
      break;
    case KMessageBox::Cancel:        // cancel
      break;
    default:
      ;    // whoa something weird happened here!
    }
    return;
  }

  doSend(TRUE);
}





void KMComposeWin::slotSelectCrypto()
{
  QString dummy;
  KListBoxDialog dialog( dummy, "", i18n( "&Select crypto module:" ) );
  dialog.resize( 350, 200 );
  dialog.entriesLB->clear();

  CryptPlugWrapper* activeCryptPlug = mCryptPlugList->active();

  CryptPlugWrapper* current;
  QPtrListIterator<CryptPlugWrapper> it( *mCryptPlugList );
  int idx=-1;
  int i=0;
  while( ( current = it.current() ) ) {
    dialog.entriesLB->insertItem( i18n("Plug-in #%1: \"%2\"").arg(i+1).arg(current->displayName()) );
    if( activeCryptPlug == current )
      idx = i;
    ++it;
    ++i;
  }
  if( 0 > idx )
    idx = i;
  dialog.entriesLB->insertItem( i18n( "[ traditional built-in crypto module ]" ) );

  dialog.entriesLB->setFocus();

  dialog.entriesLB->setSelected( idx, true );

  if( dialog.exec() == QDialog::Accepted ) {
    idx = dialog.entriesLB->currentItem();
    i = 0;
    it.toFirst();
    while( ( current = it.current() ) ) {
      current->setActive( i == idx );
      ++it;
      ++i;
    }
  }
}



void KMComposeWin::slotAppendSignature()
{
  bool mod = mEditor->isModified();

  const KMIdentity & ident =
    kernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  QString sigText = ident.signatureText();
#if 0 // will be moved to identitymanager
  if( sigText.isNull() && ident.useSignatureFile() )
  {
    // open a file dialog and let the user choose manually
    KFileDialog dlg( QDir::homeDirPath(), QString::null, this, 0, TRUE );
    if (ident.signatureIsPlainFile())
      dlg.setCaption(i18n("Choose Signature File"));
    else
      // make this "Choose Signature Command" on msg thaw.
      dlg.setCaption(i18n("Choose Signature File"));
    if( !dlg.exec() )
    {
      return;
    }
    KURL url = dlg.selectedURL();
    if( url.isEmpty() )
    {
      return;
    }
    if( !url.isLocalFile() )
    {
      KMessageBox::sorry( 0L, i18n( "Only local files are supported." ) );
      return;
    }
    QString sigFileName = url.path();
    QFileInfo qfi( sigFileName );
    if ( ident.signatureIsCommand() && !qfi.isExecutable() )
    {
      // ### Commented out due to msg freeze (remove comment and underscores):
      //KMessageBox::sorry( 0L, _i_1_8_n_( "%1 is not executable." ).arg( url.path() ) );
      return;
    }
    ident.setSignatureFile(sigFileName);
    ident.writeConfig(true);
    sigText = ident.signature(false); // try again, but don't prompt
  }
#endif
  mOldSigText = sigText;
  if( !sigText.isEmpty() )
  {
    mEditor->sync();
    mEditor->append("\n");
    mEditor->append(sigText);
    mEditor->update();
    mEditor->setModified(mod);
    mEditor->setContentsPos( 0, 0 );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotHelp()
{
  kapp->invokeHelp();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCleanSpace()
{
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

  mEditor->spellcheck();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheckDone(int result)
{
  kdDebug(5006) << "spell check complete: result = " << result << endl;
  mSpellCheckInProgress=FALSE;

  switch( result )
  {
    case KS_CANCEL:
      statusBar()->changeItem(i18n(" Spell check canceled."),0);
      break;
    case KS_STOP:
      statusBar()->changeItem(i18n(" Spell check stopped."),0);
      break;
    default:
      statusBar()->changeItem(i18n(" Spell check complete."),0);
      break;
  }
  QTimer::singleShot( 2000, this, SLOT(slotSpellcheckDoneClearStatus()) );
}

void KMComposeWin::slotSpellcheckDoneClearStatus()
{
  statusBar()->changeItem("", 0);
}

//-----------------------------------------------------------------------------
void KMComposeWin::focusNextPrevEdit(const QWidget* aCur, bool aNext)
{
  QWidget* cur;

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

//-----------------------------------------------------------------------------
void KMComposeWin::slotIdentityChanged(uint uoid)
{
  const KMIdentity & ident =
    kernel->identityManager()->identityForUoid( uoid );
  if ( ident.isNull() ) return;

  if(!ident.fullEmailAddr().isNull())
    mEdtFrom->setText(ident.fullEmailAddr());
  mEdtReplyTo->setText(ident.replyToAddr());
  mEdtBcc->setText(ident.bcc());
  // make sure the BCC field is shown because else it's ignored
  if (! ident.bcc().isEmpty()) {
    mShowHeaders |= HDR_BCC;
  }
  if (ident.organization().isEmpty())
    mMsg->removeHeaderField("Organization");
  else
    mMsg->setHeaderField("Organization", ident.organization());

  if (!mBtnTransport->isChecked()) {
    if (ident.transport().isEmpty())
      mMsg->removeHeaderField("X-KMail-Transport");
    else
      mMsg->setHeaderField("X-KMail-Transport", ident.transport());
    QString transp = ident.transport();
    if (transp.isEmpty()) transp = mTransport->text(0);
    bool found = false;
    int i;
    for (i = 0; i < mTransport->count(); i++) {
      if (mTransport->text(i) == transp) {
        found = true;
        mTransport->setCurrentItem(i);
        break;
      }
    }
    if (found == false) {
      if (i == mTransport->maxCount()) mTransport->setMaxCount(i + 1);
      mTransport->insertItem(transp,i);
      mTransport->setCurrentItem(i);
    }
  }

  if ( !mBtnFcc->isChecked() )
  {
    if ( ident.fcc().isEmpty() )
      mFcc->setFolder( kernel->sentFolder() );
    else
      setFcc( ident.fcc() );
  }

  QString edtText = mEditor->text();
  bool appendNewSig = true;
  // try to truncate the old sig
  if( !mOldSigText.isEmpty() )
  {
    if( edtText.endsWith( "\n" + mOldSigText ) )
      edtText.truncate( edtText.length() - mOldSigText.length() - 1 );
    else
      appendNewSig = false;
  }
  // now append the new sig
  mOldSigText = ident.signatureText();
  if( appendNewSig )
  {
    if( !mOldSigText.isEmpty() && mAutoSign )
      edtText.append( "\n" + mOldSigText );
    mEditor->setText( edtText );
  }

  CryptPlugWrapper* cryptPlug = mCryptPlugList ? mCryptPlugList->active() : 0;

  // disable certain actions if there is no PGP user identity set
  // for this profile
  if( !cryptPlug && ident.pgpIdentity().isEmpty() )
  {
    attachMPK->setEnabled(false);
    if (signAction->isEnabled())
    { // save the state of the sign and encrypt button
      mLastEncryptActionState = encryptAction->isChecked();
      encryptAction->setEnabled(false);
      encryptAction->setChecked(false);
      mLastSignActionState = signAction->isChecked();
      signAction->setEnabled(false);
      signAction->setChecked(false);
    }
  }
  else
  {
    attachMPK->setEnabled(true);
    if( !signAction->isEnabled() )
    { // restore the last state of the sign and encrypt button
      encryptAction->setEnabled(true);
      encryptAction->setChecked(mLastEncryptActionState);
      signAction->setEnabled(true);
      signAction->setChecked(mLastSignActionState);
    }
  }

  mEditor->setModified(TRUE);
  mId = uoid;

  // make sure the BCC field is shown if necessary
  rethinkFields( false );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheckConfig()
{
  KWin kwin;
  QTabDialog qtd (this, "tabdialog", true);
  KSpellConfig mKSpellConfig (&qtd);

  qtd.addTab (&mKSpellConfig, i18n("Spellchecker"));
  qtd.setCancelButton ();

  kwin.setIcons (qtd.winId(), kapp->icon(), kapp->miniIcon());
  qtd.setCancelButton(i18n("&Cancel"));
  qtd.setOkButton(i18n("&Ok"));

  if (qtd.exec())
    mKSpellConfig.writeGlobalSettings();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotToggleToolBar()
{
  if(toolBar("mainToolBar")->isVisible())
    toolBar("mainToolBar")->hide();
  else
    toolBar("mainToolBar")->show();
}

void KMComposeWin::slotToggleStatusBar()
{
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
}

void KMComposeWin::slotEditToolbars()
{
  saveMainWindowSettings(kapp->config(), "Composer");
  KEditToolbar dlg(actionCollection(), "kmcomposerui.rc");

  connect( &dlg, SIGNAL(newToolbarConfig()),
	   SLOT(slotUpdateToolbars()) );

  dlg.exec();
}

void KMComposeWin::slotUpdateToolbars()
{
  createGUI("kmcomposerui.rc");
  applyMainWindowSettings(kapp->config(), "Composer");
  toolbarAction->setChecked(!toolBar()->isHidden());
}

void KMComposeWin::slotEditKeys()
{
  KKeyDialog::configure( actionCollection()
#if KDE_VERSION >= 306
			 , false /*don't allow one-letter shortcuts*/
#endif
			 );
}

void KMComposeWin::setReplyFocus( bool hasMessage )
{
  mEditor->setFocus();
  if ( hasMessage )
    mEditor->setCursorPosition( 1, 0 );
}

void KMComposeWin::slotCompletionModeChanged( KGlobalSettings::Completion mode)
{
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cs( config, "Composer" );
    config->writeEntry( "Completion Mode", (int) mode );
    config->sync(); // maybe not?

    // sync all the lineedits to the same completion mode
    mEdtFrom->setCompletionMode( mode );
    mEdtReplyTo->setCompletionMode( mode );
    mEdtTo->setCompletionMode( mode );
    mEdtCc->setCompletionMode( mode );
    mEdtBcc->setCompletionMode( mode );
}

/*
* checks if the drafts-folder has been deleted
* that is not nice so we set the system-drafts-folder
*/
void KMComposeWin::slotFolderRemoved(KMFolder* folder)
{
	if ( (mFolder) && (folder->idString() == mFolder->idString()) )
	{
		mFolder = kernel->draftsFolder();
		kdDebug(5006) << "restoring drafts to " << mFolder->idString() << endl;
	}
	if (mMsg) mMsg->setParent(NULL);
}



void KMComposeWin::slotSetAlwaysSend( bool bAlways )
{
    bAlwaysSend = bAlways;
}



//=============================================================================
//
//   Class  KMAtmListViewItem
//
//=============================================================================

KMAtmListViewItem::KMAtmListViewItem(QListView *parent) :
  QObject(), QListViewItem( parent )
{
  mListview = parent;
  mCBEncrypt = new QCheckBox(mListview->viewport());
  mCBEncrypt->show();
  mCBSign = new QCheckBox(mListview->viewport());
  mCBSign->show();
}

KMAtmListViewItem::~KMAtmListViewItem()
{
}

void KMAtmListViewItem::paintCell( QPainter * p, const QColorGroup & cg,
                                  int column, int width, int align )
{
  if( 4 == column || 5 == column ) {
    QRect r = mListview->itemRect( this );
    if ( !r.size().isValid() ) {
        mListview->ensureItemVisible( this );
        mListview->repaintContents( FALSE );
        r = mListview->itemRect( this );
    }
    int colWidth = mListview->header()->sectionSize( column );
    r.setX( mListview->header()->sectionPos( column )
            + colWidth / 2
            - r.height() / 2
            - 1 );
    r.setY( r.y() + 1 );
    r.setWidth(  r.height() - 2 );
    r.setHeight( r.height() - 2 );
    r = QRect( mListview->viewportToContents( r.topLeft() ), r.size() );
    QCheckBox* cb = (4 == column) ? mCBEncrypt : mCBSign;
    cb->resize( r.size() );
    mListview->moveChild( cb, r.x(), r.y() );
  } else
    QListViewItem::paintCell( p, cg, column, width, align );
}

void KMAtmListViewItem::enableCryptoCBs(bool on)
{
  if( mCBEncrypt ) {
    mCBEncrypt->setEnabled( on );
    if( on )
      mCBEncrypt->show();
    else
      mCBEncrypt->hide();
  }
  if( mCBSign ) {
    mCBSign->setEnabled( on );
    if( on )
      mCBSign->show();
    else
      mCBSign->hide();
  }
};

void KMAtmListViewItem::setEncrypt(bool on)
{
  if( mCBEncrypt )
    mCBEncrypt->setChecked( on );
};

bool KMAtmListViewItem::isEncrypt()
{
  if( mCBEncrypt )
    return mCBEncrypt->isChecked();
  else
    return false;
};

void KMAtmListViewItem::setSign(bool on)
{
  if( mCBSign )
    mCBSign->setChecked( on );
};

bool KMAtmListViewItem::isSign()
{
  if( mCBSign )
    return mCBSign->isChecked();
  else
    return false;
};



//=============================================================================
//
//   Class  KMLineEdit
//
//=============================================================================

KMLineEdit::KMLineEdit(KMComposeWin* composer, bool useCompletion,
                       QWidget *parent, const char *name)
    : KMLineEditInherited(parent,useCompletion,name), mComposer(composer)
{
}


//-----------------------------------------------------------------------------
void KMLineEdit::keyPressEvent(QKeyEvent *e)
{
    // ---sven's Return is same Tab and arrow key navigation start ---
    if ((e->key() == Key_Enter || e->key() == Key_Return) &&
        !completionBox()->isVisible())
    {
      mComposer->focusNextPrevEdit(this,TRUE);
      return;
    }
    if (e->key() == Key_Up)
    {
      mComposer->focusNextPrevEdit(this,FALSE); // Go up
      return;
    }
    if (e->key() == Key_Down)
    {
      mComposer->focusNextPrevEdit(this,TRUE); // Go down
      return;
    }
    // ---sven's Return is same Tab and arrow key navigation end ---
  KMLineEditInherited::keyPressEvent(e);
}

//-----------------------------------------------------------------------------
void KMLineEdit::loadAddresses()
{
    KMLineEditInherited::loadAddresses();

    QStringList recent = KMRecentAddresses::self()->addresses();
    QStringList::Iterator it = recent.begin();
    for ( ; it != recent.end(); ++it )
        addAddress( *it );
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
  mComposer = composer;
  installEventFilter(this);
  KCursor::setAutoHideCursor( this, true, true );

  extEditor = false;     // the default is to use ourself

  mKSpell = NULL;
  mSpellingFilter = 0;
  mTempFile = NULL;
  mExtEditorProcess = NULL;
}


//-----------------------------------------------------------------------------
KMEdit::~KMEdit()
{

  removeEventFilter(this);

  if (mKSpell) delete mKSpell;

}


//-----------------------------------------------------------------------------
QString KMEdit::brokenText()
{
  QString temp, line;

  int num_lines = numLines();
  for (int i = 0; i < num_lines; ++i)
  {
    int lastLine = 0;
    line = textLine(i);
    for (int j = 0; j < (int)line.length(); ++j)
    {
      if (lineOfChar(i, j) > lastLine)
      {
        lastLine = lineOfChar(i, j);
        temp += '\n';
      }
      temp += line[j];
    }
    if (i + 1 < num_lines) temp += '\n';
  }

  return temp;
}

//-----------------------------------------------------------------------------
bool KMEdit::eventFilter(QObject*o, QEvent* e)
{
  if (o == this)
    KCursor::autoHideEventFilter(o, e);

  if (e->type() == QEvent::KeyPress)
  {
    QKeyEvent *k = (QKeyEvent*)e;

    if (extEditor) {
      if (k->key() == Key_Up)
      {
        mComposer->focusNextPrevEdit(0, false); //take me up
        return TRUE;
      }

      if (mTempFile) return TRUE;
      QRegExp repFn("\\%f");
      QString sysLine = mExtEditor;
      mTempFile = new KTempFile();

      mTempFile->setAutoDelete(true);

      fprintf(mTempFile->fstream(), "%s", (const char *)text().local8Bit());

      mTempFile->close();
      // replace %f in the system line
      sysLine.replace(repFn, mTempFile->name());
      mExtEditorProcess = new KProcess();
      sysLine += " ";
      while (!sysLine.isEmpty())
      {
        *mExtEditorProcess << sysLine.left(sysLine.find(" ")).local8Bit();
        sysLine.remove(0, sysLine.find(" ") + 1);
      }
      connect(mExtEditorProcess, SIGNAL(processExited(KProcess*)),
              SLOT(slotExternalEditorDone(KProcess*)));
      if (!mExtEditorProcess->start())
      {
        KMessageBox::error(NULL, i18n("Unable to start external editor."));
        delete mExtEditorProcess;
        delete mTempFile;
        mExtEditorProcess = NULL;
        mTempFile = NULL;
      }

      return TRUE;
    } else {
    // ---sven's Arrow key navigation start ---
    // Key Up in first line takes you to Subject line.
    if (k->key() == Key_Up && k->state() != ShiftButton && currentLine() == 0
      && lineOfChar(0, currentColumn()) == 0)
    {
      deselect();
      mComposer->focusNextPrevEdit(0, false); //take me up
      return TRUE;
    }
    // ---sven's Arrow key navigation end ---

    if (k->key() == Key_Backtab && k->state() == ShiftButton)
    {
      deselect();
      mComposer->focusNextPrevEdit(0, false);
	  return TRUE;
    }

    }
  }
  else if (e->type() == QEvent::Drop)
  {
    KURL::List urlList;
    QDropEvent *de = static_cast<QDropEvent*>(e);
    if(QUriDrag::canDecode(de) && KURLDrag::decode( de, urlList ))
    {
      for (KURL::List::Iterator it = urlList.begin(); it != urlList.end(); ++it)
        mComposer->addAttach(*it);
      return TRUE;
    }
  }

  return KMEditInherited::eventFilter(o, e);
}


//-----------------------------------------------------------------------------
void KMEdit::slotExternalEditorDone(KProcess* proc)
{
  assert(proc == mExtEditorProcess);
  setAutoUpdate(false);
  clear();

  // read data back in from file
  insertLine(QString::fromLocal8Bit(kFileToString(mTempFile->name(),
    TRUE, FALSE)), -1);

  setAutoUpdate(true);
  repaint();
  delete proc;
  delete mTempFile;
  mTempFile = NULL;
  mExtEditorProcess = NULL;
}


//-----------------------------------------------------------------------------
void KMEdit::spellcheck()
{
  mKSpell = new KSpell(this, i18n("Spellcheck - KMail"), this,
		       SLOT(slotSpellcheck2(KSpell*)));
  connect (mKSpell, SIGNAL( death()),
          this, SLOT (slotSpellDone()));
  connect (mKSpell, SIGNAL (misspelling (const QString &, const QStringList &, unsigned int)),
          this, SLOT (misspelling (const QString &, const QStringList &, unsigned int)));
  connect (mKSpell, SIGNAL (corrected (const QString &, const QString &, unsigned int)),
          this, SLOT (corrected (const QString &, const QString &, unsigned int)));
  connect (mKSpell, SIGNAL (done(const QString &)),
          this, SLOT (slotSpellResult (const QString&)));
}


//-----------------------------------------------------------------------------
void KMEdit::slotSpellcheck2(KSpell*)
{
  spellcheck_start();

  QString quotePrefix;
  if(mComposer && mComposer->msg())
  {
    // read the quote indicator from the preferences
    KConfig *config=kapp->config();
    KConfigGroupSaver saver(config, "General");

    int languageNr = config->readNumEntry("reply-current-language",0);
    config->setGroup( QString("KMMessage #%1").arg(languageNr) );

    quotePrefix = config->readEntry("indent-prefix", ">%_");
    quotePrefix = mComposer->msg()->formatString(quotePrefix);
  }

  kdDebug(5006) << "spelling: new SpellingFilter with prefix=\"" << quotePrefix << "\"" << endl;
  mSpellingFilter = new SpellingFilter(text(), quotePrefix, SpellingFilter::FilterUrls,
    SpellingFilter::FilterEmailAddresses);

  mKSpell->check(mSpellingFilter->filteredText());
}

//-----------------------------------------------------------------------------
void KMEdit::slotSpellResult(const QString &)
{
  spellcheck_stop();

  int dlgResult = mKSpell->dlgResult();
  if ( dlgResult == KS_CANCEL )
  {
    kdDebug(5006) << "spelling: canceled - restoring text from SpellingFilter" << endl;
    setText(mSpellingFilter->originalText());
  }

  mKSpell->cleanUp();
  emit spellcheck_done( dlgResult );
}

//-----------------------------------------------------------------------------
void KMEdit::slotSpellDone()
{
  KSpell::spellStatus status = mKSpell->status();
  delete mKSpell;
  mKSpell = 0;

  kdDebug(5006) << "spelling: delete SpellingFilter" << endl;
  delete mSpellingFilter;
  mSpellingFilter = 0;

  if (status == KSpell::Error)
  {
     KMessageBox::sorry(this, i18n("ISpell/Aspell could not be started. Please make sure you have ISpell or Aspell properly configured and in your PATH."));
     emit spellcheck_done( KS_CANCEL );
  }
  else if (status == KSpell::Crashed)
  {
     spellcheck_stop();
     KMessageBox::sorry(this, i18n("ISpell/Aspell seems to have crashed."));
     emit spellcheck_done( KS_CANCEL );
  }
}
