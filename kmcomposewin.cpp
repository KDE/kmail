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
#include <cryptplugfactory.h>

#include "klistboxdialog.h"

#include "messagecomposer.h"

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
//#include <keditlistbox.h>

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
  : MailComposerIface(), KMail::SecondaryWindow( "kmail-composer#" ),
    mMsg( 0 ),
    mAutoRequestMDN( false ),
    mId( id ), mComposer( 0 ), mNeverSign( false ), mNeverEncrypt( false )
{
  if (kmkernel->xmlGuiInstance())
    setInstance( kmkernel->xmlGuiInstance() );
  mMainWidget = new QWidget(this);

  // Initialize the plugin selection according to 'active' flag that
  // was set via the global configuration dialog.
  mSelectedCryptPlug = KMail::CryptPlugFactory::instance()->active();

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
  return KMail::SecondaryWindow::event(e);
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
  mUseFixedFont = config->readBoolEntry("use-fixed-font", false);
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
  mNeverSignWhenSavingInDrafts = config->readBoolEntry("never-sign-drafts", true);
  mNeverEncryptWhenSavingInDrafts = config->readBoolEntry("never-encrypt-drafts", true);
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
    config->writeEntry("use-fixed-font", mUseFixedFont );
  }

  {
    KConfigGroupSaver saver(config, "Geometry");
    config->writeEntry("composer", size());

    saveMainWindowSettings(config, "Composer");
    config->sync();
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::deadLetter()
{
  if (!mMsg || mComposer) return;

  connect( this, SIGNAL( applyChangesDone( bool ) ),
           this, SLOT( slotContinueDeadLetter( bool ) ) );
  // This method is called when KMail crashed, so don't try signing/encryption
  applyChanges( true, true );

  // Don't continue before the applyChanges is done!
  qApp->enter_loop();

  // Ok, it's done now - continue dead letter saving
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

void KMComposeWin::slotContinueDeadLetter( bool )
{
  disconnect( this, SIGNAL( applyChangesDone( bool ) ),
              this, SLOT( slotContinueDeadLetter( bool ) ) );
  qApp->exit_loop();
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
  rethinkHeaderLine(showHeaders,HDR_CC, row, i18n("&Copy to (CC):"),
                    mLblCc, mEdtCc, mBtnCc,
                    i18n("<qt>The email addresses you put "
                         "in this field receives a copy of the email. "
                         "Technically it is the same thing as putting all the "
                         "addresses in the <b>To:</b> field but differs in "
                         "that it usually symbolises the receiver of the "
                         "Carbon Copy (CC) is a listener, not the main "
                         "recipient.</qt>"));
  if (!fromSlot) mBccAction->setChecked(abs(mShowHeaders)&HDR_BCC);
  rethinkHeaderLine(showHeaders,HDR_BCC, row, i18n("&Blind copy to (BCC):"),
                    mLblBcc, mEdtBcc, mBtnBcc,
                    i18n("<qt>Essentially the same thing "
                         "as the <b>Copy To:</b> field but differs in that "
                         "all other recipients do not see who receives a "
                         "blind copy.</qt>"));
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
                                     QLineEdit* aEdt, QPushButton* aBtn,
                                     const QString &toolTip )
{
  if (aValue & aMask)
  {
    aLbl->setText(aLabelStr);
    if ( !toolTip.isEmpty() )
      QToolTip::add( aLbl, toolTip );
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
  mFixedFontAction->setChecked(mUseFixedFont);

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

  if( KMail::CryptPlugFactory::instance()->list().count() ){
    QStringList lst;
    lst << i18n( "inline OpenPGP (built-in)" );
    CryptPlugWrapper* current;
    QPtrListIterator<CryptPlugWrapper> it( KMail::CryptPlugFactory::instance()->list() );
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
    else if (rc == KMessageBox::Yes) {
      // doSend will close the window. Just return false from this method
      slotSaveDraft();
      return false;
    }
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
void KMComposeWin::applyChanges( bool dontSign, bool dontEncrypt )
{
  if(!mMsg) {
    kdDebug(5006) << "KMComposeWin::applyChanges() : mMsg == 0!\n" << endl;
    emit applyChangesDone( false );
    return;
  }

  if( mComposer ) {
    kdDebug(5006) << "KMComposeWin::applyChanges() : applyChanges called twice"
                  << endl;
    return;
  }

  // Make new job and execute it
  mComposer = new MessageComposer( this );
  connect( mComposer, SIGNAL( done( bool ) ),
           this, SLOT( slotComposerDone( bool ) ) );

  // TODO: Add a cancel button for this operation
  // TODO: Not always do this?
  setEnabled( false );

  mComposer->setDisableBreaking( mDisableBreaking );
  mComposer->applyChanges( dontSign, dontEncrypt );
}

void KMComposeWin::slotComposerDone( bool rc )
{
  emit applyChangesDone( rc );
  delete mComposer;
  mComposer = 0;
}

QCString KMComposeWin::pgpIdentity() const
{
  // get PGP user id for the chosen identity
  const KMIdentity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  return ident.pgpIdentity();
}

bool KMComposeWin::queryExit ()
{
  return true;
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

  dlg.setRecentAddresses( RecentAddresses::self( KMKernel::config() )->kabcAddresses() );

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
  for ( CryptPlugWrapperListIterator it( KMail::CryptPlugFactory::instance()->list() ) ;
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
  if ( mFixedFontAction ) {
    mUseFixedFont = mFixedFontAction->isChecked();
  }
  mEditor->setFont( mUseFixedFont ? mFixedFont : mBodyFont );
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

  if ( ::qt_cast<KEdit*>(fw) )
      static_cast<QMultiLineEdit*>(fw)->undo();
  else if (::qt_cast<QLineEdit*>(fw))
      static_cast<QLineEdit*>(fw)->undo();
}

void KMComposeWin::slotRedo()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (::qt_cast<KEdit*>(fw))
      static_cast<KEdit*>(fw)->redo();
  else if (::qt_cast<QLineEdit*>(fw))
      static_cast<QLineEdit*>(fw)->redo();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCut()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (::qt_cast<KEdit*>(fw))
      static_cast<KEdit*>(fw)->cut();
  else if (::qt_cast<QLineEdit*>(fw))
      static_cast<QLineEdit*>(fw)->cut();
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

  if (::qt_cast<QLineEdit*>(fw))
      static_cast<QLineEdit*>(fw)->selectAll();
  else if (::qt_cast<KEdit*>(fw))
      static_cast<KEdit*>(fw)->selectAll();
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
               && !mLastIdentityHasOpenPgpKey
               // ### hack needed as long as we don't specify S/MIME keys in identities.
               && ( !mSelectedCryptPlug || mSelectedCryptPlug->protocol() == "openpgp" ) ) {
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
  if ( sign && !mLastIdentityHasOpenPgpKey
            // ### hack needed as long as we don't specify S/MIME keys in identities.
            && ( !mSelectedCryptPlug || mSelectedCryptPlug->protocol() == "openpgp" ) ) {
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
  mMessageWasModified = ( mEditor->isModified() || mEdtFrom->edited() ||
                          mEdtReplyTo->edited() || mEdtTo->edited() ||
                          mEdtCc->edited() || mEdtBcc->edited() ||
                          mEdtSubject->edited() || mAtmModified ||
                          ( mTransport->lineEdit() &&
                            mTransport->lineEdit()->edited() ) );
  connect( this, SIGNAL( applyChangesDone( bool ) ),
           this, SLOT( slotContinuePrint( bool ) ) );
  applyChanges( true, true );
}

void KMComposeWin::slotContinuePrint( bool rc )
{
  disconnect( this, SIGNAL( applyChangesDone( bool ) ),
              this, SLOT( slotContinuePrint( bool ) ) );

  if( rc ) {
    KMCommand *command = new KMPrintCommand( this, mMsg );
    command->start();
    mEditor->setModified( mMessageWasModified );
  }
}

//----------------------------------------------------------------------------
void KMComposeWin::doSend(int aSendNow, bool saveInDrafts)
{
  mSendNow = aSendNow;
  mSaveInDrafts = saveInDrafts;

  if (!saveInDrafts)
  {
     if (to().isEmpty())
     {
        mEdtTo->setFocus();
        KMessageBox::information( this,
                                  i18n("You must specify at least one "
                                       "receiver in the To: field.") );
        return;
     }

     if (subject().isEmpty())
     {
        mEdtSubject->setFocus();
        int rc =
          KMessageBox::questionYesNo( this,
                                      i18n("You did not specify a subject. "
                                           "Send message anyway?"),
                                      i18n("No Subject Specified"),
                                      i18n("S&end as Is"),
                                      i18n("&Specify the Subject"),
                                      "no_subject_specified" );
        if( rc == KMessageBox::No )
        {
           return;
        }
     }

     if ( userForgotAttachment() )
       return;
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
  const bool neverSign = saveInDrafts && mNeverSignWhenSavingInDrafts;
  const bool neverEncrypt = saveInDrafts && mNeverEncryptWhenSavingInDrafts;
  connect( this, SIGNAL( applyChangesDone( bool ) ),
           SLOT( slotContinueDoSend( bool ) ) );
  applyChanges( neverSign, neverEncrypt );
}

void KMComposeWin::slotContinueDoSend( bool sentOk )
{
  disconnect( this, SIGNAL( applyChangesDone( bool ) ),
              this, SLOT( slotContinueDoSend( bool ) ) );

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
      return;

  // needed for imap
  mMsg->setComplete( true );

  if (mSaveInDrafts)
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
    if( !mComposer->originalBCC().isEmpty() )
      mMsg->setBcc( KMMessage::expandAliases( mComposer->originalBCC() ));
    QString recips = mMsg->headerField( "X-KMail-Recipients" );
    if( !recips.isEmpty() ) {
      mMsg->setHeaderField( "X-KMail-Recipients", KMMessage::expandAliases( recips ) );
    }
    mMsg->cleanupHeader();
    sentOk = kmkernel->msgSender()->send(mMsg, mSendNow);
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
      sentOk &= kmkernel->msgSender()->send(msg, mSendNow);
    }
  }

  if (!sentOk)
     return;

  RecentAddresses::self( KMKernel::config() )->add( bcc() );
  RecentAddresses::self( KMKernel::config() )->add( cc() );
  RecentAddresses::self( KMKernel::config() )->add( to() );

  mAutoDeleteMsg = FALSE;
  mFolder = 0;
  close();
  return;
}



//----------------------------------------------------------------------------
void KMComposeWin::slotSendLater()
{
  if ( mEditor->checkExternalEditorFinished() )
    doSend( false );
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSaveDraft() {
  if ( mEditor->checkExternalEditorFinished() )
    doSend( false, true );
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

QPopupMenu *KMLineEdit::createPopupMenu()
{
    QPopupMenu *menu = KLineEdit::createPopupMenu();
    if ( !menu )
        return 0;

    menu->insertSeparator();
    menu->insertItem( i18n( "Edit recent addresses..." ),
                      this, SLOT( editRecentAddresses() ) );

    return menu;
}

void KMLineEdit::editRecentAddresses()
{
  KRecentAddress::RecentAddressDialog dlg( this );
  dlg.setAddresses( RecentAddresses::self( KMKernel::config() )->addresses() );
  if ( dlg.exec() ) {
    RecentAddresses::self( KMKernel::config() )->clear();
    QStringList addrList = dlg.addresses();
    QStringList::Iterator it;
    for ( it = addrList.begin(); it != addrList.end(); ++it )
      RecentAddresses::self( KMKernel::config() )->add( *it );

    loadAddresses();
  }
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

    QStringList recent =
      RecentAddresses::self( KMKernel::config() )->addresses();
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
