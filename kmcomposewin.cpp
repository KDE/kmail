// -*- mode: C++; c-file-style: "gnu" -*-
// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

// keep this in sync with the define in configuredialog.h
#define DEFAULT_EDITOR_STR "kate %f"

//#define STRICT_RULES_OF_GERMAN_GOVERNMENT_01

#undef GrayScale
#undef Color
#include <config.h>

#include "kmcomposewin.h"

#include "kmmainwin.h"
#include "kmreaderwin.h"
#include "kmreadermainwin.h"
#include "kmsender.h"
#include "identitymanager.h"
#include "identitycombo.h"
#include "kmidentity.h"
#include "kfileio.h"
#include "kmmsgpartdlg.h"
#include <kpgpblock.h>
#include "kmaddrbook.h"
#include "kmmsgdict.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmfoldercombobox.h"
#include "kmtransport.h"
#include "kmcommands.h"
#include "kcursorsaver.h"
#include "kmkernel.h"
#include "attachmentlistview.h"
using KMail::AttachmentListView;
#include "dictionarycombobox.h"
using KMail::DictionaryComboBox;
#include "addressesdialog.h"
using KPIM::AddressesDialog;
#include <maillistdrag.h>
using KPIM::MailListDrag;
#include "recentaddresses.h"
using KRecentAddress::RecentAddresses;

#include <cryptplugwrapperlist.h>

#include "klistboxdialog.h"

#include <kcharsets.h>
#include <kcompletionbox.h>
#include <kcursor.h>
#include <kcombobox.h>
#include <kstdaccel.h>
#include <kpopupmenu.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kwin.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kurldrag.h>
#include <kio/scheduler.h>
#include <ktempfile.h>
#include <klocale.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <kaction.h>
#include <kdirwatch.h>
#include <kstdguiitem.h>

#include <kspell.h>
#include <kspelldlg.h>
#include <spellingfilter.h>
#include <ksyntaxhighlighter.h>

#include <qtabdialog.h>
#include <qregexp.h>
#include <qbuffer.h>
#include <qtooltip.h>
#include <qtextcodec.h>
#include <qheader.h>
#include <qpopupmenu.h>

#include <mimelib/mimepp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include "kmcomposewin.moc"

