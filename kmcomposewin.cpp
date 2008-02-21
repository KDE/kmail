/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#define REALLY_WANT_KMCOMPOSEWIN_H
#include "kmcomposewin.h"
#undef REALLY_WANT_KMCOMPOSEWIN_H

// KDEPIM includes
#include "kleo/cryptobackendfactory.h"
#include "kleo/exportjob.h"
#include "kleo/specialjob.h"
#include <libkpgp/kpgpblock.h>
#include <libkleo/ui/progressdialog.h>
#include <libkleo/ui/keyselectiondialog.h>

// LIBKDEPIM includes
#include <libkdepim/kaddrbookexternal.h>
#include <libkdepim/kmstylelistselectaction.h>
#include <libkdepim/recentaddresses.h>

using KPIM::RecentAddresses;

// KDEPIMLIBS includes
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identitycombo.h>
#include <kpimidentities/identity.h>
#include <kpimutils/kfileio.h>
#include <mailtransport/transportcombobox.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transport.h>

using MailTransport::TransportManager;
using MailTransport::Transport;

// KMail includes
#include "attachmentcollector.h"
#include "attachmentlistview.h"
#include "chiasmuskeyselector.h"
#include "dictionarycombobox.h"
#include "editorwatcher.h"
#include "kleo_util.h"
#include "kmatmlistview.h"
#include "kmcommands.h"
#include "kmcomposereditor.h"
#include "kmfoldercombobox.h"
#include "kmfolderimap.h"
#include "kmfoldermaildir.h"
#include "kmfoldermgr.h"
#include "kmmainwin.h"
#include "kmmsgpartdlg.h"
#include "kmreadermainwin.h"
#include "mailcomposeradaptor.h"
#include "messagecomposer.h"
#include "objecttreeparser.h"
#include "partNode.h"
#include "recipientseditor.h"
#include "replyphrases.h"
#include "stl_util.h"

using KMail::AttachmentListView;
using KPIM::DictionaryComboBox;

// KDELIBS includes
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kapplication.h>
#include <kcharsets.h>
#include <kcursorsaver.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kfontaction.h>
#include <kfontsizeaction.h>
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

#include <kio/jobuidelegate.h>
#include <kio/scheduler.h>
#include <sonnet/configdialog.h>

// Qt includes
#include <QBuffer>
#include <QClipboard>
#include <QEvent>
#include <QSplitter>

// System includes
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <memory>

// MOC
#include "kmcomposewin.moc"

#include "snippetwidget.h"

KMail::Composer *KMail::makeComposer( KMMessage *msg, uint identitiy ) {
  return KMComposeWin::create( msg, identitiy );
}

KMail::Composer *KMComposeWin::create( KMMessage *msg, uint identitiy ) {
  return new KMComposeWin( msg, identitiy );
}

int KMComposeWin::s_composerNumber = 0;

//-----------------------------------------------------------------------------
KMComposeWin::KMComposeWin( KMMessage *aMsg, uint id )
  : KMail::Composer( "kmail-composer#" ),
    mDone( false ),
    mAtmModified( false ),
    mMsg( 0 ),
    mAttachMenu( 0 ),
    mSigningAndEncryptionExplicitlyDisabled( false ),
    mFolder( 0 ),
    mUserUsesHtml( false ),
    mId( id ),
    mAttachPK( 0 ), mAttachMPK( 0 ),
    mAttachRemoveAction( 0 ), mAttachSaveAction( 0 ), mAttachPropertiesAction( 0 ),
    mSignAction( 0 ), mEncryptAction( 0 ), mRequestMDNAction( 0 ),
    mUrgentAction( 0 ), mAllFieldsAction( 0 ), mFromAction( 0 ),
    mReplyToAction( 0 ), mSubjectAction( 0 ),
    mIdentityAction( 0 ), mTransportAction( 0 ), mFccAction( 0 ),
    mWordWrapAction( 0 ), mFixedFontAction( 0 ), mAutoSpellCheckingAction( 0 ),
    mDictionaryAction( 0 ), mSnippetAction( 0 ),
    mEncodingAction( 0 ),
    mCryptoModuleAction( 0 ),
    mEncryptChiasmusAction( 0 ),
    mEncryptWithChiasmus( false ),
    mComposer( 0 ),
    mLabelWidth( 0 ),
    mAutoSaveTimer( 0 ), mLastAutoSaveErrno( 0 ),
    mSignatureStateIndicator( 0 ), mEncryptionStateIndicator( 0 )
{
  (void) new MailcomposerAdaptor( this );
  mdbusObjectPath = "/Composer_" + QString::number( ++s_composerNumber );
  QDBusConnection::sessionBus().registerObject( mdbusObjectPath , this );

  mSubjectTextWasSpellChecked = false;
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
  mFcc = new KMFolderComboBox( mHeadersArea );
  mFcc->showOutboxFolder( false );
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

  mEdtSubject = new KMLineEditSpell( false, mHeadersArea, "subjectLine" );
  mLblIdentity = new QLabel( i18n("&Identity:"), mHeadersArea );
  mDictionaryLabel = new QLabel( i18n("&Dictionary:"), mHeadersArea );
  mLblFcc = new QLabel( i18n("&Sent-Mail folder:"), mHeadersArea );
  mLblTransport = new QLabel( i18n("&Mail transport:"), mHeadersArea );
  mLblFrom = new QLabel( i18nc("sender address field", "&From:"), mHeadersArea );
  mLblReplyTo = new QLabel( i18n("&Reply to:"), mHeadersArea );
  mLblSubject = new QLabel( i18n("S&ubject:"), mHeadersArea );
  QString sticky = i18n("Sticky");
  mBtnIdentity = new QCheckBox( sticky, mHeadersArea );
  mBtnFcc = new QCheckBox( sticky, mHeadersArea );
  mBtnTransport = new QCheckBox( sticky, mHeadersArea );

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
    QPalette p( mSignatureStateIndicator->palette() );

    KColorScheme scheme( QPalette::Active, KColorScheme::View );
    QColor defaultSignedColor =  // pgp signed
        scheme.background( KColorScheme::PositiveBackground ).color();
    QColor defaultEncryptedColor( 0x00, 0x80, 0xFF ); // light blue // pgp encrypted
    p.setColor( QColorGroup::Window, reader.readEntry( "PGPMessageOkKeyOk",
                                                       defaultSignedColor ) );
    mSignatureStateIndicator->setPalette( p );
    mSignatureStateIndicator->setAutoFillBackground( true );

    mEncryptionStateIndicator = new QLabel( editorAndCryptoStateIndicators );
    mEncryptionStateIndicator->setAlignment( Qt::AlignHCenter );
    hbox->addWidget( mEncryptionStateIndicator );
    p.setColor( QColorGroup::Window, reader.readEntry( "PGPMessageEncr" ,
                                                       defaultEncryptedColor ) );
    mEncryptionStateIndicator->setPalette( p );
    mEncryptionStateIndicator->setAutoFillBackground( true );
  }

  mEditor = new KMComposerEditor( this, editorAndCryptoStateIndicators );
  vbox->addLayout( hbox );
  vbox->addWidget( mEditor );
  mSnippetSplitter->insertWidget( 0, editorAndCryptoStateIndicators );
  mSnippetSplitter->setOpaqueResize( true );

  mHeadersToEditorSplitter->addWidget( mSplitter );
  mEditor->setAcceptDrops( true );
  connect( mDictionaryCombo, SIGNAL( dictionaryChanged( const QString & ) ), mEditor,
           SLOT( slotDictionaryChanged( const QString & ) ) );
  connect( mEditor, SIGNAL( highlighterCreated() ), this, SLOT( slotHighlighterCreated() ) );
  connect( mEditor, SIGNAL( spellCheckStatus(const QString &)), this,
           SLOT( slotSpellCheckingStatus( const QString & ) ) );

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

  mAtmListView = new AttachmentListView( this, mSplitter );
  mSplitter->addWidget( mAtmListView );

  connect( mAtmListView,
           SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
           SLOT(slotAttachEdit()) );
  connect( mAtmListView,
           SIGNAL(rightButtonPressed(QTreeWidgetItem*)),
           SLOT(slotAttachPopupMenu(QTreeWidgetItem*)) );
  connect( mAtmListView,
           SIGNAL(itemSelectionChanged()),
           SLOT(slotUpdateAttachActions()) );
  connect( mAtmListView,
           SIGNAL(attachmentDeleted()),
           SLOT(slotAttachRemove()) );
  connect( mAtmListView,
           SIGNAL( dragStarted() ),
           SLOT( slotAttachmentDragStarted() ) );
  mAttachMenu = 0;

  readConfig();
  setupStatusBar();
  setupActions();
  setupEditor();
  rethinkFields();
  slotUpdateSignatureAndEncrypionStateIndicators();

  applyMainWindowSettings( KMKernel::config()->group( "Composer") );

  connect( mEdtSubject, SIGNAL( subjectTextSpellChecked() ),
           SLOT( slotSubjectTextSpellChecked() ) );
  connect( mEdtSubject, SIGNAL(textChanged(const QString&)),
           SLOT(slotUpdWinTitle(const QString&)) );
  connect( mIdentity, SIGNAL(identityChanged(uint)),
           SLOT(slotIdentityChanged(uint)) );
  connect( kmkernel->identityManager(), SIGNAL(changed(uint)),
           SLOT(slotIdentityChanged(uint)) );

  connect( mEdtFrom, SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
           SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)) );
  connect( kmkernel->folderMgr(), SIGNAL(folderRemoved(KMFolder*)),
           SLOT(slotFolderRemoved(KMFolder*)) );
  connect( kmkernel->imapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
           SLOT(slotFolderRemoved(KMFolder*)) );
  connect( kmkernel->dimapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)),
           SLOT(slotFolderRemoved(KMFolder*)) );
  connect( kmkernel, SIGNAL( configChanged() ),
           this, SLOT( slotConfigChanged() ) );

  connect( mEditor, SIGNAL( attachPNGImageData(const QByteArray &) ),
           this, SLOT( slotAttachPNGImageData(const QByteArray &) ) );
  connect( mEditor, SIGNAL( focusChanged(bool) ),
           this, SLOT(editorFocusChanged(bool)) );

  mMainWidget->resize( 480, 510 );
  setCentralWidget( mMainWidget );

  if ( GlobalSettings::self()->useExternalEditor() ) {
    mEditor->setUseExternalEditor( true );
    mEditor->setExternalEditorPath( GlobalSettings::self()->externalEditor() );
  }

  initAutoSave();

  mMsg = 0;
  if ( aMsg ) {
    setMsg( aMsg );
  }
  mRecipientsEditor->setFocus();
  fontChanged( mEditor->currentFont() ); // set toolbar buttons to correct values

  mDone = true;
}

//-----------------------------------------------------------------------------
KMComposeWin::~KMComposeWin()
{
  writeConfig();

  if ( mFolder && mMsg ) {
    mAutoDeleteMsg = false;
    mFolder->addMsg( mMsg );
    // Ensure that the message is correctly and fully parsed
    mFolder->unGetMsg( mFolder->count() - 1 );
  }

  if ( mAutoDeleteMsg ) {
    delete mMsg;
    mMsg = 0;
  }

  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.begin();
  while ( it != mMapAtmLoadData.end() ) {
    KIO::Job *job = it.key();
    mMapAtmLoadData.erase( it );
    job->kill();
    it = mMapAtmLoadData.begin();
  }
  deleteAll( mComposedMessages );

  qDeleteAll( mAtmList );
  qDeleteAll( mAtmTempList );

  foreach ( KTempDir *const dir, mTempDirs ) {
    delete dir;
  }
}


QString KMComposeWin::dbusObjectPath() const
{
  return mdbusObjectPath;
}

void KMComposeWin::slotHighlighterCreated()
{
  mEditor->slotDictionaryChanged( mDictionaryCombo->realDictionaryName() );
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
}

