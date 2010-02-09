/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Based on KMail code by:
 * Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kmcomposewin.h"

#include "messagehelper.h"

// KDEPIM includes
#include "kleo/cryptobackendfactory.h"
#include "kleo/exportjob.h"
#include "kleo/specialjob.h"
#include <libkpgp/kpgpblock.h>
#include <libkleo/ui/progressdialog.h>
#include <libkleo/ui/keyselectiondialog.h>

#include <kpimutils/email.h>

#include <messagecomposer/composer.h>
#include <messagecomposer/globalpart.h>
#include <messagecomposer/infopart.h>
#include <messagecomposer/textpart.h>

// LIBKDEPIM includes
#include <messagecore/attachmentpart.h>
#include <libkdepim/kaddrbookexternal.h>
#include <libkdepim/recentaddresses.h>

using KPIM::RecentAddresses;

// KDEPIMLIBS includes
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identitycombo.h>
#include <kpimidentities/identity.h>
#include <mailtransport/transportcombobox.h>
#include <mailtransport/transportmanager.h>
using MailTransport::TransportManager;
#include <mailtransport/transport.h>
using MailTransport::Transport;
#include <mailtransport/messagequeuejob.h>
#include <mailtransport/sentbehaviourattribute.h>
#include <kmime/kmime_codecs.h>
#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

// KMail includes
#include "attachmentcollector.h"
#include "attachmentcontroller.h"
#include "attachmentmodel.h"
#include "attachmentview.h"
#include "messageviewer/chiasmuskeyselector.h"
#include "codecaction.h"
#include "kleo_util.h"
#include "kmcommands.h"
#include "kmcomposereditor.h"
#include "kmkernel.h"
#include "globalsettings.h"
#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
//#include "mailcomposeradaptor.h" // TODO port all D-Bus stuff...
#include "objecttreeparser.h"
#include "recipientseditor.h"
#include "messageviewer/stl_util.h"
#include "messageviewer/util.h"
#include "messagecore/stringutil.h"
#include "util.h"
#include "templateparser.h"
#include "messagehelper.h"
#include "keyresolver.h"
#include "templatesconfiguration_kfg.h"
#include "foldercollectionmonitor.h"

using Sonnet::DictionaryComboBox;
using KMail::TemplateParser;

// KDELIBS includes
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kapplication.h>

#include <messageviewer/kcursorsaver.h>
#include <messageviewer/objecttreeparser.h>
#include <messageviewer/nodehelper.h>

#include <kcharsets.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kencodingfiledialog.h>
#include <kinputdialog.h>
#include <kmenu.h>
#include <kmimetypetrader.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <krecentfilesaction.h>
#include <krun.h>
#include <ksavefile.h>
#include <kshortcutsdialog.h>
#include <kstandarddirs.h>
#include <kstandardshortcut.h>
#include <kstatusbar.h>
#include <ktempdir.h>
#include <ktoggleaction.h>
#include <ktoolbar.h>
#include <ktoolinvocation.h>
#include <kwindowsystem.h>
#include <kzip.h>
#include <sonnet/dictionarycombobox.h>
#include <kencodingprober.h>
#include <kio/jobuidelegate.h>
#include <kio/scheduler.h>

// Qt includes
#include <QBuffer>
#include <QClipboard>
#include <QEvent>
#include <QSplitter>
#include <QTextList>
#include <QUuid>
#include <QDir>

// System includes
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <memory>

#include <akonadi/collectioncombobox.h>
#include <akonadi/changerecorder.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchjob.h>

#include <messageviewer/objecttreeemptysource.h>
// MOC
#include "kmcomposewin.moc"

#include "snippetwidget.h"

KMail::Composer *KMail::makeComposer( const KMime::Message::Ptr &msg, Composer::TemplateContext context,
                                      uint identity, const QString & textSelection,
                                      const QString & customTemplate ) {
  return KMComposeWin::create( msg, context, identity, textSelection, customTemplate );
}

KMail::Composer *KMComposeWin::create( const KMime::Message::Ptr &msg, Composer::TemplateContext context,
                                       uint identity, const QString & textSelection,
                                       const QString & customTemplate ) {
  return new KMComposeWin( msg, context, identity, textSelection, customTemplate );
}

int KMComposeWin::s_composerNumber = 0;

//-----------------------------------------------------------------------------
KMComposeWin::KMComposeWin( const KMime::Message::Ptr &aMsg, Composer::TemplateContext context, uint id,
                            const QString & textSelection, const QString & customTemplate )
  : KMail::Composer( "kmail-composer#" ),
    mDone( false ),
    //mAtmModified( false ),
    mTextSelection( textSelection ),
    mCustomTemplate( customTemplate ),
    mSigningAndEncryptionExplicitlyDisabled( false ),
    mFolder( Akonadi::Collection( -1 ) ),
    mForceDisableHtml( false ),
    mId( id ),
    mContext( context ),
    mSignAction( 0 ), mEncryptAction( 0 ), mRequestMDNAction( 0 ),
    mUrgentAction( 0 ), mAllFieldsAction( 0 ), mFromAction( 0 ),
    mReplyToAction( 0 ), mSubjectAction( 0 ),
    mIdentityAction( 0 ), mTransportAction( 0 ), mFccAction( 0 ),
    mWordWrapAction( 0 ), mFixedFontAction( 0 ), mAutoSpellCheckingAction( 0 ),
    mDictionaryAction( 0 ), mSnippetAction( 0 ),
    //mEncodingAction( 0 ),
    mCodecAction( 0 ),
    mCryptoModuleAction( 0 ),
    mEncryptChiasmusAction( 0 ),
    mEncryptWithChiasmus( false ),
    mDummyComposer( 0 ),
    mPendingQueueJobs( 0 ),
    mPendingCreateItemJobs( 0 ),
    mLabelWidth( 0 ),
    mAutoSaveTimer( 0 ), mAutoSaveErrorShown( false ),
    mSignatureStateIndicator( 0 ), mEncryptionStateIndicator( 0 ),
    mPreventFccOverwrite( false ),
    mIgnoreStickyFields( false )
{
  //(void) new MailcomposerAdaptor( this );
  mdbusObjectPath = "/Composer_" + QString::number( ++s_composerNumber );
  //QDBusConnection::sessionBus().registerObject( mdbusObjectPath, this );

  if ( kmkernel->xmlGuiInstance().isValid() ) {
    setComponentData( kmkernel->xmlGuiInstance() );
  }
  mMainWidget = new QWidget( this );
  // splitter between the headers area and the actual editor
  mHeadersToEditorSplitter = new QSplitter( Qt::Vertical, mMainWidget );
  mHeadersToEditorSplitter->setObjectName( "mHeadersToEditorSplitter" );
  mHeadersToEditorSplitter->setChildrenCollapsible( false );
  mHeadersArea = new QWidget( mHeadersToEditorSplitter );
  mHeadersArea->setSizePolicy( mHeadersToEditorSplitter->sizePolicy().horizontalPolicy(),
                               QSizePolicy::Expanding );
  mHeadersToEditorSplitter->addWidget( mHeadersArea );
  QList<int> defaultSizes;
  defaultSizes << 0;
  mHeadersToEditorSplitter->setSizes( defaultSizes );
  QVBoxLayout *v = new QVBoxLayout( mMainWidget );
  v->addWidget( mHeadersToEditorSplitter );
  mIdentity = new KPIMIdentities::IdentityCombo( kmkernel->identityManager(),
                                                 mHeadersArea );
  mDictionaryCombo = new DictionaryComboBox( mHeadersArea );
  mFcc = new Akonadi::CollectionComboBox( mHeadersArea );
  mFcc->setMimeTypeFilter( QStringList()<<FolderCollectionMonitor::mimetype() );
#if 0 //Port to akonadi
  mFcc->showOutboxFolder( false );
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  mTransport = new MailTransport::TransportComboBox( mHeadersArea );
  mEdtFrom = new KMLineEdit( false, mHeadersArea, "fromLine" );
  mEdtReplyTo = new KMLineEdit( true, mHeadersArea, "replyToLine" );
  connect( mEdtReplyTo, SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
          SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)) );

  mRecipientsEditor = new RecipientsEditor( mHeadersArea );
  connect( mRecipientsEditor,
           SIGNAL( completionModeChanged( KGlobalSettings::Completion ) ),
           SLOT( slotCompletionModeChanged( KGlobalSettings::Completion ) ) );
  connect( mRecipientsEditor, SIGNAL(sizeHintChanged()), SLOT(recipientEditorSizeHintChanged()) );

  mEdtSubject = new KMLineEdit( false, mHeadersArea, "subjectLine" );
  mLblIdentity = new QLabel( i18n("&Identity:"), mHeadersArea );
  mDictionaryLabel = new QLabel( i18n("&Dictionary:"), mHeadersArea );
  mLblFcc = new QLabel( i18n("&Sent-Mail folder:"), mHeadersArea );
  mLblTransport = new QLabel( i18n("&Mail transport:"), mHeadersArea );
  mLblFrom = new QLabel( i18nc("sender address field", "&From:"), mHeadersArea );
  mLblReplyTo = new QLabel( i18n("&Reply to:"), mHeadersArea );
  mLblSubject = new QLabel( i18nc("@label:textbox Subject of email.", "S&ubject:"), mHeadersArea );
  QString sticky = i18nc("@option:check Sticky identity.", "Sticky");
  mBtnIdentity = new QCheckBox( sticky, mHeadersArea );
  mBtnFcc = new QCheckBox( sticky, mHeadersArea );
  mBtnTransport = new QCheckBox( sticky, mHeadersArea );
  mShowHeaders = GlobalSettings::self()->headers();
  mDone = false;
  mGrid = 0;
  //mAtmListView = 0;
  //mAtmModified = false;
  mFixedFontAction = 0;
  // the attachment view is separated from the editor by a splitter
  mSplitter = new QSplitter( Qt::Vertical, mMainWidget );
  mSplitter->setObjectName( "mSplitter" );
  mSplitter->setChildrenCollapsible( false );
  mSnippetSplitter = new QSplitter( Qt::Horizontal, mSplitter );
  mSnippetSplitter->setObjectName( "mSnippetSplitter" );
  mSnippetSplitter->setChildrenCollapsible( false );
  mSplitter->addWidget( mSnippetSplitter );

  QWidget *editorAndCryptoStateIndicators = new QWidget( mSplitter );
  QVBoxLayout *vbox = new QVBoxLayout( editorAndCryptoStateIndicators );
  vbox->setMargin(0);
  QHBoxLayout *hbox = new QHBoxLayout();
  {
    hbox->setMargin(0);
    mSignatureStateIndicator = new QLabel( editorAndCryptoStateIndicators );
    mSignatureStateIndicator->setAlignment( Qt::AlignHCenter );
    hbox->addWidget( mSignatureStateIndicator );

    KConfigGroup reader( KMKernel::config(), "Reader" );

    // Get the colors for the label
    QPalette p( mSignatureStateIndicator->palette() );
    KColorScheme scheme( QPalette::Active, KColorScheme::View );
    QColor defaultSignedColor =  // pgp signed
        scheme.background( KColorScheme::PositiveBackground ).color();
    QColor defaultEncryptedColor( 0x00, 0x80, 0xFF ); // light blue // pgp encrypted
    QColor signedColor = defaultSignedColor;
    QColor encryptedColor = defaultEncryptedColor;
    if ( !GlobalSettings::self()->useDefaultColors() ) {
      signedColor = reader.readEntry( "PGPMessageOkKeyOk", defaultSignedColor );
      encryptedColor = reader.readEntry( "PGPMessageEncr", defaultEncryptedColor );
    }

    p.setColor( QPalette::Window, signedColor );
    mSignatureStateIndicator->setPalette( p );
    mSignatureStateIndicator->setAutoFillBackground( true );

    mEncryptionStateIndicator = new QLabel( editorAndCryptoStateIndicators );
    mEncryptionStateIndicator->setAlignment( Qt::AlignHCenter );
    hbox->addWidget( mEncryptionStateIndicator );
    p.setColor( QPalette::Window, encryptedColor);
    mEncryptionStateIndicator->setPalette( p );
    mEncryptionStateIndicator->setAutoFillBackground( true );
  }

  mEditor = new KMComposerEditor( this, editorAndCryptoStateIndicators );
  connect( mEditor, SIGNAL(textChanged()),
           this, SLOT(slotEditorTextChanged()) );
  vbox->addLayout( hbox );
  vbox->addWidget( mEditor );
  mSnippetSplitter->insertWidget( 0, editorAndCryptoStateIndicators );
  mSnippetSplitter->setOpaqueResize( true );

  mHeadersToEditorSplitter->addWidget( mSplitter );
  mEditor->setAcceptDrops( true );
  connect( mDictionaryCombo, SIGNAL( dictionaryChanged( const QString & ) ),
           mEditor, SLOT( setSpellCheckingLanguage( const QString & ) ) );
  connect( mEditor, SIGNAL( languageChanged(const QString &) ),
           this, SLOT( slotLanguageChanged(const QString&) ) );
  connect( mEditor, SIGNAL( spellCheckStatus(const QString &)),
           this, SLOT( slotSpellCheckingStatus(const QString &) ) );

  mSnippetWidget = new SnippetWidget( mEditor, actionCollection(), mSnippetSplitter );
  mSnippetWidget->setVisible( GlobalSettings::self()->showSnippetManager() );
  mSnippetSplitter->addWidget( mSnippetWidget );
  mSnippetSplitter->setCollapsible( 0, false );

  mSplitter->setOpaqueResize( true );

  mBtnIdentity->setWhatsThis( GlobalSettings::self()->stickyIdentityItem()->whatsThis() );
  mBtnFcc->setWhatsThis( GlobalSettings::self()->stickyFccItem()->whatsThis() );
  mBtnTransport->setWhatsThis( GlobalSettings::self()->stickyTransportItem()->whatsThis() );

  setCaption( i18n("Composer") );
  setMinimumSize( 200, 200 );

  mBtnIdentity->setFocusPolicy( Qt::NoFocus );
  mBtnFcc->setFocusPolicy( Qt::NoFocus );
  mBtnTransport->setFocusPolicy( Qt::NoFocus );

  mAttachmentModel = new KMail::AttachmentModel( this );
  mAttachmentView = new KMail::AttachmentView( mAttachmentModel, mSplitter );
  mAttachmentView->hideIfEmpty();
  mAttachmentController = new KMail::AttachmentController( mAttachmentModel, mAttachmentView, this );

  readConfig();
  setupStatusBar();
  setupActions();
  setupEditor();
  rethinkFields();
  slotUpdateSignatureAndEncrypionStateIndicators();

  applyMainWindowSettings( KMKernel::config()->group( "Composer") );

  connect( mEdtSubject, SIGNAL(textChanged(const QString&)),
           SLOT(slotUpdWinTitle(const QString&)) );
  connect( mIdentity, SIGNAL(identityChanged(uint)),
           SLOT(slotIdentityChanged(uint)) );
  connect( kmkernel->identityManager(), SIGNAL(changed(uint)),
           SLOT(slotIdentityChanged(uint)) );

  connect( mEdtFrom, SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
           SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)) );
  connect( kmkernel->monitor(), SIGNAL(collectionRemoved(const Akonadi::Collection&)),
           SLOT(slotFolderRemoved(const Akonadi::Collection&)) );
  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );

  mMainWidget->resize( 480, 510 );
  setCentralWidget( mMainWidget );

  if ( GlobalSettings::self()->useHtmlMarkup() )
    enableHtml();

  if ( GlobalSettings::self()->useExternalEditor() ) {
    mEditor->setUseExternalEditor( true );
    mEditor->setExternalEditorPath( GlobalSettings::self()->externalEditor() );
  }

  initAutoSave();

  if ( aMsg ) {
    setMsg( aMsg );
  }

  mRecipientsEditor->setFocus();
  mEditor->updateActionStates(); // set toolbar buttons to correct values

  mDone = true;

  mDummyComposer = new Message::Composer( this );
  mDummyComposer->globalPart()->setParentWidgetForGui( this );
}

//-----------------------------------------------------------------------------
KMComposeWin::~KMComposeWin()
{
  writeConfig();

  // When we have a collection set, store the message back to that collection.
  // Note that when we save the message or sent it, mFolder is set back to 0.
  // So this for example kicks in when opening a draft and then closing the window.
  if ( mFolder.isValid() && mMsg ) {
    Akonadi::Item item;
    item.setPayload( mMsg );
    item.setMimeType( "message/rfc822" );
    new Akonadi::ItemCreateJob( item, mFolder );
    // FIXME: listen to the result signal. The whole thing needs to be moved
    //        out of the destructor for this
  }

#if 0
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.begin();
  while ( it != mMapAtmLoadData.end() ) {
    KIO::Job *job = it.key();
    mMapAtmLoadData.erase( it );
    job->kill();
    it = mMapAtmLoadData.begin();
  }

  qDeleteAll( mAtmList );
  qDeleteAll( mAtmTempList );
#endif
  //deleteAll( mComposedMessages );

  foreach ( KTempDir *const dir, mTempDirs ) {
    delete dir;
  }
}


QString KMComposeWin::dbusObjectPath() const
{
  return mdbusObjectPath;
}

void KMComposeWin::slotEditorTextChanged()
{
  const bool textIsNotEmpty = !mEditor->document()->isEmpty();
  mFindText->setEnabled( textIsNotEmpty );
  mFindNextText->setEnabled( textIsNotEmpty );
  mReplaceText->setEnabled( textIsNotEmpty );
  mSelectAll->setEnabled( textIsNotEmpty );
}

