// -*- mode: C++; c-file-style: "gnu" -*-
// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#undef GrayScale
#undef Color
#include <config.h>

#define REALLY_WANT_KMCOMPOSEWIN_H
#include "kmcomposewin.h"
#undef REALLY_WANT_KMCOMPOSEWIN_H

#include "kmedit.h"
#include "kmlineeditspell.h"
#include "kmatmlistview.h"

#include "kmmainwin.h"
#include "kmreadermainwin.h"
#include "messagesender.h"
#include "kmmsgpartdlg.h"
#include <kpgpblock.h>
#include <kaddrbook.h>
#include "kmaddrbook.h"
#include "kmmsgdict.h"
#include "kmfolderimap.h"
#include "kmfoldermgr.h"
#include "kmfoldercombobox.h"
#include "kmtransport.h"
#include "kmcommands.h"
#include "kcursorsaver.h"
#include "partNode.h"
#include "transportmanager.h"
#include "attachmentlistview.h"
using KMail::AttachmentListView;
#include "dictionarycombobox.h"
using KMail::DictionaryComboBox;
#include "addressesdialog.h"
using KPIM::AddressesDialog;
#include "addresseeemailselection.h"
using KPIM::AddresseeEmailSelection;
using KPIM::AddresseeSelectorDialog;
#include <maillistdrag.h>
using KPIM::MailListDrag;
#include "recentaddresses.h"
using KRecentAddress::RecentAddresses;
#include "kleo_util.h"
#include "stl_util.h"
#include "recipientseditor.h"

#include "attachmentcollector.h"
#include "objecttreeparser.h"

#include "kmfoldermaildir.h"

#include <libkpimidentities/identitymanager.h>
#include <libkpimidentities/identitycombo.h>
#include <libkpimidentities/identity.h>
#include <libkdepim/kfileio.h>
#include <libemailfunctions/email.h>
#include <kleo/cryptobackendfactory.h>
#include <kleo/exportjob.h>
#include <kleo/specialjob.h>
#include <ui/progressdialog.h>
#include <ui/keyselectiondialog.h>

#include <gpgmepp/context.h>
#include <gpgmepp/key.h>

#include <kabc/vcardconverter.h>
#include <libkdepim/kvcarddrag.h>
#include <kio/netaccess.h>


#include "klistboxdialog.h"

#include "messagecomposer.h"
#include "chiasmuskeyselector.h"

#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kcharsets.h>
#include <kcompletionbox.h>
#include <kcursor.h>
#include <kcombobox.h>
#include <kstdaccel.h>
#include <kmenu.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <ktoolbar.h>
#include <kwin.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kio/scheduler.h>
#include <ktempfile.h>
#include <klocale.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kdirwatch.h>
#include <kstdguiitem.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <kuserprofile.h>
#include <krun.h>
#include <ktempdir.h>
#include <ktoggleaction.h>
#include <kfontaction.h>
#include <kfontsizeaction.h>
//#include <keditlistbox.h>
#include "globalsettings.h"
#include "replyphrases.h"

#include <kspell.h>
#include <kspelldlg.h>
#include <spellingfilter.h>
#include <k3syntaxhighlighter.h>
#include <kcolordialog.h>
#include <kzip.h>
#include <ksavefile.h>
#include <ktoolinvocation.h>


//Added by qt3to4:
#include <Q3CString>
#include <q3header.h>
#include <q3tabdialog.h>

#include <QBuffer>
#include <QEvent>
#include <QFontDatabase>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QRegExp>
#include <QTextCodec>
#include <QToolTip>

#include <mimelib/mimepp.h>

#include <algorithm>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <krecentfilesaction.h>
#include "kmcomposewin.moc"

KMail::Composer * KMail::makeComposer( KMMessage * msg, uint identitiy ) {
  return KMComposeWin::create( msg, identitiy );
}

KMail::Composer * KMComposeWin::create( KMMessage * msg, uint identitiy ) {
  return new KMComposeWin( msg, identitiy );
}

//-----------------------------------------------------------------------------
KMComposeWin::KMComposeWin( KMMessage *aMsg, uint id  )
  : MailComposerIface(), KMail::Composer( "kmail-composer#" ),
    mSpellCheckInProgress( false ),
    mDone( false ),
    mAtmModified( false ),
    mMsg( 0 ),
    mAttachMenu( 0 ),
    mSigningAndEncryptionExplicitlyDisabled( false ),
    mFolder( 0 ),
    mUseHTMLEditor( false ),
    mId( id ),
    mAttachPK( 0 ), mAttachMPK( 0 ),
    mAttachRemoveAction( 0 ), mAttachSaveAction( 0 ), mAttachPropertiesAction( 0 ),
    mSignAction( 0 ), mEncryptAction( 0 ), mRequestMDNAction( 0 ),
    mUrgentAction( 0 ), mAllFieldsAction( 0 ), mFromAction( 0 ),
    mReplyToAction( 0 ), mToAction( 0 ), mCcAction( 0 ), mBccAction( 0 ),
    mSubjectAction( 0 ),
    mIdentityAction( 0 ), mTransportAction( 0 ), mFccAction( 0 ),
    mWordWrapAction( 0 ), mFixedFontAction( 0 ), mAutoSpellCheckingAction( 0 ),
    mDictionaryAction( 0 ),
    mEncodingAction( 0 ),
    mCryptoModuleAction( 0 ),
    mEncryptChiasmusAction( 0 ),
    mEncryptWithChiasmus( false ),
    mComposer( 0 ),
    mLabelWidth( 0 ),
    mAutoSaveTimer( 0 ), mLastAutoSaveErrno( 0 )
{
  mClassicalRecipients = GlobalSettings::self()->recipientsEditorType() ==
    GlobalSettings::EnumRecipientsEditorType::Classic;

  mSubjectTextWasSpellChecked = false;
  if (kmkernel->xmlGuiInstance())
    setInstance( kmkernel->xmlGuiInstance() );
  mMainWidget = new QWidget(this);
  mIdentity = new KPIM::IdentityCombo(kmkernel->identityManager(), mMainWidget);
  mDictionaryCombo = new DictionaryComboBox( mMainWidget );
  mFcc = new KMFolderComboBox(mMainWidget);
  mFcc->showOutboxFolder( false );
  mTransport = new QComboBox(mMainWidget);
  mTransport->setEditable( true );
  mEdtFrom = new KMLineEdit(false,mMainWidget, "fromLine");

  mEdtReplyTo = new KMLineEdit(true,mMainWidget, "replyToLine");
  mLblReplyTo = new QLabel(mMainWidget);
  mBtnReplyTo = new QPushButton("...",mMainWidget);
  mBtnReplyTo->setFocusPolicy(Qt::NoFocus);
  connect(mBtnReplyTo,SIGNAL(clicked()),SLOT(slotAddrBookReplyTo()));
  connect(mEdtReplyTo,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
          SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));

  if ( mClassicalRecipients ) {
    mRecipientsEditor = 0;

    mEdtTo = new KMLineEdit(true,mMainWidget, "toLine");
    mEdtCc = new KMLineEdit(true,mMainWidget, "ccLine");
    mEdtBcc = new KMLineEdit(true,mMainWidget, "bccLine");

    mLblTo = new QLabel(mMainWidget);
    mLblCc = new QLabel(mMainWidget);
    mLblBcc = new QLabel(mMainWidget);

    mBtnTo = new QPushButton("...",mMainWidget);
    mBtnCc = new QPushButton("...",mMainWidget);
    mBtnBcc = new QPushButton("...",mMainWidget);
    //mBtnFrom = new QPushButton("...",mMainWidget);

    QString tip = i18n("Select email address(es)");
    mBtnTo->setToolTip( tip );
    mBtnCc->setToolTip( tip );
    mBtnBcc->setToolTip( tip );
    mBtnReplyTo->setToolTip( tip );

    mBtnTo->setFocusPolicy(Qt::NoFocus);
    mBtnCc->setFocusPolicy(Qt::NoFocus);
    mBtnBcc->setFocusPolicy(Qt::NoFocus);
    //mBtnFrom->setFocusPolicy(QWidget::NoFocus);

    connect(mBtnTo,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
    connect(mBtnCc,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
    connect(mBtnBcc,SIGNAL(clicked()),SLOT(slotAddrBookTo()));
    //connect(mBtnFrom,SIGNAL(clicked()),SLOT(slotAddrBookFrom()));

    connect(mEdtTo,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
            SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
    connect(mEdtCc,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
            SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));
    connect(mEdtBcc,SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
            SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)));

    mEdtTo->setFocus();
  } else {
    mEdtTo = 0;
    mEdtCc = 0;
    mEdtBcc = 0;

    mLblTo = 0;
    mLblCc = 0;
    mLblBcc = 0;

    mBtnTo = 0;
    mBtnCc = 0;
    mBtnBcc = 0;
    //mBtnFrom = 0;

    mRecipientsEditor = new RecipientsEditor( mMainWidget );

    mRecipientsEditor->setFocus();
  }
  mEdtSubject = new KMLineEditSpell(false,mMainWidget, "subjectLine");
  mLblIdentity = new QLabel(mMainWidget);
  mDictionaryLabel = new QLabel( mMainWidget );
  mLblFcc = new QLabel(mMainWidget);
  mLblTransport = new QLabel(mMainWidget);
  mLblFrom = new QLabel(mMainWidget);
  mLblSubject = new QLabel(mMainWidget);
  QString sticky = i18n("Sticky");
  mBtnIdentity = new QCheckBox(sticky,mMainWidget);
  mBtnFcc = new QCheckBox(sticky,mMainWidget);
  mBtnTransport = new QCheckBox(sticky,mMainWidget);

  //setWFlags( WType_TopLevel | WStyle_Dialog );
  mHtmlMarkup = GlobalSettings::self()->useHtmlMarkup();
  mShowHeaders = GlobalSettings::self()->headers();
  mDone = false;
  mGrid = 0;
  mAtmListView = 0;
  mAtmModified = false;
  mAutoDeleteMsg = false;
  mFolder = 0;
  mAutoCharset = true;
  mFixedFontAction = 0;
  mTempDir = 0;
  mSplitter = new QSplitter( Qt::Vertical, mMainWidget );
  mSplitter->setObjectName( "mSplitter" );
  mEditor = new KMEdit( mSplitter, this, mDictionaryCombo->spellConfig() );
  mSplitter->insertWidget( 0, mEditor );
  mSplitter->setOpaqueResize( true );

  mEditor->initializeAutoSpellChecking();
  mEditor->setTextFormat(Qt::PlainText);
  mEditor->setAcceptDrops( true );

  mBtnIdentity->setWhatsThis(
    GlobalSettings::self()->stickyIdentityItem()->whatsThis() );
  mBtnFcc->setWhatsThis(
    GlobalSettings::self()->stickyFccItem()->whatsThis() );
  mBtnTransport->setWhatsThis(
    GlobalSettings::self()->stickyTransportItem()->whatsThis() );

  mSpellCheckInProgress=false;

  setCaption( i18n("Composer") );
  setMinimumSize(200,200);

  mBtnIdentity->setFocusPolicy(Qt::NoFocus);
  mBtnFcc->setFocusPolicy(Qt::NoFocus);
  mBtnTransport->setFocusPolicy(Qt::NoFocus);

  mAtmListView = new AttachmentListView( this, mSplitter );
  mAtmListView->setObjectName( "attachment list view" );
  mAtmListView->setSelectionMode( Q3ListView::Extended );
  mAtmListView->addColumn( i18n("Name"), 200 );
  mAtmListView->addColumn( i18n("Size"), 80 );
  mAtmListView->addColumn( i18n("Encoding"), 120 );
  int atmColType = mAtmListView->addColumn( i18n("Type"), 120 );
  // Stretch "Type".
  mAtmListView->header()->setStretchEnabled( true, atmColType );
  mAtmEncryptColWidth = 80;
  mAtmSignColWidth = 80;
  mAtmCompressColWidth = 100;
  mAtmColCompress = mAtmListView->addColumn( i18n("Compress"),
                                            mAtmCompressColWidth );
  mAtmColEncrypt = mAtmListView->addColumn( i18n("Encrypt"),
                                            mAtmEncryptColWidth );
  mAtmColSign    = mAtmListView->addColumn( i18n("Sign"),
                                            mAtmSignColWidth );
  mAtmListView->setColumnWidth( mAtmColEncrypt, 0 );
  mAtmListView->setColumnWidth( mAtmColSign,    0 );
  mAtmListView->setAllColumnsShowFocus( true );

  connect( mAtmListView,
           SIGNAL( doubleClicked( Q3ListViewItem* ) ),
           SLOT( slotAttachProperties() ) );
  connect( mAtmListView,
           SIGNAL( rightButtonPressed( Q3ListViewItem*, const QPoint&, int ) ),
           SLOT( slotAttachPopupMenu( Q3ListViewItem*, const QPoint&, int ) ) );
  connect( mAtmListView,
           SIGNAL( selectionChanged() ),
           SLOT( slotUpdateAttachActions() ) );
  connect( mAtmListView,
           SIGNAL( attachmentDeleted() ),
           SLOT( slotAttachRemove() ) );
  mAttachMenu = 0;

  readConfig();
  setupStatusBar();
  setupActions();
  setupEditor();

  applyMainWindowSettings(KMKernel::config(), "Composer");

  connect( mEdtSubject, SIGNAL( subjectTextSpellChecked() ),
           SLOT( slotSubjectTextSpellChecked() ) );
  connect(mEdtSubject,SIGNAL(textChanged(const QString&)),
          SLOT(slotUpdWinTitle(const QString&)));
  connect(mIdentity,SIGNAL(identityChanged(uint)),
          SLOT(slotIdentityChanged(uint)));
  connect( kmkernel->identityManager(), SIGNAL(changed(uint)),
          SLOT(slotIdentityChanged(uint)));

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
  connect (mEditor, SIGNAL( pasteImage() ),
    this, SLOT (slotPaste() ) );
  connect (mEditor, SIGNAL( focusChanged(bool) ),
    this, SLOT (editorFocusChanged(bool)) );

  mMainWidget->resize(480,510);
  setCentralWidget(mMainWidget);
  rethinkFields();

  if ( !mClassicalRecipients ) {
    // This is ugly, but if it isn't called the line edits in the recipients
    // editor aren't wide enough until the first resize event comes.
    rethinkFields();
  }

  if ( GlobalSettings::self()->useExternalEditor() ) {
    mEditor->setUseExternalEditor(true);
    mEditor->setExternalEditorPath( GlobalSettings::self()->externalEditor() );
  }

  initAutoSave();

  mMsg = 0;
  if (aMsg)
    setMsg(aMsg);
  fontChanged( mEditor->currentFont() ); // set toolbar buttons to correct values

  mDone = true;
}

//-----------------------------------------------------------------------------
KMComposeWin::~KMComposeWin()
{
  writeConfig();
  if (mFolder && mMsg)
  {
    mAutoDeleteMsg = false;
    mFolder->addMsg(mMsg);
    // Ensure that the message is correctly and fully parsed
    mFolder->unGetMsg( mFolder->count() - 1 );
  }
  if (mAutoDeleteMsg) {
    delete mMsg;
    mMsg = 0;
  }
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.begin();
  while ( it != mMapAtmLoadData.end() )
  {
    KIO::Job *job = it.key();
    mMapAtmLoadData.erase( it );
    job->kill();
    it = mMapAtmLoadData.begin();
  }
  deleteAll( mComposedMessages );

  qDeleteAll( mAtmList );
  qDeleteAll( mAtmTempList );
}

