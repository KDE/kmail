// -*- mode: C++; c-file-style: "gnu" -*-
// kmcomposewin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

// keep this in sync with the define in configuredialog.h
#define DEFAULT_EDITOR_STR "kate %f"

#undef GrayScale
#undef Color
#include <config.h>

#include "kmcomposewin.h"

#include "kmmainwin.h"
#include "kmreaderwin.h"
#include "kmreadermainwin.h"
#include "kmsender.h"
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
#include "kmkernel.h"
#include "partNode.h"
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
#include <ui/progressdialog.h>
#include <ui/keyselectiondialog.h>

#include <gpgmepp/context.h>
#include <gpgmepp/key.h>

#include <kabc/vcardconverter.h>
#include <libkdepim/kvcarddrag.h>
#include <kio/netaccess.h>


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
#include <kstdaction.h>
#include <kdirwatch.h>
#include <kstdguiitem.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <kuserprofile.h>
#include <krun.h>
#include <ktempdir.h>
//#include <keditlistbox.h>
#include "globalsettings.h"
#include "replyphrases.h"

#include <kspell.h>
#include <kspelldlg.h>
#include <spellingfilter.h>
#include <ksyntaxhighlighter.h>
#include <kcolordialog.h>
#include <kzip.h>
#include <ksavefile.h>

#include <qtabdialog.h>
#include <qregexp.h>
#include <qbuffer.h>
#include <qtooltip.h>
#include <qtextcodec.h>
#include <qheader.h>
#include <qwhatsthis.h>
#include <qfontdatabase.h>

#include <mimelib/mimepp.h>

#include <algorithm>

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
    mComposer( 0 ),
    mLabelWidth( 0 ),
    mAutoSaveTimer( 0 ), mLastAutoSaveErrno( 0 )
{
  mClassicalRecipients = GlobalSettings::recipientsEditorType() ==
    GlobalSettings::EnumRecipientsEditorType::Classic;

  mSubjectTextWasSpellChecked = false;
  if (kmkernel->xmlGuiInstance())
    setInstance( kmkernel->xmlGuiInstance() );
  mMainWidget = new QWidget(this);
  mIdentity = new KPIM::IdentityCombo(kmkernel->identityManager(), mMainWidget);
  mDictionaryCombo = new DictionaryComboBox( mMainWidget );
  mFcc = new KMFolderComboBox(mMainWidget);
  mFcc->showOutboxFolder( FALSE );
  mTransport = new QComboBox(true, mMainWidget);
  mEdtFrom = new KMLineEdit(false,mMainWidget, "fromLine");

  mEdtReplyTo = new KMLineEdit(true,mMainWidget, "replyToLine");
  mLblReplyTo = new QLabel(mMainWidget);
  mBtnReplyTo = new QPushButton("...",mMainWidget);
  mBtnReplyTo->setFocusPolicy(QWidget::NoFocus);
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
    QToolTip::add( mBtnTo, tip );
    QToolTip::add( mBtnCc, tip );
    QToolTip::add( mBtnBcc, tip );
    QToolTip::add( mBtnReplyTo, tip );

    mBtnTo->setFocusPolicy(QWidget::NoFocus);
    mBtnCc->setFocusPolicy(QWidget::NoFocus);
    mBtnBcc->setFocusPolicy(QWidget::NoFocus);
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
  mHtmlMarkup = GlobalSettings::useHtmlMarkup();
  mShowHeaders = GlobalSettings::headers();
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
  mTempDir = 0;
  mSplitter = new QSplitter( Qt::Vertical, mMainWidget, "mSplitter" );
  mEditor = new KMEdit( mSplitter, this, mDictionaryCombo->spellConfig() );
  mSplitter->moveToFirst( mEditor );
  mSplitter->setOpaqueResize( true );

  mEditor->setTextFormat(Qt::PlainText);
  mEditor->setAcceptDrops( true );

  QWhatsThis::add( mBtnIdentity,
    GlobalSettings::self()->stickyIdentityItem()->whatsThis() );
  QWhatsThis::add( mBtnFcc,
    GlobalSettings::self()->stickyFccItem()->whatsThis() );
  QWhatsThis::add( mBtnTransport,
    GlobalSettings::self()->stickyTransportItem()->whatsThis() );

  mSpellCheckInProgress=FALSE;

  setCaption( i18n("Composer") );
  setMinimumSize(200,200);

  mBtnIdentity->setFocusPolicy(QWidget::NoFocus);
  mBtnFcc->setFocusPolicy(QWidget::NoFocus);
  mBtnTransport->setFocusPolicy(QWidget::NoFocus);

  mAtmListView = new AttachmentListView( this, mSplitter,
                                         "attachment list view" );
  mAtmListView->setSelectionMode( QListView::Extended );
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
           SIGNAL( doubleClicked( QListViewItem* ) ),
           SLOT( slotAttachProperties() ) );
  connect( mAtmListView,
           SIGNAL( rightButtonPressed( QListViewItem*, const QPoint&, int ) ),
           SLOT( slotAttachPopupMenu( QListViewItem*, const QPoint&, int ) ) );
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

  mMainWidget->resize(480,510);
  setCentralWidget(mMainWidget);
  rethinkFields();

  if ( !mClassicalRecipients ) {
    mRecipientsEditor->setFirstColumnWidth( mLabelWidth );
    // This is ugly, but if it isn't called the line edits in the recipients
    // editor aren't wide enough until the first resize event comes.
    rethinkFields();
  }

  if ( GlobalSettings::useExternalEditor() ) {
    mEditor->setUseExternalEditor(true);
    mEditor->setExternalEditorPath( GlobalSettings::externalEditor() );
  }

  initAutoSave();

  mMsg = 0;
  if (aMsg)
    setMsg(aMsg);

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
    mMapAtmLoadData.remove( it );
    job->kill();
    it = mMapAtmLoadData.begin();
  }
  deleteAll( mComposedMessages );
}

void KMComposeWin::setAutoDeleteWindow( bool f )
{
  if ( f )
    setWFlags( getWFlags() | WDestructiveClose );
  else
    setWFlags( getWFlags() & ~WDestructiveClose );
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
void KMComposeWin::addImageFromClipboard()
{
  bool ok;
  QFile *tmpFile;

  QString attName = KInputDialog::getText( "KMail", i18n("Name of the attachment:"), QString::null, &ok, this );
  if ( !ok )
    return;

  mTempDir = new KTempDir();
  mTempDir->setAutoDelete( true );

  if ( attName.lower().endsWith(".png") )
    tmpFile = new QFile(mTempDir->name() + attName );
  else
    tmpFile = new QFile(mTempDir->name() + attName + ".png" );

  if ( !QApplication::clipboard()->image().save( tmpFile->name(), "PNG" ) ) {
    KMessageBox::error( this, i18n("Unknown error trying to save image."), i18n("Attaching Image Failed") );
    delete mTempDir;
    mTempDir = 0;
    return;
  }

  addAttach( tmpFile->name() );
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
  if ( GlobalSettings::useDefaultColors() ) {
    mForeColor = QColor(kapp->palette().active().text());
    mBackColor = QColor(kapp->palette().active().base());
  } else {
    mForeColor = GlobalSettings::foregroundColor();
    mBackColor = GlobalSettings::backgroundColor();
  }

  // Color setup
  mPalette = kapp->palette();
  QColorGroup cgrp  = mPalette.active();
  cgrp.setColor( QColorGroup::Base, mBackColor);
  cgrp.setColor( QColorGroup::Text, mForeColor);
  mPalette.setDisabled(cgrp);
  mPalette.setActive(cgrp);
  mPalette.setInactive(cgrp);

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
  QCString str;

  GlobalSettings::self()->readConfig(); //TODO until configure* used kconfigXT
  mDefCharset = KMMessage::defaultCharset();
  mBtnIdentity->setChecked( GlobalSettings::stickyIdentity() );
  if (mBtnIdentity->isChecked()) {
    mId = (GlobalSettings::previousIdentity()!=0) ?
           GlobalSettings::previousIdentity() : mId;
  }
  mBtnFcc->setChecked( GlobalSettings::stickyFcc() );
  mBtnTransport->setChecked( GlobalSettings::stickyTransport() );
  QStringList transportHistory = GlobalSettings::transportHistory();
  QString currentTransport = GlobalSettings::currentTransport();

  mEdtFrom->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::completionMode() );
  mEdtReplyTo->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::completionMode() );
  if ( mClassicalRecipients ) {
    mEdtTo->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::completionMode() );
    mEdtCc->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::completionMode() );
    mEdtBcc->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::completionMode() );
  }

  readColorConfig();

  if ( GlobalSettings::useDefaultFonts() ) {
    mBodyFont = KGlobalSettings::generalFont();
    mFixedFont = KGlobalSettings::fixedFont();
  } else {
    mBodyFont = GlobalSettings::composerFont();
    mFixedFont = GlobalSettings::fixedFont();
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

  QSize siz = GlobalSettings::composerSize();
  if (siz.width() < 200) siz.setWidth(200);
  if (siz.height() < 200) siz.setHeight(200);
  resize(siz);

  mIdentity->setCurrentIdentity( mId );

  kdDebug(5006) << "KMComposeWin::readConfig. " << mIdentity->currentIdentityName() << endl;
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  mTransport->clear();
  mTransport->insertStringList( KMTransportInfo::availableTransports() );
  while ( transportHistory.count() > (uint)GlobalSettings::maxTransportEntries() )
    transportHistory.remove( transportHistory.last() );
  mTransport->insertStringList( transportHistory );
  if (mBtnTransport->isChecked() && !currentTransport.isEmpty())
  {
    for (int i = 0; i < mTransport->count(); i++)
      if (mTransport->text(i) == currentTransport)
        mTransport->setCurrentItem(i);
    mTransport->setEditText( currentTransport );
  }

  QString fccName = "";
  if ( mBtnFcc->isChecked() ) {
    fccName = GlobalSettings::previousFcc();
  } else if ( !ident.fcc().isEmpty() ) {
      fccName = ident.fcc();
  }

  setFcc( fccName );
}