//-----------------------------------------------------------------------------
void KMComposeWin::send( int how )
{
  switch ( how ) {
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
void KMComposeWin::addAttachmentsAndSend( const KUrl::List &urls, const QString &comment, int how )
{
  kDebug() << "addAttachment and sending!";
  for( int i =0; i < urls.count(); ++i ) {
    addAttachment( urls[i], comment );
  }

  send( how );
#if 0
  Q_UNUSED( comment );
  if ( urls.isEmpty() ) {
    send( how );
    return;
  }
  mAttachFilesSend = how;
  mAttachFilesPending = urls;
  connect( this, SIGNAL(attachmentAdded(const KUrl &, bool)), SLOT(slotAttachedFile(const KUrl &)) );
  for ( int i = 0, count = urls.count(); i < count; ++i ) {
    if ( !addAttach( urls[i] ) ) {
      mAttachFilesPending.removeAt( mAttachFilesPending.indexOf( urls[i] ) ); // only remove one copy of the url
    }
  }

  if ( mAttachFilesPending.isEmpty() && mAttachFilesSend == how ) {
    send( mAttachFilesSend );
    mAttachFilesSend = -1;
  }
#endif
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttachment( const KUrl &url, const QString &comment )
{
  Q_UNUSED( comment );
  kDebug() << "adding attachment with url:" << url;
  mAttachmentController->addAttachment( url );
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttachment( const QString &name,
                                  const QByteArray &cte,
                                  const QByteArray &data,
                                  const QByteArray &type,
                                  const QByteArray &subType,
                                  const QByteArray &paramAttr,
                                  const QString &paramValue,
                                  const QByteArray &contDisp )
{
  kDebug() << "implement me";

  KPIM::AttachmentPart::Ptr attachment = KPIM::AttachmentPart::Ptr( new KPIM::AttachmentPart() );
  if( !data.isEmpty() ) {
    attachment->setName( name );
    attachment->setData( data );
    attachment->setMimeType( type );
    // TODO what about the other fields?

    mAttachmentController->addAttachment( attachment);
  }
#if 0
  Q_UNUSED( cte );

  if ( !data.isEmpty() ) {
    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName( name );
    if ( type == "message" && subType == "rfc822" ) {
      msgPart->setMessageBody( data );
    } else {
      QList<int> dummy;
      msgPart->setBodyAndGuessCte( data, dummy,
                                   kmkernel->msgSender()->sendQuotedPrintable() );
    }
    msgPart->setTypeStr( type );
    msgPart->setSubtypeStr( subType );
    msgPart->setParameter( paramAttr, paramValue );
    msgPart->setContentDisposition( contDisp );
    mAttachmentController->addAttachment( msgPart );
  }
#endif
}

//-----------------------------------------------------------------------------
void KMComposeWin::readConfig( bool reload /* = false */ )
{
  mBtnIdentity->setChecked( GlobalSettings::self()->stickyIdentity() );
  if (mBtnIdentity->isChecked()) {
    mId = ( GlobalSettings::self()->previousIdentity() != 0 ) ?
      GlobalSettings::self()->previousIdentity() : mId;
  }
  mBtnFcc->setChecked( GlobalSettings::self()->stickyFcc() );
  mBtnTransport->setChecked( GlobalSettings::self()->stickyTransport() );
  QString currentTransport = GlobalSettings::self()->currentTransport();

  mEdtFrom->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
  mRecipientsEditor->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
  mEdtReplyTo->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );

  if ( GlobalSettings::self()->useDefaultFonts() ) {
    mBodyFont = KGlobalSettings::generalFont();
    mFixedFont = KGlobalSettings::fixedFont();
  } else {
    mBodyFont = GlobalSettings::self()->composerFont();
    mFixedFont = GlobalSettings::self()->fixedFont();
  }

  slotUpdateFont();
  mEdtFrom->setFont( mBodyFont );
  mEdtReplyTo->setFont( mBodyFont );
  mEdtSubject->setFont( mBodyFont );
  mRecipientsEditor->setEditFont( mBodyFont );

  if ( !reload ) {
    QSize siz = GlobalSettings::self()->composerSize();
    if ( siz.width() < 200 ) {
      siz.setWidth( 200 );
    }
    if ( siz.height() < 200 ) {
      siz.setHeight( 200 );
    }
    resize( siz );

    if ( !GlobalSettings::self()->snippetSplitterPosition().isEmpty() ) {
      mSnippetSplitter->setSizes( GlobalSettings::self()->snippetSplitterPosition() );
    } else {
      QList<int> defaults;
      defaults << (int)(width() * 0.8) << (int)(width() * 0.2);
      mSnippetSplitter->setSizes( defaults );
    }
  }

  mIdentity->setCurrentIdentity( mId );

  kDebug() << mIdentity->currentIdentityName();
  const KPIMIdentities::Identity & ident =
    kmkernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );

  mDictionaryCombo->setCurrentByDictionaryName( ident.dictionary() );

  if ( mBtnTransport->isChecked() && !currentTransport.isEmpty() ) {
    Transport *transport =
        TransportManager::self()->transportByName( currentTransport );
    if ( transport )
      mTransport->setCurrentTransport( transport->id() );
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
void KMComposeWin::writeConfig( void )
{
  GlobalSettings::self()->setHeaders( mShowHeaders );
  GlobalSettings::self()->setStickyFcc( mBtnFcc->isChecked() );
  if ( !mIgnoreStickyFields ) {
    GlobalSettings::self()->setCurrentTransport( mTransport->currentText() );
    GlobalSettings::self()->setStickyTransport( mBtnTransport->isChecked() );
    GlobalSettings::self()->setStickyIdentity( mBtnIdentity->isChecked() );
    GlobalSettings::self()->setPreviousIdentity( mIdentity->currentIdentity() );
  }
  GlobalSettings::self()->setPreviousFcc( QString::number(mFcc->currentCollection().id()) );
  GlobalSettings::self()->setAutoSpellChecking(
                                               mAutoSpellCheckingAction->isChecked() );
  GlobalSettings::self()->setUseFixedFont( mFixedFontAction->isChecked() );
  if ( !mForceDisableHtml )
    GlobalSettings::self()->setUseHtmlMarkup( mEditor->textMode() == KMeditor::Rich );
  GlobalSettings::self()->setComposerSize( size() );
  GlobalSettings::self()->setShowSnippetManager( mSnippetAction->isChecked() );

  saveMainWindowSettings( KMKernel::config()->group( "Composer" ) );
  if ( mSnippetAction->isChecked() )
    GlobalSettings::setSnippetSplitterPosition( mSnippetSplitter->sizes() );

  // make sure config changes are written to disk, cf. bug 127538
  GlobalSettings::self()->writeConfig();
}

//-----------------------------------------------------------------------------
void KMComposeWin::autoSaveMessage()
{
  kDebug() << "Autosaving message";

  if ( mAutoSaveTimer ) {
    mAutoSaveTimer->stop();
  }

  if( !mComposers.isEmpty() ) {
    // This may happen if e.g. the autosave timer calls applyChanges.
    kDebug() << "Called while composer active; ignoring.";
    return;
  }

  // Construct a KMime::Message to be autosaved:
  Message::Composer* composer = new Message::Composer;
  fillGlobalPart( composer->globalPart() );
  fillTextPart( composer->textPart() );
  fillInfoPart( composer->infoPart() );
  composer->addAttachmentParts( mAttachmentModel->attachments() );

  mComposers.append( composer );
  connect( composer, SIGNAL(result(KJob*)), this, SLOT(slotAutoSaveComposeResult(KJob*)) );
  composer->start();
}

void KMComposeWin::writeAutoSaveToDisk( KMime::Message::Ptr message, int msgNum )
{
  const QString filename = KMKernel::localDataPath() + "autosave/" +
    mAutoSaveUUID + "." + QString::number( msgNum );
  KSaveFile file( filename );
  QString errorMessage;
  kDebug() << "Writing message to disk as" << filename;

  if( file.open() ) {
    file.setPermissions( QFile::ReadUser | QFile::WriteUser );

    if( file.write( message->encodedContent() ) !=
        static_cast<qint64>( message->encodedContent().size() ) ) {
      errorMessage = i18n( "Could not write all data to file." );
    }
    else {
      if( !file.finalize() ) {
        errorMessage = i18n( "Could not finalize the file." );
      }
    }
  }
  else {
    errorMessage = i18n( "Could not open file." );
  }

  if ( !errorMessage.isEmpty() ) {
    kWarning() << "Auto saving failed:" << errorMessage << file.errorString();
    if ( !mAutoSaveErrorShown ) {
      KMessageBox::sorry( this, i18n( "Autosaving the message as %1 failed.\n"
                                      "%2\n"
                                      "Reason: %3",
                                      filename,
                                      errorMessage,
                                      file.errorString() ),
                          i18n( "Autosaving Message Failed" ) );

      // Error dialog shown, hide the errors the next time
      mAutoSaveErrorShown = true;
    }
  }
  else {
    // No error occurred, the next error should be shown again
    mAutoSaveErrorShown = false;
  }
}


//-----------------------------------------------------------------------------
void KMComposeWin::slotView( void )
{
  if ( !mDone ) {
    return; // otherwise called from rethinkFields during the construction
            // which is not the intended behavior
  }
  int id;

  //This sucks awfully, but no, I cannot get an activated(int id) from
  // actionContainer()
  KToggleAction *act = ::qobject_cast<KToggleAction *>( sender() );
  if ( !act ) {
    return;
  }

  if ( act == mAllFieldsAction ) {
    id = 0;
  } else if ( act == mIdentityAction ) {
    id = HDR_IDENTITY;
  } else if ( act == mTransportAction ) {
    id = HDR_TRANSPORT;
  } else if ( act == mFromAction ) {
    id = HDR_FROM;
  } else if ( act == mReplyToAction ) {
    id = HDR_REPLY_TO;
  } else if ( act == mSubjectAction ) {
    id = HDR_SUBJECT;
  } else if ( act == mFccAction ) {
    id = HDR_FCC;
  } else if ( act == mDictionaryAction ) {
    id = HDR_DICTIONARY;
  } else {
    id = 0;
    kDebug() <<"Something is wrong (Oh, yeah?)";
    return;
  }

  // sanders There's a bug here this logic doesn't work if no
  // fields are shown and then show all fields is selected.
  // Instead of all fields being shown none are.
  if ( !act->isChecked() ) {
    // hide header
    if ( id > 0 ) {
      mShowHeaders = mShowHeaders & ~id;
    } else {
      mShowHeaders = abs( mShowHeaders );
    }
  } else {
    // show header
    if ( id > 0 ) {
      mShowHeaders |= id;
    } else {
      mShowHeaders = -abs( mShowHeaders );
    }
  }
  rethinkFields( true );
}

int KMComposeWin::calcColumnWidth( int which, long allShowing, int width ) const
{
  if ( ( allShowing & which ) == 0 ) {
    return width;
  }

  QLabel *w;
  if ( which == HDR_IDENTITY ) {
    w = mLblIdentity;
  } else if ( which == HDR_DICTIONARY ) {
    w = mDictionaryLabel;
  } else if ( which == HDR_FCC ) {
    w = mLblFcc;
  } else if ( which == HDR_TRANSPORT ) {
    w = mLblTransport;
  } else if ( which == HDR_FROM ) {
    w = mLblFrom;
  } else if ( which == HDR_REPLY_TO ) {
    w = mLblReplyTo;
  } else if ( which == HDR_SUBJECT ) {
    w = mLblSubject;
  } else {
    return width;
  }

  w->setBuddy( mEditor ); // set dummy so we don't calculate width of '&' for this label.
  w->adjustSize();
  w->show();
  return qMax( width, w->sizeHint().width() );
}

void KMComposeWin::rethinkFields( bool fromSlot )
{
  //This sucks even more but again no ids. sorry (sven)
  int mask, row, numRows;
  long showHeaders;

  if ( mShowHeaders < 0 ) {
    showHeaders = HDR_ALL;
  } else {
    showHeaders = mShowHeaders;
  }

  for ( mask=1, mNumHeaders=0; mask<=showHeaders; mask<<=1 ) {
    if ( ( showHeaders & mask ) != 0 ) {
      mNumHeaders++;
    }
  }

  numRows = mNumHeaders + 1;

  delete mGrid;
  mGrid = new QGridLayout( mHeadersArea );
  mGrid->setSpacing( KDialog::spacingHint() );
  mGrid->setMargin( KDialog::marginHint() / 2 );
  mGrid->setColumnStretch( 0, 1 );
  mGrid->setColumnStretch( 1, 100 );
  mGrid->setColumnStretch( 2, 1 );
  mGrid->setRowStretch( mNumHeaders + 1, 100 );

  row = 0;
  kDebug();

  mLabelWidth = mRecipientsEditor->setFirstColumnWidth( 0 );
  mLabelWidth = calcColumnWidth( HDR_IDENTITY, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_DICTIONARY, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_FCC, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_TRANSPORT, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_FROM, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_REPLY_TO, showHeaders, mLabelWidth );
  mLabelWidth = calcColumnWidth( HDR_SUBJECT, showHeaders, mLabelWidth );

  if ( !fromSlot ) {
    mAllFieldsAction->setChecked( showHeaders == HDR_ALL );
  }

  if ( !fromSlot ) {
    mIdentityAction->setChecked( abs( mShowHeaders )&HDR_IDENTITY );
  }
  rethinkHeaderLine( showHeaders,HDR_IDENTITY, row, mLblIdentity, mIdentity,
                     mBtnIdentity );

  if ( !fromSlot ) {
    mDictionaryAction->setChecked( abs( mShowHeaders )&HDR_DICTIONARY );
  }
  rethinkHeaderLine( showHeaders,HDR_DICTIONARY, row, mDictionaryLabel,
                     mDictionaryCombo, 0 );

  if ( !fromSlot ) {
    mFccAction->setChecked( abs( mShowHeaders )&HDR_FCC );
  }
  rethinkHeaderLine( showHeaders,HDR_FCC, row, mLblFcc, mFcc, mBtnFcc );

  if ( !fromSlot ) {
    mTransportAction->setChecked( abs( mShowHeaders )&HDR_TRANSPORT );
  }
  rethinkHeaderLine( showHeaders,HDR_TRANSPORT, row, mLblTransport, mTransport,
                     mBtnTransport );

  if ( !fromSlot ) {
    mFromAction->setChecked( abs( mShowHeaders )&HDR_FROM );
  }
  rethinkHeaderLine( showHeaders,HDR_FROM, row, mLblFrom, mEdtFrom );

  QWidget *prevFocus = mEdtFrom;

  if ( !fromSlot ) {
    mReplyToAction->setChecked( abs( mShowHeaders )&HDR_REPLY_TO );
  }
  rethinkHeaderLine( showHeaders, HDR_REPLY_TO, row, mLblReplyTo, mEdtReplyTo );
  if ( showHeaders & HDR_REPLY_TO ) {
    prevFocus = connectFocusMoving( prevFocus, mEdtReplyTo );
  }

  mGrid->addWidget( mRecipientsEditor, row, 0, 1, 3 );
  ++row;
  if ( showHeaders & HDR_REPLY_TO ) {
    connect( mEdtReplyTo, SIGNAL( focusDown() ), mRecipientsEditor,
             SLOT( setFocusTop() ) );
    connect( mRecipientsEditor, SIGNAL( focusUp() ), mEdtReplyTo,
             SLOT( setFocus() ) );
  } else {
    connect( mEdtFrom, SIGNAL( focusDown() ), mRecipientsEditor,
             SLOT( setFocusTop() ) );
    connect( mRecipientsEditor, SIGNAL( focusUp() ), mEdtFrom,
             SLOT( setFocus() ) );
  }

  connect( mRecipientsEditor, SIGNAL( focusDown() ), mEdtSubject,
           SLOT( setFocus() ) );
  connect( mEdtSubject, SIGNAL( focusUp() ), mRecipientsEditor,
           SLOT( setFocusBottom() ) );

  prevFocus = mRecipientsEditor;

  if ( !fromSlot ) {
    mSubjectAction->setChecked( abs( mShowHeaders )&HDR_SUBJECT );
  }
  rethinkHeaderLine(showHeaders,HDR_SUBJECT, row, mLblSubject, mEdtSubject );
  connectFocusMoving( mEdtSubject, mEditor );

  assert( row <= mNumHeaders + 1 );


#if 0
  if ( !mAtmList.isEmpty() ) {
    mAtmListView->show();
  } else {
    mAtmListView->hide();
  }
#endif

  mHeadersArea->setMaximumHeight( mHeadersArea->sizeHint().height() );

  mIdentityAction->setEnabled(!mAllFieldsAction->isChecked());
  mDictionaryAction->setEnabled( !mAllFieldsAction->isChecked() );
  mTransportAction->setEnabled(!mAllFieldsAction->isChecked());
  mFromAction->setEnabled(!mAllFieldsAction->isChecked());
  if ( mReplyToAction ) {
    mReplyToAction->setEnabled( !mAllFieldsAction->isChecked() );
  }
  mFccAction->setEnabled( !mAllFieldsAction->isChecked() );
  mSubjectAction->setEnabled( !mAllFieldsAction->isChecked() );
  mRecipientsEditor->setFirstColumnWidth( mLabelWidth );
}

QWidget *KMComposeWin::connectFocusMoving( QWidget *prev, QWidget *next )
{
  connect( prev, SIGNAL( focusDown() ), next, SLOT( setFocus() ) );
  connect( next, SIGNAL( focusUp() ), prev, SLOT( setFocus() ) );

  return next;
}

//-----------------------------------------------------------------------------
void KMComposeWin::rethinkHeaderLine( int aValue, int aMask, int &aRow,
                                      QLabel *aLbl, KLineEdit *aEdt,
                                      QPushButton *aBtn )
{
  if ( aValue & aMask ) {
    aLbl->setFixedWidth( mLabelWidth );
    aLbl->setBuddy( aEdt );
    mGrid->addWidget( aLbl, aRow, 0 );
    aEdt->show();

    if ( aBtn ) {
      mGrid->addWidget( aEdt, aRow, 1 );
      mGrid->addWidget( aBtn, aRow, 2 );
      aBtn->show();
    } else {
      mGrid->addWidget( aEdt, aRow, 1, 1, 2 );
    }
    aRow++;
  } else {
    aLbl->hide();
    aEdt->hide();
    if ( aBtn ) {
      aBtn->hide();
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::rethinkHeaderLine( int aValue, int aMask, int &aRow,
                                      QLabel *aLbl, QComboBox *aCbx, // krazy:exclude=qclasses
                                      QCheckBox *aChk )
{
  if ( aValue & aMask ) {
    aLbl->setBuddy( aCbx );
    mGrid->addWidget( aLbl, aRow, 0 );

    mGrid->addWidget( aCbx, aRow, 1 );
    aCbx->show();
    if ( aChk ) {
      mGrid->addWidget( aChk, aRow, 2 );
      aChk->show();
    }
    aRow++;
  } else {
    aLbl->hide();
    aCbx->hide();
    if ( aChk ) {
      aChk->hide();
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::applyTemplate( uint uoid )
{
  const KPIMIdentities::Identity &ident = kmkernel->identityManager()->identityForUoid( uoid );
  if ( ident.isNull() )
    return;
  KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Templates", mMsg.get(), ident.templates(), "utf-8" );
  mMsg->setHeader( header );

  TemplateParser::Mode mode;
  switch ( mContext ) {
    case New:
      mode = TemplateParser::NewMessage;
      break;
    case Reply:
      mode = TemplateParser::Reply;
      break;
    case ReplyToAll:
      mode = TemplateParser::ReplyAll;
      break;
    case Forward:
      mode = TemplateParser::Forward;
      break;
    default:
      return;
  }

  TemplateParser parser( mMsg, mode, GlobalSettings::self()->smartQuote(),
                         GlobalSettings::self()->automaticDecrypt() );
  parser.setSelection( mTextSelection );
  if ( mode == TemplateParser::NewMessage ) {
    if ( !mCustomTemplate.isEmpty() )
      parser.process( mCustomTemplate, KMime::Message::Ptr() );
    else
      parser.processWithIdentity( uoid, KMime::Message::Ptr() );
  } else {
    if ( mMsg->headerByType( "X-KMail-Link-Message" ) ) {
      foreach( const QString& serNumStr, mMsg->headerByType( "X-KMail-Link-Message" )->asUnicodeString().split( ',' ) ) {
        const ulong serNum = serNumStr.toULong();
        Akonadi::Item item( serNum );
        Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob( item, this );
        itemFetchJob->fetchScope().fetchFullPayload( true );
        itemFetchJob->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
        //TODO convert to async
        if ( itemFetchJob->exec() ) {
          Akonadi::Item it = itemFetchJob->items().at( 0 );
          if ( !mCustomTemplate.isEmpty() )
            parser.process( mCustomTemplate, KMail::Util::message( it ) );
          else
            parser.processWithIdentity( uoid, KMail::Util::message( it ) );
        }
      }
    }
  }
#if 0
  mEditor->setText( mMsg->bodyToUnicode() );
#endif
}

void KMComposeWin::setQuotePrefix( uint uoid )
{
  QString quotePrefix = mMsg->headerByType( "X-KMail-QuotePrefix" ) ? mMsg->headerByType( "X-KMail-QuotePrefix" )->asUnicodeString() : QString();
  if ( quotePrefix.isEmpty() ) {
    // no quote prefix header, set quote prefix according in identity
    if ( mCustomTemplate.isEmpty() ) {
      const KPIMIdentities::Identity &identity =
        kmkernel->identityManager()->identityForUoidOrDefault( uoid );
      // Get quote prefix from template
      // ( custom templates don't specify custom quotes prefixes )
      ::Templates quoteTemplate( TemplatesConfiguration::configIdString( identity.uoid() ) );
      quotePrefix = quoteTemplate.quoteString();
    }
  }
  mEditor->setQuotePrefixName( MessageCore::StringUtil::formatString( quotePrefix,
                                                                mMsg->from()->asUnicodeString() ) );
}

//-----------------------------------------------------------------------------
void KMComposeWin::getTransportMenu()
{
  QStringList availTransports;

  mActNowMenu->clear();
  mActLaterMenu->clear();
  availTransports = TransportManager::self()->transportNames();
  QStringList::Iterator it;
  for ( it = availTransports.begin(); it != availTransports.end() ; ++it ) {
    QAction *action1 = new QAction( (*it).replace( '&', "&&" ), mActNowMenu );
    QAction *action2 = new QAction( (*it).replace( '&', "&&" ), mActLaterMenu );
    action1->setData( TransportManager::self()->transportByName( *it )->id() );
    action2->setData( TransportManager::self()->transportByName( *it )->id() );
    mActNowMenu->addAction( action1 );
    mActLaterMenu->addAction( action2 );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupActions( void )
{
  KActionMenu *actActionNowMenu, *actActionLaterMenu;

  if ( kmkernel->msgSender()->sendImmediate() ) {
    //default = send now, alternative = queue
    KAction *action = new KAction(KIcon("mail-send"), i18n("&Send Mail"), this);
    actionCollection()->addAction("send_default", action );
    action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Return ) );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotSendNow() ));

    // FIXME: change to mail_send_via icon when this exits.
    actActionNowMenu = new KActionMenu( KIcon( "mail-send" ), i18n("&Send Mail Via"), this );
    actActionNowMenu->setIconText( i18n( "Send" ) );
    actionCollection()->addAction( "send_default_via", actActionNowMenu );

    action = new KAction( KIcon( "mail-queue" ), i18n("Send &Later"), this );
    actionCollection()->addAction( "send_alternative", action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotSendLater()) );
    actActionLaterMenu = new KActionMenu( KIcon( "mail-queue" ), i18n("Send &Later Via"), this );
    actActionLaterMenu->setIconText( i18nc( "Queue the message for sending at a later date", "Queue" ) );
    actionCollection()->addAction( "send_alternative_via", actActionLaterMenu );

  } else {
    //default = queue, alternative = send now
    QAction *action = new KAction( KIcon( "mail-queue" ), i18n("Send &Later"), this );
    actionCollection()->addAction( "send_default", action );
    connect( action, SIGNAL(triggered(bool) ), SLOT(slotSendLater()) );
    action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Return ) );
    actActionLaterMenu = new KActionMenu( KIcon( "mail-queue" ), i18n("Send &Later Via"), this );
    actionCollection()->addAction( "send_default_via", actActionLaterMenu );

    action = new KAction( KIcon( "mail-send" ), i18n("&Send Mail"), this );
    actionCollection()->addAction( "send_alternative", action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotSendNow()) );

    // FIXME: change to mail_send_via icon when this exits.
    actActionNowMenu = new KActionMenu( KIcon( "mail-send" ), i18n("&Send Mail Via"), this );
    actionCollection()->addAction( "send_alternative_via", actActionNowMenu );

  }

  // needed for sending "default transport"
  actActionNowMenu->setDelayed( true );
  actActionLaterMenu->setDelayed( true );

  connect( actActionNowMenu, SIGNAL(triggered(bool)), this,
           SLOT(slotSendNow()) );
  connect( actActionLaterMenu, SIGNAL(triggered(bool)), this,
           SLOT(slotSendLater()) );

  mActNowMenu = actActionNowMenu->menu();
  mActLaterMenu = actActionLaterMenu->menu();

  connect( mActNowMenu, SIGNAL(triggered(QAction*)), this,
           SLOT(slotSendNowVia( QAction* )) );
  connect( mActNowMenu, SIGNAL(aboutToShow()), this,
           SLOT(getTransportMenu()) );

  connect( mActLaterMenu, SIGNAL(triggered(QAction*)), this,
            SLOT(slotSendLaterVia( QAction*)) );
  connect( mActLaterMenu, SIGNAL(aboutToShow()), this,
           SLOT(getTransportMenu()) );

  KAction *action = new KAction( KIcon( "document-save" ), i18n("Save as &Draft"), this );
  actionCollection()->addAction("save_in_drafts", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT(slotSaveDraft()) );

  action = new KAction( KIcon( "document-save" ), i18n("Save as &Template"), this );
  actionCollection()->addAction( "save_in_templates", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT(slotSaveTemplate()) );

  action = new KAction(KIcon("document-open"), i18n("&Insert File..."), this);
  actionCollection()->addAction("insert_file", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotInsertFile()));

  mRecentAction = new KRecentFilesAction(KIcon("document-open"), i18n("&Insert File Recent"), this);
  actionCollection()->addAction("insert_file_recent", mRecentAction );
  connect(mRecentAction, SIGNAL(urlSelected (const KUrl&)),
          SLOT(slotInsertRecentFile(const KUrl&)));
  connect(mRecentAction, SIGNAL(recentListCleared()),
          SLOT(slotRecentListFileClear()));
  mRecentAction->loadEntries( KMKernel::config()->group( QString() ) );

  action = new KAction(KIcon("x-office-address-book"), i18n("&Address Book"), this);
  actionCollection()->addAction("addressbook", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotAddrBook()));
  action = new KAction(KIcon("mail-message-new"), i18n("&New Composer"), this);
  actionCollection()->addAction("new_composer", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(slotNewComposer()));
  action->setShortcuts( KStandardShortcut::shortcut( KStandardShortcut::New ) );
  action = new KAction( KIcon( "window-new" ), i18n("New Main &Window"), this );
  actionCollection()->addAction( "open_mailreader", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT(slotNewMailReader()) );

  action = new KAction( i18n("Select &Recipients..."), this );
  actionCollection()->addAction( "select_recipients", action );
  connect( action, SIGNAL( triggered(bool) ),
           mRecipientsEditor, SLOT( selectRecipients()) );
  action = new KAction( i18n("Save &Distribution List..."), this );
  actionCollection()->addAction( "save_distribution_list", action );
  connect( action, SIGNAL( triggered(bool) ),
           mRecipientsEditor, SLOT( saveDistributionList() ) );

  KStandardAction::print( this, SLOT(slotPrint()), actionCollection() );
  KStandardAction::close( this, SLOT(slotClose()), actionCollection() );

  KStandardAction::undo( this, SLOT(slotUndo()), actionCollection() );
  KStandardAction::redo( this, SLOT(slotRedo()), actionCollection() );
  KStandardAction::cut( this, SLOT(slotCut()), actionCollection() );
  KStandardAction::copy( this, SLOT(slotCopy()), actionCollection() );
  KStandardAction::pasteText( this, SLOT(slotPaste()), actionCollection() );
  mSelectAll = KStandardAction::selectAll( this, SLOT(slotMarkAll()), actionCollection() );

  mFindText = KStandardAction::find( mEditor, SLOT(slotFind()), actionCollection() );
  mFindNextText = KStandardAction::findNext( mEditor, SLOT(slotFindNext()), actionCollection() );

  mReplaceText = KStandardAction::replace( mEditor, SLOT(slotReplace()), actionCollection() );
  actionCollection()->addAction( KStandardAction::Spelling, "spellcheck",
                                 mEditor, SLOT(checkSpelling()) );

  action = new KAction( i18n("Paste as Attac&hment"), this );
  actionCollection()->addAction( "paste_att", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT( slotPasteAsAttachment()) );

  mCleanSpace = new KAction( i18n("Cl&ean Spaces"), this );
  actionCollection()->addAction( "clean_spaces", mCleanSpace );
  connect( mCleanSpace, SIGNAL(triggered(bool) ), SLOT(slotCleanSpace()) );

  mFixedFontAction = new KToggleAction( i18n("Use Fi&xed Font"), this );
  actionCollection()->addAction( "toggle_fixedfont", mFixedFontAction );
  connect( mFixedFontAction, SIGNAL(triggered(bool) ), SLOT(slotUpdateFont()) );
  mFixedFontAction->setChecked( GlobalSettings::self()->useFixedFont() );

  //these are checkable!!!
  mUrgentAction = new KToggleAction(
    i18nc("@action:inmenu Mark the email as urgent.","&Urgent"), this );
  actionCollection()->addAction( "urgent", mUrgentAction );
  mRequestMDNAction = new KToggleAction( i18n("&Request Disposition Notification"), this );
  actionCollection()->addAction("options_request_mdn", mRequestMDNAction );
  mRequestMDNAction->setChecked(GlobalSettings::self()->requestMDN());
  //----- Message-Encoding Submenu
  mCodecAction = new CodecAction( CodecAction::ComposerMode, this );
  actionCollection()->addAction( "charsets", mCodecAction );
  mWordWrapAction = new KToggleAction( i18n( "&Wordwrap" ), this );
  actionCollection()->addAction( "wordwrap", mWordWrapAction );
  mWordWrapAction->setChecked( GlobalSettings::self()->wordWrap() );
  connect( mWordWrapAction, SIGNAL(toggled(bool)), SLOT(slotWordWrapToggled(bool)) );

  mSnippetAction = new KToggleAction( i18n("&Snippets"), this );
  actionCollection()->addAction( "snippets", mSnippetAction );
  connect( mSnippetAction, SIGNAL( toggled(bool) ),
           mSnippetWidget, SLOT( setVisible(bool) ) );
  mSnippetAction->setChecked( GlobalSettings::self()->showSnippetManager() );

  mAutoSpellCheckingAction = new KToggleAction( KIcon( "tools-check-spelling" ),
                                                i18n("&Automatic Spellchecking"),
                                                this );
  actionCollection()->addAction( "options_auto_spellchecking", mAutoSpellCheckingAction );
  const bool spellChecking = GlobalSettings::self()->autoSpellChecking();
  const bool spellCheckingEnabled = !GlobalSettings::self()->useExternalEditor() && spellChecking;
  mAutoSpellCheckingAction->setEnabled( !GlobalSettings::self()->useExternalEditor() );
  mAutoSpellCheckingAction->setChecked( spellCheckingEnabled );
  slotAutoSpellCheckingToggled( spellCheckingEnabled );
  connect( mAutoSpellCheckingAction, SIGNAL( toggled( bool ) ),
           this, SLOT( slotAutoSpellCheckingToggled( bool ) ) );
  connect( mEditor, SIGNAL( checkSpellingChanged( bool ) ),
           this, SLOT( slotAutoSpellCheckingToggled( bool ) ) );

  connect( mEditor, SIGNAL( textModeChanged( KRichTextEdit::Mode ) ),
           this, SLOT( slotTextModeChanged( KRichTextEdit::Mode ) ) );

  //these are checkable!!!
  markupAction = new KToggleAction( i18n("Formatting (HTML)"), this );
  markupAction->setIconText( i18n("HTML") );
  actionCollection()->addAction( "html", markupAction );
  connect( markupAction, SIGNAL(triggered(bool) ), SLOT(slotToggleMarkup()) );

  mAllFieldsAction = new KToggleAction( i18n("&All Fields"), this);
  actionCollection()->addAction( "show_all_fields", mAllFieldsAction );
  connect( mAllFieldsAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mIdentityAction = new KToggleAction(i18n("&Identity"), this);
  actionCollection()->addAction("show_identity", mIdentityAction );
  connect( mIdentityAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mDictionaryAction = new KToggleAction(i18n("&Dictionary"), this);
  actionCollection()->addAction("show_dictionary", mDictionaryAction );
  connect( mDictionaryAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mFccAction = new KToggleAction(i18n("&Sent-Mail Folder"), this);
  actionCollection()->addAction("show_fcc", mFccAction );
  connect( mFccAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mTransportAction = new KToggleAction(i18n("&Mail Transport"), this);
  actionCollection()->addAction("show_transport", mTransportAction );
  connect( mTransportAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mFromAction = new KToggleAction(i18n("&From"), this);
  actionCollection()->addAction("show_from", mFromAction );
  connect( mFromAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mReplyToAction = new KToggleAction(i18n("&Reply To"), this);
  actionCollection()->addAction("show_reply_to", mReplyToAction );
  connect( mReplyToAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  mSubjectAction = new KToggleAction(
    i18nc("@action:inmenu Show the subject in the composer window.", "S&ubject"), this);
  actionCollection()->addAction("show_subject", mSubjectAction );
  connect(mSubjectAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  //end of checkable

  action = new KAction( i18n("Append S&ignature"), this );
  actionCollection()->addAction( "append_signature", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT(slotAppendSignature()));
  action = new KAction( i18n("Pr&epend Signature"), this );
  actionCollection()->addAction( "prepend_signature", action );
  connect( action, SIGNAL( triggered(bool) ), SLOT( slotPrependSignature() ) );
  action = new KAction( i18n("Insert Signature At C&ursor Position"), this );
  actionCollection()->addAction( "insert_signature_at_cursor_position", action );
  connect( action, SIGNAL( triggered(bool) ), SLOT( slotInsertSignatureAtCursor() ) );



  mAttachmentController->createActions();





  setStandardToolBarMenuEnabled( true );

  KStandardAction::keyBindings( this, SLOT(slotEditKeys()), actionCollection());
  KStandardAction::configureToolbars( this, SLOT(slotEditToolbars()), actionCollection());
  KStandardAction::preferences( kmkernel, SLOT(slotShowConfigurationDialog()), actionCollection() );

  action = new KAction( i18n("&Spellchecker..."), this );
  action->setIconText( i18n("Spellchecker") );
  actionCollection()->addAction( "setup_spellchecker", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT(slotSpellcheckConfig()) );

  if ( Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" ) ) {
    KToggleAction *a = new KToggleAction( KIcon( "chiasmus_chi" ), i18n("Encrypt Message with Chiasmus..."), this );
    actionCollection()->addAction( "encrypt_message_chiasmus", a );
    a->setCheckedState( KGuiItem( i18n( "Encrypt Message with Chiasmus..." ), "chiencrypted" ) );
    mEncryptChiasmusAction = a;
    connect( mEncryptChiasmusAction, SIGNAL(toggled(bool)),
             this, SLOT(slotEncryptChiasmusToggled(bool)) );
  } else {
    mEncryptChiasmusAction = 0;
  }

  mEncryptAction = new KToggleAction(KIcon("document-encrypt"), i18n("&Encrypt Message"), this);
  mEncryptAction->setIconText( i18n( "Encrypt" ) );
  actionCollection()->addAction("encrypt_message", mEncryptAction );
  mSignAction = new KToggleAction(KIcon("document-sign"), i18n("&Sign Message"), this);
  mSignAction->setIconText( i18n( "Sign" ) );
  actionCollection()->addAction("sign_message", mSignAction );
  const KPIMIdentities::Identity &ident =
    KMKernel::self()->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  // PENDING(marc): check the uses of this member and split it into
  // smime/openpgp and or enc/sign, if necessary:
  mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
  mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

  mLastEncryptActionState = false;
  mLastSignActionState = GlobalSettings::self()->pgpAutoSign();

  if ( !Kleo::CryptoBackendFactory::instance()->openpgp() && !Kleo::CryptoBackendFactory::instance()->smime() ) {
    // no crypto whatsoever
    mEncryptAction->setEnabled( false );
    setEncryption( false );
    mSignAction->setEnabled( false );
    setSigning( false );
  } else {
    const bool canOpenPGPSign = Kleo::CryptoBackendFactory::instance()->openpgp() &&
      !ident.pgpSigningKey().isEmpty();
    const bool canSMIMESign = Kleo::CryptoBackendFactory::instance()->smime() &&
      !ident.smimeSigningKey().isEmpty();

    setEncryption( false );
    setSigning( ( canOpenPGPSign || canSMIMESign ) && GlobalSettings::self()->pgpAutoSign() );
  }

  connect( mEncryptAction, SIGNAL(toggled(bool)),
           SLOT(slotEncryptToggled( bool )) );
  connect( mSignAction, SIGNAL(toggled(bool)),
           SLOT(slotSignToggled( bool )) );

  QStringList l;
  for ( int i=0 ; i<numCryptoMessageFormats ; ++i ) {
    l.push_back( Kleo::cryptoMessageFormatToLabel( cryptoMessageFormats[i] ) );
  }

  mCryptoModuleAction = new KSelectAction(i18n("&Cryptographic Message Format"), this);
  actionCollection()->addAction("options_select_crypto", mCryptoModuleAction );
  connect(mCryptoModuleAction, SIGNAL(triggered(int)), SLOT(slotSelectCryptoModule()));
  mCryptoModuleAction->setItems( l );
  mCryptoModuleAction->setCurrentItem( format2cb(
      Kleo::stringToCryptoMessageFormat( ident.preferredCryptoMessageFormat() ) ) );
  slotSelectCryptoModule( true );

  actionFormatReset = new KAction( KIcon( "draw-eraser" ), i18n("Reset Font Settings"), this );
  actionFormatReset->setIconText( i18n("Reset Font") );
  actionCollection()->addAction( "format_reset", actionFormatReset );
  connect( actionFormatReset, SIGNAL(triggered(bool) ), SLOT( slotFormatReset() ) );

  mEditor->setRichTextSupport( KRichTextWidget::FullTextFormattingSupport |
                               KRichTextWidget::FullListSupport |
                               KRichTextWidget::SupportAlignment |
                               KRichTextWidget::SupportRuleLine |
                               KRichTextWidget::SupportHyperlinks |
                               KRichTextWidget::SupportAlignment );
  mEditor->enableImageActions();
  mEditor->createActions( actionCollection() );

  createGUI( "kmcomposerui.rc" );
  connect( toolBar( "htmlToolBar" )->toggleViewAction(),
           SIGNAL( toggled( bool ) ),
           SLOT( htmlToolBarVisibilityChanged( bool ) ) );

  // In Kontact, this entry would read "Configure Kontact", but bring
  // up KMail's config dialog. That's sensible, though, so fix the label.
  QAction *configureAction = actionCollection()->action( "options_configure" );
  if ( configureAction ) {
    configureAction->setText( i18n("Configure KMail..." ) );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupStatusBar( void )
{
  statusBar()->insertItem( "", 0, 1 );
  statusBar()->setItemAlignment( 0, Qt::AlignLeft | Qt::AlignVCenter );

  statusBar()->insertPermanentItem( i18n(" Spellcheck: %1 ", QString( "     " )), 3, 0) ;
  statusBar()->insertPermanentItem( i18n(" Column: %1 ", QString( "     " ) ), 2, 0 );
  statusBar()->insertPermanentItem(
    i18nc("Shows the linenumber of the cursor position.", " Line: %1 "
      , QString( "     " ) ), 1, 0 );
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupEditor( void )
{
  mEditor->document()->setModified( false );
  QFontMetrics fm( mBodyFont );
  mEditor->setTabStopWidth( fm.width( QChar(' ') ) * 8 );

  slotWordWrapToggled( GlobalSettings::self()->wordWrap() );

  // Font setup
  slotUpdateFont();

  connect( mEditor, SIGNAL( cursorPositionChanged() ),
           this, SLOT( slotCursorPositionChanged() ) );
  slotCursorPositionChanged();
}

//-----------------------------------------------------------------------------
static QString cleanedUpHeaderString( const QString &s )
{
  // remove invalid characters from the header strings
  QString res( s );
  res.remove( '\r' );
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
  return mRecipientsEditor->recipientString( Recipient::To );
}

//-----------------------------------------------------------------------------
QString KMComposeWin::cc() const
{
  return mRecipientsEditor->recipientString( Recipient::Cc );
}

//-----------------------------------------------------------------------------
QString KMComposeWin::bcc() const
{
  return mRecipientsEditor->recipientString( Recipient::Bcc );
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
void KMComposeWin::decryptOrStripOffCleartextSignature( QByteArray &body )
{
  QList<Kpgp::Block> pgpBlocks;
  QList<QByteArray> nonPgpBlocks;
  if ( Kpgp::Module::prepareMessageForDecryption( body,
                                                  pgpBlocks, nonPgpBlocks ) ) {
    // Only decrypt/strip off the signature if there is only one OpenPGP
    // block in the message
    if ( pgpBlocks.count() == 1 ) {
      Kpgp::Block &block = pgpBlocks.first();
      if ( ( block.type() == Kpgp::PgpMessageBlock ) ||
           ( block.type() == Kpgp::ClearsignedBlock ) ) {
        if ( block.type() == Kpgp::PgpMessageBlock ) {
          // try to decrypt this OpenPGP block
          block.decrypt();
        } else {
          // strip off the signature
          block.verify();
        }
        body = nonPgpBlocks.first();
        body.append( block.text() );
        body.append( nonPgpBlocks.last() );
      }
    }
  }
}

// Checks if the mail is a HTML mail.
// The catch here is that encapsulated messages can also have a HTML part, so we make
// sure that only messages where the first HTML part is in the same multipart/alternative container
// as the frist plain text part are counted as HTML mail
static bool isHTMLMail( KMime::Content *root )
{
  if ( !root )
    return false;

  using namespace MessageViewer;
  KMime::Content *firstTextPart = ObjectTreeParser::findType( root, "text/plain", true, true );
  KMime::Content *firstHtmlPart = ObjectTreeParser::findType( root, "text/html", true, true );
  if ( !firstTextPart || !firstHtmlPart )
    return false;

  KMime::Content *parent = firstTextPart->parent();
  if ( !parent || parent != firstHtmlPart->parent() )
    return false;

  if ( !parent->contentType()->isMultipart() ||
       parent->contentType()->subType() != "alternative" )
    return false;

  return true;
}

//-----------------------------------------------------------------------------
void KMComposeWin::setMsg( const KMime::Message::Ptr &newMsg, bool mayAutoSign,
                           bool allowDecryption, bool isModified )
{
  if ( !newMsg ) {
    kDebug() << "newMsg == 0!";
    return;
  }

  mMsg = newMsg;
  KPIMIdentities::IdentityManager * im = KMKernel::self()->identityManager();

  mEdtFrom->setText( mMsg->from()->asUnicodeString() );
  mEdtReplyTo->setText( mMsg->replyTo()->asUnicodeString() );
  mRecipientsEditor->setRecipientString( mMsg->to()->mailboxes(), Recipient::To );
  mRecipientsEditor->setRecipientString( mMsg->cc()->mailboxes(), Recipient::Cc );
  mRecipientsEditor->setRecipientString( mMsg->bcc()->mailboxes(), Recipient::Bcc );
  mRecipientsEditor->setFocusBottom();
  mEdtSubject->setText( mMsg->subject()->asUnicodeString() );

  const bool stickyIdentity = mBtnIdentity->isChecked() && !mIgnoreStickyFields;
  bool messageHasIdentity = false;
  if( newMsg->headerByType("X-KMail-Identity") &&
      !newMsg->headerByType("X-KMail-Identity")->asUnicodeString().isEmpty() )
    messageHasIdentity = true;
  if ( !stickyIdentity && messageHasIdentity )
    mId = newMsg->headerByType( "X-KMail-Identity" )->asUnicodeString().toUInt();

  // don't overwrite the header values with identity specific values
  // unless the identity is sticky
  if ( !stickyIdentity ) {
    disconnect( mIdentity,SIGNAL( identityChanged(uint) ),
                this, SLOT( slotIdentityChanged(uint) ) ) ;
  }
  // load the mId into the gui, sticky or not, without emitting
  mIdentity->setCurrentIdentity( mId );
  const uint idToApply = mId;
  if ( !stickyIdentity ) {
    connect( mIdentity,SIGNAL(identityChanged(uint)),
             this, SLOT(slotIdentityChanged(uint)) );
  } else {
    // load the message's state into the mId, without applying it to the gui
    // that's so we can detect that the id changed (because a sticky was set)
    // on apply()
    if ( messageHasIdentity ) {
      mId = newMsg->headerByType("X-KMail-Identity")->asUnicodeString().toUInt();
    } else {
      mId = im->defaultIdentity().uoid();
    }
  }
  // manually load the identity's value into the fields; either the one from the
  // messge, where appropriate, or the one from the sticky identity. What's in
  // mId might have changed meanwhile, thus the save value
  slotIdentityChanged( idToApply, true /*initalChange*/ );

  const KPIMIdentities::Identity &ident = im->identityForUoid( mIdentity->currentIdentity() );

  // check for the presence of a DNT header, indicating that MDN's were requested
  if( newMsg->headerByType( "Disposition-Notification-To" ) ) {
    QString mdnAddr = newMsg->headerByType( "Disposition-Notification-To" )->asUnicodeString();
    mRequestMDNAction->setChecked( ( !mdnAddr.isEmpty() &&
                                   im->thatIsMe( mdnAddr ) ) ||
                                 GlobalSettings::self()->requestMDN() );
  }
  // check for presence of a priority header, indicating urgent mail:
  //mUrgentAction->setChecked( newMsg->isUrgent() );


  if ( !ident.isXFaceEnabled() || ident.xface().isEmpty() ) {
    if( mMsg->headerByType( "X-Face" ) )
      mMsg->headerByType( "X-Face" )->clear();
  } else {
    QString xface = ident.xface();
    if ( !xface.isEmpty() ) {
      int numNL = ( xface.length() - 1 ) / 70;
      for ( int i = numNL; i > 0; --i ) {
        xface.insert( i * 70, "\n\t" );
      }
      mMsg->setHeader( new KMime::Headers::Generic( "X-Face", mMsg.get(), xface.toUtf8(), "utf-8" ) );
    }
  }

#if 0 //TODO port to kmime

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
#endif

  // if these headers are present, the state of the message should be overruled
  if ( mMsg->headerByType( "X-KMail-SignatureActionEnabled" ) )
    mLastSignActionState = (mMsg->headerByType( "X-KMail-SignatureActionEnabled" )->as7BitString().contains( "true" ));
  if ( mMsg->headerByType( "X-KMail-EncryptActionEnabled" ) )
    mLastEncryptActionState = (mMsg->headerByType( "X-KMail-EncryptActionEnabled" )->as7BitString().contains( "true") );
  if ( mMsg->headerByType( "X-KMail-CryptoMessageFormat" ) ) {
    mCryptoModuleAction->setCurrentItem( format2cb( static_cast<Kleo::CryptoMessageFormat>(
                    mMsg->headerByType( "X-KMail-CryptoMessageFormat" )->asUnicodeString().toInt() ) ) );
  }

  mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
  mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

  if ( Kleo::CryptoBackendFactory::instance()->openpgp() || Kleo::CryptoBackendFactory::instance()->smime() ) {
    const bool canOpenPGPSign = Kleo::CryptoBackendFactory::instance()->openpgp() &&
      !ident.pgpSigningKey().isEmpty();
    const bool canSMIMESign = Kleo::CryptoBackendFactory::instance()->smime() &&
      !ident.smimeSigningKey().isEmpty();

    setEncryption( mLastEncryptActionState );
    setSigning( ( canOpenPGPSign || canSMIMESign ) && mLastSignActionState );
  }
  slotUpdateSignatureAndEncrypionStateIndicators();

#if 0 //TODO port to attachmentcontroller

  // "Attach my public key" is only possible if the user uses OpenPGP
  // support and he specified his key:
  mAttachMPK->setEnabled( Kleo::CryptoBackendFactory::instance()->openpgp() &&
                          !ident.pgpEncryptionKey().isEmpty() );
#endif

  QString transportName;
  if( newMsg->headerByType( "X-KMail-Transport" ) )
    transportName = newMsg->headerByType("X-KMail-Transport")->asUnicodeString();
  const bool stickyTransport = mBtnTransport->isChecked() && !mIgnoreStickyFields;
  if ( !stickyTransport && !transportName.isEmpty() ) {
    Transport *transport =
        TransportManager::self()->transportByName( transportName );
    if ( transport )
      mTransport->setCurrentTransport( transport->id() );
  }

  QString kmailFcc;
  if ( mMsg->headerByType( "X-KMail-Fcc" ) ) {
    kmailFcc = mMsg->headerByType( "X-KMail-Fcc" )->asUnicodeString();
  }
  if ( !mBtnFcc->isChecked() ) {
    if ( kmailFcc.isEmpty() ) {
      setFcc( ident.fcc() );
    }
    else
      setFcc( kmailFcc );
  }
  mDictionaryCombo->setCurrentByDictionaryName( ident.dictionary() );


  // Restore the quote prefix. We can't just use the global quote prefix here,
  // since the prefix is different for each message, it might for example depend
  // on the original sender in a reply.
  if ( newMsg->headerByType( "X-KMail-QuotePrefix" ) )
    mEditor->setQuotePrefixName( newMsg->headerByType( "X-KMail-QuotePrefix" )->asUnicodeString() );

  // First, we copy the message and then parse it to the object tree parser.
  // The otp gets the message text out of it, in textualContent(), and also decrypts
  // the message if necessary.
  KMime::Content *msgContent = new KMime::Content;
  msgContent->setContent( mMsg->encodedContent() );
  msgContent->parse();
  MessageViewer::EmptySource emptySource;
  MessageViewer::ObjectTreeParser otp( &emptySource );//All default are ok
  otp.parseObjectTree( Akonadi::Item(), msgContent );

  // Load the attachments
  KMail::AttachmentCollector ac;
  ac.collectAttachmentsFrom( msgContent );
  for ( std::vector<KMime::Content*>::const_iterator it = ac.attachments().begin();
        it != ac.attachments().end() ; ++it ) {
    addAttach( *it );
  }

  // Set the editor text and charset
  mEditor->setText( otp.textualContent() );
  bool shouldSetCharset = true;
  if ( !( mContext == Reply || mContext == ReplyToAll || mContext == Forward ) && GlobalSettings::forceReplyCharset() )
    shouldSetCharset = false;
  if ( shouldSetCharset && !otp.textualContentCharset().isEmpty() )
    setCharset( otp.textualContentCharset() );
  else
    setAutoCharset();

  // Set the HTML text and collect HTML images
  if ( isHTMLMail( mMsg.get() ) ) {
    KMime::Content *htmlNode = MessageViewer::ObjectTreeParser::findType( msgContent, "text/html", true, true );
    Q_ASSERT( htmlNode );
    KMime::Content *parentNode = htmlNode->parent();
    if ( parentNode && parentNode->contentType()->isMultipart() ) {
      enableHtml();

      const QString htmlCharset = htmlNode->contentType()->charset();
      const QByteArray htmlBodyDecoded = htmlNode->decodedContent();
      const QTextCodec *codec = MessageViewer::NodeHelper::codecForName( htmlCharset.toAscii() );
      if ( codec ) {
        mEditor->setHtml( codec->toUnicode( htmlBodyDecoded ) );
      } else {
        mEditor->setHtml( QString::fromLocal8Bit( htmlBodyDecoded ) );
      }
    }
    collectImages( mMsg.get() );
  }

#if 0 //TODO port to kmime

  /* Handle the special case of non-mime mails */
  if ( mMsg->numBodyParts() == 0 && otp.textualContent().isEmpty() ) {
    mCharset=mMsg->charset();
    if ( mCharset.isEmpty() ||  mCharset == "default" ) {
      mCharset = Util::defaultCharset();
    }

    QByteArray bodyDecoded = mMsg->bodyDecoded();

    if ( allowDecryption ) {
      decryptOrStripOffCleartextSignature( bodyDecoded );
    }

    const QTextCodec *codec = KMail::Util::codecForName( mCharset );
    if ( codec ) {
      mEditor->setText( codec->toUnicode( bodyDecoded ) );
    } else {
      mEditor->setText( QString::fromLocal8Bit( bodyDecoded ) );
    }
  }
#endif

  if( (GlobalSettings::self()->autoTextSignature()=="auto") && mayAutoSign ) {
    //
    // Espen 2000-05-16
    // Delay the signature appending. It may start a fileseletor.
    // Not user friendy if this modal fileseletor opens before the
    // composer.
    //
    if ( GlobalSettings::self()->prependSignature() ) {
      QTimer::singleShot( 0, this, SLOT( slotPrependSignature() ) );
    } else {
      QTimer::singleShot( 0, this, SLOT( slotAppendSignature() ) );
    }
  }
  if ( mMsg->headerByType( "X-KMail-CursorPos" ) ) {
    mEditor->setCursorPositionFromStart( mMsg->headerByType( "X-KMail-CursorPos" )->asUnicodeString().toInt() );
  }
  setModified( isModified );

  // honor "keep reply in this folder" setting even when the identity is changed later on
  mPreventFccOverwrite = ( !kmailFcc.isEmpty() && ident.fcc() != kmailFcc );
}

//-----------------------------------------------------------------------------
void KMComposeWin::setTextSelection( const QString& selection )
{
  mTextSelection = selection;
}

//-----------------------------------------------------------------------------
void KMComposeWin::setCustomTemplate( const QString& customTemplate )
{
  mCustomTemplate = customTemplate;
}

//-----------------------------------------------------------------------------
void KMComposeWin::collectImages( KMime::Content *root )
{
  if ( KMime::Content * n = MessageViewer::ObjectTreeParser::findType( root, "multipart/alternative", true, true ) ) {
    KMime::Content *parentnode = n->parent();
    if ( parentnode &&
         parentnode->contentType()->isMultipart() &&
         parentnode->contentType()->subType() == "related" ) {
      KMime::Content *node = MessageViewer::NodeHelper::nextSibling( n );
      while ( node ) {
        if ( node->contentType()->isImage() ) {
          kDebug() << "found image in multipart/related : " << node->contentType()->name();
          QImage img;
          img.loadFromData( node->decodedContent() );
          mEditor->loadImage( img, "cid:" + node->contentID()->identifier(),
                              node->contentType()->name() );
        }
        node = MessageViewer::NodeHelper::nextSibling( node );
      }
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::setFcc( const QString &idString )
{
  // check if the sent-mail folder still exists
#if 0 // TODO: findFolderCollectionById deadlocks here, since it uses synchronous job execution, and we are
      // very indirectly called from a result slot of a previous job here (message retrieval in KMCommand)
      // which then deadlocks the Akonadi job scheduler as we never returned from the result slot yet.
      // possible solution is to either use async jobs or retrieve the  collection from the ETM, which can be done
      // synchronously as ETM has local caches.
  if ( ! idString.isEmpty() && KMKernel::self()->findFolderCollectionById( idString ).isValid() ) {
    mFcc->setDefaultCollection( KMKernel::self()->findFolderCollectionById( idString ) );
  } else {
    mFcc->setDefaultCollection( KMKernel::self()->sentCollectionFolder() );
  }
#else
  if ( idString.isEmpty() )
    mFcc->setDefaultCollection( KMKernel::self()->sentCollectionFolder() );
  else
    mFcc->setDefaultCollection( Akonadi::Collection( idString.toInt() ) );
#endif
}

//-----------------------------------------------------------------------------
bool KMComposeWin::isModified() const
{
  return ( mEditor->document()->isModified() ||
           mEdtFrom->isModified() ||
           ( mEdtReplyTo && mEdtReplyTo->isModified() ) ||
           mRecipientsEditor->isModified() ||
           mEdtSubject->isModified() );
           // || mAtmModified );
}

//-----------------------------------------------------------------------------
void KMComposeWin::setModified( bool modified )
{
  mEditor->document()->setModified( modified );
  if ( !modified ) {
    mEdtFrom->setModified( false );
    if ( mEdtReplyTo ) mEdtReplyTo->setModified( false );
    mRecipientsEditor->clearModified();
    mEdtSubject->setModified( false );
    //mAtmModified =  false ;
  }
}

//-----------------------------------------------------------------------------
bool KMComposeWin::queryClose ()
{
  if ( !mEditor->checkExternalEditorFinished() ) {
    return false;
  }
  if ( kmkernel->shuttingDown() || kapp->sessionSaving() ) {
    return true;
  }

  if( !mComposers.isEmpty() ) {
    kWarning() << "Tried to close while composer was active";
    return false;
  }

  if ( isModified() ) {
    bool istemplate = ( mFolder.isValid() && kmkernel->folderIsTemplates( mFolder ) );
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
                                                    KGuiItem(savebut, "document-save", QString(), savetext),
                                                    KStandardGuiItem::discard(),
                                                    KStandardGuiItem::cancel());
    if ( rc == KMessageBox::Cancel ) {
      return false;
    } else if ( rc == KMessageBox::Yes ) {
      // doSend will close the window. Just return false from this method
      if (istemplate) slotSaveTemplate();
      else slotSaveDraft();
      return false;
    }
    //else fall through: return true
  }
  cleanupAutoSave();
  return true;
}

//-----------------------------------------------------------------------------
bool KMComposeWin::userForgotAttachment()
{
  bool checkForForgottenAttachments = GlobalSettings::self()->showForgottenAttachmentWarning();

  if ( !checkForForgottenAttachments || ( mAttachmentModel->rowCount() > 0 ) ) {
    return false;
  }

  QStringList attachWordsList = GlobalSettings::self()->attachmentKeywords();
  if ( attachWordsList.isEmpty() ) {
    return false;
  }

  QRegExp rx ( QString::fromLatin1("\\b") +
               attachWordsList.join("\\b|\\b") +
               QString::fromLatin1("\\b") );
  rx.setCaseSensitivity( Qt::CaseInsensitive );

  bool gotMatch = false;

  // check whether the subject contains one of the attachment key words
  // unless the message is a reply or a forwarded message
  QString subj = subject();
  gotMatch = ( KMail::MessageHelper::stripOffPrefixes( subj ) == subj ) && ( rx.indexIn( subj ) >= 0 );

  if ( !gotMatch ) {
    // check whether the non-quoted text contains one of the attachment key
    // words
    QRegExp quotationRx ("^([ \\t]*([|>:}#]|[A-Za-z]+>))+");
    QTextDocument *doc = mEditor->document();
    for ( QTextBlock it = doc->begin(); it != doc->end(); it = it.next() ) {
      QString line = it.text();
      gotMatch = ( quotationRx.indexIn( line ) < 0 ) &&
                 ( rx.indexIn( line ) >= 0 );
      if ( gotMatch ) {
        break;
      }
    }
  }

  if ( !gotMatch ) {
    return false;
  }

  int rc = KMessageBox::warningYesNoCancel( this,
                                            i18n("The message you have composed seems to refer to an "
                                                 "attached file but you have not attached anything.\n"
                                                 "Do you want to attach a file to your message?"),
                                            i18n("File Attachment Reminder"),
                                            KGuiItem(i18n("&Attach File...")),
                                            KGuiItem(i18n("&Send as Is")) );
  if ( rc == KMessageBox::Cancel )
    return true;
  if ( rc == KMessageBox::Yes ) {
    mAttachmentController->showAddAttachmentDialog();
    //preceed with editing
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void KMComposeWin::readyForSending( bool noCrypto )
{
  kDebug() << "Entering, noCrypto:" << noCrypto;

  if(!mMsg) {
    kDebug() << "mMsg == 0!";
    return;
  }

  if( !mComposers.isEmpty() ) {
    // This may happen if e.g. the autosave timer calls applyChanges.
    kDebug() << "Called while composer active; ignoring.";
    return;
  }

  setEnabled( false );

  // we first figure out if we need to create multiple messages with different crypto formats
  // if so, we create a composer per format
  // if we aren't signing or encrypting, this just returns a single empty message
  if( noCrypto ) {
    mComposers.append( new Message::Composer );
  } else {
    mComposers = generateCryptoMessages( mSignAction->isChecked(), mEncryptAction->isChecked() );
  }

  if( mComposers.isEmpty() ) {
    setEnabled( true );
    return;
  }
  // Compose each message and queue it for sending.
  // TODO handle drafts, autosave, etc.
  foreach( Message::Composer* composer, mComposers ) {
    fillGlobalPart( composer->globalPart() );
    fillTextPart( composer->textPart() );
    fillInfoPart( composer->infoPart() );

    composer->addAttachmentParts( mAttachmentModel->attachments() );

    connect( composer, SIGNAL(result(KJob*)), this, SLOT(slotSendComposeResult(KJob*)) );
    composer->start();
    kDebug() << "Started a composer for sending!";
  }

}

QList< Message::Composer* > KMComposeWin::generateCryptoMessages( bool sign, bool encrypt )
{

  QList< Message::Composer* > composers;

  kDebug() << "filling crypto info";

  Kleo::KeyResolver* keyResolver = new Kleo::KeyResolver(  encryptToSelf(), showKeyApprovalDialog(),
                                                           GlobalSettings::self()->pgpAutoEncrypt(), cryptoMessageFormat(),
                                                           encryptKeyNearExpiryWarningThresholdInDays(),
                                                           signingKeyNearExpiryWarningThresholdInDays(),
                                                           encryptRootCertNearExpiryWarningThresholdInDays(),
                                                           signingRootCertNearExpiryWarningThresholdInDays(),
                                                           encryptChainCertNearExpiryWarningThresholdInDays(),
                                                           signingChainCertNearExpiryWarningThresholdInDays());
                                                           const KPIMIdentities::Identity &id = KMKernel::self()->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
  QStringList encryptToSelfKeys;
  QStringList signKeys;

  bool signSomething = sign;
  foreach( AttachmentPart::Ptr attachment, mAttachmentModel->attachments() ) {
    if( attachment->isSigned() )
      signSomething = true;
  }
  bool encryptSomething = encrypt;
  foreach( AttachmentPart::Ptr attachment, mAttachmentModel->attachments() ) {
    if( attachment->isEncrypted() )
      encryptSomething = true;
  }

   if( !signSomething && !encryptSomething ) {
    composers.append( new Message::Composer() );
    return composers;
  }

  if( encryptSomething ) {
    if ( !id.pgpEncryptionKey().isEmpty() )
      encryptToSelfKeys.push_back( id.pgpEncryptionKey() );
    if ( !id.smimeEncryptionKey().isEmpty() )
      encryptToSelfKeys.push_back( id.smimeEncryptionKey() );
    if ( keyResolver->setEncryptToSelfKeys( encryptToSelfKeys ) != Kpgp::Ok ) {
      kDebug() << "Failed to set encryptoToSelf keys!";
      return composers;
    }
  }

  if( signSomething ) {
    if ( !id.pgpSigningKey().isEmpty() )
      signKeys.push_back( id.pgpSigningKey() );
    if ( !id.smimeSigningKey().isEmpty() )
      signKeys.push_back( id.smimeSigningKey() );
    if ( keyResolver->setSigningKeys( signKeys ) != Kpgp::Ok ) {
      kDebug() << "Failed to set signing keys!";
      return composers;
    }
  }

  QStringList recipients, bcc;
  foreach( const Recipient &r, mRecipientsEditor->recipients() ) {
    switch( r.type() ) {
      case Recipient::To: recipients << r.email(); break;
      case Recipient::Cc: recipients << r.email(); break;
      case Recipient::Bcc: bcc << r.email(); break;
      default: Q_ASSERT( false ); break;
    }
  }

  keyResolver->setPrimaryRecipients( recipients );
  keyResolver->setSecondaryRecipients( bcc );

  if ( keyResolver->resolveAllKeys( signSomething, encryptSomething ) != Kpgp::Ok ) {
    /// TODO handle failure
    kDebug() << "failed to resolve keys! oh noes";
    return composers;
  }
  kDebug() << "done resolving keys:";

  Kleo::CryptoMessageFormat concreteEncryptFormat = Kleo::AutoFormat;
  if( encryptSomething ) {
    for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
      if ( keyResolver->encryptionItems( concreteCryptoMessageFormats[i] ).empty() )
        continue;

      if ( !(concreteCryptoMessageFormats[i] & cryptoMessageFormat()) )
        continue;

      concreteEncryptFormat = concreteCryptoMessageFormats[i];

      std::vector<Kleo::KeyResolver::SplitInfo> encData = keyResolver->encryptionItems( concreteEncryptFormat );
      std::vector<Kleo::KeyResolver::SplitInfo>::iterator it;
      QList<QPair<QStringList, std::vector<GpgME::Key> > > data;
      for( it = encData.begin(); it != encData.end(); ++it ) {
        QPair<QStringList, std::vector<GpgME::Key> > p( it->recipients, it->keys );
        data.append( p );
        kDebug() << "got resolved keys for:" << it->recipients;
      }
      Message::Composer* composer =  new Message::Composer;

      composer->setEncryptionKeys( data );
      composer->setMessageCryptoFormat( concreteEncryptFormat );

      if( signSomething ) {
        // find signing keys for this format
        std::vector<GpgME::Key> signingKeys = keyResolver->signingKeys( concreteEncryptFormat );
        composer->setSigningKeys( signingKeys );
      }

      composer->setSignAndEncrypt( sign, encrypt );

      composers.append( composer );
    }
  } else if( signSomething ) { // just signing, so check sign prefs
    Kleo::CryptoMessageFormat concreteSignFormat = Kleo::AutoFormat;

    for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
      if ( keyResolver->encryptionItems( concreteCryptoMessageFormats[i] ).empty() )
        continue;

      if ( !(concreteCryptoMessageFormats[i] & cryptoMessageFormat()) )
        continue;

      concreteSignFormat = concreteCryptoMessageFormats[i];
      std::vector<GpgME::Key> signingKeys = keyResolver->signingKeys( concreteEncryptFormat );

      Message::Composer* composer =  new Message::Composer;

      composer->setSigningKeys( signingKeys );
      composer->setMessageCryptoFormat( concreteSignFormat );
      composer->setSignAndEncrypt( sign, encrypt );

      composers.append( composer );
    }
  }

  if( composers.isEmpty() && ( signSomething || encryptSomething ) )
    Q_ASSERT_X( false, "KMComposeWin::fillCryptoInfo" , "No concrete sign or encrypt method selected");

  return composers;
}

void KMComposeWin::fillGlobalPart( Message::GlobalPart *globalPart )
{
  globalPart->setParentWidgetForGui( this );
  globalPart->setCharsets( mCodecAction->mimeCharsets() );
}

void KMComposeWin::fillTextPart( Message::TextPart *textPart )
{
  textPart->setCleanPlainText( mEditor->toCleanPlainText() );
  textPart->setWrappedPlainText( mEditor->toWrappedPlainText() );
  if( mEditor->isFormattingUsed() ) {
    textPart->setCleanHtml( mEditor->toCleanHtml() );
    textPart->setEmbeddedImages( mEditor->embeddedImages() );
  }
}

void KMComposeWin::fillInfoPart( Message::InfoPart *infoPart )
{
  QStringList to, cc, bcc;
  foreach( const Recipient &r, mRecipientsEditor->recipients() ) {
    switch( r.type() ) {
      case Recipient::To: to << r.email(); break;
      case Recipient::Cc: cc << r.email(); break;
      case Recipient::Bcc: bcc << r.email(); break;
      default: Q_ASSERT( false ); break;
    }
  }
  // TODO what about address groups and all that voodoo?

  // TODO splitAddressList and expandAliases ugliness should be handled by a
  // special AddressListEdit widget... (later: see RecipientsEditor)

  infoPart->setFcc( QString::number( mFcc->currentCollection().id() ) );
  infoPart->setTransportId( mTransport->currentTransportId() );
  infoPart->setFrom( from() );
  infoPart->setTo( to );
  infoPart->setCc( cc );
  infoPart->setBcc( bcc );
  infoPart->setSubject( subject() );

  KMime::Headers::Base::List extras;
  if( mMsg->headerByType( "X-KMail-SignatureActionEnabled" ) )
    extras << mMsg->headerByType( "X-KMail-SignatureActionEnabled" );
  if( mMsg->headerByType( "X-KMail-EncryptActionEnabled" ) )
    extras << mMsg->headerByType( "X-KMail-EncryptActionEnabled" );
  if( mMsg->headerByType( "X-KMail-CryptoMessageFormat" ) )
    extras << mMsg->headerByType( "X-KMail-CryptoMessageFormat" );

  infoPart->setExtraHeaders( extras );
}

void KMComposeWin::slotSendComposeResult( KJob *job )
{
  using Message::Composer;

  kDebug() << "error" << job->error() << "errorString" << job->errorString();
  Q_ASSERT( dynamic_cast< Composer* >( job ) );
  Composer* composer = dynamic_cast< Composer* >( job );

  Q_ASSERT( mComposers.contains( composer ) );

  if( composer->error() == Composer::NoError ) {
    // The messages were composed successfully.
    // TODO handle drafts
    kDebug() << "NoError.";
    for( int i = 0; i < composer->resultMessages().size(); ++i ) {
      if ( mSaveIn==None ) {
        queueMessage( KMime::Message::Ptr( composer->resultMessages()[i] ), composer );
      } else {
        saveMessage( KMime::Message::Ptr( composer->resultMessages()[i] ), mSaveIn );
      }
    }
  } else if( composer->error() == Composer::UserCancelledError ) {
    // The job warned the user about something, and the user chose to return
    // to the message.  Nothing to do.
    kDebug() << "UserCancelledError.";
    setEnabled( true );
  } else {
    kDebug() << "other Error.";
    QString msg;
    if( composer->error() == Composer::BugError ) {
      msg = i18n( "Error composing message:\n\n%1\n\nPlease report this bug.", job->errorString() );
    } else {
      msg = i18n( "Error composing message:\n\n%1", job->errorString() );
    }
    setEnabled( true );
    KMessageBox::sorry( this, msg, i18n( "Composer" ) );
  }

  mComposers.removeAll( composer );
}


void KMComposeWin::slotAutoSaveComposeResult( KJob *job )
{
  using Message::Composer;

  Q_ASSERT( dynamic_cast< Composer* >( job ) );
  Composer* composer = dynamic_cast< Composer* >( job );

  Q_ASSERT( mComposers.contains( composer ) );


  if( composer->error() == Composer::NoError ) {

    // The messages were composed successfully.
    for( int i = 0; i < composer->resultMessages().size(); ++i ) {
      writeAutoSaveToDisk( KMime::Message::Ptr( composer->resultMessages()[i] ), i );
    }
    if( autoSaveInterval() > 0 ) {
      updateAutoSave();
    }
    mComposers.removeAll( composer );
  } else {
    kWarning() << "Composer failed:" << composer->errorString();
  }
}

void KMComposeWin::saveMessage( KMime::Message::Ptr message, KMComposeWin::SaveIn saveIn )
{
  Akonadi::Collection target;

  if ( saveIn==KMComposeWin::Templates ) {
    target = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Templates );
  } else {
    target = Akonadi::SpecialMailCollections::self()->defaultCollection( Akonadi::SpecialMailCollections::Drafts );
  }

  if ( !target.isValid() ) {
    kWarning() << "No default collection for" << saveIn;
    return;
  }

  // Store when the draft or template got saved.
  message->date()->setDateTime( KDateTime::currentLocalDateTime() );
  message->assemble();

  Akonadi::Item item;
  item.setMimeType( "message/rfc822" );
  item.setPayload( message );
  Akonadi::ItemCreateJob *create = new Akonadi::ItemCreateJob( item, target, this );
  connect( create, SIGNAL(result(KJob*)), this, SLOT(slotCreateItemResult(KJob*)) );
  mPendingCreateItemJobs++;
}

void KMComposeWin::slotCreateItemResult( KJob *job )
{
  mPendingCreateItemJobs--;
  kDebug() << "mPendingCreateItemJobs" << mPendingCreateItemJobs;
  Q_ASSERT( mPendingCreateItemJobs >= 0 );

  if( job->error() ) {
    kDebug() << "Failed to save a message:" << job->errorString();
    return;
  }

  if( mPendingCreateItemJobs == 0 ) {
    setModified( false );
    mFolder = Akonadi::Collection( -1 );
    cleanupAutoSave();
    close();
  }
}

void KMComposeWin::queueMessage( KMime::Message::Ptr message, Message::Composer* composer )
{

  const Message::InfoPart *infoPart = composer->infoPart();
  MailTransport::MessageQueueJob *qjob = new MailTransport::MessageQueueJob( this );
  qjob->setMessage( message );
  qjob->transportAttribute().setTransportId( infoPart->transportId() );
  if( mSendMethod == KMail::MessageSender::SendLater )
    qjob->dispatchModeAttribute().setDispatchMode( MailTransport::DispatchModeAttribute::Manual );

  if ( !infoPart->fcc().isEmpty() ) {
    qjob->sentBehaviourAttribute().setSentBehaviour(
                      MailTransport::SentBehaviourAttribute::MoveToCollection );
    const Akonadi::Collection sentCollection( infoPart->fcc().toInt() );
    qjob->sentBehaviourAttribute().setMoveToCollection( sentCollection );
  } else {
    qjob->sentBehaviourAttribute().setSentBehaviour(
           MailTransport::SentBehaviourAttribute::MoveToDefaultSentCollection );
  }

  fillQueueJobHeaders( qjob, message, infoPart );

  connect( qjob, SIGNAL(result(KJob*)), this, SLOT(slotQueueResult(KJob*)) );
  mPendingQueueJobs++;
  qjob->start();

  kDebug() << "Queued a message.";
}

void KMComposeWin::fillQueueJobHeaders( MailTransport::MessageQueueJob* qjob, KMime::Message::Ptr message, const Message::InfoPart* infoPart )
{
  qjob->addressAttribute().setFrom( infoPart->from() );

  if( mEncryptAction->isChecked() && !infoPart->bcc().isEmpty() ) // have to deal with multiple message contents
  {
    // if the bcc isn't empty, then we send it to the bcc because this is the bcc-only encrypted body
    if( !message->bcc()->addresses().isEmpty() ) {
      QStringList bcc;
      foreach( QByteArray address, message->bcc()->addresses()  ) {
        bcc << QString::fromUtf8( address );
      }
      kDebug() << "sending with-bcc encr mail to a secondary recipient:" << bcc;
      qjob->addressAttribute().setTo( bcc );
    } else {
      // the main mail in the encrypted set, just don't set the bccs here
      qjob->addressAttribute().setTo( infoPart->to() );
      qjob->addressAttribute().setCc( infoPart->cc() );

      kDebug() << "sending with-bcc encrypted mail to orig recipients:" <<infoPart->to() << infoPart->cc();

    }
  } else {
    // continue as normal
    kDebug() << "no bccs";
    qjob->addressAttribute().setTo( infoPart->to() );
    qjob->addressAttribute().setCc( infoPart->cc() );
    qjob->addressAttribute().setBcc( infoPart->bcc() );
  }
}


bool KMComposeWin::encryptToSelf()
{
  // return !Kpgp::Module::getKpgp() || Kpgp::Module::getKpgp()->encryptToSelf();
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readEntry( "crypto-encrypt-to-self", true );
}

bool KMComposeWin::showKeyApprovalDialog()
{
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readEntry( "crypto-show-keys-for-approval", true );
}

int KMComposeWin::encryptKeyNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num = composer.readEntry( "crypto-warn-encr-key-near-expire-int", 14 );
  return qMax( 1, num );
}

int KMComposeWin::signingKeyNearExpiryWarningThresholdInDays()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num = composer.readEntry( "crypto-warn-sign-key-near-expire-int", 14 );
  return qMax( 1, num );
}

int KMComposeWin::encryptRootCertNearExpiryWarningThresholdInDays()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num = composer.readEntry( "crypto-warn-encr-root-near-expire-int", 14 );
  return qMax( 1, num );
}

int KMComposeWin::signingRootCertNearExpiryWarningThresholdInDays() {
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num =
  composer.readEntry( "crypto-warn-sign-root-near-expire-int", 14 );
  return qMax( 1, num );
}

int KMComposeWin::encryptChainCertNearExpiryWarningThresholdInDays()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num =
  composer.readEntry( "crypto-warn-encr-chaincert-near-expire-int", 14 );
  return qMax( 1, num );
}

int KMComposeWin::signingChainCertNearExpiryWarningThresholdInDays()
{
  const KConfigGroup composer( KMKernel::config(), "Composer" );
  if ( ! composer.readEntry( "crypto-warn-when-near-expire", true ) ) {
    return -1;
  }
  const int num =
  composer.readEntry( "crypto-warn-sign-chaincert-near-expire-int", 14 );
  return qMax( 1, num );
}

void KMComposeWin::slotQueueResult( KJob *job )
{
  mPendingQueueJobs--;
  kDebug() << "mPendingQueueJobs" << mPendingQueueJobs;
  Q_ASSERT( mPendingQueueJobs >= 0 );

  if( job->error() ) {
    kDebug() << "Failed to queue a message:" << job->errorString();
    // There is not much we can do now, since all the MessageQueueJobs have been
    // started.  So just wait for them to finish.
    // TODO show a message box or something
    return;
  }

  if( mPendingQueueJobs == 0 ) {
    setModified( false );
    cleanupAutoSave();
    close();
  }
}

const KPIMIdentities::Identity &KMComposeWin::identity() const
{
  return KMKernel::self()->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
}

uint KMComposeWin::identityUid() const {
  return mIdentity->currentIdentity();
}

Kleo::CryptoMessageFormat KMComposeWin::cryptoMessageFormat() const
{
  if ( !mCryptoModuleAction ) {
    return Kleo::AutoFormat;
  }
  return cb2format( mCryptoModuleAction->currentItem() );
}

bool KMComposeWin::encryptToSelf() const
{
//  return !Kpgp::Module::getKpgp() || Kpgp::Module::getKpgp()->encryptToSelf();
  KConfigGroup group( KMKernel::config(), "Composer" );
  return group.readEntry( "crypto-encrypt-to-self", true );
}

bool KMComposeWin::queryExit ()
{
  return true;
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttach( KMime::Content *msgPart )
{
  AttachmentPart::Ptr part( new AttachmentPart );
  part->setName( msgPart->contentDescription()->asUnicodeString() );
  part->setFileName( msgPart->contentDisposition()->filename() );
  part->setMimeType( msgPart->contentType()->mimeType() );
  part->setData( msgPart->decodedContent() );
  mAttachmentController->addAttachment( part );
}
//-----------------------------------------------------------------------------

QString KMComposeWin::prettyMimeType( const QString &type )
{
  const QString t = type.toLower();
  const KMimeType::Ptr st = KMimeType::mimeType( t );

  if ( !st ) {
    kWarning() <<"unknown mimetype" << t;
    return t;
  }

  QString pretty = !st->isDefault() ? st->comment() : t;
  if ( pretty.isEmpty() )
    return type;
  else
    return pretty;
}

void KMComposeWin::setAutoCharset()
{
  mCodecAction->setCurrentItem( 0 );
}

// We can't simply use KCodecAction::setCurrentCodec(), since that doesn't
// use fixEncoding().
static QString selectCharset( KSelectAction *root, const QString &encoding )
{
  foreach( QAction *action, root->actions() ) {
    KSelectAction *subMenu = dynamic_cast<KSelectAction *>( action );
    if ( subMenu ) {
      const QString codecNameToSet = selectCharset( subMenu, encoding );
      if ( !codecNameToSet.isEmpty() )
        return codecNameToSet;
    }
    else {
      const QString fixedActionText = MessageViewer::NodeHelper::fixEncoding( action->text() );
      if ( KGlobal::charsets()->codecForName(
                  KGlobal::charsets()->encodingForName( fixedActionText ) )
                 == KGlobal::charsets()->codecForName( encoding ) ) {
        return action->text();
      }
    }
  }
  return QString();
}

//-----------------------------------------------------------------------------
void KMComposeWin::setCharset( const QByteArray &charset )
{
  const QString codecNameToSet = selectCharset( mCodecAction, charset );
  if ( codecNameToSet.isEmpty() ) {
    kWarning() << "Could not find charset" << charset;
    setAutoCharset();
  }
  else
    mCodecAction->setCurrentCodec( codecNameToSet );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBook()
{
  KPIM::KAddrBookExternal::openAddressBook( this );
}

#if 0
// TODO port me (outlook names)

void KMComposeWin::slotAttachFileResult( KJob *job )
{
  // ...

  // For the encoding of the name, prefer the current charset of the composer first,
  // then try every other available encoding.
  QByteArray nameEncoding =
      KMail::Util::autoDetectCharset( mCharset, KMMessage::preferredCharsets(), name );
  if ( nameEncoding.isEmpty() )
    nameEncoding = "utf-8";

  QByteArray encodedName;
  if ( GlobalSettings::self()->outlookCompatibleAttachments() ) {
    encodedName = KMMsgBase::encodeRFC2047String( name, nameEncoding );
  } else {
    encodedName = KMMsgBase::encodeRFC2231String( name, nameEncoding );
  }

  bool RFC2231encoded = false;
  if ( !GlobalSettings::self()->outlookCompatibleAttachments() ) {
    RFC2231encoded = name != QString( encodedName );
  }

  // ...
}
#endif

//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertFile()
{
  KUrl u = mEditor->insertFile();
  if ( u.isEmpty() )
    return;

  mRecentAction->addUrl( u );
  // Prevent race condition updating list when multiple composers are open
  {
    KSharedConfig::Ptr config = KMKernel::config();
    KConfigGroup group( config, "Composer" );
    QString encoding = MessageViewer::NodeHelper::encodingForName( u.fileEncoding() ).toLatin1();
    QStringList urls = group.readEntry( "recent-urls", QStringList() );
    QStringList encodings = group.readEntry( "recent-encodings", QStringList() );
    // Prevent config file from growing without bound
    // Would be nicer to get this constant from KRecentFilesAction
    int mMaxRecentFiles = 30;
    while ( urls.count() > mMaxRecentFiles )
      urls.removeLast();
    while ( encodings.count() > mMaxRecentFiles )
      encodings.removeLast();
    // sanity check
    if ( urls.count() != encodings.count() ) {
      urls.clear();
      encodings.clear();
    }
    urls.prepend( u.prettyUrl() );
    encodings.prepend( encoding );
    group.writeEntry( "recent-urls", urls );
    group.writeEntry( "recent-encodings", encodings );
    mRecentAction->saveEntries( config->group( QString() ) );
  }
  slotInsertRecentFile( u );
}


void KMComposeWin::slotRecentListFileClear()
{
   KSharedConfig::Ptr config = KMKernel::config();
   KConfigGroup group( config, "Composer" );
   group.deleteEntry("recent-urls");
   group.deleteEntry("recent-encodings");
   mRecentAction->saveEntries( config->group( QString() ) );
}
//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertRecentFile( const KUrl &u )
{
  kDebug() << "implement me...";
  if ( u.fileName().isEmpty() ) {
    return;
  }
#if 0
  KIO::Job *job = KIO::get( u );
  atmLoadData ld;
  ld.url = u;
  ld.data = QByteArray();
  ld.insert = true;
  // Get the encoding previously used when inserting this file
  {
    KConfig *config = KMKernel::config();
    KConfigGroup group( config, "Composer" );
    QStringList urls = group.readEntry( "recent-urls", QStringList() );
    QStringList encodings = group.readEntry( "recent-encodings", QStringList() );
    int index = urls.indexOf( u.prettyUrl() );
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
#endif
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSelectCryptoModule( bool init )
{
  if ( !init )
    setModified( true );

  mAttachmentModel->setEncryptEnabled( canSignEncryptAttachments() );
  mAttachmentModel->setSignEnabled( canSignEncryptAttachments() );
}

//-----------------------------------------------------------------------------
bool KMComposeWin::inlineSigningEncryptionSelected()
{
  if ( !mSignAction->isChecked() && !mEncryptAction->isChecked() ) {
    return false;
  }
  return cryptoMessageFormat() == Kleo::InlineOpenPGPFormat;
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdateFont()
{
  kDebug();
  if ( !mFixedFontAction ) {
    return;
  }
  mEditor->setFontForWholeText( mFixedFontAction->isChecked() ?
                                  mFixedFont : mBodyFont );
}

QString KMComposeWin::smartQuote( const QString & msg )
{
  return MessageCore::StringUtil::smartQuote( msg, GlobalSettings::self()->lineWrapWidth() );
}

void KMComposeWin::slotPasteAsAttachment()
{
  const QMimeData *mimeData = QApplication::clipboard()->mimeData();

  if( mimeData->hasUrls() ) {
    // If the clipboard contains a list of URL, attach each file.
    const KUrl::List urls = KUrl::List::fromMimeData( mimeData );
    foreach( const KUrl &url, urls ) {
      mAttachmentController->addAttachment( url );
    }
  } else if( mimeData->hasText() ) {
    bool ok;
    const QString attName = KInputDialog::getText(
        i18n( "Insert clipboard text as attachment" ),
        i18n( "Name of the attachment:" ),
        QString(), &ok, this );
    if( ok ) {
      AttachmentPart::Ptr part = AttachmentPart::Ptr( new AttachmentPart );
      part->setName( attName );
      part->setFileName( attName );
      part->setMimeType( "text/plain" );
      part->setData( QApplication::clipboard()->text().toLatin1() );
      mAttachmentController->addAttachment( part );
    }
  }
}

QString KMComposeWin::addQuotesToText( const QString &inputText ) const
{
  QString answer = QString( inputText );
  QString indentStr = mEditor->quotePrefixName();
  answer.replace( '\n', '\n' + indentStr );
  answer.prepend( indentStr );
  answer += '\n';
  return MessageCore::StringUtil::smartQuote( answer, GlobalSettings::self()->lineWrapWidth() );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotUndo()
{
  QWidget *fw = focusWidget();
  if ( !fw ) {
    return;
  }

  if ( ::qobject_cast<KMComposerEditor*>( fw ) ) {
    static_cast<KTextEdit*>( fw )->undo();
  } else if (::qobject_cast<KLineEdit*>( fw )) {
    static_cast<KLineEdit*>( fw )->undo();
  }
}

void KMComposeWin::slotRedo()
{
  QWidget *fw = focusWidget();
  if ( !fw ) {
    return;
  }

  if ( ::qobject_cast<KMComposerEditor*>( fw ) ) {
    static_cast<KTextEdit*>( fw )->redo();
  } else if (::qobject_cast<KLineEdit*>( fw )) {
    static_cast<KLineEdit*>( fw )->redo();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCut()
{
  QWidget *fw = focusWidget();
  if ( !fw ) {
    return;
  }

  if ( ::qobject_cast<KMComposerEditor*>( fw ) ) {
    static_cast<KTextEdit*>(fw)->cut();
  } else if ( ::qobject_cast<KLineEdit*>( fw ) ) {
    static_cast<KLineEdit*>( fw )->cut();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCopy()
{
  QWidget *fw = focusWidget();
  if ( !fw ) {
    return;
  }
  QKeyEvent k( QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier );
  qApp->notify( fw, &k );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotPaste()
{
  QWidget *fw = focusWidget();
  if ( !fw ) {
    return;
  }
  if ( fw == mEditor ) {
    mEditor->paste();
  }
  else {
    QKeyEvent k( QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier );
    qApp->notify( fw, &k );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotMarkAll()
{
  QWidget *fw = focusWidget();
  if ( !fw ) {
    return;
  }

  if ( ::qobject_cast<KLineEdit*>( fw ) ) {
    static_cast<KLineEdit*>( fw )->selectAll();
  } else if (::qobject_cast<KMComposerEditor*>( fw )) {
    static_cast<KTextEdit*>( fw )->selectAll();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotClose()
{
  close();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotNewComposer()
{
  KMComposeWin *win;
  KMime::Message::Ptr msg( new KMime::Message );

  KMail::MessageHelper::initHeader( msg );
  win = new KMComposeWin( msg );
  win->show();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotNewMailReader()
{
  KMMainWin *kmmwin = new KMMainWin( 0 );
  kmmwin->show();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdWinTitle( const QString &text )
{
  QString s( text );
  // Remove characters that show badly in most window decorations:
  // newlines tend to become boxes.
  if ( text.isEmpty() ) {
    setCaption( '(' + i18n("unnamed") + ')' );
  } else {
    setCaption( s.replace( QChar('\n'), ' ' ) );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotEncryptToggled( bool on )
{
  setEncryption( on, true );
  slotUpdateSignatureAndEncrypionStateIndicators();
}

//-----------------------------------------------------------------------------
void KMComposeWin::setEncryption( bool encrypt, bool setByUser )
{
  bool wasModified = isModified();
  if ( setByUser ) {
    setModified( true );
  }
  if ( !mEncryptAction->isEnabled() ) {
    encrypt = false;
  }
  // check if the user wants to encrypt messages to himself and if he defined
  // an encryption key for the current identity
  else if ( encrypt && encryptToSelf() && !mLastIdentityHasEncryptionKey ) {
    if ( setByUser ) {
      KMessageBox::sorry( this,
                          i18n("<qt><p>You have requested that messages be "
                               "encrypted to yourself, but the currently selected "
                               "identity does not define an (OpenPGP or S/MIME) "
                               "encryption key to use for this.</p>"
                               "<p>Please select the key(s) to use "
                               "in the identity configuration.</p>"
                               "</qt>"),
                          i18n("Undefined Encryption Key") );
      setModified( wasModified );
    }
    encrypt = false;
  }

  // make sure the mEncryptAction is in the right state
  mEncryptAction->setChecked( encrypt );

  // show the appropriate icon
  if ( encrypt ) {
    mEncryptAction->setIcon( KIcon( "document-encrypt" ) );
  } else {
    mEncryptAction->setIcon( KIcon( "document-decrypt" ) );
  }

  // mark the attachments for (no) encryption
  if( canSignEncryptAttachments() ) {
    mAttachmentModel->setEncryptSelected( encrypt );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSignToggled( bool on )
{
  setSigning( on, true );
  slotUpdateSignatureAndEncrypionStateIndicators();
}

//-----------------------------------------------------------------------------
void KMComposeWin::setSigning( bool sign, bool setByUser )
{
  bool wasModified = isModified();
  if ( setByUser ) {
    setModified( true );
  }
  if ( !mSignAction->isEnabled() ) {
    sign = false;
  }

  // check if the user defined a signing key for the current identity
  if ( sign && !mLastIdentityHasSigningKey ) {
    if ( setByUser ) {
      KMessageBox::sorry( this,
                          i18n("<qt><p>In order to be able to sign "
                               "this message you first have to "
                               "define the (OpenPGP or S/MIME) signing key "
                               "to use.</p>"
                               "<p>Please select the key to use "
                               "in the identity configuration.</p>"
                               "</qt>"),
                          i18n("Undefined Signing Key") );
      setModified( wasModified );
    }
    sign = false;
  }

  // make sure the mSignAction is in the right state
  mSignAction->setChecked( sign );

  // mark the attachments for (no) signing
  if ( canSignEncryptAttachments() ) {
    mAttachmentModel->setSignSelected( sign );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotWordWrapToggled( bool on )
{
  if ( on )
    mEditor->enableWordWrap( GlobalSettings::self()->lineWrapWidth() );
  else
    mEditor->disableWordWrap();
}

//-----------------------------------------------------------------------------
void KMComposeWin::disableWordWrap()
{
  mEditor->setWordWrapMode( QTextOption::NoWrap );
}

//-----------------------------------------------------------------------------
void KMComposeWin::forceDisableHtml()
{
  mForceDisableHtml = true;
  disableHtml( NoConfirmationNeeded );
  markupAction->setEnabled( false );
  // FIXME: Remove the toggle toolbar action somehow
}

void KMComposeWin::disableRecipientNumberCheck()
{
  // mCheckForRecipients = false; // TODO composer...
}

void KMComposeWin::disableForgottenAttachmentsCheck()
{
  mCheckForForgottenAttachments = false;
}

void KMComposeWin::ignoreStickyFields()
{
  mIgnoreStickyFields = true;
  mBtnTransport->setChecked( false );
  mBtnIdentity->setChecked( false );
  mBtnTransport->setEnabled( false );
  mBtnIdentity->setEnabled( false );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotPrint()
{
  kDebug() << "Implement me.";
#if 0
  mMessageWasModified = isModified();
  connect( this, SIGNAL( applyChangesDone( bool ) ),
           this, SLOT( slotContinuePrint( bool ) ) );
  applyChanges( true );
#endif
}

void KMComposeWin::slotContinuePrint( bool rc )
{
  Q_ASSERT( false );
#if 0
  disconnect( this, SIGNAL( applyChangesDone( bool ) ),
              this, SLOT( slotContinuePrint( bool ) ) );

  if ( rc ) {
    if ( mComposedMessages.isEmpty() ) {
      kDebug() << "Composing the message failed.";
      return;
    }
    KMCommand *command = new KMPrintCommand( this, mComposedMessages.first() );
    command->start();
    setModified( mMessageWasModified );
  }
#endif
}

//----------------------------------------------------------------------------
void KMComposeWin::doSend( KMail::MessageSender::SendMethod method,
                           KMComposeWin::SaveIn saveIn )
{
  // TODO integrate with MDA online status
  if ( method != KMail::MessageSender::SendLater && KMKernel::self()->isOffline() ) {
    KMessageBox::information( this,
                              i18n("KMail is currently in offline mode. "
                                   "Your messages will be kept in the outbox until you go online."),
                              i18n("Online/Offline"), "kmailIsOffline" );
    mSendMethod = KMail::MessageSender::SendLater;
  } else {
    mSendMethod = method;
  }
  mSaveIn = saveIn;

  if ( saveIn == KMComposeWin::None ) {
    if ( KPIMUtils::firstEmailAddress( from() ).isEmpty() ) {
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
    if ( to().isEmpty() ) {
      if ( cc().isEmpty() && bcc().isEmpty() ) {
        KMessageBox::information( this,
                                  i18n("You must specify at least one receiver,"
                                       "either in the To: field or as CC or as BCC.") );

        return;
      } else {
        int rc = KMessageBox::questionYesNo( this,
                                             i18n("To: field is empty. "
                                                  "Send message anyway?"),
                                             i18n("No To: specified"),
                                             KStandardGuiItem::yes(),
                                             KStandardGuiItem::no(),
                                             ":kmail_no_to_field_specified" );
        if ( rc == KMessageBox::No ) {
          return;
        }
      }
    }

    // Validate the To:, CC: and BCC fields
    if ( !KMail::Util::validateAddresses( this, to().trimmed() ) ) {
      return;
    }

    if ( !KMail::Util::validateAddresses( this, cc().trimmed() ) ) {
      return;
    }

    if ( !KMail::Util::validateAddresses( this, bcc().trimmed() ) ) {
      return;
    }

    if ( subject().isEmpty() ) {
      mEdtSubject->setFocus();
      int rc =
        KMessageBox::questionYesNo( this,
                                    i18n("You did not specify a subject. "
                                         "Send message anyway?"),
                                    i18n("No Subject Specified"),
                                    KGuiItem(i18n("S&end as Is")),
                                    KGuiItem(i18n("&Specify the Subject")),
                                    "no_subject_specified" );
      if ( rc == KMessageBox::No ) {
        return;
      }
    }

    if ( userForgotAttachment() ) {
      return;
    }
  }

  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );

  mMsg->date()->setDateTime( KDateTime::currentLocalDateTime() );
  mMsg->setHeader( new KMime::Headers::Generic( "X-KMail-Transport", mMsg.get(), mTransport->currentText(), "utf-8" ) );

  const bool neverEncrypt = ( saveIn != KMComposeWin::None && GlobalSettings::self()->neverEncryptDrafts() ) ||
    mSigningAndEncryptionExplicitlyDisabled;

  // Save the quote prefix which is used for this message. Each message can have
  // a different quote prefix, for example depending on the original sender.
  if ( mEditor->quotePrefixName().isEmpty() )
    mMsg->removeHeader( "X-KMail-QuotePrefix" );
  else
    mMsg->setHeader( new KMime::Headers::Generic("X-KMail-QuotePrefix", mMsg.get(), mEditor->quotePrefixName(), "utf-8" ) );

  if ( mEditor->isFormattingUsed() ) {
    kDebug() << "Html mode";
    mMsg->setHeader( new KMime::Headers::Generic("X-KMail-Markup", mMsg.get(), "true", "utf-8" ) );
  } else {
    mMsg->removeHeader( "X-KMail-Markup" );
    kDebug() << "Plain text";
  }
  if ( mEditor->isFormattingUsed() &&
       inlineSigningEncryptionSelected() ) {
    QString keepBtnText = mEncryptAction->isChecked() ?
      mSignAction->isChecked() ? i18n( "&Keep markup, do not sign/encrypt" )
      : i18n( "&Keep markup, do not encrypt" )
      : i18n( "&Keep markup, do not sign" );
    QString yesBtnText = mEncryptAction->isChecked() ?
      mSignAction->isChecked() ? i18n("Sign/Encrypt (delete markup)")
      : i18n( "Encrypt (delete markup)" )
      : i18n( "Sign (delete markup)" );
    int ret = KMessageBox::warningYesNoCancel( this,
                                               i18n("<qt><p>Inline signing/encrypting of HTML messages is not possible;</p>"
                                                    "<p>do you want to delete your markup?</p></qt>"),
                                               i18n("Sign/Encrypt Message?"),
                                               KGuiItem( yesBtnText ),
                                               KGuiItem( keepBtnText ) );
    if ( KMessageBox::Cancel == ret ) {
      return;
    }
    if ( KMessageBox::No == ret ) {
      mEncryptAction->setChecked( false );
      mSignAction->setChecked( false );
    } else {
      disableHtml( NoConfirmationNeeded );
    }
  }

  if ( mForceDisableHtml )
    disableHtml( NoConfirmationNeeded );

  if ( neverEncrypt && saveIn != KMComposeWin::None ) {
      // we can't use the state of the mail itself, to remember the
      // signing and encryption state, so let's add a header instead
    mMsg->setHeader( new KMime::Headers::Generic( "X-KMail-SignatureActionEnabled", mMsg.get(), mSignAction->isChecked()? "true":"false", "utf-8" ) );
    mMsg->setHeader( new KMime::Headers::Generic( "X-KMail-EncryptActionEnabled", mMsg.get(), mEncryptAction->isChecked()? "true":"false", "utf-8" ) );
    mMsg->setHeader( new KMime::Headers::Generic( "X-KMail-CryptoMessageFormat", mMsg.get(), QString::number( cryptoMessageFormat() ), "utf-8" ) );
  } else {
    mMsg->removeHeader( "X-KMail-SignatureActionEnabled" );
    mMsg->removeHeader( "X-KMail-EncryptActionEnabled" );
    mMsg->removeHeader( "X-KMail-CryptoMessageFormat" );
  }

  kDebug() << "Calling applyChanges()";
  readyForSending( neverEncrypt );
}

bool KMComposeWin::saveDraftOrTemplate( const QString &folderName,
                                        KMime::Message *msg )
{
#if 0
  KMFolder *theFolder = 0, *imapTheFolder = 0;
  // get the draftsFolder
  if ( !folderName.isEmpty() ) {
    theFolder = kmkernel->folderMgr()->findIdString( folderName );
    if ( theFolder == 0 ) {
      // This is *NOT* supposed to be "imapDraftsFolder", because a
      // dIMAP folder works like a normal folder
      theFolder = kmkernel->dimapFolderMgr()->findIdString( folderName );
    }
    if ( theFolder == 0 ) {
      imapTheFolder = kmkernel->imapFolderMgr()->findIdString( folderName );
    }
    if ( !theFolder && !imapTheFolder ) {
      const KPIMIdentities::Identity &id = kmkernel->identityManager()->identityForUoidOrDefault( msg->headerByType( "X-KMail-Identity" ) ? msg->headerByType( "X-KMail-Identity" )->asUnicodeString().trimmed().toUInt() : 0 );
      KMessageBox::information( 0,
                                i18n("The custom drafts or templates folder for "
                                     "identify \"%1\" does not exist (anymore); "
                                     "therefore, the default drafts or templates "
                                     "folder will be used.",
                                     id.identityName() ) );
    }
  }
  if ( imapTheFolder && imapTheFolder->noContent() ) {
    imapTheFolder = 0;
  }

  if ( theFolder == 0 ) {
    theFolder = ( mSaveIn == KMComposeWin::Drafts ?
                  kmkernel->draftsFolder() : kmkernel->templatesFolder() );
  }

  theFolder->open( "composer" );
  kDebug() << "theFolder=" << theFolder->name();
  if ( imapTheFolder ) {
    kDebug() << "imapTheFolder=" << imapTheFolder->name();
  }

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
  theFolder->close( "composer" );
  return sentOk;
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
  return true;
#endif
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSendLater()
{
  if ( !TransportManager::self()->showTransportCreationDialog( this, TransportManager::IfNoTransportExists ) )
    return;
  if ( !checkRecipientNumber() )
      return;
  if ( mEditor->checkExternalEditorFinished() ) {
    doSend( KMail::MessageSender::SendLater );
  }
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSaveDraft()
{
  if ( mEditor->checkExternalEditorFinished() ) {
    doSend( KMail::MessageSender::SendLater, KMComposeWin::Drafts );
  }
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSaveTemplate()
{
  if ( mEditor->checkExternalEditorFinished() ) {
    doSend( KMail::MessageSender::SendLater, KMComposeWin::Templates );
  }
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSendNowVia( QAction *item )
{
  QList<int> availTransports= TransportManager::self()->transportIds();
  int transport = item->data().toInt();
  if ( availTransports.contains( transport ) ) {
    mTransport->setCurrentTransport( transport );
    slotSendNow();
  }
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSendLaterVia( QAction *item )
{
  QList<int> availTransports= TransportManager::self()->transportIds();
  int transport = item->data().toInt();
  if ( availTransports.contains( transport ) ) {
    mTransport->setCurrentTransport( transport );
    slotSendLater();
  }
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSendNow()
{
  if ( !mEditor->checkExternalEditorFinished() ) {
    return;
  }
  if ( !TransportManager::self()->showTransportCreationDialog( this, TransportManager::IfNoTransportExists ) )
    return;
  if ( !checkRecipientNumber() )
    return;

  if ( GlobalSettings::self()->confirmBeforeSend() ) {
    int rc = KMessageBox::warningYesNoCancel( mMainWidget,
                                              i18n("About to send email..."),
                                              i18n("Send Confirmation"),
                                              KGuiItem( i18n("&Send Now") ),
                                              KGuiItem( i18n("Send &Later") ) );

    if ( rc == KMessageBox::Yes ) {
      doSend( KMail::MessageSender::SendImmediate );
    } else if ( rc == KMessageBox::No ) {
      doSend( KMail::MessageSender::SendLater );
    }
  } else {
    doSend( KMail::MessageSender::SendImmediate );
  }
}

//----------------------------------------------------------------------------
bool KMComposeWin::checkRecipientNumber() const
{
  int thresHold = GlobalSettings::self()->recipientThreshold();
  if ( GlobalSettings::self()->tooManyRecipients() && mRecipientsEditor->recipients().count() > thresHold ) {
    if ( KMessageBox::questionYesNo( mMainWidget,
         i18n("You are trying to send the mail to more than %1 recipients. Send message anyway?", thresHold),
         i18n("Too many recipients"),
         KGuiItem( i18n("&Send as Is") ),
         KGuiItem( i18n("&Edit Recipients") ) ) == KMessageBox::No ) {
            return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void KMComposeWin::slotAppendSignature()
{
  insertSignatureHelper( KPIMIdentities::Signature::End );
}

//----------------------------------------------------------------------------
void KMComposeWin::slotPrependSignature()
{
  insertSignatureHelper( KPIMIdentities::Signature::Start );
}

//----------------------------------------------------------------------------
void KMComposeWin::slotInsertSignatureAtCursor()
{
  insertSignatureHelper( KPIMIdentities::Signature::AtCursor );
}

//----------------------------------------------------------------------------
void KMComposeWin::insertSignatureHelper( KPIMIdentities::Signature::Placement placement )
{
  // Identity::signature() is not const, although it should be, therefore the
  // const_cast.
  KPIMIdentities::Identity &ident = const_cast<KPIMIdentities::Identity&>(
      kmkernel->identityManager()->identityForUoidOrDefault(
                                mIdentity->currentIdentity() ) );
  const KPIMIdentities::Signature signature = ident.signature();

  if ( signature.isInlinedHtml() &&
       signature.type() == KPIMIdentities::Signature::Inlined ) {
    enableHtml();
  }
  KPIMIdentities::Signature::AddedText addedText = KPIMIdentities::Signature::AddNewLines;
    if ( GlobalSettings::self()->dashDashSignature() )
      addedText |= KPIMIdentities::Signature::AddSeparator;
  signature.insertIntoTextEdit( placement, addedText, mEditor );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotHelp()
{
  KToolInvocation::invokeHelp();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotCleanSpace()
{
  KPIMIdentities::Identity &ident = const_cast<KPIMIdentities::Identity&>(
      kmkernel->identityManager()->identityForUoidOrDefault(
                                mIdentity->currentIdentity() ) );
  KPIMIdentities::Signature signature = ident.signature();
  mEditor->cleanWhitespace( signature );
}

//-----------------------------------------------------------------------------
void KMComposeWin::enableHtml()
{
  if ( mForceDisableHtml ) {
    disableHtml( NoConfirmationNeeded );;
    return;
  }

  mEditor->enableRichTextMode();
  if ( !toolBar( "htmlToolBar" )->isVisible() ) {
    // Use singleshot, as we we might actually be called from a slot that wanted to disable the
    // toolbar (but the messagebox in disableHtml() prevented that and called us).
    // The toolbar can't correctly deal with being enabled right in a slot called from the "disabled"
    // signal, so wait one event loop run for that.
    QTimer::singleShot( 0, toolBar( "htmlToolBar" ), SLOT( show() ) );
  }
  if ( !markupAction->isChecked() )
    markupAction->setChecked( true );

  mSaveFont = mEditor->currentFont();
  mEditor->updateActionStates();
}

//-----------------------------------------------------------------------------
void KMComposeWin::disableHtml( Confirmation confirmation )
{
  if ( confirmation == LetUserConfirm && mEditor->isFormattingUsed() && !mForceDisableHtml ) {
    int choice = KMessageBox::warningContinueCancel( this, i18n( "Turning HTML mode off "
        "will cause the text to lose the formatting. Are you sure?" ),
        i18n( "Lose the formatting?" ), KGuiItem( i18n( "Lose Formatting" ) ), KStandardGuiItem::cancel(),
              "LoseFormattingWarning" );
    if ( choice != KMessageBox::Continue ) {
      enableHtml();
      return;
    }
  }

  mEditor->switchToPlainText();
  slotUpdateFont();
  if ( toolBar( "htmlToolBar" )->isVisible() ) {
    // See the comment in enableHtml() why we use a singleshot timer, similar situation here.
    QTimer::singleShot( 0, toolBar( "htmlToolBar" ), SLOT( hide() ) );
  }
  if ( markupAction->isChecked() )
    markupAction->setChecked( false );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotToggleMarkup()
{
  if ( markupAction->isChecked() )
    enableHtml();
  else
    disableHtml( LetUserConfirm );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotTextModeChanged( KPIM::KMeditor::Mode mode )
{
  if ( mode == KMeditor::Plain )
    disableHtml( NoConfirmationNeeded ); // ### Can this happen at all?
  else
    enableHtml();
}

//-----------------------------------------------------------------------------
void KMComposeWin::htmlToolBarVisibilityChanged( bool visible )
{
  if ( visible )
    enableHtml();
  else
    disableHtml( LetUserConfirm );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAutoSpellCheckingToggled( bool on )
{
  mAutoSpellCheckingAction->setChecked( on );
  if ( on != mEditor->checkSpellingEnabled() )
    mEditor->setCheckSpellingEnabled( on );

  QString temp;
  if ( on ) {
    temp = i18n( "Spellcheck: on" );
  } else {
    temp = i18n( "Spellcheck: off" );
  }
  statusBar()->changeItem( temp, 3 );
}

void KMComposeWin::slotSpellCheckingStatus(const QString & status)
{
  statusBar()->changeItem( status, 0 );
  QTimer::singleShot( 2000, this, SLOT(slotSpellcheckDoneClearStatus()) );
}

void KMComposeWin::slotSpellcheckDoneClearStatus()
{
  statusBar()->changeItem("", 0);
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotIdentityChanged( uint uoid, bool initalChange )
{
  if( mMsg == 0 ) {
    kDebug() << "Trying to change identity but mMsg == 0!";
    return;
  }

  const KPIMIdentities::Identity &ident =
    KMKernel::self()->identityManager()->identityForUoid( uoid );
  if ( ident.isNull() ) {
    return;
  }

  emit identityChanged( identity() );

  if ( !ident.fullEmailAddr().isNull() ) {
    mEdtFrom->setText( ident.fullEmailAddr() );
  }

  // make sure the From field is shown if it does not contain a valid email address
  if ( KPIMUtils::firstEmailAddress( from() ).isEmpty() ) {
    mShowHeaders |= HDR_FROM;
  }
  if ( mEdtReplyTo ) {
    mEdtReplyTo->setText( ident.replyToAddr() );
  }

  // remove BCC of old identity and add BCC of new identity (if they differ)
  const KPIMIdentities::Identity &oldIdentity =
      KMKernel::self()->identityManager()->identityForUoidOrDefault( mId );
  if ( oldIdentity.bcc() != ident.bcc() ) {
    mRecipientsEditor->removeRecipient( oldIdentity.bcc(), Recipient::Bcc );
    mRecipientsEditor->addRecipient( ident.bcc(), Recipient::Bcc );
    mRecipientsEditor->setFocusBottom();
  }

  if ( ident.organization().isEmpty() ) {
    mMsg->organization()->clear();
  } else {
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "Organization", mMsg.get(), ident.organization(), "utf-8" );
    mMsg->setHeader( header );
  }
  if ( !ident.isXFaceEnabled() || ident.xface().isEmpty() ) {
    mMsg->removeHeader( "X-Face" );
  } else {
    QString xface = ident.xface();
    if ( !xface.isEmpty() ) {
      int numNL = ( xface.length() - 1 ) / 70;
      for ( int i = numNL; i > 0; --i ) {
        xface.insert( i*70, "\n\t" );
      }
      KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-Face", mMsg.get(), xface, "utf-8" );
      mMsg->setHeader( header );
    }
  }
  // If the transport sticky checkbox is not checked, set the transport
  // from the new identity
  if ( !mBtnTransport->isChecked() && !mIgnoreStickyFields ) {
    QString transportName = ident.transport();
    Transport *transport =
        TransportManager::self()->transportByName( transportName, false );
    if ( !transport ) {
      mMsg->removeHeader( "X-KMail-Transport" );
      mTransport->setCurrentTransport(
                               TransportManager::self()->defaultTransportId() );
    }
    else {
      KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Transport", mMsg.get(), transportName, "utf-8" );
      mMsg->setHeader( header );
      mTransport->setCurrentTransport( transport->id() );
    }
  }

  mDictionaryCombo->setCurrentByDictionaryName( ident.dictionary() );
  mEditor->setSpellCheckingLanguage( mDictionaryCombo->currentDictionary() );

  if ( !mBtnFcc->isChecked() && !mPreventFccOverwrite ) {
    setFcc( ident.fcc() );
  }

  KPIMIdentities::Signature oldSig = const_cast<KPIMIdentities::Identity&>
                                               ( oldIdentity ).signature();
  KPIMIdentities::Signature newSig = const_cast<KPIMIdentities::Identity&>
                                               ( ident ).signature();
  // if unmodified, apply new template, if one is set
  bool msgCleared = false;
  if ( !isModified() && !( ident.templates().isEmpty() && mCustomTemplate.isEmpty() ) &&
       !initalChange ) {
    applyTemplate( uoid );
    msgCleared = true;
  }

  //replace existing signatures
  const bool replaced = mEditor->replaceSignature( oldSig, newSig );

  // Just append the signature if there was no old signature
  if ( !replaced && ( msgCleared || oldSig.rawText().isEmpty() ) ) {
    if ( GlobalSettings::self()->autoTextSignature() == "auto" ) {
      KPIMIdentities::Signature::AddedText addedText = KPIMIdentities::Signature::AddNewLines;
      if ( GlobalSettings::self()->dashDashSignature() )
        addedText |= KPIMIdentities::Signature::AddSeparator;
      if ( GlobalSettings::self()->prependSignature() )
        newSig.insertIntoTextEdit( KPIMIdentities::Signature::Start,
                                   addedText, mEditor );
      else
        newSig.insertIntoTextEdit( KPIMIdentities::Signature::End,
                                   addedText, mEditor );
    }
  }

  // disable certain actions if there is no PGP user identity set
  // for this profile
  bool bNewIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
  bool bNewIdentityHasEncryptionKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
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
  if ( bNewIdentityHasEncryptionKey && !mLastIdentityHasEncryptionKey ) {
    setEncryption( mLastEncryptActionState );
  }
  if ( bNewIdentityHasSigningKey && !mLastIdentityHasSigningKey ) {
    setSigning( mLastSignActionState );
  }

  mLastIdentityHasSigningKey = bNewIdentityHasSigningKey;
  mLastIdentityHasEncryptionKey = bNewIdentityHasEncryptionKey;

  mId = uoid;

  // make sure the From and BCC fields are shown if necessary
  rethinkFields( false );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSpellcheckConfig()
{
  mEditor->showSpellConfigDialog( "kmailrc" );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotEditToolbars()
{
  saveMainWindowSettings( KMKernel::config()->group( "Composer") );
  KEditToolBar dlg( guiFactory(), this );

  connect( &dlg, SIGNAL(newToolbarConfig()),
           SLOT(slotUpdateToolbars()) );

  dlg.exec();
}

void KMComposeWin::slotUpdateToolbars()
{
  createGUI( "kmcomposerui.rc" );
  applyMainWindowSettings( KMKernel::config()->group( "Composer") );
}

void KMComposeWin::slotEditKeys()
{
  KShortcutsDialog::configure( actionCollection(),
                               KShortcutsEditor::LetterShortcutsDisallowed );
}

void KMComposeWin::setReplyFocus( bool hasMessage )
{
  Q_UNUSED( hasMessage );

  // The cursor position is already set by setMsg(), so we only need to set the
  // focus here.
  mEditor->setFocus();
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
  kDebug() << "initalising autosave";

  // Ensure that the autosave directory exsits.
  QDir dataDirectory( KMKernel::localDataPath() );
  if( !dataDirectory.exists( "autosave" ) ) {
    kDebug() << "Creating autosave directory.";
    dataDirectory.mkdir( "autosave" );
  }

  // Construct a file name
  if ( mAutoSaveUUID.isEmpty() ) {
    mAutoSaveUUID = QUuid::createUuid().toString();
  }

  updateAutoSave();
}

void KMComposeWin::updateAutoSave()
{
  if ( autoSaveInterval() == 0 ) {
    delete mAutoSaveTimer; mAutoSaveTimer = 0;
  } else {
    if ( !mAutoSaveTimer ) {
      mAutoSaveTimer = new QTimer( this );
      connect( mAutoSaveTimer, SIGNAL( timeout() ),
               this, SLOT( autoSaveMessage() ) );
    }
    mAutoSaveTimer->start( autoSaveInterval() );
  }
}

void KMComposeWin::cleanupAutoSave()
{
  delete mAutoSaveTimer; mAutoSaveTimer = 0;
  if ( !mAutoSaveUUID.isEmpty() ) {

    kDebug() << "deleting autosave files" << mAutoSaveUUID;

    // Delete the autosave files
    QDir autoSaveDir( KMKernel::localDataPath() + "autosave" );

    // Filter out only this composer window's autosave files
    QStringList autoSaveFilter;
    autoSaveFilter << mAutoSaveUUID + "*";
    autoSaveDir.setNameFilters( autoSaveFilter );

    // Return the files to be removed
    QStringList autoSaveFiles = autoSaveDir.entryList();
    kDebug() << "There are" << autoSaveFiles.count() << "to be deleted.";

    // Delete each file
    foreach( const QString &file, autoSaveFiles ) {
      autoSaveDir.remove( file );
    }
    mAutoSaveUUID.clear();
  }
}

void KMComposeWin::slotCompletionModeChanged( KGlobalSettings::Completion mode )
{
  GlobalSettings::self()->setCompletionMode( (int) mode );

  // sync all the lineedits to the same completion mode
  mEdtFrom->setCompletionMode( mode );
  mEdtReplyTo->setCompletionMode( mode );
  mRecipientsEditor->setCompletionMode( mode );
}

void KMComposeWin::slotConfigChanged()
{
  readConfig( true /*reload*/);
  updateAutoSave();
  rethinkFields();
  slotWordWrapToggled( mWordWrapAction->isChecked() );
}

/*
 * checks if the drafts-folder has been deleted
 * that is not nice so we set the system-drafts-folder
 */
void KMComposeWin::slotFolderRemoved( const Akonadi::Collection & col )
{
  kDebug() << "you killed me.";
  // TODO: need to handle templates here?
  if ( ( mFolder.isValid() ) && ( col.id() == mFolder.id() ) ) {
    mFolder = kmkernel->draftsCollectionFolder();
    kDebug() << "restoring drafts to" << mFolder.id();
  }
}

void KMComposeWin::slotSetAlwaysSend( bool bAlways )
{
  mAlwaysSend = bAlways;
}

void KMComposeWin::slotFormatReset()
{
  mEditor->setTextForegroundColor( palette().text().color() );
  mEditor->setFont( mSaveFont );
}

void KMComposeWin::slotCursorPositionChanged()
{
  // Change Line/Column info in status bar
  int col, line;
  QString temp;
  line = mEditor->linePosition();
  col = mEditor->columnNumber();
  temp = i18nc("Shows the linenumber of the cursor position.", " Line: %1 ", line + 1 );
  statusBar()->changeItem( temp, 1 );
  temp = i18n( " Column: %1 ", col + 1 );
  statusBar()->changeItem( temp, 2 );

  // Show link target in status bar
  if ( mEditor->textCursor().charFormat().isAnchor() ) {
    QString text = mEditor->currentLinkText();
    QString url = mEditor->currentLinkUrl();
    statusBar()->changeItem( text + " -> " + url, 0 );
  }
  else {
    statusBar()->changeItem( QString(), 0 );
  }
}

namespace {
class KToggleActionResetter {
  KToggleAction *mAction;
  bool mOn;

  public:
    KToggleActionResetter( KToggleAction *action, bool on )
      : mAction( action ), mOn( on ) {}
    ~KToggleActionResetter() {
      if ( mAction ) {
        mAction->setChecked( mOn );
      }
    }
    void disable() { mAction = 0; }
};
}

void KMComposeWin::slotEncryptChiasmusToggled( bool on )
{
  mEncryptWithChiasmus = false;

  if ( !on ) {
    return;
  }

  KToggleActionResetter resetter( mEncryptChiasmusAction, false );

  const Kleo::CryptoBackend::Protocol *chiasmus =
    Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" );

  if ( !chiasmus ) {
    const QString msg = Kleo::CryptoBackendFactory::instance()->knowsAboutProtocol( "Chiasmus" ) ?
      i18n( "Please configure a Crypto Backend to use for "
            "Chiasmus encryption first.\n"
            "You can do this in the Crypto Backends tab of "
            "the configure dialog's Security page." ) :
      i18n( "It looks as though libkleopatra was compiled without "
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

  MessageViewer::ChiasmusKeySelector selectorDlg( this, i18n( "Chiasmus Encryption Key Selection" ),
                                   keys, GlobalSettings::chiasmusKey(),
                                   GlobalSettings::chiasmusOptions() );

  if ( selectorDlg.exec() != KDialog::Accepted ) {
    return;
  }

  GlobalSettings::setChiasmusOptions( selectorDlg.options() );
  GlobalSettings::setChiasmusKey( selectorDlg.key() );
  assert( !GlobalSettings::chiasmusKey().isEmpty() );
  mEncryptWithChiasmus = true;
  resetter.disable();
}

void KMComposeWin::recipientEditorSizeHintChanged()
{
  QTimer::singleShot( 1, this, SLOT(setMaximumHeaderSize()) );
}

void KMComposeWin::setMaximumHeaderSize()
{
  mHeadersArea->setMaximumHeight( mHeadersArea->sizeHint().height() );
}

void KMComposeWin::slotUpdateSignatureAndEncrypionStateIndicators()
{
  const bool showIndicatorsAlways = false; // FIXME config option?
  mSignatureStateIndicator->setText( mSignAction->isChecked() ?
                                     i18n("Message will be signed") :
                                     i18n("Message will not be signed") );
  mEncryptionStateIndicator->setText( mEncryptAction->isChecked() ?
                                      i18n("Message will be encrypted") :
                                      i18n("Message will not be encrypted") );
  if ( !showIndicatorsAlways ) {
    mSignatureStateIndicator->setVisible( mSignAction->isChecked() );
    mEncryptionStateIndicator->setVisible( mEncryptAction->isChecked() );
  }
}

void KMComposeWin::slotLanguageChanged( const QString &language )
{
  mDictionaryCombo->setCurrentByDictionary( language );
}