void KMComposeWin::setAutoDeleteWindow( bool f )
{
  setAttribute( Qt::WA_DeleteOnClose, f );
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
void KMComposeWin::addAttachment(KUrl url,QString /*comment*/)
{
  addAttach(url);
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttachment(const QString &name,
                                 const Q3CString &/*cte*/,
                                 const QByteArray &data,
                                 const Q3CString &type,
                                 const Q3CString &subType,
                                 const Q3CString &paramAttr,
                                 const QString &paramValue,
                                 const Q3CString &contDisp)
{
  if (!data.isEmpty()) {
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName(name);
    QList<int> dummy;
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
void KMComposeWin::addImageFromClipboard()
{
  bool ok;
  QFile *tmpFile;

  QString attName = KInputDialog::getText( "KMail", i18n("Name of the attachment:"), QString(), &ok, this );
  if ( !ok )
    return;

  mTempDir = new KTempDir();
  mTempDir->setAutoDelete( true );

  if ( attName.toLower().endsWith(".png") )
    tmpFile = new QFile(mTempDir->name() + attName );
  else
    tmpFile = new QFile(mTempDir->name() + attName + ".png" );

  if ( !QApplication::clipboard()->image().save( tmpFile->fileName(), "PNG" ) ) {
    KMessageBox::error( this, i18n("Unknown error trying to save image."), i18n("Attaching Image Failed") );
    delete mTempDir;
    mTempDir = 0;
    return;
  }

  addAttach( tmpFile->fileName() );
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
  return KMail::Composer::event(e);
}


//-----------------------------------------------------------------------------
void KMComposeWin::readColorConfig(void)
{
  if ( GlobalSettings::self()->useDefaultColors() ) {
    mForeColor = QColor(kapp->palette().color( QPalette::Text ));
    mBackColor = QColor(kapp->palette().color( QPalette::Base ));
  } else {
    mForeColor = GlobalSettings::self()->foregroundColor();
    mBackColor = GlobalSettings::self()->backgroundColor();
  }

  // Color setup
  mPalette = kapp->palette();
  mPalette.setColor( QPalette::Base, mBackColor );
  mPalette.setColor( QPalette::Text, mForeColor );
  # warning "FIXME: Do we need to call setDisabled/setActive/setInactive or are the setColor calls enough??"
//   mPalette.setDisabled(cgrp);
//   mPalette.setActive(cgrp);
//   mPalette.setInactive(cgrp);

  mEdtFrom->setPalette(mPalette);
  mEdtReplyTo->setPalette(mPalette);
  if ( mClassicalRecipients ) {
    mEdtTo->setPalette(mPalette);
    mEdtCc->setPalette(mPalette);
    mEdtBcc->setPalette(mPalette);
  }
  mEdtSubject->setPalette(mPalette);
  mTransport->setPalette(mPalette);
  mEditor->setPalette(mPalette);
  mFcc->setPalette(mPalette);
}

//-----------------------------------------------------------------------------
void KMComposeWin::readConfig(void)
{
  mDefCharset = KMMessage::defaultCharset();
  mBtnIdentity->setChecked( GlobalSettings::self()->stickyIdentity() );
  if (mBtnIdentity->isChecked()) {
    mId = (GlobalSettings::self()->previousIdentity()!=0) ?
           GlobalSettings::self()->previousIdentity() : mId;
  }
  mBtnFcc->setChecked( GlobalSettings::self()->stickyFcc() );
  mBtnTransport->setChecked( GlobalSettings::self()->stickyTransport() );
  QStringList transportHistory = GlobalSettings::self()->transportHistory();
  QString currentTransport = GlobalSettings::self()->currentTransport();

  mEdtFrom->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
  mEdtReplyTo->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
  if ( mClassicalRecipients ) {
    mEdtTo->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
    mEdtCc->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
    mEdtBcc->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
  }

  readColorConfig();

  if ( GlobalSettings::self()->useDefaultFonts() ) {
    mBodyFont = KGlobalSettings::generalFont();
    mFixedFont = KGlobalSettings::fixedFont();
  } else {
    mBodyFont = GlobalSettings::self()->composerFont();
    mFixedFont = GlobalSettings::self()->fixedFont();
  }

  slotUpdateFont();
  mEdtFrom->setFont(mBodyFont);
  mEdtReplyTo->setFont(mBodyFont);
  if ( mClassicalRecipients ) {
    mEdtTo->setFont(mBodyFont);
    mEdtCc->setFont(mBodyFont);
    mEdtBcc->setFont(mBodyFont);
  }
  mEdtSubject->setFont(mBodyFont);

  QSize siz = GlobalSettings::self()->composerSize();
  if (siz.width() < 200) siz.setWidth(200);
  if (siz.height() < 200) siz.setHeight(200);
  resize(siz);

  mIdentity->setCurrentIdentity( mId );

  kDebug(5006) << "KMComposeWin::readConfig. " << mIdentity->currentIdentityName() << endl;
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  mTransport->clear();
  mTransport->addItems( KMTransportInfo::availableTransports() );
  while ( transportHistory.count() > GlobalSettings::self()->maxTransportEntries() )
    transportHistory.removeLast();
  mTransport->addItems( transportHistory );
  if (mBtnTransport->isChecked() && !currentTransport.isEmpty())
  {
    for (int i = 0; i < mTransport->count(); i++)
      if (mTransport->itemText(i) == currentTransport)
        mTransport->setCurrentIndex(i);
    mTransport->setEditText( currentTransport );
  }

  if ( !mBtnTransport->isChecked() ) {
    mTransport->setItemText( mTransport->currentIndex(), GlobalSettings::self()->defaultTransport() );
  }

  QString fccName = "";
  if ( mBtnFcc->isChecked() ) {
    fccName = GlobalSettings::self()->previousFcc();
  } else if ( !ident.fcc().isEmpty() ) {
      fccName = ident.fcc();
  }

  setFcc( fccName );
}

//-----------------------------------------------------------------------------
void KMComposeWin::writeConfig(void)
{
  GlobalSettings::self()->setHeaders( mShowHeaders );
  GlobalSettings::self()->setStickyTransport( mBtnTransport->isChecked() );
  GlobalSettings::self()->setStickyIdentity( mBtnIdentity->isChecked() );
  GlobalSettings::self()->setStickyFcc( mBtnFcc->isChecked() );
  GlobalSettings::self()->setPreviousIdentity( mIdentity->currentIdentity() );
  GlobalSettings::self()->setCurrentTransport( mTransport->currentText() );
  GlobalSettings::self()->setPreviousFcc( mFcc->getFolder()->idString() );
  GlobalSettings::self()->setAutoSpellChecking(
                        mAutoSpellCheckingAction->isChecked() );
  QStringList transportHistory = GlobalSettings::self()->transportHistory();
  transportHistory.removeAll(mTransport->currentText());
    if (!KMTransportInfo::availableTransports().contains(mTransport->currentText())) {
      transportHistory.prepend(mTransport->currentText());
  }
  GlobalSettings::self()->setTransportHistory( transportHistory );
  GlobalSettings::self()->setUseFixedFont( mFixedFontAction->isChecked() );
  GlobalSettings::self()->setUseHtmlMarkup( mHtmlMarkup );
  GlobalSettings::self()->setComposerSize( size() );

  KConfigGroup group( KMKernel::config(), "Geometry" );
  saveMainWindowSettings( KMKernel::config(), "Composer" );
}

//-----------------------------------------------------------------------------
void KMComposeWin::autoSaveMessage()
{
  kDebug(5006) << k_funcinfo << endl;
  if ( !mMsg || mComposer || mAutoSaveFilename.isEmpty() )
    return;
  kDebug(5006) << k_funcinfo << "autosaving message" << endl;

  if ( mAutoSaveTimer )
    mAutoSaveTimer->stop();
  connect( this, SIGNAL( applyChangesDone( bool ) ),
           this, SLOT( slotContinueAutoSave( bool ) ) );
  // This method is called when KMail crashed, so don't try signing/encryption
  // and don't disable controls because it is also called from a timer and
  // then the disabling is distracting.
  applyChanges( true, true );

  // Don't continue before the applyChanges is done!
  qApp->enter_loop();

  // Ok, it's done now - continue dead letter saving
  if ( mComposedMessages.isEmpty() ) {
    kDebug(5006) << "Composing the message failed." << endl;
    return;
  }
  KMMessage *msg = mComposedMessages.first();

  kDebug(5006) << k_funcinfo << "opening autoSaveFile " << mAutoSaveFilename
                << endl;
  const QString filename =
    KMKernel::localDataPath() + "autosave/cur/" + mAutoSaveFilename;
  KSaveFile autoSaveFile( filename, 0600 );
  int status = autoSaveFile.status();
  kDebug(5006) << k_funcinfo << "autoSaveFile.status() = " << status << endl;
  if ( status == 0 ) { // no error
    kDebug(5006) << "autosaving message in " << filename << endl;
    int fd = autoSaveFile.handle();
    Q3CString msgStr = msg->asString();
    if ( ::write( fd, msgStr, msgStr.length() ) == -1 )
      status = errno;
  }
  if ( status == 0 ) {
    kDebug(5006) << k_funcinfo << "closing autoSaveFile" << endl;
    autoSaveFile.close();
    mLastAutoSaveErrno = 0;
  }
  else {
    kDebug(5006) << k_funcinfo << "autosaving failed" << endl;
    autoSaveFile.abort();
    if ( status != mLastAutoSaveErrno ) {
      // don't show the same error message twice
      KMessageBox::queuedMessageBox( 0, KMessageBox::Sorry,
                                     i18n("Autosaving the message as %1 "
                                          "failed.\n"
                                          "Reason: %2",
                                           filename, strerror( status ) ),
                                     i18n("Autosaving Failed") );
      mLastAutoSaveErrno = status;
    }
  }

  if ( autoSaveInterval() > 0 )
    mAutoSaveTimer->start( autoSaveInterval() );
}

void KMComposeWin::slotContinueAutoSave( bool )
{
  disconnect( this, SIGNAL( applyChangesDone( bool ) ),
              this, SLOT( slotContinueAutoSave( bool ) ) );
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
  if ( sender()->metaObject()->className() != "KToggleAction")
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
     kDebug(5006) << "Something is wrong (Oh, yeah?)" << endl;
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

int KMComposeWin::calcColumnWidth(int which, long allShowing, int width)
{
  if ( (allShowing & which) == 0 )
    return width;

  QLabel *w;
  if ( which == HDR_IDENTITY )
    w = mLblIdentity;
  else if ( which == HDR_DICTIONARY )
    w = mDictionaryLabel;
  else if ( which == HDR_FCC )
    w = mLblFcc;
  else if ( which == HDR_TRANSPORT )
    w = mLblTransport;
  else if ( which == HDR_FROM )
    w = mLblFrom;
  else if ( which == HDR_REPLY_TO )
    w = mLblReplyTo;
  else if ( which == HDR_SUBJECT )
    w = mLblSubject;
  else
    return width;

  w->setBuddy( mEditor ); // set dummy so we don't calculate width of '&' for this label.
  w->adjustSize();
  w->show();
  return qMax( width, w->sizeHint().width() );
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

  numRows = mNumHeaders + 1;

  delete mGrid;
  mGrid = new QGridLayout(mMainWidget);
  mGrid->setSpacing(KDialogBase::spacingHint());
  mGrid->setMargin(KDialogBase::marginHint()/2);
  mGrid->setColumnStretch(0, 1);
  mGrid->setColumnStretch(1, 100);
  mGrid->setColumnStretch(2, 1);
  mGrid->setRowStretch(mNumHeaders, 100);

  row = 0;
  kDebug(5006) << "KMComposeWin::rethinkFields" << endl;
  if (mRecipientsEditor)
    mLabelWidth = mRecipientsEditor->setFirstColumnWidth( 0 );
  mLabelWidth = calcColumnWidth( HDR_IDENTITY, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_DICTIONARY, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_FCC, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_TRANSPORT, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_FROM, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_REPLY_TO, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_SUBJECT, showHeaders, mLabelWidth );

  if (!fromSlot) mAllFieldsAction->setChecked(showHeaders==HDR_ALL);

  if (!fromSlot) mIdentityAction->setChecked(abs(mShowHeaders)&HDR_IDENTITY);
  rethinkHeaderLine(showHeaders,HDR_IDENTITY, row, i18n("&Identity:"),
                    mLblIdentity, mIdentity, mBtnIdentity);

  if (!fromSlot) mDictionaryAction->setChecked(abs(mShowHeaders)&HDR_DICTIONARY);
  rethinkHeaderLine(showHeaders,HDR_DICTIONARY, row, i18n("&Dictionary:"),
                    mDictionaryLabel, mDictionaryCombo, 0 );

  if (!fromSlot) mFccAction->setChecked(abs(mShowHeaders)&HDR_FCC);
  rethinkHeaderLine(showHeaders,HDR_FCC, row, i18n("&Sent-Mail folder:"),
                    mLblFcc, mFcc, mBtnFcc);

  if (!fromSlot) mTransportAction->setChecked(abs(mShowHeaders)&HDR_TRANSPORT);
  rethinkHeaderLine(showHeaders,HDR_TRANSPORT, row, i18n("&Mail transport:"),
                    mLblTransport, mTransport, mBtnTransport);

  if (!fromSlot) mFromAction->setChecked(abs(mShowHeaders)&HDR_FROM);
  rethinkHeaderLine(showHeaders,HDR_FROM, row, i18nc("sender address field", "&From:"),
                    mLblFrom, mEdtFrom /*, mBtnFrom */ );

  QWidget *prevFocus = mEdtFrom;

  if (!fromSlot) mReplyToAction->setChecked(abs(mShowHeaders)&HDR_REPLY_TO);
  rethinkHeaderLine(showHeaders,HDR_REPLY_TO,row,i18n("&Reply to:"),
                  mLblReplyTo, mEdtReplyTo, mBtnReplyTo);
  if ( showHeaders & HDR_REPLY_TO ) {
    prevFocus = connectFocusMoving( prevFocus, mEdtReplyTo );
  }

  if ( mClassicalRecipients ) {
    if (!fromSlot) mToAction->setChecked(abs(mShowHeaders)&HDR_TO);
    rethinkHeaderLine(showHeaders, HDR_TO, row, i18nc("recipient address field", "&To:"),
                    mLblTo, mEdtTo, mBtnTo,
                    i18n("Primary Recipients"),
                    i18n("<qt>The email addresses you put "
                         "in this field receive a copy of the email.</qt>"));
    if ( showHeaders & HDR_TO ) {
      prevFocus = connectFocusMoving( prevFocus, mEdtTo );
    }

    if (!fromSlot) mCcAction->setChecked(abs(mShowHeaders)&HDR_CC);
    rethinkHeaderLine(showHeaders, HDR_CC, row, i18n("&Copy to (CC):"),
                    mLblCc, mEdtCc, mBtnCc,
                    i18n("Additional Recipients"),
                    i18n("<qt>The email addresses you put "
                         "in this field receive a copy of the email. "
                         "Technically it is the same thing as putting all the "
                         "addresses in the <b>To:</b> field but differs in "
                         "that it usually symbolises the receiver of the "
                         "Carbon Copy (CC) is a listener, not the main "
                         "recipient.</qt>"));
    if ( showHeaders & HDR_CC ) {
      prevFocus = connectFocusMoving( prevFocus, mEdtCc );
    }

    if (!fromSlot) mBccAction->setChecked(abs(mShowHeaders)&HDR_BCC);
    rethinkHeaderLine(showHeaders,HDR_BCC, row, i18n("&Blind copy to (BCC):"),
                    mLblBcc, mEdtBcc, mBtnBcc,
                    i18n("Hidden Recipients"),
                    i18n("<qt>Essentially the same thing "
                         "as the <b>Copy To:</b> field but differs in that "
                         "all other recipients do not see who receives a "
                         "blind copy.</qt>"));
    if ( showHeaders & HDR_BCC ) {
      prevFocus = connectFocusMoving( prevFocus, mEdtBcc );
    }
  } else {
    mGrid->addWidget( mRecipientsEditor, row, 0, 1, 3 );
    ++row;

    if ( showHeaders & HDR_REPLY_TO ) {
      connect( mEdtReplyTo, SIGNAL( focusDown() ), mRecipientsEditor,
        SLOT( setFocusTop() ) );
    } else {
    connect( mEdtFrom, SIGNAL( focusDown() ), mRecipientsEditor,
      SLOT( setFocusTop() ) );
    }
    if ( showHeaders & HDR_REPLY_TO ) {
      connect( mRecipientsEditor, SIGNAL( focusUp() ), mEdtReplyTo, SLOT( setFocus() ) );
    } else {
      connect( mRecipientsEditor, SIGNAL( focusUp() ), mEdtFrom, SLOT( setFocus() ) );
    }

    connect( mRecipientsEditor, SIGNAL( focusDown() ), mEdtSubject,
      SLOT( setFocus() ) );
    connect( mEdtSubject, SIGNAL( focusUp() ), mRecipientsEditor,
      SLOT( setFocusBottom() ) );

    prevFocus = mRecipientsEditor;
  }
  if (!fromSlot) mSubjectAction->setChecked(abs(mShowHeaders)&HDR_SUBJECT);
  rethinkHeaderLine(showHeaders,HDR_SUBJECT, row, i18n("S&ubject:"),
                    mLblSubject, mEdtSubject);
  connectFocusMoving( mEdtSubject, mEditor );

  assert(row<=mNumHeaders);

  mGrid->addWidget(mSplitter, row, 0, mNumHeaders- row+1, 3 );

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
  if ( mReplyToAction ) mReplyToAction->setEnabled(!mAllFieldsAction->isChecked());
  if ( mToAction ) mToAction->setEnabled(!mAllFieldsAction->isChecked());
  if ( mCcAction ) mCcAction->setEnabled(!mAllFieldsAction->isChecked());
  if ( mBccAction ) mBccAction->setEnabled(!mAllFieldsAction->isChecked());
  mFccAction->setEnabled(!mAllFieldsAction->isChecked());
  mSubjectAction->setEnabled(!mAllFieldsAction->isChecked());
  if (mRecipientsEditor)
    mRecipientsEditor->setFirstColumnWidth( mLabelWidth );
}

QWidget *KMComposeWin::connectFocusMoving( QWidget *prev, QWidget *next )
{
  connect( prev, SIGNAL( focusDown() ), next, SLOT( setFocus() ) );
  connect( next, SIGNAL( focusUp() ), prev, SLOT( setFocus() ) );

  return next;
}

//-----------------------------------------------------------------------------
void KMComposeWin::rethinkHeaderLine(int aValue, int aMask, int& aRow,
                                     const QString &aLabelStr, QLabel* aLbl,
                                     QLineEdit* aEdt, QPushButton* aBtn,
                                     const QString &toolTip, const QString &whatsThis )
{
  if (aValue & aMask)
  {
    aLbl->setText(aLabelStr);
    if ( !toolTip.isEmpty() )
      aLbl->setToolTip( toolTip );
    if ( !whatsThis.isEmpty() )
      aLbl->setWhatsThis( whatsThis );
    aLbl->setFixedWidth( mLabelWidth );
    aLbl->setBuddy(aEdt);
    mGrid->addWidget(aLbl, aRow, 0);
    QPalette pal;
    pal.setColor( aEdt->backgroundRole(), mBackColor );
    aEdt->setPalette( pal );
    aEdt->show();

    if (aBtn) {
      mGrid->addWidget(aEdt, aRow, 1);

      mGrid->addWidget(aBtn, aRow, 2);
      aBtn->show();
    } else {
      mGrid->addWidget(aEdt, aRow, 1, 1, 2 );
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
void KMComposeWin::getTransportMenu()
{
  QStringList availTransports;

  mActNowMenu->clear();
  mActLaterMenu->clear();
  availTransports = KMail::TransportManager::transportNames();
  QStringList::Iterator it;
  int id = 0;
  for(it = availTransports.begin(); it != availTransports.end() ; ++it, id++)
  {
    mActNowMenu->insertItem((*it).replace("&", "&&"), id);
    mActLaterMenu->insertItem((*it).replace("&", "&&"), id);
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupActions(void)
{
  KActionMenu *actActionNowMenu, *actActionLaterMenu;

  if (kmkernel->msgSender()->sendImmediate()) //default == send now?
  {
    //default = send now, alternative = queue
    ( void )  new KAction( i18n("&Send Mail"), "mail_send", Qt::CTRL+Qt::Key_Return,
                        this, SLOT(slotSendNow()), actionCollection(),"send_default");

    // FIXME: change to mail_send_via icon when this exits.
    actActionNowMenu =  new KActionMenu ( KIcon( "mail_send"), i18n("&Send Mail Via"),
		    actionCollection(), "send_default_via" );

    KAction *action = new KAction(KIcon("queue"), i18n("Send &Later"), actionCollection(), "send_alternative");
    connect(action, SIGNAL(triggered(bool)), SLOT(slotSendLater()));
    actActionLaterMenu = new KActionMenu ( KIcon("queue"), i18n("Send &Later Via"),
		    actionCollection(), "send_alternative_via" );

  }
  else //no, default = send later
  {
    //default = queue, alternative = send now
    (void) new KAction (i18n("Send &Later"), "queue",
                        Qt::CTRL+Qt::Key_Return,
                        this, SLOT(slotSendLater()), actionCollection(),"send_default");
    actActionLaterMenu = new KActionMenu ( KIcon("queue"), i18n("Send &Later Via"),
		    actionCollection(), "send_default_via" );

   ( void )  new KAction( i18n("&Send Mail"), "mail_send", 0,
                        this, SLOT(slotSendNow()), actionCollection(),"send_alternative");

    // FIXME: change to mail_send_via icon when this exits.
    actActionNowMenu =  new KActionMenu ( KIcon("mail_send"), i18n("&Send Mail Via"),
		    actionCollection(), "send_alternative_via" );

  }

  // needed for sending "default transport"
  actActionNowMenu->setDelayed(true);
  actActionLaterMenu->setDelayed(true);

  connect(  actActionNowMenu, SIGNAL(  activated() ), this,
		    SLOT( slotSendNow() ) );
  connect(  actActionLaterMenu, SIGNAL(  activated() ), this,
		    SLOT( slotSendLater() ) );


  mActNowMenu = actActionNowMenu->popupMenu();
  mActLaterMenu = actActionLaterMenu->popupMenu();

  connect(  mActNowMenu, SIGNAL(  activated( int ) ), this,
		    SLOT( slotSendNowVia( int ) ) );
  connect(  mActNowMenu, SIGNAL(  aboutToShow() ), this,
		    SLOT( getTransportMenu() ) );

  connect(  mActLaterMenu, SIGNAL(  activated( int ) ), this,
		  SLOT( slotSendLaterVia( int ) ) );
  connect(  mActLaterMenu, SIGNAL(  aboutToShow() ), this,
		  SLOT( getTransportMenu() ) );




  (void) new KAction (i18n("Save in &Drafts Folder"), "filesave", 0,
                      this, SLOT(slotSaveDraft()),
                      actionCollection(), "save_in_drafts");
  (void) new KAction (i18n("&Insert File..."), "fileopen", 0,
                      this,  SLOT(slotInsertFile()),
                      actionCollection(), "insert_file");
  mRecentAction = new KRecentFilesAction (i18n("&Insert File Recent"),
		      "fileopen", 0,
		      this,  SLOT(slotInsertRecentFile(const KUrl&)),
		      actionCollection(), "insert_file_recent");

  mRecentAction->loadEntries( KMKernel::config() );

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

  if ( !mClassicalRecipients ) {
    KAction *action = new KAction( i18n("Select &Recipients..."), actionCollection(), "select_recipients" );
    connect(action, SIGNAL(triggered(bool) ), mRecipientsEditor, SLOT( selectRecipients() ));
    action->setShortcut(Qt::CTRL + Qt::Key_L);
    action = new KAction( i18n("Save &Distribution List..."), actionCollection(), "save_distribution_list" );
    connect(action, SIGNAL(triggered(bool) ), mRecipientsEditor, SLOT( saveDistributionList() ));
  }

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
  KStdAction::findNext(this, SLOT(slotSearchAgain()), actionCollection());

  KStdAction::replace (this, SLOT(slotReplace()), actionCollection());
  KStdAction::spelling (this, SLOT(slotSpellcheck()), actionCollection(), "spellcheck");

  mPasteQuotation = new KAction(i18n("Pa&ste as Quotation"), actionCollection(), "paste_quoted");
  connect(mPasteQuotation, SIGNAL(triggered(bool) ), SLOT( slotPasteAsQuotation()));

  KAction *action = new KAction(i18n("Paste as Attac&hment"), actionCollection(), "paste_att");
  connect(action, SIGNAL(triggered(bool) ), SLOT( slotPasteAsAttachment()));

  mAddQuoteChars = new KAction(i18n("Add &Quote Characters"), actionCollection(), "tools_quote");
  connect(mAddQuoteChars, SIGNAL(triggered(bool) ), SLOT(slotAddQuotes()));

  mRemQuoteChars = new KAction(i18n("Re&move Quote Characters"), actionCollection(), "tools_unquote");
  connect(mRemQuoteChars, SIGNAL(triggered(bool) ), SLOT(slotRemoveQuotes()));


  action = new KAction(i18n("Cl&ean Spaces"), actionCollection(), "clean_spaces");
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotCleanSpace()));

  mFixedFontAction = new KToggleAction( i18n("Use Fi&xed Font"), actionCollection(), "toggle_fixedfont" );
  connect(mFixedFontAction, SIGNAL(triggered(bool) ), SLOT(slotUpdateFont()));
  mFixedFontAction->setChecked( GlobalSettings::self()->useFixedFont() );

  //these are checkable!!!
  mUrgentAction = new KToggleAction (i18n("&Urgent"), KShortcut(),
                                    actionCollection(),
                                    "urgent");
  mRequestMDNAction = new KToggleAction ( i18n("&Request Disposition Notification"), KShortcut(),
                                         actionCollection(),
                                         "options_request_mdn");
  mRequestMDNAction->setChecked(GlobalSettings::self()->requestMDN());
  //----- Message-Encoding Submenu
  mEncodingAction = new KSelectAction( i18n( "Se&t Encoding" ), "charset",
                                      0, this, SLOT(slotSetCharset() ),
                                      actionCollection(), "charsets" );
  mWordWrapAction = new KToggleAction (i18n("&Wordwrap"), KShortcut(),
                      actionCollection(), "wordwrap");
  mWordWrapAction->setChecked(GlobalSettings::self()->wordWrap());
  connect(mWordWrapAction, SIGNAL(toggled(bool)), SLOT(slotWordWrapToggled(bool)));

  mAutoSpellCheckingAction =
    new KToggleAction( i18n( "&Automatic Spellchecking" ), "spellcheck", 0,
                       actionCollection(), "options_auto_spellchecking" );
  const bool spellChecking = GlobalSettings::self()->autoSpellChecking();
  mAutoSpellCheckingAction->setEnabled( !GlobalSettings::self()->useExternalEditor() );
  mAutoSpellCheckingAction->setChecked( !GlobalSettings::self()->useExternalEditor() && spellChecking );
  slotAutoSpellCheckingToggled( !GlobalSettings::self()->useExternalEditor() && spellChecking );
  connect( mAutoSpellCheckingAction, SIGNAL( toggled( bool ) ),
           this, SLOT( slotAutoSpellCheckingToggled( bool ) ) );

  QStringList encodings = KMMsgBase::supportedEncodings(true);
  encodings.prepend( i18n("Auto-Detect"));
  mEncodingAction->setItems( encodings );
  mEncodingAction->setCurrentItem( -1 );

  //these are checkable!!!
  markupAction = new KToggleAction(i18n("Formatting (HTML)"), actionCollection(), "html");
  connect(markupAction, SIGNAL(triggered(bool) ), SLOT(slotToggleMarkup()));

  mAllFieldsAction = new KToggleAction(i18n("&All Fields"), actionCollection(), "show_all_fields");
  connect(mAllFieldsAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mIdentityAction = new KToggleAction(i18n("&Identity"), actionCollection(), "show_identity");
  connect(mIdentityAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mDictionaryAction = new KToggleAction(i18n("&Dictionary"), actionCollection(), "show_dictionary");
  connect(mDictionaryAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mFccAction = new KToggleAction(i18n("&Sent-Mail Folder"), actionCollection(), "show_fcc");
  connect(mFccAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mTransportAction = new KToggleAction(i18n("&Mail Transport"), actionCollection(), "show_transport");
  connect(mTransportAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mFromAction = new KToggleAction(i18n("&From"), actionCollection(), "show_from");
  connect(mFromAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mReplyToAction = new KToggleAction(i18n("&Reply To"), actionCollection(), "show_reply_to");
  connect(mReplyToAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  if ( mClassicalRecipients ) {
    mToAction = new KToggleAction(i18n("&To"), actionCollection(), "show_to");
    connect(mToAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
    mCcAction = new KToggleAction(i18n("&CC"), actionCollection(), "show_cc");
    connect(mCcAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
    mBccAction = new KToggleAction(i18n("&BCC"), actionCollection(), "show_bcc");
    connect(mBccAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  }
  mSubjectAction = new KToggleAction(i18n("S&ubject"), actionCollection(), "show_subject");
  connect(mSubjectAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  //end of checkable

  action = new KAction(i18n("Append S&ignature"), actionCollection(), "append_signature");
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotAppendSignature()));
  mAttachPK = new KAction(i18n("Attach &Public Key..."), actionCollection(), "attach_public_key");
  connect(mAttachPK, SIGNAL(triggered(bool) ), SLOT(slotInsertPublicKey()));
  mAttachMPK = new KAction(i18n("Attach &My Public Key"), actionCollection(), "attach_my_public_key");
  connect(mAttachMPK, SIGNAL(triggered(bool) ), SLOT(slotInsertMyPublicKey()));
  (void) new KAction (i18n("&Attach File..."), "attach", 0, this, SLOT(slotAttachFile()), actionCollection(), "attach");
  mAttachRemoveAction = new KAction(i18n("&Remove Attachment"), actionCollection(), "remove");
  connect(mAttachRemoveAction, SIGNAL(triggered(bool) ), SLOT(slotAttachRemove()));
  mAttachSaveAction = new KAction (i18n("&Save Attachment As..."), "filesave",0,
                      this, SLOT(slotAttachSave()), actionCollection(), "attach_save");
  mAttachPropertiesAction = new KAction(i18n("Attachment Pr&operties"), actionCollection(), "attach_properties");
  connect(mAttachPropertiesAction, SIGNAL(triggered(bool) ), SLOT(slotAttachProperties()));

  setStandardToolBarMenuEnabled(true);

  KStdAction::keyBindings(this, SLOT(slotEditKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
  KStdAction::preferences(kmkernel, SLOT(slotShowConfigurationDialog()), actionCollection());

  action = new KAction(i18n("&Spellchecker..."), actionCollection(), "setup_spellchecker");
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotSpellcheckConfig()));

  if ( Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" ) ) {
    KToggleAction * a = new KToggleAction( i18n( "Encrypt Message with Chiasmus..." ),
                                           "chidecrypted", 0, actionCollection(),
                                           "encrypt_message_chiasmus" );
    a->setCheckedState( KGuiItem( i18n( "Encrypt Message with Chiasmus..." ), "chiencrypted" ) );
    mEncryptChiasmusAction = a;
    connect( mEncryptChiasmusAction, SIGNAL(toggled(bool)),
             this, SLOT(slotEncryptChiasmusToggled(bool)) );
  } else {
    mEncryptChiasmusAction = 0;
  }

  mEncryptAction = new KToggleAction (i18n("&Encrypt Message"),
                                     "decrypted", 0,
                                     actionCollection(), "encrypt_message");
  mSignAction = new KToggleAction (i18n("&Sign Message"),
                                  "signature", 0,
                                  actionCollection(), "sign_message");
  // get PGP user id for the chosen identity
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  // PENDING(marc): check the uses of this member and split it into
  // smime/openpgp and or enc/sign, if necessary:
  mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
  mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

  mLastEncryptActionState = false;
  mLastSignActionState = GlobalSettings::self()->pgpAutoSign();

  // "Attach public key" is only possible if OpenPGP support is available:
  mAttachPK->setEnabled( Kleo::CryptoBackendFactory::instance()->openpgp() );

  // "Attach my public key" is only possible if OpenPGP support is
  // available and the user specified his key for the current identity:
  mAttachMPK->setEnabled( Kleo::CryptoBackendFactory::instance()->openpgp() &&
			  !ident.pgpEncryptionKey().isEmpty() );

  if ( !Kleo::CryptoBackendFactory::instance()->openpgp() && !Kleo::CryptoBackendFactory::instance()->smime() ) {
    // no crypto whatsoever
    mEncryptAction->setEnabled( false );
    setEncryption( false );
    mSignAction->setEnabled( false );
    setSigning( false );
  } else {
    const bool canOpenPGPSign = Kleo::CryptoBackendFactory::instance()->openpgp()
      && !ident.pgpSigningKey().isEmpty();
    const bool canSMIMESign = Kleo::CryptoBackendFactory::instance()->smime()
      && !ident.smimeSigningKey().isEmpty();

    setEncryption( false );
    setSigning( ( canOpenPGPSign || canSMIMESign ) && GlobalSettings::self()->pgpAutoSign() );
  }

  connect(mEncryptAction, SIGNAL(toggled(bool)),
                         SLOT(slotEncryptToggled( bool )));
  connect(mSignAction,    SIGNAL(toggled(bool)),
                         SLOT(slotSignToggled(    bool )));

  QStringList l;
  for ( int i = 0 ; i < numCryptoMessageFormats ; ++i )
    l.push_back( Kleo::cryptoMessageFormatToLabel( cryptoMessageFormats[i] ) );

  mCryptoModuleAction = new KSelectAction( i18n( "&Cryptographic Message Format" ), 0,
					   this, SLOT(slotSelectCryptoModule()),
					   actionCollection(), "options_select_crypto" );
  mCryptoModuleAction->setItems( l );
  mCryptoModuleAction->setCurrentItem( format2cb( ident.preferredCryptoMessageFormat() ) );
  slotSelectCryptoModule( true /* initialize */ );

  QStringList styleItems;
  styleItems << i18n( "Standard" );
  styleItems << i18n( "Bulleted List (Disc)" );
  styleItems << i18n( "Bulleted List (Circle)" );
  styleItems << i18n( "Bulleted List (Square)" );
  styleItems << i18n( "Ordered List (Decimal)" );
  styleItems << i18n( "Ordered List (Alpha lower)" );
  styleItems << i18n( "Ordered List (Alpha upper)" );

  listAction = new KSelectAction( i18n( "Select Style" ), KShortcut(), actionCollection(),
                                 "text_list" );
  listAction->setItems( styleItems );
  connect( listAction, SIGNAL( activated( const QString& ) ),
           SLOT( slotListAction( const QString& ) ) );
  fontAction = new KFontAction( "Select Font", KShortcut(), actionCollection(),
                               "text_font" );
  connect( fontAction, SIGNAL( triggered( const QString& ) ),
           SLOT( slotFontAction( const QString& ) ) );
  fontSizeAction = new KFontSizeAction( "Select Size", KShortcut(), actionCollection(),
                                       "text_size" );
  connect( fontSizeAction, SIGNAL( fontSizeChanged( int ) ),
           SLOT( slotSizeAction( int ) ) );

  alignLeftAction = new KToggleAction(KIcon("text_left"), i18n("Align Left"), actionCollection(), "align_left");
  connect(alignLeftAction, SIGNAL(triggered(bool) ), SLOT(slotAlignLeft()));
  alignLeftAction->setChecked( true );
  alignRightAction = new KToggleAction(KIcon("text_right"), i18n("Align Right"), actionCollection(), "align_right");
  connect(alignRightAction, SIGNAL(triggered(bool) ), SLOT(slotAlignRight()));
  alignCenterAction = new KToggleAction(KIcon("text_center"), i18n("Align Center"), actionCollection(), "align_center");
  connect(alignCenterAction, SIGNAL(triggered(bool) ), SLOT(slotAlignCenter()));
  textBoldAction = new KToggleAction(KIcon("text_bold"),  i18n("&Bold"), actionCollection(), "text_bold");
  connect(textBoldAction, SIGNAL(triggered(bool) ), SLOT(slotTextBold()));
  textBoldAction->setShortcut(Qt::CTRL+Qt::Key_B);
  textItalicAction = new KToggleAction(KIcon("text_italic"),  i18n("&Italic"), actionCollection(), "text_italic");
  connect(textItalicAction, SIGNAL(triggered(bool) ), SLOT(slotTextItalic()));
  textItalicAction->setShortcut(Qt::CTRL+Qt::Key_I);
  textUnderAction = new KToggleAction(KIcon("text_under"),  i18n("&Underline"), actionCollection(), "text_under");
  connect(textUnderAction, SIGNAL(triggered(bool) ), SLOT(slotTextUnder()));
  textUnderAction->setShortcut(Qt::CTRL+Qt::Key_U);
  actionFormatReset = new KAction( i18n( "Reset Font Settings" ), "eraser", 0,
                                     this, SLOT( slotFormatReset() ),
                                     actionCollection(), "format_reset");
  actionFormatColor = new KAction( i18n( "Text Color..." ), "colorize", 0,
                                     this, SLOT( slotTextColor() ),
                                     actionCollection(), "format_color");

  createGUI("kmcomposerui.rc");

  connect( toolBar("htmlToolBar"), SIGNAL( visibilityChanged(bool) ),
           this, SLOT( htmlToolBarVisibilityChanged(bool) ) );

  // In Kontact, this entry would read "Configure Kontact", but bring
  // up KMail's config dialog. That's sensible, though, so fix the label.
  KAction* configureAction = actionCollection()->action("options_configure" );
  if ( configureAction )
    configureAction->setText( i18n("Configure KMail" ) );
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupStatusBar(void)
{
  statusBar()->insertItem("", 0, 1);
  statusBar()->setItemAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);

  statusBar()->insertPermanentItem(i18n(" Column: %1 ", QString("     ")),2,0);
  statusBar()->insertPermanentItem(i18n(" Line: %1 ", QString("     ")),1,0);
}


//-----------------------------------------------------------------------------
void KMComposeWin::updateCursorPosition()
{
  int col,line;
  QString temp;
  line = mEditor->currentLine();
  col = mEditor->currentColumn();
  temp = i18n(" Line: %1 ", line+1);
  statusBar()->changeItem(temp,1);
  temp = i18n(" Column: %1 ", col+1);
  statusBar()->changeItem(temp,2);
}


//-----------------------------------------------------------------------------
void KMComposeWin::setupEditor(void)
{
  //QPopupMenu* menu;
  mEditor->setModified(false);
  QFontMetrics fm(mBodyFont);
  mEditor->setTabStopWidth(fm.width(QChar(' ')) * 8);
  //mEditor->setFocusPolicy(QWidget::ClickFocus);

  if (GlobalSettings::self()->wordWrap())
  {
    mEditor->setWordWrap( Q3MultiLineEdit::FixedColumnWidth );
    mEditor->setWrapColumnOrWidth( GlobalSettings::self()->lineWrapWidth() );
  }
  else
  {
    mEditor->setWordWrap( Q3MultiLineEdit::NoWrap );
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
  menu->addSeparator();
  //#endif //BROKEN
  menu->insertItem(i18n("Cut"), this, SLOT(slotCut()));
  menu->insertItem(i18n("Copy"), this, SLOT(slotCopy()));
  menu->insertItem(i18n("Paste"), this, SLOT(slotPaste()));
  menu->insertItem(i18n("Mark All"),this, SLOT(slotMarkAll()));
  menu->addSeparator();
  menu->insertItem(i18n("Find..."), this, SLOT(slotFind()));
  menu->insertItem(i18n("Replace..."), this, SLOT(slotReplace()));
  menu->addSeparator();
  menu->insertItem(i18n("Fixed Font Widths"), this, SLOT(slotUpdateFont()));
  mEditor->installRBPopup(menu);
  */
  updateCursorPosition();
  connect(mEditor,SIGNAL(CursorPositionChanged()),SLOT(updateCursorPosition()));
  connect( mEditor, SIGNAL( currentFontChanged( const QFont & ) ),
          this, SLOT( fontChanged( const QFont & ) ) );
  connect( mEditor, SIGNAL( currentAlignmentChanged( int ) ),
          this, SLOT( alignmentChanged( int ) ) );

}


//-----------------------------------------------------------------------------
static QString cleanedUpHeaderString( const QString & s )
{
  // remove invalid characters from the header strings
  QString res( s );
  res.replace( '\r', "" );
  res.replace( '\n', " " );
  return res.trimmed();
}

//-----------------------------------------------------------------------------
QString KMComposeWin::subject() const
{
  return cleanedUpHeaderString( mEdtSubject->text() );
}

//-----------------------------------------------------------------------------
QString KMComposeWin::to() const
{
  if ( mEdtTo ) {
    return cleanedUpHeaderString( mEdtTo->text() );
  } else if ( mRecipientsEditor ) {
    return mRecipientsEditor->recipientString( Recipient::To );
  } else {
    return QString();
  }
}

//-----------------------------------------------------------------------------
QString KMComposeWin::cc() const
{
  if ( mEdtCc && !mEdtCc->isHidden() ) {
    return cleanedUpHeaderString( mEdtCc->text() );
  } else if ( mRecipientsEditor ) {
    return mRecipientsEditor->recipientString( Recipient::Cc );
  } else {
    return QString();
  }
}

//-----------------------------------------------------------------------------
QString KMComposeWin::bcc() const
{
  if ( mEdtBcc && !mEdtBcc->isHidden() ) {
    return cleanedUpHeaderString( mEdtBcc->text() );
  } else if ( mRecipientsEditor ) {
    return mRecipientsEditor->recipientString( Recipient::Bcc );
  } else {
    return QString();
  }
}

//-----------------------------------------------------------------------------
QString KMComposeWin::from() const
{
  return cleanedUpHeaderString( mEdtFrom->text() );
}

//-----------------------------------------------------------------------------
QString KMComposeWin::replyTo() const
{
  if ( mEdtReplyTo ) {
    return cleanedUpHeaderString( mEdtReplyTo->text() );
  } else {
    return QString();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::verifyWordWrapLengthIsAdequate(const QString &body)
{
  int maxLineLength = 0;
  int curPos;
  int oldPos = 0;
  if (mEditor->Q3MultiLineEdit::wordWrap() == Q3MultiLineEdit::FixedColumnWidth) {
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
void KMComposeWin::decryptOrStripOffCleartextSignature( Q3CString& body )
{
  QList<Kpgp::Block> pgpBlocks;
  QList<QByteArray> nonPgpBlocks;
  if( Kpgp::Module::prepareMessageForDecryption( body,
                                                 pgpBlocks, nonPgpBlocks ) )
  {
    // Only decrypt/strip off the signature if there is only one OpenPGP
    // block in the message
    if( pgpBlocks.count() == 1 )
    {
      Kpgp::Block& block = pgpBlocks.first();
      if( ( block.type() == Kpgp::PgpMessageBlock ) ||
          ( block.type() == Kpgp::ClearsignedBlock ) )
      {
        if( block.type() == Kpgp::PgpMessageBlock )
          // try to decrypt this OpenPGP block
          block.decrypt();
        else
          // strip off the signature
          block.verify();

        body = nonPgpBlocks.first();
        body.append( block.text() );
        body.append( nonPgpBlocks.last() );
      }
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::setMsg(KMMessage* newMsg, bool mayAutoSign,
                          bool allowDecryption, bool isModified)
{
  //assert(newMsg!=0);
  if(!newMsg)
    {
      kDebug(5006) << "KMComposeWin::setMsg() : newMsg == 0!" << endl;
      return;
    }
  mMsg = newMsg;

  mEdtFrom->setText(mMsg->from());
  mEdtReplyTo->setText(mMsg->replyTo());
  if ( mClassicalRecipients ) {
    mEdtTo->setText(mMsg->to());
    mEdtCc->setText(mMsg->cc());
    mEdtBcc->setText(mMsg->bcc());
  } else {
    mRecipientsEditor->setRecipientString( mMsg->to(), Recipient::To );
    mRecipientsEditor->setRecipientString( mMsg->cc(), Recipient::Cc );
    mRecipientsEditor->setRecipientString( mMsg->bcc(), Recipient::Bcc );
  }
  mEdtSubject->setText(mMsg->subject());

  if (!mBtnIdentity->isChecked() && !newMsg->headerField("X-KMail-Identity").isEmpty())
    mId = newMsg->headerField("X-KMail-Identity").trimmed().toUInt();

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

  KPIM::IdentityManager * im = kmkernel->identityManager();

  const KPIM::Identity & ident = im->identityForUoid( mIdentity->currentIdentity() );

  // check for the presence of a DNT header, indicating that MDN's were
  // requested
  QString mdnAddr = newMsg->headerField("Disposition-Notification-To");
  mRequestMDNAction->setChecked( ( !mdnAddr.isEmpty() &&
                                  im->thatIsMe( mdnAddr ) ) ||
                                  GlobalSettings::self()->requestMDN() );

  // check for presence of a priority header, indicating urgent mail:
  mUrgentAction->setChecked( newMsg->isUrgent() );

  if (!ident.isXFaceEnabled() || ident.xface().isEmpty())
    mMsg->removeHeaderField("X-Face");
  else
  {
    QString xface = ident.xface();
    if (!xface.isEmpty())
    {
      int numNL = ( xface.length() - 1 ) / 70;
      for ( int i = numNL; i > 0; --i )
        xface.insert( i*70, "\n\t" );
      mMsg->setHeaderField("X-Face", xface);
    }
  }

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

  mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
  mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

  if ( Kleo::CryptoBackendFactory::instance()->openpgp() || Kleo::CryptoBackendFactory::instance()->smime() ) {
    const bool canOpenPGPSign = Kleo::CryptoBackendFactory::instance()->openpgp()
      && !ident.pgpSigningKey().isEmpty();
    const bool canSMIMESign = Kleo::CryptoBackendFactory::instance()->smime()
      && !ident.smimeSigningKey().isEmpty();

    setEncryption( mLastEncryptActionState );
    setSigning( ( canOpenPGPSign || canSMIMESign ) && mLastSignActionState );
  }

  // "Attach my public key" is only possible if the user uses OpenPGP
  // support and he specified his key:
  mAttachMPK->setEnabled( Kleo::CryptoBackendFactory::instance()->openpgp() &&
			  !ident.pgpEncryptionKey().isEmpty() );

  QString transport = newMsg->headerField("X-KMail-Transport");
  if (!mBtnTransport->isChecked() && !transport.isEmpty())
  {
    for (int i = 0; i < mTransport->count(); i++)
      if (mTransport->itemText(i) == transport)
        mTransport->setCurrentIndex(i);
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

  partNode * root = partNode::fromMessage( mMsg );

  KMail::ObjectTreeParser otp; // all defaults are ok
  otp.parseObjectTree( root );

  KMail::AttachmentCollector ac;
  ac.setDiveIntoEncryptions( true );
  ac.setDiveIntoSignatures( true );
  ac.setDiveIntoMessages( false );

  ac.collectAttachmentsFrom( root );

  for ( std::vector<partNode*>::const_iterator it = ac.attachments().begin() ; it != ac.attachments().end() ; ++it )
    addAttach( new KMMessagePart( (*it)->msgPart() ) );

  mEditor->setText( otp.textualContent() );
  mCharset = otp.textualContentCharset();
  if ( mCharset.isEmpty() )
    mCharset = mMsg->charset();
  if ( mCharset.isEmpty() )
    mCharset = mDefCharset;
  setCharset( mCharset );

  if ( partNode * n = root->findType( DwMime::kTypeText, DwMime::kSubtypeHtml ) )
    if ( partNode * p = n->parentNode() )
      if ( p->hasType( DwMime::kTypeMultipart ) &&
           p->hasSubType( DwMime::kSubtypeAlternative ) )
        if ( mMsg->headerField( "X-KMail-Markup" ) == "true" )
          toggleMarkup( true );

  /* Handle the special case of non-mime mails */
  if ( mMsg->numBodyParts() == 0 && otp.textualContent().isEmpty() ) {
    mCharset=mMsg->charset();
    if ( mCharset.isEmpty() ||  mCharset == "default" )
      mCharset = mDefCharset;

    Q3CString bodyDecoded = mMsg->bodyDecoded();

    if( allowDecryption )
      decryptOrStripOffCleartextSignature( bodyDecoded );

    const QTextCodec *codec = KMMsgBase::codecForName(mCharset);
    if (codec) {
      mEditor->setText(codec->toUnicode(bodyDecoded));
    } else
      mEditor->setText(QString::fromLocal8Bit(bodyDecoded));
  }


#ifdef BROKEN_FOR_OPAQUE_SIGNED_OR_ENCRYPTED_MAILS
  const int num = mMsg->numBodyParts();
  kDebug(5006) << "KMComposeWin::setMsg() mMsg->numBodyParts="
                << mMsg->numBodyParts() << endl;

  if ( num > 0 ) {
    KMMessagePart bodyPart;
    int firstAttachment = 0;

    mMsg->bodyPart(1, &bodyPart);
    if ( bodyPart.typeStr().toLower() == "text" &&
         bodyPart.subtypeStr().toLower() == "html" ) {
      // check whether we are inside a mp/al body part
      partNode *root = partNode::fromMessage( mMsg );
      partNode *node = root->findType( DwMime::kTypeText,
                                       DwMime::kSubtypeHtml );
      if ( node && node->parentNode() &&
           node->parentNode()->hasType( DwMime::kTypeMultipart ) &&
           node->parentNode()->hasSubType( DwMime::kSubtypeAlternative ) ) {
        // we have a mp/al body part with a text and an html body
      kDebug(5006) << "KMComposeWin::setMsg() : text/html found" << endl;
      firstAttachment = 2;
        if ( mMsg->headerField( "X-KMail-Markup" ) == "true" )
          toggleMarkup( true );
      }
      delete root; root = 0;
    }
    if ( firstAttachment == 0 ) {
        mMsg->bodyPart(0, &bodyPart);
        if ( bodyPart.typeStr().toLower() == "text" ) {
          // we have a mp/mx body with a text body
        kDebug(5006) << "KMComposeWin::setMsg() : text/* found" << endl;
          firstAttachment = 1;
        }
      }

    if ( firstAttachment != 0 ) // there's text to show
    {
      mCharset = bodyPart.charset();
      if ( mCharset.isEmpty() || mCharset == "default" )
        mCharset = mDefCharset;

      Q3CString bodyDecoded = bodyPart.bodyDecoded();

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
      //mEditor->insertLine("\n", -1); <-- why ?
    } else mEditor->setText("");
    for( int i = firstAttachment; i < num; ++i )
    {
      KMMessagePart *msgPart = new KMMessagePart;
      mMsg->bodyPart(i, msgPart);
      Q3CString mimeType = msgPart->typeStr().toLower() + '/'
                        + msgPart->subtypeStr().toLower();
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

    Q3CString bodyDecoded = mMsg->bodyDecoded();

    if( allowDecryption )
      decryptOrStripOffCleartextSignature( bodyDecoded );

    const QTextCodec *codec = KMMsgBase::codecForName(mCharset);
    if (codec) {
      mEditor->setText(codec->toUnicode(bodyDecoded));
    } else
      mEditor->setText(QString::fromLocal8Bit(bodyDecoded));
  }

  setCharset(mCharset);
#endif // BROKEN_FOR_OPAQUE_SIGNED_OR_ENCRYPTED_MAILS

  if( (GlobalSettings::self()->autoTextSignature()=="auto") && mayAutoSign ) {
    //
    // Espen 2000-05-16
    // Delay the signature appending. It may start a fileseletor.
    // Not user friendy if this modal fileseletor opens before the
    // composer.
    //
    QTimer::singleShot( 200, this, SLOT(slotAppendSignature()) );
  }
  setModified( isModified );
}


//-----------------------------------------------------------------------------
void KMComposeWin::setFcc( const QString &idString )
{
  // check if the sent-mail folder still exists
  if ( ! idString.isEmpty() && kmkernel->findFolderById( idString ) ) {
    mFcc->setFolder( idString );
  } else {
    mFcc->setFolder( kmkernel->sentFolder() );
  }
}


//-----------------------------------------------------------------------------
bool KMComposeWin::isModified() const
{
  return ( mEditor->isModified() ||
           mEdtFrom->isModified() ||
           ( mEdtReplyTo && mEdtReplyTo->isModified() ) ||
           ( mEdtTo && mEdtTo->isModified() ) ||
           ( mEdtCc && mEdtCc->isModified() ) ||
           ( mEdtBcc && mEdtBcc->isModified() ) ||
           ( mRecipientsEditor && mRecipientsEditor->isModified() ) ||
           mEdtSubject->isModified() ||
           mAtmModified ||
           ( mTransport->lineEdit() && mTransport->lineEdit()->isModified() ) );
}


//-----------------------------------------------------------------------------
void KMComposeWin::setModified( bool modified )
{
  mEditor->setModified( modified );
  if ( !modified ) {
    mEdtFrom->setModified( false );
    if ( mEdtReplyTo ) mEdtReplyTo->setModified( false );
    if ( mEdtTo ) mEdtTo->setModified( false );
    if ( mEdtCc ) mEdtCc->setModified( false );
    if ( mEdtBcc ) mEdtBcc->setModified( false );
    if ( mRecipientsEditor ) mRecipientsEditor->clearModified();
    mEdtSubject->setModified( false );
    mAtmModified =  false ;
    if ( mTransport->lineEdit() )
      mTransport->lineEdit()->setModified( false );
  }
}


//-----------------------------------------------------------------------------
bool KMComposeWin::queryClose ()
{
  if ( !mEditor->checkExternalEditorFinished() )
    return false;
  if (kmkernel->shuttingDown() || kapp->sessionSaving())
    return true;

  if ( isModified() ) {
    const int rc = KMessageBox::warningYesNoCancel(this,
           i18n("Do you want to save the message for later or discard it?"),
           i18n("Close Composer"),
           KGuiItem(i18n("&Save as Draft"), "filesave", QString(),
                  i18n("Save this message in the Drafts folder. It can "
                  "then be edited and sent at a later time.")),
           KStdGuiItem::discard() );
    if (rc == KMessageBox::Cancel)
      return false;
    else if (rc == KMessageBox::Yes) {
      // doSend will close the window. Just return false from this method
      slotSaveDraft();
      return false;
    }
  }
  cleanupAutoSave();
  return true;
}

//-----------------------------------------------------------------------------
bool KMComposeWin::userForgotAttachment()
{
  bool checkForForgottenAttachments = GlobalSettings::self()->showForgottenAttachmentWarning();

  if ( !checkForForgottenAttachments || ( mAtmList.count() > 0 ) )
    return false;


  QStringList attachWordsList = GlobalSettings::self()->attachmentKeywords();

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
  rx.setCaseSensitive( Qt::CaseInsensitive );

  bool gotMatch = false;

  // check whether the subject contains one of the attachment key words
  // unless the message is a reply or a forwarded message
  QString subj = subject();
  gotMatch =    ( KMMessage::stripOffPrefixes( subj ) == subj )
             && ( rx.indexIn( subj ) >= 0 );

  if ( !gotMatch ) {
    // check whether the non-quoted text contains one of the attachment key
    // words
    QRegExp quotationRx ("^([ \\t]*([|>:}#]|[A-Za-z]+>))+");
    for ( int i = 0; i < mEditor->numLines(); ++i ) {
      QString line = mEditor->textLine( i );
      gotMatch =    ( quotationRx.indexIn( line ) < 0 )
                 && ( rx.indexIn( line ) >= 0 );
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
void KMComposeWin::applyChanges( bool dontSignNorEncrypt, bool dontDisable )
{
  kDebug(5006) << "entering KMComposeWin::applyChanges" << endl;

  if(!mMsg) {
    kDebug(5006) << "KMComposeWin::applyChanges() : mMsg == 0!\n" << endl;
    emit applyChangesDone( false );
    return;
  }

  if( mComposer ) {
    kDebug(5006) << "KMComposeWin::applyChanges() : applyChanges called twice"
                  << endl;
    return;
  }

  // Make new job and execute it
  mComposer = new MessageComposer( this );
  connect( mComposer, SIGNAL( done( bool ) ),
           this, SLOT( slotComposerDone( bool ) ) );

  // TODO: Add a cancel button for the following operations?
  // Disable any input to the window, so that we have a snapshot of the
  // composed stuff
  if ( !dontDisable ) setEnabled( false );
  // apply the current state to the composer and let it do it's thing
  mComposer->setDisableBreaking( mDisableBreaking ); // FIXME
  mComposer->applyChanges( dontSignNorEncrypt );
}

void KMComposeWin::slotComposerDone( bool rc )
{
  deleteAll( mComposedMessages );
  mComposedMessages = mComposer->composedMessageList();
  emit applyChangesDone( rc );
  delete mComposer;
  mComposer = 0;

  // re-enable the composewin, the messsage composition is now done
  setEnabled( true );
}

const KPIM::Identity & KMComposeWin::identity() const {
  return kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
}

uint KMComposeWin::identityUid() const {
  return mIdentity->currentIdentity();
}

Kleo::CryptoMessageFormat KMComposeWin::cryptoMessageFormat() const {
  if ( !mCryptoModuleAction )
    return Kleo::AutoFormat;
  return cb2format( mCryptoModuleAction->currentItem() );
}

bool KMComposeWin::encryptToSelf() const {
  return !Kpgp::Module::getKpgp() || Kpgp::Module::getKpgp()->encryptToSelf();
}

bool KMComposeWin::queryExit ()
{
  return true;
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(const KUrl aUrl)
{
  if ( !aUrl.isValid() ) {
    KMessageBox::sorry( this, i18n( "<qt><p>KMail could not recognize the location of the attachment (%1);</p>"
                                 "<p>you have to specify the full path if you wish to attach a file.</p></qt>" ,
                          aUrl.prettyURL() ) );
    return;
  }
  KIO::TransferJob *job = KIO::get(aUrl);
  KIO::Scheduler::scheduleJob( job );
  atmLoadData ld;
  ld.url = aUrl;
  ld.data = QByteArray();
  ld.insert = false;
  if( !aUrl.fileEncoding().isEmpty() )
    ld.encoding = aUrl.fileEncoding().toLatin1();

  mMapAtmLoadData.insert(job, ld);
  connect(job, SIGNAL(result(KJob *)),
          this, SLOT(slotAttachFileResult(KJob *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotAttachFileData(KIO::Job *, const QByteArray &)));
}


//-----------------------------------------------------------------------------
void KMComposeWin::addAttach(const KMMessagePart* msgPart)
{
  mAtmList.append( (KMMessagePart*) msgPart );

  // show the attachment listbox if it does not up to now
  if (mAtmList.count()==1)
  {
    mAtmListView->resize(mAtmListView->width(), 50);
    mAtmListView->show();
    resize(size());
  }

  // add a line in the attachment listbox
  KMAtmListViewItem *lvi = new KMAtmListViewItem( mAtmListView );
  msgPartToItem(msgPart, lvi);
  mAtmItemList.append(lvi);

  // the Attach file job has finished, so the possibly present tmp dir can be deleted now.
  if ( mTempDir != 0 ) {
    delete mTempDir;
    mTempDir = 0;
  }

  connect( lvi, SIGNAL( compress( int ) ),
      this, SLOT( compressAttach( int ) ) );
  connect( lvi, SIGNAL( uncompress( int ) ),
      this, SLOT( uncompressAttach( int ) ) );

  slotUpdateAttachActions();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdateAttachActions()
{
  int selectedCount = 0;
  QList<Q3ListViewItem*>::const_iterator it = mAtmItemList.constBegin();
  while ( it != mAtmItemList.constEnd() ) {
    if ( (*it)->isSelected() ) {
      ++selectedCount;
    }
    ++it;
  }

  mAttachRemoveAction->setEnabled( selectedCount >= 1 );
  mAttachSaveAction->setEnabled( selectedCount == 1 );
  mAttachPropertiesAction->setEnabled( selectedCount == 1 );
}


//-----------------------------------------------------------------------------

QString KMComposeWin::prettyMimeType( const QString& type )
{
  QString t = type.toLower();
  KServiceType::Ptr st = KServiceType::serviceType( t );
  return st ? st->comment() : t;
}

void KMComposeWin::msgPartToItem(const KMMessagePart* msgPart,
                                 KMAtmListViewItem *lvi, bool loadDefaults)
{
  assert(msgPart != 0);

  if (!msgPart->fileName().isEmpty())
    lvi->setText(0, msgPart->fileName());
  else
    lvi->setText(0, msgPart->name());
  lvi->setText(1, KIO::convertSize( msgPart->decodedSize()));
  lvi->setText(2, msgPart->contentTransferEncodingStr());
  lvi->setText(3, prettyMimeType(msgPart->typeStr() + '/' + msgPart->subtypeStr()));

  if ( loadDefaults ) {
    if( canSignEncryptAttachments() ) {
      lvi->enableCryptoCBs( true );
      lvi->setEncrypt( mEncryptAction->isChecked() );
      lvi->setSign(    mSignAction->isChecked() );
    } else {
      lvi->enableCryptoCBs( false );
    }
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::removeAttach(const QString &aUrl)
{
  int idx;
  QList<KMMessagePart*>::const_iterator it;
  for( idx = 0, it = mAtmList.begin();
      (*it) && it != mAtmList.end(); ++it, idx++) {
    if ( (*it)->name() == aUrl ) {
      removeAttach( idx );
      return;
    }
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::removeAttach(int idx)
{
  mAtmModified = true;
  delete mAtmList.takeAt(idx);
  delete mAtmItemList.takeAt(idx);

  if( mAtmList.empty() )
  {
    mAtmListView->hide();
    mAtmListView->setMinimumSize(0, 0);
    resize(size());
  }
}


//-----------------------------------------------------------------------------
bool KMComposeWin::encryptFlagOfAttachment(int idx)
{
  return (int)(mAtmItemList.count()) > idx
    ? static_cast<KMAtmListViewItem*>( mAtmItemList.at( idx ) )->isEncrypt()
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
  if ( mClassicalRecipients ) {
    if ( GlobalSettings::self()->addresseeSelectorType() ==
         GlobalSettings::EnumAddresseeSelectorType::New ) {
      addrBookSelIntoNew();
    } else {
      addrBookSelIntoOld();
    }
  } else {
    kWarning() << "To be implemented: call recipients picker." << endl;
  }
}

void KMComposeWin::addrBookSelIntoOld()
{
  AddressesDialog dlg( this );
  QString txt;
  QStringList lst;

  txt = to();
  if ( !txt.isEmpty() ) {
      lst = EmailAddressTools::splitAddressList( txt );
      dlg.setSelectedTo( lst );
  }

  txt = mEdtCc->text();
  if ( !txt.isEmpty() ) {
      lst = EmailAddressTools::splitAddressList( txt );
      dlg.setSelectedCC( lst );
  }

  txt = mEdtBcc->text();
  if ( !txt.isEmpty() ) {
      lst = EmailAddressTools::splitAddressList( txt );
      dlg.setSelectedBCC( lst );
  }

  dlg.setRecentAddresses( RecentAddresses::self( KMKernel::config() )->kabcAddresses() );

  if (dlg.exec()==QDialog::Rejected) return;

  mEdtTo->setText( dlg.to().join(", ") );
  mEdtTo->setModified( true );

  mEdtCc->setText( dlg.cc().join(", ") );
  mEdtCc->setModified( true );

  mEdtBcc->setText( dlg.bcc().join(", ") );
  mEdtBcc->setModified( true );

  //Make sure BCC field is shown if needed
  if ( !mEdtBcc->text().isEmpty() ) {
    mShowHeaders |= HDR_BCC;
    rethinkFields( false );
  }
}

void KMComposeWin::addrBookSelIntoNew()
{
  AddresseeEmailSelection selection;

  AddresseeSelectorDialog dlg( &selection );

  QString txt;
  QStringList lst;

  txt = to();
  if ( !txt.isEmpty() ) {
      lst = EmailAddressTools::splitAddressList( txt );
      selection.setSelectedTo( lst );
  }

  txt = mEdtCc->text();
  if ( !txt.isEmpty() ) {
      lst = EmailAddressTools::splitAddressList( txt );
      selection.setSelectedCC( lst );
  }

  txt = mEdtBcc->text();
  if ( !txt.isEmpty() ) {
      lst = EmailAddressTools::splitAddressList( txt );
      selection.setSelectedBCC( lst );
  }

  if (dlg.exec()==QDialog::Rejected) return;

  QStringList list = selection.to() + selection.toDistributionLists();
  mEdtTo->setText( list.join(", ") );
  mEdtTo->setModified( true );

  list = selection.cc() + selection.ccDistributionLists();
  mEdtCc->setText( list.join(", ") );
  mEdtCc->setModified( true );

  list = selection.bcc() + selection.bccDistributionLists();
  mEdtBcc->setText( list.join(", ") );
  mEdtBcc->setModified( true );

  //Make sure BCC field is shown if needed
  if ( !mEdtBcc->text().isEmpty() ) {
    mShowHeaders |= HDR_BCC;
    rethinkFields( false );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::setCharset(const QByteArray& aCharset, bool forceDefault)
{
  if ((forceDefault && GlobalSettings::self()->forceReplyCharset()) || aCharset.isEmpty())
    mCharset = mDefCharset;
  else
    mCharset = aCharset.toLower();

  if ( mCharset.isEmpty() || mCharset == "default" )
     mCharset = mDefCharset;

  if (mAutoCharset)
  {
    mEncodingAction->setCurrentItem( 0 );
    return;
  }

  QStringList encodings = mEncodingAction->items();
  int i = 0;
  bool charsetFound = false;
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
      charsetFound = true;
      break;
    }
  }
  if (!aCharset.isEmpty() && !charsetFound) setCharset("", true);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBook()
{
  KAddrBookExternal::openAddressBook(this);
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

  KFileDialog fdlg( QString::null, QString::null, this );
  fdlg.setOperationMode( KFileDialog::Other );
  fdlg.setCaption(i18n("Attach File"));
  fdlg.okButton()->setGuiItem(KGuiItem(i18n("&Attach"),"fileopen"));
  fdlg.setMode(KFile::Files);
  fdlg.exec();
  KUrl::List files = fdlg.selectedURLs();

  for (KUrl::List::Iterator it = files.begin(); it != files.end(); ++it)
    addAttach(*it);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFileData(KIO::Job *job, const QByteArray &data)
{
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.find(job);
  assert(it != mMapAtmLoadData.end());
  QBuffer buff(&(*it).data);
  buff.open(QIODevice::WriteOnly | QIODevice::Append);
  buff.write(data.data(), data.size());
  buff.close();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFileResult(KJob *job)
{
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.find(static_cast<KIO::Job*>(job));
  assert(it != mMapAtmLoadData.end());
  if (job->error())
  {
    mMapAtmLoadData.erase(it);
    static_cast<KIO::Job*>(job)->showErrorDialog();
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
    mMapAtmLoadData.erase(it);
    return;
  }
  const Q3CString partCharset = (*it).url.fileEncoding().isEmpty()
                             ? mCharset
                             : Q3CString((*it).url.fileEncoding().toLatin1());

  KMMessagePart* msgPart;

  KCursorSaver busy(KBusyPtr::busy());
  QString name( (*it).url.fileName() );
  // ask the job for the mime type of the file
  QString mimeType = static_cast<KIO::MimetypeJob*>(job)->mimetype();

  if ( name.isEmpty() ) {
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
        int i = ext.lastIndexOf( '.' );
        if( i == -1 )
          ext.prepend( '.' );
        else if( i > 0 )
          ext = ext.mid( i );
      }
      name = QString("unknown") += ext;
    }
  }

  name.truncate( 256 ); // is this needed?

  Q3CString encoding = KMMsgBase::autoDetectCharset(partCharset,
    KMMessage::preferredCharsets(), name);
  if (encoding.isEmpty()) encoding = "utf-8";

  Q3CString encName;
  if ( GlobalSettings::self()->outlookCompatibleAttachments() )
    encName = KMMsgBase::encodeRFC2047String( name, encoding );
  else
    encName = KMMsgBase::encodeRFC2231String( name, encoding );
  bool RFC2231encoded = false;
  if ( !GlobalSettings::self()->outlookCompatibleAttachments() )
    RFC2231encoded = name != QString( encName );

  // create message part
  msgPart = new KMMessagePart;
  msgPart->setName(name);
  QList<int> allowedCTEs;
  msgPart->setBodyAndGuessCte((*it).data, allowedCTEs,
                              !kmkernel->msgSender()->sendQuotedPrintable());
  kDebug(5006) << "autodetected cte: " << msgPart->cteStr() << endl;
  int slash = mimeType.indexOf( '/' );
  if( slash == -1 )
    slash = mimeType.length();
  msgPart->setTypeStr( mimeType.left( slash ).toLatin1() );
  msgPart->setSubtypeStr( mimeType.mid( slash + 1 ).toLatin1() );
  msgPart->setContentDisposition(Q3CString("attachment;\n\tfilename")
    + ((RFC2231encoded) ? "*" : "") +  "=\"" + encName + "\"");

  mMapAtmLoadData.erase(it);

  msgPart->setCharset(partCharset);

  // show message part dialog, if not configured away (default):
  KConfigGroup composer(KMKernel::config(), "Composer");
  if ( GlobalSettings::self()->showMessagePartDialogOnAttach() ) {
    const KCursorSaver saver( Qt::ArrowCursor );
    KMMsgPartDialogCompat dlg;
    int encodings = 0;
    for ( QList<int>::ConstIterator it = allowedCTEs.begin() ;
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
  mAtmModified = true;
  if (msgPart->typeStr().toLower() != "text") msgPart->setCharset(Q3CString());

  // add the new attachment to the list
  addAttach(msgPart);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertFile()
{
  KFileDialog fdlg( QString::null, QString::null, this );
  fdlg.setOperationMode( KFileDialog::Opening );
  fdlg.okButton()->setText(i18n("&Insert"));
  fdlg.setCaption(i18n("Insert File"));
  KComboBox *combo = new KComboBox( this );
  combo->addItems( KMMsgBase::supportedEncodings(false) );
  fdlg.toolBar()->addWidget( combo );
  for (int i = 0; i < combo->count(); i++)
    if (KGlobal::charsets()->codecForName(KGlobal::charsets()->
      encodingForName(combo->itemText(i)))
      == QTextCodec::codecForLocale()) combo->setCurrentIndex(i);
  if (!fdlg.exec()) return;

  KUrl u = fdlg.selectedURL();
  mRecentAction->addUrl(u);
  // Prevent race condition updating list when multiple composers are open
  {
    KConfig *config = KMKernel::config();
    KConfigGroup group( config, "Composer" );
    QString encoding = KGlobal::charsets()->encodingForName(combo->currentText()).toLatin1();
    QStringList urls = group.readEntry( "recent-urls" , QStringList() );
    QStringList encodings = group.readEntry( "recent-encodings" , QStringList() );
    // Prevent config file from growing without bound
    // Would be nicer to get this constant from KRecentFilesAction
    uint mMaxRecentFiles = 30;
    while ((uint)urls.count() > mMaxRecentFiles)
      urls.removeLast();
    while ((uint)encodings.count() > mMaxRecentFiles)
      encodings.removeLast();
    // sanity check
    if (urls.count() != encodings.count()) {
      urls.clear();
      encodings.clear();
    }
    urls.prepend( u.prettyURL() );
    encodings.prepend( encoding );
    group.writeEntry( "recent-urls", urls );
    group.writeEntry( "recent-encodings", encodings );
    mRecentAction->saveEntries( config );
  }
  slotInsertRecentFile(u);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertRecentFile(const KUrl& u)
{
  if (u.fileName().isEmpty()) return;

  KIO::Job *job = KIO::get(u);
  atmLoadData ld;
  ld.url = u;
  ld.data = QByteArray();
  ld.insert = true;
  // Get the encoding previously used when inserting this file
  {
    KConfig *config = KMKernel::config();
    KConfigGroup group( config, "Composer" );
    QStringList urls = group.readEntry( "recent-urls" , QStringList() );
    QStringList encodings = group.readEntry( "recent-encodings" , QStringList() );
    int index = urls.indexOf( u.prettyURL() );
    if (index != -1) {
      QString encoding = encodings[ index ];
      ld.encoding = encoding.toLatin1();
    }
  }
  mMapAtmLoadData.insert(job, ld);
  connect(job, SIGNAL(result(KJob *)),
          this, SLOT(slotAttachFileResult(KJob *)));
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
    currentText() ).toLatin1();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotSelectCryptoModule( bool init )
{
  if ( !init ) {
    setModified( true );
  }
  if( canSignEncryptAttachments() ) {
    // if the encrypt/sign columns are hidden then show them
    if( 0 == mAtmListView->columnWidth( mAtmColEncrypt ) ) {
      // set/unset signing/encryption for all attachments according to the
      // state of the global sign/encrypt action
      if( !mAtmList.isEmpty() ) {
        QList<Q3ListViewItem*>::const_iterator it;
        for( it = mAtmItemList.constBegin(); it != mAtmItemList.constBegin(); ++it ) {
          KMAtmListViewItem* lvi = static_cast<KMAtmListViewItem*> (*it);
          if ( lvi ) {
            lvi->setSign( mSignAction->isChecked() );
            lvi->setEncrypt( mEncryptAction->isChecked() );
          }
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
      QList<Q3ListViewItem*>::const_iterator it;
      for( it = mAtmItemList.constBegin(); it != mAtmItemList.constBegin(); ++it ) {
        KMAtmListViewItem* lvi = static_cast<KMAtmListViewItem*> (*it);
        if ( lvi ) {
          lvi->enableCryptoCBs( true );
        }
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
      QList<Q3ListViewItem*>::const_iterator it;
      for( it = mAtmItemList.constBegin(); it != mAtmItemList.constBegin(); ++it ) {
        KMAtmListViewItem* lvi = static_cast<KMAtmListViewItem*> (*it);
        if ( lvi ) {
          lvi->enableCryptoCBs( false );
        }
      }
    }
  }
}

static void showExportError( QWidget * w, const GpgME::Error & err ) {
  assert( err );
  const QString msg = i18n("<qt><p>An error occurred while trying to export "
			   "the key from the backend:</p>"
			   "<p><b>%1</b></p></qt>",
      QString::fromLocal8Bit( err.asString() ) );
  KMessageBox::error( w, msg, i18n("Key Export Failed") );
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertMyPublicKey()
{
  // get PGP user id for the chosen identity
  mFingerprint =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() ).pgpEncryptionKey();
  if ( !mFingerprint.isEmpty() )
    startPublicKeyExport();
}

void KMComposeWin::startPublicKeyExport() {
  if ( mFingerprint.isEmpty() || !Kleo::CryptoBackendFactory::instance()->openpgp() )
    return;
  Kleo::ExportJob * job = Kleo::CryptoBackendFactory::instance()->openpgp()->publicKeyExportJob( true );
  assert( job );

  connect( job, SIGNAL(result(const GpgME::Error&,const QByteArray&)),
	   this, SLOT(slotPublicKeyExportResult(const GpgME::Error&,const QByteArray&)) );

  const GpgME::Error err = job->start( QStringList( mFingerprint ) );
  if ( err )
    showExportError( this, err );
  else
    (void)new Kleo::ProgressDialog( job, i18n("Exporting key..."), this );
}

void KMComposeWin::slotPublicKeyExportResult( const GpgME::Error & err, const QByteArray & keydata ) {
  if ( err ) {
    showExportError( this, err );
    return;
  }

  // create message part
  KMMessagePart * msgPart = new KMMessagePart();
  msgPart->setName( i18n("OpenPGP key 0x%1", mFingerprint ) );
  msgPart->setTypeStr("application");
  msgPart->setSubtypeStr("pgp-keys");
  QList<int> dummy;
  msgPart->setBodyAndGuessCte(keydata, dummy, false);
  msgPart->setContentDisposition( "attachment;\n\tfilename=0x" + Q3CString( mFingerprint.toLatin1() ) + ".asc" );

  // add the new attachment to the list
  addAttach(msgPart);
  rethinkFields(); //work around initial-size bug in Qt-1.32
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertPublicKey()
{
  Kleo::KeySelectionDialog dlg( i18n("Attach Public OpenPGP Key"),
                                i18n("Select the public key which should "
                                     "be attached."),
				std::vector<GpgME::Key>(),
				Kleo::KeySelectionDialog::PublicKeys|Kleo::KeySelectionDialog::OpenPGPKeys,
				false /* no multi selection */,
				false /* no remember choice box */,
				this, "attach public key selection dialog" );
  if ( dlg.exec() != QDialog::Accepted )
    return;

  mFingerprint = dlg.fingerprint();
  startPublicKeyExport();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachPopupMenu(Q3ListViewItem *, const QPoint &, int)
{
  if (!mAttachMenu)
  {
     mAttachMenu = new QMenu(this);

     mOpenId = mAttachMenu->addAction(i18nc("to open", "Open"), this,
                             SLOT(slotAttachOpen()));
     mViewId = mAttachMenu->addAction(i18nc("to view", "View"), this,
                             SLOT(slotAttachView()));
     mRemoveId = mAttachMenu->addAction(i18n("Remove"), this, SLOT(slotAttachRemove()));
     mSaveAsId = mAttachMenu->addAction( SmallIconSet("filesaveas"), i18n("Save As..."), this,
                                          SLOT( slotAttachSave() ) );
     mPropertiesId = mAttachMenu->addAction( i18n("Properties"), this,
                                              SLOT( slotAttachProperties() ) );
     mAttachMenu->addSeparator();
     mAttachMenu->addAction(i18n("Add Attachment..."), this, SLOT(slotAttachFile()));
  }

  int selectedCount = 0;
  QList<Q3ListViewItem*>::const_iterator it = mAtmItemList.constBegin();
  while ( it != mAtmItemList.constEnd() ) {
    if ( (*it)->isSelected() ) {
      ++selectedCount;
    }
    ++it;
  }

  mOpenId->setEnabled( selectedCount > 0 );
  mViewId->setEnabled( selectedCount > 0 );
  mRemoveId->setEnabled( selectedCount > 0 );
  mSaveAsId->setEnabled( selectedCount == 1 );
  mPropertiesId->setEnabled( selectedCount == 1 );

  mAttachMenu->popup(QCursor::pos());
}

//-----------------------------------------------------------------------------
int KMComposeWin::currentAttachmentNum()
{
  return mAtmItemList.indexOf( mAtmListView->currentItem() );
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
  if( canSignEncryptAttachments() && listItem ) {
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
    mAtmModified = true;
    // values may have changed, so recreate the listbox line
    if( listItem ) {
      msgPartToItem(msgPart, listItem);
      if( canSignEncryptAttachments() ) {
        listItem->setSign(    dlg.isSigned()    );
        listItem->setEncrypt( dlg.isEncrypted() );
      }
    }
  }
  if (msgPart->typeStr().toLower() != "text") msgPart->setCharset(Q3CString());
}

//-----------------------------------------------------------------------------
void KMComposeWin::compressAttach( int idx )
{
  if (idx < 0) return;

  int i;
  for ( i = 0; i < mAtmItemList.count(); ++i )
      if ( mAtmItemList.at( i )->itemPos() == idx )
          break;

  if ( i > mAtmItemList.count() )
      return;

  KMMessagePart* msgPart;
  msgPart = mAtmList.at( i );
  QByteArray array;
  QBuffer dev( &array );
  KZip zip( &dev );
  QByteArray decoded = msgPart->bodyDecodedBinary();
  if ( ! zip.open( QIODevice::WriteOnly ) ) {
    KMessageBox::sorry(0, i18n("KMail could not compress the file.") );
    static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) )->setCompress( false );
    return;
  }

  zip.setCompression( KZip::DeflateCompression );
  if ( ! zip.writeFile( msgPart->name(), "", "", decoded.data(),decoded.size())) {
    KMessageBox::sorry(0, i18n("KMail could not compress the file.") );
    static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) )->setCompress( false );
    return;
  }
  zip.close();
  if ( array.size() >= decoded.size() ) {
    if ( KMessageBox::questionYesNo( this, i18n("The compressed file is larger "
        "than the original. Do you want to keep the original one?" ), QString(), i18n("Keep"), i18n("Compress") )
         == KMessageBox::Yes ) {
      static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) )->setCompress( false );
      return;
    }
  }
  static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) )->setUncompressedCodec(
      msgPart->cteStr() );

  msgPart->setCteStr( "base64" );
  msgPart->setBodyEncodedBinary( array );
  QString name = msgPart->name() + ".zip";

  msgPart->setName( name );

  Q3CString cDisp = "attachment;";
  Q3CString encoding = KMMsgBase::autoDetectCharset( msgPart->charset(),
    KMMessage::preferredCharsets(), name );
  kDebug(5006) << "encoding: " << encoding << endl;
  if ( encoding.isEmpty() ) encoding = "utf-8";
  kDebug(5006) << "encoding after: " << encoding << endl;
  Q3CString encName;
  if ( GlobalSettings::self()->outlookCompatibleAttachments() )
    encName = KMMsgBase::encodeRFC2047String( name, encoding );
  else
    encName = KMMsgBase::encodeRFC2231String( name, encoding );

  cDisp += "\n\tfilename";
  if ( name != QString( encName ) )
    cDisp += "*=" + encName;
  else
    cDisp += "=\"" + encName + '"';
  msgPart->setContentDisposition( cDisp );

  static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) )->setUncompressedMimeType(
      msgPart->typeStr(), msgPart->subtypeStr() );
  msgPart->setTypeStr( "application" );
  msgPart->setSubtypeStr( "x-zip" );

  KMAtmListViewItem* listItem = static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) );
  msgPartToItem( msgPart, listItem, false );
}

//-----------------------------------------------------------------------------

void KMComposeWin::uncompressAttach( int idx )
{
  if (idx < 0) return;

  int i;
  for ( i = 0; i < mAtmItemList.count(); ++i )
      if ( mAtmItemList.at( i )->itemPos() == idx )
          break;

  if ( i > mAtmItemList.count() )
      return;

  KMMessagePart* msgPart;
  msgPart = mAtmList.at( i );

  QByteArray ba = msgPart->bodyDecodedBinary();
  QBuffer dev( &ba );
  KZip zip( &dev );
  QByteArray decoded;

  decoded = msgPart->bodyDecodedBinary();
  if ( ! zip.open( QIODevice::ReadOnly ) ) {
    KMessageBox::sorry(0, i18n("KMail could not uncompress the file.") );
    static_cast<KMAtmListViewItem *>( mAtmItemList.at( i ) )->setCompress( true );
    return;
  }
  const KArchiveDirectory *dir = zip.directory();

  KZipFileEntry *entry;
  if ( dir->entries().count() != 1 ) {
    KMessageBox::sorry(0, i18n("KMail could not uncompress the file.") );
    static_cast<KMAtmListViewItem *>( mAtmItemList.at( i ) )->setCompress( true );
    return;
  }
  entry = (KZipFileEntry*)dir->entry( dir->entries()[0] );

  msgPart->setCteStr(
      static_cast<KMAtmListViewItem*>( mAtmItemList.at(i) )->uncompressedCodec() );

  msgPart->setBodyEncodedBinary( entry->data() );
  QString name = entry->name();
  msgPart->setName( name );

  zip.close();

  Q3CString cDisp = "attachment;";
  Q3CString encoding = KMMsgBase::autoDetectCharset( msgPart->charset(),
    KMMessage::preferredCharsets(), name );
  if ( encoding.isEmpty() ) encoding = "utf-8";

  Q3CString encName;
  if ( GlobalSettings::self()->outlookCompatibleAttachments() )
    encName = KMMsgBase::encodeRFC2047String( name, encoding );
  else
    encName = KMMsgBase::encodeRFC2231String( name, encoding );

  cDisp += "\n\tfilename";
  if ( name != QString( encName ) )
    cDisp += "*=" + encName;
  else
    cDisp += "=\"" + encName + '"';
  msgPart->setContentDisposition( cDisp );

  Q3CString type, subtype;
  static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) )->uncompressedMimeType( type,
        subtype );

  msgPart->setTypeStr( type );
  msgPart->setSubtypeStr( subtype );

  KMAtmListViewItem* listItem = static_cast<KMAtmListViewItem*>(mAtmItemList.at( i ));
  msgPartToItem( msgPart, listItem, false );
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachView()
{
  int i = 0;
  QList<Q3ListViewItem*>::const_iterator it;
  for ( it = mAtmItemList.constBegin();
        it != mAtmItemList.constEnd(); ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      viewAttach( i );
    }
  }
}
//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachOpen()
{
  int i = 0;
  QList<Q3ListViewItem*>::const_iterator it;
  for ( it = mAtmItemList.constBegin();
        it != mAtmItemList.constEnd(); ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      openAttach( i );
    }
  }
}

//-----------------------------------------------------------------------------
bool KMComposeWin::inlineSigningEncryptionSelected() {
  if ( !mSignAction->isChecked() && !mEncryptAction->isChecked() )
    return false;
  return cryptoMessageFormat() == Kleo::InlineOpenPGPFormat;
}

//-----------------------------------------------------------------------------
void KMComposeWin::viewAttach( int index )
{
  QString pname;
  KMMessagePart* msgPart;
  msgPart = mAtmList.at(index);
  pname = msgPart->name().trimmed();
  if (pname.isEmpty()) pname=msgPart->contentDescription();
  if (pname.isEmpty()) pname="unnamed";

  KTempFile* atmTempFile = new KTempFile();
  mAtmTempList.append( atmTempFile );
  atmTempFile->setAutoDelete( true );
  KPIM::kByteArrayToFile(msgPart->bodyDecodedBinary(), atmTempFile->name(), false, false,
    false);
  KMReaderMainWin *win = new KMReaderMainWin(msgPart, false,
    atmTempFile->name(), pname, mCharset );
  win->show();
}

//-----------------------------------------------------------------------------
void KMComposeWin::openAttach( int index )
{
  KMMessagePart* msgPart = mAtmList.at(index);
  const QString contentTypeStr =
    ( msgPart->typeStr() + '/' + msgPart->subtypeStr() ).toLower();

  KMimeType::Ptr mimetype;
  mimetype = KMimeType::mimeType( contentTypeStr );

  KTempFile* atmTempFile = new KTempFile();
  mAtmTempList.append( atmTempFile );
  const bool autoDelete = true;
  atmTempFile->setAutoDelete( autoDelete );

  KUrl url;
  url.setPath( atmTempFile->name() );

  KPIM::kByteArrayToFile( msgPart->bodyDecodedBinary(), atmTempFile->name(), false, false,
    false );
  if ( ::chmod( QFile::encodeName( atmTempFile->name() ), S_IRUSR ) != 0) {
    QFile::remove(url.path());
    return;
  }

  KService::Ptr offer =
    KServiceTypeProfile::preferredService( mimetype->name(), "Application" );

  if ( !offer || mimetype->name() == "application/octet-stream" ) {
    if ( ( !KRun::displayOpenWithDialog( url, autoDelete ) ) && autoDelete ) {
      QFile::remove(url.path());
    }
  }
  else {
    if ( ( !KRun::run( *offer, url, autoDelete ) ) && autoDelete ) {
        QFile::remove( url.path() );
    }
  }
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

  KUrl url = KFileDialog::getSaveURL(QString(), QString(), 0, i18n("Save Attachment As"));

  if( url.isEmpty() )
    return;

  kmkernel->byteArrayToRemoteFile(msgPart->bodyDecodedBinary(), url);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachRemove()
{
  bool attachmentRemoved = false;
  int i = 0;
  QList<Q3ListViewItem*>::iterator it = mAtmItemList.begin();
  while ( it != mAtmItemList.end() ) {
    if ( (*it)->isSelected() ) {
      removeAttach( i );
      it = mAtmItemList.begin();
      attachmentRemoved = true;
    }
    else {
      ++it;
      ++i;
    }
  }

  if ( attachmentRemoved ) {
    setModified( true );
    slotUpdateAttachActions();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotFind()
{
  mEditor->search();
}

void KMComposeWin::slotSearchAgain()
{
  mEditor->repeatSearch();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotReplace()
{
  mEditor->replace();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdateFont()
{
  kDebug() << "KMComposeWin::slotUpdateFont " << endl;
  if ( ! mFixedFontAction ) {
    return;
  }
  mEditor->setFont( mFixedFontAction->isChecked() ? mFixedFont : mBodyFont );
}

QString KMComposeWin::quotePrefixName() const
{
    if ( !msg() )
        return QString();

    int languageNr = GlobalSettings::self()->replyCurrentLanguage();
    ReplyPhrases replyPhrases( QString::number(languageNr) );
    replyPhrases.readConfig();
    QString quotePrefix = msg()->formatString(
                 replyPhrases.indentPrefix() );

    quotePrefix = msg()->formatString(quotePrefix);
    return quotePrefix;
}

void KMComposeWin::slotPasteAsQuotation()
{
    if( mEditor->hasFocus() && msg() )
    {
        QString s = QApplication::clipboard()->text();
        if (!s.isEmpty())
            mEditor->insert(addQuotesToText(s));
    }
}

void KMComposeWin::slotPasteAsAttachment()
{
  KUrl url( QApplication::clipboard()->text( QClipboard::Clipboard ) );
  if ( url.isValid() ) {
    addAttach(QApplication::clipboard()->text( QClipboard::Clipboard ) );
    return;
  }

  if ( QApplication::clipboard()->image().isNull() )  {
    bool ok;
    QString attName = KInputDialog::getText( "KMail", i18n("Name of the attachment:"), QString(), &ok, this );
    if ( !ok )
      return;
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName(attName);
    QList<int> dummy;
    msgPart->setBodyAndGuessCte(Q3CString(QApplication::clipboard()->text().toLatin1()), dummy,
                                kmkernel->msgSender()->sendQuotedPrintable());
    addAttach(msgPart);
  }
  else
    addImageFromClipboard();

}

void KMComposeWin::slotAddQuotes()
{
    if( mEditor->hasFocus() && msg() )
    {
        // TODO: I think this is backwards.
        // i.e, if no region is marked then add quotes to every line
        // else add quotes only on the lines that are marked.

        if ( mEditor->hasMarkedText() ) {
            QString s = mEditor->markedText();
            if(!s.isEmpty())
                mEditor->insert(addQuotesToText(s));
        } else {
            int l =  mEditor->currentLine();
            int c =  mEditor->currentColumn();
            QString s =  mEditor->textLine(l);
            s.prepend(quotePrefixName());
            mEditor->insertLine(s,l);
            mEditor->removeLine(l+1);
            mEditor->setCursorPosition(l,c+2);
        }
    }
}

QString KMComposeWin::addQuotesToText(const QString &inputText)
{
    QString answer = QString( inputText );
    QString indentStr = quotePrefixName();
    answer.replace( '\n', '\n' + indentStr);
    answer.prepend( indentStr );
    answer += '\n';
    return KMMessage::smartQuote( answer, GlobalSettings::self()->lineWrapWidth() );
}

QString KMComposeWin::removeQuotesFromText(const QString &inputText)
{
    QString s = inputText;

    // remove first leading quote
    QString quotePrefix = '^' + quotePrefixName();
    QRegExp rx(quotePrefix);
    s.remove(rx);

    // now remove all remaining leading quotes
    quotePrefix = '\n' + quotePrefixName();
    QRegExp srx(quotePrefix);
    s.replace(srx, "\n");

    return s;
}

void KMComposeWin::slotRemoveQuotes()
{
    if( mEditor->hasFocus() && msg() )
    {
        // TODO: I think this is backwards.
        // i.e, if no region is marked then remove quotes from every line
        // else remove quotes only on the lines that are marked.

        if ( mEditor->hasMarkedText() ) {
            QString s = mEditor->markedText();
            mEditor->insert(removeQuotesFromText(s));
        } else {
            int l = mEditor->currentLine();
            int c = mEditor->currentColumn();
            QString s = mEditor->textLine(l);
            mEditor->insertLine(removeQuotesFromText(s),l);
            mEditor->removeLine(l+1);
            mEditor->setCursorPosition(l,c-2);
        }
    }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotUndo()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if ( ::qobject_cast<KEdit*>(fw) )
      static_cast<Q3MultiLineEdit*>(fw)->undo();
  else if (::qobject_cast<QLineEdit*>(fw))
      static_cast<QLineEdit*>(fw)->undo();
}

void KMComposeWin::slotRedo()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (::qobject_cast<KEdit*>(fw))
      static_cast<KEdit*>(fw)->redo();
  else if (::qobject_cast<QLineEdit*>(fw))
      static_cast<QLineEdit*>(fw)->redo();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCut()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (::qobject_cast<KEdit*>(fw))
      static_cast<KEdit*>(fw)->cut();
  else if (::qobject_cast<QLineEdit*>(fw))
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

  QKeyEvent k(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
  kapp->notify(fw, &k);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotPaste()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if ( ! QApplication::clipboard()->image().isNull() )  {
    addImageFromClipboard();
  }
  else {

#ifdef KeyPress
#undef KeyPress
#endif

    QKeyEvent k(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
    kapp->notify(fw, &k);
  }

}


//-----------------------------------------------------------------------------
void KMComposeWin::slotMarkAll()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  if (::qobject_cast<QLineEdit*>(fw))
      static_cast<QLineEdit*>(fw)->selectAll();
  else if (::qobject_cast<KEdit*>(fw))
      static_cast<KEdit*>(fw)->selectAll();
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotClose()
{
  #warning "Port me: make sure to set/unset the  Qt::WA_DeleteOnClose flag to match the old behavior of close(false)"
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
       setCaption('(' + i18n("unnamed") + ')');
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
  if ( setByUser )
    setModified( true );
  if ( !mEncryptAction->isEnabled() )
    encrypt = false;
  // check if the user wants to encrypt messages to himself and if he defined
  // an encryption key for the current identity
  else if ( encrypt && encryptToSelf() && !mLastIdentityHasEncryptionKey ) {
    if ( setByUser )
      KMessageBox::sorry( this,
                          i18n("<qt><p>You have requested that messages be "
			       "encrypted to yourself, but the currently selected "
			       "identity does not define an (OpenPGP or S/MIME) "
			       "encryption key to use for this.</p>"
                               "<p>Please select the key(s) to use "
                               "in the identity configuration.</p>"
                               "</qt>"),
                          i18n("Undefined Encryption Key") );
    encrypt = false;
  }

  // make sure the mEncryptAction is in the right state
  mEncryptAction->setChecked( encrypt );

  // show the appropriate icon
  if ( encrypt )
    mEncryptAction->setIcon( KIcon("encrypted") );
  else
    mEncryptAction->setIcon( KIcon("decrypted") );

  // mark the attachments for (no) encryption
  if ( canSignEncryptAttachments() ) {
    QList<Q3ListViewItem*>::const_iterator it;
    for (it = mAtmItemList.constBegin(); it != mAtmItemList.constEnd(); ++it ) {
      KMAtmListViewItem* entry = static_cast<KMAtmListViewItem*> (*it);
      if ( entry )
        entry->setEncrypt( encrypt );
    }
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
  if ( setByUser )
    setModified( true );
  if ( !mSignAction->isEnabled() )
    sign = false;

  // check if the user defined a signing key for the current identity
  if ( sign && !mLastIdentityHasSigningKey ) {
    if ( setByUser )
      KMessageBox::sorry( this,
                          i18n("<qt><p>In order to be able to sign "
                               "this message you first have to "
                               "define the (OpenPGP or S/MIME) signing key "
			       "to use.</p>"
                               "<p>Please select the key to use "
                               "in the identity configuration.</p>"
                               "</qt>"),
                          i18n("Undefined Signing Key") );
    sign = false;
  }

  // make sure the mSignAction is in the right state
  mSignAction->setChecked( sign );

  // mark the attachments for (no) signing
  if ( canSignEncryptAttachments() ) {
    QList<Q3ListViewItem*>::const_iterator it;
    for (it = mAtmItemList.constBegin(); it != mAtmItemList.constEnd(); ++it ) {
      KMAtmListViewItem* entry = static_cast<KMAtmListViewItem*> (*it);
      if ( entry )
        entry->setSign( sign );
    }
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotWordWrapToggled(bool on)
{
  if (on)
  {
    mEditor->setWordWrap( Q3MultiLineEdit::FixedColumnWidth );
    mEditor->setWrapColumnOrWidth( GlobalSettings::self()->lineWrapWidth() );
  }
  else
  {
    mEditor->setWordWrap( Q3MultiLineEdit::NoWrap );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotPrint()
{
  mMessageWasModified = isModified();
  connect( this, SIGNAL( applyChangesDone( bool ) ),
           this, SLOT( slotContinuePrint( bool ) ) );
  applyChanges( true );
}

void KMComposeWin::slotContinuePrint( bool rc )
{
  disconnect( this, SIGNAL( applyChangesDone( bool ) ),
              this, SLOT( slotContinuePrint( bool ) ) );

  if( rc ) {
    if ( mComposedMessages.isEmpty() ) {
      kDebug(5006) << "Composing the message failed." << endl;
      return;
    }
    KMCommand *command = new KMPrintCommand( this, mComposedMessages.first() );
    command->start();
    setModified( mMessageWasModified );
  }
}

//----------------------------------------------------------------------------
bool KMComposeWin::validateAddresses( QWidget * parent, const QString & addresses )
{
  QString brokenAddress;
  EmailAddressTools::EmailParseResult errorCode = KMMessage::isValidEmailAddressList( KMMessage::expandAliases( addresses ), brokenAddress );
  if ( !( errorCode == EmailAddressTools::AddressOk || errorCode == EmailAddressTools::AddressEmpty ) ) {
    QString errorMsg( "<qt><p><b>" + brokenAddress +
                      "</b></p><p>" + EmailAddressTools::emailParseResultToString( errorCode ) +
                      "</p></qt>" );
    KMessageBox::sorry( parent, errorMsg, i18n("Invalid Email Address") );
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void KMComposeWin::doSend( KMail::MessageSender::SendMethod method, bool saveInDrafts)
{
  if ( method != KMail::MessageSender::SendLater && kmkernel->isOffline() ) {
    KMessageBox::information( this,
                              i18n("KMail is currently in offline mode. "
                                   "Your messages will be kept in the outbox until you go online."),
                              i18n("Online/Offline"), "kmailIsOffline" );
    mSendMethod = KMail::MessageSender::SendLater;
  } else {
    mSendMethod = method;
  }
  mSaveInDrafts = saveInDrafts;

  if (!saveInDrafts)
  {
    if ( EmailAddressTools::firstEmailAddress( from() ).isEmpty() ) {
      if ( !( mShowHeaders & HDR_FROM ) ) {
        mShowHeaders |= HDR_FROM;
        rethinkFields( false );
      }
      mEdtFrom->setFocus();
      KMessageBox::sorry( this,
                          i18n("You must enter your email address in the "
                               "From: field. You should also set your email "
                               "address for all identities, so that you do "
                               "not have to enter it for each message.") );
      return;
    }
    if (to().isEmpty() && cc().isEmpty() && bcc().isEmpty())
    {
      if ( mEdtTo ) mEdtTo->setFocus();
      KMessageBox::information( this,
                                i18n("You must specify at least one receiver,"
                                     "either in the To: field or as CC or as BCC.") );
      return;
    }

    // Validate the To:, CC: and BCC fields
    if ( !validateAddresses( this, to().trimmed() ) ) {
      return;
    }

    if ( !validateAddresses( this, cc().trimmed() ) ) {
      return;
    }

    if ( !validateAddresses( this, bcc().trimmed() ) ) {
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
  if ((mTransport->currentText() != mTransport->itemText(0)) ||
      (!hf.isEmpty() && (hf != mTransport->itemText(0))))
    mMsg->setHeaderField("X-KMail-Transport", mTransport->currentText());

  mDisableBreaking = saveInDrafts;

  const bool neverEncrypt = ( saveInDrafts && GlobalSettings::self()->neverEncryptDrafts() )
                           || mSigningAndEncryptionExplicitlyDisabled;
  connect( this, SIGNAL( applyChangesDone( bool ) ),
           SLOT( slotContinueDoSend( bool ) ) );

  if ( mEditor->textFormat() == Qt::RichText )
    mMsg->setHeaderField( "X-KMail-Markup", "true" );
  else
    mMsg->removeHeaderField( "X-KMail-Markup" );
  if ( mEditor->textFormat() == Qt::RichText && inlineSigningEncryptionSelected() ) {
    QString keepBtnText = mEncryptAction->isChecked() ?
      mSignAction->isChecked() ? i18n( "&Keep markup, do not sign/encrypt" )
                               : i18n( "&Keep markup, do not encrypt" )
      : i18n( "&Keep markup, do not sign" );
    QString yesBtnText = mEncryptAction->isChecked() ?
      mSignAction->isChecked() ? i18n("Sign/Encrypt (delete markup)")
      : i18n( "Encrypt (delete markup)" )
      : i18n( "Sign (delete markup)" );
    int ret = KMessageBox::warningYesNoCancel(this,
                                      i18n("<qt><p>Inline signing/encrypting of HTML messages is not possible;</p>"
                                           "<p>do you want to delete your markup?</p></qt>"),
                                           i18n("Sign/Encrypt Message?"),
                                           KGuiItem( yesBtnText ),
                                           KGuiItem( keepBtnText ) );
    if ( KMessageBox::Cancel == ret )
      return;
    if ( KMessageBox::No == ret ) {
      mEncryptAction->setChecked(false);
      mSignAction->setChecked(false);
    }
    else {
      toggleMarkup(false);
    }
  }

  kDebug(5006) << "KMComposeWin::doSend() - calling applyChanges()"
                << endl;
  applyChanges( neverEncrypt );
}

void KMComposeWin::slotContinueDoSend( bool sentOk )
{
  kDebug(5006) << "KMComposeWin::slotContinueDoSend( " << sentOk << " )"
                << endl;
  disconnect( this, SIGNAL( applyChangesDone( bool ) ),
              this, SLOT( slotContinueDoSend( bool ) ) );

  if ( !sentOk ) {
    mDisableBreaking = false;
    return;
  }

  for ( QVector<KMMessage*>::iterator it = mComposedMessages.begin() ; it != mComposedMessages.end() ; ++it ) {

    // remove fields that contain no data (e.g. an empty Cc: or Bcc:)
    (*it)->cleanupHeader();

    // needed for imap
    (*it)->setComplete( true );

    if (mSaveInDrafts) {
      KMFolder* draftsFolder = 0, *imapDraftsFolder = 0;
      // get the draftsFolder
      if ( !(*it)->drafts().isEmpty() ) {
        draftsFolder = kmkernel->folderMgr()->findIdString( (*it)->drafts() );
        if ( draftsFolder == 0 )
          // This is *NOT* supposed to be "imapDraftsFolder", because a
          // dIMAP folder works like a normal folder
          draftsFolder = kmkernel->dimapFolderMgr()->findIdString( (*it)->drafts() );
        if ( draftsFolder == 0 )
          imapDraftsFolder = kmkernel->imapFolderMgr()->findIdString( (*it)->drafts() );
        if ( !draftsFolder && !imapDraftsFolder ) {
          const KPIM::Identity & id = kmkernel->identityManager()
            ->identityForUoidOrDefault( (*it)->headerField( "X-KMail-Identity" ).trimmed().toUInt() );
          KMessageBox::information(0, i18n("The custom drafts folder for identity "
                                           "\"%1\" does not exist (anymore); "
                                           "therefore, the default drafts folder "
                                           "will be used.",
                                     id.identityName() ) );
        }
      }
      if (imapDraftsFolder && imapDraftsFolder->noContent())
	imapDraftsFolder = 0;

      if ( draftsFolder == 0 ) {
	draftsFolder = kmkernel->draftsFolder();
      } else {
	draftsFolder->open();
      }
      kDebug(5006) << "saveindrafts: drafts=" << draftsFolder->name() << endl;
      if (imapDraftsFolder)
	kDebug(5006) << "saveindrafts: imapdrafts="
		      << imapDraftsFolder->name() << endl;

      sentOk = !(draftsFolder->addMsg((*it)));

      //Ensure the drafts message is correctly and fully parsed
      draftsFolder->unGetMsg(draftsFolder->count() - 1);
      (*it) = draftsFolder->getMsg(draftsFolder->count() - 1);

      if (imapDraftsFolder) {
	// move the message to the imap-folder and highlight it
	imapDraftsFolder->moveMsg((*it));
	(static_cast<KMFolderImap*>(imapDraftsFolder->storage()))->getFolder();
      }

    } else {
      (*it)->setTo( KMMessage::expandAliases( to() ));
      (*it)->setCc( KMMessage::expandAliases( cc() ));
      if( !mComposer->originalBCC().isEmpty() )
	(*it)->setBcc( KMMessage::expandAliases( mComposer->originalBCC() ));
      QString recips = (*it)->headerField( "X-KMail-Recipients" );
      if( !recips.isEmpty() ) {
	(*it)->setHeaderField( "X-KMail-Recipients", KMMessage::expandAliases( recips ), KMMessage::Address );
      }
      (*it)->cleanupHeader();
      sentOk = kmkernel->msgSender()->send((*it), mSendMethod);
    }

    if (!sentOk)
      return;

    *it = 0; // don't kill it later...
  }

  RecentAddresses::self( KMKernel::config() )->add( bcc() );
  RecentAddresses::self( KMKernel::config() )->add( cc() );
  RecentAddresses::self( KMKernel::config() )->add( to() );

  setModified( false );
  mAutoDeleteMsg = false;
  mFolder = 0;
  cleanupAutoSave();
  close();
  return;
}



//----------------------------------------------------------------------------
void KMComposeWin::slotSendLater()
{
  if ( mEditor->checkExternalEditorFinished() )
    doSend( KMail::MessageSender::SendLater );
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSaveDraft() {
  if ( mEditor->checkExternalEditorFinished() )
    doSend( KMail::MessageSender::SendLater, true );
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSendNowVia( int item )
{
  QStringList availTransports= KMail::TransportManager::transportNames();
  QString customTransport = availTransports[ item ];

  mTransport->setItemText( mTransport->currentIndex(), customTransport );
  slotSendNow();
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSendLaterVia( int item )
{
  QStringList availTransports= KMail::TransportManager::transportNames();
  QString customTransport = availTransports[ item ];

  mTransport->setItemText( mTransport->currentIndex(), customTransport );
  slotSendLater();
}


//----------------------------------------------------------------------------
void KMComposeWin::slotSendNow() {
  if ( !mEditor->checkExternalEditorFinished() )
    return;
  if ( GlobalSettings::self()->confirmBeforeSend() )
  {
    int rc = KMessageBox::warningYesNoCancel( mMainWidget,
                                        i18n("About to send email..."),
                                        i18n("Send Confirmation"),
                                        i18n("&Send Now"),
                                        i18n("Send &Later") );

    if ( rc == KMessageBox::Yes )
      doSend( KMail::MessageSender::SendImmediate );
    else if ( rc == KMessageBox::No )
      doSend( KMail::MessageSender::SendLater );
  }
  else
    doSend( KMail::MessageSender::SendImmediate );
}


//----------------------------------------------------------------------------
void KMComposeWin::slotAppendSignature()
{
  bool mod = mEditor->isModified();

  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  mOldSigText = ident.signatureText();
  if( !mOldSigText.isEmpty() )
  {
    mEditor->append(mOldSigText);
    mEditor->setModified(mod);
    mEditor->setContentsPos( 0, 0 );
    mEditor->sync();
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotHelp()
{
  KToolInvocation::invokeHelp();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCleanSpace()
{
  // Originally we simply used the KEdit::cleanWhiteSpace() method,
  // but that code doesn't handle quoted-lines or signatures, so instead
  // we now simply use regexp's to squeeze sequences of tabs and spaces
  // into a single space, and make sure all our lines are single-spaced.
  //
  // Yes, extra space in a quote string is squeezed.
  // Signatures are respected (i.e. not cleaned).

  QString s;
  if ( mEditor->hasMarkedText() ) {
    s = mEditor->markedText();
    if( s.isEmpty() )
      return;
  } else {
    s = mEditor->text();
  }

  // Remove the signature for now.
  QString sig;
  bool restore = false;
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoid( mId );
  if ( !ident.isNull() ) {
    sig = ident.signatureText();
    if( !sig.isEmpty() ) {
      if( s.endsWith( sig ) ) {
        s.truncate( s.length() - sig.length() );
        restore = true;
      }
    }
  }

  // Squeeze tabs and spaces
  QRegExp squeeze( "[\t, ]+" );
  s.replace( squeeze, QChar( ' ' ) );

  // Remove trailing whitespace
  QRegExp trailing( "\\s+$" );
  s.replace( trailing, QChar( '\n' ) );

  // Single space lines
  QRegExp singleSpace( "[\n]{2,}" );
  s.replace( singleSpace, QChar( '\n' ) );

  // Restore the signature
  if ( restore )
    s.append( sig );

  // Put the new text in place.
  // The lines below do not clear the undo history, but unfortuately cause
  // the side-effect that you need to press Ctrl-Z twice (first Ctrl-Z will
  // show cleared text area) to get back the original, pre-cleaned text.
  // If you use mEditor->setText( s ) then the undo history is cleared so
  // that isn't a good solution either.
  // TODO: is Qt4 better at handling the undo history??
  if ( !mEditor->hasMarkedText() )
    mEditor->clear();
  mEditor->insert( s );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotToggleMarkup()
{
 if ( markupAction->isChecked() ) {
    mHtmlMarkup = true;
    toolBar("htmlToolBar")->show();
   // markup will be toggled as soon as markup is actually used
   fontChanged( mEditor->currentFont() ); // set buttons in correct position
   mSaveFont = mEditor->currentFont();
 }
 else
   toggleMarkup(false);

}
//-----------------------------------------------------------------------------
void KMComposeWin::toggleMarkup(bool markup)
{
  if ( markup ) {
    if ( !mUseHTMLEditor ) {
      kDebug(5006) << "setting RichText editor" << endl;
      mUseHTMLEditor = true; // set it directly to true. setColor hits another toggleMarkup
      mHtmlMarkup = true;

      // set all highlighted text caused by spelling back to black
      int paraFrom, indexFrom, paraTo, indexTo;
      mEditor->getSelection ( &paraFrom, &indexFrom, &paraTo, &indexTo);
      mEditor->selectAll();
      // save the buttonstates because setColor calls fontChanged
      bool _bold = textBoldAction->isChecked();
      bool _italic = textItalicAction->isChecked();
      mEditor->setColor(QColor(0,0,0));
      textBoldAction->setChecked(_bold);
      textItalicAction->setChecked(_italic);
      mEditor->setSelection ( paraFrom, indexFrom, paraTo, indexTo);

      mEditor->setTextFormat(Qt::RichText);
      mEditor->setModified(true);
      markupAction->setChecked(true);
      toolBar( "htmlToolBar" )->show();
      mEditor->deleteAutoSpellChecking();
      mAutoSpellCheckingAction->setChecked(false);
      slotAutoSpellCheckingToggled(false);
    }
  } else { // markup is to be turned off
    kDebug(5006) << "setting PlainText editor" << endl;
    mHtmlMarkup = false;
    toolBar("htmlToolBar")->hide();
    if ( mUseHTMLEditor ) { // it was turned on
      mUseHTMLEditor = false;
      mEditor->setTextFormat(Qt::PlainText);
      QString text = mEditor->text();
      mEditor->setText(text); // otherwise the text still looks formatted
      mEditor->setModified(true);
      slotAutoSpellCheckingToggled(true);
    }
  }
}

void KMComposeWin::htmlToolBarVisibilityChanged( bool visible )
{
  // disable markup if the user hides the HTML toolbar
  if ( !visible ) {
    markupAction->setChecked( false );
    toggleMarkup( false );
  }
}

void KMComposeWin::slotSubjectTextSpellChecked()
{
  mSubjectTextWasSpellChecked = true;
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAutoSpellCheckingToggled( bool on )
{
  if ( mEditor->autoSpellChecking(on) == -1 )
    mAutoSpellCheckingAction->setChecked(false); // set it to false again
}
//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheck()
{
  if (mSpellCheckInProgress) return;
  mSubjectTextWasSpellChecked = false;
  mSpellCheckInProgress=true;
  /*
    connect (mEditor, SIGNAL (spellcheck_progress (unsigned)),
    this, SLOT (spell_progress (unsigned)));
    */

  mEditor->spellcheck();
}

#warning "ensurePolished() should be a const method, but we call non-const method"
void KMComposeWin::ensurePolished()
{
  // Ensure the html toolbar is appropriately shown/hidden
  markupAction->setChecked(mHtmlMarkup);
  if (mHtmlMarkup)
    toolBar("htmlToolBar")->show();
  else
    toolBar("htmlToolBar")->hide();
  KMail::Composer::ensurePolished();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheckDone(int result)
{
  kDebug(5006) << "spell check complete: result = " << result << endl;
  mSpellCheckInProgress=false;

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
void KMComposeWin::slotIdentityChanged( uint uoid )
{
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoid( uoid );
  if( ident.isNull() ) return;

  if( !ident.fullEmailAddr().isNull() )
    mEdtFrom->setText(ident.fullEmailAddr());
  // make sure the From field is shown if it does not contain a valid email address
  if ( EmailAddressTools::firstEmailAddress( from() ).isEmpty() )
    mShowHeaders |= HDR_FROM;
  if ( mEdtReplyTo ) mEdtReplyTo->setText(ident.replyToAddr());

  if ( mRecipientsEditor ) {
    // remove BCC of old identity and add BCC of new identity (if they differ)
    const KPIM::Identity & oldIdentity =
      kmkernel->identityManager()->identityForUoidOrDefault( mId );
    if ( oldIdentity.bcc() != ident.bcc() ) {
      mRecipientsEditor->removeRecipient( oldIdentity.bcc(), Recipient::Bcc );
      mRecipientsEditor->addRecipient( ident.bcc(), Recipient::Bcc );
    }
  }

  // don't overwrite the BCC field under certain circomstances
  // NOT edited and preset BCC from the identity
  if( mEdtBcc && !mEdtBcc->isModified() && !ident.bcc().isEmpty() ) {
    // BCC NOT empty AND contains a diff address then the preset BCC
    // of the new identity
    if( !mEdtBcc->text().isEmpty() && mEdtBcc->text() != ident.bcc() && !mEdtBcc->isModified() ) {
      mEdtBcc->setText( ident.bcc() );
    } else {
      // user type into the editbox an address that != to the preset bcc
      // of the identity, we assume that since the user typed it
      // they want to keep it
      if ( mEdtBcc->text() != ident.bcc() && !mEdtBcc->text().isEmpty() ) {
        QString temp_string( mEdtBcc->text() + QString::fromLatin1(",") + ident.bcc() );
        mEdtBcc->setText( temp_string );
      } else {
        // if the user typed the same address as the preset BCC
        // from the identity we will overwrite it to avoid duplicates.
        mEdtBcc->setText( ident.bcc() );
      }
    }
  }
  // user edited the bcc box and has a preset bcc in the identity
  // we will append whatever the user typed to the preset address
  // allowing the user to keep all addresses
  if( mEdtBcc && mEdtBcc->isModified() && !ident.bcc().isEmpty() ) {
    if( !mEdtBcc->text().isEmpty() ) {
      QString temp_string ( mEdtBcc->text() + QString::fromLatin1(",") + ident.bcc() );
      mEdtBcc->setText( temp_string );
    } else {
      mEdtBcc->setText( ident.bcc() );
    }
  }
  // user typed nothing and the identity does not have a preset bcc
  // we then reset the value to get rid of any previous
  // values if the user changed identity mid way through.
  if( mEdtBcc && !mEdtBcc->isModified() && ident.bcc().isEmpty() ) {
    mEdtBcc->setText( ident.bcc() );
  }
  // make sure the BCC field is shown because else it's ignored
  if ( !ident.bcc().isEmpty() ) {
    mShowHeaders |= HDR_BCC;
  }

  if ( ident.organization().isEmpty() )
    mMsg->removeHeaderField("Organization");
  else
    mMsg->setHeaderField("Organization", ident.organization());

  if (!ident.isXFaceEnabled() || ident.xface().isEmpty())
    mMsg->removeHeaderField("X-Face");
  else
  {
    QString xface = ident.xface();
    if (!xface.isEmpty())
    {
      int numNL = ( xface.length() - 1 ) / 70;
      for ( int i = numNL; i > 0; --i )
        xface.insert( i*70, "\n\t" );
      mMsg->setHeaderField("X-Face", xface);
    }
  }

  if ( !mBtnTransport->isChecked() ) {
    QString transp = ident.transport();
    if ( transp.isEmpty() )
    {
      mMsg->removeHeaderField("X-KMail-Transport");
      transp = mTransport->itemText(0);
    }
    else
      mMsg->setHeaderField("X-KMail-Transport", transp);
    bool found = false;
    int i;
    for (i = 0; i < mTransport->count(); i++) {
      if (mTransport->itemText(i) == transp) {
        found = true;
        mTransport->setCurrentIndex(i);
        break;
      }
    }
    if (found == false) {
      if (i == mTransport->maxCount()) mTransport->setMaxCount(i + 1);
      mTransport->addItem(transp,i);
      mTransport->setCurrentIndex(i);
    }
  }

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  if ( !mBtnFcc->isChecked() ) {
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
    if( (!mOldSigText.isEmpty()) &&
                   (GlobalSettings::self()->autoTextSignature()=="auto") )
      edtText.append( mOldSigText );
    mEditor->setText( edtText );
  }

  // disable certain actions if there is no PGP user identity set
  // for this profile
  bool bNewIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
  bool bNewIdentityHasEncryptionKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
  mAttachMPK->setEnabled( Kleo::CryptoBackendFactory::instance()->openpgp() &&
			  !ident.pgpEncryptionKey().isEmpty() );
  // save the state of the sign and encrypt button
  if ( !bNewIdentityHasEncryptionKey && mLastIdentityHasEncryptionKey ) {
    mLastEncryptActionState = mEncryptAction->isChecked();
    setEncryption( false );
  }
  if ( !bNewIdentityHasSigningKey && mLastIdentityHasSigningKey ) {
    mLastSignActionState = mSignAction->isChecked();
    setSigning( false );
  }
  // restore the last state of the sign and encrypt button
  if ( bNewIdentityHasEncryptionKey && !mLastIdentityHasEncryptionKey )
      setEncryption( mLastEncryptActionState );
  if ( bNewIdentityHasSigningKey && !mLastIdentityHasSigningKey )
    setSigning( mLastSignActionState );

  mLastIdentityHasSigningKey = bNewIdentityHasSigningKey;
  mLastIdentityHasEncryptionKey = bNewIdentityHasEncryptionKey;

  setModified( true );
  mId = uoid;

  // make sure the From and BCC fields are shown if necessary
  rethinkFields( false );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheckConfig()
{
  KDialogBase dlg(KDialogBase::Plain, i18n("Spellchecker"),
                  KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok,
                  this, 0, true, true );
  KWin kwin;
  Q3TabDialog qtd (this, "tabdialog", true);
  KSpellConfig mKSpellConfig (&qtd);
  mKSpellConfig.layout()->setMargin( KDialog::marginHint() );

  qtd.addTab (&mKSpellConfig, i18n("Spellchecker"));
  qtd.setCancelButton ();

  kwin.setIcons (qtd.winId(), qApp->windowIcon().pixmap(IconSize(K3Icon::Desktop),IconSize(K3Icon::Desktop)), qApp->windowIcon().pixmap(IconSize(K3Icon::Small),IconSize(K3Icon::Small)));
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
  KEditToolbar dlg(guiFactory(), this);

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
    KKeyChooser::LetterShortcutsDisallowed );
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

int KMComposeWin::autoSaveInterval() const
{
  return GlobalSettings::self()->autosaveInterval() * 1000 * 60;
}

void KMComposeWin::initAutoSave()
{
  kDebug(5006) << k_funcinfo << endl;
  // make sure the autosave folder exists
  KMFolderMaildir::createMaildirFolders( KMKernel::localDataPath() + "autosave" );
  if ( mAutoSaveFilename.isEmpty() ) {
    mAutoSaveFilename = KMFolderMaildir::constructValidFileName();
  }

  updateAutoSave();
}

void KMComposeWin::updateAutoSave()
{
  if ( autoSaveInterval() == 0 ) {
    delete mAutoSaveTimer; mAutoSaveTimer = 0;
  }
  else {
    if ( !mAutoSaveTimer ) {
      mAutoSaveTimer = new QTimer( this );
      connect( mAutoSaveTimer, SIGNAL( timeout() ),
               this, SLOT( autoSaveMessage() ) );
    }
    mAutoSaveTimer->start( autoSaveInterval() );
  }
}

void KMComposeWin::setAutoSaveFilename( const QString & filename )
{
  if ( !mAutoSaveFilename.isEmpty() )
    KMFolderMaildir::removeFile( KMKernel::localDataPath() + "autosave",
                                 mAutoSaveFilename );
  mAutoSaveFilename = filename;
}

void KMComposeWin::cleanupAutoSave()
{
  delete mAutoSaveTimer; mAutoSaveTimer = 0;
  if ( !mAutoSaveFilename.isEmpty() ) {
    kDebug(5006) << k_funcinfo << "deleting autosave file "
                  << mAutoSaveFilename << endl;
    KMFolderMaildir::removeFile( KMKernel::localDataPath() + "autosave",
                                 mAutoSaveFilename );
    mAutoSaveFilename.clear();
  }
}

void KMComposeWin::slotCompletionModeChanged( KGlobalSettings::Completion mode)
{
  GlobalSettings::self()->setCompletionMode( (int) mode );

  // sync all the lineedits to the same completion mode
  mEdtFrom->setCompletionMode( mode );
  mEdtReplyTo->setCompletionMode( mode );
  if ( mClassicalRecipients ) {
    mEdtTo->setCompletionMode( mode );
    mEdtCc->setCompletionMode( mode );
    mEdtBcc->setCompletionMode( mode );
  }
}

void KMComposeWin::slotConfigChanged()
{
  readConfig();
  updateAutoSave();
  rethinkFields();
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
    kDebug(5006) << "restoring drafts to " << mFolder->idString() << endl;
  }
  if (mMsg) mMsg->setParent(0);
}


void KMComposeWin::editorFocusChanged(bool gained)
{
  mPasteQuotation->setEnabled(gained);
  mAddQuoteChars->setEnabled(gained);
  mRemQuoteChars->setEnabled(gained);
}

void KMComposeWin::slotSetAlwaysSend( bool bAlways )
{
    mAlwaysSend = bAlways;
}

void KMComposeWin::slotListAction( const QString& style )
{
    toggleMarkup(true);
    if ( style == i18n( "Standard" ) )
       mEditor->setParagType( Q3StyleSheetItem::DisplayBlock, Q3StyleSheetItem::ListDisc );
    else if ( style == i18n( "Bulleted List (Disc)" ) )
       mEditor->setParagType( Q3StyleSheetItem::DisplayListItem, Q3StyleSheetItem::ListDisc );
    else if ( style == i18n( "Bulleted List (Circle)" ) )
       mEditor->setParagType( Q3StyleSheetItem::DisplayListItem, Q3StyleSheetItem::ListCircle );
    else if ( style == i18n( "Bulleted List (Square)" ) )
       mEditor->setParagType( Q3StyleSheetItem::DisplayListItem, Q3StyleSheetItem::ListSquare );
    else if ( style == i18n( "Ordered List (Decimal)" ))
       mEditor->setParagType( Q3StyleSheetItem::DisplayListItem, Q3StyleSheetItem::ListDecimal );
    else if ( style == i18n( "Ordered List (Alpha lower)" ) )
       mEditor->setParagType( Q3StyleSheetItem::DisplayListItem, Q3StyleSheetItem::ListLowerAlpha );
    else if ( style == i18n( "Ordered List (Alpha upper)" ) )
       mEditor->setParagType( Q3StyleSheetItem::DisplayListItem, Q3StyleSheetItem::ListUpperAlpha );
    mEditor->viewport()->setFocus();
}

void KMComposeWin::slotFontAction( const QString& font)
{
    toggleMarkup(true);
    mEditor->Q3TextEdit::setFamily( font );
    mEditor->viewport()->setFocus();
}

void KMComposeWin::slotSizeAction( int size )
{
    toggleMarkup(true);
    mEditor->setPointSize( size );
    mEditor->viewport()->setFocus();
}

void KMComposeWin::slotAlignLeft()
{
    toggleMarkup(true);
    mEditor->Q3TextEdit::setAlignment( Qt::AlignLeft );
}

void KMComposeWin::slotAlignCenter()
{
    toggleMarkup(true);
    mEditor->Q3TextEdit::setAlignment( Qt::AlignHCenter );
}

void KMComposeWin::slotAlignRight()
{
    toggleMarkup(true);
    mEditor->Q3TextEdit::setAlignment( Qt::AlignRight );
}

void KMComposeWin::slotTextBold()
{
    toggleMarkup(true);
    mEditor->Q3TextEdit::setBold( textBoldAction->isChecked() );
}

void KMComposeWin::slotTextItalic()
{
    toggleMarkup(true);
    mEditor->Q3TextEdit::setItalic( textItalicAction->isChecked() );
}

void KMComposeWin::slotTextUnder()
{
    toggleMarkup(true);
    mEditor->Q3TextEdit::setUnderline( textUnderAction->isChecked() );
}

void KMComposeWin::slotFormatReset()
{
  mEditor->setColor(mForeColor);
  mEditor->setCurrentFont( mSaveFont ); // fontChanged is called now
}
void KMComposeWin::slotTextColor()
{
  QColor color = mEditor->color();

  if ( KColorDialog::getColor( color, this ) ) {
    toggleMarkup(true);
    mEditor->setColor( color );
  }
}

void KMComposeWin::fontChanged( const QFont &f )
{
  QFontDatabase *fontdb = new QFontDatabase();

  if ( fontdb->bold(f.family(), "Bold") ) {
    textBoldAction->setChecked( f.bold() ); // if the user has the default font with bold, then show it in the buttonstate
    textBoldAction->setEnabled(true);
  }
  else
    textBoldAction->setEnabled(false);

  if ( fontdb->italic(f.family(), "Italic") ) {
    textItalicAction->setChecked( f.italic() );
    textItalicAction->setEnabled(true);
  }
  else
    textItalicAction->setEnabled(false);

  textUnderAction->setChecked( f.underline() );

  fontAction->setFont( f.family() );
  fontSizeAction->setFontSize( f.pointSize() );
  delete fontdb;
}

void KMComposeWin::alignmentChanged( int a )
{
    //toggleMarkup();
    alignLeftAction->setChecked( ( a == Qt::AlignLeft ) || ( a & Qt::AlignLeft ) );
    alignCenterAction->setChecked( ( a & Qt::AlignHCenter ) );
    alignRightAction->setChecked( ( a & Qt::AlignRight ) );
}

namespace {
  class KToggleActionResetter {
    KToggleAction * mAction;
    bool mOn;
  public:
    KToggleActionResetter( KToggleAction * action, bool on )
      : mAction( action ),  mOn( on ) {}
    ~KToggleActionResetter() {
      if ( mAction )
        mAction->setChecked( mOn );
    }
    void disable() { mAction = 0; }
  };
}

void KMComposeWin::slotEncryptChiasmusToggled( bool on ) {
  mEncryptWithChiasmus = false;

  if ( !on )
    return;

  KToggleActionResetter resetter( mEncryptChiasmusAction, false );

  const Kleo::CryptoBackend::Protocol * chiasmus =
    Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" );

  if ( !chiasmus ) {
    const QString msg = Kleo::CryptoBackendFactory::instance()->knowsAboutProtocol( "Chiasmus" )
      ? i18n( "Please configure a Crypto Backend to use for "
              "Chiasmus encryption first.\n"
              "You can do this in the Crypto Backends tab of "
              "the configure dialog's Security page." )
      : i18n( "It looks as though libkleopatra was compiled without "
              "Chiasmus support. You might want to recompile "
              "libkleopatra with --enable-chiasmus.");
    KMessageBox::information( this, msg, i18n("No Chiasmus Backend Configured" ) );
    return;
  }

  std::auto_ptr<Kleo::SpecialJob> job( chiasmus->specialJob( "x-obtain-keys", QMap<QString,QVariant>() ) );
  if ( !job.get() ) {
    const QString msg = i18n( "Chiasmus backend does not offer the "
                              "\"x-obtain-keys\" function. Please report this bug." );
    KMessageBox::error( this, msg, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  if ( job->exec() ) {
    job->showErrorDialog( this, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  const QVariant result = job->property( "result" );
  if ( result.type() != QVariant::StringList ) {
    const QString msg = i18n( "Unexpected return value from Chiasmus backend: "
                              "The \"x-obtain-keys\" function did not return a "
                              "string list. Please report this bug." );
    KMessageBox::error( this, msg, i18n( "Chiasmus Backend Error" ) );
    return;
  }

  const QStringList keys = result.toStringList();
  if ( keys.empty() ) {
    const QString msg = i18n( "No keys have been found. Please check that a "
                              "valid key path has been set in the Chiasmus "
                              "configuration." );
    KMessageBox::information( this, msg, i18n( "No Chiasmus Keys Found" ) );
    return;
  }

  ChiasmusKeySelector selectorDlg( this, i18n( "Chiasmus Encryption Key Selection" ),
                                   keys, GlobalSettings::chiasmusKey(),
                                   GlobalSettings::chiasmusOptions() );
  if ( selectorDlg.exec() != QDialog::Accepted )
    return;

  GlobalSettings::setChiasmusOptions( selectorDlg.options() );
  GlobalSettings::setChiasmusKey( selectorDlg.key() );
  assert( !GlobalSettings::chiasmusKey().isEmpty() );
  mEncryptWithChiasmus = true;
  resetter.disable();
}