void KMComposeWin::slotAttachedFile( const KUrl &url )
{
  if ( mAttachFilesPending.isEmpty() ) {
    return;
  }
  mAttachFilesPending.removeAt( mAttachFilesPending.indexOf( url ) ); // only remove one copy of url
  if ( mAttachFilesPending.isEmpty() ) {
    send( mAttachFilesSend );
    mAttachFilesSend = -1;
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttachment( const KUrl &url, const QString &comment )
{
  Q_UNUSED( comment );
  addAttach( url );
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
    addAttach( msgPart );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachPNGImageData( const QByteArray &image )
{
  bool ok;

  QString attName =
    KInputDialog::getText( "KMail", i18n("Name of the attachment:"), QString(), &ok, this );
  if ( !ok ) {
    return;
  }

  addAttachment( attName, "base64", image, "image", "png", QByteArray(),
                 QString(), QByteArray() );
}

//-----------------------------------------------------------------------------
void KMComposeWin::setBody( const QString &body )
{
  mEditor->setPlainText( body );
}

//-----------------------------------------------------------------------------
bool KMComposeWin::event( QEvent *e )
{
  if ( e->type() == QEvent::ApplicationPaletteChange ) {
    readColorConfig();
  }
  return KMail::Composer::event( e );
}

//-----------------------------------------------------------------------------
void KMComposeWin::readColorConfig( void )
{
  if ( GlobalSettings::self()->useDefaultColors() ) {
    mForeColor = QColor( qApp->palette().color( QPalette::Text ) );
    mBackColor = QColor( qApp->palette().color( QPalette::Base ) );
  } else {
    mForeColor = GlobalSettings::self()->foregroundColor();
    mBackColor = GlobalSettings::self()->backgroundColor();
  }

  // Color setup
  mPalette = qApp->palette();
  mPalette.setColor( QPalette::Base, mBackColor );
  mPalette.setColor( QPalette::Text, mForeColor );
#ifdef __GNUC__
# warning "FIXME: Do we need to call setDisabled/setActive/setInactive or are the setColor calls enough??"
#endif
  //   mPalette.setDisabled(cgrp);
  //   mPalette.setActive(cgrp);
  //   mPalette.setInactive(cgrp);

  mEdtFrom->setPalette( mPalette );
  mEdtReplyTo->setPalette( mPalette );
  mEdtSubject->setPalette( mPalette );
  mTransport->setPalette( mPalette );
  mEditor->setPalette( mPalette );
  mFcc->setPalette( mPalette );
}

//-----------------------------------------------------------------------------
void KMComposeWin::readConfig( void )
{
  mDefCharset = KMMessage::defaultCharset();
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

  readColorConfig();

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


  mIdentity->setCurrentIdentity( mId );

  kDebug(5006) <<"KMComposeWin::readConfig." << mIdentity->currentIdentityName();
  const KPIMIdentities::Identity & ident =
    kmkernel->identityManager()->identityForUoid( mIdentity->currentIdentity() );

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

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
  GlobalSettings::self()->setStickyTransport( mBtnTransport->isChecked() );
  GlobalSettings::self()->setStickyIdentity( mBtnIdentity->isChecked() );
  GlobalSettings::self()->setStickyFcc( mBtnFcc->isChecked() );
  GlobalSettings::self()->setPreviousIdentity( mIdentity->currentIdentity() );
  GlobalSettings::self()->setCurrentTransport( mTransport->currentText() );
  GlobalSettings::self()->setPreviousFcc( mFcc->getFolder()->idString() );
  GlobalSettings::self()->setAutoSpellChecking(
                                               mAutoSpellCheckingAction->isChecked() );
  GlobalSettings::self()->setUseFixedFont( mFixedFontAction->isChecked() );
  GlobalSettings::self()->setUseHtmlMarkup( mHtmlMarkup );
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
  kDebug(5006) ;
  if ( !mMsg || mComposer || mAutoSaveFilename.isEmpty() ) {
    return;
  }
  kDebug(5006) <<"autosaving message";

  if ( mAutoSaveTimer ) {
    mAutoSaveTimer->stop();
  }
  connect( this, SIGNAL( applyChangesDone( bool ) ),
           this, SLOT( slotContinueAutoSave() ) );
  // This method is called when KMail crashed, so don't try signing/encryption
  // and don't disable controls because it is also called from a timer and
  // then the disabling is distracting.
  applyChanges( true, true );

  // Don't continue before the applyChanges is done!
}

void KMComposeWin::slotContinueAutoSave()
{
  // Ok, it's done now - continue dead letter saving
  if ( mComposedMessages.isEmpty() ) {
    kDebug(5006) <<"Composing the message failed.";
    return;
  }
  KMMessage *msg = mComposedMessages.first();
  if ( !msg ) // a bit of extra defensiveness
    return;

  kDebug(5006) <<"opening autoSaveFile" << mAutoSaveFilename;
  const QString filename =
    KMKernel::localDataPath() + "autosave/cur/" + mAutoSaveFilename;
  KSaveFile autoSaveFile( filename );
  int status = 0;
  bool opened = autoSaveFile.open();
  kDebug(5006) <<"autoSaveFile.open() =" << opened;
  if ( opened ) { // no error
    autoSaveFile.setPermissions( QFile::ReadUser|QFile::WriteUser );
    kDebug(5006) <<"autosaving message in" << filename;
    int fd = autoSaveFile.handle();
    const DwString &msgStr = msg->asDwString();
    if ( ::write( fd, msgStr.data(), msgStr.length() ) == -1 ) {
      status = errno;
    }
  }
  if ( status == 0 ) {
    kDebug(5006) <<"closing autoSaveFile";
    autoSaveFile.finalize();
    mLastAutoSaveErrno = 0;
  } else {
    kDebug(5006) <<"autosaving failed";
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

  if ( autoSaveInterval() > 0 ) {
    updateAutoSave();
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
    kDebug(5006) <<"Something is wrong (Oh, yeah?)";
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
  kDebug(5006);

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


  if ( !mAtmList.isEmpty() ) {
    mAtmListView->show();
  } else {
    mAtmListView->hide();
  }
  resize( this->size() );
  repaint();

  mHeadersArea->setMaximumHeight( mHeadersArea->sizeHint().height() );
  mGrid->activate();
  mHeadersArea->show();

  slotUpdateAttachActions();
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
                                      QLabel *aLbl, QLineEdit *aEdt,
                                      QPushButton *aBtn )
{
  if ( aValue & aMask ) {
    aLbl->setFixedWidth( mLabelWidth );
    aLbl->setBuddy( aEdt );
    mGrid->addWidget( aLbl, aRow, 0 );
    QPalette pal;
    pal.setColor( aEdt->backgroundRole(), mBackColor );
    aEdt->setPalette( pal );
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
                                      QLabel *aLbl, QComboBox *aCbx,
                                      QCheckBox *aChk )
{
  if ( aValue & aMask ) {
    aLbl->adjustSize();
    aLbl->resize( (int)aLbl->sizeHint().width(), aLbl->sizeHint().height() + 6 );
    aLbl->setMinimumSize( aLbl->size() );
    aLbl->show();
    aLbl->setBuddy( aCbx );
    mGrid->addWidget( aLbl, aRow, 0 );
    aCbx->show();
    aCbx->setMinimumSize( 100, aLbl->height() + 2 );

    mGrid->addWidget( aCbx, aRow, 1 );
    if ( aChk ) {
      mGrid->addWidget( aChk, aRow, 2 );
      aChk->setFixedSize( aChk->sizeHint().width(), aLbl->height() );
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
void KMComposeWin::getTransportMenu()
{
  QStringList availTransports;

  mActNowMenu->clear();
  mActLaterMenu->clear();
  availTransports = TransportManager::self()->transportNames();
  QStringList::Iterator it;
  for ( it = availTransports.begin(); it != availTransports.end() ; ++it ) {
    QAction *action1 = new QAction( (*it).replace( "&", "&&" ), mActNowMenu );
    QAction *action2 = new QAction( (*it).replace( "&", "&&" ), mActLaterMenu );
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
    QAction *action = new KAction(KIcon("mail-send"), i18n("&Send Mail"), this);
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
    actActionLaterMenu->setIconText( i18n( "Queue" ) );
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

  connect( actActionNowMenu, SIGNAL(activated()), this,
           SLOT(slotSendNow()) );
  connect( actActionLaterMenu, SIGNAL(activated()), this,
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

  QAction *action = new KAction( KIcon( "document-save" ), i18n("Save as &Draft"), this );
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
  action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_L ) );
  action = new KAction( i18n("Save &Distribution List...") , this );
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
  KStandardAction::selectAll( this, SLOT(slotMarkAll()), actionCollection() );

  KStandardAction::find( mEditor, SLOT(slotFind()), actionCollection() );
  KStandardAction::findNext( mEditor, SLOT(slotFindNext()), actionCollection() );

  KStandardAction::replace( mEditor, SLOT(slotReplace()), actionCollection() );
  actionCollection()->addAction( KStandardAction::Spelling , "spellcheck", mEditor, SLOT(checkSpelling()) );

  mPasteQuotation = new KAction( i18n("Pa&ste as Quotation"), this );
  actionCollection()->addAction("paste_quoted", mPasteQuotation );
  connect( mPasteQuotation, SIGNAL(triggered(bool) ), mEditor, SLOT( slotPasteAsQuotation()) );

  action = new KAction( i18n("Paste as Attac&hment"), this );
  actionCollection()->addAction( "paste_att", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT( slotPasteAsAttachment()) );

  mAddQuoteChars = new KAction( i18n("Add &Quote Characters"), this );
  actionCollection()->addAction( "tools_quote", mAddQuoteChars );
  connect( mAddQuoteChars, SIGNAL(triggered(bool) ), mEditor, SLOT(slotAddQuotes()) );

  mRemQuoteChars = new KAction( i18n("Re&move Quote Characters"), this );
  actionCollection()->addAction( "tools_unquote", mRemQuoteChars );
  connect (mRemQuoteChars, SIGNAL(triggered(bool) ),mEditor, SLOT(slotRemoveQuotes()) );

  mCleanSpace = new KAction( i18n("Cl&ean Spaces"), this );
  actionCollection()->addAction( "clean_spaces", mCleanSpace );
  connect( mCleanSpace, SIGNAL(triggered(bool) ), SLOT(slotCleanSpace()) );

  mFixedFontAction = new KToggleAction( i18n("Use Fi&xed Font"), this );
  actionCollection()->addAction( "toggle_fixedfont", mFixedFontAction );
  connect( mFixedFontAction, SIGNAL(triggered(bool) ), SLOT(slotUpdateFont()) );
  mFixedFontAction->setChecked( GlobalSettings::self()->useFixedFont() );

  //these are checkable!!!
  mUrgentAction = new KToggleAction( i18n("&Urgent"), this );
  actionCollection()->addAction( "urgent", mUrgentAction );
  mRequestMDNAction = new KToggleAction( i18n("&Request Disposition Notification"), this );
  actionCollection()->addAction("options_request_mdn", mRequestMDNAction );
  mRequestMDNAction->setChecked(GlobalSettings::self()->requestMDN());
  //----- Message-Encoding Submenu
  mEncodingAction = new KSelectAction( KIcon( "accessories-character-map" ), i18n("Se&t Encoding"), this );
  actionCollection()->addAction( "charsets", mEncodingAction );
  connect( mEncodingAction, SIGNAL(triggered(bool)), SLOT(slotSetCharset()) );
  mWordWrapAction = new KToggleAction( i18n( "&Wordwrap" ), this );
  actionCollection()->addAction( "wordwrap", mWordWrapAction );
  mWordWrapAction->setChecked( GlobalSettings::self()->wordWrap() );
  connect( mWordWrapAction, SIGNAL(toggled(bool)), SLOT(slotWordWrapToggled(bool)) );

  mSnippetAction = new KToggleAction( i18n("&Snippets"), this );
  actionCollection()->addAction( "snippets", mSnippetAction );
  connect( mSnippetAction, SIGNAL( toggled(bool) ),
           mSnippetWidget, SLOT( setShown(bool) ) );
  mSnippetAction->setChecked( GlobalSettings::self()->showSnippetManager() );

  mAutoSpellCheckingAction = new KToggleAction( KIcon( "tools-check-spelling" ), i18n("&Automatic Spellchecking"), this );
  actionCollection()->addAction( "options_auto_spellchecking", mAutoSpellCheckingAction );
  const bool spellChecking = GlobalSettings::self()->autoSpellChecking();
  mAutoSpellCheckingAction->setEnabled( !GlobalSettings::self()->useExternalEditor() );
  mAutoSpellCheckingAction->setChecked( !GlobalSettings::self()->useExternalEditor() && spellChecking );
  slotAutoSpellCheckingToggled( !GlobalSettings::self()->useExternalEditor() && spellChecking );
  connect( mAutoSpellCheckingAction, SIGNAL( toggled( bool ) ),
           this, SLOT( slotAutoSpellCheckingToggled( bool ) ) );
  connect( mEditor, SIGNAL(checkSpellingChanged(bool)), this, SLOT(slotUpdateCheckSpellChecking(bool)));

  QStringList encodings = KMMsgBase::supportedEncodings( true );
  encodings.prepend( i18n("Auto-Detect") );
  mEncodingAction->setItems( encodings );
  mEncodingAction->setCurrentItem( -1 );

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
  mSubjectAction = new KToggleAction(i18n("S&ubject"), this);
  actionCollection()->addAction("show_subject", mSubjectAction );
  connect(mSubjectAction, SIGNAL(triggered(bool) ), SLOT(slotView()));
  //end of checkable

  action = new KAction( i18n("Append S&ignature"), this );
  actionCollection()->addAction( "append_signature", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT(slotAppendSignature()));
  mAttachPK = new KAction(i18n("Attach &Public Key..."), this);
  actionCollection()->addAction("attach_public_key", mAttachPK );
  connect( mAttachPK, SIGNAL(triggered(bool) ), SLOT(slotInsertPublicKey()));
  mAttachMPK = new KAction(i18n("Attach &My Public Key"), this);
  actionCollection()->addAction("attach_my_public_key", mAttachMPK );
  connect( mAttachMPK, SIGNAL(triggered(bool) ), SLOT(slotInsertMyPublicKey()));
  action = new KAction(KIcon("mail-attachment"), i18n("&Attach File..."), this);
  action->setIconText( i18n( "Attach" ) );
  actionCollection()->addAction("attach", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT(slotAttachFile()));
  mAttachRemoveAction = new KAction(i18n("&Remove Attachment"), this);
  actionCollection()->addAction("remove", mAttachRemoveAction );
  connect( mAttachRemoveAction, SIGNAL(triggered(bool) ), SLOT(slotAttachRemove()));
  mAttachSaveAction = new KAction(KIcon("document-save"), i18n("&Save Attachment As..."), this);
  actionCollection()->addAction("attach_save", mAttachSaveAction );
  connect( mAttachSaveAction, SIGNAL(triggered(bool) ), SLOT(slotAttachSave()));
  mAttachPropertiesAction = new KAction(i18n("Attachment Pr&operties"), this);
  actionCollection()->addAction("attach_properties", mAttachPropertiesAction );
  connect( mAttachPropertiesAction, SIGNAL(triggered(bool) ), SLOT(slotAttachProperties()));

  setStandardToolBarMenuEnabled( true );

  KStandardAction::keyBindings( this, SLOT(slotEditKeys()), actionCollection());
  KStandardAction::configureToolbars( this, SLOT(slotEditToolbars()), actionCollection());
  KStandardAction::preferences( kmkernel, SLOT(slotShowConfigurationDialog()), actionCollection() );

  action = new KAction( i18n("&Spellchecker..."), this );
  action->setIconText( i18n("Spellchecker") );
  actionCollection()->addAction( "setup_spellchecker", action );
  connect( action, SIGNAL(triggered(bool) ), SLOT(slotSpellcheckConfig()) );

  if ( Kleo::CryptoBackendFactory::instance()->protocol( "Chiasmus" ) ) {
    KToggleAction *a = new KToggleAction( KIcon( "chidecrypted" ), i18n("Encrypt Message with Chiasmus..."), this );
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
  // get PGP user id for the chosen identity
  const KPIMIdentities::Identity & ident =
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


  listAction = new KMStyleListSelectAction(i18n("Select Style"), this);
  actionCollection()->addAction("text_list", listAction );

  connect( listAction, SIGNAL( applyStyle(QTextListFormat::Style) ),
           SLOT( slotChangeParagStyle(QTextListFormat::Style) ) );

  fontAction = new KFontAction(i18n("Select Font"), this);
  actionCollection()->addAction("text_font", fontAction );
  connect( fontAction, SIGNAL( triggered( const QString& ) ),
           SLOT( slotFontAction( const QString& ) ) );
  fontSizeAction = new KFontSizeAction(i18n("Select Size"), this);
  actionCollection()->addAction("text_size", fontSizeAction );
  connect( fontSizeAction, SIGNAL( fontSizeChanged( int ) ),
           SLOT( slotSizeAction( int ) ) );

  alignLeftAction = new KToggleAction( KIcon( "format-justify-left" ), i18n("Align Left"), this );
  alignLeftAction->setIconText( i18n("Left") );
  actionCollection()->addAction( "align_left", alignLeftAction );
  connect( alignLeftAction, SIGNAL( triggered(bool) ), SLOT( slotAlignLeft() ) );
  alignLeftAction->setChecked( true );
  alignRightAction = new KToggleAction( KIcon( "format-justify-right" ), i18n("Align Right"), this );
  alignRightAction->setIconText( i18n("Right") );
  actionCollection()->addAction( "align_right", alignRightAction );
  connect( alignRightAction, SIGNAL( triggered(bool) ), SLOT( slotAlignRight() ) );
  alignCenterAction = new KToggleAction( KIcon( "format-justify-center" ), i18n("Align Center"), this );
  alignCenterAction->setIconText( i18n("Center") );
  actionCollection()->addAction( "align_center", alignCenterAction );
  connect( alignCenterAction, SIGNAL( triggered(bool) ), SLOT( slotAlignCenter() ) );
  textBoldAction = new KToggleAction( KIcon( "format-text-bold" ), i18n("&Bold"), this );
  actionCollection()->addAction( "text_bold", textBoldAction );
  connect( textBoldAction, SIGNAL( triggered(bool) ), SLOT( slotTextBold(bool) ) );
  textBoldAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_B ) );
  textItalicAction = new KToggleAction( KIcon( "format-text-italic" ), i18n("&Italic"), this );
  actionCollection()->addAction( "text_italic", textItalicAction );
  connect( textItalicAction, SIGNAL( triggered(bool) ), SLOT( slotTextItalic(bool) ) );
  textItalicAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_I ) );
  textUnderAction = new KToggleAction( KIcon( "format-text-underline" ), i18n("&Underline"), this );
  actionCollection()->addAction( "text_under", textUnderAction );
  connect( textUnderAction, SIGNAL( triggered(bool) ), SLOT( slotTextUnder(bool) ) );
  textUnderAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_U ) );
  actionFormatReset = new KAction( KIcon( "draw-eraser" ), i18n("Reset Font Settings"), this );
  actionFormatReset->setIconText( i18n("Reset Font") );
  actionCollection()->addAction( "format_reset", actionFormatReset );
  connect( actionFormatReset, SIGNAL(triggered(bool) ), SLOT( slotFormatReset() ) );
  actionFormatColor = new KAction( KIcon( "format-stroke-color" ), i18n("Text Color..."), this );
  actionFormatColor->setIconText( i18n("Text Color") );
  actionCollection()->addAction("format_color", actionFormatColor );
  connect( actionFormatColor, SIGNAL(triggered(bool) ),mEditor, SLOT( slotTextColor() ));

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

void KMComposeWin::slotUpdateCheckSpellChecking(bool _b)
{
   mAutoSpellCheckingAction->setChecked(_b);
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupStatusBar( void )
{
  statusBar()->insertItem( "", 0, 1 );
  statusBar()->setItemAlignment( 0, Qt::AlignLeft | Qt::AlignVCenter );

  statusBar()->insertPermanentItem( i18n(" Spellcheck: %1 ", QString( "     " )), 3, 0) ;
  statusBar()->insertPermanentItem( i18n(" Column: %1 ", QString( "     " ) ), 2, 0 );
  statusBar()->insertPermanentItem( i18n(" Line: %1 ", QString( "     " ) ), 1, 0 );
}

//-----------------------------------------------------------------------------
void KMComposeWin::updateCursorPosition()
{
  int col, line;
  QString temp;
  line = mEditor->linePosition();
  col = mEditor->columnNumber ();
  temp = i18n(" Line: %1 ", line+1);
  statusBar()->changeItem( temp, 1 );
  temp = i18n(" Column: %1 ", col + 1 );
  statusBar()->changeItem( temp, 2 );
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

  updateCursorPosition();
  connect( mEditor, SIGNAL(cursorPositionChanged()), SLOT(updateCursorPosition()) );
  connect( mEditor, SIGNAL( currentFontChanged( const QFont & ) ),
           this, SLOT( fontChanged( const QFont & ) ) );
  connect( mEditor, SIGNAL( cursorPositionChanged() ),
           this, SLOT( slotCursorPositionChanged() ) );
}

//-----------------------------------------------------------------------------
static QString cleanedUpHeaderString( const QString &s )
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
void KMComposeWin::verifyWordWrapLengthIsAdequate( const QString &body )
{
  int maxLineLength = 0;
  int curPos;
  int oldPos = 0;
  if ( mEditor->lineWrapMode () == QTextEdit::FixedColumnWidth ) {
    for ( curPos = 0; curPos < (int)body.length(); ++curPos ) {
      if ( body[curPos] == '\n' ) {
        if ( (curPos - oldPos ) > maxLineLength ) {
          maxLineLength = curPos - oldPos;
        }
        oldPos = curPos;
      }
    }
    if ( ( curPos - oldPos ) > maxLineLength ) {
      maxLineLength = curPos - oldPos;
    }
    if ( mEditor->lineWrapColumnOrWidth() < maxLineLength ) {
      mEditor->setLineWrapColumnOrWidth( maxLineLength );
    }
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

//-----------------------------------------------------------------------------
void KMComposeWin::setMsg( KMMessage *newMsg, bool mayAutoSign,
                           bool allowDecryption, bool isModified )
{
  if ( !newMsg ) {
    kDebug(5006) <<"KMComposeWin::setMsg() : newMsg == 0!";
    return;
  }
  mMsg = newMsg;
  KPIMIdentities::IdentityManager * im = kmkernel->identityManager();

  mEdtFrom->setText( mMsg->from() );
  mEdtReplyTo->setText( mMsg->replyTo() );
  mRecipientsEditor->setRecipientString( mMsg->to(), Recipient::To );
  mRecipientsEditor->setRecipientString( mMsg->cc(), Recipient::Cc );
  mRecipientsEditor->setRecipientString( mMsg->bcc(), Recipient::Bcc );
  mRecipientsEditor->setFocusBottom();
  mEdtSubject->setText( mMsg->subject() );

  const bool stickyIdentity = mBtnIdentity->isChecked();
  const bool messageHasIdentity = !newMsg->headerField("X-KMail-Identity").isEmpty();
  if (!stickyIdentity && messageHasIdentity)
    mId = newMsg->headerField("X-KMail-Identity").simplified().toUInt();

  // don't overwrite the header values with identity specific values
  // unless the identity is sticky
  if ( !stickyIdentity ) {
    disconnect( mIdentity,SIGNAL(identityChanged(uint)),
                this, SLOT(slotIdentityChanged(uint)) ) ;
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
      mId = newMsg->headerField("X-KMail-Identity").simplified().toUInt();
    } else {
      mId = im->defaultIdentity().uoid();
    }
  }
  // manually load the identity's value into the fields; either the one from the
  // messge, where appropriate, or the one from the sticky identity. What's in
  // mId might have changed meanwhile, thus the save value
  slotIdentityChanged( idToApply );

  const KPIMIdentities::Identity &ident = im->identityForUoid( mIdentity->currentIdentity() );

  // check for the presence of a DNT header, indicating that MDN's were requested
  QString mdnAddr = newMsg->headerField( "Disposition-Notification-To" );
  mRequestMDNAction->setChecked( ( !mdnAddr.isEmpty() &&
                                   im->thatIsMe( mdnAddr ) ) ||
                                 GlobalSettings::self()->requestMDN() );

  // check for presence of a priority header, indicating urgent mail:
  mUrgentAction->setChecked( newMsg->isUrgent() );

  if ( !ident.isXFaceEnabled() || ident.xface().isEmpty() ) {
    mMsg->removeHeaderField( "X-Face" );
  } else {
    QString xface = ident.xface();
    if ( !xface.isEmpty() ) {
      int numNL = ( xface.length() - 1 ) / 70;
      for ( int i = numNL; i > 0; --i ) {
        xface.insert( i * 70, "\n\t" );
      }
      mMsg->setHeaderField( "X-Face", xface );
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
  if ( mMsg->headers().FindField( "X-KMail-CryptoMessageFormat" ) )
    mCryptoModuleAction->setCurrentItem( format2cb( static_cast<Kleo::CryptoMessageFormat>(
                    mMsg->headerField( "X-KMail-CryptoMessageFormat" ).toInt() ) ) );

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

  // "Attach my public key" is only possible if the user uses OpenPGP
  // support and he specified his key:
  mAttachMPK->setEnabled( Kleo::CryptoBackendFactory::instance()->openpgp() &&
                          !ident.pgpEncryptionKey().isEmpty() );

  QString transportName = newMsg->headerField("X-KMail-Transport");
  if ( !mBtnTransport->isChecked() && !transportName.isEmpty() ) {
    Transport *transport =
        TransportManager::self()->transportByName( transportName );
    if ( transport )
      mTransport->setCurrentTransport( transport->id() );
  }

  if ( !mBtnFcc->isChecked() ) {
    if ( !mMsg->fcc().isEmpty() ) {
      setFcc( mMsg->fcc() );
    } else {
      setFcc( ident.fcc() );
    }
  }

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  partNode *root = partNode::fromMessage( mMsg );

  KMail::ObjectTreeParser otp; // all defaults are ok
  otp.parseObjectTree( root );

  KMail::AttachmentCollector ac;
  ac.setDiveIntoEncryptions( true );
  ac.setDiveIntoSignatures( true );
  ac.setDiveIntoMessages( false );

  ac.collectAttachmentsFrom( root );

  for ( std::vector<partNode*>::const_iterator it = ac.attachments().begin();
        it != ac.attachments().end() ; ++it ) {
    addAttach( new KMMessagePart( (*it)->msgPart() ) );
  }

  mEditor->setText( otp.textualContent() );
  mCharset = otp.textualContentCharset();

  if ( partNode * n = root->findType( DwMime::kTypeText, DwMime::kSubtypeHtml ) ) {
    if ( partNode * p = n->parentNode() ) {
      if ( p->hasType( DwMime::kTypeMultipart ) &&
           p->hasSubType( DwMime::kSubtypeAlternative ) ) {
        if ( mMsg->headerField( "X-KMail-Markup" ) == "true" ) {
          toggleMarkup( true );

          // get cte decoded body part
          mCharset = n->msgPart().charset();
          QByteArray bodyDecoded = n->msgPart().bodyDecoded();

          // respect html part charset
          const QTextCodec *codec = KMMsgBase::codecForName( mCharset );
          if ( codec ) {
            mEditor->setHtml( codec->toUnicode( bodyDecoded ) );
          } else {
            mEditor->setHtml( QString::fromLocal8Bit( bodyDecoded ) );
          }
        }
      }
    }
  }

  if ( mCharset.isEmpty() ) {
    mCharset = mMsg->charset();
  }
  if ( mCharset.isEmpty() ) {
    mCharset = mDefCharset;
  }
  setCharset( mCharset );

  /* Handle the special case of non-mime mails */
  if ( mMsg->numBodyParts() == 0 && otp.textualContent().isEmpty() ) {
    mCharset=mMsg->charset();
    if ( mCharset.isEmpty() ||  mCharset == "default" ) {
      mCharset = mDefCharset;
    }

    QByteArray bodyDecoded = mMsg->bodyDecoded();

    if ( allowDecryption ) {
      decryptOrStripOffCleartextSignature( bodyDecoded );
    }

    const QTextCodec *codec = KMMsgBase::codecForName( mCharset );
    if ( codec ) {
      mEditor->setText( codec->toUnicode( bodyDecoded ) );
    } else {
      mEditor->setText( QString::fromLocal8Bit( bodyDecoded ) );
    }
  }

#ifdef BROKEN_FOR_OPAQUE_SIGNED_OR_ENCRYPTED_MAILS
  const int num = mMsg->numBodyParts();
  kDebug(5006) <<"KMComposeWin::setMsg() mMsg->numBodyParts="
               << mMsg->numBodyParts();

  if ( num > 0 ) {
    KMMessagePart bodyPart;
    int firstAttachment = 0;

    mMsg->bodyPart( 1, &bodyPart );
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
        kDebug(5006) <<"KMComposeWin::setMsg() : text/html found";
        firstAttachment = 2;
        if ( mMsg->headerField( "X-KMail-Markup" ) == "true" ) {
          toggleMarkup( true );
        }
      }
      delete root;
      root = 0;
    }
    if ( firstAttachment == 0 ) {
      mMsg->bodyPart( 0, &bodyPart );
      if ( bodyPart.typeStr().toLower() == "text" ) {
        // we have a mp/mx body with a text body
        kDebug(5006) <<"KMComposeWin::setMsg() : text/* found";
        firstAttachment = 1;
      }
    }

    if ( firstAttachment != 0 )  {
      mCharset = bodyPart.charset();
      if ( mCharset.isEmpty() || mCharset == "default" )
        mCharset = mDefCharset;

      QByteArray bodyDecoded = bodyPart.bodyDecoded();

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
    for( int i = firstAttachment; i < num; ++i ) {
      KMMessagePart *msgPart = new KMMessagePart;
      mMsg->bodyPart(i, msgPart);
      QByteArray mimeType = msgPart->typeStr().toLower() + '/'
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

    QByteArray bodyDecoded = mMsg->bodyDecoded();

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

  // Make sure the cursor is at the correct position, which is set by
  // the template parser.
  if ( mMsg->getCursorPos() > 0 )
    mEditor->setCursorPositionFromStart( mMsg->getCursorPos() );

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
  return ( mEditor->document()->isModified() ||
           mEdtFrom->isModified() ||
           ( mEdtReplyTo && mEdtReplyTo->isModified() ) ||
           mRecipientsEditor->isModified() ||
           mEdtSubject->isModified() ||
           mAtmModified );
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
    mAtmModified =  false ;
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

  if ( mComposer && mComposer->isPerformingSignOperation() ) {
    // since the non-gpg-agent gpg plugin gets a passphrase using
    // QDialog::exec() the user can try to close the window,
    // which destroys mComposer mid-call.
    return false;
  }

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
                                                    KGuiItem(savebut, "document-save", QString(), savetext),
                                                    KStandardGuiItem::discard() );
    if ( rc == KMessageBox::Cancel )
      return false;
    else if ( rc == KMessageBox::Yes ) {
      // doSend will close the window. Just return false from this method
      if (istemplate) slotSaveTemplate();
      else slotSaveDraft();
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

  if ( !checkForForgottenAttachments || ( mAtmList.count() > 0 ) ) {
    return false;
  }

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
  rx.setCaseSensitivity( Qt::CaseInsensitive );

  bool gotMatch = false;

  // check whether the subject contains one of the attachment key words
  // unless the message is a reply or a forwarded message
  QString subj = subject();
  gotMatch = ( KMMessage::stripOffPrefixes( subj ) == subj )
    && ( rx.indexIn( subj ) >= 0 );

  if ( !gotMatch ) {
    // check whether the non-quoted text contains one of the attachment key
    // words
    QRegExp quotationRx ("^([ \\t]*([|>:}#]|[A-Za-z]+>))+");
    QTextDocument *doc = mEditor->document();
    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next()) {
      QString line = mEditor->text();
      gotMatch =    ( quotationRx.indexIn( line ) < 0 )
        && ( rx.indexIn( line ) >= 0 );
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
    slotAttachFile();
    //preceed with editing
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void KMComposeWin::applyChanges( bool dontSignNorEncrypt, bool dontDisable )
{
  kDebug(5006) <<"entering KMComposeWin::applyChanges";

  if(!mMsg) {
    kDebug(5006) <<"KMComposeWin::applyChanges() : mMsg == 0!";
    emit applyChangesDone( false );
    return;
  }

  if( mComposer ) {
    kDebug(5006) <<"KMComposeWin::applyChanges() : applyChanges called twice";
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

const KPIMIdentities::Identity & KMComposeWin::identity() const
{
  return kmkernel->identityManager()->identityForUoidOrDefault( mIdentity->currentIdentity() );
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
bool KMComposeWin::addAttach( const KUrl &aUrl )
{
  if ( !aUrl.isValid() ) {
    KMessageBox::sorry( this, i18n( "<qt><p>KMail could not recognize the location of the attachment (%1);</p>"
                                    "<p>you have to specify the full path if you wish to attach a file.</p></qt>" ,
                                    aUrl.prettyUrl() ) );
    return false;
  }

  const int maxAttachmentSize = GlobalSettings::maximumAttachmentSize();
  const uint maximumAttachmentSizeInBytes = maxAttachmentSize*1024*1024;
  if ( aUrl.isLocalFile() && QFileInfo( aUrl.pathOrUrl() ).size() > maximumAttachmentSizeInBytes ) {
    KMessageBox::sorry( this, i18n( "<qt><p>Your administrator has disallowed attaching files bigger than %1 MB.</p>", maxAttachmentSize ) );
    return false;
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
  mAttachJobs[job] = aUrl;
  connect(job, SIGNAL(result(KJob *)),
          this, SLOT(slotAttachFileResult(KJob *)));
  connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
          this, SLOT(slotAttachFileData(KIO::Job *, const QByteArray &)));
  return true;
}

//-----------------------------------------------------------------------------
void KMComposeWin::addAttach( KMMessagePart *msgPart )
{
  mAtmList.append( (KMMessagePart*) msgPart );

  // show the attachment listbox if it does not up to now
  if ( mAtmList.count() == 1 ) {
    mAtmListView->resize(mAtmListView->width(), 50);
    mAtmListView->show();
    resize(size());
  }

  // add a line in the attachment listbox
  KMAtmListViewItem *lvi = new KMAtmListViewItem( mAtmListView, msgPart );
  msgPartToItem( msgPart, lvi );
  mAtmItemList.append( lvi );

  connect( lvi, SIGNAL( compress( KMAtmListViewItem* ) ),
           this, SLOT( compressAttach( KMAtmListViewItem* ) ) );
  connect( lvi, SIGNAL( uncompress( KMAtmListViewItem* ) ),
           this, SLOT( uncompressAttach( KMAtmListViewItem* ) ) );

  slotUpdateAttachActions();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdateAttachActions()
{
  int selectedCount = 0;
  QList<KMAtmListViewItem*>::const_iterator it = mAtmItemList.constBegin();
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

QString KMComposeWin::prettyMimeType( const QString &type )
{
  const QString t = type.toLower();
  const KMimeType::Ptr st = KMimeType::mimeType( t );

  if ( !st ) {
    kWarning(5006) <<"unknown mimetype" << t;
    return QString();
  }

  return !st->isDefault() ? st->comment() : t;
}

void KMComposeWin::msgPartToItem( const KMMessagePart *msgPart,
                                  KMAtmListViewItem *lvi, bool loadDefaults )
{
  assert( msgPart != 0 );

  if ( !msgPart->fileName().isEmpty() ) {
    lvi->setText( 0, msgPart->fileName() );
  } else {
    lvi->setText( 0, msgPart->name() );
  }
  lvi->setText( 1, KIO::convertSize( msgPart->decodedSize() ) );
  lvi->setText( 2, msgPart->contentTransferEncodingStr() );
  lvi->setText( 3, prettyMimeType( msgPart->typeStr() + '/' + msgPart->subtypeStr() ) );
  lvi->setAttachmentSize( msgPart->decodedSize() );

  if ( loadDefaults ) {
    if( canSignEncryptAttachments() ) {
      mAtmListView->enableCryptoCBs( true );
      lvi->setEncrypt( mEncryptAction->isChecked() );
      lvi->setSign(    mSignAction->isChecked() );
    } else {
      mAtmListView->enableCryptoCBs( false );
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::removeAttach( const QString &aUrl )
{
  QList<KMMessagePart*>::const_iterator it = mAtmList.begin();
  for( int idx = 0; it != mAtmList.end(); ++it, idx++) {
    if ( (*it)->name() == aUrl ) {
      removeAttach( idx );
      return;
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::removeAttach( int idx )
{
  mAtmModified = true;
  delete mAtmList.takeAt( idx );
  delete mAtmItemList.takeAt( idx );

  if ( mAtmList.empty() ) {
    mAtmListView->hide();
    mAtmListView->setMinimumSize( 0, 0 );
    resize( size() );
  }
}

//-----------------------------------------------------------------------------
bool KMComposeWin::encryptFlagOfAttachment( int idx )
{
  return (int)(mAtmItemList.count()) > idx
    ? static_cast<KMAtmListViewItem*>( mAtmItemList.at( idx ) )->isEncrypt()
    : false;
}

//-----------------------------------------------------------------------------
bool KMComposeWin::signFlagOfAttachment( int idx )
{
  return (int)(mAtmItemList.count()) > idx
    ? ((KMAtmListViewItem*)(mAtmItemList.at( idx )))->isSign()
    : false;
}

//-----------------------------------------------------------------------------
void KMComposeWin::setCharset( const QByteArray &aCharset, bool forceDefault )
{
  if ( ( forceDefault && GlobalSettings::self()->forceReplyCharset() ) ||
       aCharset.isEmpty() ) {
    mCharset = mDefCharset;
  } else {
    mCharset = aCharset.toLower();
  }

  if ( mCharset.isEmpty() || mCharset == "default" ) {
    mCharset = mDefCharset;
  }

  if ( mAutoCharset ) {
    mEncodingAction->setCurrentItem( 0 );
    return;
  }

  QStringList encodings = mEncodingAction->items();
  int i = 0;
  bool charsetFound = false;
  for ( QStringList::Iterator it = encodings.begin(); it != encodings.end();
        ++it, i++ ) {
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

  if ( !aCharset.isEmpty() && !charsetFound ) {
    setCharset( "", true );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAddrBook()
{
  KPIM::KAddrBookExternal::openAddressBook( this );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFile()
{
  // Create File Dialog and return selected file(s)
  // We will not care about any permissions, existence or whatsoever in
  // this function.

  KUrl url;
  KFileDialog fdlg( url, QString(), this );
  fdlg.setOperationMode( KFileDialog::Other );
  fdlg.setCaption( i18n("Attach File") );
  fdlg.okButton()->setGuiItem( KGuiItem( i18n("&Attach"), "document-open") );
  fdlg.setMode( KFile::Files );
  fdlg.exec();
  KUrl::List files = fdlg.selectedUrls();

  for ( KUrl::List::Iterator it = files.begin(); it != files.end(); ++it ) {
    addAttach(*it);
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFileData( KIO::Job *job, const QByteArray &data )
{
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.find( job );
  assert( it != mMapAtmLoadData.end() );
  QBuffer buff( &(*it).data );
  buff.open( QIODevice::WriteOnly | QIODevice::Append );
  buff.write( data.data(), data.size() );
  buff.close();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachFileResult( KJob *job )
{
  QMap<KIO::Job*, atmLoadData>::Iterator it = mMapAtmLoadData.find(static_cast<KIO::Job*>(job));

  assert( it != mMapAtmLoadData.end() );
  KUrl attachUrl;
  QMap<KJob*, KUrl>::Iterator jit = mAttachJobs.find( job );
  bool attachURLfound = (jit != mAttachJobs.end());
  if ( attachURLfound ) {
    attachUrl = jit.value();
    mAttachJobs.erase( jit );
  }
  if ( job->error() ) {
    mMapAtmLoadData.erase( it );
    static_cast<KIO::Job*>(job)->ui()->setWindow( 0 );
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
    if ( attachURLfound ) {
      emit attachmentAdded( attachUrl, false );
    }
    return;
  }

  if ( (*it).insert ) {
    (*it).data.resize((*it).data.size() + 1);
    (*it).data[(*it).data.size() - 1] = '\0';
    if ( const QTextCodec *codec = KGlobal::charsets()->codecForName((*it).encoding) ) {
        mEditor->textCursor().insertText( codec->toUnicode( (*it).data ) );
    } else {
        mEditor->textCursor().insertText( QString::fromLocal8Bit( (*it).data ) );
    }
    mMapAtmLoadData.erase(it);
    if ( attachURLfound ) {
      emit attachmentAdded( attachUrl, true );
    }
    return;
  }
  const QByteArray partCharset = (*it).url.fileEncoding().isEmpty()
    ? mCharset
    : QByteArray((*it).url.fileEncoding().toLatin1());

  KMMessagePart* msgPart;

  KCursorSaver busy( KBusyPtr::busy() );
  QString name( (*it).url.fileName() );
  // ask the job for the mime type of the file
  QString mimeType = static_cast<KIO::TransferJob*>(job)->mimetype();

  if ( name.isEmpty() ) {
    // URL ends with '/' (e.g. http://www.kde.org/)
    // guess a reasonable filename
    if ( mimeType == "text/html" ) {
      name = "index.html";
    } else {
      // try to determine a reasonable extension
      QStringList patterns( KMimeType::mimeType( mimeType )->patterns() );
      QString ext;
      if ( !patterns.isEmpty() ) {
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

  QByteArray encoding = KMMsgBase::autoDetectCharset(partCharset,
                                                     KMMessage::preferredCharsets(), name);
  if (encoding.isEmpty()) encoding = "utf-8";

  QByteArray encName;
  if ( GlobalSettings::self()->outlookCompatibleAttachments() ) {
    encName = KMMsgBase::encodeRFC2047String( name, encoding );
  } else {
    encName = KMMsgBase::encodeRFC2231String( name, encoding );
  }

  bool RFC2231encoded = false;
  if ( !GlobalSettings::self()->outlookCompatibleAttachments() ) {
    RFC2231encoded = name != QString( encName );
  }

  // create message part
  msgPart = new KMMessagePart;
  msgPart->setName( name );
  QList<int> allowedCTEs;
  if ( mimeType == "message/rfc822" ) {
    msgPart->setMessageBody( (*it).data );
    allowedCTEs << DwMime::kCte7bit;
    allowedCTEs << DwMime::kCte8bit;
  } else {
    msgPart->setBodyAndGuessCte( (*it).data, allowedCTEs,
                                 !kmkernel->msgSender()->sendQuotedPrintable() );
    kDebug(5006) <<"autodetected cte:" << msgPart->cteStr();
  }
  int slash = mimeType.indexOf( '/' );
  if( slash == -1 )
    slash = mimeType.length();
  msgPart->setTypeStr( mimeType.left( slash ).toLatin1() );
  msgPart->setSubtypeStr( mimeType.mid( slash + 1 ).toLatin1() );
  msgPart->setContentDisposition(QByteArray("attachment;\n\tfilename")
                                 + ( RFC2231encoded ? "*=" + encName : "=\"" + encName + '"' ) );

  mMapAtmLoadData.erase(it);

  msgPart->setCharset( partCharset );

  // show message part dialog, if not configured away (default):
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
      if ( attachURLfound ) {
        emit attachmentAdded( attachUrl, false );
      }
      return;
    }
  }
  mAtmModified = true;
  if (msgPart->typeStr().toLower() != "text") msgPart->setCharset(QByteArray());

  // add the new attachment to the list
  addAttach(msgPart);

  if ( attachURLfound ) {
    emit attachmentAdded( attachUrl, true );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertFile()
{
  QString encodingStr;
  KUrl u = mEditor->insertFile(KMMsgBase::supportedEncodings(false),encodingStr );
  if( u.isEmpty()) return;

  mRecentAction->addUrl(u);
  // Prevent race condition updating list when multiple composers are open
  {
    KConfig *config = KMKernel::config();
    KConfigGroup group( config, "Composer" );
    QString encoding = KGlobal::charsets()->encodingForName(encodingStr).toLatin1();
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
    urls.prepend( u.prettyUrl() );
    encodings.prepend( encoding );
    group.writeEntry( "recent-urls", urls );
    group.writeEntry( "recent-encodings", encodings );
    mRecentAction->saveEntries( config->group( QString() ) );
  }
  slotInsertRecentFile(u);
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotInsertRecentFile( const KUrl &u )
{
  if ( u.fileName().isEmpty() ) {
    return;
  }

  KIO::Job *job = KIO::get( u );
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
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotSetCharset()
{
  if ( mEncodingAction->currentItem() == 0 ) {
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
  if ( !init )
    setModified( true );

  mAtmListView->enableCryptoCBs( canSignEncryptAttachments() );
}

static void showExportError( QWidget * w, const GpgME::Error & err )
{
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
  if ( !mFingerprint.isEmpty() ) {
    startPublicKeyExport();
  }
}

void KMComposeWin::startPublicKeyExport()
{
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

void KMComposeWin::slotPublicKeyExportResult( const GpgME::Error & err, const QByteArray & keydata )
{
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
  msgPart->setContentDisposition( "attachment;\n\tfilename=0x" + QByteArray( mFingerprint.toLatin1() ) + ".asc" );

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

  if ( dlg.exec() != QDialog::Accepted ) {
    return;
  }

  mFingerprint = dlg.fingerprint();
  startPublicKeyExport();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachPopupMenu( QTreeWidgetItem* )
{
  if ( !mAttachMenu ) {
    mAttachMenu = new QMenu( this );

    mOpenId = mAttachMenu->addAction( i18nc("to open", "Open"), this,
                                      SLOT(slotAttachOpen()) );
    mViewId = mAttachMenu->addAction( i18nc("to view", "View"), this,
                                      SLOT(slotAttachView())) ;
    mEditAction = mAttachMenu->addAction( i18n("Edit"), this, SLOT(slotAttachEdit()) );
    mEditWithAction = mAttachMenu->addAction( i18n("Edit With..."), this,
                                            SLOT(slotAttachEditWith()) );
    mRemoveId = mAttachMenu->addAction( i18n("Remove"), this, SLOT(slotAttachRemove()) );
    mSaveAsId = mAttachMenu->addAction( KIcon("document-save-as"), i18n("Save As..."), this,
                                        SLOT( slotAttachSave() ) );
    mPropertiesId = mAttachMenu->addAction( i18n("Properties"), this,
                                            SLOT( slotAttachProperties() ) );
    mAttachMenu->addSeparator();
    mAttachMenu->addAction( i18n("Add Attachment..."), this, SLOT(slotAttachFile()) );
  }

  int selectedCount = 0;
  QList<KMAtmListViewItem*>::const_iterator it = mAtmItemList.constBegin();
  while ( it != mAtmItemList.constEnd() ) {
    if ( (*it)->isSelected() ) {
      ++selectedCount;
    }
    ++it;
  }

  mOpenId->setEnabled( selectedCount > 0 );
  mViewId->setEnabled( selectedCount > 0 );
  mEditAction->setEnabled( selectedCount == 1 );
  mEditWithAction->setEnabled( selectedCount == 1 );
  mRemoveId->setEnabled( selectedCount > 0 );
  mSaveAsId->setEnabled( selectedCount == 1 );
  mPropertiesId->setEnabled( selectedCount == 1 );

  mAttachMenu->popup(QCursor::pos());
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachProperties()
{
  KMAtmListViewItem *currentItem =
      static_cast<KMAtmListViewItem*>( mAtmListView->currentItem() );
  if ( !currentItem )
    return;

  KMMessagePart *msgPart = currentItem->attachment();
  msgPart->setCharset( mCharset );

  KMMsgPartDialogCompat dlg;
  dlg.setMsgPart( msgPart );
  if ( canSignEncryptAttachments() ) {
    dlg.setCanSign( true );
    dlg.setCanEncrypt( true );
    dlg.setSigned( currentItem->isSign() );
    dlg.setEncrypted( currentItem->isEncrypt() );
  } else {
    dlg.setCanSign( false );
    dlg.setCanEncrypt( false );
  }
  if ( dlg.exec() ) {
    mAtmModified = true;
    // values may have changed, so recreate the listbox line
    msgPartToItem( msgPart, currentItem );
    if ( canSignEncryptAttachments() ) {
      currentItem->setSign( dlg.isSigned() );
      currentItem->setEncrypt( dlg.isEncrypted() );
    }
  }
  if ( msgPart->typeStr().toLower() != "text" ) {
    msgPart->setCharset( QByteArray() );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::compressAttach( KMAtmListViewItem *attachmentItem )
{
  if ( !attachmentItem )
    return;

  KMMessagePart *msgPart = attachmentItem->attachment();
  QByteArray array;
  QBuffer dev( &array );
  KZip zip( &dev );
  QByteArray decoded = msgPart->bodyDecodedBinary();
  if ( ! zip.open( QIODevice::WriteOnly ) ) {
    KMessageBox::sorry(0, i18n("KMail could not compress the file.") );
    attachmentItem->setCompress( false );
    return;
  }

  zip.setCompression( KZip::DeflateCompression );
  if ( ! zip.writeFile( msgPart->name(), "", "", decoded.data(), decoded.size()) ) {
    KMessageBox::sorry (0, i18n("KMail could not compress the file.") );
    attachmentItem->setCompress( false );
    return;
  }
  zip.close();
  if ( array.size() >= decoded.size() ) {
    if ( KMessageBox::questionYesNo( this,
                                     i18n( "The compressed file is larger "
                                           "than the original. Do you want to keep the original one?" ),
                                     QString(),
                                     KGuiItem( i18nc("Do not compress", "Keep") ),
                                     KGuiItem( i18n("Compress") ) ) == KMessageBox::Yes ) {
      attachmentItem->setCompress( false );
      return;
    }
  }
  attachmentItem->setUncompressedCodec( msgPart->cteStr() );

  msgPart->setCteStr( "base64" );
  msgPart->setBodyEncodedBinary( array );
  QString name = msgPart->name() + ".zip";

  msgPart->setName( name );

  QByteArray cDisp = "attachment;";
  QByteArray encoding = KMMsgBase::autoDetectCharset( msgPart->charset(),
                                                      KMMessage::preferredCharsets(), name );
  kDebug(5006) <<"encoding:" << encoding;
  if ( encoding.isEmpty() ) {
    encoding = "utf-8";
  }
  kDebug(5006) <<"encoding after:" << encoding;
  QByteArray encName;
  if ( GlobalSettings::self()->outlookCompatibleAttachments() ) {
    encName = KMMsgBase::encodeRFC2047String( name, encoding );
  } else {
    encName = KMMsgBase::encodeRFC2231String( name, encoding );
  }

  cDisp += "\n\tfilename";
  if ( name != QString( encName ) ) {
    cDisp += "*=" + encName;
  } else {
    cDisp += "=\"" + encName + '"';
  }
  msgPart->setContentDisposition( cDisp );

  attachmentItem->setUncompressedMimeType( msgPart->typeStr(),
                                           msgPart->subtypeStr() );
  msgPart->setTypeStr( "application" );
  msgPart->setSubtypeStr( "zip" );

  msgPartToItem( msgPart, attachmentItem, false );
}

//-----------------------------------------------------------------------------

void KMComposeWin::uncompressAttach( KMAtmListViewItem *attachmentItem )
{
  if ( !attachmentItem )
    return;

  KMMessagePart *msgPart = attachmentItem->attachment();
  QByteArray ba = msgPart->bodyDecodedBinary();
  QBuffer dev( &ba );
  KZip zip( &dev );
  QByteArray decoded;

  decoded = msgPart->bodyDecodedBinary();
  if ( ! zip.open( QIODevice::ReadOnly ) ) {
    KMessageBox::sorry(0, i18n("KMail could not uncompress the file.") );
    attachmentItem->setCompress( true );
    return;
  }
  const KArchiveDirectory *dir = zip.directory();

  KZipFileEntry *entry;
  if ( dir->entries().count() != 1 ) {
    KMessageBox::sorry(0, i18n("KMail could not uncompress the file.") );
    attachmentItem->setCompress( true );
    return;
  }
  entry = (KZipFileEntry*)dir->entry( dir->entries()[0] );

  msgPart->setCteStr( attachmentItem->uncompressedCodec() );

  msgPart->setBodyEncodedBinary( entry->data() );
  QString name = entry->name();
  msgPart->setName( name );

  zip.close();

  QByteArray cDisp = "attachment;";
  QByteArray encoding = KMMsgBase::autoDetectCharset( msgPart->charset(),
                                                      KMMessage::preferredCharsets(), name );
  if ( encoding.isEmpty() ) {
    encoding = "utf-8";
  }

  QByteArray encName;
  if ( GlobalSettings::self()->outlookCompatibleAttachments() ) {
    encName = KMMsgBase::encodeRFC2047String( name, encoding );
  } else {
    encName = KMMsgBase::encodeRFC2231String( name, encoding );
  }

  cDisp += "\n\tfilename";
  if ( name != QString( encName ) ) {
    cDisp += "*=" + encName;
  } else {
    cDisp += "=\"" + encName + '"';
  }
  msgPart->setContentDisposition( cDisp );

  QByteArray type, subtype;
  attachmentItem->uncompressedMimeType( type, subtype );

  msgPart->setTypeStr( type );
  msgPart->setSubtypeStr( subtype );

  msgPartToItem( msgPart, attachmentItem, false );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachView()
{
  int i = 0;
  QList<KMAtmListViewItem*>::const_iterator it;
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
  QList<KMAtmListViewItem*>::const_iterator it;
  for ( it = mAtmItemList.constBegin();
        it != mAtmItemList.constEnd(); ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      openAttach( i );
    }
  }
}

//-----------------------------------------------------------------------------
bool KMComposeWin::inlineSigningEncryptionSelected()
{
  if ( !mSignAction->isChecked() && !mEncryptAction->isChecked() ) {
    return false;
  }
  return cryptoMessageFormat() == Kleo::InlineOpenPGPFormat;
}

void KMComposeWin::slotAttachEdit()
{
  int i = 0;
  for ( QList<KMAtmListViewItem*>::ConstIterator it = mAtmItemList.constBegin();
        it != mAtmItemList.constEnd(); ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      editAttach( i, false );
    }
  }
}

void KMComposeWin::slotAttachEditWith()
{
  int i = 0;
  for ( QList<KMAtmListViewItem*>::ConstIterator it = mAtmItemList.constBegin();
        it != mAtmItemList.constEnd(); ++it, ++i ) {
    if ( (*it)->isSelected() ) {
      editAttach( i, true );
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::viewAttach( int index )
{
  QString pname;
  KMMessagePart *msgPart;
  msgPart = mAtmList.at( index );
  pname = msgPart->name().trimmed();
  if ( pname.isEmpty() ) {
    pname = msgPart->contentDescription();
  }
  if ( pname.isEmpty() ) {
    pname = "unnamed";
  }

  KTemporaryFile *atmTempFile = new KTemporaryFile();
  atmTempFile->open();
  mAtmTempList.append( atmTempFile );
  KPIMUtils::kByteArrayToFile( msgPart->bodyDecodedBinary(), atmTempFile->fileName(),
                               false, false, false );
  KMReaderMainWin *win =
    new KMReaderMainWin( msgPart, false, atmTempFile->fileName(), pname, mCharset );
  win->show();
}

//-----------------------------------------------------------------------------
void KMComposeWin::openAttach( int index )
{
  KMMessagePart *msgPart = mAtmList.at( index );
  const QString contentTypeStr =
    ( msgPart->typeStr() + '/' + msgPart->subtypeStr() ).toLower();

  KTemporaryFile *atmTempFile = new KTemporaryFile();
  atmTempFile->open();
  mAtmTempList.append( atmTempFile );
  const bool autoDelete = true;
  atmTempFile->setAutoRemove( autoDelete );

  KUrl url;
  url.setPath( atmTempFile->fileName() );

  KPIMUtils::kByteArrayToFile( msgPart->bodyDecodedBinary(), atmTempFile->fileName(), false, false,
                          false );
  if ( ::chmod( QFile::encodeName( atmTempFile->fileName() ), S_IRUSR ) != 0) {
    QFile::remove(url.path());
    return;
  }

  KMimeType::Ptr mimetype = KMimeType::mimeType( contentTypeStr );
  KService::Ptr offer;
  if ( !mimetype.isNull() ) {
    offer =
      KMimeTypeTrader::self()->preferredService( mimetype->name(), "Application" );
  }

  if ( !offer || mimetype.isNull() ) {
    if ( ( !KRun::displayOpenWithDialog( url, this, autoDelete ) ) && autoDelete ) {
      QFile::remove(url.path());
    }
  } else {
    if ( ( !KRun::run( *offer, url, this, autoDelete ) ) && autoDelete ) {
      QFile::remove( url.path() );
    }
  }
}

void KMComposeWin::editAttach(int index, bool openWith)
{
  KMMessagePart* msgPart = mAtmList.at(index);
  const QString contentTypeStr =
    ( msgPart->typeStr() + '/' + msgPart->subtypeStr() ).toLower();

  KTemporaryFile* atmTempFile = new KTemporaryFile();
  if ( !atmTempFile->open() ) {
    KMessageBox::sorry( this,
         i18nc( "@info",
                "KMail was unable to create the temporary file <filename>%1</filename>.\n"
                "Because of this, editing this attachment is not possible.",
                atmTempFile->fileName() ),
         i18n( "Unable to edit attachment" ) );
    return;
  }
  mAtmTempList.append( atmTempFile );
  atmTempFile->setAutoRemove( true );
  atmTempFile->write( msgPart->bodyDecodedBinary() );
  atmTempFile->flush();


  KMail::EditorWatcher *watcher =
      new KMail::EditorWatcher( KUrl( atmTempFile->fileName() ),
                                contentTypeStr, openWith, this );
  connect( watcher, SIGNAL(editDone(KMail::EditorWatcher*)),
           SLOT(slotEditDone(KMail::EditorWatcher*)) );
  if ( watcher->start() ) {
    mEditorMap.insert( watcher, msgPart );
    mEditorTempFiles.insert( watcher, atmTempFile );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachSave()
{
  KMAtmListViewItem *currentItem =
      static_cast<KMAtmListViewItem*>( mAtmListView->currentItem() );
  if ( !currentItem )
    return;

  KMMessagePart *msgPart = currentItem->attachment();
  QString pname = msgPart->name();
  if ( pname.isEmpty() ) {
    pname = "unnamed";
  }

  KUrl url = KFileDialog::getSaveUrl(QString(), QString(), 0, i18n("Save Attachment As"));

  if ( url.isEmpty() ) {
    return;
  }

  kmkernel->byteArrayToRemoteFile( msgPart->bodyDecodedBinary(), url );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotAttachRemove()
{
  bool attachmentRemoved = false;
  for ( int i = 0; i < mAtmItemList.size(); ) {
    KMAtmListViewItem *cur = mAtmItemList[i];
    if ( cur->isSelected() ) {
      removeAttach( i );
      attachmentRemoved = true;
    }
    else
      i++;
  }

  if ( attachmentRemoved ) {
    setModified( true );
    slotUpdateAttachActions();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotUpdateFont()
{
  kDebug(5006) <<"KMComposeWin::slotUpdateFont";
  if ( ! mFixedFontAction ) {
    return;
  }
  mEditor->setFont( mFixedFontAction->isChecked() ? mFixedFont : mBodyFont );
}

QString KMComposeWin::quotePrefixName() const
{
  if ( !msg() ) {
    return QString();
  }

  int languageNr = GlobalSettings::self()->replyCurrentLanguage();
  ReplyPhrases replyPhrases( QString::number( languageNr ) );
  replyPhrases.readConfig();
  QString quotePrefix = msg()->formatString( replyPhrases.indentPrefix() );

  quotePrefix = msg()->formatString( quotePrefix );
  return quotePrefix;
}

QString KMComposeWin::smartQuote( const QString & msg )
{
  return KMMessage::smartQuote( msg, GlobalSettings::self()->lineWrapWidth() );
}

void KMComposeWin::slotPasteAsAttachment()
{
  if ( KUrl::List::canDecode( QApplication::clipboard()->mimeData() ) )
  {
    QStringList data = QApplication::clipboard()->text().split("\n", QString::SkipEmptyParts);
    for ( QStringList::Iterator it=data.begin(); it!=data.end(); ++it )
    {
      addAttach( *it );
    }
    return;
  }

  const QMimeData *mimeData = QApplication::clipboard()->mimeData();
  if ( mimeData->hasImage() ) {
    slotAttachPNGImageData( mimeData->data( "image/png" ) );
  } else {
    bool ok;
    QString attName =
      KInputDialog::getText( "KMail", i18n("Name of the attachment:"), QString(), &ok, this );
    if ( !ok ) {
      return;
    }

    KMMessagePart *msgPart = new KMMessagePart;
    msgPart->setName( attName );
    QList<int> dummy;
    msgPart->setBodyAndGuessCte( QByteArray( QApplication::clipboard()->text().toLatin1() ),
                                 dummy, kmkernel->msgSender()->sendQuotedPrintable());
    addAttach( msgPart );
  }
}

QString KMComposeWin::addQuotesToText( const QString &inputText ) const
{
  QString answer = QString( inputText );
  QString indentStr = quotePrefixName();
  answer.replace( '\n', '\n' + indentStr );
  answer.prepend( indentStr );
  answer += '\n';
  return KMMessage::smartQuote( answer, GlobalSettings::self()->lineWrapWidth() );
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
  } else if (::qobject_cast<QLineEdit*>( fw )) {
    static_cast<QLineEdit*>( fw )->undo();
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
  } else if (::qobject_cast<QLineEdit*>( fw )) {
    static_cast<QLineEdit*>( fw )->redo();
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
  } else if ( ::qobject_cast<QLineEdit*>( fw ) ) {
    static_cast<QLineEdit*>( fw )->cut();
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

  const QMimeData *mimeData = QApplication::clipboard()->mimeData();
  const KUrl::List urlList = KUrl::List::fromMimeData( mimeData );
  if ( mimeData->hasFormat( "image/png" ) )  {
    slotAttachPNGImageData( mimeData->data( "image/png" ) );
  } else if ( !urlList.isEmpty() ) {
    const QString asText = i18n("Add as Text");
    const QString asAttachment = i18n("Add as Attachment");
    const QString text = i18n("Please select whether you want to insert the content as text into the editor, "
        "or append the referenced file as an attachment.");
    const QString caption = i18n("Paste as text or attachment?");
    int id = KMessageBox::questionYesNoCancel( this, text, caption,
        KGuiItem( asText ),
        KGuiItem( asAttachment) );
    switch ( id) {
      case KMessageBox::Yes:
        for ( KUrl::List::ConstIterator it = urlList.begin();
            it != urlList.end(); ++it ) {
          mEditor->textCursor().insertText( (*it).url() );
        }
        break;
      case KMessageBox::No:
        for ( KUrl::List::ConstIterator it = urlList.begin();
            it != urlList.end(); ++it ) {
          addAttach( *it );
        }
        break;
    }
  } else if ( mimeData->hasText() ) {
      if ( mHtmlMarkup )
      {
        toggleMarkup( true );
        mEditor->textCursor().insertHtml( mimeData->html() );
      }
      else
        mEditor->textCursor().insertText( mimeData->text() );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotMarkAll()
{
  QWidget *fw = focusWidget();
  if ( !fw ) {
    return;
  }

  if ( ::qobject_cast<QLineEdit*>( fw ) ) {
    static_cast<QLineEdit*>( fw )->selectAll();
  } else if (::qobject_cast<KMComposerEditor*>( fw )) {
    static_cast<KTextEdit*>( fw )->selectAll();
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotClose()
{
#ifdef __GNUC__
#warning "Port me: make sure to set/unset the  Qt::WA_DeleteOnClose flag to match the old behavior of close(false)"
#endif
  close();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotNewComposer()
{
  KMComposeWin *win;
  KMMessage *msg = new KMMessage;

  msg->initHeader();
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
  if ( canSignEncryptAttachments() ) {
    QList<KMAtmListViewItem*>::const_iterator it;
    for (it = mAtmItemList.constBegin(); it != mAtmItemList.constEnd(); ++it ) {
      KMAtmListViewItem *entry = static_cast<KMAtmListViewItem*> (*it);
      if ( entry ) {
        entry->setEncrypt( encrypt );
      }
    }
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
    }
    sign = false;
  }

  // make sure the mSignAction is in the right state
  mSignAction->setChecked( sign );

  // mark the attachments for (no) signing
  if ( canSignEncryptAttachments() ) {
    QList<KMAtmListViewItem*>::const_iterator it;
    for (it = mAtmItemList.constBegin(); it != mAtmItemList.constEnd(); ++it ) {
      KMAtmListViewItem *entry = static_cast<KMAtmListViewItem*> (*it);
      if ( entry ) {
        entry->setSign( sign );
      }
    }
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotWordWrapToggled( bool on )
{
  mEditor->wordWrapToggled( on );
  if (on)
  {
    mEditor->setWrapColumnOrWidth( GlobalSettings::self()->lineWrapWidth() );
  }
}

void KMComposeWin::disableWordWrap()
{
  mEditor->setWordWrapMode( QTextOption::NoWrap );
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

  if ( rc ) {
    if ( mComposedMessages.isEmpty() ) {
      kDebug(5006) <<"Composing the message failed.";
      return;
    }
    KMCommand *command = new KMPrintCommand( this, mComposedMessages.first() );
    command->start();
    setModified( mMessageWasModified );
  }
}

//----------------------------------------------------------------------------
bool KMComposeWin::validateAddresses( QWidget *parent, const QString &addresses )
{
  QString brokenAddress;
  KPIMUtils::EmailParseResult errorCode =
    KPIMUtils::isValidAddressList( KMMessage::expandAliases( addresses ),
                                   brokenAddress );
  if ( !( errorCode == KPIMUtils::AddressOk ||
          errorCode == KPIMUtils::AddressEmpty ) ) {
    QString errorMsg( "<qt><p><b>" + brokenAddress +
                      "</b></p><p>" +
                      KPIMUtils::emailParseResultToString( errorCode ) +
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
                                             i18n("No To: specified") );
        if ( rc == KMessageBox::No ) {
          return;
        }
      }
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

  KCursorSaver busy( KBusyPtr::busy() );
  mMsg->setDateToday();

  mMsg->setHeaderField( "X-KMail-Transport", mTransport->currentText() );

  mDisableBreaking = ( saveIn != KMComposeWin::None );

  const bool neverEncrypt = ( mDisableBreaking && GlobalSettings::self()->neverEncryptDrafts() ) ||
    mSigningAndEncryptionExplicitlyDisabled;
  connect( this, SIGNAL( applyChangesDone( bool ) ),
           SLOT( slotContinueDoSend( bool ) ) );

  if ( mEditor->htmlMode() ) {
    kDebug(5006) << "Html mode";
    kDebug(5006) << "mailtext :" << mEditor->text();
    mMsg->setHeaderField( "X-KMail-Markup", "true" );
  } else {
    mMsg->removeHeaderField( "X-KMail-Markup" );
    kDebug(5006) << "Plain text";
    kDebug(5006) << "mailtext :" << mEditor->text();
  }
  if ( mEditor->htmlMode() && inlineSigningEncryptionSelected() ) {
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
      toggleMarkup( false );
    }
  }

  kDebug(5006) <<"KMComposeWin::doSend() - calling applyChanges()";

  if (neverEncrypt && saveIn != KMComposeWin::None ) {
      // we can't use the state of the mail itself, to remember the
      // signing and encryption state, so let's add a header instead
    mMsg->setHeaderField( "X-KMail-SignatureActionEnabled", mSignAction->isChecked()? "true":"false" );
    mMsg->setHeaderField( "X-KMail-EncryptActionEnabled", mEncryptAction->isChecked()? "true":"false"  );
    mMsg->setHeaderField( "X-KMail-CryptoMessageFormat", QString::number( cryptoMessageFormat() ) );
  } else {
    mMsg->removeHeaderField( "X-KMail-SignatureActionEnabled" );
    mMsg->removeHeaderField( "X-KMail-EncryptActionEnabled" );
    mMsg->removeHeaderField( "X-KMail-CryptoMessageFormat" );
  }

  applyChanges( neverEncrypt );
}

bool KMComposeWin::saveDraftOrTemplate( const QString &folderName,
                                        KMMessage *msg )
{
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
      const KPIMIdentities::Identity &id = kmkernel->identityManager()->identityForUoidOrDefault( msg->headerField( "X-KMail-Identity" ).trimmed().toUInt() );
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
  kDebug(5006) <<"theFolder=" << theFolder->name();
  if ( imapTheFolder ) {
    kDebug(5006) <<"imapTheFolder=" << imapTheFolder->name();
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
}

void KMComposeWin::slotContinueDoSend( bool sentOk )
{
  kDebug(5006) <<"KMComposeWin::slotContinueDoSend(" << sentOk <<")";
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

    if ( mSaveIn == KMComposeWin::Drafts ) {
      sentOk = saveDraftOrTemplate( (*it)->drafts(), (*it) );
    } else if ( mSaveIn == KMComposeWin::Templates ) {
      sentOk = saveDraftOrTemplate( (*it)->templates(), (*it) );
    } else {
      (*it)->setTo( KMMessage::expandAliases( to() ));
      (*it)->setCc( KMMessage::expandAliases( cc() ));
      if ( !mComposer->originalBCC().isEmpty() ) {
        (*it)->setBcc( KMMessage::expandAliases( mComposer->originalBCC() ) );
      }
      QString recips = (*it)->headerField( "X-KMail-Recipients" );
      if ( !recips.isEmpty() ) {
        (*it)->setHeaderField( "X-KMail-Recipients", KMMessage::expandAliases( recips ), KMMessage::Address );
      }
      (*it)->cleanupHeader();
      sentOk = kmkernel->msgSender()->send( (*it), mSendMethod );
    }

    if ( !sentOk ) {
      return;
    }

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
}

//----------------------------------------------------------------------------
void KMComposeWin::slotSendLater()
{
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
void KMComposeWin::slotAppendSignature()
{
  insertSignatureHelper( KPIM::KMeditor::End );
}

//----------------------------------------------------------------------------
void KMComposeWin::slotPrependSignature()
{
  insertSignatureHelper( KPIM::KMeditor::Start );
}

//----------------------------------------------------------------------------
void KMComposeWin::slotInsertSignatureAtCursor()
{
  insertSignatureHelper( KPIM::KMeditor::AtCursor );
}

//----------------------------------------------------------------------------
void KMComposeWin::insertSignatureHelper( KPIM::KMeditor::Placement placement )
{
  // Identity::signature() is not const, although it should be, therefore the
  // const_cast.
  KPIMIdentities::Identity &ident = const_cast<KPIMIdentities::Identity&>(
      kmkernel->identityManager()->identityForUoidOrDefault(
                                mIdentity->currentIdentity() ) );

  mOldSigText = ident.signatureText();
  if ( ident.signatureIsInlinedHtml() ) {
    kDebug(5006) << "Html signature, turning editor into html mode";
    toggleMarkup( true );
    mEditor->insertSignature( ident.signature(), placement, true );
  }
  else
    mEditor->insertSignature( ident.signature(), placement );
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
  QTextCursor cursor = mEditor->textCursor();
  if ( cursor.hasSelection() ) {
    s = cursor.selectedText();
    if ( s.isEmpty() ) {
      return;
    }
  } else {
    s = mEditor->text();
  }

  // Remove the signature for now.
  QString sig;
  bool restore = false;
  const KPIMIdentities::Identity &ident =
    kmkernel->identityManager()->identityForUoid( mId );
  if ( !ident.isNull() ) {
    sig = ident.signatureText();
    if ( !sig.isEmpty() ) {
      if ( s.endsWith( sig ) ) {
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
  if ( restore ) {
    s.append( sig );
  }

  // Put the new text in place.
  // The lines below do not clear the undo history, but unfortuately cause
  // the side-effect that you need to press Ctrl-Z twice (first Ctrl-Z will
  // show cleared text area) to get back the original, pre-cleaned text.
  // If you use mEditor->setText( s ) then the undo history is cleared so
  // that isn't a good solution either.
  // TODO: is Qt4 better at handling the undo history??
  if ( !cursor.hasSelection() ) {
    cursor.clearSelection();
  }
  mEditor->insert( s );
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotToggleMarkup()
{
  if ( markupAction->isChecked() ) {
    mHtmlMarkup = true;
    toolBar( "htmlToolBar" )->show();
    // markup will be toggled as soon as markup is actually used
    fontChanged( mEditor->currentFont() ); // set buttons in correct position
    mSaveFont = mEditor->currentFont();
  } else {
    toggleMarkup( false );
  }
}

//-----------------------------------------------------------------------------
void KMComposeWin::toggleMarkup( bool markup )
{
  if ( markup ) {
    if ( !mUserUsesHtml ) {
      kDebug(5006) << "user wants Html";
      mUserUsesHtml = true; // set it directly to true. setColor hits another toggleMarkup
      mHtmlMarkup = true;
      // save the buttonstates because setHtmlMode calls fontChanged
      bool _bold = textBoldAction->isChecked();
      bool _italic = textItalicAction->isChecked();
      mEditor->setHtmlMode( true );
      textBoldAction->setChecked( _bold );
      textItalicAction->setChecked( _italic );

      markupAction->setChecked( true );
      toolBar( "htmlToolBar" )->show();
      //mEditor->deleteAutoSpellChecking();
      mAutoSpellCheckingAction->setChecked( false );
      slotAutoSpellCheckingToggled( false );
      mCleanSpace->setEnabled(false);
    }
  } else { // markup is to be turned off
    kDebug(5006) <<" user wants textmode";
    mHtmlMarkup = false;
    toolBar( "htmlToolBar" )->hide();
    if ( mUserUsesHtml ) { // it was turned on
      mUserUsesHtml = false;
      mEditor->setHtmlMode( false );
      mEditor->selectAll();
      mEditor->setCurrentFont( mSaveFont );
      mEditor->moveCursor( QTextCursor::Start, QTextCursor::MoveAnchor ); // deselect
      // like the next 2 lines, or should we selectAll and apply the default font?
      slotAutoSpellCheckingToggled( true );
      mCleanSpace->setEnabled(true);
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
  // FIXME this doesn't work at the moment since KMeditor does not
  // support continuous spell checking when in RichText mode, i.e.
  // the createHighlighter() method is never called.
  mEditor->setCheckSpellingEnabled( on );

  QString temp;
  if ( on ) {
    temp = i18n( "Spellcheck: on" );
  } else {
    temp = i18n( "Spellcheck: off" );
  }
  statusBar()->changeItem( temp, 3 );
}


// FIXME "ensurePolished() should be a const method, but we call non-const method"
void KMComposeWin::ensurePolished()
{
  // Ensure the html toolbar is appropriately shown/hidden
  markupAction->setChecked( mHtmlMarkup );
  if ( mHtmlMarkup ) {
    toolBar( "htmlToolBar" )->show();
  } else {
    toolBar( "htmlToolBar" )->hide();
  }
  KMail::Composer::ensurePolished();
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
void KMComposeWin::slotIdentityChanged( uint uoid )
{
  const KPIMIdentities::Identity &ident =
    kmkernel->identityManager()->identityForUoid( uoid );
  if ( ident.isNull() ) {
    return;
  }

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
      kmkernel->identityManager()->identityForUoidOrDefault( mId );
  if ( oldIdentity.bcc() != ident.bcc() ) {
    mRecipientsEditor->removeRecipient( oldIdentity.bcc(), Recipient::Bcc );
    mRecipientsEditor->addRecipient( ident.bcc(), Recipient::Bcc );
    mRecipientsEditor->setFocusBottom();
  }

  if ( ident.organization().isEmpty() ) {
    mMsg->removeHeaderField( "Organization" );
  } else {
    mMsg->setHeaderField( "Organization", ident.organization() );
  }

  if ( !ident.isXFaceEnabled() || ident.xface().isEmpty() ) {
    mMsg->removeHeaderField( "X-Face" );
  } else {
    QString xface = ident.xface();
    if ( !xface.isEmpty() ) {
      int numNL = ( xface.length() - 1 ) / 70;
      for ( int i = numNL; i > 0; --i ) {
        xface.insert( i*70, "\n\t" );
      }
      mMsg->setHeaderField( "X-Face", xface );
    }
  }

  // If the transport sticky checkbox is not checked, set the transport
  // from the new identity
  if ( !mBtnTransport->isChecked() ) {
    QString transportName = ident.transport();
    Transport *transport =
        TransportManager::self()->transportByName( transportName, false );
    if ( !transport ) {
      mMsg->removeHeaderField( "X-KMail-Transport" );
      mTransport->setCurrentTransport(
                               TransportManager::self()->defaultTransportId() );
    }
    else {
      mMsg->setHeaderField( "X-KMail-Transport", transportName );
      mTransport->setCurrentTransport( transport->id() );
    }
  }

  mDictionaryCombo->setCurrentByDictionary( ident.dictionary() );

  if ( !mBtnFcc->isChecked() ) {
    setFcc( ident.fcc() );
  }

  QString edtText = mEditor->text();
  bool appendNewSig = true;
  // try to truncate the old sig
  if ( !mOldSigText.isEmpty() ) {
    if ( edtText.endsWith( mOldSigText ) ) {
      edtText.truncate( edtText.length() - mOldSigText.length() );
    } else {
      appendNewSig = false;
    }
  }
  // now append the new sig
  mOldSigText = ident.signatureText();
  if ( appendNewSig ) {
    if ( (!mOldSigText.isEmpty()) &&
         (GlobalSettings::self()->autoTextSignature() == "auto") ) {
      if ( mEditor->htmlMode() == false ) // TODO : support for adding according signature when changing identities in html
        edtText.append( mOldSigText );
    }
    if ( mEditor->htmlMode() == false )
      mEditor->setPlainText( edtText );
    else
      mEditor->setHtml( edtText );
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
  if ( bNewIdentityHasEncryptionKey && !mLastIdentityHasEncryptionKey ) {
    setEncryption( mLastEncryptActionState );
  }
  if ( bNewIdentityHasSigningKey && !mLastIdentityHasSigningKey ) {
    setSigning( mLastSignActionState );
  }

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
  //Laurent TODO fix me
  KConfig config("kmailrc");
  Sonnet::ConfigDialog dialog(&config, this);
  dialog.setWindowIcon( KIcon( "internet-mail" ) );
  dialog.exec();
}

//-----------------------------------------------------------------------------
void KMComposeWin::slotStatusMessage( const QString &message )
{
  statusBar()->changeItem( message, 0 );
}

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
  kDebug(5006) ;
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
  } else {
    if ( !mAutoSaveTimer ) {
      mAutoSaveTimer = new QTimer( this );
      connect( mAutoSaveTimer, SIGNAL( timeout() ),
               this, SLOT( autoSaveMessage() ) );
    }
    mAutoSaveTimer->start( autoSaveInterval() );
  }
}

void KMComposeWin::setAutoSaveFilename( const QString &filename )
{
  if ( !mAutoSaveFilename.isEmpty() ) {
    KMFolderMaildir::removeFile( KMKernel::localDataPath() + "autosave",
                                 mAutoSaveFilename );
  }
  mAutoSaveFilename = filename;
}

void KMComposeWin::cleanupAutoSave()
{
  delete mAutoSaveTimer; mAutoSaveTimer = 0;
  if ( !mAutoSaveFilename.isEmpty() ) {
    kDebug(5006) <<"deleting autosave file"
                 << mAutoSaveFilename;
    KMFolderMaildir::removeFile( KMKernel::localDataPath() + "autosave",
                                 mAutoSaveFilename );
    mAutoSaveFilename.clear();
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
  readConfig();
  updateAutoSave();
  rethinkFields();
}

/*
 * checks if the drafts-folder has been deleted
 * that is not nice so we set the system-drafts-folder
 */
void KMComposeWin::slotFolderRemoved( KMFolder *folder )
{
  // TODO: need to handle templates here?
  if ( (mFolder) && (folder->idString() == mFolder->idString()) ) {
    mFolder = kmkernel->draftsFolder();
    kDebug(5006) <<"restoring drafts to" << mFolder->idString();
  }
  if ( mMsg ) {
    mMsg->setParent( 0 );
  }
}

void KMComposeWin::editorFocusChanged( bool gained )
{
  mPasteQuotation->setEnabled( gained );
  mAddQuoteChars->setEnabled( gained );
  mRemQuoteChars->setEnabled( gained );
}

void KMComposeWin::slotSetAlwaysSend( bool bAlways )
{
  mAlwaysSend = bAlways;
}

void KMComposeWin::slotChangeParagStyle( QTextListFormat::Style style )
{
  toggleMarkup( true );
  mEditor->slotChangeParagStyle( style );
}

void KMComposeWin::slotAlignLeft()
{
  toggleMarkup( true );
  mEditor->slotAlignLeft();
  alignCenterAction->setChecked( false );
  alignRightAction->setChecked( false );
}

void KMComposeWin::slotAlignCenter()
{
  toggleMarkup( true );
  mEditor->slotAlignCenter();
  alignLeftAction->setChecked( false );
  alignRightAction->setChecked( false );
}

void KMComposeWin::slotAlignRight()
{
  toggleMarkup( true );
  mEditor->setAlignment( Qt::AlignRight );
  alignLeftAction->setChecked( false );
  alignCenterAction->setChecked( false );
}

void KMComposeWin::slotFontAction( const QString &font )
{
  toggleMarkup( true );
  mEditor->slotFontFamilyChanged( font );
}

void KMComposeWin::slotSizeAction( int size )
{
  toggleMarkup( true );
  mEditor->slotFontSizeChanged( size );
}

void KMComposeWin::slotTextBold( bool bold )
{
  toggleMarkup( true );
  mEditor->slotTextBold( bold );
}

void KMComposeWin::slotTextItalic( bool italic )
{
 toggleMarkup( true );
 mEditor->slotTextItalic( italic );
}

void KMComposeWin::slotTextUnder( bool under )
{
 toggleMarkup( true );
 mEditor->slotTextUnder( under );
}

void KMComposeWin::slotTextColor()
{
  // also if user cancels the dialog, html is turned on for now
  toggleMarkup( true );
  mEditor->slotTextColor();
}

void KMComposeWin::slotFormatReset()
{
  mEditor->setColor( mForeColor );
  mEditor->setFont( mSaveFont ); // fontChanged is called now
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
    textItalicAction->setEnabled( true );
  } else {
    textItalicAction->setEnabled( false );
  }

  textUnderAction->setChecked( f.underline() );

  fontAction->setFont( f.family() );
  fontSizeAction->setFontSize( f.pointSize() );
}

void KMComposeWin::slotCursorPositionChanged()
{
  switch (mEditor->alignment() )
  {
    case Qt::AlignLeft :
      alignLeftAction->setChecked( true );
      alignCenterAction->setChecked( false );
      alignRightAction->setChecked( false );
      break;
    case Qt::AlignHCenter :
      alignLeftAction->setChecked( false );
      alignCenterAction->setChecked( true );
      alignRightAction->setChecked( false );
      break;
    case Qt::AlignRight :
      alignLeftAction->setChecked( false );
      alignCenterAction->setChecked( false );
      alignRightAction->setChecked( true );
      break;
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

  ChiasmusKeySelector selectorDlg( this, i18n( "Chiasmus Encryption Key Selection" ),
                                   keys, GlobalSettings::chiasmusKey(),
                                   GlobalSettings::chiasmusOptions() );

  if ( selectorDlg.exec() != QDialog::Accepted ) {
    return;
  }

  GlobalSettings::setChiasmusOptions( selectorDlg.options() );
  GlobalSettings::setChiasmusKey( selectorDlg.key() );
  assert( !GlobalSettings::chiasmusKey().isEmpty() );
  mEncryptWithChiasmus = true;
  resetter.disable();
}

void KMComposeWin::slotEditDone(KMail::EditorWatcher * watcher)
{
  kDebug(5006) ;
  KMMessagePart *part = mEditorMap[ watcher ];
  KTemporaryFile *tf = mEditorTempFiles[ watcher ];
  mEditorMap.remove( watcher );
  mEditorTempFiles.remove( watcher );
  if ( !watcher->fileChanged() )
    return;

  tf->reset();
  QByteArray data = tf->readAll();
  part->setBodyEncodedBinary( data );

  // Find the listview item associated with this attachment and update it
  KMAtmListViewItem *listviewItem;
  foreach( listviewItem, mAtmItemList ) {
    if ( listviewItem->attachment() == part ) {
      msgPartToItem( part, listviewItem, false );
      break;
    }
  }
}

void KMComposeWin::slotAttachmentDragStarted()
{
  kDebug(5006);
  QList<QUrl> urls;
  foreach ( KMAtmListViewItem *const item, mAtmItemList ) {
    if ( item->isSelected() ) {
      KMMessagePart *msgPart = item->attachment();
      KTempDir *tempDir = new KTempDir(); // will remove the directory on destruction
      mTempDirs.append( tempDir );
      const QString fileName = tempDir->name() + '/' + msgPart->name();
      KPIMUtils::kByteArrayToFile( msgPart->bodyDecodedBinary(),
                                   fileName,
                                   false, false, false );
      QUrl url;
      url.setScheme( "file" );
      url.setPath( fileName );
      urls.append( url );
    }
  }
  if ( urls.isEmpty() )
    return;

  QDrag *drag = new QDrag( mAtmListView );
  QMimeData *mimeData = new QMimeData();
  mimeData->setUrls( urls );
  drag->setMimeData( mimeData );
  drag->exec( Qt::CopyAction );
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

