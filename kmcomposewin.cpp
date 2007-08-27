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
#include "attachmentlistview.h"
#include "transportmanager.h"
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
#include "editorwatcher.h"

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
#include <memory>

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

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
    mAppendSignatureAction( 0 ), mPrependSignatureAction( 0 ), mInsertSignatureAction( 0 ),
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
    connect( mRecipientsEditor,
             SIGNAL( completionModeChanged( KGlobalSettings::Completion ) ),
             SLOT( slotCompletionModeChanged( KGlobalSettings::Completion ) ) );

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
  mAtmList.setAutoDelete(true);
  mAtmTempList.setAutoDelete(true);
  mAtmModified = false;
  mAutoDeleteMsg = false;
  mFolder = 0;
  mAutoCharset = true;
  mFixedFontAction = 0;
  mTempDir = 0;
  mSplitter = new QSplitter( Qt::Vertical, mMainWidget, "mSplitter" );
  mEditor = new KMEdit( mSplitter, this, mDictionaryCombo->spellConfig() );
  mSplitter->moveToFirst( mEditor );
  mSplitter->setOpaqueResize( true );

  mEditor->initializeAutoSpellChecking();
  mEditor->setTextFormat(Qt::PlainText);
  mEditor->setAcceptDrops( true );

  QWhatsThis::add( mBtnIdentity,
    GlobalSettings::self()->stickyIdentityItem()->whatsThis() );
  QWhatsThis::add( mBtnFcc,
    GlobalSettings::self()->stickyFccItem()->whatsThis() );
  QWhatsThis::add( mBtnTransport,
    GlobalSettings::self()->stickyTransportItem()->whatsThis() );

  mSpellCheckInProgress=false;

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
           SLOT( slotAttachEdit() ) );
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
  connect (mEditor, SIGNAL( attachPNGImageData(const QByteArray &) ),
    this, SLOT ( slotAttachPNGImageData(const QByteArray &) ) );
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
  slotUpdateSignatureActions();
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
void KMComposeWin::addAttachmentsAndSend(const KURL::List &urls, const QString &/*comment*/, int how)
{
  if (urls.isEmpty())
  {
    send(how);
    return;
  }
  mAttachFilesSend = how;
  mAttachFilesPending = urls;
  connect(this, SIGNAL(attachmentAdded(const KURL&, bool)), SLOT(slotAttachedFile(const KURL&)));
  for( KURL::List::ConstIterator itr = urls.begin(); itr != urls.end(); ++itr ) {
    if (!addAttach( *itr ))
      mAttachFilesPending.remove(mAttachFilesPending.find(*itr)); // only remove one copy of the url
  }

  if (mAttachFilesPending.isEmpty() && mAttachFilesSend == how)
  {
    send(mAttachFilesSend);
    mAttachFilesSend = -1;
  }
}