//-----------------------------------------------------------------------------
KMComposeWin::KMComposeWin( KMMessage *aMsg, uint id  )
  : MailComposerIface(), KMTopLevelWidget("kmail-composer#"),
    mMsg( 0 ),
    mAutoRequestMDN( false ),
    mId( id ), mNeverSign( false ), mNeverEncrypt( false )
{
  if (kmkernel->xmlGuiInstance())
    setInstance( kmkernel->xmlGuiInstance() );
  mMainWidget = new QWidget(this);

  // Initialize the plugin selection according to 'active' flag that
  // was set via the global configuration dialog.
  mSelectedCryptPlug = kmkernel->cryptPlugList() ? kmkernel->cryptPlugList()->active() : 0;

  mIdentity = new IdentityCombo(mMainWidget);
  mDictionaryCombo = new DictionaryComboBox( mMainWidget );
  mFcc = new KMFolderComboBox(mMainWidget);
  mFcc->showOutboxFolder( FALSE );
  mTransport = new QComboBox(true, mMainWidget);
  mEdtFrom = new KMLineEdit(this,false,mMainWidget);
  mEdtReplyTo = new KMLineEdit(this,false,mMainWidget);
  mEdtTo = new KMLineEdit(this,true,mMainWidget);
  mEdtCc = new KMLineEdit(this,true,mMainWidget);
  mEdtBcc = new KMLineEdit(this,true,mMainWidget);
  mEdtSubject = new KMLineEditSpell(this,false,mMainWidget, "subjectLine");
  mLblIdentity = new QLabel(mMainWidget);
  mDictionaryLabel = new QLabel( mMainWidget );
  mLblFcc = new QLabel(mMainWidget);
  mLblTransport = new QLabel(mMainWidget);
  mLblFrom = new QLabel(mMainWidget);
  mLblReplyTo = new QLabel(mMainWidget);
  mLblTo = new QLabel(mMainWidget);
  mLblCc = new QLabel(mMainWidget);
  mLblBcc = new QLabel(mMainWidget);
  mLblSubject = new QLabel(mMainWidget);
  QString sticky = i18n("Sticky");
  mBtnIdentity = new QCheckBox(sticky,mMainWidget);
  mBtnFcc = new QCheckBox(sticky,mMainWidget);
  mBtnTransport = new QCheckBox(sticky,mMainWidget);
  mBtnTo = new QPushButton("...",mMainWidget);
  mBtnCc = new QPushButton("...",mMainWidget);
  mBtnBcc = new QPushButton("...",mMainWidget);
  //mBtnFrom = new QPushButton("...",mMainWidget);
  mBtnReplyTo = new QPushButton("...",mMainWidget);

  //setWFlags( WType_TopLevel | WStyle_Dialog );
  mDone = false;
  mGrid = 0;
  mAtmListView = 0;
  mAtmList.setAutoDelete(TRUE);
  mAtmTempList.setAutoDelete(TRUE);
  mAtmModified = FALSE;
  mAutoDeleteMsg = FALSE;
  mFolder = 0;
  mAutoCharset = TRUE;
  mFixedFontAction = 0;
  mEditor = new KMEdit( mMainWidget, this, mDictionaryCombo->spellConfig() );
  mEditor->setTextFormat(Qt::PlainText);
  mEditor->setAcceptDrops( true );

  mDisableBreaking = false;
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
  //mBtnFrom->setFocusPolicy(QWidget::NoFocus);
  mBtnReplyTo->setFocusPolicy(QWidget::NoFocus);

  mAtmListView = new AttachmentListView( this, mMainWidget,
                                         "attachment list view" );
  mAtmListView->setSelectionMode( QListView::Extended );
  mAtmListView->setFocusPolicy( QWidget::NoFocus );
  mAtmListView->addColumn( i18n("Name"), 200 );
  mAtmListView->addColumn( i18n("Size"), 80 );
  mAtmListView->addColumn( i18n("Encoding"), 120 );
  int atmColType = mAtmListView->addColumn( i18n("Type"), 120 );
  // Stretch "Type".
  mAtmListView->header()->setStretchEnabled( true, atmColType );
  mAtmEncryptColWidth = 80;
  mAtmSignColWidth = 80;
  mAtmColEncrypt = mAtmListView->addColumn( i18n("Encrypt"),
                                            mAtmEncryptColWidth );
  mAtmColSign    = mAtmListView->addColumn( i18n("Sign"),
                                            mAtmSignColWidth );
  if( mSelectedCryptPlug ) {
    mAtmListView->setColumnWidth( mAtmColEncrypt, mAtmEncryptColWidth );
    mAtmListView->setColumnWidth( mAtmColSign,    mAtmSignColWidth );
  }
  else {
    mAtmListView->setColumnWidth( mAtmColEncrypt, 0 );
    mAtmListView->setColumnWidth( mAtmColSign,    0 );
  }
  mAtmListView->setAllColumnsShowFocus( true );

  connect( mAtmListView,
           SIGNAL( doubleClicked( QListViewItem* ) ),
           SLOT( slotAttachProperties() ) );
  connect( mAtmListView,
           SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
           SLOT( slotAttachPopupMenu( QListViewItem*, const QPoint&, int ) ) );
  connect( mAtmListView,
           SIGNAL( selectionChanged() ),
           SLOT( slotUpdateAttachActions() ) );
  mAttachMenu = 0;

  readConfig();
  setupStatusBar();
  setupEditor();
  setupActions();
  applyMainWindowSettings(KMKernel::config(), "Composer");

  connect(mEdtSubject,SIGNAL(textChanged(const QString&)),
          SLOT(slotUpdWinTitle(const QString&)));
  connect(mBtnTo,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
  connect(mBtnCc,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
  connect(mBtnBcc,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
  connect(mBtnReplyTo,SIGNAL(clicked()),SLOT(slotAddrBookReplyTo()));
  //connect(mBtnFrom,SIGNAL(clicked()),SLOT(slotAddrBookFrom()));
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
        connect(kmkernel->folderMgr(),SIGNAL(folderRemoved(KMFolder*)),
                                        SLOT(slotFolderRemoved(KMFolder*)));
        connect(kmkernel->imapFolderMgr(),SIGNAL(folderRemoved(KMFolder*)),
                                        SLOT(slotFolderRemoved(KMFolder*)));
        connect(kmkernel->dimapFolderMgr(),SIGNAL(folderRemoved(KMFolder*)),
                                        SLOT(slotFolderRemoved(KMFolder*)));
  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );

  connect (mEditor, SIGNAL (spellcheck_done(int)),
    this, SLOT (slotSpellcheckDone (int)));

  mMainWidget->resize(480,510);
  setCentralWidget(mMainWidget);
  rethinkFields();

  if (mUseExtEditor) {
    mEditor->setUseExternalEditor(true);
    mEditor->setExternalEditorPath(mExtEditor);
  }

  mMsg = 0;
  mBccMsgList.setAutoDelete( false );
  if (aMsg)
    setMsg(aMsg);

  mEdtTo->setFocus();
  mErrorProcessingStructuringInfo =
    i18n("<qt><p>Structuring information returned by the Crypto plug-in "
         "could not be processed correctly; the plug-in might be damaged.</p>"
         "<p>Please contact your system administrator.</p></qt>");
  mErrorNoCryptPlugAndNoBuildIn =
    i18n("<p>No active Crypto Plug-In was found and the built-in OpenPGP code "
         "did not run successfully.</p>"
         "<p>You can do two things to change this:</p>"
         "<ul><li><em>either</em> activate a Plug-In using the "
         "Settings->Configure KMail->Plug-In dialog.</li>"
         "<li><em>or</em> specify traditional OpenPGP settings on the same dialog's "
         "Identity->Advanced tab.</li></ul>");

  if(getenv("KMAIL_DEBUG_COMPOSER_CRYPTO") != 0){
    QCString cE = getenv("KMAIL_DEBUG_COMPOSER_CRYPTO");
    mDebugComposerCrypto = cE == "1" || cE.upper() == "ON" || cE.upper() == "TRUE";
    kdDebug(5006) << "KMAIL_DEBUG_COMPOSER_CRYPTO = TRUE" << endl;
  }else{
    mDebugComposerCrypto = false;
    kdDebug(5006) << "KMAIL_DEBUG_COMPOSER_CRYPTO = FALSE" << endl;
  }
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
  if (mAutoDeleteMsg) {
    delete mMsg;
    mMsg = 0;
  }
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.begin();
  while ( it != mMapAtmLoadData.end() )
  {
    KIO::Job *job = it.key();
    mMapAtmLoadData.remove( it );
    job->kill();
    it = mMapAtmLoadData.begin();
  }
  mBccMsgList.clear();
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
                                kmkernel->msgSender()->sendQuotedPrintable());
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
  KConfig *config = KMKernel::config();
  KConfigGroupSaver saver(config, "Reader");
  QColor c1=QColor(kapp->palette().active().text());
  QColor c4=QColor(kapp->palette().active().base());

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    mForeColor = config->readColorEntry("ForegroundColor",&c1);
    mBackColor = config->readColorEntry("BackgroundColor",&c4);
  }
  else {
    mForeColor = c1;
    mBackColor = c4;
  }

  // Color setup
  mPalette = kapp->palette();
  QColorGroup cgrp  = mPalette.active();
  cgrp.setColor( QColorGroup::Base, mBackColor);
  cgrp.setColor( QColorGroup::Text, mForeColor);
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
  KConfig *config = KMKernel::config();
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
  QString previousFcc = kmkernel->sentFolder()->idString();
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
  mAutoRequestMDN = config->readBoolEntry("request-mdn", false);

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
    mExtEditor = config->readPathEntry("external-editor", DEFAULT_EDITOR_STR);
    mUseExtEditor = config->readBoolEntry("use-external-editor", FALSE);

    int headerCount = config->readNumEntry("mime-header-count", 0);
    mCustHeaders.clear();
    mCustHeaders.setAutoDelete(true);
    for (int i = 0; i < headerCount; i++) {
      QString thisGroup;
      _StringPair *thisItem = new _StringPair;
      thisGroup.sprintf("Mime #%d", i);
      KConfigGroupSaver saver(config, thisGroup);
      thisItem->name = config->readEntry("name");
      if ((thisItem->name).length() > 0) {
        thisItem->value = config->readEntry("value");
        mCustHeaders.append(thisItem);
      } else {
        delete thisItem;
        thisItem = 0;
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

  kdDebug(5006) << "KMComposeWin::readConfig. " << mIdentity->currentIdentityName() << endl;
  const KMIdentity & ident =
    kmkernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  mTransport->clear();
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
      kdDebug(5006) << "KMComposeWin::readConfig: identity.fcc()='"
                    << ident.fcc() << "'" << endl;
      if ( ident.fcc().isEmpty() )
        previousFcc = kmkernel->sentFolder()->idString();
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
  KConfig *config = KMKernel::config();
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
    config->writeEntry( "autoSpellChecking",
                        mAutoSpellCheckingAction->isChecked() );
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
  // temporarily disable signing/encryption
  bool bSaveNeverSign    = mNeverSign;    mNeverSign    = true;
  bool bSaveNeverEncrypt = mNeverEncrypt; mNeverEncrypt = true;
  applyChanges( true );
  mNeverSign    = bSaveNeverSign;
  mNeverEncrypt = bSaveNeverEncrypt;
  QCString msgStr = mMsg->asString();
  QCString fname = getenv("HOME");
  fname += "/dead.letter.tmp";
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
    fprintf(stderr,"appending message to ~/dead.letter.tmp\n");
  } else {
    perror("cannot open ~/dead.letter.tmp for saving the current message");
    kmkernel->emergencyExit( i18n("cannot open ~/dead.letter.tmp for saving the current message: ")  +
                              QString::fromLocal8Bit(strerror(errno)));
  }
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

  if (act == mAllFieldsAction)
    id = 0;
  else if (act == mIdentityAction)
    id = HDR_IDENTITY;
  else if (act == mTransportAction)
    id = HDR_TRANSPORT;
  else if (act == mFromAction)
    id = HDR_FROM;
  else if (act == mReplyToAction)
    id = HDR_REPLY_TO;
  else if (act == mToAction)
    id = HDR_TO;
  else if (act == mCcAction)
    id = HDR_CC;
  else  if (act == mBccAction)
    id = HDR_BCC;
  else if (act == mSubjectAction)
    id = HDR_SUBJECT;
  else if (act == mFccAction)
    id = HDR_FCC;
  else if ( act == mDictionaryAction )
    id = HDR_DICTIONARY;
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

  delete mGrid;
  mGrid = new QGridLayout(mMainWidget, numRows, 3, 4, 4);
  mGrid->setColStretch(0, 1);
  mGrid->setColStretch(1, 100);
  mGrid->setColStretch(2, 1);
  mGrid->setRowStretch(mNumHeaders, 100);

  mEdtList.clear();
  row = 0;
  kdDebug(5006) << "KMComposeWin::rethinkFields" << endl;
  if (!fromSlot) mAllFieldsAction->setChecked(showHeaders==HDR_ALL);

  if (!fromSlot) mIdentityAction->setChecked(abs(mShowHeaders)&HDR_IDENTITY);
  rethinkHeaderLine(showHeaders,HDR_IDENTITY, row, i18n("&Identity:"),
                    mLblIdentity, mIdentity, mBtnIdentity);
  if (!fromSlot) mDictionaryAction->setChecked(abs(mShowHeaders)&HDR_DICTIONARY);
  rethinkHeaderLine(showHeaders,HDR_DICTIONARY, row, i18n("&Dictionary:"),
                    mDictionaryLabel, mDictionaryCombo, 0 );
  if (!fromSlot) mFccAction->setChecked(abs(mShowHeaders)&HDR_FCC);
  rethinkHeaderLine(showHeaders,HDR_FCC, row, i18n("Se&nt-Mail folder:"),
                    mLblFcc, mFcc, mBtnFcc);
  if (!fromSlot) mTransportAction->setChecked(abs(mShowHeaders)&HDR_TRANSPORT);
  rethinkHeaderLine(showHeaders,HDR_TRANSPORT, row, i18n("Mai&l transport:"),
                    mLblTransport, mTransport, mBtnTransport);
  if (!fromSlot) mFromAction->setChecked(abs(mShowHeaders)&HDR_FROM);
  rethinkHeaderLine(showHeaders,HDR_FROM, row, i18n("&From:"),
                    mLblFrom, mEdtFrom /*, mBtnFrom */ );
  if (!fromSlot) mReplyToAction->setChecked(abs(mShowHeaders)&HDR_REPLY_TO);
  rethinkHeaderLine(showHeaders,HDR_REPLY_TO,row,i18n("&Reply to:"),
                    mLblReplyTo, mEdtReplyTo, mBtnReplyTo);
  if (!fromSlot) mToAction->setChecked(abs(mShowHeaders)&HDR_TO);
  rethinkHeaderLine(showHeaders,HDR_TO, row, i18n("To:"),
                    mLblTo, mEdtTo, mBtnTo);
  if (!fromSlot) mCcAction->setChecked(abs(mShowHeaders)&HDR_CC);
  rethinkHeaderLine(showHeaders,HDR_CC, row, i18n("&CC:"),
                    mLblCc, mEdtCc, mBtnCc);
  if (!fromSlot) mBccAction->setChecked(abs(mShowHeaders)&HDR_BCC);
  rethinkHeaderLine(showHeaders,HDR_BCC, row, i18n("&BCC:"),
                    mLblBcc, mEdtBcc, mBtnBcc);
  if (!fromSlot) mSubjectAction->setChecked(abs(mShowHeaders)&HDR_SUBJECT);
  rethinkHeaderLine(showHeaders,HDR_SUBJECT, row, i18n("S&ubject:"),
                    mLblSubject, mEdtSubject);
  assert(row<=mNumHeaders);

  mGrid->addMultiCellWidget(mEditor, row, mNumHeaders, 0, 2);
  mGrid->addMultiCellWidget(mAtmListView, mNumHeaders+1, mNumHeaders+1, 0, 2);

  if( !mAtmList.isEmpty() )
    mAtmListView->show();
  else
    mAtmListView->hide();
  resize(this->size());
  repaint();

  mGrid->activate();

  slotUpdateAttachActions();
  mIdentityAction->setEnabled(!mAllFieldsAction->isChecked());
  mDictionaryAction->setEnabled( !mAllFieldsAction->isChecked() );
  mTransportAction->setEnabled(!mAllFieldsAction->isChecked());
  mFromAction->setEnabled(!mAllFieldsAction->isChecked());
  mReplyToAction->setEnabled(!mAllFieldsAction->isChecked());
  mToAction->setEnabled(!mAllFieldsAction->isChecked());
  mCcAction->setEnabled(!mAllFieldsAction->isChecked());
  mBccAction->setEnabled(!mAllFieldsAction->isChecked());
  mFccAction->setEnabled(!mAllFieldsAction->isChecked());
  mSubjectAction->setEnabled(!mAllFieldsAction->isChecked());
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

    aEdt->setBackgroundColor( mBackColor );
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

    //    aCbx->setBackgroundColor( mBackColor );
    aCbx->show();
    aCbx->setMinimumSize(100, aLbl->height()+2);

    mGrid->addWidget(aCbx, aRow, 1);
    if ( aChk ) {
      mGrid->addWidget(aChk, aRow, 2);
      aChk->setFixedSize(aChk->sizeHint().width(), aLbl->height());
      aChk->show();
    }
    aRow++;
  }
  else
  {
    aLbl->hide();
    aCbx->hide();
    if ( aChk )
      aChk->hide();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupActions(void)
{
  if (kmkernel->msgSender()->sendImmediate()) //default == send now?
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
    (void) new KAction (i18n("&Send Now"), "mail_send", 0,
                        this, SLOT(slotSendNow()),
                        actionCollection(), "send_alternative");
  }

  (void) new KAction (i18n("Save in &Drafts Folder"), "filesave", 0,
                      this, SLOT(slotSaveDraft()),
                      actionCollection(), "save_in_drafts");
  (void) new KAction (i18n("&Insert File..."), "fileopen", 0,
                      this,  SLOT(slotInsertFile()),
                      actionCollection(), "insert_file");
  (void) new KAction (i18n("&Address Book"), "contents",0,
                      this, SLOT(slotAddrBook()),
                      actionCollection(), "addressbook");
  (void) new KAction (i18n("&New Composer"), "mail_new",
                      KStdAccel::shortcut(KStdAccel::New),
                      this, SLOT(slotNewComposer()),
                      actionCollection(), "new_composer");
  (void) new KAction (i18n("New Main &Window"), "window_new", 0,
                      this, SLOT(slotNewMailReader()),
                      actionCollection(), "open_mailreader");


  //KStdAction::save(this, SLOT(), actionCollection(), "save_message");
  KStdAction::print (this, SLOT(slotPrint()), actionCollection());
  KStdAction::close (this, SLOT(slotClose()), actionCollection());

  KStdAction::undo (this, SLOT(slotUndo()), actionCollection());
  KStdAction::redo (this, SLOT(slotRedo()), actionCollection());
  KStdAction::cut (this, SLOT(slotCut()), actionCollection());
  KStdAction::copy (this, SLOT(slotCopy()), actionCollection());
  KStdAction::pasteText (this, SLOT(slotPaste()), actionCollection());
  KStdAction::selectAll (this, SLOT(slotMarkAll()), actionCollection());

  KStdAction::find (this, SLOT(slotFind()), actionCollection());
  KStdAction::replace (this, SLOT(slotReplace()), actionCollection());
  KStdAction::spelling (this, SLOT(slotSpellcheck()), actionCollection(), "spellcheck");

  (void) new KAction (i18n("Pa&ste as Quotation"),0,this,SLOT( slotPasteAsQuotation()),
                      actionCollection(), "paste_quoted");

  (void) new KAction(i18n("Add &Quote Characters"), 0, this,
              SLOT(slotAddQuotes()), actionCollection(), "tools_quote");

  (void) new KAction(i18n("Re&move Quote Characters"), 0, this,
              SLOT(slotRemoveQuotes()), actionCollection(), "tools_unquote");


  (void) new KAction (i18n("Cl&ean Spaces"), 0, this, SLOT(slotCleanSpace()),
                      actionCollection(), "clean_spaces");

  mFixedFontAction = new KToggleAction( i18n("Use Fi&xed Font"), 0, this,
                      SLOT(slotUpdateFont()), actionCollection(), "toggle_fixedfont" );

  //these are checkable!!!
  mUrgentAction = new KToggleAction (i18n("&Urgent"), 0,
                                    actionCollection(),
                                    "urgent");
  mRequestMDNAction = new KToggleAction ( i18n("&Request Disposition Notification"), 0,
                                         actionCollection(),
                                         "options_request_mdn");
  mRequestMDNAction->setChecked(mAutoRequestMDN);
  //----- Message-Encoding Submenu
  mEncodingAction = new KSelectAction( i18n( "Se&t Encoding" ), "charset",
                                      0, this, SLOT(slotSetCharset() ),
                                      actionCollection(), "charsets" );
  mWordWrapAction = new KToggleAction (i18n("&Wordwrap"), 0,
                      actionCollection(), "wordwrap");
  mWordWrapAction->setChecked(mWordWrap);
  connect(mWordWrapAction, SIGNAL(toggled(bool)), SLOT(slotWordWrapToggled(bool)));

  mAutoSpellCheckingAction =
    new KToggleAction( i18n( "&Automatic Spellchecking" ), "spellcheck", 0,
                       actionCollection(), "options_auto_spellchecking" );
  KConfigGroup composerConfig( KMKernel::config(), "Composer" );
  const bool spellChecking =
    composerConfig.readBoolEntry( "autoSpellChecking", true );
  mAutoSpellCheckingAction->setEnabled( !mUseExtEditor );
  mAutoSpellCheckingAction->setChecked( !mUseExtEditor && spellChecking );
  mEditor->slotAutoSpellCheckingToggled( !mUseExtEditor && spellChecking );
  connect( mAutoSpellCheckingAction, SIGNAL( toggled( bool ) ),
           mEditor, SLOT( slotAutoSpellCheckingToggled( bool ) ) );

  QStringList encodings = KMMsgBase::supportedEncodings(TRUE);
  encodings.prepend( i18n("Auto-Detect"));
  mEncodingAction->setItems( encodings );
  mEncodingAction->setCurrentItem( -1 );

  //these are checkable!!!
  mAllFieldsAction = new KToggleAction (i18n("&All Fields"), 0, this,
                                       SLOT(slotView()),
                                       actionCollection(), "show_all_fields");
  mIdentityAction = new KToggleAction (i18n("&Identity"), 0, this,
                                      SLOT(slotView()),
                                      actionCollection(), "show_identity");
  mDictionaryAction = new KToggleAction (i18n("&Dictionary"), 0, this,
                                         SLOT(slotView()),
                                         actionCollection(), "show_dictionary");
  mFccAction = new KToggleAction (i18n("Sent-Mail F&older"), 0, this,
                                 SLOT(slotView()),
                                 actionCollection(), "show_fcc");
  mTransportAction = new KToggleAction (i18n("&Mail Transport"), 0, this,
                                      SLOT(slotView()),
                                      actionCollection(), "show_transport");
  mFromAction = new KToggleAction (i18n("&From"), 0, this,
                                  SLOT(slotView()),
                                  actionCollection(), "show_from");
  mReplyToAction = new KToggleAction (i18n("&Reply To"), 0, this,
                                     SLOT(slotView()),
                                     actionCollection(), "show_reply_to");
  mToAction = new KToggleAction (i18n("&To"), 0, this,
                                SLOT(slotView()),
                                actionCollection(), "show_to");
  mCcAction = new KToggleAction (i18n("&CC"), 0, this,
                                SLOT(slotView()),
                                actionCollection(), "show_cc");
  mBccAction = new KToggleAction (i18n("&BCC"), 0, this,
                                 SLOT(slotView()),
                                 actionCollection(), "show_bcc");
  mSubjectAction = new KToggleAction (i18n("&Subject"), 0, this,
                                     SLOT(slotView()),
                                     actionCollection(), "show_subject");
  //end of checkable

  (void) new KAction (i18n("Append S&ignature"), 0, this,
                      SLOT(slotAppendSignature()),
                      actionCollection(), "append_signature");
  mAttachPK  = new KAction (i18n("Attach &Public Key..."), 0, this,
                           SLOT(slotInsertPublicKey()),
                           actionCollection(), "attach_public_key");
  mAttachMPK = new KAction (i18n("Attach &My Public Key"), 0, this,
                           SLOT(slotInsertMyPublicKey()),
                           actionCollection(), "attach_my_public_key");
  (void) new KAction (i18n("&Attach File..."), "attach",
                      0, this, SLOT(slotAttachFile()),
                      actionCollection(), "attach");
  mAttachRemoveAction = new KAction (i18n("&Remove Attachment"), 0, this,
                      SLOT(slotAttachRemove()),
                      actionCollection(), "remove");
  mAttachSaveAction = new KAction (i18n("&Save Attachment As..."), "filesave",0,
                      this, SLOT(slotAttachSave()),
                      actionCollection(), "attach_save");
  mAttachPropertiesAction = new KAction (i18n("Attachment Pr&operties..."), 0, this,
                      SLOT(slotAttachProperties()),
                      actionCollection(), "attach_properties");

  createStandardStatusBarAction();
  setStandardToolBarMenuEnabled(true);

  KStdAction::keyBindings(this, SLOT(slotEditKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
  KStdAction::preferences(kmkernel, SLOT(slotShowConfigurationDialog()), actionCollection());

  (void) new KAction (i18n("&Spellchecker..."), 0, this, SLOT(slotSpellcheckConfig()),
                      actionCollection(), "setup_spellchecker");

  mEncryptAction = new KToggleAction (i18n("&Encrypt Message"),
                                     "decrypted", 0,
                                     actionCollection(), "encrypt_message");
  mSignAction = new KToggleAction (i18n("&Sign Message"),
                                  "signature", 0,
                                  actionCollection(), "sign_message");
  // get PGP user id for the chosen identity
  const KMIdentity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  QCString pgpUserId = ident.pgpIdentity();
  mLastIdentityHasOpenPgpKey = !pgpUserId.isEmpty();

  mLastEncryptActionState =
    ( mSelectedCryptPlug && EncryptEmail_EncryptAll == mSelectedCryptPlug->encryptEmail() );
  mLastSignActionState =
    (    (!mSelectedCryptPlug && mAutoPgpSign)
      || ( mSelectedCryptPlug && SignEmail_SignAll == mSelectedCryptPlug->signEmail()) );

  // "Attach public key" is only possible if the built-in OpenPGP support is
  // used
  mAttachPK->setEnabled(Kpgp::Module::getKpgp()->usePGP());

  // "Attach my public key" is only possible if the built-in OpenPGP support is
  // used and the user specified his key for the current identity
  mAttachMPK->setEnabled( Kpgp::Module::getKpgp()->usePGP() &&
                         !pgpUserId.isEmpty() );

  if ( !mSelectedCryptPlug && !Kpgp::Module::getKpgp()->usePGP() ) {
    mEncryptAction->setEnabled( false );
    setEncryption( false );
    mSignAction->setEnabled( false );
    setSigning( false );
  }
  else if ( !mSelectedCryptPlug && pgpUserId.isEmpty() ) {
    setEncryption( false );
    setSigning( false );
  }
  else {
    setEncryption( mLastEncryptActionState );
    setSigning( mLastSignActionState );
  }

  connect(mEncryptAction, SIGNAL(toggled(bool)),
                         SLOT(slotEncryptToggled( bool )));
  connect(mSignAction,    SIGNAL(toggled(bool)),
                         SLOT(slotSignToggled(    bool )));

  if( kmkernel->cryptPlugList() && kmkernel->cryptPlugList()->count() ){
    QStringList lst;
    lst << i18n( "inline OpenPGP (built-in)" );
    CryptPlugWrapper* current;
    QPtrListIterator<CryptPlugWrapper> it( *kmkernel->cryptPlugList() );
    int idx=0;
    int i=1;
    while( ( current = it.current() ) ) {
        lst << i18n("%1 (plugin)").arg(current->displayName());
        if( mSelectedCryptPlug == current )
          idx = i;
        ++it;
        ++i;
    }

    mCryptoModuleAction = new KSelectAction( i18n( "Select &Crypto Module" ),
                             0, // no accel
                             this, SLOT( slotSelectCryptoModule() ),
                             actionCollection(),
                             "options_select_crypto" );
    mCryptoModuleAction->setItems( lst );
    mCryptoModuleAction->setCurrentItem( idx );
  }

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
  menu->insertItem(i18n("Fixed Font Widths"), this, SLOT(slotUpdateFont()));
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
void KMComposeWin::setMsg(KMMessage* newMsg, bool mayAutoSign,
                          bool allowDecryption, bool isModified)
{
  KMMessagePart bodyPart, *msgPart;
  int i, num;

  //assert(newMsg!=0);
  if(!newMsg)
    {
      kdDebug(5006) << "KMComposeWin::setMsg() : newMsg == 0!\n" << endl;
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

  // don't overwrite the header values with identity specific values
  // unless the identity is sticky
  if ( !mBtnIdentity->isChecked() ) {
    disconnect(mIdentity,SIGNAL(identityChanged(uint)),
               this, SLOT(slotIdentityChanged(uint)));
  }
   mIdentity->setCurrentIdentity( mId );
  if ( !mBtnIdentity->isChecked() ) {
    connect(mIdentity,SIGNAL(identityChanged(uint)),
            this, SLOT(slotIdentityChanged(uint)));
  }
  else {
    // make sure the header values are overwritten with the values of the
    // sticky identity (the slot isn't called by the signal for new messages
    // since the identity has already been set before the signal was connected)
    slotIdentityChanged( mId );
  }

  IdentityManager * im = kmkernel->identityManager();

  const KMIdentity & ident = im->identityForUoid( mIdentity->currentIdentity() );

  mOldSigText = ident.signatureText();

  // check for the presence of a DNT header, indicating that MDN's were
  // requested
  QString mdnAddr = newMsg->headerField("Disposition-Notification-To");
  mRequestMDNAction->setChecked( ( !mdnAddr.isEmpty() &&
                                  im->thatIsMe( mdnAddr ) ) || mAutoRequestMDN );

  // check for presence of a priority header, indicating urgent mail:
  mUrgentAction->setChecked( newMsg->isUrgent() );

  // enable/disable encryption if the message was/wasn't encrypted
  switch ( mMsg->encryptionState() ) {
    case KMMsgFullyEncrypted: // fall through
    case KMMsgPartiallyEncrypted:
      mLastEncryptActionState = true;
      break;
    case KMMsgNotEncrypted:
      mLastEncryptActionState = false;
      break;
    default: // nothing
      break;
  }

  // enable/disable signing if the message was/wasn't signed
  switch ( mMsg->signatureState() ) {
    case KMMsgFullySigned: // fall through
    case KMMsgPartiallySigned:
      mLastSignActionState = true;
      break;
    case KMMsgNotSigned:
      mLastSignActionState = false;
      break;
    default: // nothing
      break;
  }

  // get PGP user id for the currently selected identity
  QCString pgpUserId = ident.pgpIdentity();
  mLastIdentityHasOpenPgpKey = !pgpUserId.isEmpty();

  if ( mSelectedCryptPlug || Kpgp::Module::getKpgp()->usePGP() ) {
    if ( !mSelectedCryptPlug && pgpUserId.isEmpty() ) {
      setEncryption( false );
      setSigning( false );
    }
    else {
      setEncryption( mLastEncryptActionState );
      setSigning( mLastSignActionState );
    }
  }

  // "Attach my public key" is only possible if the user uses the built-in
  // OpenPGP support and he specified his key
  mAttachMPK->setEnabled( Kpgp::Module::getKpgp()->usePGP() &&
                         !pgpUserId.isEmpty() );

  QString transport = newMsg->headerField("X-KMail-Transport");
  if (!mBtnTransport->isChecked() && !transport.isEmpty())
  {
    for (int i = 0; i < mTransport->count(); i++)
      if (mTransport->text(i) == transport)
        mTransport->setCurrentItem(i);
    mTransport->setEditText( transport );
  }

  if (!mBtnFcc->isChecked())
  {
    if (!mMsg->fcc().isEmpty())
      setFcc(mMsg->fcc());
    else
      setFcc(ident.fcc());
  }

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  num = mMsg->numBodyParts();

  if (num > 0)
  {
    QCString bodyDecoded;
    mMsg->bodyPart(0, &bodyPart);

    int firstAttachment = (bodyPart.typeStr().lower() == "text") ? 1 : 0;
    if (firstAttachment)
    {
      mCharset = bodyPart.charset();
      if ( mCharset.isEmpty() || mCharset == "default" )
        mCharset = mDefCharset;

      bodyDecoded = bodyPart.bodyDecoded();

      if( allowDecryption )
        decryptOrStripOffCleartextSignature( bodyDecoded );

      // As nobody seems to know the purpose of the following line and
      // as it breaks word wrapping of long lines if drafts with attachments
      // are opened for editting in the composer (cf. Bug#41102) I comment it
      // out. Ingo, 2002-04-21
      //verifyWordWrapLengthIsAdequate(bodyDecoded);

      const QTextCodec *codec = KMMsgBase::codecForName(mCharset);
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
      QCString mimeType = msgPart->typeStr().lower() + '/'
                        + msgPart->subtypeStr().lower();
      // don't add the detached signature as attachment when editting a
      // PGP/MIME signed message
      if( mimeType != "application/pgp-signature" ) {
        addAttach(msgPart);
      }
    }
  } else{
    mCharset=mMsg->charset();
    if ( mCharset.isEmpty() ||  mCharset == "default" )
      mCharset = mDefCharset;

    QCString bodyDecoded = mMsg->bodyDecoded();

    if( allowDecryption )
      decryptOrStripOffCleartextSignature( bodyDecoded );

    const QTextCodec *codec = KMMsgBase::codecForName(mCharset);
    if (codec) {
      mEditor->setText(codec->toUnicode(bodyDecoded));
    } else
      mEditor->setText(QString::fromLocal8Bit(bodyDecoded));
  }

  setCharset(mCharset);

  if( mAutoSign && mayAutoSign ) {
    //
    // Espen 2000-05-16
    // Delay the signature appending. It may start a fileseletor.
    // Not user friendy if this modal fileseletor opens before the
    // composer.
    //
    QTimer::singleShot( 0, this, SLOT(slotAppendSignature()) );
  } else {
    kmkernel->dumpDeadLetters();
  }
  mEditor->setModified(isModified);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setFcc( const QString &idString )
{
  // check if the sent-mail folder still exists
  KMFolder *folder = kmkernel->folderMgr()->findIdString( idString );
  if ( !folder )
    folder = kmkernel->imapFolderMgr()->findIdString( idString );
  if ( !folder )
    folder = kmkernel->dimapFolderMgr()->findIdString( idString );
  if ( folder )
    mFcc->setFolder( idString );
  else
    mFcc->setFolder( kmkernel->sentFolder() );
}


//-----------------------------------------------------------------------------
bool KMComposeWin::queryClose ()
{
  if ( !mEditor->checkExternalEditorFinished() )
    return false;
  if (kmkernel->shuttingDown() || kapp->sessionSaving())
    return true;

  if(mEditor->isModified() || mEdtFrom->edited() || mEdtReplyTo->edited() ||
     mEdtTo->edited() || mEdtCc->edited() || mEdtBcc->edited() ||
     mEdtSubject->edited() || mAtmModified ||
     (mTransport->lineEdit() && mTransport->lineEdit()->edited()))
  {
    const int rc = KMessageBox::warningYesNoCancel(this,
           i18n("Do you want to discard the message or save it for later?"),
           i18n("Discard or Save Message"),
           i18n("&Save as Draft"),
           KStdGuiItem::discard() );
    if (rc == KMessageBox::Cancel)
      return false;
    else if (rc == KMessageBox::Yes)
      return slotSaveDraft();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool KMComposeWin::userForgotAttachment()
{
  KConfigGroup composer( KMKernel::config(), "Composer" );
  bool checkForForgottenAttachments =
    composer.readBoolEntry( "showForgottenAttachmentWarning", true );

  if ( !checkForForgottenAttachments || ( mAtmList.count() > 0 ) )
    return false;


  QStringList attachWordsList =
    composer.readListEntry( "attachment-keywords" );

  if ( attachWordsList.isEmpty() ) {
    // default value (FIXME: this is duplicated in configuredialog.cpp)
    attachWordsList << QString::fromLatin1("attachment")
                    << QString::fromLatin1("attached");
    if ( QString::fromLatin1("attachment") != i18n("attachment") )
      attachWordsList << i18n("attachment");
    if ( QString::fromLatin1("attached") != i18n("attached") )
      attachWordsList << i18n("attached");
  }

  QRegExp rx ( QString::fromLatin1("\\b") +
               attachWordsList.join("\\b|\\b") +
               QString::fromLatin1("\\b") );
  rx.setCaseSensitive( false );

  bool gotMatch = false;

  // check whether the subject contains one of the attachment key words
  // unless the message is a reply or a forwarded message
  QString subj = mEdtSubject->text();
  gotMatch =    ( KMMessage::stripOffPrefixes( subj ) == subj )
             && ( rx.search( subj ) >= 0 );

  if ( !gotMatch ) {
    // check whether the non-quoted text contains one of the attachment key
    // words
    QRegExp quotationRx ("^([ \\t]*([|>:}#]|[A-Za-z]+>))+");
    for ( int i = 0; i < mEditor->numLines(); ++i ) {
      QString line = mEditor->textLine( i );
      gotMatch =    ( quotationRx.search( line ) < 0 )
                 && ( rx.search( line ) >= 0 );
      if ( gotMatch )
        break;
    }
  }

  if ( !gotMatch )
    return false;

  int rc = KMessageBox::warningYesNoCancel( this,
             i18n("The message you have composed seems to refer to an "
                  "attached file but you have not attached anything.\n"
                  "Do you want to attach a file to your message?"),
             i18n("File Attachment Reminder"),
             i18n("&Attach File..."),
             i18n("&Send as Is") );
  if ( rc == KMessageBox::Cancel )
    return true;
  if ( rc == KMessageBox::Yes ) {
    slotAttachFile();
    //preceed with editing
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool KMComposeWin::applyChanges( bool backgroundMode )
{
  QString str, atmntStr;
  QString temp, replyAddr;

  //assert(mMsg!=0);
  if(!mMsg)
  {
    kdDebug(5006) << "KMComposeWin::applyChanges() : mMsg == 0!\n" << endl;
    return FALSE;
  }

  mBccMsgList.clear();

  if (mAutoCharset) {
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

//  kdDebug(5006) << "\n\n\n\nKMComposeWin::applyChanges: 1" << endl;
  mMsg->setTo(to());
  mMsg->setFrom(from());
  mMsg->setCc(cc());
  mMsg->setSubject(subject());
  mMsg->setReplyTo(replyTo());
  mMsg->setBcc(bcc());

  const KMIdentity & id
    = kmkernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );
  kdDebug(5006) << "\n\n\n\nKMComposeWin::applyChanges: " << mFcc->currentText() << "=="
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
  else mMsg->setHeaderField("X-KMail-Identity", QString::number( id.uoid() ));

  if (!replyTo().isEmpty()) replyAddr = replyTo();
  else replyAddr = from();

  if (mRequestMDNAction->isChecked())
    mMsg->setHeaderField("Disposition-Notification-To", replyAddr);
  else
    mMsg->removeHeaderField("Disposition-Notification-To");

  if (mUrgentAction->isChecked()) {
    mMsg->setHeaderField("X-PRIORITY", "2 (High)");
    mMsg->setHeaderField("Priority", "urgent");
  } else {
    mMsg->removeHeaderField("X-PRIORITY");
    mMsg->removeHeaderField("Priority");
  }

  _StringPair *pCH;
  for (pCH  = mCustHeaders.first();
       pCH != 0;
       pCH  = mCustHeaders.next()) {
    mMsg->setHeaderField(KMMsgBase::toUsAscii(pCH->name), pCH->value);
  }

  // we have to remember the Bcc because it might have been overwritten
  // by a custom header (therefore we can't use bcc() later) and because
  // mimelib removes addresses without domain part (therefore we can't use
  // mMsg->bcc() later)
  mBcc = mMsg->bcc();

  bool doSign    = mSignAction->isChecked()    && !mNeverSign;
  bool doEncrypt = mEncryptAction->isChecked() && !mNeverEncrypt;

  // get PGP user id for the chosen identity
  const KMIdentity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  QCString pgpUserId = ident.pgpIdentity();

  // check settings of composer buttons *and* attachment check boxes
  bool doSignCompletely    = doSign;
  bool doEncryptCompletely = doEncrypt;
  bool doEncryptPartially  = doEncrypt;
  if( mSelectedCryptPlug && ( !mAtmList.isEmpty() ) ) {
    int idx=0;
    KMMessagePart *attachPart;
    for( attachPart = mAtmList.first();
         attachPart;
         attachPart=mAtmList.next(), ++idx ) {
      if( encryptFlagOfAttachment( idx ) ) {
        doEncryptPartially = true;
      }
      else {
        doEncryptCompletely = false;
      }
      if( !signFlagOfAttachment(    idx ) )
        doSignCompletely = false;
    }
  }

  bool bOk = true;

  if( !doSignCompletely ) {
    if( mSelectedCryptPlug ) {
      // note: only ask for signing if "Warn me" flag is set! (khz)
      if( mSelectedCryptPlug->warnSendUnsigned() && !mNeverSign ) {
        int ret =
        KMessageBox::warningYesNoCancel( this,
          QString( "<qt><b>"
            + i18n("Warning:")
            + "</b><br>"
            + ((doSign && !doSignCompletely)
              ? i18n("You specified not to sign some parts of this message, but"
                     " you wanted to be warned not to send unsigned messages!")
              : i18n("You specified not to sign this message, but"
                     " you wanted to be warned not to send unsigned messages!") )
            + "<br>&nbsp;<br><b>"
            + i18n("Sign all parts of this message?")
            + "</b></qt>" ),
          i18n("Signature Warning"),
          KGuiItem( i18n("&Sign All Parts") ),
          KGuiItem( i18n("Send &as Is") ) );
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

  if( bOk ) {
    if( mNeverEncrypt )
      doEncrypt = false;
    else {
      // check whether all encrypted messages should be encrypted to self
      bool bEncryptToSelf = mSelectedCryptPlug
        ? mSelectedCryptPlug->alwaysEncryptToSelf()
        : Kpgp::Module::getKpgp()->encryptToSelf();
      // check whether we have the user's key if necessary
      bool bEncryptionPossible = !bEncryptToSelf || !pgpUserId.isEmpty();
      // check whether we are using OpenPGP (built-in or plug-in)
      bool bUsingOpenPgp = !mSelectedCryptPlug || ( mSelectedCryptPlug &&
                                                    ( -1 != mSelectedCryptPlug->libName().find( "openpgp" ) ) );
      // only try automatic encryption if all of the following conditions hold
      // a) the user enabled automatic encryption
      // b) we have the user's key if he wants to encrypt to himself
      // c) we are using OpenPGP
      // d) no message part is marked for encryption
      if( mAutoPgpEncrypt && bEncryptionPossible && bUsingOpenPgp &&
          !doEncryptPartially ) {
        // check if encryption is possible and if yes suggest encryption
        // first determine the complete list of recipients
        QString _to = to().simplifyWhiteSpace();
        if( !cc().isEmpty() ) {
          if( !_to.endsWith(",") )
            _to += ",";
          _to += cc().simplifyWhiteSpace();
        }
        if( !mBcc.isEmpty() ) {
          if( !_to.endsWith(",") )
            _to += ",";
          _to += mBcc.simplifyWhiteSpace();
        }
        QStringList allRecipients = KMMessage::splitEmailAddrList(_to);
        // now check if encrypting to these recipients is possible and desired
        Kpgp::Module *pgp = Kpgp::Module::getKpgp();
        int status = pgp->encryptionPossible( allRecipients );
        if( 1 == status ) {
          // encrypt all message parts
          doEncrypt = true;
          doEncryptCompletely = true;
        }
        else if( 2 == status ) {
          // the user wants to be asked or has to be asked
          KCursorSaver idle(KBusyPtr::idle());
          int ret;
          if( doSign )
            ret = KMessageBox::questionYesNoCancel( this,
                                                    i18n("<qt><p>You have a trusted OpenPGP key for every "
                                                         "recipient of this message and the message will "
                                                         "be signed.</p>"
                                                         "<p>Should this message also be "
                                                         "encrypted?</p></qt>"),
                                                    i18n("Encrypt Message?"),
                                                    KGuiItem( i18n("Sign && &Encrypt") ),
                                                    KGuiItem( i18n("&Sign Only") ) );
          else
            ret = KMessageBox::questionYesNoCancel( this,
                                                    i18n("<qt><p>You have a trusted OpenPGP key for every "
                                                         "recipient of this message.</p>"
                                                         "<p>Should this message be encrypted?</p></qt>"),
                                                    i18n("Encrypt Message?"),
                                                    KGuiItem( i18n("&Encrypt") ),
                                                    KGuiItem( i18n("&Don't Encrypt") ) );
          if( KMessageBox::Cancel == ret )
            return false;
          else if( KMessageBox::Yes == ret ) {
            // encrypt all message parts
            doEncrypt = true;
            doEncryptCompletely = true;
          }
        }
        else if( status == -1 )
        {
          // warn the user that there are conflicting encryption preferences
          KCursorSaver idle(KBusyPtr::idle());
          int ret =
            KMessageBox::warningYesNoCancel( this,
                                             i18n("<qt><p>There are conflicting encryption "
                                                  "preferences!</p>"
                                                  "<p>Should this message be encrypted?</p></qt>"),
                                             i18n("Encrypt Message?"),
                                             KGuiItem( i18n("&Encrypt") ),
                                             KGuiItem( i18n("&Don't Encrypt") ) );
          if( KMessageBox::Cancel == ret )
            bOk = false;
          else if( KMessageBox::Yes == ret ) {
            // encrypt all message parts
            doEncrypt = true;
            doEncryptCompletely = true;
          }
        }
      }
      else if( !doEncryptCompletely && mSelectedCryptPlug ) {
        // note: only ask for encrypting if "Warn me" flag is set! (khz)
        if( mSelectedCryptPlug->warnSendUnencrypted() ) {
          int ret =
            KMessageBox::warningYesNoCancel( this,
                                             QString( "<qt><b>"
                                                      + i18n("Warning:")
                                                      + "</b><br>"
                                                      + ((doEncrypt && !doEncryptCompletely)
                                                         ? i18n("You specified not to encrypt some parts of this message, but"
                                                                " you wanted to be warned not to send unencrypted messages!")
                                                         : i18n("You specified not to encrypt this message, but"
                                                                " you wanted to be warned not to send unencrypted messages!") )
                                                      + "<br>&nbsp;<br><b>"
                                                      + i18n("Encrypt all parts of this message?")
                                                      + "</b></qt>" ),
                                             i18n("Encryption Warning"),
                                             KGuiItem( i18n("&Encrypt All Parts") ),
                                             KGuiItem( i18n("Send &as Is") ) );
          if( ret == KMessageBox::Cancel )
            bOk = false;
          else if( ret == KMessageBox::Yes ) {
            doEncrypt = true;
            doEncryptCompletely = true;
          }
        }

        /*
          note: Processing the mSelectedCryptPlug->encryptEmail() flag here would
          be absolutely wrong: this is used for specifying
          if messages should be encrypted 'in general'.
          --> This sets the initial state of a freshly started Composer.
          --> This does *not* mean overriding user setting made while
          editing in that composer window!         (khz, 2002/06/26)
        */

      }
    }
  }

  if( bOk ) {
    // if necessary mark all attachments for signing/encryption
    if( mSelectedCryptPlug && ( !mAtmList.isEmpty() ) &&
        ( doSignCompletely || doEncryptCompletely ) ) {
      for( KMAtmListViewItem* lvi = (KMAtmListViewItem*)mAtmItemList.first();
           lvi;
           lvi = (KMAtmListViewItem*)mAtmItemList.next() ) {
        if( doSignCompletely )
          lvi->setSign( true );
        if( doEncryptCompletely )
          lvi->setEncrypt( true );
      }
    }
  }
  // This c-string (init empty here) is set by *first* testing of expiring
  // signature certificate and stops us from repeatedly asking same questions.
  QCString signCertFingerprint;

  // note: Must create extra message *before* calling compose on mMsg.
  KMMessage* extraMessage = new KMMessage( *mMsg );

  if( bOk )
    bOk = (composeMessage( pgpUserId,
                           *mMsg, doSign, doEncrypt, false,
                           signCertFingerprint ) == Kpgp::Ok);
  if( bOk ) {
    bool saveMessagesEncrypted = mSelectedCryptPlug ? mSelectedCryptPlug->saveMessagesEncrypted()
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
    // choose to obey the rules or to ignore them explicitly.
    if(    mSelectedCryptPlug
        && ( 0 <= mSelectedCryptPlug->libName().find( "smime",   0, false ) )
        && ( doEncrypt && saveMessagesEncrypted ) ){

      if( doEncrypt && saveMessagesEncrypted ) {
        QString headTxt =
          i18n("Warning: Your S/MIME Plug-in configuration is unsafe.");
        QString encrTxt =
          i18n("Encrypted messages should be stored in unencrypted form; saving locally in encrypted form is not allowed.");
        QString footTxt =
          i18n("Please correct the wrong settings in KMail's Plug-in configuration pages as soon as possible.");
        QString question =
          i18n("Store message in the recommended way?");

        if( KMessageBox::Yes == KMessageBox::warningYesNo(this,
                 "<qt><p><b>" + headTxt + "</b><br>" + encrTxt + "</p><p>"
                 + footTxt + "</p><p><b>" + question + "</b></p></qt>",
                 i18n("Unsafe S/MIME Configuration"),
                 KGuiItem( i18n("Save &Unencrypted") ),
                 KGuiItem( i18n("Save &Encrypted") ) ) ) {
            saveMessagesEncrypted = false;
        }
      }
    }
    kdDebug(5006) << "KMComposeWin::applyChanges(void)  -  Send encrypted=" << doEncrypt << "  Store encrypted=" << saveMessagesEncrypted << endl;
#endif
    if( doEncrypt && ! saveMessagesEncrypted ){
      if( mSelectedCryptPlug ){
        for( KMAtmListViewItem* entry = (KMAtmListViewItem*)mAtmItemList.first();
             entry;
             entry = (KMAtmListViewItem*)mAtmItemList.next() )
          entry->setEncrypt( false );
      }
      bOk = (composeMessage( pgpUserId,
                             *extraMessage,
                             doSign,
                             false,
                             true,
                             signCertFingerprint ) == Kpgp::Ok);
kdDebug(5006) << "KMComposeWin::applyChanges(void)  -  Store message in decrypted form." << endl;
      extraMessage->cleanupHeader();
      mMsg->setUnencryptedMsg( extraMessage );
    }
  }
  return bOk;
}


Kpgp::Result KMComposeWin::composeMessage( QCString pgpUserId,
                                           KMMessage& theMessage,
                                           bool doSign,
                                           bool doEncrypt,
                                           bool ignoreBcc,
                                           QCString& signCertFingerprint )
{
  Kpgp::Result result = Kpgp::Ok;
  // create informative header for those that have no mime-capable
  // email client
  theMessage.setBody( "This message is in MIME format." );

  // preprocess the body text
  QCString body = breakLinesAndApplyCodec();

  if (body.isNull()) return Kpgp::Failure;

  if (body.isEmpty()) body = "\n"; // don't crash

  // From RFC 3156:
  //  Note: The accepted OpenPGP convention is for signed data to end
  //  with a <CR><LF> sequence.  Note that the <CR><LF> sequence
  //  immediately preceding a MIME boundary delimiter line is considered
  //  to be part of the delimiter in [3], 5.1.  Thus, it is not part of
  //  the signed data preceding the delimiter line.  An implementation
  //  which elects to adhere to the OpenPGP convention has to make sure
  //  it inserts a <CR><LF> pair on the last line of the data to be
  //  signed and transmitted (signed message and transmitted message
  //  MUST be identical).
  // So make sure that the body ends with a <LF>.
  if( body[body.length()-1] != '\n' ) {
    kdDebug(5006) << "Added an <LF> on the last line" << endl;
    body += "\n";
  }

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
    mSelectedCryptPlug && ( !mAtmList.isEmpty() ) && (doSign || doEncrypt);

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
  oldBodyPart.setContentDisposition( "inline" );

  QCString boundaryCStr;

  bool isQP = kmkernel->msgSender()->sendQuotedPrintable();

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
    innerBodyPart.setContentDisposition( "inline" );
    QValueList<int> allowedCTEs;
    // the signed body must not be 8bit encoded
    innerBodyPart.setBodyAndGuessCte(body, allowedCTEs, !isQP && !doSign,
                                     doSign);
    innerBodyPart.setCharset(mCharset);
    innerBodyPart.setBodyEncoded( body );
    DwBodyPart* innerDwPart = theMessage.createDWBodyPart( &innerBodyPart );
    innerDwPart->Assemble();
    body  = "--";
    body +=     boundaryCStr;
    body +=                 "\n";
    body += innerDwPart->AsString().c_str();
    delete innerDwPart;
    innerDwPart = 0;
    // add all matching Attachments
    // NOTE: This code will be changed when KMime is complete.
    int idx;
    KMMessagePart *attachPart;
    for( idx=0, attachPart = mAtmList.first();
        attachPart;
        attachPart=mAtmList.next(),
        ++idx ) {
      bool bEncrypt = encryptFlagOfAttachment( idx );
      bool bSign = signFlagOfAttachment( idx );
      if( !mSelectedCryptPlug
          || ( ( doEncrypt == bEncrypt )  && ( doSign == bSign ) ) ) {
        // signed/encrypted body parts must be either QP or base64 encoded
        // Why not 7 bit? Because the LF->CRLF canonicalization would render
        // e.g. 7 bit encoded shell scripts unusable because of the CRs.
        if( bSign || bEncrypt ) {
          QCString cte = attachPart->cteStr().lower();
          if( ( "8bit" == cte )
              || ( ( attachPart->type() == DwMime::kTypeText )
                   && ( "7bit" == cte ) ) ) {
            QByteArray body = attachPart->bodyDecodedBinary();
            QValueList<int> dummy;
            attachPart->setBodyAndGuessCte(body, dummy, false, bSign);
            kdDebug(5006) << "Changed encoding of message part from "
                          << cte << " to " << attachPart->cteStr() << endl;
          }
        }
        innerDwPart = theMessage.createDWBodyPart( attachPart );
        innerDwPart->Assemble();
        body += "\n--";
        body +=       boundaryCStr;
        body +=                   "\n";
        body += innerDwPart->AsString().c_str();
        delete innerDwPart;
        innerDwPart = 0;
      }
    }
    body += "\n--";
    body +=       boundaryCStr;
    body +=                   "--\n";
  }
  else
  {
    QValueList<int> allowedCTEs;
    // the signed body must not be 8bit encoded
    oldBodyPart.setBodyAndGuessCte(body, allowedCTEs, !isQP && !doSign,
                                   doSign);
    oldBodyPart.setCharset(mCharset);
  }
  // create S/MIME body part for signing and/or encrypting
  oldBodyPart.setBodyEncoded( body );

  QCString encodedBody; // only needed if signing and/or encrypting

  if( doSign || doEncrypt ) {
    if( mSelectedCryptPlug ) {
      // get string representation of body part (including the attachments)
      DwBodyPart* dwPart = theMessage.createDWBodyPart( &oldBodyPart );
      dwPart->Assemble();
      encodedBody = dwPart->AsString().c_str();
      delete dwPart;
      dwPart = 0;

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

      if( (0 <= mSelectedCryptPlug->libName().find( "smime",   0, false )) ||
          (0 <= mSelectedCryptPlug->libName().find( "openpgp", 0, false )) ) {
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
    if( mSelectedCryptPlug ) {
      StructuringInfoWrapper structuring( mSelectedCryptPlug );

      // kdDebug(5006) << "\n\n\n******* c) encodedBody = \"" << encodedBody << "\"******\n\n" << endl;

      QByteArray signature = pgpSignedMsg( encodedBody,
                                           structuring,
                                           signCertFingerprint );
      kdDebug(5006) << "                           size of signature: " << signature.count() << "\n" << endl;
      result = signature.isEmpty() ? Kpgp::Failure : Kpgp::Ok;
      if( result == Kpgp::Ok ) {
        result = processStructuringInfo( QString::fromUtf8( mSelectedCryptPlug->bugURL() ),
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
                                      newBodyPart ) ? Kpgp::Ok : Kpgp::Failure;
        if( result == Kpgp::Ok ) {
          if( newBodyPart.name().isEmpty() )
            newBodyPart.setName("signed message part");
          newBodyPart.setCharset( mCharset );
        } else
          KMessageBox::sorry(this, mErrorProcessingStructuringInfo );
      }
    }
    else if ( !doEncrypt ) {
      // we try calling the *old* build-in code for OpenPGP clearsigning
      Kpgp::Block block;
      block.setText( encodedBody );

      // clearsign the message
      result = block.clearsign( pgpUserId, mCharset );

      if( result == Kpgp::Ok ) {
        newBodyPart.setType(                       oldBodyPart.type() );
        newBodyPart.setSubtype(                    oldBodyPart.subtype() );
        newBodyPart.setCharset(                    oldBodyPart.charset() );
        newBodyPart.setContentTransferEncodingStr( oldBodyPart.contentTransferEncodingStr() );
        newBodyPart.setContentDescription(         oldBodyPart.contentDescription() );
        newBodyPart.setContentDisposition(         oldBodyPart.contentDisposition() );
        newBodyPart.setBodyEncoded( block.text() );
      }
      else if ( result == Kpgp::Failure )
        KMessageBox::sorry(this,
                   i18n("<qt><p>This message could not be signed.</p>%1</qt>")
                   .arg( mErrorNoCryptPlugAndNoBuildIn ));
    }
  }

  if( result == Kpgp::Ok ) {
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
        result = encryptMessage( yetAnotherMessageForBCC,
                              tmpRecips,
                              doSign, doEncrypt, encodedBody,
                              previousBoundaryLevel,
                              oldBodyPart,
                              earlyAddAttachments, allAttachmentsAreInBody,
                              tmpNewBodyPart,
                                 signCertFingerprint );
        if( result == Kpgp::Ok ){
          yetAnotherMessageForBCC->setHeaderField( "X-KMail-Recipients", *it );
          mBccMsgList.append( yetAnotherMessageForBCC );
          //kdDebug(5006) << "###BCC AFTER \"" << theMessage.asString() << "\""<<endl;
        }
      }
      theMessage.setHeaderField( "X-KMail-Recipients", recipientsWithoutBcc.join(",") );
    }

    // run encrypting for public recipient(s)
    if( result == Kpgp::Ok ){
      result = encryptMessage( &theMessage,
                            recipientsWithoutBcc,
                            doSign, doEncrypt, encodedBody,
                            previousBoundaryLevel,
                            oldBodyPart,
                            earlyAddAttachments, allAttachmentsAreInBody,
                            newBodyPart,
                               signCertFingerprint );
    }
    //        kdDebug(5006) << "###AFTER ENCRYPTION\"" << theMessage.asString() << "\""<<endl;
  }
  return result;
}


bool KMComposeWin::queryExit ()
{
  return true;
}

Kpgp::Result KMComposeWin::getEncryptionCertificates(
                                             const QStringList& recipients,
                                             QCString& encryptionCertificates )
{
  Kpgp::Result result = Kpgp::Ok;

  // find out whether we are dealing with the OpenPGP or the S/MIME plugin
  if ( -1 != mSelectedCryptPlug->libName().find( "openpgp" ) ) {
    // We are dealing with the OpenPGP plugin. Use Kpgp to determine
    // the encryption keys.
    // get the OpenPGP key ID for the chosen identity
    const KMIdentity & ident =
      kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
    const QCString userKeyId = ident.pgpIdentity();
    Kpgp::Module *pgp = Kpgp::Module::getKpgp();
    Kpgp::KeyIDList encryptionKeyIds;

    // temporarily set encrypt_to_self to the value specified in the
    // plugin configuration. this value is used implicitely by the
    // function which determines the encryption keys.
    const bool bEncryptToSelf_Old = pgp->encryptToSelf();
    pgp->setEncryptToSelf( mSelectedCryptPlug->alwaysEncryptToSelf() );
    result = pgp->getEncryptionKeys( encryptionKeyIds, recipients, userKeyId );
    // reset encrypt_to_self to the old value
    pgp->setEncryptToSelf( bEncryptToSelf_Old );

    if ( result == Kpgp::Ok && !encryptionKeyIds.isEmpty() ) {
      // loop over all key IDs
      for ( Kpgp::KeyIDList::ConstIterator it = encryptionKeyIds.begin();
           it != encryptionKeyIds.end(); ++it ) {
        const Kpgp::Key* key = pgp->publicKey( *it );
        if ( key ) {
          QCString certFingerprint = key->primaryFingerprint();
          kdDebug(5006) << "Fingerprint of encryption key: "
                        << certFingerprint << endl;
          // add this key to the list of encryption keys
          if( !encryptionCertificates.isEmpty() )
            encryptionCertificates += '\1';
          encryptionCertificates += certFingerprint;
        }
      }
    }
  }
  else {
    // S/MIME
    QStringList allRecipients = recipients;
    if ( mSelectedCryptPlug->alwaysEncryptToSelf() )
      allRecipients << from();
    for ( QStringList::ConstIterator it = allRecipients.begin();
          it != allRecipients.end();
          ++it ) {
      QCString certFingerprint = getEncryptionCertificate( *it );

      if ( certFingerprint.isEmpty() ) {
        // most likely the user canceled the certificate selection
        encryptionCertificates.truncate( 0 );
        return Kpgp::Canceled;
      }

      certFingerprint.remove( 0, certFingerprint.findRev( '(' ) + 1 );
      certFingerprint.truncate( certFingerprint.length() - 1 );
      kdDebug(5006) << "\n\n                    Recipient: " << *it
                    <<   "\nFingerprint of encryption key: "
                    << certFingerprint << "\n\n" << endl;

      const bool certOkay =
        checkForEncryptCertificateExpiry( *it, certFingerprint );
      if( certOkay ) {
        if( !encryptionCertificates.isEmpty() )
          encryptionCertificates += '\1';
        encryptionCertificates += certFingerprint;
      }
      else {
        // ###### This needs to be improved: Tell the user that the certificate
        // ###### expired and let him choose a different one.
        encryptionCertificates.truncate( 0 );
        return Kpgp::Failure;
      }
    }
  }
  return result;
}

Kpgp::Result KMComposeWin::encryptMessage( KMMessage* msg,
                                   const QStringList& recipients,
                                   bool doSign,
                                   bool doEncrypt,
                                   const QCString& encodedBody,
                                   int previousBoundaryLevel,
                                   const KMMessagePart& oldBodyPart,
                                   bool earlyAddAttachments,
                                   bool allAttachmentsAreInBody,
                                   KMMessagePart newBodyPart,
                                   QCString& signCertFingerprint )
{
  Kpgp::Result result = Kpgp::Ok;
  if(!msg)
  {
    kdDebug(5006) << "KMComposeWin::encryptMessage() : msg == 0!\n" << endl;
    return Kpgp::Failure;
  }

  // This c-string (init empty here) is set by *first* testing of expiring
  // encryption certificate: stops us from repeatedly asking same questions.
  QCString encryptCertFingerprints;

  // determine the encryption certificates in case we need them
  if ( mSelectedCryptPlug ) {
    bool encrypt = doEncrypt;
    if( !encrypt ) {
      // check whether at least one attachment is marked for encryption
      for ( KMAtmListViewItem* atmlvi =
              static_cast<KMAtmListViewItem*>( mAtmItemList.first() );
            atmlvi;
            atmlvi = static_cast<KMAtmListViewItem*>( mAtmItemList.next() ) ) {
        if ( atmlvi->isEncrypt() ) {
          encrypt = true;
          break;
        }
      }
    }
    if ( encrypt ) {
      result = getEncryptionCertificates( recipients,
                                          encryptCertFingerprints );
      if ( result != Kpgp::Ok )
        return result;
      if ( encryptCertFingerprints.isEmpty() ) {
        // the user wants to send the message unencrypted
        setEncryption( false, false );
        doEncrypt = false;
      }
    }
  }

  // encrypt message
  if( doEncrypt ) {
    QCString innerContent;
    if( doSign && mSelectedCryptPlug ) {
      DwBodyPart* dwPart = msg->createDWBodyPart( &newBodyPart );
      dwPart->Assemble();
      innerContent = dwPart->AsString().c_str();
      delete dwPart;
      dwPart = 0;
    } else
      innerContent = encodedBody;

    // now do the encrypting:
    {
      if( mSelectedCryptPlug ) {
        if( (0 <= mSelectedCryptPlug->libName().find( "smime",   0, false )) ||
            (0 <= mSelectedCryptPlug->libName().find( "openpgp", 0, false )) ) {
          // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
          // according to RfC 2633, 3.1.1 Canonicalization
          kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
          innerContent = KMMessage::lf2crlf( innerContent );
          kdDebug(5006) << "                                                       done." << endl;
        }

        StructuringInfoWrapper structuring( mSelectedCryptPlug );

        QByteArray encryptedBody;
        result = pgpEncryptedMsg( encryptedBody, innerContent,
                                  structuring,
                                  encryptCertFingerprints );

        if( Kpgp::Ok == result ) {
          result = processStructuringInfo( QString::fromUtf8( mSelectedCryptPlug->bugURL() ),
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
                                        newBodyPart ) ? Kpgp::Ok : Kpgp::Failure;
          if( Kpgp::Ok == result ) {
            if( newBodyPart.name().isEmpty() )
              newBodyPart.setName("encrypted message part");
          } else if ( Kpgp::Failure == result )
            KMessageBox::sorry(this, mErrorProcessingStructuringInfo);
        } else if ( Kpgp::Failure == result )
          KMessageBox::sorry(this,
            i18n("<qt><p><b>This message could not be encrypted!</b></p>"
                 "<p>The Crypto Plug-in '%1' did not return an encoded text "
                 "block.</p>"
                 "<p>Probably a recipient's public key was not found or is "
                 "untrusted.</p></qt>")
            .arg(mSelectedCryptPlug->libName()));
      } else {
        // we try calling the *old* build-in code for OpenPGP encrypting
        Kpgp::Block block;
        block.setText( innerContent );

        // get PGP user id for the chosen identity
        const KMIdentity & ident =
          kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
        QCString pgpUserId = ident.pgpIdentity();

        // encrypt the message
        result = block.encrypt( recipients, pgpUserId, doSign, mCharset );

        if( Kpgp::Ok == result ) {
          newBodyPart.setBodyEncodedBinary( block.text() );
          newBodyPart.setCharset( oldBodyPart.charset() );
        }
        else if( Kpgp::Failure == result ) {
          KMessageBox::sorry(this,
            i18n("<qt><p>This message could not be encrypted!</p>%1</qt>")
           .arg( mErrorNoCryptPlugAndNoBuildIn ));
        }
      }
    }
  }

  // process the attachments that are not included into the body
  if( Kpgp::Ok == result ) {
    const KMMessagePart& ourFineBodyPart( (doSign || doEncrypt)
                                          ? newBodyPart
                                          : oldBodyPart );
    if( !mAtmList.isEmpty()
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

        const bool cryptFlagsDifferent = mSelectedCryptPlug
                        ? (    (encryptFlagOfAttachment( idx ) != doEncrypt)
                            || (signFlagOfAttachment(    idx ) != doSign) )
                        : false;
        const bool encryptThisNow = !mNeverEncrypt && ( cryptFlagsDifferent ? encryptFlagOfAttachment( idx ) : false );
        const bool signThisNow    = !mNeverSign && ( cryptFlagsDifferent ? signFlagOfAttachment(    idx ) : false );

        if( cryptFlagsDifferent || !earlyAddAttachments ) {

          if( encryptThisNow || signThisNow ) {

            KMMessagePart& rEncryptMessagePart( *attachPart );

            // prepare the attachment's content
            // signed/encrypted body parts must be either QP or base64 encoded
            QCString cte = attachPart->cteStr().lower();
            if( ( "8bit" == cte )
                || ( ( attachPart->type() == DwMime::kTypeText )
                     && ( "7bit" == cte ) ) ) {
              QByteArray body = attachPart->bodyDecodedBinary();
              QValueList<int> dummy;
              attachPart->setBodyAndGuessCte(body, dummy, false, true);
              kdDebug(5006) << "Changed encoding of message part from "
                            << cte << " to " << attachPart->cteStr() << endl;
            }
            DwBodyPart* innerDwPart = msg->createDWBodyPart( attachPart );
            innerDwPart->Assemble();
            QCString encodedAttachment = innerDwPart->AsString().c_str();
            delete innerDwPart;
            innerDwPart = 0;

            if( (0 <= mSelectedCryptPlug->libName().find( "smime",   0, false )) ||
                (0 <= mSelectedCryptPlug->libName().find( "openpgp", 0, false )) ) {
              // replace simple LFs by CRLFs for all MIME supporting CryptPlugs
              // according to RfC 2633, 3.1.1 Canonicalization
              kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
              encodedAttachment = KMMessage::lf2crlf( encodedAttachment );
              kdDebug(5006) << "                                                       done." << endl;
            }

            // sign this attachment
            if( signThisNow ) {
kdDebug(5006) << "                                 sign " << idx << ". attachment separately" << endl;
              StructuringInfoWrapper structuring( mSelectedCryptPlug );

              QByteArray signature = pgpSignedMsg( encodedAttachment,
                                                   structuring,
                                                   signCertFingerprint );
              result = signature.isEmpty() ? Kpgp::Failure : Kpgp::Ok;
              if( Kpgp::Ok == result ) {
                result = processStructuringInfo( QString::fromUtf8( mSelectedCryptPlug->bugURL() ),
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
                                              newAttachPart ) ? Kpgp::Ok : Kpgp::Failure;
                if( Kpgp::Ok == result ) {
                  if( newAttachPart.name().isEmpty() )
                    newAttachPart.setName("signed attachment");
                  if( encryptThisNow ) {
                    rEncryptMessagePart = newAttachPart;
                    DwBodyPart* dwPart = msg->createDWBodyPart( &newAttachPart );
                    dwPart->Assemble();
                    encodedAttachment = dwPart->AsString().c_str();
                    delete dwPart;
                    dwPart = 0;
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
              StructuringInfoWrapper structuring( mSelectedCryptPlug );
              QByteArray encryptedBody;
              result = pgpEncryptedMsg( encryptedBody, encodedAttachment,
                                        structuring,
                                        encryptCertFingerprints );

              if( Kpgp::Ok == result ) {
                result = processStructuringInfo( QString::fromUtf8( mSelectedCryptPlug->bugURL() ),
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
                                              newAttachPart ) ? Kpgp::Ok : Kpgp::Failure;
                if( Kpgp::Ok == result ) {
                  if( newAttachPart.name().isEmpty() ) {
                    newAttachPart.setName("encrypted attachment");
                  }
                } else if ( Kpgp::Failure == result )
                  KMessageBox::sorry(this, mErrorProcessingStructuringInfo);
              }
            }
            msg->addBodyPart( &newAttachPart );
          } else
            msg->addBodyPart( attachPart );

kdDebug(5006) << "                                 added " << idx << ". attachment to this Multipart/Mixed" << endl;
        } else {
kdDebug(5006) << "                                 " << idx << ". attachment was part of the BODY already" << endl;
        }
      }
    } else {
      if( ourFineBodyPart.originalContentTypeStr() ) {
        //msg->headers().Assemble();
        //kdDebug(5006) << "\n\n\nKMComposeWin::composeMessage():\n      A.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
        msg->headers().ContentType().FromString( ourFineBodyPart.originalContentTypeStr() );
        //msg->headers().Assemble();
        //kdDebug(5006) << "\n\n\nKMComposeWin::composeMessage():\n      B.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
        msg->headers().ContentType().Parse();
        //msg->headers().Assemble();
        //kdDebug(5006) << "\n\n\nKMComposeWin::composeMessage():\n      C.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
kdDebug(5006) << "KMComposeWin::encryptMessage() : set top level Content-Type from originalContentTypeStr()" << endl;
      } else {
        msg->headers().ContentType().FromString( ourFineBodyPart.typeStr() + "/" + ourFineBodyPart.subtypeStr() );
kdDebug(5006) << "KMComposeWin::encryptMessage() : set top level Content-Type from typeStr()/subtypeStr()" << endl;
      }
      //msg->headers().Assemble();
      //kdDebug(5006) << "\n\n\nKMComposeWin::composeMessage():\n      D.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
      msg->setCharset( ourFineBodyPart.charset() );
      //msg->headers().Assemble();
      //kdDebug(5006) << "\n\n\nKMComposeWin::composeMessage():\n      E.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
      msg->setHeaderField( "Content-Transfer-Encoding",
                            ourFineBodyPart.contentTransferEncodingStr() );
      //msg->headers().Assemble();
      //kdDebug(5006) << "\n\n\nKMComposeWin::composeMessage():\n      F.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
      msg->setHeaderField( "Content-Description",
                            ourFineBodyPart.contentDescription() );
      msg->setHeaderField( "Content-Disposition",
                            ourFineBodyPart.contentDisposition() );

kdDebug(5006) << "KMComposeWin::encryptMessage() : top level headers and body adjusted" << endl;

      // set body content
      // msg->setBody( ourFineBodyPart.body() );
      msg->setMultiPartBody( ourFineBodyPart.body() );
      //kdDebug(5006) << "\n\n\n\n\n\n\nKMComposeWin::composeMessage():\n      99.:\n\n\n\n|||" << msg->asString() << "|||\n\n\n\n\n\n" << endl;
      //msg->headers().Assemble();
      //kdDebug(5006) << "\n\n\nKMComposeWin::composeMessage():\n      Z.:\n\n" << msg->headerAsString() << "|||\n\n\n\n\n" << endl;
    }

  }
  return result;
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
  kdDebug(5006) << "||| entering KMComposeWin::processStructuringInfo()" << endl;
#endif
  //assert(mMsg!=0);
  if(!mMsg)
  {
    kdDebug(5006) << "KMComposeWin::processStructuringInfo() : mMsg == 0!\n" << endl;
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
    mainDwStr = mainHeader + "\n\n";
    DwBodyPart mainDwPa( mainDwStr, 0 );
    mainDwPa.Parse();
    KMMessage::bodyPart(&mainDwPa, &resultingPart);
/*
kdDebug(5006) << "***************************************" << endl;
kdDebug(5006) << "***************************************" << endl;
kdDebug(5006) << "***************************************" << endl;
kdDebug(5006) << mainHeader << endl;
kdDebug(5006) << "***************************************" << endl;
kdDebug(5006) << "***************************************" << endl;
kdDebug(5006) << "***************************************" << endl;
kdDebug(5006) << resultingPart.additionalCTypeParamStr() << endl;
kdDebug(5006) << "***************************************" << endl;
kdDebug(5006) << "***************************************" << endl;
kdDebug(5006) << "***************************************" << endl;
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
        tmpDwPa = 0;
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

        DwBodyPart versDwPa( versStr + "\n\n", 0 );
        versDwPa.Parse();
        KMMessagePart versKmPa;
        KMMessage::bodyPart(&versDwPa, &versKmPa);
        versKmPa.setBodyEncoded( structuring.data.bodyTextVersion );
        // store string representation of the cleartext headers
        versCStr = versDwPa.Headers().AsString().c_str();
        // store string representation of encoded cleartext
        versCStr += "\n";
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

        DwBodyPart codeDwPa( codeStr + "\n\n", 0 );
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
        codeCStr += "\n";
        codeCStr += codeKmPa.body();
#if 0
        kdDebug(5006) << "***************************************" << endl;
        kdDebug(5006) << "***************************************" << endl;
        kdDebug(5006) << codeCStr << endl;
        kdDebug(5006) << "***************************************" << endl;
        kdDebug(5006) << "***************************************" << endl;
#endif
      } else {

        // Plugin error!
        KMessageBox::sorry( this,
          i18n("<qt><p>Error: The Crypto Plug-in '%1' returned<br>"
               "       \" structuring.makeMultiMime \"<br>"
               "but did <b>not</b> specify a Content-Type header "
               "for the ciphertext that was generated.</p>"
               "<p>Please report this bug:<br>%2</p></qt>")
          .arg(mSelectedCryptPlug->libName())
          .arg(bugURL) );
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
    //       texts containing all necessary line breaks.
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
          i18n("<qt><p>Error: The Crypto Plug-in '%1' did not return "
               "any encoded data.</p>"
               "<p>Please report this bug:<br>%2</p></qt>")
          .arg(mSelectedCryptPlug->libName())
          .arg(bugURL) );
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
  kdDebug(5006) << "||| leaving KMComposeWin::processStructuringInfo()\n||| returning: " << bOk << endl;
#endif

  return bOk;
}

//-----------------------------------------------------------------------------
QCString KMComposeWin::breakLinesAndApplyCodec()
{
  QString text;
  QCString cText;

  if (mDisableBreaking)
      text = mEditor->text();
  else
      text = mEditor->brokenText();

  text.truncate(text.length()); // to ensure text.size()==text.length()+1

  {
    // Provide a local scope for newText.
    QString newText;
    const QTextCodec *codec = KMMsgBase::codecForName(mCharset);

    if (mCharset == "us-ascii") {
      cText = KMMsgBase::toUsAscii(text);
      newText = QString::fromLatin1(cText);
    } else if (codec == 0) {
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
      KCursorSaver idle(KBusyPtr::idle());
      bool anyway = (KMessageBox::warningYesNo(this,
                                               i18n("<qt>Not all characters fit into the chosen"
                                                    " encoding.<br><br>Send the message anyway?</qt>"),
                                               i18n("Some characters will be lost"),
                                               i18n("Send"), i18n("Change Encoding") ) == KMessageBox::Yes);
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
  if( mSelectedCryptPlug ) {
    kdDebug(5006) << "\nKMComposeWin::pgpSignedMsg calling CRYPTPLUG "
                  << mSelectedCryptPlug->libName() << endl;

    bool bSign = true;

    if( signCertFingerprint.isEmpty() ) {
      // find out whether we are dealing with the OpenPGP or the S/MIME plugin
      if( -1 != mSelectedCryptPlug->libName().find( "openpgp" ) ) {
        // We are dealing with the OpenPGP plugin. Use Kpgp to determine
        // the signing key.
        // get the OpenPGP key ID for the chosen identity
        const KMIdentity & ident =
          kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
        QCString userKeyId = ident.pgpIdentity();
        if( !userKeyId.isEmpty() ) {
          Kpgp::Module *pgp = Kpgp::Module::getKpgp();
          Kpgp::Key* key = pgp->publicKey( userKeyId );
          if( key ) {
            signCertFingerprint = key->primaryFingerprint();
            kdDebug(5006) << "                          Signer: " << from()
                          << "\nFingerprint of signature key: "
                          << QString( signCertFingerprint ) << endl;
          }
          else {
            KMessageBox::sorry( this,
                                i18n("<qt>This message could not be signed "
                                     "because the OpenPGP key which should be "
                                     "used for signing messages with this "
                                     "identity couldn't be found in your "
                                     "keyring.<br><br>"
                                     "You can change the OpenPGP key "
                                     "which should be used with the current "
                                     "identity in the identity configuration.</qt>"),
                                i18n("Missing Signing Key") );
            bSign = false;
          }
        }
        else {
          KMessageBox::sorry( this,
                              i18n("<qt>This message could not be signed "
                                   "because you didn't define the OpenPGP "
                                   "key which should be used for signing "
                                   "messages with this identity.<br><br>"
                                   "You can define the OpenPGP key "
                                   "which should be used with the current "
                                   "identity in the identity configuration.</qt>"),
                              i18n("Undefined Signing Key") );
          bSign = false;
        }
      }
      else { // S/MIME
        int certSize = 0;
        QByteArray certificate;
        QString selectedCert;
        KListBoxDialog dialog( selectedCert, "", i18n( "&Select certificate:") );
        dialog.resize( 700, 200 );

        QCString signer = from().utf8();
        signer.replace('\x001', ' ');

        kdDebug(5006) << "\n\nRetrieving keys for: " << from() << endl;
        char* certificatePtr = 0;
        bool findCertsOk = mSelectedCryptPlug->findCertificates(
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
                        dialog.setCaption( i18n("Select Certificate [%1]")
                                           .arg( from() ) );
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
        kdDebug(5006) << ds << endl;
#endif

        // Check for expiry of the signer, CA, and Root certificate.
        // Only do the expiry check if the plugin has this feature
        // and if there in *no* fingerprint in signCertFingerprint already.
        if( mSelectedCryptPlug->hasFeature( Feature_WarnSignCertificateExpiry ) ){
            int sigDaysLeft = mSelectedCryptPlug->signatureCertificateDaysLeftToExpiry( signCertFingerprint );
            if( mSelectedCryptPlug->signatureCertificateExpiryNearWarning() &&
                sigDaysLeft <
                mSelectedCryptPlug->signatureCertificateExpiryNearInterval() ) {
                QString txt1;
                if( 0 < sigDaysLeft )
                    txt1 = i18n( "The certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer." ).arg( sigDaysLeft );
                else if( 0 > sigDaysLeft )
                    txt1 = i18n( "The certificate you want to use for signing expired %1 days ago.<br>This means that the recipients will not be able to check your signature." ).arg( -sigDaysLeft );
                else
                    txt1 = i18n( "The certificate you want to use for signing expires today.<br>This means that, starting from tomorrow, the recipients will not be able to check your signature any longer." );
                int ret = KMessageBox::warningYesNo( this,
                            i18n( "<qt><p>%1</p>"
                                  "<p>Do you still want to use this "
                                  "certificate?</p></qt>" )
                            .arg( txt1 ),
                            i18n( "Certificate Warning" ),
                            KGuiItem( i18n("&Use Certificate") ),
                            KGuiItem( i18n("&Don't Use Certificate") ) );
                if( ret == KMessageBox::No )
                    bSign = false;
            }

            if( bSign && ( 0 <= mSelectedCryptPlug->libName().find( "smime", 0, false ) ) ) {
                int rootDaysLeft = mSelectedCryptPlug->rootCertificateDaysLeftToExpiry( signCertFingerprint );
                if( mSelectedCryptPlug->rootCertificateExpiryNearWarning() &&
                    rootDaysLeft <
                    mSelectedCryptPlug->rootCertificateExpiryNearInterval() ) {
                    QString txt1;
                    if( 0 < rootDaysLeft )
                        txt1 = i18n( "The root certificate of the certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer." ).arg( rootDaysLeft );
                    else if( 0 > rootDaysLeft )
                        txt1 = i18n( "The root certificate of the certificate you want to use for signing expired %1 days ago.<br>This means that the recipients will not be able to check your signature." ).arg( -rootDaysLeft );
                    else
                        txt1 = i18n( "The root certificate of the certificate you want to use for signing expires today.<br>This means that beginning from tomorrow, the recipients will not be able to check your signature any longer." );
                    int ret = KMessageBox::warningYesNo( this,
                                i18n( "<qt><p>%1</p>"
                                      "<p>Do you still want to use this "
                                      "certificate?</p></qt>" )
                                .arg( txt1 ),
                                i18n( "Certificate Warning" ),
                                KGuiItem( i18n("&Use Certificate") ),
                                KGuiItem( i18n("&Don't Use Certificate") ) );
                    if( ret == KMessageBox::No )
                        bSign = false;
                }
            }


            if( bSign && ( 0 <= mSelectedCryptPlug->libName().find( "smime", 0, false ) ) ) {
                int caDaysLeft = mSelectedCryptPlug->caCertificateDaysLeftToExpiry( signCertFingerprint );
                if( mSelectedCryptPlug->caCertificateExpiryNearWarning() &&
                    caDaysLeft <
                    mSelectedCryptPlug->caCertificateExpiryNearInterval() ) {
                    QString txt1;
                    if( 0 < caDaysLeft )
                        txt1 = i18n( "The CA certificate of the certificate you want to use for signing expires in %1 days.<br>This means that after this period, the recipients will not be able to check your signature any longer." ).arg( caDaysLeft );
                    else if( 0 > caDaysLeft )
                        txt1 = i18n( "The CA certificate of the certificate you want to use for signing expired %1 days ago.<br>This means that the recipients will not be able to check your signature." ).arg( -caDaysLeft );
                    else
                        txt1 = i18n( "The CA certificate of the certificate you want to use for signing expires today.<br>This means that beginning from tomorrow, the recipients will not be able to check your signature any longer." );
                    int ret = KMessageBox::warningYesNo( this,
                                i18n( "<qt><p>%1</p>"
                                      "<p>Do you still want to use this "
                                      "certificate?</p></qt>" )
                                .arg( txt1 ),
                                i18n( "Certificate Warning" ),
                                KGuiItem( i18n("&Use Certificate") ),
                                KGuiItem( i18n("&Don't Use Certificate") ) );
                    if( ret == KMessageBox::No )
                        bSign = false;
                }
            }
        }
        // Check whether the sender address of the signer is contained in
        // the certificate, but only do this if the plugin has this feature.
        if( mSelectedCryptPlug->hasFeature( Feature_WarnSignEmailNotInCertificate ) ) {
            if( bSign && mSelectedCryptPlug->warnNoCertificate() &&
                !mSelectedCryptPlug->isEmailInCertificate( QString( KMMessage::getEmailAddr( from() ) ).utf8(), signCertFingerprint ) )  {
                QString txt1 = i18n( "The certificate you want to use for signing does not contain your sender email address.<br>This means that it is not possible for the recipients to check whether the email really came from you." );
                int ret = KMessageBox::warningYesNo( this,
                            i18n( "<qt><p>%1</p>"
                                  "<p>Do you still want to use this "
                                  "certificate?</p></qt>" )
                            .arg( txt1 ),
                            i18n( "Certificate Warning" ),
                            KGuiItem( i18n("&Use Certificate") ),
                            KGuiItem( i18n("&Don't Use Certificate") ) );
                if( ret == KMessageBox::No )
                    bSign = false;
            }
        }
    } // if( signCertFingerprint.isEmpty() )


    // Finally sign the message, but only if the plugin has this feature.
    if( mSelectedCryptPlug->hasFeature( Feature_SignMessages ) ) {
        size_t cipherLen;

        const char* cleartext = cText;
        char* ciphertext  = 0;

        if( mDebugComposerCrypto ){
            QFile fileS( "dat_11_sign.input" );
            if( fileS.open( IO_WriteOnly ) ) {
                QDataStream ds( &fileS );
                ds.writeRawBytes( cleartext, strlen( cleartext ) );
                fileS.close();
            }
        }

        if ( bSign ){
            int   errId = 0;
            char* errTxt = 0;
            if ( mSelectedCryptPlug->signMessage( cleartext,
                                         &ciphertext, &cipherLen,
                                         signCertFingerprint,
                                         structuring,
                                         &errId,
                                         &errTxt ) ){
                if( mDebugComposerCrypto ){
                    QFile fileD( "dat_12_sign.output" );
                    if( fileD.open( IO_WriteOnly ) ) {
                        QDataStream ds( &fileD );
                        ds.writeRawBytes( ciphertext, cipherLen );
                        fileD.close();
                    }
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
                    kdDebug(5006) << ds << endl << endl;
                }
                signature.assign( ciphertext, cipherLen );
            } else if ( errId == /*GPGME_Canceled*/20 ) {
                return false;
            } else {
                QString error("#");
                error += QString::number( errId );
                error += "  :  ";
                if( errTxt )
                  error += errTxt;
                else
                  error += i18n("[unknown error]");
                KMessageBox::sorry(this,
                  i18n("<qt><p><b>This message could not be signed!</b></p>"
                       "<p>The Crypto Plug-In '%1' reported the following "
                       "details:</p>"
                       "<p><i>%2</i></p>"
                       "<p>Your configuration might be invalid or the Plug-In "
                       "damaged.</p>"
                       "<p><b>Please contact your system "
                       "administrator.</b></p></qt>")
                  .arg(mSelectedCryptPlug->libName())
                  .arg( error ) );
            }
            // we do NOT call a "delete ciphertext" !
            // since "signature" will take care for it (is a QByteArray)
            delete errTxt;
            errTxt = 0;
        }
    }

    // PENDING(khz,kalle) Warn if there was no signature? (because of
    // a problem or because the plugin does not allow signing?

/* ----------------------------- */


    kdDebug(5006) << "\nKMComposeWin::pgpSignedMsg returning from CRYPTPLUG.\n" << endl;
  } else
      KMessageBox::sorry(this,
        i18n("<qt>No active Crypto Plug-In could be found.<br><br>"
             "Please activate a Plug-In in the configuration dialog.</qt>"));
  return signature;
}


//-----------------------------------------------------------------------------
Kpgp::Result KMComposeWin::pgpEncryptedMsg( QByteArray & encryptedBody,
                                            QCString cText,
                                            StructuringInfoWrapper& structuring,
                                            QCString& encryptCertFingerprints )
{
  Kpgp::Result result = Kpgp::Ok;

  // we call the cryptplug
  if( mSelectedCryptPlug ) {
    kdDebug(5006) << "\nKMComposeWin::pgpEncryptedMsg: going to call CRYPTPLUG "
                  << mSelectedCryptPlug->libName() << endl;


#if 0
    // ### This has been removed since according to the Sphinx specs the CRLs
    // have to be refreshed every day. This means warning that the CRL will
    // expire in one day is pointless. Disabling this has been recommended
    // by Karl-Heinz Zimmer.

    // Check for CRL expiry, but only if the plugin has this
    // feature.
    if( encryptCertFingerprints.isEmpty() &&
        mSelectedCryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) &&
        mSelectedCryptPlug->hasFeature( Feature_EncryptionCRLs ) ) {
        int crlDaysLeft = mSelectedCryptPlug->encryptionCRLsDaysLeftToExpiry();
        if( mSelectedCryptPlug->encryptionUseCRLs() &&
            mSelectedCryptPlug->encryptionCRLExpiryNearWarning() &&
            crlDaysLeft <
            mSelectedCryptPlug->encryptionCRLNearExpiryInterval() ) {
            int ret = KMessageBox::warningYesNo( this,
                        i18n( "<qt><p>The certification revocation lists, that "
                              "are used for checking the validity of the "
                              "certificate you want to use for encrypting, "
                              "expire in %1 days.</p>"
                              "<p>Do you still want to encrypt this message?"
                              "</p></qt>" )
                        .arg( crlDaysLeft ),
                        i18n( "Certificate Warning" ),
                        KGuiItem( i18n( "&Encrypt" ) ),
                        KGuiItem( i18n( "&Don't Encrypt" ) ) );
            if( ret == KMessageBox::No )
              return Kpgp::Canceled;
        }
    }
#endif

    // PENDING(khz,kalle) Warn if no encryption?

    const char* cleartext  = cText;
    const char* ciphertext = 0;

    // Actually do the encryption, if the plugin supports this
    size_t cipherLen;

    int errId = 0;
    char* errTxt = 0;
    if( mSelectedCryptPlug->hasFeature( Feature_EncryptMessages ) &&
        mSelectedCryptPlug->encryptMessage( cleartext,
                                            &ciphertext, &cipherLen,
                                            encryptCertFingerprints,
                                            structuring,
                                            &errId,
                                            &errTxt )
        && ciphertext )
      encryptedBody.assign( ciphertext, cipherLen );
    else {
      QString error("#");
      error += QString::number( errId );
      error += "  :  ";
      if( errTxt )
        error += errTxt;
      else
        error += i18n("[unknown error]");
      KMessageBox::sorry(this,
                  i18n("<qt><p><b>This message could not be encrypted!</b></p>"
                       "<p>The Crypto Plug-In '%1' reported the following "
                       "details:</p>"
                       "<p><i>%2</i></p>"
                       "<p>Your configuration might be invalid or the Plug-In "
                       "damaged.</p>"
                       "<p><b>Please contact your system "
                       "administrator.</b></p></qt>")
                  .arg(mSelectedCryptPlug->libName())
                  .arg( error ) );
    }
    delete errTxt;
    errTxt = 0;

    // we do NOT delete the "ciphertext" !
    // bacause "encoding" will take care for it (is a QByteArray)

    kdDebug(5006) << "\nKMComposeWin::pgpEncryptedMsg: returning from CRYPTPLUG.\n" << endl;

  } else
      KMessageBox::sorry(this,
        i18n("<qt>No active Crypto Plug-In could be found.<br><br>"
             "Please activate a Plug-In in the configuration dialog.</qt>"));

  return result;
}


//-----------------------------------------------------------------------------
QCString
KMComposeWin::getEncryptionCertificate( const QString& recipient )
{
  bool bEncrypt = true;

  QCString addressee = recipient.utf8();
  addressee.replace('\x001', ' ');
  kdDebug(5006) << "\n\n1st try:  Retrieving keys for: " << recipient << endl;


  QString selectedCert;
  KListBoxDialog dialog( selectedCert, "", i18n( "&Select certificate:" ) );
  dialog.resize( 700, 200 );
  bool useDialog;
  int certSize = 0;
  QByteArray certificateList;

  bool askForDifferentSearchString = false;
  do {

    certSize = 0;
    char* certificatePtr = 0;
    bool findCertsOk;
    if( askForDifferentSearchString )
      findCertsOk = false;
    else {
      findCertsOk = mSelectedCryptPlug->findCertificates( &(*addressee),
                                                &certificatePtr,
                                                &certSize,
                                                false )
                    && (0 < certSize);
      kdDebug(5006) << "         keys retrieved successfully: " << findCertsOk << "\n" << endl;
      kdDebug(5006) << "findCertificates() 1st try returned " << certificatePtr << endl;
      if( findCertsOk )
        certificateList.assign( certificatePtr, certSize );
    }
    while( !findCertsOk ) {
      bool bOk = false;
      addressee = KInputDialog::getText(
                    askForDifferentSearchString
                    ? i18n("Look for Other Certificates")
                    : i18n("No Certificate Found"),
                    i18n("Enter different address for recipient %1 "
                        "or enter \" * \" to see all certificates:")
                    .arg(recipient),
                    addressee, &bOk, this )
                  .stripWhiteSpace().utf8();
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
        findCertsOk = mSelectedCryptPlug->findCertificates(
                                      &(*addressee),
                                      &certificatePtr,
                                      &certSize,
                                      false )
                      && (0 < certSize);
        kdDebug(5006) << "         keys retrieved successfully: " << findCertsOk << "\n" << endl;
        kdDebug(5006) << "findCertificates() 2nd try returned " << certificatePtr << endl;
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
      bool bAlwaysShowDialog = true;

      useDialog = false;
      int iA = 0;
      int iZ = 0;
      while( iZ < certSize ) {
        if( (certificateList.at(iZ) == '\1') || (certificateList.at(iZ) == '\0') ) {
          kdDebug(5006) << "iA=" << iA << " iZ=" << iZ << endl;
          char c = certificateList.at(iZ);
          if( (bAlwaysShowDialog || (c == '\1')) && !useDialog ) {
            // set up selection dialog
            useDialog = true;
            dialog.setCaption( i18n( "Select Certificate for Encryption [%1]" )
                              .arg( recipient ) );
            dialog.setLabelAbove(
              i18n( "&Select certificate for recipient %1:" )
              .arg( recipient ) );
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
          .arg( addressee )
          .arg( recipient ) );
        dialog.entriesLB->setFocus();
        dialog.entriesLB->setSelected( 0, true );
        askForDifferentSearchString = (dialog.exec() != QDialog::Accepted);
      }
    }
  } while ( askForDifferentSearchString );

  if( bEncrypt )
    return selectedCert.utf8();
  else
    return QCString();
}


bool KMComposeWin::checkForEncryptCertificateExpiry( const QString& recipient,
                                                     const QCString& certFingerprint )
{
  bool bEncrypt = true;

  // Check for expiry of various certificates, but only if the
  // plugin supports this.
  if( mSelectedCryptPlug->hasFeature( Feature_WarnEncryptCertificateExpiry ) ) {
    QString captionWarn = i18n( "Certificate Warning [%1]" ).arg( recipient );

    int encRecvDaysLeft =
      mSelectedCryptPlug->receiverCertificateDaysLeftToExpiry( certFingerprint );
    if( mSelectedCryptPlug->receiverCertificateExpiryNearWarning() &&
        encRecvDaysLeft <
        mSelectedCryptPlug->receiverCertificateExpiryNearWarningInterval() ) {
      QString txt1;
      if( 0 < encRecvDaysLeft )
        txt1 = i18n( "The certificate of the recipient you want to send this "
                     "message to expires in %1 days.<br>This means that after "
                     "this period, the recipient will not be able to read "
                     "your message any longer." )
               .arg( encRecvDaysLeft );
      else if( 0 > encRecvDaysLeft )
        txt1 = i18n( "The certificate of the recipient you want to send this "
                     "message to expired %1 days ago.<br>This means that the "
                     "recipient will not be able to read your message." )
               .arg( -encRecvDaysLeft );
      else
        txt1 = i18n( "The certificate of the recipient you want to send this "
                     "message to expires today.<br>This means that beginning "
                     "from tomorrow, the recipient will not be able to read "
                     "your message any longer." );
      int ret = KMessageBox::warningYesNo( this,
                                  i18n( "<qt><p>%1</p>"
                                        "<p>Do you still want to use "
                                        "this certificate?</p></qt>" )
                                  .arg( txt1 ),
                                  captionWarn,
                                  KGuiItem( i18n("&Use Certificate") ),
                                  KGuiItem( i18n("&Don't Use Certificate") ) );
      if( ret == KMessageBox::No )
        bEncrypt = false;
    }

    if( bEncrypt ) {
      int certInChainDaysLeft =
        mSelectedCryptPlug->certificateInChainDaysLeftToExpiry( certFingerprint );
      if( mSelectedCryptPlug->certificateInChainExpiryNearWarning() &&
          certInChainDaysLeft <
          mSelectedCryptPlug->certificateInChainExpiryNearWarningInterval() ) {
        QString txt1;
        if( 0 < certInChainDaysLeft )
          txt1 = i18n( "One of the certificates in the chain of the "
                       "certificate of the recipient you want to send this "
                       "message to expires in %1 days.<br>"
                       "This means that after this period, the recipient "
                       "might not be able to read your message any longer." )
                 .arg( certInChainDaysLeft );
        else if( 0 > certInChainDaysLeft )
          txt1 = i18n( "One of the certificates in the chain of the "
                       "certificate of the recipient you want to send this "
                       "message to expired %1 days ago.<br>"
                       "This means that the recipient might not be able to "
                       "read your message." )
                 .arg( -certInChainDaysLeft );
        else
          txt1 = i18n( "One of the certificates in the chain of the "
                       "certificate of the recipient you want to send this "
                       "message to expires today.<br>This means that "
                       "beginning from tomorrow, the recipient might not be "
                       "able to read your message any longer." );
        int ret = KMessageBox::warningYesNo( this,
                                  i18n( "<qt><p>%1</p>"
                                        "<p>Do you still want to use this "
                                        "certificate?</p></qt>" )
                                  .arg( txt1 ),
                                  captionWarn,
                                  KGuiItem( i18n("&Use Certificate") ),
                                  KGuiItem( i18n("&Don't Use Certificate") ) );
        if( ret == KMessageBox::No )
          bEncrypt = false;
      }
    }

      /*  The following test is not necessary, since we _got_ the certificate
          by looking for all certificates of our addressee - so it _must_ be valid
          for the respective address!

          // Check whether the receiver address is contained in
          // the certificate.
          if( bEncrypt && mSelectedCryptPlug->receiverEmailAddressNotInCertificateWarning() &&
          !mSelectedCryptPlug->isEmailInCertificate( QString( KMMessage::getEmailAddr( recipient ) ).utf8(),
          certFingerprint ) )  {
          int ret = KMessageBox::warningYesNo( this,
          i18n( "The certificate does not contain the email address of the sender.\nThis means that it will not be possible for the recipient to read this message.\n\nDo you still want to use this certificate?" ),
          captionWarn );
          if( ret == KMessageBox::No )
          bEncrypt = false;
          }
      */
  }

  return bEncrypt;
}


//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(const KURL aUrl)
{
  if ( !aUrl.isValid() ) {
    KMessageBox::sorry( this, i18n( "<qt><p>KMail couldn't recognize the location of the attachment (%1).</p>"
                                 "<p>You have to specify the full path if you wish to attach a file.</p></qt>" )
                        .arg( aUrl.prettyURL() ) );
    return;
  }
  KIO::TransferJob *job = KIO::get(aUrl);
  KIO::Scheduler::scheduleJob( job );
  atmLoadData ld;
  ld.url = aUrl;
  ld.data = QByteArray();
  ld.insert = false;
  mMapAtmLoadData.insert(job, ld);
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
    mAtmListView->setMinimumSize(100, 80);
    mAtmListView->setMaximumHeight( 100 );
    mAtmListView->show();
    resize(size());
  }

  // add a line in the attachment listbox
  KMAtmListViewItem *lvi = new KMAtmListViewItem( mAtmListView );
  msgPartToItem(msgPart, lvi);
  mAtmItemList.append(lvi);

  slotUpdateAttachActions();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdateAttachActions()
{
  int selectedCount = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it ) {
    if ( (*it)->isSelected() ) {
      ++selectedCount;
    }
  }

  mAttachRemoveAction->setEnabled( selectedCount >= 1 );
  mAttachSaveAction->setEnabled( selectedCount == 1 );
  mAttachPropertiesAction->setEnabled( selectedCount == 1 );
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
  assert(msgPart != 0);

  if (!msgPart->fileName().isEmpty())
    lvi->setText(0, msgPart->fileName());
  else
    lvi->setText(0, msgPart->name());
  lvi->setText(1, KIO::convertSize( msgPart->decodedSize()));
  lvi->setText(2, msgPart->contentTransferEncodingStr());
  lvi->setText(3, prettyMimeType(msgPart->typeStr() + "/" + msgPart->subtypeStr()));
  if( mSelectedCryptPlug ) {
    lvi->enableCryptoCBs( true );
    lvi->setEncrypt( mEncryptAction->isChecked() );
    lvi->setSign(    mSignAction->isChecked() );
  } else {
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

  if( mAtmList.isEmpty() )
  {
    mAtmListView->hide();
    mGrid->setRowStretch(mNumHeaders+1, 0);
    mAtmListView->setMinimumSize(0, 0);
    resize(size());
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
void KMComposeWin::addrBookSelInto()
{
  AddressesDialog dlg( this );
  QString txt;
  QStringList lst;

  txt = mEdtTo->text().stripWhiteSpace();
  if ( !txt.isEmpty() ) {
      lst = KMMessage::splitEmailAddrList( txt );
      dlg.setSelectedTo( lst );
  }

  txt = mEdtCc->text().stripWhiteSpace();
  if ( !txt.isEmpty() ) {
      lst = KMMessage::splitEmailAddrList( txt );
      dlg.setSelectedCC( lst );
  }

  txt = mEdtBcc->text().stripWhiteSpace();
  if ( !txt.isEmpty() ) {
      lst = KMMessage::splitEmailAddrList( txt );
      dlg.setSelectedBCC( lst );
  }

  dlg.setRecentAddresses( RecentAddresses::self()->kabcAddresses() );

  if (dlg.exec()==QDialog::Rejected) return;

  mEdtTo->setText( dlg.to().join(", ") );
  mEdtTo->setEdited( true );

  mEdtCc->setText( dlg.cc().join(", ") );
  mEdtCc->setEdited( true );

  mEdtBcc->setText( dlg.bcc().join(", ") );
  mEdtBcc->setEdited( true );
}


//-----------------------------------------------------------------------------
void KMComposeWin::setCharset(const QCString& aCharset, bool forceDefault)
{
  if ((forceDefault && mForceReplyCharset) || aCharset.isEmpty())
    mCharset = mDefCharset;
  else
    mCharset = aCharset.lower();

  if ( mCharset.isEmpty() || mCharset == "default" )
     mCharset = mDefCharset;

  if (mAutoCharset)
  {
    mEncodingAction->setCurrentItem( 0 );
    return;
  }

  QStringList encodings = mEncodingAction->items();
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
      mEncodingAction->setCurrentItem( i );
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
  KMAddrBookExternal::openAddressBook(this);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookFrom()
{
  addrBookSelInto();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookReplyTo()
{
  addrBookSelInto();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBookTo()
{
  addrBookSelInto();
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
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.find(job);
  assert(it != mMapAtmLoadData.end());
  QBuffer buff((*it).data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(data.data(), data.size());
  buff.close();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFileResult(KIO::Job *job)
{
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.find(job);
  assert(it != mMapAtmLoadData.end());
  if (job->error())
  {
    mMapAtmLoadData.remove(it);
    job->showErrorDialog();
    return;
  }
  if ((*it).insert)
  {
    (*it).data.resize((*it).data.size() + 1);
    (*it).data[(*it).data.size() - 1] = '\0';
    if ( const QTextCodec * codec = KGlobal::charsets()->codecForName((*it).encoding) )
      mEditor->insert( codec->toUnicode( (*it).data ) );
    else
      mEditor->insert( QString::fromLocal8Bit( (*it).data ) );
    mMapAtmLoadData.remove(it);
    return;
  }
  QString name;
  QString urlStr = (*it).url.prettyURL();
  KMMessagePart* msgPart;
  int i;

  KCursorSaver busy(KBusyPtr::busy());

  // ask the job for the mime type of the file
  QString mimeType = static_cast<KIO::MimetypeJob*>(job)->mimetype();

  i = urlStr.findRev('/');
  if( i == -1 )
    name = urlStr;
  else if( i + 1 < int( urlStr.length() ) )
    name = urlStr.mid( i + 1, 256 );
  else {
    // URL ends with '/' (e.g. http://www.kde.org/)
    // guess a reasonable filename
    if( mimeType == "text/html" )
      name = "index.html";
    else {
      // try to determine a reasonable extension
      QStringList patterns( KMimeType::mimeType( mimeType )->patterns() );
      QString ext;
      if( !patterns.isEmpty() ) {
        ext = patterns[0];
        int i = ext.findRev( '.' );
        if( i == -1 )
          ext.prepend( '.' );
        else if( i > 0 )
          ext = ext.mid( i );
      }
      name = QString("unknown") += ext;
    }
  }

  QCString encoding = KMMsgBase::autoDetectCharset(mCharset,
    KMMessage::preferredCharsets(), name);
  if (encoding.isEmpty()) encoding = "utf-8";
  QCString encName = KMMsgBase::encodeRFC2231String(name, encoding);
  bool RFC2231encoded = name != QString(encName);

  // create message part
  msgPart = new KMMessagePart;
  msgPart->setName(name);
  QValueList<int> allowedCTEs;
  msgPart->setBodyAndGuessCte((*it).data, allowedCTEs,
                              !kmkernel->msgSender()->sendQuotedPrintable());
  kdDebug(5006) << "autodetected cte: " << msgPart->cteStr() << endl;
  int slash = mimeType.find( '/' );
  if( slash == -1 )
    slash = mimeType.length();
  msgPart->setTypeStr( mimeType.left( slash ).latin1() );
  msgPart->setSubtypeStr( mimeType.mid( slash + 1 ).latin1() );
  msgPart->setContentDisposition(QCString("attachment;\n\tfilename")
    + ((RFC2231encoded) ? "*" : "") +  "=\"" + encName + "\"");

  mMapAtmLoadData.remove(it);

  msgPart->setCharset(mCharset);

  // show message part dialog, if not configured away (default):
  KConfigGroup composer(KMKernel::config(), "Composer");
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
      msgPart = 0;
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
  KFileDialog fdlg(QString::null, QString::null, this, 0, TRUE);
  fdlg.setCaption(i18n("Insert File"));
  fdlg.toolBar()->insertCombo(KMMsgBase::supportedEncodings(FALSE), 4711,
    false, 0, 0, 0);
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
  mMapAtmLoadData.insert(job, ld);
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotAttachFileResult(KIO::Job *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotAttachFileData(KIO::Job *, const QByteArray &)));
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSetCharset()
{
  if (mEncodingAction->currentItem() == 0)
  {
    mAutoCharset = true;
    return;
  }
  mAutoCharset = false;

  mCharset = KGlobal::charsets()->encodingForName( mEncodingAction->
    currentText() ).latin1();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSelectCryptoModule()
{
  mSelectedCryptPlug = 0;
  int sel = mCryptoModuleAction->currentItem();
  int i = 1;  // start at 1 since 0'th entry is "inline OpenPGP (builtin)"
  for ( CryptPlugWrapperListIterator it( *(kmkernel->cryptPlugList()) ) ;
        it.current() ;
        ++it, ++i )
    if( i == sel ){
      mSelectedCryptPlug = it.current();
      break;
    }
  if( mSelectedCryptPlug ) {
    // if the encrypt/sign columns are hidden then show them
    if( 0 == mAtmListView->columnWidth( mAtmColEncrypt ) ) {
      // set/unset signing/encryption for all attachments according to the
      // state of the global sign/encrypt action
      if( !mAtmList.isEmpty() ) {
        for( KMAtmListViewItem* lvi = (KMAtmListViewItem*)mAtmItemList.first();
             lvi;
             lvi = (KMAtmListViewItem*)mAtmItemList.next() ) {
          lvi->setSign( mSignAction->isChecked() );
          lvi->setEncrypt( mEncryptAction->isChecked() );
        }
      }
      int totalWidth = 0;
      // determine the total width of the columns
      for( int col=0; col < mAtmColEncrypt; col++ )
        totalWidth += mAtmListView->columnWidth( col );
      int reducedTotalWidth = totalWidth - mAtmEncryptColWidth
                                         - mAtmSignColWidth;
      // reduce the width of all columns so that the encrypt and sign column
      // fit
      int usedWidth = 0;
      for( int col=0; col < mAtmColEncrypt-1; col++ ) {
        int newWidth = mAtmListView->columnWidth( col ) * reducedTotalWidth
                                                       / totalWidth;
        mAtmListView->setColumnWidth( col, newWidth );
        usedWidth += newWidth;
      }
      // the last column before the encrypt column gets the remaining space
      // (because of rounding errors the width of this column isn't calculated
      // the same way as the width of the other columns)
      mAtmListView->setColumnWidth( mAtmColEncrypt-1,
                                    reducedTotalWidth - usedWidth );
      mAtmListView->setColumnWidth( mAtmColEncrypt, mAtmEncryptColWidth );
      mAtmListView->setColumnWidth( mAtmColSign,    mAtmSignColWidth );
      for( KMAtmListViewItem* lvi = (KMAtmListViewItem*)mAtmItemList.first();
           lvi;
           lvi = (KMAtmListViewItem*)mAtmItemList.next() ) {
        lvi->enableCryptoCBs( true );
      }
    }
  } else {
    // if the encrypt/sign columns are visible then hide them
    if( 0 != mAtmListView->columnWidth( mAtmColEncrypt ) ) {
      mAtmEncryptColWidth = mAtmListView->columnWidth( mAtmColEncrypt );
      mAtmSignColWidth = mAtmListView->columnWidth( mAtmColSign );
      int totalWidth = 0;
      // determine the total width of the columns
      for( int col=0; col < mAtmListView->columns(); col++ )
        totalWidth += mAtmListView->columnWidth( col );
      int reducedTotalWidth = totalWidth - mAtmEncryptColWidth
                                         - mAtmSignColWidth;
      // increase the width of all columns so that the visible columns take
      // up the whole space
      int usedWidth = 0;
      for( int col=0; col < mAtmColEncrypt-1; col++ ) {
        int newWidth = mAtmListView->columnWidth( col ) * totalWidth
                                                       / reducedTotalWidth;
        mAtmListView->setColumnWidth( col, newWidth );
        usedWidth += newWidth;
      }
      // the last column before the encrypt column gets the remaining space
      // (because of rounding errors the width of this column isn't calculated
      // the same way as the width of the other columns)
      mAtmListView->setColumnWidth( mAtmColEncrypt-1, totalWidth - usedWidth );
      mAtmListView->setColumnWidth( mAtmColEncrypt, 0 );
      mAtmListView->setColumnWidth( mAtmColSign,    0 );
      for( KMAtmListViewItem* lvi = (KMAtmListViewItem*)mAtmItemList.first();
           lvi;
           lvi = (KMAtmListViewItem*)mAtmItemList.next() ) {
        lvi->enableCryptoCBs( false );
      }
    }
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertMyPublicKey()
{
  KMMessagePart* msgPart;

  KCursorSaver busy(KBusyPtr::busy());

  // get PGP user id for the chosen identity
  QCString pgpUserId =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() ).pgpIdentity();

  QCString armoredKey = Kpgp::Module::getKpgp()->getAsciiPublicKey(pgpUserId);
  if (armoredKey.isEmpty())
  {
    KCursorSaver idle(KBusyPtr::idle());
    KMessageBox::sorry( this, i18n("Unable to obtain your public key.") );
    return;
  }

  // create message part
  msgPart = new KMMessagePart;
  msgPart->setName(i18n("My OpenPGP key"));
  msgPart->setTypeStr("application");
  msgPart->setSubtypeStr("pgp-keys");
  QValueList<int> dummy;
  msgPart->setBodyAndGuessCte(armoredKey, dummy, false);
  msgPart->setContentDisposition("attachment;\n\tfilename=public_key.asc");

  // add the new attachment to the list
  addAttach(msgPart);
  rethinkFields(); //work around initial-size bug in Qt-1.32
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
    msgPart->setName(i18n("OpenPGP key 0x%1").arg(keyID));
    msgPart->setTypeStr("application");
    msgPart->setSubtypeStr("pgp-keys");
    QValueList<int> dummy;
    msgPart->setBodyAndGuessCte(armoredKey, dummy, false);
    msgPart->setContentDisposition("attachment;\n\tfilename=0x" + keyID + ".asc");

    // add the new attachment to the list
    addAttach(msgPart);
    rethinkFields(); //work around initial-size bug in Qt-1.32
  } else {
    KMessageBox::sorry( this,
                        i18n( "Unable to obtain the selected public key." ) );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachPopupMenu(QListViewItem *, const QPoint &, int)
{
  if (!mAttachMenu)
  {
     mAttachMenu = new QPopupMenu(this);

     mAttachMenu->insertItem(i18n("to view", "View"), this,
                             SLOT(slotAttachView()));
     mAttachMenu->insertItem(i18n("Remove"), this, SLOT(slotAttachRemove()));
     mSaveAsId = mAttachMenu->insertItem( i18n("Save As..."), this,
                                          SLOT( slotAttachSave() ) );
     mPropertiesId = mAttachMenu->insertItem( i18n("Properties"), this,
                                              SLOT( slotAttachProperties() ) );
     mAttachMenu->insertSeparator();
     mAttachMenu->insertItem(i18n("Add Attachment..."), this, SLOT(slotAttachFile()));
  }

  int selectedCount = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it ) {
    if ( (*it)->isSelected() ) {
      ++selectedCount;
    }
  }
  bool multiSelection = ( selectedCount > 1 );
  mAttachMenu->setItemEnabled( mSaveAsId, !multiSelection );
  mAttachMenu->setItemEnabled( mPropertiesId, !multiSelection );

  mAttachMenu->popup(QCursor::pos());
}

//-----------------------------------------------------------------------------
int KMComposeWin::currentAttachmentNum()
{
  int i = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it, ++i )
    if ( *it == mAtmListView->currentItem() )
      return i;
  return -1;
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachProperties()
{
  int idx = currentAttachmentNum();

  if (idx < 0) return;

  KMMessagePart* msgPart = mAtmList.at(idx);
  msgPart->setCharset(mCharset);

  KMMsgPartDialogCompat dlg;
  dlg.setMsgPart(msgPart);
  KMAtmListViewItem* listItem = (KMAtmListViewItem*)(mAtmItemList.at(idx));
  if( mSelectedCryptPlug && listItem ) {
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
      if( mSelectedCryptPlug ) {
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
  int i = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      viewAttach( i );
    }
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::viewAttach( int index )
{
  QString str, pname;
  KMMessagePart* msgPart;
  msgPart = mAtmList.at(index);
  pname = msgPart->name().stripWhiteSpace();
  if (pname.isEmpty()) pname=msgPart->contentDescription();
  if (pname.isEmpty()) pname="unnamed";

  KTempFile* atmTempFile = new KTempFile();
  mAtmTempList.append( atmTempFile );
  atmTempFile->setAutoDelete( true );
  kByteArrayToFile(msgPart->bodyDecodedBinary(), atmTempFile->name(), false, false,
    false);
  KMReaderMainWin *win = new KMReaderMainWin(msgPart, false,
    atmTempFile->name(), pname, KMMsgBase::codecForName(mCharset) );
  win->show();
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

  kmkernel->byteArrayToRemoteFile(msgPart->bodyDecodedBinary(), url);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachRemove()
{
  bool attachmentRemoved = false;
  int i = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ) {
    if ( (*it)->isSelected() ) {
      removeAttach( i );
      attachmentRemoved = true;
    }
    else {
      ++it;
      ++i;
    }
  }

  if ( attachmentRemoved ) {
    mEditor->setModified( true );
    slotUpdateAttachActions();
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
  mEditor->setFont( mFixedFontAction && (mFixedFontAction->isChecked())
    ? mFixedFont : mBodyFont );
}

QString KMComposeWin::quotePrefixName() const
{
    if ( !msg() )
        return QString::null;

    KConfig *config=KMKernel::config();
    KConfigGroupSaver saver(config, "General");

    int languageNr = config->readNumEntry("reply-current-language",0);
    config->setGroup( QString("KMMessage #%1").arg(languageNr) );

    QString quotePrefix = config->readEntry("indent-prefix", ">%_");
    quotePrefix = msg()->formatString(quotePrefix);
    return quotePrefix;
}

void KMComposeWin::slotPasteAsQuotation()
{
    if( mEditor->hasFocus() && msg() )
    {
        QString quotePrefix = quotePrefixName();
        QString s = QApplication::clipboard()->text();
        if (!s.isEmpty()) {
            for (int i=0; (uint)i<s.length(); i++) {
                if ( s[i] < ' ' && s[i] != '\n' && s[i] != '\t' )
                    s[i] = ' ';
            }
            s.prepend(quotePrefix);
            s.replace("\n","\n"+quotePrefix);
            mEditor->insert(s);
        }
    }
}


void KMComposeWin::slotAddQuotes()
{
    if( mEditor->hasFocus() && msg() )
    {
        if ( mEditor->hasMarkedText()) {
            QString s =  mEditor->markedText();
            QString quotePrefix = quotePrefixName();
            s.prepend(quotePrefix);
            s.replace("\n", "\n"+quotePrefix);
            mEditor->insert(s);
        } else {
            int l =  mEditor->currentLine();
            int c =  mEditor->currentColumn();
            QString s =  mEditor->textLine(l);
            s.prepend("> ");
            mEditor->insertLine(s,l);
            mEditor->removeLine(l+1);
            mEditor->setCursorPosition(l,c+2);
        }
    }
}


void KMComposeWin::slotRemoveQuotes()
{
    if( mEditor->hasFocus() && msg() )
    {
        QString quotePrefix = quotePrefixName();
        if (mEditor->hasMarkedText()) {
            QString s = mEditor->markedText();
            QString quotePrefix = quotePrefixName();
            if (s.left(2) == quotePrefix )
                s.remove(0,2);
            s.replace("\n"+quotePrefix,"\n");
            mEditor->insert(s);
        } else {
            int l = mEditor->currentLine();
            int c = mEditor->currentColumn();
            QString s = mEditor->textLine(l);
            if (s.left(2) == quotePrefix) {
                s.remove(0,2);
                mEditor->insertLine(s,l);
                mEditor->removeLine(l+1);
                mEditor->setCursorPosition(l,c-2);
            }
        }
    }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotUndo()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (fw->inherits("KEdit"))
    ((QMultiLineEdit*)fw)->undo();
  else if (fw->inherits("QLineEdit"))
    ((QLineEdit*)fw)->undo();
}

void KMComposeWin::slotRedo()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (fw->inherits("KEdit"))
    ((QMultiLineEdit*)fw)->redo();
  else if (fw->inherits("QLineEdit"))
    ((QLineEdit*)fw)->redo();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCut()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (fw->inherits("KEdit"))
    ((QMultiLineEdit*)fw)->cut();
  else if (fw->inherits("QLineEdit"))
    ((QLineEdit*)fw)->cut();
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
  win = new KMComposeWin(msg);
  win->show();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotNewMailReader()
{
  KMMainWin *kmmwin = new KMMainWin(0);
  kmmwin->show();
  //d->resize(d->size());
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdWinTitle(const QString& text)
{
  if (text.isEmpty())
       setCaption("("+i18n("unnamed")+")");
  else setCaption(text);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotEncryptToggled(bool on)
{
  setEncryption( on, true /* set by the user */ );
}


//-----------------------------------------------------------------------------
void KMComposeWin::setEncryption( bool encrypt, bool setByUser )
{
  if ( !mEncryptAction->isEnabled() )
    encrypt = false;

  // check if the user wants to encrypt messages to himself and if he defined
  // an encryption key for the current identity
  if ( encrypt && Kpgp::Module::getKpgp()->encryptToSelf()
               && !mLastIdentityHasOpenPgpKey ) {
    if ( setByUser ) {
      KMessageBox::sorry( this,
                          i18n("<qt><p>In order to be able to encrypt "
                               "this message you first have to "
                               "define the OpenPGP key, which should be "
                               "used to encrypt the message to "
                               "yourself.</p>"
                               "<p>You can define the OpenPGP key, "
                               "which should be used with the current "
                               "identity, in the identity configuration.</p>"
                               "</qt>"),
                          i18n("Undefined Encryption Key") );
    }
    encrypt = false;
  }

  // make sure the mEncryptAction is in the right state
  mEncryptAction->setChecked( encrypt );

  // show the appropriate icon
  if ( encrypt )
    mEncryptAction->setIcon("encrypted");
  else
    mEncryptAction->setIcon("decrypted");

  // mark the attachments for (no) encryption
  if ( mSelectedCryptPlug ) {
    for ( KMAtmListViewItem* entry =
            static_cast<KMAtmListViewItem*>( mAtmItemList.first() );
          entry;
          entry = static_cast<KMAtmListViewItem*>( mAtmItemList.next() ) )
      entry->setEncrypt( encrypt );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSignToggled(bool on)
{
  setSigning( on, true /* set by the user */ );
}


//-----------------------------------------------------------------------------
void KMComposeWin::setSigning( bool sign, bool setByUser )
{
  if ( !mSignAction->isEnabled() )
    sign = false;

  // check if the user defined a signing key for the current identity
  if ( sign && !mLastIdentityHasOpenPgpKey ) {
    if ( setByUser ) {
      KMessageBox::sorry( this,
                          i18n("<qt><p>In order to be able to sign "
                               "this message you first have to "
                               "define the OpenPGP key which should be "
                               "used for this.</p>"
                               "<p>You can define the OpenPGP key "
                               "which should be used with the current "
                               "identity in the identity configuration.</p>"
                               "</qt>"),
                          i18n("Undefined Signing Key") );
    }
    sign = false;
  }

  // make sure the mSignAction is in the right state
  mSignAction->setChecked( sign );

  // mark the attachments for (no) signing
  if ( mSelectedCryptPlug ) {
    for ( KMAtmListViewItem* entry =
            static_cast<KMAtmListViewItem*>( mAtmItemList.first() );
          entry;
          entry = static_cast<KMAtmListViewItem*>( mAtmItemList.next() ) )
      entry->setSign( sign );
  }
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
  bool bMessageWasModified = ( mEditor->isModified() || mEdtFrom->edited() ||
                               mEdtReplyTo->edited() || mEdtTo->edited() ||
                               mEdtCc->edited() || mEdtBcc->edited() ||
                               mEdtSubject->edited() || mAtmModified ||
                               ( mTransport->lineEdit() &&
                                 mTransport->lineEdit()->edited() ) );
  applyChanges( true );
  KMCommand *command = new KMPrintCommand( this, mMsg );
  command->start();
  mEditor->setModified( bMessageWasModified );
}


//----------------------------------------------------------------------------
bool KMComposeWin::doSend(int aSendNow, bool saveInDrafts)
{
  if (!saveInDrafts)
  {
     if (to().isEmpty())
     {
        mEdtTo->setFocus();
        KMessageBox::information( this,
                                  i18n("You must specify at least one "
                                       "receiver in the To: field.") );
        return false;
     }

     if (subject().isEmpty())
     {
        mEdtSubject->setFocus();
        int rc =
          KMessageBox::questionYesNo( this,
                                      i18n("You did not specify a subject. "
                                           "Send message anyway?"),
                                      i18n("No Subject Specified"),
                                      i18n("&Yes, Send as Is"),
                                      i18n("&No, Let Me Specify the Subject"),
                                      "no_subject_specified" );
        if( rc == KMessageBox::No )
        {
           return false;
        }
     }

     if ( userForgotAttachment() )
       return false;
  }

  KCursorSaver busy(KBusyPtr::busy());
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

  mDisableBreaking = saveInDrafts;

  mBccMsgList.clear();
  bool sentOk = applyChanges();
  if( sentOk ) {
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
  }

  mDisableBreaking = false;

  if (!sentOk)
      return false;

  // needed for imap
  mMsg->setComplete( true );

  if (saveInDrafts)
  {
    KMFolder* draftsFolder = 0, *imapDraftsFolder = 0;
    // get the draftsFolder
    if ( !mMsg->drafts().isEmpty() )
    {
      draftsFolder = kmkernel->folderMgr()->findIdString( mMsg->drafts() );
      if ( draftsFolder == 0 )
        // This is *NOT* supposed to be "imapDraftsFolder", because a
        // dIMAP folder works like a normal folder
        draftsFolder = kmkernel->imapFolderMgr()->findIdString( mMsg->drafts() );
      if ( draftsFolder == 0 )
        imapDraftsFolder = kmkernel->imapFolderMgr()->findIdString( mMsg->drafts() );
    }
    if (imapDraftsFolder && imapDraftsFolder->noContent())
      imapDraftsFolder = 0;

    if ( draftsFolder == 0 ) {
      draftsFolder = kmkernel->draftsFolder();
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
      (static_cast<KMFolderImap*>(imapDraftsFolder->storage()))->getFolder();
    }

  } else {
    mMsg->setTo( KMMessage::expandAliases( to() ));
    mMsg->setCc( KMMessage::expandAliases( cc() ));
    if( !mBcc.isEmpty() )
      mMsg->setBcc( KMMessage::expandAliases( mBcc ));
    QString recips = mMsg->headerField( "X-KMail-Recipients" );
    if( !recips.isEmpty() ) {
      mMsg->setHeaderField( "X-KMail-Recipients", KMMessage::expandAliases( recips ) );
    }
    mMsg->cleanupHeader();
    sentOk = kmkernel->msgSender()->send(mMsg, aSendNow);
    KMMessage* msg;
    for( msg = mBccMsgList.first(); msg; msg = mBccMsgList.next() ) {
      msg->setTo( KMMessage::expandAliases( to() ));
      msg->setCc( KMMessage::expandAliases( cc() ));
      msg->setBcc( KMMessage::expandAliases( bcc() ));
      QString recips = msg->headerField( "X-KMail-Recipients" );
      if( !recips.isEmpty() ) {
        msg->setHeaderField( "X-KMail-Recipients", KMMessage::expandAliases( recips ) );
      }
      msg->cleanupHeader();
      sentOk &= kmkernel->msgSender()->send(msg, aSendNow);
    }
  }

  if (!sentOk)
     return false;

  if (saveInDrafts || !aSendNow)
      emit messageQueuedOrDrafted();

  RecentAddresses::self()->add( bcc() );
  RecentAddresses::self()->add( cc() );
  RecentAddresses::self()->add( to() );

  mAutoDeleteMsg = FALSE;
  mFolder = 0;
  close();
  return true;
}



//----------------------------------------------------------------------------
void KMComposeWin::slotSendLater()
{
  if ( mEditor->checkExternalEditorFinished() )
    doSend( false );
}


//----------------------------------------------------------------------------
bool KMComposeWin::slotSaveDraft() {
  return mEditor->checkExternalEditorFinished() && doSend( false, true );
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSendNow() {
  if ( !mEditor->checkExternalEditorFinished() )
    return;
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


//----------------------------------------------------------------------------
void KMComposeWin::slotAppendSignature()
{
  bool mod = mEditor->isModified();

  const KMIdentity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  mOldSigText = ident.signatureText();
  if( !mOldSigText.isEmpty() )
  {
    mEditor->sync();
    mEditor->append(mOldSigText);
    mEditor->update();
    mEditor->setModified(mod);
    mEditor->setContentsPos( 0, 0 );
  }
  kmkernel->dumpDeadLetters();
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
    kmkernel->identityManager()->identityForUoid( uoid );
  if ( ident.isNull() ) return;

  if(!ident.fullEmailAddr().isNull())
    mEdtFrom->setText(ident.fullEmailAddr());
  mEdtReplyTo->setText(ident.replyToAddr());
  // don't overwrite the BCC field when the user has edited it and the
  // BCC field of the new identity is empty
  if( !mEdtBcc->edited() || !ident.bcc().isEmpty() )
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
    QString transp = ident.transport();
    if (transp.isEmpty())
    {
      mMsg->removeHeaderField("X-KMail-Transport");
      transp = mTransport->text(0);
    }
    else
      mMsg->setHeaderField("X-KMail-Transport", transp);
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

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  if ( !mBtnFcc->isChecked() )
  {
    if ( ident.fcc().isEmpty() )
      mFcc->setFolder( kmkernel->sentFolder() );
    else
      setFcc( ident.fcc() );
  }

  QString edtText = mEditor->text();
  bool appendNewSig = true;
  // try to truncate the old sig
  if( !mOldSigText.isEmpty() )
  {
    if( edtText.endsWith( mOldSigText ) )
      edtText.truncate( edtText.length() - mOldSigText.length() );
    else
      appendNewSig = false;
  }
  // now append the new sig
  mOldSigText = ident.signatureText();
  if( appendNewSig )
  {
    if( !mOldSigText.isEmpty() && mAutoSign )
      edtText.append( mOldSigText );
    mEditor->setText( edtText );
  }

  // disable certain actions if there is no PGP user identity set
  // for this profile
  bool bNewIdentityHasOpenPgpKey = !ident.pgpIdentity().isEmpty();
  if( !mSelectedCryptPlug && !bNewIdentityHasOpenPgpKey )
  {
    mAttachMPK->setEnabled(false);
    if( mLastIdentityHasOpenPgpKey )
    { // save the state of the sign and encrypt button
      mLastEncryptActionState = mEncryptAction->isChecked();
      setEncryption( false );
      mLastSignActionState = mSignAction->isChecked();
      setSigning( false );
    }
  }
  else
  {
    mAttachMPK->setEnabled(true);
    if( !mLastIdentityHasOpenPgpKey )
    { // restore the last state of the sign and encrypt button
      setEncryption( mLastEncryptActionState );
      setSigning( mLastSignActionState );
    }
  }
  mLastIdentityHasOpenPgpKey = bNewIdentityHasOpenPgpKey;

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
  qtd.setCancelButton(KStdGuiItem::cancel().text());
  qtd.setOkButton(KStdGuiItem::ok().text());

  if (qtd.exec())
    mKSpellConfig.writeGlobalSettings();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotStatusMessage(const QString &message)
{
    statusBar()->changeItem( message, 0 );
}

void KMComposeWin::slotEditToolbars()
{
  saveMainWindowSettings(KMKernel::config(), "Composer");
  KEditToolbar dlg(actionCollection(), "kmcomposerui.rc");

  connect( &dlg, SIGNAL(newToolbarConfig()),
           SLOT(slotUpdateToolbars()) );

  dlg.exec();
}

void KMComposeWin::slotUpdateToolbars()
{
  createGUI("kmcomposerui.rc");
  applyMainWindowSettings(KMKernel::config(), "Composer");
}

void KMComposeWin::slotEditKeys()
{
  KKeyDialog::configure( actionCollection(),
                         false /*don't allow one-letter shortcuts*/
                         );
}

void KMComposeWin::setReplyFocus( bool hasMessage )
{
  mEditor->setFocus();
  if ( hasMessage )
    mEditor->setCursorPosition( 1, 0 );
}

void KMComposeWin::setFocusToSubject()
{
  mEdtSubject->setFocus();
}

void KMComposeWin::slotCompletionModeChanged( KGlobalSettings::Completion mode)
{
    KConfig *config = KMKernel::config();
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

void KMComposeWin::slotConfigChanged()
{
    readConfig();
}

/*
* checks if the drafts-folder has been deleted
* that is not nice so we set the system-drafts-folder
*/
void KMComposeWin::slotFolderRemoved(KMFolder* folder)
{
        if ( (mFolder) && (folder->idString() == mFolder->idString()) )
        {
                mFolder = kmkernel->draftsFolder();
                kdDebug(5006) << "restoring drafts to " << mFolder->idString() << endl;
        }
        if (mMsg) mMsg->setParent(0);
}



void KMComposeWin::slotSetAlwaysSend( bool bAlways )
{
    mAlwaysSend = bAlways;
}

void KMEdit::contentsDragEnterEvent(QDragEnterEvent *e)
{
    if (e->provides(MailListDrag::format()))
        e->accept(true);
    else
        return KEdit::dragEnterEvent(e);
}

void KMEdit::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if (e->provides(MailListDrag::format()))
        e->accept();
    else
        return KEdit::dragMoveEvent(e);
}

void KMEdit::keyPressEvent( QKeyEvent* e )
{
    if( e->key() == Key_Return ) {
        int line, col;
        getCursorPosition( &line, &col );
        QString lineText = text( line );
        // returns line with additional trailing space (bug in Qt?), cut it off
        lineText.truncate( lineText.length() - 1 );
        // special treatment of quoted lines only if the cursor is neither at
        // the begin nor at the end of the line
        if( ( col > 0 ) && ( col < int( lineText.length() ) ) ) {
            bool isQuotedLine = false;
            uint bot = 0; // bot = begin of text after quote indicators
            while( bot < lineText.length() ) {
                if( ( lineText[bot] == '>' ) || ( lineText[bot] == '|' ) ) {
                    isQuotedLine = true;
                    ++bot;
                }
                else if( lineText[bot].isSpace() ) {
                    ++bot;
                }
                else {
                    break;
                }
            }

            KEdit::keyPressEvent( e );

            // duplicate quote indicators of the previous line before the new
            // line if the line actually contained text (apart from the quote
            // indicators) and the cursor is behind the quote indicators
            if( isQuotedLine
                && ( bot != lineText.length() )
                && ( col >= int( bot ) ) ) {
                QString newLine = text( line + 1 );
                // remove leading white space from the new line and instead
                // add the quote indicators of the previous line
                unsigned int leadingWhiteSpaceCount = 0;
                while( ( leadingWhiteSpaceCount < newLine.length() )
                       && newLine[leadingWhiteSpaceCount].isSpace() ) {
                    ++leadingWhiteSpaceCount;
                }
                newLine = newLine.replace( 0, leadingWhiteSpaceCount,
                                           lineText.left( bot ) );
                removeParagraph( line + 1 );
                insertParagraph( newLine, line + 1 );
                // place the cursor at the begin of the new line since
                // we assume that the user split the quoted line in order
                // to add a comment to the first part of the quoted line
                setCursorPosition( line + 1 , 0 );
            }
        }
        else
            KEdit::keyPressEvent( e );
    }
    else
        KEdit::keyPressEvent( e );
}

void KMEdit::contentsDropEvent(QDropEvent *e)
{
    if (e->provides(MailListDrag::format())) {
        // Decode the list of serial numbers stored as the drag data
        QByteArray serNums;
        MailListDrag::decode( e, serNums );
        QBuffer serNumBuffer(serNums);
        serNumBuffer.open(IO_ReadOnly);
        QDataStream serNumStream(&serNumBuffer);
        unsigned long serNum;
        KMFolder *folder = 0;
        int idx;
        QPtrList<KMMsgBase> messageList;
        while (!serNumStream.atEnd()) {
            KMMsgBase *msgBase = 0;
            serNumStream >> serNum;
            kmkernel->msgDict()->getLocation(serNum, &folder, &idx);
            if (folder)
                msgBase = folder->getMsgBase(idx);
            if (msgBase)
                messageList.append( msgBase );
        }
        serNumBuffer.close();
        uint identity = folder ? folder->identity() : 0;
        KMCommand *command =
            new KMForwardAttachedCommand(mComposer, messageList,
                                         identity, mComposer);
        command->start();
    }
    else if( KURLDrag::canDecode( e ) ) {
        KURL::List urlList;
        if( KURLDrag::decode( e, urlList ) ) {
            for( KURL::List::Iterator it = urlList.begin();
                 it != urlList.end(); ++it ) {
                mComposer->addAttach( *it );
            }
        }
    }
    else {
        return KEdit::dropEvent(e);
    }
}

//=============================================================================
//
//   Class  KMAtmListViewItem
//
//=============================================================================

KMAtmListViewItem::KMAtmListViewItem(QListView *parent) :
  QObject(), QListViewItem( parent )
{

  mCBSignEnabled = false;
  mCBEncryptEnabled = false;

  mListview = parent;
  mCBEncrypt = new QCheckBox(mListview->viewport());
  mCBSign = new QCheckBox(mListview->viewport());

  mCBEncrypt->hide();
  mCBSign->hide();
}

KMAtmListViewItem::~KMAtmListViewItem()
{
}

void KMAtmListViewItem::paintCell( QPainter * p, const QColorGroup & cg,
                                  int column, int width, int align )
{
  // this is also called for the encrypt/sign columns to assure that the
  // background is cleared
  QListViewItem::paintCell( p, cg, column, width, align );
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

    QColor bg;
    if (isSelected())
      bg = cg.highlight();
    else
      bg = cg.base();

    bool enabled = (4 == column) ? mCBEncryptEnabled : mCBSignEnabled;
    cb->setPaletteBackgroundColor(bg);
    if (enabled) cb->show();
  }
}

void KMAtmListViewItem::enableCryptoCBs(bool on)
{
  if( mCBEncrypt ) {
    mCBEncryptEnabled = on;
    mCBEncrypt->setEnabled( on );
  }
  if( mCBSign ) {
    mCBSignEnabled = on;
    mCBSign->setEnabled( on );
  }
}

void KMAtmListViewItem::setEncrypt(bool on)
{
  if( mCBEncrypt )
    mCBEncrypt->setChecked( on );
}

bool KMAtmListViewItem::isEncrypt()
{
  if( mCBEncrypt )
    return mCBEncrypt->isChecked();
  else
    return false;
}

void KMAtmListViewItem::setSign(bool on)
{
  if( mCBSign )
    mCBSign->setChecked( on );
}

bool KMAtmListViewItem::isSign()
{
  if( mCBSign )
    return mCBSign->isChecked();
  else
    return false;
}



//=============================================================================
//
//   Class  KMLineEdit
//
//=============================================================================

KMLineEdit::KMLineEdit(KMComposeWin* composer, bool useCompletion,
                       QWidget *parent, const char *name)
    : AddressLineEdit(parent,useCompletion,name), mComposer(composer)
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
  AddressLineEdit::keyPressEvent(e);
}

#if 0
//-----------------------------------------------------------------------------
void KMLineEdit::dropEvent(QDropEvent *e)
{
  KURL::List uriList;
  if(KURLDrag::decode( e, uriList ))
  {
    for (KURL::List::ConstIterator it = uriList.begin(); it != uriList.end(); ++it)
    {
      smartInsert( (*it).url() );
    }
  }
  else {
    if (m_useCompletion)
       m_smartPaste = true;
    QLineEdit::dropEvent(e);
    m_smartPaste = false;
  }
}

void KMLineEdit::smartInsert( const QString &str, int pos /* = -1 */ )
{
    QString newText = str.stripWhiteSpace();
    if (newText.isEmpty())
        return;

    // remove newlines in the to-be-pasted string:
    newText.replace( QRegExp("\r?\n"), " " );

    QString contents = text();
    // determine the position where to insert the to-be-pasted string
    if( ( pos < 0 ) || ( pos > (int) contents.length() ) )
        pos = contents.length();
    int start_sel = 0;
    int end_sel = 0;
    if (getSelection(&start_sel, &end_sel))
    {
        // Cut away the selection.
        if (pos > end_sel)
            pos -= (end_sel - start_sel);
        else if (pos > start_sel)
            pos = start_sel;
        contents = contents.left(start_sel) + contents.right(end_sel+1);
    }

    int eot = contents.length();
    // remove trailing whitespace from the contents of the line edit
    while ((eot > 0) && contents[eot-1].isSpace()) eot--;
    if (eot == 0)
     {
         contents = QString::null;
     }
    else if (pos >= eot)
    {
        if (contents[eot-1] == ',')
            eot--;
        contents.truncate(eot);
        contents += ", ";
        pos = eot+2;
    }

    if (newText.startsWith("mailto:"))
    {
        kdDebug(5006) << "Pasting '" << newText << "'" << endl;
        KURL u(newText);
        newText = u.path();
        kdDebug(5006) << "path of mailto URL: '" << newText << "'" << endl;
        // Is the mailto URL RFC 2047 encoded (cf. RFC 2368)?
        if (-1 != newText.find( QRegExp("=\\?.*\\?[bq]\\?.*\\?=") ) )
            newText = KMMsgBase::decodeRFC2047String( newText.latin1() );
    }
    else if (-1 != newText.find(" at "))
    {
        // Anti-spam stuff
        newText.replace( " at ", "@" );
        newText.replace( " dot ", "." );
    }
    else if (newText.contains("(at)"))
    {
        newText.replace( QRegExp("\\s*\\(at\\)\\s*"), "@" );
    }
    contents = contents.left(pos)+newText+contents.mid(pos);
    setText(contents);
    setEdited( true );
    setCursorPosition(pos+newText.length());
}
#endif

//-----------------------------------------------------------------------------
void KMLineEdit::loadAddresses()
{
    AddressLineEdit::loadAddresses();

    QStringList recent = RecentAddresses::self()->addresses();
    QStringList::Iterator it = recent.begin();
    for ( ; it != recent.end(); ++it )
        addAddress( *it );
}


KMLineEditSpell::KMLineEditSpell(KMComposeWin* composer, bool useCompletion,
                       QWidget *parent, const char *name)
    : KMLineEdit(composer,useCompletion,parent,name)
{
}


void KMLineEditSpell::highLightWord( unsigned int length, unsigned int pos )
{
    setSelection ( pos, length );
}

void KMLineEditSpell::spellCheckDone( const QString &s )
{
    if( s != text() )
        setText( s );
}

void KMLineEditSpell::spellCheckerMisspelling( const QString &_text, const QStringList&, unsigned int pos)
{
     highLightWord( _text.length(),pos );
}

void KMLineEditSpell::spellCheckerCorrected( const QString &old, const QString &corr, unsigned int pos)
{
    if( old!= corr )
    {
        setSelection ( pos, old.length() );
        insert( corr );
        setSelection ( pos, corr.length() );
    }
}


//=============================================================================
//
//   Class  KMEdit
//
//=============================================================================
KMEdit::KMEdit(QWidget *parent, KMComposeWin* composer,
               KSpellConfig* autoSpellConfig,
               const char *name)
  : KEdit( parent, name ),
    mComposer( composer ),
    mKSpell( 0 ),
    mSpellingFilter( 0 ),
    mExtEditorTempFile( 0 ),
    mExtEditorTempFileWatcher( 0 ),
    mExtEditorProcess( 0 ),
    mUseExtEditor( false ),
    mWasModifiedBeforeSpellCheck( false ),
    mSpellChecker( 0 ),
    mSpellLineEdit( false )
{
  installEventFilter(this);
  KCursor::setAutoHideCursor( this, true, true );

  initializeAutoSpellChecking( autoSpellConfig );
}

//-----------------------------------------------------------------------------
void KMEdit::initializeAutoSpellChecking( KSpellConfig* autoSpellConfig )
{
  KConfigGroup readerConfig( KMKernel::config(), "Reader" );
  QColor defaultColor1( 0x00, 0x80, 0x00 ); // defaults from kmreaderwin.cpp
  QColor defaultColor2( 0x00, 0x70, 0x00 );
  QColor defaultColor3( 0x00, 0x60, 0x00 );
  QColor defaultForeground( kapp->palette().active().text() );
  QColor col1 = readerConfig.readColorEntry( "ForegroundColor", &defaultForeground );
  QColor col2 = readerConfig.readColorEntry( "QuotedText3", &defaultColor3 );
  QColor col3 = readerConfig.readColorEntry( "QuotedText2", &defaultColor2 );
  QColor col4 = readerConfig.readColorEntry( "QuotedText1", &defaultColor1 );
  QColor c = Qt::red;
  QColor misspelled = readerConfig.readColorEntry( "MisspelledColor", &c );

  mSpellChecker = new KDictSpellingHighlighter( this, /*active*/ true,
                                                /*autoEnabled*/ false,
                                                /*spellColor*/ misspelled,
                                                /*colorQuoting*/ true,
                                                col1, col2, col3, col4,
                                                autoSpellConfig );
  connect( mSpellChecker, SIGNAL(activeChanged(const QString &)),
           mComposer, SLOT(slotStatusMessage(const QString &)));
  connect( mSpellChecker, SIGNAL(newSuggestions(const QString&, const QStringList&, unsigned int)),
           this, SLOT(addSuggestion(const QString&, const QStringList&, unsigned int)) );
}

//-----------------------------------------------------------------------------
void KMEdit::addSuggestion(const QString& text, const QStringList& lst, unsigned int )
{
  mReplacements[text] = lst;
}

//-----------------------------------------------------------------------------
KMEdit::~KMEdit()
{
  removeEventFilter(this);

  delete mKSpell;
  delete mSpellChecker;
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

    if (mUseExtEditor) {
      if (k->key() == Key_Up)
      {
        mComposer->focusNextPrevEdit(0, false); //take me up
        return TRUE;
      }

      // ignore modifier keys (cf. bug 48841)
      if ( (k->key() == Key_Shift) || (k->key() == Key_Control) ||
           (k->key() == Key_Meta) || (k->key() == Key_Alt) )
        return true;
      if (mExtEditorTempFile) return TRUE;
      QString sysLine = mExtEditor;
      mExtEditorTempFile = new KTempFile();

      mExtEditorTempFile->setAutoDelete(true);

      (*mExtEditorTempFile->textStream()) << text();

      mExtEditorTempFile->close();
      // replace %f in the system line
      sysLine.replace( "%f", mExtEditorTempFile->name() );
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
        KMessageBox::error( topLevelWidget(),
                            i18n("Unable to start external editor.") );
        killExternalEditor();
      } else {
        mExtEditorTempFileWatcher = new KDirWatch( this, "mExtEditorTempFileWatcher" );
        connect( mExtEditorTempFileWatcher, SIGNAL(dirty(const QString&)),
                 SLOT(slotExternalEditorTempFileChanged(const QString&)) );
        mExtEditorTempFileWatcher->addFile( mExtEditorTempFile->name() );
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
  } else if ( e->type() == QEvent::ContextMenu ) {
    QContextMenuEvent *event = (QContextMenuEvent*) e;

    int para = 1, charPos, firstSpace, lastSpace;

    //Get the character at the position of the click
    charPos = charAt( viewportToContents(event->pos()), &para );
    QString paraText = text( para );

    if( !paraText.at(charPos).isSpace() )
    {
      //Get word right clicked on
      const QRegExp wordBoundary( "[\\s\\W]" );
      firstSpace = paraText.findRev( wordBoundary, charPos ) + 1;
      lastSpace = paraText.find( wordBoundary, charPos );
      if( lastSpace == -1 )
        lastSpace = paraText.length();
      QString word = paraText.mid( firstSpace, lastSpace - firstSpace );
      //Continue if this word was misspelled
      if( !word.isEmpty() && mReplacements.contains( word ) )
      {
        KPopupMenu p;
        p.insertTitle( i18n("Suggestions") );

        //Add the suggestions to the popup menu
        QStringList reps = mReplacements[word];
        if( reps.count() > 0 )
        {
          int listPos = 0;
          for ( QStringList::Iterator it = reps.begin(); it != reps.end(); ++it ) {
            p.insertItem( *it, listPos );
            listPos++;
          }
        }
        else
        {
          p.insertItem( QString::fromLatin1("No Suggestions"), -2 );
        }

        //Execute the popup inline
        int id = p.exec( mapToGlobal( event->pos() ) );

        if( id > -1 )
        {
          //Save the cursor position
          int parIdx = 1, txtIdx = 1;
          getCursorPosition(&parIdx, &txtIdx);
          setSelection(para, firstSpace, para, lastSpace);
          insert(mReplacements[word][id]);
          // Restore the cursor position; if the cursor was behind the
          // misspelled word then adjust the cursor position
          if ( para == parIdx && txtIdx >= lastSpace )
            txtIdx += mReplacements[word][id].length() - word.length();
          setCursorPosition(parIdx, txtIdx);
        }
        //Cancel original event
        return true;
      }
    }
  }

  return KEdit::eventFilter(o, e);
}


//-----------------------------------------------------------------------------
void KMEdit::slotAutoSpellCheckingToggled( bool on )
{
  // don't autoEnable spell checking if the user turned spell checking off
  mSpellChecker->setAutomatic( on );
  mSpellChecker->setActive( on );
}


//-----------------------------------------------------------------------------
void KMEdit::slotExternalEditorTempFileChanged( const QString & fileName ) {
  if ( !mExtEditorTempFile )
    return;
  if ( fileName != mExtEditorTempFile->name() )
    return;
  // read data back in from file
  setAutoUpdate(false);
  clear();

  insertLine(QString::fromLocal8Bit(kFileToString( fileName, true, false )), -1);
  setAutoUpdate(true);
  repaint();
}

void KMEdit::slotExternalEditorDone( KProcess * proc ) {
  assert(proc == mExtEditorProcess);
  // make sure, we update even when KDirWatcher is too slow:
  slotExternalEditorTempFileChanged( mExtEditorTempFile->name() );
  killExternalEditor();
}

void KMEdit::killExternalEditor() {
  delete mExtEditorTempFileWatcher; mExtEditorTempFileWatcher = 0;
  delete mExtEditorTempFile; mExtEditorTempFile = 0;
  delete mExtEditorProcess; mExtEditorProcess = 0;
}


bool KMEdit::checkExternalEditorFinished() {
  if ( !mExtEditorProcess )
    return true;
  switch ( KMessageBox::warningYesNoCancel( topLevelWidget(),
           i18n("The external editor is still running.\n"
                "Abort the external editor or leave it open?"),
           i18n("External Editor"),
           i18n("Abort Editor"), i18n("Leave Editor Open") ) ) {
  case KMessageBox::Yes:
    killExternalEditor();
    return true;
  case KMessageBox::No:
    return true;
  default:
    return false;
  }
}

//-----------------------------------------------------------------------------
void KMEdit::spellcheck()
{
  if ( mKSpell )
    return;
  mWasModifiedBeforeSpellCheck = isModified();
  mSpellLineEdit = !mSpellLineEdit;
  mKSpell = new KSpell(this, i18n("Spellcheck - KMail"), this,
                       SLOT(slotSpellcheck2(KSpell*)));
  QStringList l = KSpellingHighlighter::personalWords();
  for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
      mKSpell->addPersonal( *it );
  }
  connect (mKSpell, SIGNAL( death()),
          this, SLOT (slotSpellDone()));
  connect (mKSpell, SIGNAL (misspelling (const QString &, const QStringList &, unsigned int)),
          this, SLOT (slotMisspelling (const QString &, const QStringList &, unsigned int)));
  connect (mKSpell, SIGNAL (corrected (const QString &, const QString &, unsigned int)),
          this, SLOT (slotCorrected (const QString &, const QString &, unsigned int)));
  connect (mKSpell, SIGNAL (done(const QString &)),
          this, SLOT (slotSpellResult (const QString&)));
}

#if KDE_IS_VERSION( 3, 1, 92 )
void KMEdit::cut()
{
    KEdit::cut();
    mSpellChecker->restartBackgroundSpellCheck();
}

void KMEdit::clear()
{
    KEdit::clear();
    mSpellChecker->restartBackgroundSpellCheck();
}

void KMEdit::del()
{
    KEdit::del();
    mSpellChecker->restartBackgroundSpellCheck();
}
#else
// can't #ifdef slots :-(
void KMEdit::cut() { KEdit::cut(); }
void KMEdit::clear() { KEdit::clear(); }
void KMEdit::del() { KEdit::del(); }
#endif


void KMEdit::slotMisspelling(const QString &text, const QStringList &lst, unsigned int pos)
{
    kdDebug()<<"void KMEdit::slotMisspelling(const QString &text, const QStringList &lst, unsigned int pos) : "<<text <<endl;
    if( mSpellLineEdit )
        mComposer->sujectLineWidget()->spellCheckerMisspelling( text, lst, pos);
    else
        misspelling(text, lst, pos);
}

void KMEdit::slotCorrected (const QString &oldWord, const QString &newWord, unsigned int pos)
{
    kdDebug()<<"slotCorrected (const QString &oldWord, const QString &newWord, unsigned int pos) : "<<oldWord<<endl;
    if( mSpellLineEdit )
        mComposer->sujectLineWidget()->spellCheckerCorrected( oldWord, newWord, pos);
     else
         corrected(oldWord, newWord, pos);
}

//-----------------------------------------------------------------------------
void KMEdit::slotSpellcheck2(KSpell*)
{
    if( !mSpellLineEdit)
    {
        spellcheck_start();

        QString quotePrefix;
        if(mComposer && mComposer->msg())
        {
            // read the quote indicator from the preferences
            KConfig *config=KMKernel::config();
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
    else if( mComposer )
        mKSpell->check( mComposer->sujectLineWidget()->text());
}

//-----------------------------------------------------------------------------
void KMEdit::slotSpellResult(const QString &s)
{
    if( !mSpellLineEdit)
        spellcheck_stop();

  int dlgResult = mKSpell->dlgResult();
  if ( dlgResult == KS_CANCEL )
  {
      if( mSpellLineEdit)
      {
          //stop spell check
          mSpellLineEdit = false;
          QString tmpText( s );
          tmpText =  tmpText.remove('\n');

          if( tmpText != mComposer->sujectLineWidget()->text() )
              mComposer->sujectLineWidget()->setText( tmpText );
      }
      else
      {
          kdDebug(5006) << "spelling: canceled - restoring text from SpellingFilter" << endl;
          setText(mSpellingFilter->originalText());
          setModified(mWasModifiedBeforeSpellCheck);
      }
  }
  mKSpell->cleanUp();
  KDictSpellingHighlighter::dictionaryChanged();

  emit spellcheck_done( dlgResult );
}

//-----------------------------------------------------------------------------
void KMEdit::slotSpellDone()
{
    kdDebug()<<" void KMEdit::slotSpellDone()**********************************************\n";
  KSpell::spellStatus status = mKSpell->status();
  delete mKSpell;
  mKSpell = 0;

  kdDebug() << "spelling: delete SpellingFilter" << endl;
  delete mSpellingFilter;
  mSpellingFilter = 0;
  mComposer->sujectLineWidget()->deselect();
  if (status == KSpell::Error)
  {
     KMessageBox::sorry( topLevelWidget(),
                         i18n("ISpell/Aspell could not be started. Please "
                              "make sure you have ISpell or Aspell properly "
                              "configured and in your PATH.") );
     emit spellcheck_done( KS_CANCEL );
  }
  else if (status == KSpell::Crashed)
  {
     spellcheck_stop();
     KMessageBox::sorry( topLevelWidget(),
                         i18n("ISpell/Aspell seems to have crashed.") );
     emit spellcheck_done( KS_CANCEL );
  }
  else
  {
      if( mSpellLineEdit )
          spellcheck();
#if KDE_IS_VERSION( 3, 1, 90 )
      else if( status == KSpell::FinishedNoMisspellingsEncountered )
          KMessageBox::information( topLevelWidget(),
                                    i18n("No misspellings encountered.") );
#endif
  }
}