//-----------------------------------------------------------------------------
void KMComposeWin::writeConfig(void)
{
  KConfig *config = KMKernel::config();
  QString str;

    KConfigGroupSaver saver(config, "Composer");
  GlobalSettings::setHeaders( mShowHeaders );
  GlobalSettings::setStickyTransport( mBtnTransport->isChecked() );
  GlobalSettings::setStickyIdentity( mBtnIdentity->isChecked() );
  GlobalSettings::setStickyFcc( mBtnFcc->isChecked() );
  GlobalSettings::setPreviousIdentity( mIdentity->currentIdentity() );
  GlobalSettings::setCurrentTransport( mTransport->currentText() );
  GlobalSettings::setPreviousFcc( mFcc->getFolder()->idString() );
  GlobalSettings::setAutoSpellChecking(
                        mAutoSpellCheckingAction->isChecked() );
  QStringList transportHistory = GlobalSettings::transportHistory();
  transportHistory.remove(mTransport->currentText());
    if (KMTransportInfo::availableTransports().findIndex(mTransport
    ->currentText()) == -1) {
      transportHistory.prepend(mTransport->currentText());
  }
  GlobalSettings::setTransportHistory( transportHistory );
  GlobalSettings::setUseFixedFont( mFixedFontAction->isChecked() );
  GlobalSettings::setUseHtmlMarkup( mHtmlMarkup );
  GlobalSettings::writeConfig();

  GlobalSettings::setComposerSize( size() );
  {
    KConfigGroupSaver saver(config, "Geometry");
    saveMainWindowSettings(config, "Composer");
    config->sync();
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::autoSaveMessage()
{
  kdDebug(5006) << k_funcinfo << endl;
  if ( !mMsg || mComposer || mAutoSaveFilename.isEmpty() )
    return;
  kdDebug(5006) << k_funcinfo << "autosaving message" << endl;

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
    kdDebug(5006) << "Composing the message failed." << endl;
    return;
  }
  KMMessage *msg = mComposedMessages.first();

  kdDebug(5006) << k_funcinfo << "opening autoSaveFile " << mAutoSaveFilename
                << endl;
  const QString filename =
    KMKernel::localDataPath() + "autosave/cur/" + mAutoSaveFilename;
  KSaveFile autoSaveFile( filename, 0600 );
  int status = autoSaveFile.status();
  kdDebug(5006) << k_funcinfo << "autoSaveFile.status() = " << status << endl;
  if ( status == 0 ) { // no error
    kdDebug(5006) << "autosaving message in " << filename << endl;
    int fd = autoSaveFile.handle();
    QCString msgStr = msg->asString();
    if ( ::write( fd, msgStr, msgStr.length() ) == -1 )
      status = errno;
  }
  if ( status == 0 ) {
    kdDebug(5006) << k_funcinfo << "closing autoSaveFile" << endl;
    autoSaveFile.close();
    mLastAutoSaveErrno = 0;
  }
  else {
    kdDebug(5006) << k_funcinfo << "autosaving failed" << endl;
    autoSaveFile.abort();
    if ( status != mLastAutoSaveErrno ) {
      // don't show the same error message twice
      KMessageBox::queuedMessageBox( 0, KMessageBox::Sorry,
                                     i18n("Autosaving the message as %1 "
                                          "failed.\n"
                                          "Reason: %2" )
                                     .arg( filename, strerror( status ) ),
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

  numRows = mNumHeaders + 1;

  delete mGrid;
  mGrid = new QGridLayout(mMainWidget, numRows, 3, 4, 4);
  mGrid->setColStretch(0, 1);
  mGrid->setColStretch(1, 100);
  mGrid->setColStretch(2, 1);
  mGrid->setRowStretch(mNumHeaders, 100);

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
  rethinkHeaderLine(showHeaders,HDR_FCC, row, i18n("&Sent-Mail folder:"),
                    mLblFcc, mFcc, mBtnFcc);

  if (!fromSlot) mTransportAction->setChecked(abs(mShowHeaders)&HDR_TRANSPORT);
  rethinkHeaderLine(showHeaders,HDR_TRANSPORT, row, i18n("&Mail transport:"),
                    mLblTransport, mTransport, mBtnTransport);

  if (!fromSlot) mFromAction->setChecked(abs(mShowHeaders)&HDR_FROM);
  rethinkHeaderLine(showHeaders,HDR_FROM, row, i18n("sender address field", "&From:"),
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
    rethinkHeaderLine(showHeaders, HDR_TO, row, i18n("recipient address field", "&To:"),
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
    mGrid->addMultiCellWidget( mRecipientsEditor, row, row, 0, 2 );
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

  mGrid->addMultiCellWidget(mSplitter, row, mNumHeaders, 0, 2);

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
      QToolTip::add( aLbl, toolTip );
    if ( !whatsThis.isEmpty() )
      QWhatsThis::add( aLbl, whatsThis );
    aLbl->adjustSize();
    aLbl->resize((int)aLbl->sizeHint().width(),aLbl->sizeHint().height() + 6);
    aLbl->setMinimumSize(aLbl->size());
    aLbl->show();
    aLbl->setBuddy(aEdt);
    mGrid->addWidget(aLbl, aRow, 0);
    if ( aLbl->width() > mLabelWidth ) mLabelWidth = aLbl->width();

    aEdt->setBackgroundColor( mBackColor );
    aEdt->show();
    aEdt->setMinimumSize(100, aLbl->height()+2);

    if (aBtn) {
      mGrid->addWidget(aEdt, aRow, 1);

      mGrid->addWidget(aBtn, aRow, 2);
      aBtn->setFixedSize(aBtn->sizeHint().width(), aLbl->height());
      aBtn->show();
    } else {
      mGrid->addMultiCellWidget(aEdt, aRow, aRow, 1, 2 );
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
    if ( aLbl->width() > mLabelWidth ) mLabelWidth = aLbl->width();

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
    (void) new KAction (i18n("&Send Now"), "mail_send", CTRL+Key_Return,
                        this, SLOT(slotSendNow()), actionCollection(),
                        "send_default");
    (void) new KAction (i18n("Send &Later"), "queue", 0,
                        this, SLOT(slotSendLater()),
                        actionCollection(), "send_alternative");
  }
  else //no, default = send later
  {
    //default = queue, alternative = send now
    (void) new KAction (i18n("Send &Later"), "queue",
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
  mRecentAction = new KRecentFilesAction (i18n("&Insert File Recent"),
		      "fileopen", 0,
		      this,  SLOT(slotInsertRecentFile(const KURL&)),
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
    new KAction( i18n("Select &Recipients..."), CTRL + Key_L, mRecipientsEditor,
      SLOT( selectRecipients() ), actionCollection(), "select_recipients" );
    new KAction( i18n("Save &Distribution List..."), 0, mRecipientsEditor,
      SLOT( saveDistributionList() ), actionCollection(),
      "save_distribution_list" );
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

  (void) new KAction (i18n("Pa&ste as Quotation"),0,this,SLOT( slotPasteAsQuotation()),
                      actionCollection(), "paste_quoted");

  (void) new KAction (i18n("Paste as Attac&hment"),0,this,SLOT( slotPasteAsAttachment()),
                      actionCollection(), "paste_att");

  (void) new KAction(i18n("Add &Quote Characters"), 0, this,
              SLOT(slotAddQuotes()), actionCollection(), "tools_quote");

  (void) new KAction(i18n("Re&move Quote Characters"), 0, this,
              SLOT(slotRemoveQuotes()), actionCollection(), "tools_unquote");


  (void) new KAction (i18n("Cl&ean Spaces"), 0, this, SLOT(slotCleanSpace()),
                      actionCollection(), "clean_spaces");

  mFixedFontAction = new KToggleAction( i18n("Use Fi&xed Font"), 0, this,
                      SLOT(slotUpdateFont()), actionCollection(), "toggle_fixedfont" );
  mFixedFontAction->setChecked( GlobalSettings::useFixedFont() );

  //these are checkable!!!
  mUrgentAction = new KToggleAction (i18n("&Urgent"), 0,
                                    actionCollection(),
                                    "urgent");
  mRequestMDNAction = new KToggleAction ( i18n("&Request Disposition Notification"), 0,
                                         actionCollection(),
                                         "options_request_mdn");
  mRequestMDNAction->setChecked(GlobalSettings::requestMDN());
  //----- Message-Encoding Submenu
  mEncodingAction = new KSelectAction( i18n( "Se&t Encoding" ), "charset",
                                      0, this, SLOT(slotSetCharset() ),
                                      actionCollection(), "charsets" );
  mWordWrapAction = new KToggleAction (i18n("&Wordwrap"), 0,
                      actionCollection(), "wordwrap");
  mWordWrapAction->setChecked(GlobalSettings::wordWrap());
  connect(mWordWrapAction, SIGNAL(toggled(bool)), SLOT(slotWordWrapToggled(bool)));

  mAutoSpellCheckingAction =
    new KToggleAction( i18n( "&Automatic Spellchecking" ), "spellcheck", 0,
                       actionCollection(), "options_auto_spellchecking" );
  const bool spellChecking = GlobalSettings::autoSpellChecking();
  mAutoSpellCheckingAction->setEnabled( !GlobalSettings::useExternalEditor() );
  mAutoSpellCheckingAction->setChecked( !GlobalSettings::useExternalEditor() && spellChecking );
  slotAutoSpellCheckingToggled( !GlobalSettings::useExternalEditor() && spellChecking );
  connect( mAutoSpellCheckingAction, SIGNAL( toggled( bool ) ),
           this, SLOT( slotAutoSpellCheckingToggled( bool ) ) );

  QStringList encodings = KMMsgBase::supportedEncodings(TRUE);
  encodings.prepend( i18n("Auto-Detect"));
  mEncodingAction->setItems( encodings );
  mEncodingAction->setCurrentItem( -1 );

  //these are checkable!!!
  markupAction = new KToggleAction (i18n("Formatting (HTML)"), 0, this,
                                    SLOT(slotToggleMarkup()),
                      actionCollection(), "html");

  mAllFieldsAction = new KToggleAction (i18n("&All Fields"), 0, this,
                                       SLOT(slotView()),
                                       actionCollection(), "show_all_fields");
  mIdentityAction = new KToggleAction (i18n("&Identity"), 0, this,
                                      SLOT(slotView()),
                                      actionCollection(), "show_identity");
  mDictionaryAction = new KToggleAction (i18n("&Dictionary"), 0, this,
                                         SLOT(slotView()),
                                         actionCollection(), "show_dictionary");
  mFccAction = new KToggleAction (i18n("&Sent-Mail Folder"), 0, this,
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
  if ( mClassicalRecipients ) {
    mToAction = new KToggleAction (i18n("&To"), 0, this,
                                  SLOT(slotView()),
                                  actionCollection(), "show_to");
    mCcAction = new KToggleAction (i18n("&CC"), 0, this,
                                  SLOT(slotView()),
                                  actionCollection(), "show_cc");
    mBccAction = new KToggleAction (i18n("&BCC"), 0, this,
                                   SLOT(slotView()),
                                   actionCollection(), "show_bcc");
  }
  mSubjectAction = new KToggleAction (i18n("S&ubject"), 0, this,
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
  mAttachPropertiesAction = new KAction (i18n("Attachment Pr&operties"), 0, this,
                      SLOT(slotAttachProperties()),
                      actionCollection(), "attach_properties");

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
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  // PENDING(marc): check the uses of this member and split it into
  // smime/openpgp and or enc/sign, if necessary:
  mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
  mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

  mLastEncryptActionState = false;
  mLastSignActionState = GlobalSettings::pgpAutoSign();

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
    setSigning( ( canOpenPGPSign || canSMIMESign ) && GlobalSettings::pgpAutoSign() );
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

  listAction = new KSelectAction( i18n( "Select Style" ), 0, actionCollection(),
                                 "text_list" );
  listAction->setItems( styleItems );
  connect( listAction, SIGNAL( activated( const QString& ) ),
           SLOT( slotListAction( const QString& ) ) );
  fontAction = new KFontAction( "Select Font", 0, actionCollection(),
                               "text_font" );
  connect( fontAction, SIGNAL( activated( const QString& ) ),
           SLOT( slotFontAction( const QString& ) ) );
  fontSizeAction = new KFontSizeAction( "Select Size", 0, actionCollection(),
                                       "text_size" );
  connect( fontSizeAction, SIGNAL( fontSizeChanged( int ) ),
           SLOT( slotSizeAction( int ) ) );

  alignLeftAction = new KToggleAction (i18n("Align Left"), "text_left", 0,
                      this, SLOT(slotAlignLeft()), actionCollection(),
                      "align_left");
  alignLeftAction->setChecked( TRUE );
  alignRightAction = new KToggleAction (i18n("Align Right"), "text_right", 0,
                      this, SLOT(slotAlignRight()), actionCollection(),
                      "align_right");
  alignCenterAction = new KToggleAction (i18n("Align Center"), "text_center", 0,
                       this, SLOT(slotAlignCenter()), actionCollection(),
                       "align_center");
  textBoldAction = new KToggleAction( i18n("&Bold"), "text_bold", CTRL+Key_B,
                                     this, SLOT(slotTextBold()),
                                     actionCollection(), "text_bold");
  textItalicAction = new KToggleAction( i18n("&Italic"), "text_italic", CTRL+Key_I,
                                       this, SLOT(slotTextItalic()),
                                       actionCollection(), "text_italic");
  textUnderAction = new KToggleAction( i18n("&Underline"), "text_under", CTRL+Key_U,
                                     this, SLOT(slotTextUnder()),
                                     actionCollection(), "text_under");
  actionFormatReset = new KAction( i18n( "Reset Font Settings" ), "eraser", 0,
                                     this, SLOT( slotFormatReset() ),
                                     actionCollection(), "format_reset");
  actionFormatColor = new KAction( i18n( "Text Color..." ), "colorize", 0,
                                     this, SLOT( slotTextColor() ),
                                     actionCollection(), "format_color");

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

  if (GlobalSettings::wordWrap())
  {
    mEditor->setWordWrap( QMultiLineEdit::FixedColumnWidth );
    mEditor->setWrapColumnOrWidth( GlobalSettings::lineWrapWidth() );
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
  return res.stripWhiteSpace();
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
    return QString::null;
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
    return QString::null;
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
    return QString::null;
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
    return QString::null;
  }
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
  //assert(newMsg!=0);
  if(!newMsg)
    {
      kdDebug(5006) << "KMComposeWin::setMsg() : newMsg == 0!" << endl;
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

  KPIM::IdentityManager * im = kmkernel->identityManager();

  const KPIM::Identity & ident = im->identityForUoid( mIdentity->currentIdentity() );

  // check for the presence of a DNT header, indicating that MDN's were
  // requested
  QString mdnAddr = newMsg->headerField("Disposition-Notification-To");
  mRequestMDNAction->setChecked( ( !mdnAddr.isEmpty() &&
                                  im->thatIsMe( mdnAddr ) ) ||
                                  GlobalSettings::requestMDN() );

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

    QCString bodyDecoded = mMsg->bodyDecoded();

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
  kdDebug(5006) << "KMComposeWin::setMsg() mMsg->numBodyParts="
                << mMsg->numBodyParts() << endl;

  if ( num > 0 ) {
    KMMessagePart bodyPart;
    int firstAttachment = 0;

    mMsg->bodyPart(1, &bodyPart);
    if ( bodyPart.typeStr().lower() == "text" &&
         bodyPart.subtypeStr().lower() == "html" ) {
      // check whether we are inside a mp/al body part
      partNode *root = partNode::fromMessage( mMsg );
      partNode *node = root->findType( DwMime::kTypeText,
                                       DwMime::kSubtypeHtml );
      if ( node && node->parentNode() &&
           node->parentNode()->hasType( DwMime::kTypeMultipart ) &&
           node->parentNode()->hasSubType( DwMime::kSubtypeAlternative ) ) {
        // we have a mp/al body part with a text and an html body
      kdDebug(5006) << "KMComposeWin::setMsg() : text/html found" << endl;
      firstAttachment = 2;
        if ( mMsg->headerField( "X-KMail-Markup" ) == "true" )
          toggleMarkup( true );
      }
      delete root; root = 0;
    }
    if ( firstAttachment == 0 ) {
        mMsg->bodyPart(0, &bodyPart);
        if ( bodyPart.typeStr().lower() == "text" ) {
          // we have a mp/mx body with a text body
        kdDebug(5006) << "KMComposeWin::setMsg() : text/* found" << endl;
          firstAttachment = 1;
        }
      }

    if ( firstAttachment != 0 ) // there's text to show
    {
      mCharset = bodyPart.charset();
      if ( mCharset.isEmpty() || mCharset == "default" )
        mCharset = mDefCharset;

      QCString bodyDecoded = bodyPart.bodyDecoded();

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
#endif // BROKEN_FOR_OPAQUE_SIGNED_OR_ENCRYPTED_MAILS

  if( (GlobalSettings::autoTextSignature()=="auto") && mayAutoSign ) {
    //
    // Espen 2000-05-16
    // Delay the signature appending. It may start a fileseletor.
    // Not user friendy if this modal fileseletor opens before the
    // composer.
    //
    QTimer::singleShot( 0, this, SLOT(slotAppendSignature()) );
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
           mEdtFrom->edited() ||
           ( mEdtReplyTo && mEdtReplyTo->edited() ) ||
           ( mEdtTo && mEdtTo->edited() ) ||
           ( mEdtCc && mEdtCc->edited() ) ||
           ( mEdtBcc && mEdtBcc->edited() ) ||
           mEdtSubject->edited() ||
           mAtmModified ||
           ( mTransport->lineEdit() && mTransport->lineEdit()->edited() ) );
}


//-----------------------------------------------------------------------------
void KMComposeWin::setModified( bool modified )
{
  mEditor->setModified( modified );
  if ( !modified ) {
    mEdtFrom->setEdited( false );
    if ( mEdtReplyTo ) mEdtReplyTo->setEdited( false );
    if ( mEdtTo ) mEdtTo->setEdited( false );
    if ( mEdtCc ) mEdtCc->setEdited( false );
    if ( mEdtBcc ) mEdtBcc->setEdited( false );
    mEdtSubject->setEdited( false );
    mAtmModified =  false ;
    if ( mTransport->lineEdit() )
      mTransport->lineEdit()->setEdited( false );
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
           KGuiItem(i18n("&Save as Draft"), "filesave", QString::null,
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
  KConfigGroup composer( KMKernel::config(), "Composer" );
  bool checkForForgottenAttachments = GlobalSettings::showForgottenAttachmentWarning();

  if ( !checkForForgottenAttachments || ( mAtmList.count() > 0 ) )
    return false;


  QStringList attachWordsList = GlobalSettings::attachmentKeywords();

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
  QString subj = subject();
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
void KMComposeWin::applyChanges( bool dontSignNorEncrypt, bool dontDisable )
{
#ifdef DEBUG
  kdDebug(5006) << "entering KMComposeWin::applyChanges" << endl;
#endif

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
void KMComposeWin::addAttach(const KURL aUrl)
{
  if ( !aUrl.isValid() ) {
    KMessageBox::sorry( this, i18n( "<qt><p>KMail could not recognize the location of the attachment (%1);</p>"
                                 "<p>you have to specify the full path if you wish to attach a file.</p></qt>" )
                        .arg( aUrl.prettyURL() ) );
    return;
  }
  KIO::TransferJob *job = KIO::get(aUrl);
  KIO::Scheduler::scheduleJob( job );
  atmLoadData ld;
  ld.url = aUrl;
  ld.data = QByteArray();
  ld.insert = false;
  if( !aUrl.fileEncoding().isEmpty() )
    ld.encoding = aUrl.fileEncoding().latin1();

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
                                 KMAtmListViewItem *lvi, bool loadDefaults)
{
  assert(msgPart != 0);

  if (!msgPart->fileName().isEmpty())
    lvi->setText(0, msgPart->fileName());
  else
    lvi->setText(0, msgPart->name());
  lvi->setText(1, KIO::convertSize( msgPart->decodedSize()));
  lvi->setText(2, msgPart->contentTransferEncodingStr());
  lvi->setText(3, prettyMimeType(msgPart->typeStr() + "/" + msgPart->subtypeStr()));

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
    if ( GlobalSettings::addresseeSelectorType() ==
         GlobalSettings::EnumAddresseeSelectorType::New ) {
      addrBookSelIntoNew();
    } else {
      addrBookSelIntoOld();
    }
  } else {
    kdWarning() << "To be implemented: call recipients picker." << endl;
  }
}

void KMComposeWin::addrBookSelIntoOld()
{
  AddressesDialog dlg( this );
  QString txt;
  QStringList lst;

  txt = to();
  if ( !txt.isEmpty() ) {
      lst = KPIM::splitEmailAddrList( txt );
      dlg.setSelectedTo( lst );
  }

  txt = mEdtCc->text();
  if ( !txt.isEmpty() ) {
      lst = KPIM::splitEmailAddrList( txt );
      dlg.setSelectedCC( lst );
  }

  txt = mEdtBcc->text();
  if ( !txt.isEmpty() ) {
      lst = KPIM::splitEmailAddrList( txt );
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
      lst = KPIM::splitEmailAddrList( txt );
      selection.setSelectedTo( lst );
  }

  txt = mEdtCc->text();
  if ( !txt.isEmpty() ) {
      lst = KPIM::splitEmailAddrList( txt );
      selection.setSelectedCC( lst );
  }

  txt = mEdtBcc->text();
  if ( !txt.isEmpty() ) {
      lst = KPIM::splitEmailAddrList( txt );
      selection.setSelectedBCC( lst );
  }

  if (dlg.exec()==QDialog::Rejected) return;

  QStringList list = selection.to() + selection.toDistributionLists();
  mEdtTo->setText( list.join(", ") );
  mEdtTo->setEdited( true );

  list = selection.cc() + selection.ccDistributionLists();
  mEdtCc->setText( list.join(", ") );
  mEdtCc->setEdited( true );

  list = selection.bcc() + selection.bccDistributionLists();
  mEdtBcc->setText( list.join(", ") );
  mEdtBcc->setEdited( true );

  //Make sure BCC field is shown if needed
  if ( !mEdtBcc->text().isEmpty() ) {
    mShowHeaders |= HDR_BCC;
    rethinkFields( false );
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::setCharset(const QCString& aCharset, bool forceDefault)
{
  if ((forceDefault && GlobalSettings::forceReplyCharset()) || aCharset.isEmpty())
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

  KFileDialog fdlg(QString::null, QString::null, this, 0, TRUE);
  fdlg.setOperationMode( KFileDialog::Other );
  fdlg.setCaption(i18n("Attach File"));
  fdlg.okButton()->setGuiItem(KGuiItem(i18n("&Attach"),"fileopen"));
  fdlg.setMode(KFile::Files);
  fdlg.exec();
  KURL::List files = fdlg.selectedURLs();

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
  const QCString partCharset = (*it).url.fileEncoding().isEmpty()
                             ? mCharset
                             : QCString((*it).url.fileEncoding().latin1());

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
        int i = ext.findRev( '.' );
        if( i == -1 )
          ext.prepend( '.' );
        else if( i > 0 )
          ext = ext.mid( i );
      }
      name = QString("unknown") += ext;
    }
  }

  name.truncate( 256 ); // is this needed?

  QCString encoding = KMMsgBase::autoDetectCharset(partCharset,
    KMMessage::preferredCharsets(), name);
  if (encoding.isEmpty()) encoding = "utf-8";

  QCString encName;
  if ( GlobalSettings::outlookCompatibleAttachments() )
    encName = KMMsgBase::encodeRFC2047String( name, encoding );
  else
    encName = KMMsgBase::encodeRFC2231String( name, encoding );
  bool RFC2231encoded = false;
  if ( !GlobalSettings::outlookCompatibleAttachments() )
    RFC2231encoded = name != QString( encName );

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

  msgPart->setCharset(partCharset);

  // show message part dialog, if not configured away (default):
  KConfigGroup composer(KMKernel::config(), "Composer");
  if ( GlobalSettings::showMessagePartDialogOnAttach() ) {
    const KCursorSaver saver( QCursor::ArrowCursor );
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
  fdlg.setOperationMode( KFileDialog::Opening );
  fdlg.okButton()->setText(i18n("&Insert"));
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
  mRecentAction->addURL(u);
  // Prevent race condition updating list when multiple composers are open
  {
    KConfig *config = KMKernel::config();
    KConfigGroupSaver saver( config, "Composer" );
    QString encoding = KGlobal::charsets()->encodingForName(combo->currentText()).latin1();
    QStringList urls = config->readListEntry( "recent-urls" );
    QStringList encodings = config->readListEntry( "recent-encodings" );
    // Prevent config file from growing without bound
    // Would be nicer to get this constant from KRecentFilesAction
    uint mMaxRecentFiles = 30;
    while (urls.count() > mMaxRecentFiles)
      urls.erase( urls.fromLast() );
    while (encodings.count() > mMaxRecentFiles)
      urls.erase( encodings.fromLast() );
    // sanity check
    if (urls.count() != encodings.count()) {
      urls.clear();
      encodings.clear();
    }
    urls.prepend( u.prettyURL() );
    encodings.prepend( encoding );
    config->writeEntry( "recent-urls", urls );
    config->writeEntry( "recent-encodings", encodings );
    mRecentAction->saveEntries( config );
  }
  slotInsertRecentFile(u);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertRecentFile(const KURL& u)
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
    KConfigGroupSaver saver( config, "Composer" );
    QStringList urls = config->readListEntry( "recent-urls" );
    QStringList encodings = config->readListEntry( "recent-encodings" );
    int index = urls.findIndex( u.prettyURL() );
    if (index != -1) {
      QString encoding = encodings[ index ];
      ld.encoding = encoding.latin1();
    }
  }
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
        for( KMAtmListViewItem* lvi = static_cast<KMAtmListViewItem*>( mAtmItemList.first() );
             lvi;
             lvi = static_cast<KMAtmListViewItem*>( mAtmItemList.next() ) ) {
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
      for( KMAtmListViewItem* lvi = static_cast<KMAtmListViewItem*>( mAtmItemList.first() );
           lvi;
           lvi = static_cast<KMAtmListViewItem*>( mAtmItemList.next() ) ) {
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
      for( KMAtmListViewItem* lvi = static_cast<KMAtmListViewItem*>( mAtmItemList.first() );
           lvi;
           lvi = static_cast<KMAtmListViewItem*>( mAtmItemList.next() ) ) {
        lvi->enableCryptoCBs( false );
      }
    }
  }
}

static void showExportError( QWidget * w, const GpgME::Error & err ) {
  assert( err );
  const QString msg = i18n("<qt><p>An error occurred while trying to export "
			   "the key from the backend:</p>"
			   "<p><b>%1</b></p></qt>")
    .arg( QString::fromLocal8Bit( err.asString() ) );
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
  if ( mFingerprint.isEmpty() )
    return;
  Kleo::ExportJob * job = Kleo::CryptoBackendFactory::instance()->openpgp()->publicKeyExportJob( true );
  assert( job );

  connect( job, SIGNAL(result(const GpgME::Error&,const QByteArray&)),
	   this, SLOT(slotPublicKeyExportResult(const GpgME::Error&,const QByteArray&)) );

  const GpgME::Error err = job->start( mFingerprint );
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
  msgPart->setName( i18n("OpenPGP key 0x%1").arg( mFingerprint ) );
  msgPart->setTypeStr("application");
  msgPart->setSubtypeStr("pgp-keys");
  QValueList<int> dummy;
  msgPart->setBodyAndGuessCte(keydata, dummy, false);
  msgPart->setContentDisposition( "attachment;\n\tfilename=0x" + QCString( mFingerprint.latin1() ) + ".asc" );

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
void KMComposeWin::slotAttachPopupMenu(QListViewItem *, const QPoint &, int)
{
  if (!mAttachMenu)
  {
     mAttachMenu = new QPopupMenu(this);

     mOpenId = mAttachMenu->insertItem(i18n("to open", "Open"), this,
                             SLOT(slotAttachOpen()));
     mViewId = mAttachMenu->insertItem(i18n("to view", "View"), this,
                             SLOT(slotAttachView()));
     mRemoveId = mAttachMenu->insertItem(i18n("Remove"), this, SLOT(slotAttachRemove()));
     mSaveAsId = mAttachMenu->insertItem( SmallIconSet("filesaveas"), i18n("Save As..."), this,
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

  mAttachMenu->setItemEnabled( mOpenId, selectedCount > 0 );
  mAttachMenu->setItemEnabled( mViewId, selectedCount > 0 );
  mAttachMenu->setItemEnabled( mRemoveId, selectedCount > 0 );
  mAttachMenu->setItemEnabled( mSaveAsId, selectedCount == 1 );
  mAttachMenu->setItemEnabled( mPropertiesId, selectedCount == 1 );

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
    mAtmModified = TRUE;
    // values may have changed, so recreate the listbox line
    if( listItem ) {
      msgPartToItem(msgPart, listItem);
      if( canSignEncryptAttachments() ) {
        listItem->setSign(    dlg.isSigned()    );
        listItem->setEncrypt( dlg.isEncrypted() );
      }
    }
  }
  if (msgPart->typeStr().lower() != "text") msgPart->setCharset(QCString());
}

//-----------------------------------------------------------------------------
void KMComposeWin::compressAttach( int idx )
{
  if (idx < 0) return;

  unsigned int i;
  for ( i = 0; i < mAtmItemList.count(); ++i )
      if ( mAtmItemList.at( i )->itemPos() == idx )
          break;

  if ( i > mAtmItemList.count() )
      return;

  KMMessagePart* msgPart;
  msgPart = mAtmList.at( i );
  QByteArray array;
  QBuffer dev( array );
  KZip zip( &dev );
  QByteArray decoded = msgPart->bodyDecodedBinary();
  if ( ! zip.open( IO_WriteOnly ) ) {
    KMessageBox::sorry(0, i18n("KMail could not compress the file.") );
    static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) )->setCompress( false );
    return;
  }

  zip.setCompression( KZip::DeflateCompression );
  if ( ! zip.writeFile( msgPart->name(), "", "", decoded.size(),
           decoded.data() ) ) {
    KMessageBox::sorry(0, i18n("KMail could not compress the file.") );
    static_cast<KMAtmListViewItem*>( mAtmItemList.at( i ) )->setCompress( false );
    return;
  }
  zip.close();
  if ( array.size() >= decoded.size() ) {
    if ( KMessageBox::questionYesNo( this, i18n("The compressed file is larger "
        "than the original. Do you want to keep the original one?" ) )
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

  QCString cDisp = "attachment;";
  QCString encoding = KMMsgBase::autoDetectCharset( msgPart->charset(),
    KMMessage::preferredCharsets(), name );
  kdDebug(5006) << "encoding: " << encoding << endl;
  if ( encoding.isEmpty() ) encoding = "utf-8";
  kdDebug(5006) << "encoding after: " << encoding << endl;
  QCString encName;
  if ( GlobalSettings::outlookCompatibleAttachments() )
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

  unsigned int i;
  for ( i = 0; i < mAtmItemList.count(); ++i )
      if ( mAtmItemList.at( i )->itemPos() == idx )
          break;

  if ( i > mAtmItemList.count() )
      return;

  KMMessagePart* msgPart;
  msgPart = mAtmList.at( i );

  QBuffer dev( msgPart->bodyDecodedBinary() );
  KZip zip( &dev );
  QByteArray decoded;

  decoded = msgPart->bodyDecodedBinary();
  if ( ! zip.open( IO_ReadOnly ) ) {
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

  QCString cDisp = "attachment;";
  QCString encoding = KMMsgBase::autoDetectCharset( msgPart->charset(),
    KMMessage::preferredCharsets(), name );
  if ( encoding.isEmpty() ) encoding = "utf-8";

  QCString encName;
  if ( GlobalSettings::outlookCompatibleAttachments() )
    encName = KMMsgBase::encodeRFC2047String( name, encoding );
  else
    encName = KMMsgBase::encodeRFC2231String( name, encoding );

  cDisp += "\n\tfilename";
  if ( name != QString( encName ) )
    cDisp += "*=" + encName;
  else
    cDisp += "=\"" + encName + '"';
  msgPart->setContentDisposition( cDisp );

  QCString type, subtype;
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
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      viewAttach( i );
    }
  }
}
//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachOpen()
{
  int i = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it, ++i ) {
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
  QString str, pname;
  KMMessagePart* msgPart;
  msgPart = mAtmList.at(index);
  pname = msgPart->name().stripWhiteSpace();
  if (pname.isEmpty()) pname=msgPart->contentDescription();
  if (pname.isEmpty()) pname="unnamed";

  KTempFile* atmTempFile = new KTempFile();
  mAtmTempList.append( atmTempFile );
  atmTempFile->setAutoDelete( true );
  KPIM::kByteArrayToFile(msgPart->bodyDecodedBinary(), atmTempFile->name(), false, false,
    false);
  KMReaderMainWin *win = new KMReaderMainWin(msgPart, false,
    atmTempFile->name(), pname, KMMsgBase::codecForName(mCharset) );
  win->show();
}

//-----------------------------------------------------------------------------
void KMComposeWin::openAttach( int index )
{
  KMMessagePart* msgPart = mAtmList.at(index);
  const QString contentTypeStr =
    ( msgPart->typeStr() + '/' + msgPart->subtypeStr() ).lower();

  KMimeType::Ptr mimetype;
  mimetype = KMimeType::mimeType( contentTypeStr );

  KTempFile* atmTempFile = new KTempFile();
  mAtmTempList.append( atmTempFile );
  const bool autoDelete = true;
  atmTempFile->setAutoDelete( autoDelete );

  KURL url;
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
  kdDebug() << "KMComposeWin::slotUpdateFont " << endl;
  if ( ! mFixedFontAction ) {
    return;
  }
  mEditor->setFont( mFixedFontAction->isChecked() ? mFixedFont : mBodyFont );
}

QString KMComposeWin::quotePrefixName() const
{
    if ( !msg() )
        return QString::null;

    int languageNr = GlobalSettings::replyCurrentLanguage();
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

void KMComposeWin::slotPasteAsAttachment()
{
  KURL url( QApplication::clipboard()->text( QClipboard::Clipboard ) );
  if ( url.isValid() ) {
    addAttach(QApplication::clipboard()->text( QClipboard::Clipboard ) );
    return;
  }

  if ( QApplication::clipboard()->image().isNull() )  {
    bool ok;
    QString attName = KInputDialog::getText( "KMail", i18n("Name of the attachment:"), QString::null, &ok, this );
    if ( !ok )
      return;
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName(attName);
    QValueList<int> dummy;
    msgPart->setBodyAndGuessCte(QCString(QApplication::clipboard()->text().latin1()), dummy,
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

  if ( ! QApplication::clipboard()->image().isNull() )  {
    addImageFromClipboard();
  }
  else {

#ifdef KeyPress
#undef KeyPress
#endif

    QKeyEvent k(QEvent::KeyPress, Key_V , 0 , ControlButton);
    kapp->notify(fw, &k);
  }

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
    mEncryptAction->setIcon("encrypted");
  else
    mEncryptAction->setIcon("decrypted");

  // mark the attachments for (no) encryption
  if ( canSignEncryptAttachments() ) {
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
    mEditor->setWrapColumnOrWidth( GlobalSettings::lineWrapWidth() );
  }
  else
  {
    mEditor->setWordWrap( QMultiLineEdit::NoWrap );
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
      kdDebug(5006) << "Composing the message failed." << endl;
      return;
    }
    KMCommand *command = new KMPrintCommand( this, mComposedMessages.first() );
    command->start();
    setModified( mMessageWasModified );
  }
}


//----------------------------------------------------------------------------
void KMComposeWin::doSend(int aSendNow, bool saveInDrafts)
{
  mSendNow = aSendNow;
  mSaveInDrafts = saveInDrafts;

  if (!saveInDrafts)
  {
    if ( KPIM::getFirstEmailAddress( from() ).isEmpty() ) {
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

  const bool neverEncrypt = ( saveInDrafts && GlobalSettings::neverEncryptDrafts() )
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

  kdDebug(5006) << "KMComposeWin::doSend() - calling applyChanges()"
                << endl;
  applyChanges( neverEncrypt );
}

void KMComposeWin::slotContinueDoSend( bool sentOk )
{
  kdDebug(5006) << "KMComposeWin::slotContinueDoSend( " << sentOk << " )"
                << endl;
  disconnect( this, SIGNAL( applyChangesDone( bool ) ),
              this, SLOT( slotContinueDoSend( bool ) ) );

  if ( !sentOk ) {
    mDisableBreaking = false;
    return;
  }

  for ( QValueVector<KMMessage*>::iterator it = mComposedMessages.begin() ; it != mComposedMessages.end() ; ++it ) {

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
            ->identityForUoidOrDefault( (*it)->headerField( "X-KMail-Identity" ).stripWhiteSpace().toUInt() );
          KMessageBox::information(0, i18n("The custom drafts folder for identity "
                                           "\"%1\" does not exist (anymore); "
                                           "therefore, the default drafts folder "
                                           "will be used.")
                                   .arg( id.identityName() ) );
        }
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
	(*it)->setHeaderField( "X-KMail-Recipients", KMMessage::expandAliases( recips ) );
      }
      (*it)->cleanupHeader();
      sentOk = kmkernel->msgSender()->send((*it), mSendNow);
    }

    if (!sentOk)
      return;

    *it = 0; // don't kill it later...
  }

  RecentAddresses::self( KMKernel::config() )->add( bcc() );
  RecentAddresses::self( KMKernel::config() )->add( cc() );
  RecentAddresses::self( KMKernel::config() )->add( to() );

  setModified( false );
  mAutoDeleteMsg = FALSE;
  mFolder = 0;
  cleanupAutoSave();
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
  if ( GlobalSettings::confirmBeforeSend() ) {
    switch(KMessageBox::warningYesNoCancel(mMainWidget,
                                    i18n("About to send email..."),
                                    i18n("Send Confirmation"),
                                    i18n("&Send Now"),
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

  const KPIM::Identity & ident =
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
void KMComposeWin::slotToggleMarkup()
{
 if ( markupAction->isChecked() ) {
    mHtmlMarkup = true;
    toolBar("htmlToolBar")->show();
   // markup will be toggled as soon as markup is actually used
   fontChanged( mEditor->currentFont().family() ); // set buttons in correct position
   fontAction->setFont( mEditor->currentFont().family() );
   fontSizeAction->setFontSize( mEditor->currentFont().pointSize() );
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
    kdDebug(5006) << "setting RichText editor" << endl;
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
  }
  else if ( mUseHTMLEditor ) {
    kdDebug(5006) << "setting PlainText editor" << endl;
    mHtmlMarkup = false;
    mUseHTMLEditor = false;
    mEditor->setTextFormat(Qt::PlainText);
    QString text = mEditor->text();
    mEditor->setText(text); // otherwise the text still looks formatted
    mEditor->setModified(true);
    toolBar("htmlToolBar")->hide();
    mEditor->initializeAutoSpellChecking( mDictionaryCombo->spellConfig());
    slotAutoSpellCheckingToggled(true);
  }
  else if ( !markup && !mUseHTMLEditor )
    {
      mHtmlMarkup = false;
      toolBar("htmlToolBar")->hide();
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
  mSpellCheckInProgress=TRUE;
  /*
    connect (mEditor, SIGNAL (spellcheck_progress (unsigned)),
    this, SLOT (spell_progress (unsigned)));
    */

  mEditor->spellcheck();
}

void KMComposeWin::polish()
{
  // Ensure the html toolbar is appropriately shown/hidden
  markupAction->setChecked(mHtmlMarkup);
  if (mHtmlMarkup)
    toolBar("htmlToolBar")->show();
  else
    toolBar("htmlToolBar")->hide();
  KMail::SecondaryWindow::polish();
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
void KMComposeWin::slotIdentityChanged( uint uoid )
{
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoid( uoid );
  if( ident.isNull() ) return;

  if( !ident.fullEmailAddr().isNull() )
    mEdtFrom->setText(ident.fullEmailAddr());
  // make sure the From field is shown if it does not contain a valid email address
  if ( KPIM::getFirstEmailAddress( from() ).isEmpty() )
    mShowHeaders |= HDR_FROM;
  if ( mEdtReplyTo ) mEdtReplyTo->setText(ident.replyToAddr());
  // don't overwrite the BCC field under certain circomstances
  // NOT edited and preset BCC from the identity
  if( mEdtBcc && !mEdtBcc->edited() && !ident.bcc().isEmpty() ) {
    // BCC NOT empty AND contains a diff adress then the preset BCC
    // of the new identity
    if( !mEdtBcc->text().isEmpty() && mEdtBcc->text() != ident.bcc() && !mEdtBcc->edited() ) {
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
  if( mEdtBcc && mEdtBcc->edited() && !ident.bcc().isEmpty() ) {
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
  if( mEdtBcc && !mEdtBcc->edited() && ident.bcc().isEmpty() ) {
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
                   (GlobalSettings::autoTextSignature()=="auto") )
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

int KMComposeWin::autoSaveInterval() const
{
  return GlobalSettings::autosaveInterval() * 1000 * 60;
}

void KMComposeWin::initAutoSave()
{
  kdDebug(5006) << k_funcinfo << endl;
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
  kdDebug(5006) << k_funcinfo << endl;
  delete mAutoSaveTimer; mAutoSaveTimer = 0;
  if ( !mAutoSaveFilename.isEmpty() ) {
    kdDebug(5006) << k_funcinfo << "deleting autosave file "
                  << mAutoSaveFilename << endl;
    KMFolderMaildir::removeFile( KMKernel::localDataPath() + "autosave",
                                 mAutoSaveFilename );
    mAutoSaveFilename = QString();
  }
}

void KMComposeWin::slotCompletionModeChanged( KGlobalSettings::Completion mode)
{
  GlobalSettings::setCompletionMode( (int) mode );

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
    kdDebug(5006) << "restoring drafts to " << mFolder->idString() << endl;
  }
  if (mMsg) mMsg->setParent(0);
}



void KMComposeWin::slotSetAlwaysSend( bool bAlways )
{
    mAlwaysSend = bAlways;
}

void KMComposeWin::slotListAction( const QString& style )
{
    toggleMarkup(true);
    if ( style == i18n( "Standard" ) )
       mEditor->setParagType( QStyleSheetItem::DisplayBlock, QStyleSheetItem::ListDisc );
    else if ( style == i18n( "Bulleted List (Disc)" ) )
       mEditor->setParagType( QStyleSheetItem::DisplayListItem, QStyleSheetItem::ListDisc );
    else if ( style == i18n( "Bulleted List (Circle)" ) )
       mEditor->setParagType( QStyleSheetItem::DisplayListItem, QStyleSheetItem::ListCircle );
    else if ( style == i18n( "Bulleted List (Square)" ) )
       mEditor->setParagType( QStyleSheetItem::DisplayListItem, QStyleSheetItem::ListSquare );
    else if ( style == i18n( "Ordered List (Decimal)" ))
       mEditor->setParagType( QStyleSheetItem::DisplayListItem, QStyleSheetItem::ListDecimal );
    else if ( style == i18n( "Ordered List (Alpha lower)" ) )
       mEditor->setParagType( QStyleSheetItem::DisplayListItem, QStyleSheetItem::ListLowerAlpha );
    else if ( style == i18n( "Ordered List (Alpha upper)" ) )
       mEditor->setParagType( QStyleSheetItem::DisplayListItem, QStyleSheetItem::ListUpperAlpha );
    mEditor->viewport()->setFocus();
}

void KMComposeWin::slotFontAction( const QString& font)
{
    toggleMarkup(true);
    mEditor->QTextEdit::setFamily( font );
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
    mEditor->QTextEdit::setAlignment( AlignLeft );
}

void KMComposeWin::slotAlignCenter()
{
    toggleMarkup(true);
    mEditor->QTextEdit::setAlignment( AlignHCenter );
}

void KMComposeWin::slotAlignRight()
{
    toggleMarkup(true);
    mEditor->QTextEdit::setAlignment( AlignRight );
}

void KMComposeWin::slotTextBold()
{
    toggleMarkup(true);
    mEditor->QTextEdit::setBold( textBoldAction->isChecked() );
}

void KMComposeWin::slotTextItalic()
{
    toggleMarkup(true);
    mEditor->QTextEdit::setItalic( textItalicAction->isChecked() );
}

void KMComposeWin::slotTextUnder()
{
    toggleMarkup(true);
    mEditor->QTextEdit::setUnderline( textUnderAction->isChecked() );
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
    alignLeftAction->setChecked( ( a == AlignAuto ) || ( a & AlignLeft ) );
    alignCenterAction->setChecked( ( a & AlignHCenter ) );
    alignRightAction->setChecked( ( a & AlignRight ) );
}

void KMEdit::contentsDragEnterEvent(QDragEnterEvent *e)
{
    if (e->provides(MailListDrag::format()))
        e->accept(true);
    else
        return KEdit::contentsDragEnterEvent(e);
}

void KMEdit::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if (e->provides(MailListDrag::format()))
        e->accept();
    else
        return KEdit::contentsDragMoveEvent(e);
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

		// The cursor position might have changed unpredictably if there was selected
		// text which got replaced by a new line, so we query it again:
		getCursorPosition( &line, &col );
                QString newLine = text( line );
                // remove leading white space from the new line and instead
                // add the quote indicators of the previous line
                unsigned int leadingWhiteSpaceCount = 0;
                while( ( leadingWhiteSpaceCount < newLine.length() )
                       && newLine[leadingWhiteSpaceCount].isSpace() ) {
                    ++leadingWhiteSpaceCount;
                }
                newLine = newLine.replace( 0, leadingWhiteSpaceCount,
                                           lineText.left( bot ) );
                removeParagraph( line );
                insertParagraph( newLine, line );
                // place the cursor at the begin of the new line since
                // we assume that the user split the quoted line in order
                // to add a comment to the first part of the quoted line
                setCursorPosition( line, 0 );
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
            KPopupMenu p;
            p.insertItem( i18n("Add as Text"), 0 );
            p.insertItem( i18n("Add as Attachment"), 1 );
            int id = p.exec( mapToGlobal( e->pos() ) );
            switch ( id) {
              case 0:
                for ( KURL::List::Iterator it = urlList.begin();
                     it != urlList.end(); ++it ) {
                  insert( (*it).url() );
                }
                break;
              case 1:
                for ( KURL::List::Iterator it = urlList.begin();
                     it != urlList.end(); ++it ) {
                  mComposer->addAttach( *it );
                }
                break;
            }
        }
        else if ( QTextDrag::canDecode( e ) ) {
          QString s;
          if ( QTextDrag::decode( e, s ) )
            insert( s );
        }
        else
          kdDebug(5006) << "KMEdit::contentsDropEvent, unable to add dropped object" << endl;
    }
    else {
        return KEdit::contentsDropEvent(e);
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
  mCBEncrypt = new QCheckBox( mListview->viewport() );
  mCBSign = new QCheckBox( mListview->viewport() );
  mCBCompress = new QCheckBox( mListview->viewport() );
  connect( mCBCompress, SIGNAL( clicked() ), this, SLOT( slotCompress() ) );

  mCBEncrypt->hide();
  mCBSign->hide();
}

KMAtmListViewItem::~KMAtmListViewItem()
{
  delete mCBEncrypt;
  mCBEncrypt = 0;
  delete mCBSign;
  mCBSign = 0;
  delete mCBCompress;
  mCBCompress = 0;
}

void KMAtmListViewItem::paintCell( QPainter * p, const QColorGroup & cg,
                                  int column, int width, int align )
{
  // this is also called for the encrypt/sign columns to assure that the
  // background is cleared
  QListViewItem::paintCell( p, cg, column, width, align );
  if ( 4 == column ) {
    QRect r = mListview->itemRect( this );
    if ( !r.size().isValid() ) {
        mListview->ensureItemVisible( this );
        mListview->repaintContents( FALSE );
        r = mListview->itemRect( this );
    }
    int colWidth = mListview->header()->sectionSize( column );
    r.setX( mListview->header()->sectionPos( column )
            - mListview->header()->offset()
            + colWidth / 2
            - r.height() / 2
            - 1 );
    r.setY( r.y() + 1 );
    r.setWidth(  r.height() - 2 );
    r.setHeight( r.height() - 2 );
    r = QRect( mListview->viewportToContents( r.topLeft() ), r.size() );

    mCBCompress->resize( r.size() );
    mListview->moveChild( mCBCompress, r.x(), r.y() );

    QColor bg;
    if (isSelected())
      bg = cg.highlight();
    else
      bg = cg.base();

    mCBCompress->setPaletteBackgroundColor(bg);
    mCBCompress->show();
  }
  if( 5 == column || 6 == column ) {
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

    QCheckBox* cb = (5 == column) ? mCBEncrypt : mCBSign;
    cb->resize( r.size() );
    mListview->moveChild( cb, r.x(), r.y() );

    QColor bg;
    if (isSelected())
      bg = cg.highlight();
    else
      bg = cg.base();

    bool enabled = (5 == column) ? mCBEncryptEnabled : mCBSignEnabled;
    cb->setPaletteBackgroundColor(bg);
    if (enabled) cb->show();
  }
}

void KMAtmListViewItem::enableCryptoCBs(bool on)
{
  if( mCBEncrypt ) {
    mCBEncryptEnabled = on;
    mCBEncrypt->setEnabled( on );
    mCBEncrypt->setShown( on );
  }
  if( mCBSign ) {
    mCBSignEnabled = on;
    mCBSign->setEnabled( on );
    mCBSign->setShown( on );
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

void KMAtmListViewItem::setCompress(bool on)
{
  mCBCompress->setChecked( on );
}

bool KMAtmListViewItem::isCompress()
{
  return mCBCompress->isChecked();
}

void KMAtmListViewItem::slotCompress()
{
    if ( mCBCompress->isChecked() )
        emit compress( itemPos() );
    else
        emit uncompress( itemPos() );
}

void KMAtmListViewItem::setUncompressedMimeType( QCString type,
    QCString subtype )
{
    mType = type;
    mSubtype = subtype;
}

void KMAtmListViewItem::uncompressedMimeType( QCString &type,
    QCString &subtype )
{
    type = mType;
    subtype = mSubtype;
}

void KMAtmListViewItem::setUncompressedCodec( QCString codec )
{
    mCodec = codec;
}

QCString KMAtmListViewItem::uncompressedCodec()
{
    return mCodec;
}


//=============================================================================
//
//   Class  KMLineEdit
//
//=============================================================================

KMLineEdit::KMLineEdit(bool useCompletion,
                       QWidget *parent, const char *name)
    : KPIM::AddresseeLineEdit(parent,useCompletion,name)
{
}


//-----------------------------------------------------------------------------
void KMLineEdit::keyPressEvent(QKeyEvent *e)
{
  if ((e->key() == Key_Enter || e->key() == Key_Return) &&
      !completionBox()->isVisible())
  {
    emit focusDown();
    AddresseeLineEdit::keyPressEvent(e);
    return;
  }
  if (e->key() == Key_Up)
  {
    emit focusUp();
    return;
  }
  if (e->key() == Key_Down)
  {
    emit focusDown();
    return;
  }
  AddresseeLineEdit::keyPressEvent(e);
}


void KMLineEdit::insertEmails( QStringList emails )
{
  if ( !emails.isEmpty() ) {
    QStringList::Iterator it;
    it = emails.begin();
    QString contents = text();
    if ( !contents.isEmpty() )
      contents.append(",");
    // only one address, don't need kpopup to choose
    if ( emails.count() == 1 ) {
      contents.append( *it );
    } else {
      //multiple emails, let the user choose one
      KPopupMenu *menu = new KPopupMenu( this,"Addresschooser" );
      for ( it = emails.begin(); it != emails.end(); ++it )
        menu->insertItem( *it );
      int result = menu->exec( QCursor::pos() );
      contents.append( menu->text( result) );
    }
  setText( contents );
  }
}

void KMLineEdit::dropEvent(QDropEvent *event)
{
  QString vcards;
  KVCardDrag::decode( event, vcards );
  if ( !vcards.isEmpty() ) {
    KABC::VCardConverter converter;
    KABC::Addressee::List list = converter.parseVCards( vcards );
    KABC::Addressee::List::Iterator ait;
    for ( ait = list.begin(); ait != list.end(); ++ait ){
      insertEmails( (*ait).emails() );
    }
  } else {
    KURL::List urls;
    if ( KURLDrag::decode( event, urls) ) {
      //kdDebug(5006) << "urlList" << endl;
      KURL::List::Iterator it = urls.begin();
      KABC::VCardConverter converter;
      KABC::Addressee::List list;
      QString fileName;
      QString caption( i18n( "vCard Import Failed" ) );
      for ( it = urls.begin(); it != urls.end(); ++it ) {
        if ( KIO::NetAccess::download( *it, fileName, parentWidget() ) ) {
          QFile file( fileName );
          file.open( IO_ReadOnly );
          QByteArray rawData = file.readAll();
          file.close();
          QString data = QString::fromUtf8( rawData.data(), rawData.size() + 1 );
          list += converter.parseVCards( data );
          KIO::NetAccess::removeTempFile( fileName );
        } else {
          QString text = i18n( "<qt>Unable to access <b>%1</b>.</qt>" );
          KMessageBox::error( parentWidget(), text.arg( (*it).url() ), caption );
        }
        KABC::Addressee::List::Iterator ait;
        for ( ait = list.begin(); ait != list.end(); ++ait )
          insertEmails((*ait).emails());
      }
    } else {
      KPIM::AddresseeLineEdit::dropEvent( event );
    }
  }
}

QPopupMenu *KMLineEdit::createPopupMenu()
{
    QPopupMenu *menu = KPIM::AddresseeLineEdit::createPopupMenu();
    if ( !menu )
        return 0;

    menu->insertSeparator();
    menu->insertItem( i18n( "Edit Recent Addresses..." ),
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

    loadContacts();
  }
}


//-----------------------------------------------------------------------------
void KMLineEdit::loadContacts()
{
  // was: KABC::AddressLineEdit::loadAddresses()
  AddresseeLineEdit::loadContacts();

  if ( KMKernel::self() ) {
    QStringList recent =
      RecentAddresses::self( KMKernel::config() )->addresses();
    QStringList::Iterator it = recent.begin();
    QString name, email;
    for ( ; it != recent.end(); ++it ) {
      KABC::Addressee addr;
      KPIM::getNameAndMail(*it, name, email);
      addr.setNameFromString( name );
      addr.insertEmail( email, true );
      addContact( addr, 120 ); // more weight than kabc entries and more than ldap results
    }
  }
}


KMLineEditSpell::KMLineEditSpell(bool useCompletion,
                       QWidget *parent, const char *name)
    : KMLineEdit(useCompletion,parent,name)
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
        emit subjectTextSpellChecked();
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
  setOverwriteEnabled( true );

  initializeAutoSpellChecking( autoSpellConfig );
}

//-----------------------------------------------------------------------------
void KMEdit::initializeAutoSpellChecking( KSpellConfig* autoSpellConfig )
{
  if ( mSpellChecker )
    return; // already initialized
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
QPopupMenu *KMEdit::createPopupMenu( const QPoint& pos )
{
  enum { IdUndo, IdRedo, IdSep1, IdCut, IdCopy, IdPaste, IdClear, IdSep2, IdSelectAll };

  QPopupMenu *menu = KEdit::createPopupMenu( pos );
  if ( !QApplication::clipboard()->image().isNull() ) {
    int id = menu->idAt(0);
    menu->setItemEnabled( id - IdPaste, true);
  }

  return menu;
}
//-----------------------------------------------------------------------------
void KMEdit::deleteAutoSpellChecking()
{ // because the highlighter doesn't support RichText, delete its instance.
  delete mSpellChecker;
  mSpellChecker =0;
}
//-----------------------------------------------------------------------------
void KMEdit::addSuggestion(const QString& text, const QStringList& lst, unsigned int )
{
  mReplacements[text] = lst;
}

void KMEdit::setSpellCheckingActive(bool spellCheckingActive)
{
  if ( mSpellChecker ) {
    mSpellChecker->setActive(spellCheckingActive);
  }
}

//-----------------------------------------------------------------------------
KMEdit::~KMEdit()
{
  removeEventFilter(this);

  delete mKSpell;
  delete mSpellChecker;
  mSpellChecker = 0;

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
unsigned int KMEdit::lineBreakColumn() const
{
  unsigned int lineBreakColumn = 0;
  unsigned int numlines = numLines();
  while ( numlines-- ) {
    lineBreakColumn = QMAX( lineBreakColumn, textLine( numlines ).length() );
  }
  return lineBreakColumn;
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
        emit focusUp();
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
      mExtEditorProcess->setUseShell( true );
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
      emit focusUp();
      return TRUE;
    }
    // ---sven's Arrow key navigation end ---

    if (k->key() == Key_Backtab && k->state() == ShiftButton)
    {
      deselect();
      emit focusUp();
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
int KMEdit::autoSpellChecking( bool on )
{
  if ( textFormat() == Qt::RichText ) {
     // syntax highlighter doesn't support extended text properties
     if ( on )
       KMessageBox::sorry(this, i18n("Automatic spellchecking is not possible on text with markup."));
     return -1;
  }

  // don't autoEnable spell checking if the user turned spell checking off
  mSpellChecker->setAutomatic( on );
  mSpellChecker->setActive( on );
  return 1;
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

  insertLine(QString::fromLocal8Bit(KPIM::kFileToString( fileName, true, false )), -1);
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
//  maybe for later, for now plaintext is given to KSpell
//  if (textFormat() == Qt::RichText ) {
//    kdDebug(5006) << "KMEdit::spellcheck, spellchecking for RichText" << endl;
//    mKSpell = new KSpell(this, i18n("Spellcheck - KMail"), this,
//                    SLOT(slotSpellcheck2(KSpell*)),0,true,false,KSpell::HTML);
//  }
//  else {
    mKSpell = new KSpell(this, i18n("Spellcheck - KMail"), this,
                      SLOT(slotSpellcheck2(KSpell*)));
//  }

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

void KMEdit::cut()
{
  KEdit::cut();
  if ( textFormat() != Qt::RichText )
    mSpellChecker->restartBackgroundSpellCheck();
}

void KMEdit::clear()
{
  KEdit::clear();
  if ( textFormat() != Qt::RichText )
    mSpellChecker->restartBackgroundSpellCheck();
}

void KMEdit::del()
{
  KEdit::del();
  if ( textFormat() != Qt::RichText )
    mSpellChecker->restartBackgroundSpellCheck();
}

void KMEdit::paste()
{
  if ( ! QApplication::clipboard()->image().isNull() )  {
    emit pasteImage();
  }
  else
    KEdit::paste();
}

void KMEdit::slotMisspelling(const QString &text, const QStringList &lst, unsigned int pos)
{
    kdDebug(5006)<<"void KMEdit::slotMisspelling(const QString &text, const QStringList &lst, unsigned int pos) : "<<text <<endl;
    if( mSpellLineEdit )
        mComposer->sujectLineWidget()->spellCheckerMisspelling( text, lst, pos);
    else
        misspelling(text, lst, pos);
}

void KMEdit::slotCorrected (const QString &oldWord, const QString &newWord, unsigned int pos)
{
    kdDebug(5006)<<"slotCorrected (const QString &oldWord, const QString &newWord, unsigned int pos) : "<<oldWord<<endl;
    if( mSpellLineEdit )
        mComposer->sujectLineWidget()->spellCheckerCorrected( oldWord, newWord, pos);
    else {
        unsigned int l = 0;
        unsigned int cnt = 0;
        bool _bold,_underline,_italic;
        QColor _color;
        QFont _font;
        posToRowCol (pos, l, cnt);
        setCursorPosition(l, cnt+1); // the new word will get the same markup now as the first character of the word
        _bold = bold();
        _underline = underline();
        _italic = italic();
        _color = color();
        _font = currentFont();
        corrected(oldWord, newWord, pos);
        setSelection (l, cnt, l, cnt+newWord.length());
        setBold(_bold);
        setItalic(_italic);
        setUnderline(_underline);
        setColor(_color);
        setCurrentFont(_font);
    }

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
            int languageNr = GlobalSettings::replyCurrentLanguage();
            ReplyPhrases replyPhrases( QString::number(languageNr) );
            replyPhrases.readConfig();

            quotePrefix = mComposer->msg()->formatString(
                 replyPhrases.indentPrefix() );
        }

        kdDebug(5006) << "spelling: new SpellingFilter with prefix=\"" << quotePrefix << "\"" << endl;
        QTextEdit plaintext;
        plaintext.setText(text());
        plaintext.setTextFormat(Qt::PlainText);
        mSpellingFilter = new SpellingFilter(plaintext.text(), quotePrefix, SpellingFilter::FilterUrls,
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
          setModified(true);
      }
  }
  mKSpell->cleanUp();
  KDictSpellingHighlighter::dictionaryChanged();

  emit spellcheck_done( dlgResult );
}

//-----------------------------------------------------------------------------
void KMEdit::slotSpellDone()
{
  kdDebug(5006)<<" void KMEdit::slotSpellDone()\n";
  KSpell::spellStatus status = mKSpell->status();
  delete mKSpell;
  mKSpell = 0;

  kdDebug(5006) << "spelling: delete SpellingFilter" << endl;
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
      else if( !mComposer->subjectTextWasSpellChecked() && status == KSpell::FinishedNoMisspellingsEncountered )
          KMessageBox::information( topLevelWidget(),
                                    i18n("No misspellings encountered.") );
  }
}