void KMComposeWin::slotAttachedFile(const KURL &url)
{
  if (mAttachFilesPending.isEmpty())
    return;
  mAttachFilesPending.remove(mAttachFilesPending.find(url)); // only remove one copy of url
  if (mAttachFilesPending.isEmpty())
  {
    send(mAttachFilesSend);
    mAttachFilesSend = -1;
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
    if( type == "message" && subType == "rfc822" ) {
       msgPart->setMessageBody( data );
    } else {
       QValueList<int> dummy;
       msgPart->setBodyAndGuessCte(data, dummy,
              kmkernel->msgSender()->sendQuotedPrintable());
    }
    msgPart->setTypeStr(type);
    msgPart->setSubtypeStr(subType);
    msgPart->setParameter(paramAttr,paramValue);
    msgPart->setContentDisposition(contDisp);
    addAttach(msgPart);
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachPNGImageData(const QByteArray &image)
{
  bool ok;

  QString attName = KInputDialog::getText( "KMail", i18n("Name of the attachment:"), QString::null, &ok, this );
  if ( !ok )
    return;

  if ( !attName.lower().endsWith(".png") ) attName += ".png";

  addAttachment( attName, "base64", image, "image", "png", QCString(), QString(), QCString() );
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
    mForeColor = QColor(kapp->palette().active().text());
    mBackColor = QColor(kapp->palette().active().base());
  } else {
    mForeColor = GlobalSettings::self()->foregroundColor();
    mBackColor = GlobalSettings::self()->backgroundColor();
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
  else
    mRecipientsEditor->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );

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

  kdDebug(5006) << "KMComposeWin::readConfig. " << mIdentity->currentIdentityName() << endl;
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  mTransport->clear();
  mTransport->insertStringList( KMTransportInfo::availableTransports() );
  while ( transportHistory.count() > (uint)GlobalSettings::self()->maxTransportEntries() )
    transportHistory.remove( transportHistory.last() );
  mTransport->insertStringList( transportHistory );
  mTransport->setCurrentText( GlobalSettings::self()->defaultTransport() );
  if ( mBtnTransport->isChecked() ) {
    setTransport( currentTransport );
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
  transportHistory.remove(mTransport->currentText());
    if (KMTransportInfo::availableTransports().findIndex(mTransport
    ->currentText()) == -1) {
      transportHistory.prepend(mTransport->currentText());
  }
  GlobalSettings::self()->setTransportHistory( transportHistory );
  GlobalSettings::self()->setUseFixedFont( mFixedFontAction->isChecked() );
  GlobalSettings::self()->setUseHtmlMarkup( mHtmlMarkup );
  GlobalSettings::self()->setComposerSize( size() );

  KConfigGroupSaver saver( KMKernel::config(), "Geometry" );
  saveMainWindowSettings( KMKernel::config(), "Composer" );
  // make sure config changes are written to disk, cf. bug 127538
  GlobalSettings::self()->writeConfig();
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
    const DwString& msgStr = msg->asDwString();
    if ( ::write( fd, msgStr.data(), msgStr.length() ) == -1 )
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
  return QMAX( width, w->sizeHint().width() );
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
  mGrid = new QGridLayout(mMainWidget, numRows, 3, KDialogBase::marginHint()/2, KDialogBase::spacingHint());
  mGrid->setColStretch(0, 1);
  mGrid->setColStretch(1, 100);
  mGrid->setColStretch(2, 1);
  mGrid->setRowStretch(mNumHeaders, 100);

  row = 0;
  kdDebug(5006) << "KMComposeWin::rethinkFields" << endl;
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
      QToolTip::add( aLbl, toolTip );
    if ( !whatsThis.isEmpty() )
      QWhatsThis::add( aLbl, whatsThis );
    aLbl->setFixedWidth( mLabelWidth );
    aLbl->setBuddy(aEdt);
    mGrid->addWidget(aLbl, aRow, 0);
    aEdt->setBackgroundColor( mBackColor );
    aEdt->show();

    if (aBtn) {
      mGrid->addWidget(aEdt, aRow, 1);

      mGrid->addWidget(aBtn, aRow, 2);
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
    ( void )  new KAction( i18n("&Send Mail"), "mail_send", CTRL+Key_Return,
                        this, SLOT(slotSendNow()), actionCollection(),"send_default");

    // FIXME: change to mail_send_via icon when this exits.
    actActionNowMenu =  new KActionMenu (i18n("&Send Mail Via"), "mail_send",
		    actionCollection(), "send_default_via" );

    (void) new KAction (i18n("Send &Later"), "queue", 0, this,
			SLOT(slotSendLater()), actionCollection(),"send_alternative");
    actActionLaterMenu = new KActionMenu (i18n("Send &Later Via"), "queue",
		    actionCollection(), "send_alternative_via" );

  }
  else //no, default = send later
  {
    //default = queue, alternative = send now
    (void) new KAction (i18n("Send &Later"), "queue",
                        CTRL+Key_Return,
                        this, SLOT(slotSendLater()), actionCollection(),"send_default");
    actActionLaterMenu = new KActionMenu (i18n("Send &Later Via"), "queue",
		    actionCollection(), "send_default_via" );

   ( void )  new KAction( i18n("&Send Mail"), "mail_send", 0,
                        this, SLOT(slotSendNow()), actionCollection(),"send_alternative");

    // FIXME: change to mail_send_via icon when this exits.
    actActionNowMenu =  new KActionMenu (i18n("&Send Mail Via"), "mail_send",
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




  (void) new KAction (i18n("Save as &Draft"), "filesave", 0,
                      this, SLOT(slotSaveDraft()),
                      actionCollection(), "save_in_drafts");
  (void) new KAction (i18n("Save as &Template"), "filesave", 0,
                      this, SLOT(slotSaveTemplate()),
                      actionCollection(), "save_in_templates");
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

  mPasteQuotation = new KAction (i18n("Pa&ste as Quotation"),0,this,SLOT( slotPasteAsQuotation()),
                      actionCollection(), "paste_quoted");

  (void) new KAction (i18n("Paste as Attac&hment"),0,this,SLOT( slotPasteAsAttachment()),
                      actionCollection(), "paste_att");

  mAddQuoteChars = new KAction(i18n("Add &Quote Characters"), 0, this,
              SLOT(slotAddQuotes()), actionCollection(), "tools_quote");

  mRemQuoteChars = new KAction(i18n("Re&move Quote Characters"), 0, this,
              SLOT(slotRemoveQuotes()), actionCollection(), "tools_unquote");


  (void) new KAction (i18n("Cl&ean Spaces"), 0, this, SLOT(slotCleanSpace()),
                      actionCollection(), "clean_spaces");

  mFixedFontAction = new KToggleAction( i18n("Use Fi&xed Font"), 0, this,
                      SLOT(slotUpdateFont()), actionCollection(), "toggle_fixedfont" );
  mFixedFontAction->setChecked( GlobalSettings::self()->useFixedFont() );

  //these are checkable!!!
  mUrgentAction = new KToggleAction (i18n("&Urgent"), 0,
                                    actionCollection(),
                                    "urgent");
  mRequestMDNAction = new KToggleAction ( i18n("&Request Disposition Notification"), 0,
                                         actionCollection(),
                                         "options_request_mdn");
  mRequestMDNAction->setChecked(GlobalSettings::self()->requestMDN());
  //----- Message-Encoding Submenu
  mEncodingAction = new KSelectAction( i18n( "Se&t Encoding" ), "charset",
                                      0, this, SLOT(slotSetCharset() ),
                                      actionCollection(), "charsets" );
  mWordWrapAction = new KToggleAction (i18n("&Wordwrap"), 0,
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

  mAppendSignatureAction = new KAction (i18n("Append S&ignature"), 0, this,
                      SLOT(slotAppendSignature()),
                      actionCollection(), "append_signature");
  mPrependSignatureAction =  new KAction (i18n("Prepend S&ignature"), 0, this,
                      SLOT(slotPrependSignature()),
                      actionCollection(), "prepend_signature");

  mInsertSignatureAction =  new KAction (i18n("Insert Signature At C&ursor Position"), "edit", 0, this,
                      SLOT(slotInsertSignatureAtCursor()),
                      actionCollection(), "insert_signature_at_cursor_position");

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
  alignLeftAction->setChecked( true );
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

  //  editorFocusChanged(false);
  createGUI("kmcomposerui.rc");

  connect( toolBar("htmlToolBar"), SIGNAL( visibilityChanged(bool) ),
           this, SLOT( htmlToolBarVisibilityChanged(bool) ) );

  // In Kontact, this entry would read "Configure Kontact", but bring
  // up KMail's config dialog. That's sensible, though, so fix the label.
  KAction* configureAction = actionCollection()->action("options_configure" );
  if ( configureAction )
    configureAction->setText( i18n("Configure KMail..." ) );
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupStatusBar(void)
{
  statusBar()->insertItem("", 0, 1);
  statusBar()->setItemAlignment(0, AlignLeft | AlignVCenter);

  statusBar()->insertItem(i18n( " Spellcheck: %1 ").arg( "   " ), 3, 0, true );
  statusBar()->insertItem(i18n( " Column: %1 ").arg("     "), 2, 0, true);
  statusBar()->insertItem(i18n( " Line: %1 ").arg("     "), 1, 0, true);
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
  mEditor->setModified(false);
  QFontMetrics fm(mBodyFont);
  mEditor->setTabStopWidth(fm.width(QChar(' ')) * 8);
  //mEditor->setFocusPolicy(QWidget::ClickFocus);

  if (GlobalSettings::self()->wordWrap())
  {
    mEditor->setWordWrap( QTextEdit::FixedColumnWidth );
    mEditor->setWrapColumnOrWidth( GlobalSettings::self()->lineWrapWidth() );
  }
  else
  {
    mEditor->setWordWrap( QTextEdit::NoWrap );
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
  if (mEditor->QTextEdit::wordWrap() == QTextEdit::FixedColumnWidth) {
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
void KMComposeWin::setTransport( const QString & transport )
{
  kdDebug(5006) << "KMComposeWin::setTransport( \"" << transport << "\" )" << endl;
  // Don't change the transport combobox if transport is empty
  if ( transport.isEmpty() )
    return;

  bool transportFound = false;
  for ( int i = 0; i < mTransport->count(); ++i ) {
    if ( mTransport->text(i) == transport ) {
      transportFound = true;
      mTransport->setCurrentItem(i);
      kdDebug(5006) << "transport found, it's no. " << i << " in the list" << endl;
      break;
    }
  }
  if ( !transportFound ) { // unknown transport
    kdDebug(5006) << "unknown transport \"" << transport << "\"" << endl;
    if ( transport.startsWith("smtp://") || transport.startsWith("smtps://") ||
         transport.startsWith("file://") ) {
      // set custom transport
      mTransport->setEditText( transport );
    }
    else {
      // neither known nor custom transport -> use default transport
      mTransport->setCurrentText( GlobalSettings::self()->defaultTransport() );
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
  KPIM::IdentityManager * im = kmkernel->identityManager();

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
    mRecipientsEditor->setFocusBottom();
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
    uint savedId = mId;
    if ( !newMsg->headerField("X-KMail-Identity").isEmpty() )
      mId = newMsg->headerField("X-KMail-Identity").stripWhiteSpace().toUInt();
    else
      mId = im->defaultIdentity().uoid();
    slotIdentityChanged( savedId );
  }


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

  // if these headers are present, the state of the message should be overruled
  if ( mMsg->headers().FindField( "X-KMail-SignatureActionEnabled" ) )
    mLastSignActionState = (mMsg->headerField( "X-KMail-SignatureActionEnabled" ) == "true");
  if ( mMsg->headers().FindField( "X-KMail-EncryptActionEnabled" ) )
    mLastEncryptActionState = (mMsg->headerField( "X-KMail-EncryptActionEnabled" ) == "true");

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
    setTransport( transport );

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
  if ( partNode * n = root->findType( DwMime::kTypeText, DwMime::kSubtypeHtml ) )
    if ( partNode * p = n->parentNode() )
      if ( p->hasType( DwMime::kTypeMultipart ) &&
           p->hasSubType( DwMime::kSubtypeAlternative ) )
        if ( mMsg->headerField( "X-KMail-Markup" ) == "true" ) {
          toggleMarkup( true );

          // get cte decoded body part
          mCharset = n->msgPart().charset();
          QCString bodyDecoded = n->msgPart().bodyDecoded();

          // respect html part charset
          const QTextCodec *codec = KMMsgBase::codecForName( mCharset );
          if ( codec ) {
            mEditor->setText( codec->toUnicode( bodyDecoded ) );
          } else {
            mEditor->setText( QString::fromLocal8Bit( bodyDecoded ) );
          }
        }

  if ( mCharset.isEmpty() )
    mCharset = mMsg->charset();
  if ( mCharset.isEmpty() )
    mCharset = mDefCharset;
  setCharset( mCharset );

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

  if( (GlobalSettings::self()->autoTextSignature()=="auto") && mayAutoSign ) {
    //
    // Espen 2000-05-16
    // Delay the signature appending. It may start a fileseletor.
    // Not user friendy if this modal fileseletor opens before the
    // composer.
    //
    //QTimer::singleShot( 200, this, SLOT(slotAppendSignature()) );
      if ( GlobalSettings::self()->prependSignature() ) {
        QTimer::singleShot( 0, this, SLOT(slotPrependSignature()) );
      } else {
        QTimer::singleShot( 0, this, SLOT(slotAppendSignature()) );
      }
  }
  setModified( isModified );

  // do this even for new messages
  mEditor->setCursorPositionFromStart( (unsigned int) mMsg->getCursorPos() );
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
           ( mRecipientsEditor && mRecipientsEditor->isModified() ) ||
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
    if ( mRecipientsEditor ) mRecipientsEditor->clearModified();
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
  if ( kmkernel->shuttingDown() || kapp->sessionSaving() )
    return true;
  if ( mComposer && mComposer->isPerformingSignOperation() ) // since the non-gpg-agent gpg plugin gets a passphrase using QDialog::exec()
    return false;                                            // the user can try to close the window, which destroys mComposer mid-call.

  if ( isModified() ) {
    bool istemplate = ( mFolder!=0 && mFolder->isTemplates() );
    const QString savebut = ( istemplate ?
                              i18n("Re&save as Template") :
                              i18n("&Save as Draft") );
    const QString savetext = ( istemplate ?
                               i18n("Resave this message in the Templates folder. "
                                    "It can then be used at a later time.") :
                               i18n("Save this message in the Drafts folder. "
                                    "It can then be edited and sent at a later time.") );

    const int rc = KMessageBox::warningYesNoCancel( this,
           i18n("Do you want to save the message for later or discard it?"),
           i18n("Close Composer"),
           KGuiItem(savebut, "filesave", QString::null, savetext),
           KStdGuiItem::discard() );
    if ( rc == KMessageBox::Cancel )
      return false;
    else if ( rc == KMessageBox::Yes ) {
      // doSend will close the window. Just return false from this method
      if ( istemplate ) {
        slotSaveTemplate();
      } else {
        slotSaveDraft();
      }
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
  kdDebug(5006) << "entering KMComposeWin::applyChanges" << endl;

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
//  return !Kpgp::Module::getKpgp() || Kpgp::Module::getKpgp()->encryptToSelf();
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readBoolEntry( "crypto-encrypt-to-self", true );
}

bool KMComposeWin::queryExit ()
{
  return true;
}

//-----------------------------------------------------------------------------
bool KMComposeWin::addAttach(const KURL aUrl)
{
  if ( !aUrl.isValid() ) {
    KMessageBox::sorry( this, i18n( "<qt><p>KMail could not recognize the location of the attachment (%1);</p>"
                                 "<p>you have to specify the full path if you wish to attach a file.</p></qt>" )
                        .arg( aUrl.prettyURL() ) );
    return false;
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
  mAttachJobs[job] = aUrl;
  connect(job, SIGNAL(result(KIO::Job *)),
          this, SLOT(slotAttachFileResult(KIO::Job *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotAttachFileData(KIO::Job *, const QByteArray &)));
  return true;
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
  lvi->setAttachmentSize(msgPart->decodedSize());

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
  mAtmModified = true;
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
    if ( GlobalSettings::self()->addresseeSelectorType() ==
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
  if ((forceDefault && GlobalSettings::self()->forceReplyCharset()) || aCharset.isEmpty())
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

  KFileDialog fdlg(QString::null, QString::null, this, 0, true);
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
  KURL attachURL;
  QMap<KIO::Job*, KURL>::iterator jit = mAttachJobs.find(job);
  bool attachURLfound = (jit != mAttachJobs.end());
  if (attachURLfound)
  {
    attachURL = jit.data();
    mAttachJobs.remove(jit);
  }
  if (job->error())
  {
    mMapAtmLoadData.remove(it);
    job->showErrorDialog();
    if (attachURLfound)
      emit attachmentAdded(attachURL, false);
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
    if (attachURLfound)
      emit attachmentAdded(attachURL, true);
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
  QValueList<int> allowedCTEs;
  if ( mimeType == "message/rfc822" ) {
    msgPart->setMessageBody( (*it).data );
    allowedCTEs << DwMime::kCte7bit;
    allowedCTEs << DwMime::kCte8bit;
  } else {
    msgPart->setBodyAndGuessCte((*it).data, allowedCTEs,
                               !kmkernel->msgSender()->sendQuotedPrintable());
    kdDebug(5006) << "autodetected cte: " << msgPart->cteStr() << endl;
  }
  int slash = mimeType.find( '/' );
  if( slash == -1 )
    slash = mimeType.length();
  msgPart->setTypeStr( mimeType.left( slash ).latin1() );
  msgPart->setSubtypeStr( mimeType.mid( slash + 1 ).latin1() );
  msgPart->setContentDisposition(QCString("attachment;\n\tfilename")
    + ( RFC2231encoded ? "*=" + encName : "=\"" + encName + '"' ) );

  mMapAtmLoadData.remove(it);

  msgPart->setCharset(partCharset);

  // show message part dialog, if not configured away (default):
  KConfigGroup composer(KMKernel::config(), "Composer");
  if ( GlobalSettings::self()->showMessagePartDialogOnAttach() ) {
    const KCursorSaver saver( QCursor::ArrowCursor );
    KMMsgPartDialogCompat dlg(mMainWidget);
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
      if (attachURLfound)
        emit attachmentAdded(attachURL, false);
      return;
    }
  }
  mAtmModified = true;
  if (msgPart->typeStr().lower() != "text") msgPart->setCharset(QCString());

  // add the new attachment to the list
  addAttach(msgPart);

  if (attachURLfound)
    emit attachmentAdded(attachURL, true);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertFile()
{
  KFileDialog fdlg(QString::null, QString::null, this, 0, true);
  fdlg.setOperationMode( KFileDialog::Opening );
  fdlg.okButton()->setText(i18n("&Insert"));
  fdlg.setCaption(i18n("Insert File"));
  fdlg.toolBar()->insertCombo(KMMsgBase::supportedEncodings(false), 4711,
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
      encodings.erase( encodings.fromLast() );
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
  if ( mFingerprint.isEmpty() || !Kleo::CryptoBackendFactory::instance()->openpgp() )
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
     mOpenWithId = mAttachMenu->insertItem(i18n("Open With..."), this,
                             SLOT(slotAttachOpenWith()));
     mViewId = mAttachMenu->insertItem(i18n("to view", "View"), this,
                             SLOT(slotAttachView()));
     mEditId = mAttachMenu->insertItem( i18n("Edit"), this, SLOT(slotAttachEdit()) );
     mEditWithId = mAttachMenu->insertItem( i18n("Edit With..."), this,
                                            SLOT(slotAttachEditWith()) );
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
  mAttachMenu->setItemEnabled( mOpenWithId, selectedCount > 0 );
  mAttachMenu->setItemEnabled( mViewId, selectedCount > 0 );
  mAttachMenu->setItemEnabled( mEditId, selectedCount == 1 );
  mAttachMenu->setItemEnabled( mEditWithId, selectedCount == 1 );
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

  KMMsgPartDialogCompat dlg(mMainWidget);
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
        "than the original. Do you want to keep the original one?" ), QString::null, i18n("Keep"), i18n("Compress") )
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
      openAttach( i, false );
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachOpenWith()
{
  int i = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      openAttach( i, true );
    }
  }
}

void KMComposeWin::slotAttachEdit()
{
  int i = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      editAttach( i, false );
    }
  }
}

void KMComposeWin::slotAttachEditWith()
{
  int i = 0;
  for ( QPtrListIterator<QListViewItem> it(mAtmItemList); *it; ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      editAttach( i, true );
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
  pname = msgPart->name().stripWhiteSpace();
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
void KMComposeWin::openAttach( int index, bool with )
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

  if ( with || !offer || mimetype->name() == "application/octet-stream" ) {
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

void KMComposeWin::editAttach(int index, bool openWith)
{
  KMMessagePart* msgPart = mAtmList.at(index);
  const QString contentTypeStr =
    ( msgPart->typeStr() + '/' + msgPart->subtypeStr() ).lower();

  KTempFile* atmTempFile = new KTempFile();
  mAtmTempList.append( atmTempFile );
  atmTempFile->setAutoDelete( true );
  atmTempFile->file()->writeBlock( msgPart->bodyDecodedBinary() );
  atmTempFile->file()->flush();


  KMail::EditorWatcher *watcher = new KMail::EditorWatcher( KURL( atmTempFile->name() ), contentTypeStr, openWith, this );
  connect( watcher, SIGNAL(editDone(KMail::EditorWatcher*)), SLOT(slotEditDone(KMail::EditorWatcher*)) );
  if ( watcher->start() ) {
    mEditorMap.insert( watcher, msgPart );
    mEditorTempFiles.insert( watcher, atmTempFile );
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
  KURL url( QApplication::clipboard()->text( QClipboard::Clipboard ) );
  if ( url.isValid() ) {
    addAttach(QApplication::clipboard()->text( QClipboard::Clipboard ) );
    return;
  }

  QMimeSource *mimeSource = QApplication::clipboard()->data();
  if ( QImageDrag::canDecode(mimeSource) ) {
    slotAttachPNGImageData(mimeSource->encodedData("image/png"));
  }
  else {
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
    rx = quotePrefix;
    s.replace(rx, "\n");

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

  if ( ::qt_cast<KEdit*>(fw) )
      static_cast<QTextEdit*>(fw)->undo();
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

  QKeyEvent k(QEvent::KeyPress, Key_C, 0, ControlButton);
  kapp->notify(fw, &k);
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotPaste()
{
  QWidget* fw = focusWidget();
  if (!fw) return;

  QMimeSource *mimeSource = QApplication::clipboard()->data();
  if ( mimeSource->provides("image/png") )  {
    slotAttachPNGImageData(mimeSource->encodedData("image/png"));
  } else if ( KURLDrag::canDecode( mimeSource ) ) {
        KURL::List urlList;
        if( KURLDrag::decode( mimeSource, urlList ) ) {
            const QString asText = i18n("Add as Text");
            const QString asAttachment = i18n("Add as Attachment");
            const QString text = i18n("Please select whether you want to insert the content as text into the editor, "
                    "or append the referenced file as an attachment.");
            const QString caption = i18n("Paste as text or attachment?");

            int id = KMessageBox::questionYesNoCancel( this, text, caption,
                    KGuiItem( asText ), KGuiItem( asAttachment) );
            switch ( id) {
              case KMessageBox::Yes:
                for ( KURL::List::Iterator it = urlList.begin();
                     it != urlList.end(); ++it ) {
                  mEditor->insert( (*it).url() );
                }
                break;
              case KMessageBox::No:
                for ( KURL::List::Iterator it = urlList.begin();
                     it != urlList.end(); ++it ) {
                  addAttach( *it );
                }
                break;
            }
        }
  } else {

#ifdef KeyPress
#undef KeyPress
#endif

    QKeyEvent k(QEvent::KeyPress, Key_V, 0, ControlButton);
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
  close(false);
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
  QString s( text );
  // Remove characters that show badly in most window decorations:
  // newlines tend to become boxes.
  if (text.isEmpty())
    setCaption("("+i18n("unnamed")+")");
  else setCaption( s.replace( QChar('\n'), ' ' ) );
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
    mEditor->setWordWrap( QTextEdit::FixedColumnWidth );
    mEditor->setWrapColumnOrWidth( GlobalSettings::self()->lineWrapWidth() );
  }
  else
  {
    mEditor->setWordWrap( QTextEdit::NoWrap );
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
bool KMComposeWin::validateAddresses( QWidget * parent, const QString & addresses )
{
  QString brokenAddress;
  KPIM::EmailParseResult errorCode = KMMessage::isValidEmailAddressList( KMMessage::expandAliases( addresses ), brokenAddress );
  if ( !( errorCode == KPIM::AddressOk || errorCode == KPIM::AddressEmpty ) ) {
    QString errorMsg( "<qt><p><b>" + brokenAddress +
                      "</b></p><p>" + KPIM::emailParseResultToString( errorCode ) +
                      "</p></qt>" );
    KMessageBox::sorry( parent, errorMsg, i18n("Invalid Email Address") );
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void KMComposeWin::doSend( KMail::MessageSender::SendMethod method,
                           KMComposeWin::SaveIn saveIn )
{
  if ( method != KMail::MessageSender::SendLater && kmkernel->isOffline() ) {
    KMessageBox::information( this,
                              i18n("KMail is currently in offline mode,"
                                   "your messages will be kept in the outbox until you go online."),
                              i18n("Online/Offline"), "kmailIsOffline" );
    mSendMethod = KMail::MessageSender::SendLater;
  } else {
    mSendMethod = method;
  }
  mSaveIn = saveIn;

  if ( saveIn == KMComposeWin::None ) {
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
    if ( to().isEmpty() )
    {
        if (  cc().isEmpty() && bcc().isEmpty()) {
          if ( mEdtTo ) mEdtTo->setFocus();
          KMessageBox::information( this,
                                i18n("You must specify at least one receiver,"
                                     "either in the To: field or as CC or as BCC.") );
          return;
        }
        else {
                if ( mEdtTo ) mEdtTo->setFocus();
                int rc =
                            KMessageBox::questionYesNo( this,
                                                        i18n("To field is missing."
                                                              "Send message anyway?"),
                                                        i18n("No To: specified") );
                if ( rc == KMessageBox::No ){
                   return;
                }
        }
    }

    // Validate the To:, CC: and BCC fields
    if ( !validateAddresses( this, to().stripWhiteSpace() ) ) {
      return;
    }

    if ( !validateAddresses( this, cc().stripWhiteSpace() ) ) {
      return;
    }

    if ( !validateAddresses( this, bcc().stripWhiteSpace() ) ) {
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

  mDisableBreaking = ( saveIn != KMComposeWin::None );

  const bool neverEncrypt = ( mDisableBreaking && GlobalSettings::self()->neverEncryptDrafts() )
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

  if (neverEncrypt && saveIn != KMComposeWin::None ) {
      // we can't use the state of the mail itself, to remember the
      // signing and encryption state, so let's add a header instead
    mMsg->setHeaderField( "X-KMail-SignatureActionEnabled", mSignAction->isChecked()? "true":"false" );
    mMsg->setHeaderField( "X-KMail-EncryptActionEnabled", mEncryptAction->isChecked()? "true":"false"  );
  } else {
    mMsg->removeHeaderField( "X-KMail-SignatureActionEnabled" );
    mMsg->removeHeaderField( "X-KMail-EncryptActionEnabled" );
  }


  kdDebug(5006) << "KMComposeWin::doSend() - calling applyChanges()"
                << endl;
  applyChanges( neverEncrypt );
}

bool KMComposeWin::saveDraftOrTemplate( const QString &folderName,
                                        KMMessage *msg )
{
  KMFolder *theFolder = 0, *imapTheFolder = 0;
  // get the draftsFolder
  if ( !folderName.isEmpty() ) {
    theFolder = kmkernel->folderMgr()->findIdString( folderName );
    if ( theFolder == 0 )
      // This is *NOT* supposed to be "imapDraftsFolder", because a
      // dIMAP folder works like a normal folder
      theFolder = kmkernel->dimapFolderMgr()->findIdString( folderName );
    if ( theFolder == 0 )
      imapTheFolder = kmkernel->imapFolderMgr()->findIdString( folderName );
    if ( !theFolder && !imapTheFolder ) {
      const KPIM::Identity & id = kmkernel->identityManager()
        ->identityForUoidOrDefault( msg->headerField( "X-KMail-Identity" ).stripWhiteSpace().toUInt() );
      KMessageBox::information( 0,
                                i18n("The custom drafts or templates folder for "
                                     "identify \"%1\" does not exist (anymore); "
                                     "therefore, the default drafts or templates "
                                     "folder will be used.")
                                .arg( id.identityName() ) );
    }
  }
  if ( imapTheFolder && imapTheFolder->noContent() )
    imapTheFolder = 0;

  if ( theFolder == 0 ) {
    theFolder = ( mSaveIn==KMComposeWin::Drafts ?
                  kmkernel->draftsFolder() : kmkernel->templatesFolder() );
  } else {
    theFolder->open();
  }
  kdDebug(5006) << k_funcinfo << "theFolder=" << theFolder->name() << endl;
  if ( imapTheFolder )
    kdDebug(5006) << k_funcinfo << "imapTheFolder=" << imapTheFolder->name() << endl;

  bool sentOk = !( theFolder->addMsg( msg ) );

  // Ensure the message is correctly and fully parsed
  theFolder->unGetMsg( theFolder->count() - 1 );
  msg = theFolder->getMsg( theFolder->count() - 1 );
  // Does that assignment needs to be propagated out to the caller?
  // Assuming the send is OK, the iterator is set to 0 immediately afterwards.
  if ( imapTheFolder ) {
    // move the message to the imap-folder and highlight it
    imapTheFolder->moveMsg( msg );
    (static_cast<KMFolderImap*>( imapTheFolder->storage() ))->getFolder();
  }

  return sentOk;
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

    if ( mSaveIn==KMComposeWin::Drafts ) {
      sentOk = saveDraftOrTemplate( (*it)->drafts(), (*it) );
    } else if ( mSaveIn==KMComposeWin::Templates ) {
      sentOk = saveDraftOrTemplate( (*it)->templates(), (*it) );
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
    doSend( KMail::MessageSender::SendLater, KMComposeWin::Drafts );
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSaveTemplate() {
  if ( mEditor->checkExternalEditorFinished() )
    doSend( KMail::MessageSender::SendLater, KMComposeWin::Templates );
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSendNowVia( int item )
{
  QStringList availTransports= KMail::TransportManager::transportNames();
  QString customTransport = availTransports[ item ];

  mTransport->setCurrentText( customTransport );
  slotSendNow();
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSendLaterVia( int item )
{
  QStringList availTransports= KMail::TransportManager::transportNames();
  QString customTransport = availTransports[ item ];

  mTransport->setCurrentText( customTransport );
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
    insertSignature();
}

//----------------------------------------------------------------------------
void KMComposeWin::slotPrependSignature()
{
    insertSignature( false );
}

//----------------------------------------------------------------------------
void KMComposeWin::slotInsertSignatureAtCursor()
{
    insertSignature( false, mEditor->currentLine() );
}

//----------------------------------------------------------------------------
void KMComposeWin::insertSignature( bool append, int pos )
{
   bool mod = mEditor->isModified();

   const KPIM::Identity &ident =
     kmkernel->identityManager()->
     identityForUoidOrDefault( mIdentity->currentIdentity() );

   mOldSigText = GlobalSettings::self()->prependSignature()? ident.signature().rawText() : ident.signatureText();

   if( !mOldSigText.isEmpty() )
   {
      mEditor->sync();
      if ( append ) {
       mEditor->append(mOldSigText);
    } else {
       mEditor->insertAt(mOldSigText, pos, 0);
    }
    mEditor->update();
    mEditor->setModified(mod);
    // for append and prepend, move the cursor to 0,0, for insertAt,
    // keep it in the same row, but move to first column
    mEditor->setCursorPosition( pos, 0 );
    if ( !append && pos == 0 )
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
  QRegExp squeeze( "[\t ]+" );
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
  } else { // markup is to be turned off
    kdDebug(5006) << "setting PlainText editor" << endl;
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
  if ( mEditor->autoSpellChecking(on) == -1 ) {
    mAutoSpellCheckingAction->setChecked(false); // set it to false again
  }

  QString temp;
  if ( on )
    temp = i18n( "Spellcheck: on" );
  else
    temp = i18n( "Spellcheck: off" );
  statusBar()->changeItem( temp, 3 );
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
//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdateSignatureActions()
{
  //Check if an identity has signature or not and turn on/off actions in the
  //edit menu accordingly.
  const KPIM::Identity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  QString sig = ident.signatureText();

  if ( sig.isEmpty() ) {
     mAppendSignatureAction->setEnabled( false );
     mPrependSignatureAction->setEnabled( false );
     mInsertSignatureAction->setEnabled( false );
  }
  else {
      mAppendSignatureAction->setEnabled( true );
      mPrependSignatureAction->setEnabled( true );
      mInsertSignatureAction->setEnabled( true );
  }
}

void KMComposeWin::polish()
{
  // Ensure the html toolbar is appropriately shown/hidden
  markupAction->setChecked(mHtmlMarkup);
  if (mHtmlMarkup)
    toolBar("htmlToolBar")->show();
  else
    toolBar("htmlToolBar")->hide();
  KMail::Composer::polish();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheckDone(int result)
{
  kdDebug(5006) << "spell check complete: result = " << result << endl;
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

  //Turn on/off signature actions if identity has no signature.
  slotUpdateSignatureActions();

  if( !ident.fullEmailAddr().isNull() )
    mEdtFrom->setText(ident.fullEmailAddr());
  // make sure the From field is shown if it does not contain a valid email address
  if ( KPIM::getFirstEmailAddress( from() ).isEmpty() )
    mShowHeaders |= HDR_FROM;
  if ( mEdtReplyTo ) mEdtReplyTo->setText(ident.replyToAddr());

  if ( mRecipientsEditor ) {
    // remove BCC of old identity and add BCC of new identity (if they differ)
    const KPIM::Identity & oldIdentity =
      kmkernel->identityManager()->identityForUoidOrDefault( mId );
    if ( oldIdentity.bcc() != ident.bcc() ) {
      mRecipientsEditor->removeRecipient( oldIdentity.bcc(), Recipient::Bcc );
      mRecipientsEditor->addRecipient( ident.bcc(), Recipient::Bcc );
      mRecipientsEditor->setFocusBottom();
    }
  }

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
      transp = GlobalSettings::self()->defaultTransport();
    }
    else
      mMsg->setHeaderField("X-KMail-Transport", transp);
    setTransport( transp );
  }

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  if ( !mBtnFcc->isChecked() ) {
      setFcc( ident.fcc() );
  }

  QString edtText = mEditor->text();

  if ( mOldSigText.isEmpty() ) {
    const KPIM::Identity &id =
      kmkernel->
      identityManager()->
      identityForUoidOrDefault( mMsg->headerField( "X-KMail-Identity" ).
                                stripWhiteSpace().toUInt() );
    mOldSigText = id.signatureText();
  }

  // try to truncate the old sig
  // First remove any trailing whitespace
  while ( !edtText.isEmpty() && edtText[edtText.length()-1].isSpace() )
    edtText.truncate( edtText.length() - 1 );
  // From the sig too, just in case
  while ( !mOldSigText.isEmpty() && mOldSigText[mOldSigText.length()-1].isSpace() )
    mOldSigText.truncate( mOldSigText.length() - 1 );

  if( edtText.endsWith( mOldSigText ) )
    edtText.truncate( edtText.length() - mOldSigText.length() );

  // now append the new sig
  mOldSigText = ident.signatureText();
  if( ( !mOldSigText.isEmpty() ) &&
      ( GlobalSettings::self()->autoTextSignature() == "auto" ) ) {
    edtText.append( mOldSigText );
  }
  mEditor->setText( edtText );

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
  mKSpellConfig.layout()->setMargin( KDialog::marginHint() );

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
  if ( hasMessage ) {
    if( mMsg->getCursorPos() ) {
      mEditor->setCursorPositionFromStart( (unsigned int) mMsg->getCursorPos() );
    } else {
      mEditor->setCursorPosition( 1, 0 );
    }
  }
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
      mAutoSaveTimer = new QTimer( this, "mAutoSaveTimer" );
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
    kdDebug(5006) << k_funcinfo << "deleting autosave file "
                  << mAutoSaveFilename << endl;
    KMFolderMaildir::removeFile( KMKernel::localDataPath() + "autosave",
                                 mAutoSaveFilename );
    mAutoSaveFilename = QString();
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
  }else
    mRecipientsEditor->setCompletionMode( mode );
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
  // TODO: need to handle templates here?
  if ( (mFolder) && (folder->idString() == mFolder->idString()) )
  {
    mFolder = kmkernel->draftsFolder();
    kdDebug(5006) << "restoring drafts to " << mFolder->idString() << endl;
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
  QFont fontTemp = f;
  fontTemp.setBold( true );
  fontTemp.setItalic( true );
  QFontInfo fontInfo( fontTemp );

  if ( fontInfo.bold() ) {
    textBoldAction->setChecked( f.bold() );
    textBoldAction->setEnabled( true ) ;
  } else {
    textBoldAction->setEnabled( false );
  }

  if ( fontInfo.italic() ) {
    textItalicAction->setChecked( f.italic() );
    textItalicAction->setEnabled( true ) ;
  } else {
    textItalicAction->setEnabled( false );
  }

  textUnderAction->setChecked( f.underline() );

  fontAction->setFont( f.family() );
  fontSizeAction->setFontSize( f.pointSize() );
}

void KMComposeWin::alignmentChanged( int a )
{
    //toggleMarkup();
    alignLeftAction->setChecked( ( a == AlignAuto ) || ( a & AlignLeft ) );
    alignCenterAction->setChecked( ( a & AlignHCenter ) );
    alignRightAction->setChecked( ( a & AlignRight ) );
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

  STD_NAMESPACE_PREFIX auto_ptr<Kleo::SpecialJob> job( chiasmus->specialJob( "x-obtain-keys", QMap<QString,QVariant>() ) );
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

void KMComposeWin::slotEditDone(KMail::EditorWatcher * watcher)
{
  kdDebug(5006) << k_funcinfo << endl;
  KMMessagePart *part = mEditorMap[ watcher ];
  KTempFile *tf = mEditorTempFiles[ watcher ];
  mEditorMap.remove( watcher );
  mEditorTempFiles.remove( watcher );
  if ( !watcher->fileChanged() )
    return;

  tf->file()->reset();
  QByteArray data = tf->file()->readAll();
  part->setBodyEncodedBinary( data );
}

