/*
 * This file is part of KMail.
 * Copyright (c) 2011-2015 Laurent Montel <montel@kde.org>
 *
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

// KMail includes
#include "job/addressvalidationjob.h"
#include "attachmentcontroller.h"
#include "messagecomposer/attachment/attachmentmodel.h"
#include "attachmentview.h"
#include "codecaction.h"
#include <messagecomposer/job/emailaddressresolvejob.h>
#include "kleo_util.h"
#include "kmcommands.h"
#include "editor/kmcomposereditor.h"
#include "kmkernel.h"
#include "settings/globalsettings.h"
#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "mailcomposeradaptor.h" // TODO port all D-Bus stuff...
#include "messageviewer/viewer/stl_util.h"
#include "messageviewer/utils/util.h"
#include "messagecore/utils/stringutil.h"
#include "messagecore/attachment/attachmentcollector.h"
#include "util.h"
#include "editor/snippetwidget.h"
#include "templatesconfiguration_kfg.h"
#include "foldercollectionmonitor.h"
#include "kernel/mailkernel.h"
#include "custommimeheader.h"
#include "pimcommon/autocorrection/widgets/lineeditwithautocorrection.h"
#include "pimcommon/translator/translatorwidget.h"
#include "pimcommon/widgets/customtoolswidget.h"
#include "warningwidgets/attachmentmissingwarning.h"
#include "job/createnewcontactjob.h"
#include "job/savedraftjob.h"
#include "warningwidgets/externaleditorwarning.h"
#include "cryptostateindicatorwidget.h"
#include "validatesendmailshortcut.h"
#include "job/saveasfilejob.h"
#include "editor/kmstorageservice.h"
#include "followupreminder/followupreminderselectdatedialog.h"
#include "followupreminder/followupremindercreatejob.h"
#include "agents/followupreminderagent/followupreminderutil.h"
#include "pimcommon/util/vcardutil.h"
#include "editor/potentialphishingemail/potentialphishingemailwarning.h"
#include "kmcomposerglobalaction.h"

#include "libkdepim/progresswidget/statusbarprogresswidget.h"
#include "libkdepim/progresswidget/progressstatusbarwidget.h"

#include "pimcommon/util/editorutil.h"
#include "pimcommon/storageservice/storageservicemanager.h"
#include "pimcommon/storageservice/storageserviceprogressmanager.h"

#include "messagecomposer/utils/util.h"

#include <kabc/vcardconverter.h>
#include "agents/sendlateragent/sendlaterutil.h"
#include "agents/sendlateragent/sendlaterdialog.h"
#include "agents/sendlateragent/sendlaterinfo.h"

// KDEPIM includes
#include <libkpgp/kpgpblock.h>
#include <libkleo/ui/progressdialog.h>
#include <libkleo/ui/keyselectiondialog.h>
#include "kleo/cryptobackendfactory.h"
#include "kleo/exportjob.h"
#include "kleo/specialjob.h"
#include <messageviewer/viewer/objecttreeemptysource.h>

#ifndef QT_NO_CURSOR
#include <messageviewer/utils/kcursorsaver.h>
#endif

#include <messageviewer/viewer/objecttreeparser.h>
#include <messageviewer/viewer/nodehelper.h>
#include <messageviewer/settings/globalsettings.h>
#include <messagecomposer/composer/composer.h>
#include <messagecomposer/part/globalpart.h>
#include <messagecomposer/part/infopart.h>
#include <messagecomposer/part/textpart.h>
#include <settings/messagecomposersettings.h>
#include <messagecomposer/helper/messagehelper.h>
#include <messagecomposer/composer/signaturecontroller.h>
#include <messagecomposer/job/inserttextfilejob.h>
#include <messagecomposer/composer/composerlineedit.h>
#include <messagecore/attachment/attachmentpart.h>
#include "messagecore/settings/globalsettings.h"
#include <templateparser/templateparser.h>
#include <templatesconfiguration.h>
#include "messagecore/helpers/nodehelper.h"
#include <akonadi/kmime/messagestatus.h>
#include "messagecore/helpers/messagehelpers.h"
#include "mailcommon/folder/folderrequester.h"
#include "mailcommon/folder/foldercollection.h"

#include "widgets/statusbarlabeltoggledstate.h"

// LIBKDEPIM includes
#include <libkdepim/addressline/recentaddress/recentaddresses.h>

// KDEPIMLIBS includes
#include <akonadi/changerecorder.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/entitymimetypefiltermodel.h>
#include <akonadi/kmime/messageflags.h>
#include <akonadi/itemfetchjob.h>
#include <kpimutils/email.h>
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identitycombo.h>
#include <kpimidentities/identity.h>
#include <kpimidentities/signature.h>
#include <mailtransport/transportcombobox.h>
#include <mailtransport/transportmanager.h>
#include <mailtransport/transport.h>
#include <kmime/kmime_codecs.h>
#include <kmime/kmime_message.h>
#include <kpimtextedit/selectspecialchar.h>


// KDELIBS includes
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kapplication.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <kdescendantsproxymodel.h>
#include <kedittoolbar.h>
#include <kinputdialog.h>
#include <kmenu.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <krecentfilesaction.h>
#include <kshortcutsdialog.h>
#include <kstandarddirs.h>
#include <kstandardshortcut.h>
#include <kstatusbar.h>
#include <ktempdir.h>
#include <ktoggleaction.h>
#include <ktoolbar.h>
#include <ktoolinvocation.h>
#include <sonnet/dictionarycombobox.h>
#include <krun.h>
#include <KIO/JobUiDelegate>
#include <KPrintPreview>
#include <KFileDialog>

// Qt includes
#include <QClipboard>
#include <QSplitter>
#include <QMimeData>
#include <QTextDocumentWriter>

// System includes
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <widgets/splittercollapser.h>
#include <Akonadi/Contact/ContactGroupExpandJob>
#include <editor/potentialphishingemail/potentialphishingemailjob.h>

using Sonnet::DictionaryComboBox;
using MailTransport::TransportManager;
using MailTransport::Transport;
using KPIM::RecentAddresses;
using MessageComposer::KMeditor;

KMail::Composer *KMail::makeComposer( const KMime::Message::Ptr &msg, bool lastSignState, bool lastEncryptState, Composer::TemplateContext context,
                                      uint identity, const QString & textSelection,
                                      const QString & customTemplate ) {
    return KMComposeWin::create( msg, lastSignState, lastEncryptState, context, identity, textSelection, customTemplate );
}

KMail::Composer *KMComposeWin::create( const KMime::Message::Ptr &msg, bool lastSignState, bool lastEncryptState, Composer::TemplateContext context,
                                       uint identity, const QString & textSelection,
                                       const QString & customTemplate ) {
    return new KMComposeWin( msg, lastSignState, lastEncryptState, context, identity, textSelection, customTemplate );
}

int KMComposeWin::s_composerNumber = 0;


KMComposeWin::KMComposeWin( const KMime::Message::Ptr &aMsg, bool lastSignState, bool lastEncryptState, Composer::TemplateContext context, uint id,
                            const QString & textSelection, const QString & customTemplate )
    : KMail::Composer( "kmail-composer#" ),
      mDone( false ),
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
      mDictionaryAction( 0 ), mSnippetAction( 0 ), mTranslateAction(0),
      mAppendSignature( 0 ), mPrependSignature( 0 ), mInsertSignatureAtCursorPosition( 0 ),
      mGenerateShortenUrl( 0 ),
      mCodecAction( 0 ),
      mCryptoModuleAction( 0 ),
      mFindText( 0 ),
      mFindNextText( 0 ),
      mReplaceText( 0 ),
      mSelectAll( 0 ),
      mDummyComposer( 0 ),
      mLabelWidth( 0 ),
      mComposerBase( 0 ),
      mSelectSpecialChar( 0 ),
      m_verifyMissingAttachment( 0 ),
      mPreventFccOverwrite( false ),
      mCheckForForgottenAttachments( true ),
      mIgnoreStickyFields( false ),
      mWasModified( false ),
      mCryptoStateIndicatorWidget(0),
      mStorageService(new KMStorageService(this, this)),
      mSendNowByShortcutUsed(false),
      mFollowUpToggleAction(0),
      mStatusBarLabelToggledOverrideMode(0),
      mStatusBarLabelSpellCheckingChangeMode(0)
{
    mGlobalAction = new KMComposerGlobalAction(this, this);
    mComposerBase = new MessageComposer::ComposerViewBase( this, this );
    mComposerBase->setIdentityManager( kmkernel->identityManager() );

    connect( mComposerBase, SIGNAL(disableHtml(MessageComposer::ComposerViewBase::Confirmation)),
             this, SLOT(disableHtml(MessageComposer::ComposerViewBase::Confirmation)) );

    connect( mComposerBase, SIGNAL(enableHtml()),
             this, SLOT(enableHtml()) );
    connect( mComposerBase, SIGNAL(failed(QString,MessageComposer::ComposerViewBase::FailedType)), this, SLOT(slotSendFailed(QString,MessageComposer::ComposerViewBase::FailedType)) );
    connect( mComposerBase, SIGNAL(sentSuccessfully(QString)), this, SLOT(slotSendSuccessful(QString)) );
    connect( mComposerBase, SIGNAL(modified(bool)), this, SLOT(setModified(bool)) );

    (void) new MailcomposerAdaptor( this );
    mdbusObjectPath = QLatin1String("/Composer_") + QString::number( ++s_composerNumber );
    QDBusConnection::sessionBus().registerObject( mdbusObjectPath, this );

    MessageComposer::SignatureController* sigController = new MessageComposer::SignatureController( this );
    connect( sigController, SIGNAL(enableHtml()), SLOT(enableHtml()) );
    mComposerBase->setSignatureController( sigController );

    if ( kmkernel->xmlGuiInstance().isValid() ) {
        setComponentData( kmkernel->xmlGuiInstance() );
    }
    mMainWidget = new QWidget( this );
    // splitter between the headers area and the actual editor
    mHeadersToEditorSplitter = new QSplitter( Qt::Vertical, mMainWidget );
    mHeadersToEditorSplitter->setObjectName( QLatin1String("mHeadersToEditorSplitter") );
    mHeadersToEditorSplitter->setChildrenCollapsible( false );
    mHeadersArea = new QWidget( mHeadersToEditorSplitter );
    mHeadersArea->setSizePolicy( mHeadersToEditorSplitter->sizePolicy().horizontalPolicy(),
                                 QSizePolicy::Expanding );
    mHeadersToEditorSplitter->addWidget( mHeadersArea );
    QList<int> defaultSizes;
    defaultSizes << 0;
    mHeadersToEditorSplitter->setSizes( defaultSizes );



    QVBoxLayout *v = new QVBoxLayout( mMainWidget );
    v->setMargin(0);
    v->addWidget( mHeadersToEditorSplitter );
    KPIMIdentities::IdentityCombo* identity = new KPIMIdentities::IdentityCombo( kmkernel->identityManager(),
                                                                                 mHeadersArea );
    identity->setToolTip( i18n( "Select an identity for this message" ) );
    mComposerBase->setIdentityCombo( identity );

    sigController->setIdentityCombo( identity );
    sigController->suspend(); // we have to do identity change tracking ourselves due to the template code

    Sonnet::DictionaryComboBox *dictionaryCombo = new DictionaryComboBox( mHeadersArea );
    dictionaryCombo->setToolTip( i18n( "Select the dictionary to use when spell-checking this message" ) );
    mComposerBase->setDictionary(dictionaryCombo);

    mFccFolder = new MailCommon::FolderRequester( mHeadersArea );
    mFccFolder->setNotAllowToCreateNewFolder( true );
    mFccFolder->setMustBeReadWrite( true );


    mFccFolder->setToolTip( i18n( "Select the sent-mail folder where a copy of this message will be saved" ) );
    connect( mFccFolder, SIGNAL(folderChanged(Akonadi::Collection)),
             this, SLOT(slotFccFolderChanged(Akonadi::Collection)) );

    MailTransport::TransportComboBox* transport = new MailTransport::TransportComboBox( mHeadersArea );
    transport->setToolTip( i18n( "Select the outgoing account to use for sending this message" ) );
    mComposerBase->setTransportCombo( transport );
    connect(transport, SIGNAL(activated(int)), this, SLOT(slotTransportChanged()));

    mEdtFrom = new MessageComposer::ComposerLineEdit( false, mHeadersArea );
    mEdtFrom->setObjectName( QLatin1String("fromLine") );
    mEdtFrom->setRecentAddressConfig( MessageComposer::MessageComposerSettings::self()->config() );
    mEdtFrom->setToolTip( i18n( "Set the \"From:\" email address for this message" ) );
    mEdtReplyTo = new MessageComposer::ComposerLineEdit( true, mHeadersArea );
    mEdtReplyTo->setObjectName( QLatin1String("replyToLine") );
    mEdtReplyTo->setRecentAddressConfig( MessageComposer::MessageComposerSettings::self()->config() );
    mEdtReplyTo->setToolTip( i18n( "Set the \"Reply-To:\" email address for this message" ) );
    connect( mEdtReplyTo, SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
             SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)) );

    MessageComposer::RecipientsEditor* recipientsEditor = new MessageComposer::RecipientsEditor( mHeadersArea );
    recipientsEditor->setRecentAddressConfig( MessageComposer::MessageComposerSettings::self()->config() );
    connect( recipientsEditor,
             SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
             SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)) );
    connect( recipientsEditor, SIGNAL(sizeHintChanged()), SLOT(recipientEditorSizeHintChanged()) );
    mComposerBase->setRecipientsEditor( recipientsEditor );

    mEdtSubject = new PimCommon::LineEditWithAutoCorrection( mHeadersArea, QLatin1String( "kmail2rc" ) );
    mEdtSubject->setActivateLanguageMenu(false);
    mEdtSubject->setToolTip( i18n( "Set a subject for this message" ) );
    mEdtSubject->setAutocorrection(KMKernel::self()->composerAutoCorrection());
    mLblIdentity = new QLabel( i18n("&Identity:"), mHeadersArea );
    mDictionaryLabel = new QLabel( i18n("&Dictionary:"), mHeadersArea );
    mLblFcc = new QLabel( i18n("&Sent-Mail folder:"), mHeadersArea );
    mLblTransport = new QLabel( i18n("&Mail transport:"), mHeadersArea );
    mLblFrom = new QLabel( i18nc("sender address field", "&From:"), mHeadersArea );
    mLblReplyTo = new QLabel( i18n("&Reply to:"), mHeadersArea );
    mLblSubject = new QLabel( i18nc("@label:textbox Subject of email.", "S&ubject:"), mHeadersArea );
    QString sticky = i18nc("@option:check Sticky identity.", "Sticky");
    mBtnIdentity = new QCheckBox( sticky, mHeadersArea );
    mBtnIdentity->setToolTip( i18n( "Use the selected value as your identity for future messages" ) );
    mBtnFcc = new QCheckBox( sticky, mHeadersArea );
    mBtnFcc->setToolTip( i18n( "Use the selected value as your sent-mail folder for future messages" ) );
    mBtnTransport = new QCheckBox( sticky, mHeadersArea );
    mBtnTransport->setToolTip( i18n( "Use the selected value as your outgoing account for future messages" ) );
    mBtnDictionary = new QCheckBox( sticky, mHeadersArea );
    mBtnDictionary->setToolTip( i18n( "Use the selected value as your dictionary for future messages" ) );

    mShowHeaders = GlobalSettings::self()->headers();
    mDone = false;
    mGrid = 0;
    mFixedFontAction = 0;
    // the attachment view is separated from the editor by a splitter
    mSplitter = new QSplitter( Qt::Vertical, mMainWidget );
    mSplitter->setObjectName( QLatin1String("mSplitter") );
    mSplitter->setChildrenCollapsible( false );
    mSnippetSplitter = new QSplitter( Qt::Horizontal, mSplitter );
    mSnippetSplitter->setObjectName( QLatin1String("mSnippetSplitter") );
    mSplitter->addWidget( mSnippetSplitter );

    QWidget *editorAndCryptoStateIndicators = new QWidget( mSplitter );
    mCryptoStateIndicatorWidget = new CryptoStateIndicatorWidget;
    mCryptoStateIndicatorWidget->setShowAlwaysIndicator(GlobalSettings::self()->showCryptoLabelIndicator());

    QVBoxLayout *vbox = new QVBoxLayout(editorAndCryptoStateIndicators);
    vbox->setMargin(0);
    KMComposerEditor* editor = new KMComposerEditor( this, mCryptoStateIndicatorWidget );

    connect( editor, SIGNAL(textChanged()),
             this, SLOT(slotEditorTextChanged()) );
    mComposerBase->setEditor( editor );
    vbox->addWidget( mCryptoStateIndicatorWidget );
    vbox->addWidget( editor );

    mSnippetSplitter->insertWidget( 0, editorAndCryptoStateIndicators );
    mSnippetSplitter->setOpaqueResize( true );
    sigController->setEditor( editor );

    mHeadersToEditorSplitter->addWidget( mSplitter );
    editor->setAcceptDrops( true );
    connect(sigController, SIGNAL(signatureAdded()), mComposerBase->editor(), SLOT(startExternalEditor()));

    connect( mComposerBase->dictionary(), SIGNAL(dictionaryChanged(QString)),
             this, SLOT(slotSpellCheckingLanguage(QString)) );

    connect( editor, SIGNAL(languageChanged(QString)),
             this, SLOT(slotLanguageChanged(QString)) );
    connect( editor, SIGNAL(spellCheckStatus(QString)),
             this, SLOT(slotSpellCheckingStatus(QString)) );
    connect( editor, SIGNAL(insertModeChanged()),
             this, SLOT(slotOverwriteModeChanged()) );
    connect(editor,SIGNAL(spellCheckingFinished()),this,SLOT(slotCheckSendNow()));
    mSnippetWidget = new SnippetWidget( editor, actionCollection(), mSnippetSplitter );
    mSnippetWidget->setVisible( GlobalSettings::self()->showSnippetManager() );
    mSnippetSplitter->addWidget( mSnippetWidget );
    mSnippetSplitter->setCollapsible( 0, false );
    mSnippetSplitterCollapser = new PimCommon::SplitterCollapser(mSnippetWidget, mSnippetSplitter);
    mSnippetSplitterCollapser->setVisible( GlobalSettings::self()->showSnippetManager() );

    mSplitter->setOpaqueResize( true );

    mBtnIdentity->setWhatsThis( GlobalSettings::self()->stickyIdentityItem()->whatsThis() );
    mBtnFcc->setWhatsThis( GlobalSettings::self()->stickyFccItem()->whatsThis() );
    mBtnTransport->setWhatsThis( GlobalSettings::self()->stickyTransportItem()->whatsThis() );
    mBtnDictionary->setWhatsThis( GlobalSettings::self()->stickyDictionaryItem()->whatsThis() );

    setCaption( i18n("Composer") );
    setMinimumSize( 200, 200 );

    mBtnIdentity->setFocusPolicy( Qt::NoFocus );
    mBtnFcc->setFocusPolicy( Qt::NoFocus );
    mBtnTransport->setFocusPolicy( Qt::NoFocus );
    mBtnDictionary->setFocusPolicy( Qt::NoFocus );

    mCustomToolsWidget = new PimCommon::CustomToolsWidget(this);
    mSplitter->addWidget(mCustomToolsWidget);
    connect(mCustomToolsWidget, SIGNAL(insertShortUrl(QString)), this, SLOT(slotInsertShortUrl(QString)));

    MessageComposer::AttachmentModel* attachmentModel = new MessageComposer::AttachmentModel( this );
    KMail::AttachmentView *attachmentView = new KMail::AttachmentView( attachmentModel, mSplitter );
    attachmentView->hideIfEmpty();
    connect(attachmentView,SIGNAL(modified(bool)),SLOT(setModified(bool)));
    KMail::AttachmentController* attachmentController = new KMail::AttachmentController( attachmentModel, attachmentView, this );

    mComposerBase->setAttachmentModel( attachmentModel );
    mComposerBase->setAttachmentController( attachmentController );

    mAttachmentMissing = new AttachmentMissingWarning(this);
    connect(mAttachmentMissing, SIGNAL(attachMissingFile()), this, SLOT(slotAttachMissingFile()));
    connect(mAttachmentMissing, SIGNAL(explicitClosedMissingAttachment()), this, SLOT(slotExplicitClosedMissingAttachment()));
    v->addWidget(mAttachmentMissing);

    mPotentialPhishingEmailWarning = new PotentialPhishingEmailWarning(this);
    connect(mPotentialPhishingEmailWarning, SIGNAL(sendNow()), this, SLOT(slotCheckSendNowStep2()));
    v->addWidget(mPotentialPhishingEmailWarning);

    if (GlobalSettings::self()->showForgottenAttachmentWarning()) {
        m_verifyMissingAttachment = new QTimer(this);
        m_verifyMissingAttachment->setSingleShot(true);
        m_verifyMissingAttachment->setInterval(1000*5);
        connect( m_verifyMissingAttachment, SIGNAL(timeout()), this, SLOT(slotVerifyMissingAttachmentTimeout()) );
    }
    connect( attachmentController, SIGNAL(fileAttached()), mAttachmentMissing, SLOT(slotFileAttached()) );

    mExternalEditorWarning = new ExternalEditorWarning(this);
    v->addWidget(mExternalEditorWarning);

    readConfig();
    setupStatusBar(attachmentView->widget());
    setupActions();
    setupEditor();
    rethinkFields();
    updateSignatureAndEncryptionStateIndicators();

    applyMainWindowSettings( KMKernel::self()->config()->group( "Composer") );

    connect( mEdtSubject, SIGNAL(textChanged()),
             SLOT(slotUpdWinTitle()) );
    connect( identity, SIGNAL(identityChanged(uint)),
             SLOT(slotIdentityChanged(uint)) );
    connect( kmkernel->identityManager(), SIGNAL(changed(uint)),
             SLOT(slotIdentityChanged(uint)) );

    connect( mEdtFrom, SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
             SLOT(slotCompletionModeChanged(KGlobalSettings::Completion)) );
    connect( kmkernel->folderCollectionMonitor(), SIGNAL(collectionRemoved(Akonadi::Collection)),
             SLOT(slotFolderRemoved(Akonadi::Collection)) );
    connect( kmkernel, SIGNAL(configChanged()),
             this, SLOT(slotConfigChanged()) );

    mMainWidget->resize( 480, 510 );
    setCentralWidget( mMainWidget );

    if ( GlobalSettings::self()->useHtmlMarkup() )
        enableHtml();
    else
        disableHtml( MessageComposer::ComposerViewBase::LetUserConfirm );

    if ( GlobalSettings::self()->useExternalEditor() ) {
        editor->setUseExternalEditor( true );
        editor->setExternalEditorPath( GlobalSettings::self()->externalEditor() );
    }

    if ( aMsg ) {
        setMessage( aMsg, lastSignState, lastEncryptState );
    }

    mComposerBase->recipientsEditor()->setFocus();
    editor->updateActionStates(); // set toolbar buttons to correct values

    mDone = true;

    mDummyComposer = new MessageComposer::Composer( this );
    mDummyComposer->globalPart()->setParentWidgetForGui( this );

    connect(mStorageService, SIGNAL(insertShareLink(QString)), this, SLOT(slotShareLinkDone(QString)));
}


KMComposeWin::~KMComposeWin()
{
    writeConfig();

    // When we have a collection set, store the message back to that collection.
    // Note that when we save the message or sent it, mFolder is set back to 0.
    // So this for example kicks in when opening a draft and then closing the window.
    if ( mFolder.isValid() && mMsg && isModified() ) {
        SaveDraftJob *saveDraftJob = new SaveDraftJob(mMsg, mFolder);
        saveDraftJob->start();
    }

    delete mComposerBase;
}


void KMComposeWin::slotSpellCheckingLanguage(const QString& language)
{
    mComposerBase->editor()->setSpellCheckingLanguage(language );
    mEdtSubject->setSpellCheckingLanguage(language );
}

QString KMComposeWin::dbusObjectPath() const
{
    return mdbusObjectPath;
}

void KMComposeWin::slotEditorTextChanged()
{
    const bool textIsNotEmpty = !mComposerBase->editor()->document()->isEmpty();
    mFindText->setEnabled( textIsNotEmpty );
    mFindNextText->setEnabled( textIsNotEmpty );
    mReplaceText->setEnabled( textIsNotEmpty );
    mSelectAll->setEnabled( textIsNotEmpty );
    if (m_verifyMissingAttachment && !m_verifyMissingAttachment->isActive()) {
        m_verifyMissingAttachment->start();
    }
}


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


void KMComposeWin::addAttachmentsAndSend( const KUrl::List &urls, const QString &comment, int how )
{
    kDebug() << "addAttachment and sending!";
    const int nbUrl = urls.count();
    for ( int i =0; i < nbUrl; ++i ) {
        mComposerBase->addAttachment( urls.at(i), comment, true );
    }

    send( how );
}


void KMComposeWin::addAttachment( const KUrl &url, const QString &comment )
{
    mComposerBase->addAttachment( url, comment, false );
}


void KMComposeWin::addAttachment( const QString& name,
                                  KMime::Headers::contentEncoding cte,
                                  const QString& charset,
                                  const QByteArray& data,
                                  const QByteArray& mimeType )
{
    Q_UNUSED( cte );
    mComposerBase->addAttachment( name, name, charset, data, mimeType );
}


void KMComposeWin::readConfig( bool reload /* = false */ )
{
    mBtnIdentity->setChecked( GlobalSettings::self()->stickyIdentity() );
    if (mBtnIdentity->isChecked()) {
        mId = ( GlobalSettings::self()->previousIdentity() != 0 ) ?
                    GlobalSettings::self()->previousIdentity() : mId;
    }
    mBtnFcc->setChecked( GlobalSettings::self()->stickyFcc() );
    mBtnTransport->setChecked( GlobalSettings::self()->stickyTransport() );
    const int currentTransport = GlobalSettings::self()->currentTransport().isEmpty() ? -1 : GlobalSettings::self()->currentTransport().toInt();
    mBtnDictionary->setChecked( GlobalSettings::self()->stickyDictionary() );

    mEdtFrom->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
    mComposerBase->recipientsEditor()->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );
    mEdtReplyTo->setCompletionMode( (KGlobalSettings::Completion)GlobalSettings::self()->completionMode() );

    if ( MessageCore::GlobalSettings::self()->useDefaultFonts() ) {
        mBodyFont = KGlobalSettings::generalFont();
        mFixedFont = KGlobalSettings::fixedFont();
    } else {
        mBodyFont = GlobalSettings::self()->composerFont();
        mFixedFont = MessageViewer::GlobalSettings::self()->fixedFont();
    }

    slotUpdateFont();
    mEdtFrom->setFont( mBodyFont );
    mEdtReplyTo->setFont( mBodyFont );
    mEdtSubject->setFont( mBodyFont );

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

    mComposerBase->identityCombo()->setCurrentIdentity( mId );
    kDebug() << mComposerBase->identityCombo()->currentIdentityName();
    const KPIMIdentities::Identity & ident =
            kmkernel->identityManager()->identityForUoid( mId );

    if ( mBtnTransport->isChecked() && currentTransport != -1 ) {
        const Transport *transport = TransportManager::self()->transportById( currentTransport );
        if ( transport )
            mComposerBase->transportComboBox()->setCurrentTransport( transport->id() );
    }

    mComposerBase->setAutoSaveInterval( GlobalSettings::self()->autosaveInterval() * 1000 * 60 );


    if ( mBtnDictionary->isChecked() ) {
        mComposerBase->dictionary()->setCurrentByDictionaryName( GlobalSettings::self()->previousDictionary() );
    } else {
        mComposerBase->dictionary()->setCurrentByDictionaryName( ident.dictionary() );
    }

    QString fccName;
    if ( mBtnFcc->isChecked() ) {
        fccName = GlobalSettings::self()->previousFcc();
    } else if ( !ident.fcc().isEmpty() ) {
        fccName = ident.fcc();
    }
    setFcc( fccName );
}


void KMComposeWin::writeConfig( void )
{
    GlobalSettings::self()->setHeaders( mShowHeaders );
    GlobalSettings::self()->setStickyFcc( mBtnFcc->isChecked() );
    if ( !mIgnoreStickyFields ) {
        GlobalSettings::self()->setCurrentTransport( mComposerBase->transportComboBox()->currentText() );
        GlobalSettings::self()->setStickyTransport( mBtnTransport->isChecked() );
        GlobalSettings::self()->setStickyDictionary( mBtnDictionary->isChecked() );
        GlobalSettings::self()->setStickyIdentity( mBtnIdentity->isChecked() );
        GlobalSettings::self()->setPreviousIdentity( mComposerBase->identityCombo()->currentIdentity() );
    }
    GlobalSettings::self()->setPreviousFcc( QString::number(mFccFolder->collection().id()) );
    GlobalSettings::self()->setPreviousDictionary( mComposerBase->dictionary()->currentDictionaryName() );
    GlobalSettings::self()->setAutoSpellChecking(
                mAutoSpellCheckingAction->isChecked() );
    MessageViewer::GlobalSettings::self()->setUseFixedFont( mFixedFontAction->isChecked() );
    if ( !mForceDisableHtml )
        GlobalSettings::self()->setUseHtmlMarkup( mComposerBase->editor()->textMode() == KMeditor::Rich );
    GlobalSettings::self()->setComposerSize( size() );
    GlobalSettings::self()->setShowSnippetManager( mSnippetAction->isChecked() );

    saveMainWindowSettings( KMKernel::self()->config()->group( "Composer" ) );
    if ( mSnippetAction->isChecked() )
        GlobalSettings::setSnippetSplitterPosition( mSnippetSplitter->sizes() );

    // make sure config changes are written to disk, cf. bug 127538
    KMKernel::self()->slotSyncConfig();
}

MessageComposer::Composer* KMComposeWin::createSimpleComposer()
{
    QList< QByteArray > charsets = mCodecAction->mimeCharsets();
    if( !mOriginalPreferredCharset.isEmpty() ) {
        charsets.insert( 0, mOriginalPreferredCharset );
    }
    mComposerBase->setFrom( from() );
    mComposerBase->setReplyTo( replyTo() );
    mComposerBase->setSubject( subject() );
    mComposerBase->setCharsets( charsets );
    return mComposerBase->createSimpleComposer();
}

bool KMComposeWin::canSignEncryptAttachments() const
{
    return cryptoMessageFormat() != Kleo::InlineOpenPGPFormat;
}


void KMComposeWin::slotUpdateView( void )
{
    if ( !mDone ) {
        return; // otherwise called from rethinkFields during the construction
        // which is not the intended behavior
    }

    //This sucks awfully, but no, I cannot get an activated(int id) from
    // actionContainer()
    KToggleAction *act = ::qobject_cast<KToggleAction *>( sender() );
    if ( !act ) {
        return;
    }
    int id;

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

    w->setBuddy( mComposerBase->editor() ); // set dummy so we don't calculate width of '&' for this label.
    w->adjustSize();
    w->show();
    return qMax( width, w->sizeHint().width() );
}

void KMComposeWin::rethinkFields( bool fromSlot )
{
    //This sucks even more but again no ids. sorry (sven)
    int mask, row;
    long showHeaders;

    if ( mShowHeaders < 0 ) {
        showHeaders = HDR_ALL;
    } else {
        showHeaders = mShowHeaders;
    }

    for ( mask=1, mNumHeaders=0; mask<=showHeaders; mask<<=1 ) {
        if ( ( showHeaders & mask ) != 0 ) {
            ++mNumHeaders;
        }
    }

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

    mLabelWidth = mComposerBase->recipientsEditor()->setFirstColumnWidth( 0 );
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
    rethinkHeaderLine( showHeaders,HDR_IDENTITY, row, mLblIdentity, mComposerBase->identityCombo(),
                       mBtnIdentity );

    if ( !fromSlot ) {
        mDictionaryAction->setChecked( abs( mShowHeaders )&HDR_DICTIONARY );
    }
    rethinkHeaderLine( showHeaders,HDR_DICTIONARY, row, mDictionaryLabel,
                       mComposerBase->dictionary(), mBtnDictionary );

    if ( !fromSlot ) {
        mFccAction->setChecked( abs( mShowHeaders )&HDR_FCC );
    }
    rethinkHeaderLine( showHeaders,HDR_FCC, row, mLblFcc, mFccFolder, mBtnFcc );

    if ( !fromSlot ) {
        mTransportAction->setChecked( abs( mShowHeaders )&HDR_TRANSPORT );
    }
    rethinkHeaderLine( showHeaders,HDR_TRANSPORT, row, mLblTransport, mComposerBase->transportComboBox(),
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

    mGrid->addWidget( mComposerBase->recipientsEditor(), row, 0, 1, 3 );
    ++row;
    if ( showHeaders & HDR_REPLY_TO ) {
        connect( mEdtReplyTo, SIGNAL(focusDown()), mComposerBase->recipientsEditor(),
                 SLOT(setFocusTop()) );
        connect( mComposerBase->recipientsEditor(), SIGNAL(focusUp()), mEdtReplyTo,
                 SLOT(setFocus()) );
    } else {
        connect( mEdtFrom, SIGNAL(focusDown()), mComposerBase->recipientsEditor(),
                 SLOT(setFocusTop()) );
        connect( mComposerBase->recipientsEditor(), SIGNAL(focusUp()), mEdtFrom,
                 SLOT(setFocus()) );
    }

    connect( mComposerBase->recipientsEditor(), SIGNAL(focusDown()), mEdtSubject,
             SLOT(setFocus()) );
    connect( mEdtSubject, SIGNAL(focusUp()), mComposerBase->recipientsEditor(),
             SLOT(setFocusBottom()) );

    prevFocus = mComposerBase->recipientsEditor();

    if ( !fromSlot ) {
        mSubjectAction->setChecked( abs( mShowHeaders )&HDR_SUBJECT );
    }
    rethinkHeaderLine(showHeaders,HDR_SUBJECT, row, mLblSubject, mEdtSubject );
    connectFocusMoving( mEdtSubject, mComposerBase->editor() );

    assert( row <= mNumHeaders + 1 );


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
    mComposerBase->recipientsEditor()->setFirstColumnWidth( mLabelWidth );
}

QWidget *KMComposeWin::connectFocusMoving( QWidget *prev, QWidget *next )
{
    connect( prev, SIGNAL(focusDown()), next, SLOT(setFocus()) );
    connect( next, SIGNAL(focusUp()), prev, SLOT(setFocus()) );

    return next;
}


void KMComposeWin::rethinkHeaderLine( int aValue, int aMask, int &aRow,
                                      QLabel *aLbl, QWidget *aEdt,
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


void KMComposeWin::rethinkHeaderLine( int aValue, int aMask, int &aRow,
                                      QLabel *aLbl, QWidget *aCbx,
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


void KMComposeWin::applyTemplate( uint uoid, uint uOldId )
{
    const KPIMIdentities::Identity &ident = kmkernel->identityManager()->identityForUoid( uoid );
    if ( ident.isNull() )
        return;
    KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Templates", mMsg.get(), ident.templates(), "utf-8" );
    mMsg->setHeader( header );

    TemplateParser::TemplateParser::Mode mode;
    switch ( mContext ) {
    case New:
        mode = TemplateParser::TemplateParser::NewMessage;
        break;
    case Reply:
        mode = TemplateParser::TemplateParser::Reply;
        break;
    case ReplyToAll:
        mode = TemplateParser::TemplateParser::ReplyAll;
        break;
    case Forward:
        mode = TemplateParser::TemplateParser::Forward;
        break;
    default:
        return;
    }

    if ( mode == TemplateParser::TemplateParser::NewMessage ) {
        TemplateParser::TemplateParser parser( mMsg, mode );
        parser.setSelection( mTextSelection );
        parser.setAllowDecryption( MessageViewer::GlobalSettings::self()->automaticDecrypt() );
        parser.setIdentityManager( KMKernel::self()->identityManager() );
        if ( !mCustomTemplate.isEmpty() )
            parser.process( mCustomTemplate, mMsg, mCollectionForNewMessage );
        else
            parser.processWithIdentity( uoid, mMsg, mCollectionForNewMessage );
        mComposerBase->updateTemplate( mMsg );
        updateSignature(uoid, uOldId);
        return;
    }

    if ( mMsg->headerByType( "X-KMail-Link-Message" ) ) {
        Akonadi::Item::List items;
        foreach( const QString& serNumStr, mMsg->headerByType( "X-KMail-Link-Message" )->asUnicodeString().split( QLatin1Char(',') ) )
            items << Akonadi::Item( serNumStr.toLongLong() );


        Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( items, this );
        job->fetchScope().fetchFullPayload( true );
        job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
        job->setProperty( "mode", (int)mode );
        job->setProperty( "uoid", uoid );
        job->setProperty( "uOldid", uOldId );
        connect( job, SIGNAL(result(KJob*)), SLOT(slotDelayedApplyTemplate(KJob*)) );
    }
}

void KMComposeWin::slotDelayedApplyTemplate( KJob *job )
{
    const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
    const Akonadi::Item::List items = fetchJob->items();

    const TemplateParser::TemplateParser::Mode mode = static_cast<TemplateParser::TemplateParser::Mode>( fetchJob->property( "mode" ).toInt() );
    const uint uoid = fetchJob->property( "uoid" ).toUInt();
    const uint uOldId = fetchJob->property( "uOldid" ).toUInt();

    TemplateParser::TemplateParser parser( mMsg, mode );
    parser.setSelection( mTextSelection );
    parser.setAllowDecryption( MessageViewer::GlobalSettings::self()->automaticDecrypt() );
    parser.setWordWrap( MessageComposer::MessageComposerSettings::self()->wordWrap(), MessageComposer::MessageComposerSettings::self()->lineWrapWidth() );
    parser.setIdentityManager( KMKernel::self()->identityManager() );
    foreach ( const Akonadi::Item &item, items ) {
        if ( !mCustomTemplate.isEmpty() )
            parser.process( mCustomTemplate, MessageCore::Util::message( item ) );
        else
            parser.processWithIdentity( uoid, MessageCore::Util::message( item ) );
    }
    mComposerBase->updateTemplate( mMsg );
    updateSignature(uoid, uOldId);
}

void KMComposeWin::updateSignature(uint uoid, uint uOldId)
{
    const KPIMIdentities::Identity &ident = kmkernel->identityManager()->identityForUoid( uoid );
    const KPIMIdentities::Identity &oldIdentity = kmkernel->identityManager()->identityForUoid( uOldId  );
    mComposerBase->identityChanged( ident, oldIdentity, true );
}

void KMComposeWin::setCollectionForNewMessage( const Akonadi::Collection& folder)
{
    mCollectionForNewMessage = folder;
}

void KMComposeWin::setQuotePrefix( uint uoid )
{
    QString quotePrefix = mMsg->headerByType( "X-KMail-QuotePrefix" ) ? mMsg->headerByType( "X-KMail-QuotePrefix" )->asUnicodeString() : QString();
    if ( quotePrefix.isEmpty() ) {
        // no quote prefix header, set quote prefix according in identity
        // TODO port templates to ComposerViewBase

        if ( mCustomTemplate.isEmpty() ) {
            const KPIMIdentities::Identity &identity = kmkernel->identityManager()->identityForUoidOrDefault( uoid );
            // Get quote prefix from template
            // ( custom templates don't specify custom quotes prefixes )
            TemplateParser::Templates quoteTemplate(
                        TemplateParser::TemplatesConfiguration::configIdString( identity.uoid() ) );
            quotePrefix = quoteTemplate.quoteString();
        }
    }
    mComposerBase->editor()->setQuotePrefixName( MessageCore::StringUtil::formatString( quotePrefix,
                                                                                        mMsg->from()->asUnicodeString() ) );
}


void KMComposeWin::getTransportMenu()
{
    mActNowMenu->clear();
    mActLaterMenu->clear();
    const QList<Transport*> transports = TransportManager::self()->transports();
    foreach ( Transport *transport, transports ) {
        const QString name = transport->name().replace( QLatin1Char('&'), QLatin1String("&&") );
        QAction *action1 = new QAction( name, mActNowMenu );
        QAction *action2 = new QAction( name, mActLaterMenu );
        action1->setData( transport->id() );
        action2->setData( transport->id() );
        mActNowMenu->addAction( action1 );
        mActLaterMenu->addAction( action2 );
    }
}


void KMComposeWin::setupActions( void )
{
    KActionMenu *actActionNowMenu, *actActionLaterMenu;

    if ( MessageComposer::MessageComposerSettings::self()->sendImmediate() ) {
        //default = send now, alternative = queue
        KAction *action = new KAction(KIcon(QLatin1String("mail-send")), i18n("&Send Mail"), this);
        actionCollection()->addAction(QLatin1String("send_mail_default"), action );
        connect( action, SIGNAL(triggered(bool)), SLOT(slotSendNow()));

        action = new KAction(KIcon(QLatin1String("mail-send")), i18n("Send Mail Using Shortcut"), this);
        actionCollection()->addAction(QLatin1String("send_mail"), action );
        action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Return ) );
        connect( action, SIGNAL(triggered(bool)), SLOT(slotSendNowByShortcut()));


        // FIXME: change to mail_send_via icon when this exist.
        actActionNowMenu = new KActionMenu( KIcon( QLatin1String("mail-send") ), i18n("&Send Mail Via"), this );
        actActionNowMenu->setIconText( i18n( "Send" ) );
        actionCollection()->addAction( QLatin1String("send_default_via"), actActionNowMenu );

        action = new KAction( KIcon( QLatin1String("mail-queue") ), i18n("Send &Later"), this );
        actionCollection()->addAction( QLatin1String("send_alternative"), action );
        connect( action, SIGNAL(triggered(bool)), SLOT(slotSendLater()) );
        actActionLaterMenu = new KActionMenu( KIcon( QLatin1String("mail-queue") ), i18n("Send &Later Via"), this );
        actActionLaterMenu->setIconText( i18nc( "Queue the message for sending at a later date", "Queue" ) );
        actionCollection()->addAction( QLatin1String("send_alternative_via"), actActionLaterMenu );

    } else {
        //default = queue, alternative = send now
        KAction *action = new KAction( KIcon( QLatin1String("mail-queue") ), i18n("Send &Later"), this );
        actionCollection()->addAction( QLatin1String("send_mail"), action );
        connect( action, SIGNAL(triggered(bool)), SLOT(slotSendLater()) );
        action->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Return ) );
        actActionLaterMenu = new KActionMenu( KIcon( QLatin1String("mail-queue") ), i18n("Send &Later Via"), this );
        actionCollection()->addAction( QLatin1String("send_default_via"), actActionLaterMenu );

        action = new KAction( KIcon( QLatin1String("mail-send") ), i18n("&Send Mail"), this );
        actionCollection()->addAction( QLatin1String("send_alternative"), action );
        connect( action, SIGNAL(triggered(bool)), SLOT(slotSendNow()) );

        // FIXME: change to mail_send_via icon when this exits.
        actActionNowMenu = new KActionMenu( KIcon( QLatin1String("mail-send") ), i18n("&Send Mail Via"), this );
        actionCollection()->addAction( QLatin1String("send_alternative_via"), actActionNowMenu );

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

    connect( TransportManager::self(), SIGNAL(transportsChanged()), this, SLOT(getTransportMenu()));
    connect( mActNowMenu, SIGNAL(triggered(QAction*)), this,
             SLOT(slotSendNowVia(QAction*)) );

    connect( mActLaterMenu, SIGNAL(triggered(QAction*)), this,
             SLOT(slotSendLaterVia(QAction*)) );

    KAction *action = new KAction( KIcon( QLatin1String("document-save") ), i18n("Save as &Draft"), this );
    actionCollection()->addAction(QLatin1String("save_in_drafts"), action );
    action->setHelpText(i18n("Save email in Draft folder"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    connect( action, SIGNAL(triggered(bool)), SLOT(slotSaveDraft()) );

    action = new KAction( KIcon( QLatin1String("document-save") ), i18n("Save as &Template"), this );
    action->setHelpText(i18n("Save email in Template folder"));
    actionCollection()->addAction( QLatin1String("save_in_templates"), action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotSaveTemplate()) );

    action = new KAction( KIcon( QLatin1String("document-save") ), i18n("Save as &File"), this );
    action->setHelpText(i18n("Save email as text or html file"));
    actionCollection()->addAction( QLatin1String("save_as_file"), action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotSaveAsFile()) );

    action = new KAction(KIcon( QLatin1String( "contact-new" ) ), i18n("New AddressBook Contact..."),this);
    actionCollection()->addAction(QLatin1String("kmail_new_addressbook_contact"), action );
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotCreateAddressBookContact()));



    action = new KAction(KIcon(QLatin1String("document-open")), i18n("&Insert Text File..."), this);
    actionCollection()->addAction(QLatin1String("insert_file"), action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotInsertFile()));

    mRecentAction = new KRecentFilesAction( KIcon( QLatin1String("document-open") ),
                                            i18n( "&Insert Recent Text File" ), this );
    actionCollection()->addAction(QLatin1String("insert_file_recent"), mRecentAction );
    connect(mRecentAction, SIGNAL(urlSelected(KUrl)),
            SLOT(slotInsertRecentFile(KUrl)));
    connect(mRecentAction, SIGNAL(recentListCleared()),
            SLOT(slotRecentListFileClear()));
    mRecentAction->loadEntries( KMKernel::self()->config()->group( QString() ) );

    action = new KAction(KIcon(QLatin1String("x-office-address-book")), i18n("&Address Book"), this);
    action->setHelpText(i18n("Open Address Book"));
    actionCollection()->addAction(QLatin1String("addressbook"), action );
    if (KStandardDirs::findExe(QLatin1String("kaddressbook")).isEmpty())
        action->setEnabled(false);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotAddrBook()));
    action = new KAction(KIcon(QLatin1String("mail-message-new")), i18n("&New Composer"), this);
    actionCollection()->addAction(QLatin1String("new_composer"), action );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotNewComposer()));
    action->setShortcuts( KStandardShortcut::shortcut( KStandardShortcut::New ) );

    action = new KAction( i18n("Select &Recipients..."), this );
    actionCollection()->addAction( QLatin1String("select_recipients"), action );
    connect( action, SIGNAL(triggered(bool)),
             mComposerBase->recipientsEditor(), SLOT(selectRecipients()) );
    action = new KAction( i18n("Save &Distribution List..."), this );
    actionCollection()->addAction( QLatin1String("save_distribution_list"), action );
    connect( action, SIGNAL(triggered(bool)),
             mComposerBase->recipientsEditor(), SLOT(saveDistributionList()) );

    KStandardAction::print( this, SLOT(slotPrint()), actionCollection() );
    if(KPrintPreview::isAvailable())
        KStandardAction::printPreview( this, SLOT(slotPrintPreview()), actionCollection() );
    KStandardAction::close( this, SLOT(slotClose()), actionCollection() );

    KStandardAction::undo( mGlobalAction, SLOT(slotUndo()), actionCollection() );
    KStandardAction::redo( mGlobalAction, SLOT(slotRedo()), actionCollection() );
    KStandardAction::cut( mGlobalAction, SLOT(slotCut()), actionCollection() );
    KStandardAction::copy( mGlobalAction, SLOT(slotCopy()), actionCollection() );
    KStandardAction::pasteText( mGlobalAction, SLOT(slotPaste()), actionCollection() );
    mSelectAll = KStandardAction::selectAll( mGlobalAction, SLOT(slotMarkAll()), actionCollection() );

    mFindText = KStandardAction::find( mComposerBase->editor(), SLOT(slotFind()), actionCollection() );
    mFindNextText = KStandardAction::findNext( mComposerBase->editor(), SLOT(slotFindNext()), actionCollection() );

    mReplaceText = KStandardAction::replace( mComposerBase->editor(), SLOT(slotReplace()), actionCollection() );
    actionCollection()->addAction( KStandardAction::Spelling, QLatin1String("spellcheck"),
                                   mComposerBase->editor(), SLOT(checkSpelling()) );

    action = new KAction( i18n("Paste as Attac&hment"), this );
    actionCollection()->addAction( QLatin1String("paste_att"), action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotPasteAsAttachment()) );

    action = new KAction( i18n("Cl&ean Spaces"), this );
    actionCollection()->addAction( QLatin1String("clean_spaces"), action );
    connect( action, SIGNAL(triggered(bool)), mComposerBase->signatureController(), SLOT(cleanSpace()) );

    mFixedFontAction = new KToggleAction( i18n("Use Fi&xed Font"), this );
    actionCollection()->addAction( QLatin1String("toggle_fixedfont"), mFixedFontAction );
    connect( mFixedFontAction, SIGNAL(triggered(bool)), SLOT(slotUpdateFont()) );
    mFixedFontAction->setChecked( MessageViewer::GlobalSettings::self()->useFixedFont() );

    //these are checkable!!!
    mUrgentAction = new KToggleAction(
                i18nc("@action:inmenu Mark the email as urgent.","&Urgent"), this );
    actionCollection()->addAction( QLatin1String("urgent"), mUrgentAction );
    mRequestMDNAction = new KToggleAction( i18n("&Request Disposition Notification"), this );
    actionCollection()->addAction(QLatin1String("options_request_mdn"), mRequestMDNAction );
    mRequestMDNAction->setChecked(GlobalSettings::self()->requestMDN());
    //----- Message-Encoding Submenu
    mCodecAction = new CodecAction( CodecAction::ComposerMode, this );
    actionCollection()->addAction( QLatin1String("charsets"), mCodecAction );
    mWordWrapAction = new KToggleAction( i18n( "&Wordwrap" ), this );
    actionCollection()->addAction( QLatin1String("wordwrap"), mWordWrapAction );
    mWordWrapAction->setChecked( MessageComposer::MessageComposerSettings::self()->wordWrap() );
    connect( mWordWrapAction, SIGNAL(toggled(bool)), SLOT(slotWordWrapToggled(bool)) );

    mSnippetAction = new KToggleAction( i18n("&Snippets"), this );
    actionCollection()->addAction( QLatin1String("snippets"), mSnippetAction );
    connect( mSnippetAction, SIGNAL(toggled(bool)), this, SLOT(slotSnippetWidgetVisibilityChanged(bool)));
    mSnippetAction->setChecked( GlobalSettings::self()->showSnippetManager() );

    mAutoSpellCheckingAction = new KToggleAction( KIcon( QLatin1String("tools-check-spelling") ),
                                                  i18n("&Automatic Spellchecking"),
                                                  this );
    actionCollection()->addAction( QLatin1String("options_auto_spellchecking"), mAutoSpellCheckingAction );
    const bool spellChecking = GlobalSettings::self()->autoSpellChecking();
    const bool useKmailEditor = !GlobalSettings::self()->useExternalEditor();
    const bool spellCheckingEnabled = useKmailEditor && spellChecking;
    mAutoSpellCheckingAction->setEnabled( useKmailEditor );

    mAutoSpellCheckingAction->setChecked( spellCheckingEnabled );
    slotAutoSpellCheckingToggled( spellCheckingEnabled );
    connect( mAutoSpellCheckingAction, SIGNAL(toggled(bool)),
             this, SLOT(slotAutoSpellCheckingToggled(bool)) );
    connect( mComposerBase->editor(), SIGNAL(checkSpellingChanged(bool)),
             this, SLOT(slotAutoSpellCheckingToggled(bool)) );

    connect( mComposerBase->editor(), SIGNAL(textModeChanged(KRichTextEdit::Mode)),
             this, SLOT(slotTextModeChanged(KRichTextEdit::Mode)) );
    connect( mComposerBase->editor(), SIGNAL(externalEditorClosed()), this, SLOT(slotExternalEditorClosed()));
    connect( mComposerBase->editor(), SIGNAL(externalEditorStarted()), this, SLOT(slotExternalEditorStarted()));
    //these are checkable!!!
    markupAction = new KToggleAction( i18n("Rich Text Editing"), this );
    markupAction->setIcon( KIcon( QLatin1String("preferences-desktop-font" )) );
    markupAction->setIconText( i18n("Rich Text") );
    markupAction->setToolTip( i18n( "Toggle rich text editing mode" ) );
    actionCollection()->addAction( QLatin1String("html"), markupAction );
    connect( markupAction, SIGNAL(triggered(bool)), SLOT(slotToggleMarkup()) );

    mAllFieldsAction = new KToggleAction( i18n("&All Fields"), this);
    actionCollection()->addAction( QLatin1String("show_all_fields"), mAllFieldsAction );
    connect( mAllFieldsAction, SIGNAL(triggered(bool)), SLOT(slotUpdateView()));
    mIdentityAction = new KToggleAction(i18n("&Identity"), this);
    actionCollection()->addAction(QLatin1String("show_identity"), mIdentityAction );
    connect( mIdentityAction, SIGNAL(triggered(bool)), SLOT(slotUpdateView()));
    mDictionaryAction = new KToggleAction(i18n("&Dictionary"), this);
    actionCollection()->addAction(QLatin1String("show_dictionary"), mDictionaryAction );
    connect( mDictionaryAction, SIGNAL(triggered(bool)), SLOT(slotUpdateView()));
    mFccAction = new KToggleAction(i18n("&Sent-Mail Folder"), this);
    actionCollection()->addAction(QLatin1String("show_fcc"), mFccAction );
    connect( mFccAction, SIGNAL(triggered(bool)), SLOT(slotUpdateView()));
    mTransportAction = new KToggleAction(i18n("&Mail Transport"), this);
    actionCollection()->addAction(QLatin1String("show_transport"), mTransportAction );
    connect( mTransportAction, SIGNAL(triggered(bool)), SLOT(slotUpdateView()));
    mFromAction = new KToggleAction(i18n("&From"), this);
    actionCollection()->addAction(QLatin1String("show_from"), mFromAction );
    connect( mFromAction, SIGNAL(triggered(bool)), SLOT(slotUpdateView()));
    mReplyToAction = new KToggleAction(i18n("&Reply To"), this);
    actionCollection()->addAction(QLatin1String("show_reply_to"), mReplyToAction );
    connect( mReplyToAction, SIGNAL(triggered(bool)), SLOT(slotUpdateView()));
    mSubjectAction = new KToggleAction(
                i18nc("@action:inmenu Show the subject in the composer window.", "S&ubject"), this);
    actionCollection()->addAction(QLatin1String("show_subject"), mSubjectAction );
    connect(mSubjectAction, SIGNAL(triggered(bool)), SLOT(slotUpdateView()));
    //end of checkable

    mAppendSignature = new KAction( i18n("Append S&ignature"), this );
    actionCollection()->addAction( QLatin1String("append_signature"), mAppendSignature );
    connect( mAppendSignature, SIGNAL(triggered(bool)), mComposerBase->signatureController(), SLOT(appendSignature()));

    mPrependSignature = new KAction( i18n("Pr&epend Signature"), this );
    actionCollection()->addAction( QLatin1String("prepend_signature"), mPrependSignature );
    connect( mPrependSignature, SIGNAL(triggered(bool)), mComposerBase->signatureController(), SLOT(prependSignature()) );

    mInsertSignatureAtCursorPosition = new KAction( i18n("Insert Signature At C&ursor Position"), this );
    actionCollection()->addAction( QLatin1String("insert_signature_at_cursor_position"), mInsertSignatureAtCursorPosition );
    connect( mInsertSignatureAtCursorPosition, SIGNAL(triggered(bool)), mComposerBase->signatureController(), SLOT(insertSignatureAtCursor()) );


    action = new KAction( i18n("Insert Special Character..."), this );
    actionCollection()->addAction( QLatin1String("insert_special_character"), action );
    connect( action, SIGNAL(triggered(bool)), this, SLOT(insertSpecialCharacter()) );

    KAction *upperCase = new KAction( i18n("Uppercase"), this );
    actionCollection()->addAction( QLatin1String("change_to_uppercase"), upperCase );
    connect( upperCase, SIGNAL(triggered(bool)), this, SLOT(slotUpperCase()) );

    KAction *sentenceCase = new KAction( i18n("Sentence case"), this );
    actionCollection()->addAction( QLatin1String("change_to_sentencecase"), sentenceCase );
    connect( sentenceCase, SIGNAL(triggered(bool)), this, SLOT(slotSentenceCase()) );


    KAction *lowerCase = new KAction( i18n("Lowercase"), this );
    actionCollection()->addAction( QLatin1String("change_to_lowercase"), lowerCase );
    connect( lowerCase, SIGNAL(triggered(bool)), this, SLOT(slotLowerCase()) );

    mChangeCaseMenu = new KActionMenu(i18n("Change Case"), this);
    actionCollection()->addAction(QLatin1String("change_case_menu"), mChangeCaseMenu );
    mChangeCaseMenu->addAction(sentenceCase);
    mChangeCaseMenu->addAction(upperCase);
    mChangeCaseMenu->addAction(lowerCase);

    mComposerBase->attachmentController()->createActions();

    setStandardToolBarMenuEnabled( true );

    KStandardAction::keyBindings( this, SLOT(slotEditKeys()), actionCollection());
    KStandardAction::configureToolbars( this, SLOT(slotEditToolbars()), actionCollection());
    KStandardAction::preferences( kmkernel, SLOT(slotShowConfigurationDialog()), actionCollection() );

    action = new KAction( i18n("&Spellchecker..."), this );
    action->setIconText( i18n("Spellchecker") );
    actionCollection()->addAction( QLatin1String("setup_spellchecker"), action );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotSpellcheckConfig()) );

    mTranslateAction = mCustomToolsWidget->action(PimCommon::CustomToolsWidget::TranslatorTool);
    actionCollection()->addAction( QLatin1String("translator"), mTranslateAction );

    mGenerateShortenUrl = mCustomToolsWidget->action(PimCommon::CustomToolsWidget::ShortUrlTool);
    actionCollection()->addAction( QLatin1String("shorten_url"), mGenerateShortenUrl );

    mEncryptAction = new KToggleAction(KIcon(QLatin1String("document-encrypt")), i18n("&Encrypt Message"), this);
    mEncryptAction->setIconText( i18n( "Encrypt" ) );
    actionCollection()->addAction(QLatin1String("encrypt_message"), mEncryptAction );
    mSignAction = new KToggleAction(KIcon(QLatin1String("document-sign")), i18n("&Sign Message"), this);
    mSignAction->setIconText( i18n( "Sign" ) );
    actionCollection()->addAction(QLatin1String("sign_message"), mSignAction );
    const KPIMIdentities::Identity &ident =
            KMKernel::self()->identityManager()->identityForUoidOrDefault( mComposerBase->identityCombo()->currentIdentity() );
    // PENDING(marc): check the uses of this member and split it into
    // smime/openpgp and or enc/sign, if necessary:
    mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
    mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

    mLastEncryptActionState = false;
    mLastSignActionState = ident.pgpAutoSign();

    changeCryptoAction();

    connect( mEncryptAction, SIGNAL(triggered(bool)),
             SLOT(slotEncryptToggled(bool)) );
    connect( mSignAction, SIGNAL(triggered(bool)),
             SLOT(slotSignToggled(bool)) );

    QStringList l;
    for ( int i=0 ; i<numCryptoMessageFormats ; ++i ) {
        l.push_back( Kleo::cryptoMessageFormatToLabel( cryptoMessageFormats[i] ) );
    }

    mCryptoModuleAction = new KSelectAction(i18n("&Cryptographic Message Format"), this);
    actionCollection()->addAction(QLatin1String("options_select_crypto"), mCryptoModuleAction );
    connect(mCryptoModuleAction, SIGNAL(triggered(int)), SLOT(slotSelectCryptoModule()));
    mCryptoModuleAction->setItems( l );
    mCryptoModuleAction->setToolTip( i18n( "Select a cryptographic format for this message" ) );

    mComposerBase->editor()->createActions( actionCollection() );
    actionCollection()->addAction( QLatin1String("shared_link"), mStorageService->menuShareLinkServices() );

    mFollowUpToggleAction = new KToggleAction( i18n("Follow Up Mail..."), this );
    actionCollection()->addAction( QLatin1String("follow_up_mail"), mFollowUpToggleAction );
    connect( mFollowUpToggleAction, SIGNAL(triggered(bool)), this, SLOT(slotFollowUpMail(bool)) );
    mFollowUpToggleAction->setEnabled(FollowUpReminder::FollowUpReminderUtil::followupReminderAgentEnabled());

    createGUI( QLatin1String("kmcomposerui.rc") );
    connect( toolBar( QLatin1String("htmlToolBar") )->toggleViewAction(),
             SIGNAL(toggled(bool)),
             SLOT(htmlToolBarVisibilityChanged(bool)) );


    // In Kontact, this entry would read "Configure Kontact", but bring
    // up KMail's config dialog. That's sensible, though, so fix the label.
    QAction *configureAction = actionCollection()->action( QLatin1String("options_configure") );
    if ( configureAction ) {
        configureAction->setText( i18n("Configure KMail..." ) );
    }
    getTransportMenu();
}

void KMComposeWin::changeCryptoAction()
{
    const KPIMIdentities::Identity &ident =
            KMKernel::self()->identityManager()->identityForUoidOrDefault( mComposerBase->identityCombo()->currentIdentity() );
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
        setSigning( ( canOpenPGPSign || canSMIMESign ) && ident.pgpAutoSign() );
    }

}


void KMComposeWin::setupStatusBar( QWidget *w )
{
    KPIM::ProgressStatusBarWidget * progressStatusBarWidget = new KPIM::ProgressStatusBarWidget(statusBar(), this, PimCommon::StorageServiceProgressManager::progressTypeValue());
    statusBar()->addWidget(w);

    statusBar()->insertItem( QString(), 0, 1 );
    statusBar()->setItemAlignment( 0, Qt::AlignLeft | Qt::AlignVCenter );
    mStatusBarLabelToggledOverrideMode = new StatusBarLabelToggledState(this);
    mStatusBarLabelToggledOverrideMode->setStateString(i18n("OVR"), i18n("INS"));
    statusBar()->addPermanentWidget(mStatusBarLabelToggledOverrideMode,0 );
    connect(mStatusBarLabelToggledOverrideMode, SIGNAL(toggleModeChanged(bool)), this, SLOT(slotOverwriteModeWasChanged(bool)));

    mStatusBarLabelSpellCheckingChangeMode = new StatusBarLabelToggledState(this);
    mStatusBarLabelSpellCheckingChangeMode->setStateString(i18n( "Spellcheck: on" ), i18n( "Spellcheck: off" ));
    statusBar()->addPermanentWidget(mStatusBarLabelSpellCheckingChangeMode, 0 );
    connect(mStatusBarLabelSpellCheckingChangeMode, SIGNAL(toggleModeChanged(bool)), this, SLOT(slotAutoSpellCheckingToggled(bool)));

    statusBar()->insertPermanentItem( i18n(" Column: %1 ", QLatin1String( "     " ) ), 2, 0 );
    statusBar()->insertPermanentItem(
                i18nc("Shows the linenumber of the cursor position.", " Line: %1 "
                      , QLatin1String( "     " ) ), 1, 0 );
    statusBar()->addPermanentWidget(progressStatusBarWidget->littleProgress());
}


void KMComposeWin::setupEditor( void )
{
    QFontMetrics fm( mBodyFont );
    mComposerBase->editor()->setTabStopWidth( fm.width( QLatin1Char(' ') ) * 8 );

    slotWordWrapToggled( MessageComposer::MessageComposerSettings::self()->wordWrap() );

    // Font setup
    slotUpdateFont();

    connect( mComposerBase->editor(), SIGNAL(cursorPositionChanged()),
             this, SLOT(slotCursorPositionChanged()) );
    slotCursorPositionChanged();
}


QString KMComposeWin::subject() const
{
    return MessageComposer::Util::cleanedUpHeaderString( mEdtSubject->toPlainText() );
}


QString KMComposeWin::from() const
{
    return MessageComposer::Util::cleanedUpHeaderString( mEdtFrom->text() );
}


QString KMComposeWin::replyTo() const
{
    if ( mEdtReplyTo ) {
        return MessageComposer::Util::cleanedUpHeaderString( mEdtReplyTo->text() );
    } else {
        return QString();
    }
}


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

void KMComposeWin::setCurrentTransport( int transportId )
{
    mComposerBase->transportComboBox()->setCurrentTransport( transportId );
}

void KMComposeWin::setCurrentReplyTo(const QString& replyTo)
{
    if ( mEdtReplyTo ) {
        mEdtReplyTo->setText( replyTo );
    }
}


void KMComposeWin::setMessage( const KMime::Message::Ptr &newMsg, bool lastSignState, bool lastEncryptState, bool mayAutoSign,
                               bool allowDecryption, bool isModified )
{
    if ( !newMsg ) {
        kDebug() << "newMsg == 0!";
        return;
    }

    if( lastSignState )
        mLastSignActionState = true;

    if ( lastEncryptState )
        mLastEncryptActionState = true;

    mComposerBase->setMessage( newMsg );
    mMsg = newMsg;
    KPIMIdentities::IdentityManager * im = KMKernel::self()->identityManager();

    mEdtFrom->setText( mMsg->from()->asUnicodeString() );
    mEdtSubject->setText( mMsg->subject()->asUnicodeString() );


    // Restore the quote prefix. We can't just use the global quote prefix here,
    // since the prefix is different for each message, it might for example depend
    // on the original sender in a reply.
    if ( mMsg->headerByType( "X-KMail-QuotePrefix" ) )
        mComposerBase->editor()->setQuotePrefixName( mMsg->headerByType( "X-KMail-QuotePrefix" )->asUnicodeString() );

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
        disconnect( mComposerBase->identityCombo(),SIGNAL(identityChanged(uint)),
                    this, SLOT(slotIdentityChanged(uint)) ) ;
    }

    // load the mId into the gui, sticky or not, without emitting
    mComposerBase->identityCombo()->setCurrentIdentity( mId );
    const uint idToApply = mId;
    if ( !stickyIdentity ) {
        connect( mComposerBase->identityCombo(),SIGNAL(identityChanged(uint)),
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

    const KPIMIdentities::Identity &ident = im->identityForUoid( mComposerBase->identityCombo()->currentIdentity() );

    const bool stickyTransport = mBtnTransport->isChecked() && !mIgnoreStickyFields;
    if( stickyTransport ) {
        mComposerBase->transportComboBox()->setCurrentTransport( ident.transport().toInt() );
    }

    // TODO move the following to ComposerViewBase
    // however, requires the actions to be there as well in order to share with mobile client

    // check for the presence of a DNT header, indicating that MDN's were requested
    if( newMsg->headerByType( "Disposition-Notification-To" ) ) {
        QString mdnAddr = newMsg->headerByType( "Disposition-Notification-To" )->asUnicodeString();
        mRequestMDNAction->setChecked( ( !mdnAddr.isEmpty() &&
                                         im->thatIsMe( mdnAddr ) ) ||
                                       GlobalSettings::self()->requestMDN() );
    }
    // check for presence of a priority header, indicating urgent mail:
    if ( newMsg->headerByType( "X-PRIORITY" ) && newMsg->headerByType("Priority" ) )
    {
        const QString xpriority = newMsg->headerByType( "X-PRIORITY" )->asUnicodeString();
        const QString priority = newMsg->headerByType( "Priority" )->asUnicodeString();
        if ( xpriority == QLatin1String( "2 (High)" ) && priority == QLatin1String( "urgent" ) )
            mUrgentAction->setChecked( true );
    }

    if ( !ident.isXFaceEnabled() || ident.xface().isEmpty() ) {
        if( mMsg->headerByType( "X-Face" ) )
            mMsg->headerByType( "X-Face" )->clear();
    } else {
        QString xface = ident.xface();
        if ( !xface.isEmpty() ) {
            int numNL = ( xface.length() - 1 ) / 70;
            for ( int i = numNL; i > 0; --i ) {
                xface.insert( i * 70, QLatin1String("\n\t") );
            }
            mMsg->setHeader( new KMime::Headers::Generic( "X-Face", mMsg.get(), xface, "utf-8" ) );
        }
    }

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
    updateSignatureAndEncryptionStateIndicators();

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

    const bool stickyDictionary = mBtnDictionary->isChecked() && !mIgnoreStickyFields;
    if ( !stickyDictionary ) {
        if ( mMsg->headerByType( "X-KMail-Dictionary" ) ) {
            const QString dictionary = mMsg->headerByType( "X-KMail-Dictionary" )->asUnicodeString();
            if (!dictionary.isEmpty()) {
                mComposerBase->dictionary()->setCurrentByDictionary( dictionary );
            }
        } else {
            mComposerBase->dictionary()->setCurrentByDictionaryName( ident.dictionary() );
        }


    }

    mEdtReplyTo->setText( mMsg->replyTo()->asUnicodeString() );

    KMime::Content *msgContent = new KMime::Content;
    msgContent->setContent( mMsg->encodedContent() );
    msgContent->parse();
    MessageViewer::EmptySource emptySource;
    MessageViewer::ObjectTreeParser otp( &emptySource );//All default are ok
    emptySource.setAllowDecryption( allowDecryption );
    otp.parseObjectTree( msgContent );

    bool shouldSetCharset = false;
    if ( ( mContext == Reply || mContext == ReplyToAll || mContext == Forward ) && MessageComposer::MessageComposerSettings::forceReplyCharset() )
        shouldSetCharset = true;
    if ( shouldSetCharset && !otp.plainTextContentCharset().isEmpty() )
        mOriginalPreferredCharset = otp.plainTextContentCharset();
    // always set auto charset, but prefer original when composing if force reply is set.
    setAutoCharset();

    delete msgContent;
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

    if( (MessageComposer::MessageComposerSettings::self()->autoTextSignature()==QLatin1String( "auto" )) && mayAutoSign ) {
        //
        // Espen 2000-05-16
        // Delay the signature appending. It may start a fileseletor.
        // Not user friendy if this modal fileseletor opens before the
        // composer.
        //
        if ( MessageComposer::MessageComposerSettings::self()->prependSignature() ) {
            QTimer::singleShot( 0, mComposerBase->signatureController(), SLOT(prependSignature()) );
        } else {
            QTimer::singleShot( 0, mComposerBase->signatureController(), SLOT(appendSignature()) );
        }
    } else {
        mComposerBase->editor()->startExternalEditor();
    }

    setModified( isModified );

    // honor "keep reply in this folder" setting even when the identity is changed later on
    mPreventFccOverwrite = ( !kmailFcc.isEmpty() && ident.fcc() != kmailFcc );
    QTimer::singleShot( 0, this, SLOT(forceAutoSaveMessage()) ); //Force autosaving to make sure this composer reappears if a crash happens before the autosave timer kicks in.
}

void KMComposeWin::setAutoSaveFileName(const QString& fileName)
{
    mComposerBase->setAutoSaveFileName( fileName );
}


void KMComposeWin::setTextSelection( const QString& selection )
{
    mTextSelection = selection;
}


void KMComposeWin::setCustomTemplate( const QString& customTemplate )
{
    mCustomTemplate = customTemplate;
}

void KMComposeWin::setSigningAndEncryptionDisabled(bool v)
{
    mSigningAndEncryptionExplicitlyDisabled = v;
}

void KMComposeWin::setFolder(const Akonadi::Collection &aFolder)
{
    mFolder = aFolder;
}


void KMComposeWin::setFcc( const QString &idString )
{
    // check if the sent-mail folder still exists
    Akonadi::Collection col;
    if ( idString.isEmpty() )
        col = CommonKernel->sentCollectionFolder();
    else
        col = Akonadi::Collection( idString.toLongLong() );

    mComposerBase->setFcc( col );
    mFccFolder->setCollection( col );
}

bool KMComposeWin::isComposerModified() const
{
    return ( mComposerBase->editor()->document()->isModified() ||
             mEdtFrom->isModified() ||
             ( mEdtReplyTo && mEdtReplyTo->isModified() ) ||
             mComposerBase->recipientsEditor()->isModified() ||
             mEdtSubject->document()->isModified() );
}


bool KMComposeWin::isModified() const
{
    return mWasModified || isComposerModified();
}


void KMComposeWin::setModified( bool modified )
{
    mWasModified = modified;
    changeModifiedState( modified );
}


void KMComposeWin::changeModifiedState( bool modified )
{
    mComposerBase->editor()->document()->setModified( modified );
    if ( !modified ) {
        mEdtFrom->setModified( false );
        if ( mEdtReplyTo ) mEdtReplyTo->setModified( false );
        mComposerBase->recipientsEditor()->clearModified();
        mEdtSubject->document()->setModified( false );
    }
}


bool KMComposeWin::queryClose ()
{
    if ( !mComposerBase->editor()->checkExternalEditorFinished() ) {
        return false;
    }
    if ( kmkernel->shuttingDown() || kapp->sessionSaving() ) {
        return true;
    }

    if ( isModified() ) {
        const bool istemplate = ( mFolder.isValid() && CommonKernel->folderIsTemplates( mFolder ) );
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
                                                        KGuiItem(savebut, QLatin1String("document-save"), QString(), savetext),
                                                        KStandardGuiItem::discard(),
                                                        KStandardGuiItem::cancel());
        if ( rc == KMessageBox::Cancel ) {
            return false;
        } else if ( rc == KMessageBox::Yes ) {
            // doSend will close the window. Just return false from this method
            if (istemplate)
                slotSaveTemplate();
            else
                slotSaveDraft();
            return false;
        }
        //else fall through: return true
    }
    mComposerBase->cleanupAutoSave();

    if( !mMiscComposers.isEmpty() ) {
        kWarning() << "Tried to close while composer was active";
        return false;
    }
    return true;
}


MessageComposer::ComposerViewBase::MissingAttachment KMComposeWin::userForgotAttachment()
{
    bool checkForForgottenAttachments = mCheckForForgottenAttachments && GlobalSettings::self()->showForgottenAttachmentWarning();

    if ( !checkForForgottenAttachments )
        return MessageComposer::ComposerViewBase::NoMissingAttachmentFound;

    mComposerBase->setSubject( subject() ); //be sure the composer knows the subject
    MessageComposer::ComposerViewBase::MissingAttachment missingAttachments = mComposerBase->checkForMissingAttachments( GlobalSettings::self()->attachmentKeywords() );

    return missingAttachments;
}

void KMComposeWin::forceAutoSaveMessage()
{
    autoSaveMessage( true );
}

void KMComposeWin::autoSaveMessage(bool force)
{
    if ( isComposerModified() || force ) {
        applyComposerSetting( mComposerBase );
        mComposerBase->saveMailSettings();
        mComposerBase->autoSaveMessage();
        if ( !force ) {
            mWasModified = true;
            changeModifiedState( false );
        }
    } else {
        mComposerBase->updateAutoSave();
    }
}

bool KMComposeWin::encryptToSelf() const
{
    return MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelf();
}



void KMComposeWin::slotSendFailed( const QString& msg,MessageComposer::ComposerViewBase::FailedType type)
{
    setEnabled( true );
    if (!msg.isEmpty()) {
        KMessageBox::sorry( mMainWidget, msg,
                            (type == MessageComposer::ComposerViewBase::AutoSave) ? i18n( "Autosave Message Failed" ) : i18n( "Sending Message Failed" ) );
    }
}

void KMComposeWin::slotSendSuccessful(const QString &messageId)
{
    setModified( false );
    addFollowupReminder(messageId);
    mComposerBase->cleanupAutoSave();
    mFolder = Akonadi::Collection(); // see dtor
    close();
}

void KMComposeWin::addFollowupReminder(const QString &messageId)
{
    const QDate date = mComposerBase->followUpDate();
    if (date.isValid()) {
        FollowupReminderCreateJob *job = new FollowupReminderCreateJob;
        job->setSubject(subject());
        job->setMessageId(messageId);
        job->setTo(replyTo());
        job->setFollowUpReminderDate(date);
        job->setCollectionToDo(mComposerBase->followUpCollection());
        job->start();
    }
}

const KPIMIdentities::Identity &KMComposeWin::identity() const
{
    return KMKernel::self()->identityManager()->identityForUoidOrDefault( mComposerBase->identityCombo()->currentIdentity() );
}

Kleo::CryptoMessageFormat KMComposeWin::cryptoMessageFormat() const
{
    if ( !mCryptoModuleAction ) {
        return Kleo::AutoFormat;
    }
    return cb2format( mCryptoModuleAction->currentItem() );
}


void KMComposeWin::addAttach( KMime::Content *msgPart )
{
    mComposerBase->addAttachmentPart( msgPart );
    setModified( true );
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


void KMComposeWin::setCharset( const QByteArray &charset )
{
    const QString codecNameToSet = selectCharset( mCodecAction, QString::fromLatin1(charset) );
    if ( codecNameToSet.isEmpty() ) {
        kWarning() << "Could not find charset" << charset;
        setAutoCharset();
    }
    else
        mCodecAction->setCurrentCodec( codecNameToSet );
}


void KMComposeWin::slotAddrBook()
{
    KRun::runCommand(QLatin1String("kaddressbook"), window());
}


void KMComposeWin::slotInsertFile()
{
    KUrl u = mComposerBase->editor()->insertFile();
    if ( u.isEmpty() )
        return;

    mRecentAction->addUrl( u );
    // Prevent race condition updating list when multiple composers are open
    {
        const QString encoding = MessageViewer::NodeHelper::encodingForName( u.fileEncoding() );
        QStringList urls = GlobalSettings::self()->recentUrls();
        QStringList encodings = GlobalSettings::self()->recentEncodings();
        // Prevent config file from growing without bound
        // Would be nicer to get this constant from KRecentFilesAction
        const int mMaxRecentFiles = 30;
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
        GlobalSettings::self()->setRecentUrls( urls );
        GlobalSettings::self()->setRecentEncodings( encodings );
        mRecentAction->saveEntries( KMKernel::self()->config()->group( QString() ) );
    }
    slotInsertRecentFile( u );
}


void KMComposeWin::slotRecentListFileClear()
{
    KSharedConfig::Ptr config = KMKernel::self()->config();
    KConfigGroup group( config, "Composer" );
    group.deleteEntry("recent-urls");
    group.deleteEntry("recent-encodings");
    mRecentAction->saveEntries( config->group( QString() ) );
}

void KMComposeWin::slotInsertRecentFile( const KUrl &u )
{
    if ( u.fileName().isEmpty() ) {
        return;
    }

    // Get the encoding previously used when inserting this file
    QString encoding;
    const QStringList urls = GlobalSettings::self()->recentUrls();
    const QStringList encodings = GlobalSettings::self()->recentEncodings();
    const int index = urls.indexOf( u.prettyUrl() );
    if ( index != -1 ) {
        encoding = encodings[ index ];
    } else {
        kDebug()<<" encoding not found so we can't insert text"; //see InsertTextFileJob
        return;
    }


    MessageComposer::InsertTextFileJob *job = new MessageComposer::InsertTextFileJob( mComposerBase->editor(), u );
    job->setEncoding( encoding );
    connect(job, SIGNAL(result(KJob*)), SLOT(slotInsertTextFile(KJob*)));
    job->start();
}

void KMComposeWin::slotInsertTextFile(KJob*job)
{
    if ( job->error() ) {
        if ( static_cast<KIO::Job*>(job)->ui() )
            static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
        else
            kDebug()<<" job->errorString() :"<<job->errorString();
    }
}

void KMComposeWin::slotSelectCryptoModule( bool init )
{
    if ( !init )
        setModified( true );

    mComposerBase->attachmentModel()->setEncryptEnabled( canSignEncryptAttachments() );
    mComposerBase->attachmentModel()->setSignEnabled( canSignEncryptAttachments() );
}


void KMComposeWin::slotUpdateFont()
{
    kDebug();
    if ( !mFixedFontAction ) {
        return;
    }
    mComposerBase->editor()->setFontForWholeText( mFixedFontAction->isChecked() ?
                                                      mFixedFont : mBodyFont );
}

QString KMComposeWin::smartQuote( const QString & msg )
{
    return MessageCore::StringUtil::smartQuote( msg, MessageComposer::MessageComposerSettings::self()->lineWrapWidth() );
}


bool KMComposeWin::insertFromMimeData( const QMimeData *source, bool forceAttachment )
{
    // If this is a PNG image, either add it as an attachment or as an inline image
    if ( source->hasImage() && source->hasFormat( QLatin1String("image/png") ) ) {
        // Get the image data before showing the dialog, since that processes events which can delete
        // the QMimeData object behind our back
        const QByteArray imageData = source->data( QLatin1String("image/png") );
        if ( imageData.isEmpty() ) {
            return true;
        }
        if ( !forceAttachment ) {
            if ( mComposerBase->editor()->textMode() == KRichTextEdit::Rich && mComposerBase->editor()->isEnableImageActions() ) {
                QImage image = qvariant_cast<QImage>( source->imageData() );
                QFileInfo fi( source->text() );

                KMenu menu;
                const QAction *addAsInlineImageAction = menu.addAction( i18n("Add as &Inline Image") );
                /*const QAction *addAsAttachmentAction = */menu.addAction( i18n("Add as &Attachment") );
                const QAction *selectedAction = menu.exec( QCursor::pos() );
                if ( selectedAction == addAsInlineImageAction ) {
                    // Let the textedit from kdepimlibs handle inline images
                    mComposerBase->editor()->insertImage( image, fi );
                    return true;
                } else if( !selectedAction ) {
                    return true;
                }
                // else fall through
            }
        }
        // Ok, when we reached this point, the user wants to add the image as an attachment.
        // Ask for the filename first.
        bool ok;
        const QString attName =
                KInputDialog::getText( i18n("KMail"), i18n( "Name of the attachment:" ), QString(), &ok, this );
        if ( !ok ) {
            return true;
        }
        addAttachment( attName, KMime::Headers::CEbase64, QString(), imageData, "image/png" );
        return true;
    }

    // If this is a URL list, add those files as attachments or text
    const KUrl::List urlList = KUrl::List::fromMimeData( source );
    if ( !urlList.isEmpty() ) {
        //Search if it's message items.
        Akonadi::Item::List items;
        Akonadi::Collection::List collections;
        bool allLocalURLs = true;

        foreach ( const KUrl &url, urlList ) {
            if ( !url.isLocalFile() ) {
                allLocalURLs = false;
            }
            const Akonadi::Item item = Akonadi::Item::fromUrl( url );
            if ( item.isValid() ) {
                items << item;
            } else {
                const Akonadi::Collection collection = Akonadi::Collection::fromUrl( url );
                if ( collection.isValid() )
                    collections << collection;
            }
        }

        if ( items.isEmpty() && collections.isEmpty() ) {
            if ( allLocalURLs || forceAttachment ) {
                foreach( const KUrl &url, urlList ) {
                    addAttachment( url, QString() );
                }
            } else {
                KMenu p;
                const QAction *addAsTextAction = p.addAction( i18np("Add URL into Message", "Add URLs into Message", urlList.size() ) );
                const QAction *addAsAttachmentAction = p.addAction( i18np("Add File as &Attachment", "Add Files as &Attachment", urlList.size() ) );
                const QAction *selectedAction = p.exec( QCursor::pos() );

                if ( selectedAction == addAsTextAction ) {
                    foreach( const KUrl &url, urlList ) {
                        mComposerBase->editor()->insertLink(url.url());
                    }
                } else if ( selectedAction == addAsAttachmentAction ) {
                    foreach( const KUrl &url, urlList ) {
                        addAttachment( url, QString() );
                    }
                }
            }
            return true;
        } else {
            if ( !items.isEmpty() ){
                Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob( items, this );
                itemFetchJob->fetchScope().fetchFullPayload( true );
                itemFetchJob->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
                connect( itemFetchJob, SIGNAL(result(KJob*)), this, SLOT(slotFetchJob(KJob*)) );
            }
            if ( !collections.isEmpty() ) {
                //TODO
            }
            return true;
        }
    }
    return false;
}

void KMComposeWin::slotPasteAsAttachment()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if ( insertFromMimeData( mimeData, true ) )
        return;
    if( mimeData->hasText() ) {
        bool ok;
        const QString attName = KInputDialog::getText(
                    i18n( "Insert clipboard text as attachment" ),
                    i18n( "Name of the attachment:" ),
                    QString(), &ok, this );
        if( ok ) {
            mComposerBase->addAttachment( attName, attName, QLatin1String("utf-8"), QApplication::clipboard()->text().toUtf8(), "text/plain" );
        }
        return;
    }
}


void KMComposeWin::slotFetchJob(KJob*job)
{
    if ( job->error() ) {
        if ( static_cast<KIO::Job*>(job)->ui() )
            static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
        else
            kDebug()<<" job->errorString() :"<<job->errorString();
        return;
    }
    Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob*>( job );
    if ( !fjob )
        return;
    const Akonadi::Item::List items = fjob->items();

    if ( items.isEmpty() )
        return;

    if ( items.first().mimeType() == KMime::Message::mimeType() ) {
        uint identity = 0;
        if ( items.at( 0 ).isValid() && items.at( 0 ).parentCollection().isValid() ) {
            QSharedPointer<MailCommon::FolderCollection> fd( MailCommon::FolderCollection::forCollection( items.at( 0 ).parentCollection(), false ) );
            if ( fd )
                identity = fd->identity();
        }
        KMCommand *command = new KMForwardAttachedCommand( this, items,identity, this );
        command->start();
    } else {
        foreach ( const Akonadi::Item &item, items ) {
            QString attachmentName = QLatin1String( "attachment" );
            if ( item.hasPayload<KABC::Addressee>() ) {
                const KABC::Addressee contact = item.payload<KABC::Addressee>();
                attachmentName = contact.realName() + QLatin1String( ".vcf" );
                //Workaround about broken kaddressbook fields.
                QByteArray data = item.payloadData();
                PimCommon::VCardUtil vcardUtil;
                vcardUtil.adaptVcard(data);
                addAttachment( attachmentName, KMime::Headers::CEbase64, QString(), data, "text/x-vcard" );
            } else if ( item.hasPayload<KABC::ContactGroup>() ) {
                const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
                attachmentName = group.name() + QLatin1String( ".vcf" );
                Akonadi::ContactGroupExpandJob *expandJob = new Akonadi::ContactGroupExpandJob( group, this );
                expandJob->setProperty("groupName", attachmentName);
                connect( expandJob, SIGNAL(result(KJob*)), this, SLOT(slotExpandGroupResult(KJob*)) );
                expandJob->start();
            } else {
                addAttachment( attachmentName, KMime::Headers::CEbase64, QString(), item.payloadData(), item.mimeType().toLatin1() );
            }
        }
    }
}

void KMComposeWin::slotExpandGroupResult(KJob *job)
{
    Akonadi::ContactGroupExpandJob *expandJob = qobject_cast<Akonadi::ContactGroupExpandJob*>( job );
    Q_ASSERT( expandJob );

    const QString attachmentName = expandJob->property("groupName").toString();
    KABC::VCardConverter converter;
    const QByteArray groupData = converter.exportVCards(expandJob->contacts(), KABC::VCardConverter::v3_0);
    if (!groupData.isEmpty()) {
        addAttachment( attachmentName, KMime::Headers::CEbase64, QString(), groupData, "text/x-vcard" );
    }
}


QString KMComposeWin::addQuotesToText( const QString &inputText ) const
{
    QString answer( inputText );
    const QString indentStr = mComposerBase->editor()->quotePrefixName();
    answer.replace( QLatin1Char('\n'), QLatin1Char('\n') + indentStr );
    answer.prepend( indentStr );
    answer += QLatin1Char('\n');
    return MessageCore::StringUtil::smartQuote( answer, MessageComposer::MessageComposerSettings::self()->lineWrapWidth() );
}

void KMComposeWin::slotClose()
{
    close();
}


void KMComposeWin::slotNewComposer()
{
    KMComposeWin *win;
    KMime::Message::Ptr msg( new KMime::Message );

    MessageHelper::initHeader( msg, KMKernel::self()->identityManager() );
    win = new KMComposeWin( msg, false, false, KMail::Composer::New );
    win->setCollectionForNewMessage(mCollectionForNewMessage);
    win->show();
}


void KMComposeWin::slotUpdWinTitle()
{
    QString s( mEdtSubject->toPlainText() );
    // Remove characters that show badly in most window decorations:
    // newlines tend to become boxes.
    if ( s.isEmpty() ) {
        setCaption( QLatin1Char('(') + i18n("unnamed") + QLatin1Char(')') );
    } else {
        setCaption( s.replace( QLatin1Char('\n'), QLatin1Char(' ') ) );
    }
}


void KMComposeWin::slotEncryptToggled( bool on )
{
    setEncryption( on, true );
    updateSignatureAndEncryptionStateIndicators();
}


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
    if(!setByUser) {
        updateSignatureAndEncryptionStateIndicators();
    }
    // show the appropriate icon
    if ( encrypt ) {
        mEncryptAction->setIcon( KIcon( QLatin1String("document-encrypt") ) );
    } else {
        mEncryptAction->setIcon( KIcon( QLatin1String("document-decrypt") ) );
    }

    // mark the attachments for (no) encryption
    if( canSignEncryptAttachments() ) {
        mComposerBase->attachmentModel()->setEncryptSelected( encrypt );
    }
}


void KMComposeWin::slotSignToggled( bool on )
{
    setSigning( on, true );
    updateSignatureAndEncryptionStateIndicators();
}


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

    if(!setByUser) {
        updateSignatureAndEncryptionStateIndicators();
    }
    // mark the attachments for (no) signing
    if ( canSignEncryptAttachments() ) {
        mComposerBase->attachmentModel()->setSignSelected( sign );
    }
}


void KMComposeWin::slotWordWrapToggled( bool on )
{
    if ( on )
        mComposerBase->editor()->enableWordWrap( validateLineWrapWidth() );
    else
        disableWordWrap();
}

int KMComposeWin::validateLineWrapWidth()
{
    int lineWrap = MessageComposer::MessageComposerSettings::self()->lineWrapWidth();
    if ((lineWrap == 0) || (lineWrap > 78))
        lineWrap = 78;
    else if (lineWrap < 30)
        lineWrap = 30;
    return lineWrap;
}


void KMComposeWin::disableWordWrap()
{
    mComposerBase->editor()->disableWordWrap();
}


void KMComposeWin::forceDisableHtml()
{
    mForceDisableHtml = true;
    disableHtml( MessageComposer::ComposerViewBase::NoConfirmationNeeded );
    markupAction->setEnabled( false );
    // FIXME: Remove the toggle toolbar action somehow
}

bool KMComposeWin::isComposing() const
{
    return mComposerBase && mComposerBase->isComposing();
}

void KMComposeWin::disableForgottenAttachmentsCheck()
{
    mCheckForForgottenAttachments = false;
}

void KMComposeWin::ignoreStickyFields()
{
    mIgnoreStickyFields = true;
    mBtnTransport->setChecked( false );
    mBtnDictionary->setChecked( false );
    mBtnIdentity->setChecked( false );
    mBtnTransport->setEnabled( false );
    mBtnDictionary->setEnabled( false );
    mBtnIdentity->setEnabled( false );
}


void KMComposeWin::slotPrint()
{
    printComposer(false);
}

void KMComposeWin::slotPrintPreview()
{
    printComposer(true);
}

void KMComposeWin::printComposer(bool preview)
{
    MessageComposer::Composer* composer = createSimpleComposer();
    mMiscComposers.append( composer );
    composer->setProperty("preview",preview);
    connect( composer, SIGNAL(result(KJob*)),
             this, SLOT(slotPrintComposeResult(KJob*)) );
    composer->start();
}

void KMComposeWin::slotPrintComposeResult( KJob *job )
{
    const bool preview = job->property("preview").toBool();
    printComposeResult( job, preview );
}

void KMComposeWin::printComposeResult( KJob *job, bool preview )
{
    Q_ASSERT( dynamic_cast< MessageComposer::Composer* >( job ) );
    MessageComposer::Composer* composer = dynamic_cast< MessageComposer::Composer* >( job );
    Q_ASSERT( mMiscComposers.contains( composer ) );
    mMiscComposers.removeAll( composer );

    if( composer->error() == MessageComposer::Composer::NoError ) {

        Q_ASSERT( composer->resultMessages().size() == 1 );
        Akonadi::Item printItem;
        printItem.setPayload<KMime::Message::Ptr>( composer->resultMessages().first() );
        Akonadi::MessageFlags::copyMessageFlags(*(composer->resultMessages().first()), printItem);
        const bool isHtml = mComposerBase->editor()->textMode() == KMeditor::Rich;
        const MessageViewer::Viewer::DisplayFormatMessage format = isHtml ? MessageViewer::Viewer::Html : MessageViewer::Viewer::Text;
        KMPrintCommand *command = new KMPrintCommand( this, printItem,0,
                                                      0, format, isHtml );
        command->setPrintPreview( preview );
        command->start();
    } else {
        if ( static_cast<KIO::Job*>(job)->ui() ) {
            static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
        } else {
            kWarning() << "Composer for printing failed:" << composer->errorString();
        }
    }

}


void KMComposeWin::doSend( MessageComposer::MessageSender::SendMethod method,
                           MessageComposer::MessageSender::SaveIn saveIn )
{
    if ( mStorageService->numProgressUpdateFile() > 0) {
        KMessageBox::sorry( this, i18np( "There is %1 file upload in progress.",
                                         "There are %1 file uploads in progress.",
                                         mStorageService->numProgressUpdateFile() ) );
        return;
    }
    // TODO integrate with MDA online status
    if ( method == MessageComposer::MessageSender::SendImmediate ) {
        if( !MessageComposer::Util::sendMailDispatcherIsOnline() ) {
            method = MessageComposer::MessageSender::SendLater;
        }
    }


    if ( saveIn == MessageComposer::MessageSender::SaveInNone ) { // don't save as draft or template, send immediately
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
        if ( mComposerBase->to().isEmpty() ) {
            if ( mComposerBase->cc().isEmpty() && mComposerBase->bcc().isEmpty() ) {
                KMessageBox::information( this,
                                          i18n("You must specify at least one receiver, "
                                               "either in the To: field or as CC or as BCC.") );

                return;
            } else {
                int rc = KMessageBox::questionYesNo( this,
                                                     i18n("To: field is empty. "
                                                          "Send message anyway?"),
                                                     i18n("No To: specified"),
                                                     KStandardGuiItem::yes(),
                                                     KStandardGuiItem::no(),
                                                     QLatin1String(":kmail_no_to_field_specified") );
                if ( rc == KMessageBox::No ) {
                    return;
                }
            }
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
                                                QLatin1String("no_subject_specified") );
            if ( rc == KMessageBox::No ) {
                return;
            }
        }

        const MessageComposer::ComposerViewBase::MissingAttachment forgotAttachment = userForgotAttachment();
        if ( (forgotAttachment == MessageComposer::ComposerViewBase::FoundMissingAttachmentAndAddedAttachment) ||
             (forgotAttachment == MessageComposer::ComposerViewBase::FoundMissingAttachmentAndCancel) ) {
            return;
        }


        setEnabled( false );
        // Validate the To:, CC: and BCC fields
        const QStringList recipients = QStringList() << mComposerBase->to().trimmed() << mComposerBase->cc().trimmed() << mComposerBase->bcc().trimmed();

        AddressValidationJob *job = new AddressValidationJob( recipients.join( QLatin1String( ", ") ), this, this );
        const KPIMIdentities::Identity &ident = KMKernel::self()->identityManager()->identityForUoid( mComposerBase->identityCombo()->currentIdentity() );
        QString defaultDomainName;
        if ( !ident.isNull() ) {
            defaultDomainName = ident.defaultDomainName();
        }
        job->setDefaultDomain(defaultDomainName);
        job->setProperty( "method", static_cast<int>( method ) );
        job->setProperty( "saveIn", static_cast<int>( saveIn ) );
        connect( job, SIGNAL(result(KJob*)), SLOT(slotDoDelayedSend(KJob*)) );
        job->start();

        // we'll call send from within slotDoDelaySend
    } else {
        if( saveIn == MessageComposer::MessageSender::SaveInDrafts && mEncryptAction->isChecked() &&
                !GlobalSettings::self()->neverEncryptDrafts() &&
                mComposerBase->to().isEmpty() && mComposerBase->cc().isEmpty() ) {

            KMessageBox::information( this, i18n("You must specify at least one receiver "
                                                 "in order to be able to encrypt a draft.")
                                      );
            return;
        }
        doDelayedSend( method, saveIn );
    }
}

void KMComposeWin::slotDoDelayedSend( KJob *job )
{
    if ( job->error() ) {
        KMessageBox::error( this, job->errorText() );
        setEnabled(true);
        return;
    }

    const AddressValidationJob *validateJob = qobject_cast<AddressValidationJob*>( job );

    // Abort sending if one of the recipient addresses is invalid ...
    if ( !validateJob->isValid() ) {
        setEnabled(true);
        return;
    }

    // ... otherwise continue as usual
    const MessageComposer::MessageSender::SendMethod method = static_cast<MessageComposer::MessageSender::SendMethod>( job->property( "method" ).toInt() );
    const MessageComposer::MessageSender::SaveIn saveIn = static_cast<MessageComposer::MessageSender::SaveIn>( job->property( "saveIn" ).toInt() );

    doDelayedSend( method, saveIn );
}

void KMComposeWin::applyComposerSetting( MessageComposer::ComposerViewBase* mComposerBase )
{

    QList< QByteArray > charsets = mCodecAction->mimeCharsets();
    if( !mOriginalPreferredCharset.isEmpty() ) {
        charsets.insert( 0, mOriginalPreferredCharset );
    }
    mComposerBase->setFrom( from() );
    mComposerBase->setReplyTo( replyTo() );
    mComposerBase->setSubject( subject() );
    mComposerBase->setCharsets( charsets );
    mComposerBase->setUrgent( mUrgentAction->isChecked() );
    mComposerBase->setMDNRequested( mRequestMDNAction->isChecked() );
}


void KMComposeWin::doDelayedSend( MessageComposer::MessageSender::SendMethod method, MessageComposer::MessageSender::SaveIn saveIn )
{
#ifndef QT_NO_CURSOR
    MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );
#endif
    applyComposerSetting( mComposerBase );
    if ( mForceDisableHtml )
        disableHtml( MessageComposer::ComposerViewBase::NoConfirmationNeeded );
    bool sign = mSignAction->isChecked();
    bool encrypt = mEncryptAction->isChecked();

    mComposerBase->setCryptoOptions( sign, encrypt, cryptoMessageFormat(),
                                     ( ( saveIn != MessageComposer::MessageSender::SaveInNone && GlobalSettings::self()->neverEncryptDrafts() )
                                       || mSigningAndEncryptionExplicitlyDisabled ) );

    const int num = GlobalSettings::self()->customMessageHeadersCount();
    QMap<QByteArray, QString> customHeader;
    for (int ix=0; ix<num; ++ix) {
        CustomMimeHeader customMimeHeader( QString::number(ix) );
        customMimeHeader.readConfig();
        customHeader.insert(customMimeHeader.custHeaderName().toLatin1(), customMimeHeader.custHeaderValue() );
    }

    QMapIterator<QByteArray, QString> extraCustomHeader(mExtraHeaders);
    while (extraCustomHeader.hasNext()) {
        extraCustomHeader.next();
        customHeader.insert(extraCustomHeader.key(), extraCustomHeader.value() );
    }

    mComposerBase->setCustomHeader( customHeader );
    mComposerBase->send( method, saveIn, false );
}


void KMComposeWin::slotSendLater()
{
    if ( !TransportManager::self()->showTransportCreationDialog( this, TransportManager::IfNoTransportExists ) )
        return;
    if ( !checkRecipientNumber() )
        return;
    if ( mComposerBase->editor()->checkExternalEditorFinished() ) {
        const bool wasRegistered = (SendLater::SendLaterUtil::sentLaterAgentWasRegistered() && SendLater::SendLaterUtil::sentLaterAgentEnabled());
        if (wasRegistered) {
            SendLater::SendLaterInfo *info = 0;
            QPointer<SendLater::SendLaterDialog> dlg = new SendLater::SendLaterDialog(info, this);
            if (dlg->exec()) {
                info = dlg->info();
                const SendLater::SendLaterDialog::SendLaterAction action = dlg->action();
                delete dlg;
                switch (action) {
                case SendLater::SendLaterDialog::Unknown:
                    kDebug()<<"Sendlater action \"Unknown\": Need to fix it.";
                    break;
                case SendLater::SendLaterDialog::Canceled:
                    return;
                    break;
                case SendLater::SendLaterDialog::PutInOutbox:
                    doSend( MessageComposer::MessageSender::SendLater );
                    break;
                case SendLater::SendLaterDialog::SendDeliveryAtTime:
                {
                    mComposerBase->setSendLaterInfo(info);
                    if (info->isRecurrence()) {
                        doSend( MessageComposer::MessageSender::SendLater, MessageComposer::MessageSender::SaveInTemplates );
                    } else {
                        doSend( MessageComposer::MessageSender::SendLater, MessageComposer::MessageSender::SaveInDrafts );
                    }
                    break;
                }
                }
            } else {
                delete dlg;
            }
        } else {
            doSend( MessageComposer::MessageSender::SendLater );
        }
    }
}


void KMComposeWin::slotSaveDraft()
{
    if ( mComposerBase->editor()->checkExternalEditorFinished() ) {
        doSend( MessageComposer::MessageSender::SendLater, MessageComposer::MessageSender::SaveInDrafts );
    }
}


void KMComposeWin::slotSaveTemplate()
{
    if ( mComposerBase->editor()->checkExternalEditorFinished() ) {
        doSend( MessageComposer::MessageSender::SendLater, MessageComposer::MessageSender::SaveInTemplates );
    }
}


void KMComposeWin::slotSendNowVia( QAction *item )
{
    const QList<int> availTransports= TransportManager::self()->transportIds();
    const int transport = item->data().toInt();
    if ( availTransports.contains( transport ) ) {
        mComposerBase->transportComboBox()->setCurrentTransport( transport );
        slotSendNow();
    }
}


void KMComposeWin::slotSendLaterVia( QAction *item )
{
    const QList<int> availTransports= TransportManager::self()->transportIds();
    const int transport = item->data().toInt();
    if ( availTransports.contains( transport ) ) {
        mComposerBase->transportComboBox()->setCurrentTransport( transport );
        slotSendLater();
    }
}


void KMComposeWin::sendNow(bool shortcutUsed)
{
    if ( !mComposerBase->editor()->checkExternalEditorFinished() ) {
        return;
    }
    if ( !TransportManager::self()->showTransportCreationDialog( this, TransportManager::IfNoTransportExists ) )
        return;
    if ( !checkRecipientNumber() )
        return;
    mSendNowByShortcutUsed = shortcutUsed;
    if( GlobalSettings::self()->checkSpellingBeforeSend()) {
        mComposerBase->editor()->forceSpellChecking();
    } else {
        slotCheckSendNow();
    }
}

void KMComposeWin::slotSendNowByShortcut()
{
    sendNow(true);
}

void KMComposeWin::slotSendNow()
{
    sendNow(false);
}

void KMComposeWin::confirmBeforeSend()
{
    const int rc = KMessageBox::warningYesNoCancel( mMainWidget,
                                                    i18n("About to send email..."),
                                                    i18n("Send Confirmation"),
                                                    KGuiItem( i18n("&Send Now") ),
                                                    KGuiItem( i18n("Send &Later") ) );

    if ( rc == KMessageBox::Yes ) {
        doSend( MessageComposer::MessageSender::SendImmediate );
    } else if ( rc == KMessageBox::No ) {
        doSend( MessageComposer::MessageSender::SendLater );
    }
}

void KMComposeWin::slotCheckSendNowStep2()
{
    if ( GlobalSettings::self()->confirmBeforeSend() ) {
        confirmBeforeSend();
    } else {
        if (mSendNowByShortcutUsed) {
            if (!GlobalSettings::self()->checkSendDefaultActionShortcut()) {
                ValidateSendMailShortcut validateShortcut(actionCollection(), this);
                if (!validateShortcut.validate()) {
                    return;
                }
            }
            if (GlobalSettings::self()->confirmBeforeSendWhenUseShortcut()) {
                confirmBeforeSend();
                return;
            }
        }
        doSend( MessageComposer::MessageSender::SendImmediate );
    }
}

void KMComposeWin::slotCheckSendNow()
{
    PotentialPhishingEmailJob *job = new PotentialPhishingEmailJob(this);
    KConfigGroup group( KGlobal::config(), "PotentialPhishing");
    const QStringList whiteList = group.readEntry("whiteList", QStringList());
    job->setEmailWhiteList(whiteList);
    QStringList lst;
    lst << mComposerBase->to();
    if (!mComposerBase->cc().isEmpty())
        lst << mComposerBase->cc().split(QLatin1Char(','));
    if (!mComposerBase->bcc().isEmpty())
        lst << mComposerBase->bcc().split(QLatin1Char(','));
    job->setEmails(lst);
    connect(job, SIGNAL(potentialPhishingEmailsFound(QStringList)), this, SLOT(slotPotentialPhishingEmailsFound(QStringList)));
    job->start();
}

void KMComposeWin::slotPotentialPhishingEmailsFound(const QStringList &list)
{
    if (list.isEmpty()) {
        slotCheckSendNowStep2();
    } else {
        mPotentialPhishingEmailWarning->setPotentialPhisingEmail(list);
    }
}

bool KMComposeWin::checkRecipientNumber() const
{
    const int thresHold = GlobalSettings::self()->recipientThreshold();
    if ( GlobalSettings::self()->tooManyRecipients() && mComposerBase->recipientsEditor()->recipients().count() > thresHold ) {
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


void KMComposeWin::slotHelp()
{
    KToolInvocation::invokeHelp();
}


void KMComposeWin::enableHtml()
{
    if ( mForceDisableHtml ) {
        disableHtml( MessageComposer::ComposerViewBase::NoConfirmationNeeded );
        return;
    }

    mComposerBase->editor()->enableRichTextMode();
    if ( !toolBar( QLatin1String("htmlToolBar") )->isVisible() ) {
        // Use singleshot, as we we might actually be called from a slot that wanted to disable the
        // toolbar (but the messagebox in disableHtml() prevented that and called us).
        // The toolbar can't correctly deal with being enabled right in a slot called from the "disabled"
        // signal, so wait one event loop run for that.
        QTimer::singleShot( 0, toolBar( QLatin1String("htmlToolBar") ), SLOT(show()) );
    }
    if ( !markupAction->isChecked() )
        markupAction->setChecked( true );

    mComposerBase->editor()->updateActionStates();
    mComposerBase->editor()->setActionsEnabled( true );
}




void KMComposeWin::disableHtml( MessageComposer::ComposerViewBase::Confirmation confirmation )
{
    bool forcePlainTextMarkup = false;
    if ( confirmation == MessageComposer::ComposerViewBase::LetUserConfirm && mComposerBase->editor()->isFormattingUsed() && !mForceDisableHtml ) {
        int choice = KMessageBox::warningYesNoCancel( this, i18n( "Turning HTML mode off "
                                                                  "will cause the text to lose the formatting. Are you sure?" ),
                                                      i18n( "Lose the formatting?" ), KGuiItem( i18n( "Lose Formatting" ) ), KGuiItem( i18n( "Add Markup Plain Text" ) ) , KStandardGuiItem::cancel(),
                                                      QLatin1String("LoseFormattingWarning") );

        switch(choice) {
        case KMessageBox::Cancel:
            enableHtml();
            return;
        case KMessageBox::No:
            forcePlainTextMarkup = true;
            break;
        case KMessageBox::Yes:
            break;
        }
    }

    mComposerBase->editor()->forcePlainTextMarkup(forcePlainTextMarkup);
    mComposerBase->editor()->switchToPlainText();
    mComposerBase->editor()->setActionsEnabled( false );

    slotUpdateFont();
    if ( toolBar( QLatin1String("htmlToolBar") )->isVisible() ) {
        // See the comment in enableHtml() why we use a singleshot timer, similar situation here.
        QTimer::singleShot( 0, toolBar( QLatin1String("htmlToolBar") ), SLOT(hide()) );
    }
    if ( markupAction->isChecked() )
        markupAction->setChecked( false );
}


void KMComposeWin::slotToggleMarkup()
{
    htmlToolBarVisibilityChanged( markupAction->isChecked() );
}


void KMComposeWin::slotTextModeChanged( MessageComposer::KMeditor::Mode mode )
{
    if ( mode == KMeditor::Plain )
        disableHtml( MessageComposer::ComposerViewBase::NoConfirmationNeeded ); // ### Can this happen at all?
    else
        enableHtml();
}


void KMComposeWin::htmlToolBarVisibilityChanged( bool visible )
{
    if ( visible )
        enableHtml();
    else
        disableHtml( MessageComposer::ComposerViewBase::LetUserConfirm );
}


void KMComposeWin::slotAutoSpellCheckingToggled( bool on )
{
    mAutoSpellCheckingAction->setChecked( on );
    if ( on != mComposerBase->editor()->checkSpellingEnabled() )
        mComposerBase->editor()->setCheckSpellingEnabled( on );
    if ( on != mEdtSubject->checkSpellingEnabled() )
        mEdtSubject->setCheckSpellingEnabled( on );
    mStatusBarLabelSpellCheckingChangeMode->setToggleMode(on);
}

void KMComposeWin::slotSpellCheckingStatus(const QString & status)
{
    statusBar()->changeItem( status, 0 );
    QTimer::singleShot( 2000, this, SLOT(slotSpellcheckDoneClearStatus()) );
}

void KMComposeWin::slotSpellcheckDoneClearStatus()
{
    statusBar()->changeItem(QString(), 0);
}


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
    bool wasModified(isModified());
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


    if ( ident.organization().isEmpty() ) {
        mMsg->organization()->clear();
    } else {
        KMime::Headers::Organization * const organization
                = new KMime::Headers::Organization( mMsg.get(), ident.organization(), "utf-8" );
        mMsg->setHeader( organization );
    }
    if ( !ident.isXFaceEnabled() || ident.xface().isEmpty() ) {
        mMsg->removeHeader( "X-Face" );
    } else {
        QString xface = ident.xface();
        if ( !xface.isEmpty() ) {
            int numNL = ( xface.length() - 1 ) / 70;
            for ( int i = numNL; i > 0; --i ) {
                xface.insert( i*70, QLatin1String("\n\t") );
            }
            KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-Face", mMsg.get(), xface, "utf-8" );
            mMsg->setHeader( header );
        }
    }
    // If the transport sticky checkbox is not checked, set the transport
    // from the new identity
    if ( !mBtnTransport->isChecked() && !mIgnoreStickyFields ) {
        const int transportId = ident.transport().isEmpty() ? -1 : ident.transport().toInt();
        const Transport *transport = TransportManager::self()->transportById( transportId, true );
        if ( !transport ) {
            mMsg->removeHeader( "X-KMail-Transport" );
            mComposerBase->transportComboBox()->setCurrentTransport( TransportManager::self()->defaultTransportId() );
        } else {
            KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-Transport", mMsg.get(), QString::number( transport->id() ), "utf-8" );
            mMsg->setHeader( header );
            mComposerBase->transportComboBox()->setCurrentTransport( transport->id() );
        }
    }

    const bool fccIsDisabled = ident.disabledFcc();
    if (fccIsDisabled) {
        KMime::Headers::Generic *header = new KMime::Headers::Generic( "X-KMail-FccDisabled", mMsg.get(), QLatin1String("true"), "utf-8" );
        mMsg->setHeader( header );
    } else {
        mMsg->removeHeader( "X-KMail-FccDisabled" );
    }
    mFccFolder->setEnabled(!fccIsDisabled);


    if ( !mBtnDictionary->isChecked() && !mIgnoreStickyFields ) {
        mComposerBase->dictionary()->setCurrentByDictionaryName( ident.dictionary() );
    }
    slotSpellCheckingLanguage( mComposerBase->dictionary()->currentDictionary() );
    if ( !mBtnFcc->isChecked() && !mPreventFccOverwrite ) {
        setFcc( ident.fcc() );
    }

    // if unmodified, apply new template, if one is set
    if ( !wasModified && !( ident.templates().isEmpty() && mCustomTemplate.isEmpty() ) &&
         !initalChange ) {
        applyTemplate( uoid, mId );
    } else {
        mComposerBase->identityChanged( ident, oldIdentity, false );
        mEdtSubject->setAutocorrectionLanguage(ident.autocorrectionLanguage());
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

    mCryptoModuleAction->setCurrentItem( format2cb(
                                             Kleo::stringToCryptoMessageFormat( ident.preferredCryptoMessageFormat() ) ) );
    slotSelectCryptoModule( true );

    mLastIdentityHasSigningKey = bNewIdentityHasSigningKey;
    mLastIdentityHasEncryptionKey = bNewIdentityHasEncryptionKey;
    const KPIMIdentities::Signature sig = const_cast<KPIMIdentities::Identity&>( ident ).signature();
    bool isEnabledSignature = sig.isEnabledSignature();
    mAppendSignature->setEnabled(isEnabledSignature);
    mPrependSignature->setEnabled(isEnabledSignature);
    mInsertSignatureAtCursorPosition->setEnabled(isEnabledSignature);

    mId = uoid;
    changeCryptoAction();
    // make sure the From and BCC fields are shown if necessary
    rethinkFields( false );
    setModified(wasModified);
}


void KMComposeWin::slotSpellcheckConfig()
{
    static_cast<KMComposerEditor *>(mComposerBase->editor())->showSpellConfigDialog( QLatin1String("kmail2rc") );
}


void KMComposeWin::slotEditToolbars()
{
    saveMainWindowSettings( KMKernel::self()->config()->group( "Composer") );
    KEditToolBar dlg( guiFactory(), this );

    connect( &dlg, SIGNAL(newToolBarConfig()),
             SLOT(slotUpdateToolbars()) );

    dlg.exec();
}

void KMComposeWin::slotUpdateToolbars()
{
    createGUI( QLatin1String("kmcomposerui.rc") );
    applyMainWindowSettings( KMKernel::self()->config()->group( "Composer") );
}

void KMComposeWin::slotEditKeys()
{
    KShortcutsDialog::configure( actionCollection(),
                                 KShortcutsEditor::LetterShortcutsDisallowed );
}

void KMComposeWin::setFocusToEditor()
{
    // The cursor position is already set by setMsg(), so we only need to set the
    // focus here.
    mComposerBase->editor()->setFocus();
}

void KMComposeWin::setFocusToSubject()
{
    mEdtSubject->setFocus();
}

void KMComposeWin::slotCompletionModeChanged( KGlobalSettings::Completion mode )
{
    GlobalSettings::self()->setCompletionMode( (int) mode );

    // sync all the lineedits to the same completion mode
    mEdtFrom->setCompletionMode( mode );
    mEdtReplyTo->setCompletionMode( mode );
    mComposerBase->recipientsEditor()->setCompletionMode( mode );
}

void KMComposeWin::slotConfigChanged()
{
    readConfig( true /*reload*/);
    mComposerBase->updateAutoSave();
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
        mFolder = CommonKernel->draftsCollectionFolder();
        kDebug() << "restoring drafts to" << mFolder.id();
    }
}


void KMComposeWin::slotOverwriteModeChanged()
{
    const bool overwriteMode = mComposerBase->editor()->overwriteMode ();
    mComposerBase->editor()->setCursorWidth( overwriteMode ? 5 : 1 );
    mStatusBarLabelToggledOverrideMode->setToggleMode(overwriteMode);
}

void KMComposeWin::slotCursorPositionChanged()
{
    // Change Line/Column info in status bar
    int col, line;
    QString temp;
    line = mComposerBase->editor()->linePosition();
    col = mComposerBase->editor()->columnNumber();
    temp = i18nc("Shows the linenumber of the cursor position.", " Line: %1 ", line + 1 );
    statusBar()->changeItem( temp, 1 );
    temp = i18n( " Column: %1 ", col + 1 );
    statusBar()->changeItem( temp, 2 );

    // Show link target in status bar
    if ( mComposerBase->editor()->textCursor().charFormat().isAnchor() ) {
        const QString text = mComposerBase->editor()->currentLinkText();
        const QString url = mComposerBase->editor()->currentLinkUrl();
        statusBar()->changeItem( text + QLatin1String(" -> ") + url, 0 );
    }
    else {
        statusBar()->changeItem( QString(), 0 );
    }
}


void KMComposeWin::recipientEditorSizeHintChanged()
{
    QTimer::singleShot( 1, this, SLOT(setMaximumHeaderSize()) );
}

void KMComposeWin::setMaximumHeaderSize()
{
    mHeadersArea->setMaximumHeight( mHeadersArea->sizeHint().height() );
}

void KMComposeWin::updateSignatureAndEncryptionStateIndicators()
{
    mCryptoStateIndicatorWidget->updateSignatureAndEncrypionStateIndicators(mSignAction->isChecked(), mEncryptAction->isChecked());
}

void KMComposeWin::slotLanguageChanged( const QString &language )
{
    mComposerBase->dictionary()->setCurrentByDictionary( language );
}


void KMComposeWin::slotFccFolderChanged(const Akonadi::Collection& collection)
{
    mComposerBase->setFcc( collection );
    mComposerBase->editor()->document()->setModified(true);
}

void KMComposeWin::insertSpecialCharacter()
{
    if(!mSelectSpecialChar) {
        mSelectSpecialChar = new KPIMTextEdit::SelectSpecialChar(this);
        mSelectSpecialChar->setCaption(i18n("Insert Special Character"));
        mSelectSpecialChar->setOkButtonText(i18n("Insert"));
        connect(mSelectSpecialChar,SIGNAL(charSelected(QChar)),this,SLOT(charSelected(QChar)));
    }
    mSelectSpecialChar->show();
}

void KMComposeWin::charSelected(const QChar& c)
{
    mComposerBase->editor()->insertPlainText(c);
}

void KMComposeWin::slotSaveAsFile()
{
    SaveAsFileJob *job = new SaveAsFileJob(this);
    job->setParentWidget(this);
    job->setHtmlMode(mComposerBase->editor()->textMode() == KMeditor::Rich);
    job->setEditor(mComposerBase->editor());
    job->start();
    //not necessary to delete it. It done in SaveAsFileJob
}

void KMComposeWin::slotCreateAddressBookContact()
{
    CreateNewContactJob *job = new CreateNewContactJob( this, this );
    job->start();
}

void KMComposeWin::slotAttachMissingFile()
{
    mComposerBase->attachmentController()->showAddAttachmentFileDialog();
}

void KMComposeWin::slotVerifyMissingAttachmentTimeout()
{
    if( mComposerBase->hasMissingAttachments( GlobalSettings::self()->attachmentKeywords() )) {
        mAttachmentMissing->animatedShow();
    }
}

void KMComposeWin::slotExplicitClosedMissingAttachment()
{
    if(m_verifyMissingAttachment) {
        m_verifyMissingAttachment->stop();
        delete m_verifyMissingAttachment;
        m_verifyMissingAttachment = 0;
    }
}

void KMComposeWin::addExtraCustomHeaders( const QMap<QByteArray, QString> &headers)
{
    mExtraHeaders = headers;
}

void KMComposeWin::slotSentenceCase()
{
    QTextCursor textCursor = mComposerBase->editor()->textCursor();
    PimCommon::EditorUtil editorUtil;
    editorUtil.sentenceCase(textCursor);
}

void KMComposeWin::slotUpperCase()
{
    PimCommon::EditorUtil editorUtil;
    QTextCursor textCursor = mComposerBase->editor()->textCursor();
    editorUtil.upperCase(textCursor);
}

void KMComposeWin::slotLowerCase()
{
    QTextCursor textCursor = mComposerBase->editor()->textCursor();
    PimCommon::EditorUtil editorUtil;
    editorUtil.lowerCase(textCursor);
}

void KMComposeWin::slotExternalEditorStarted()
{
    mComposerBase->identityCombo()->setEnabled(false);
    mExternalEditorWarning->show();
}

void KMComposeWin::slotExternalEditorClosed()
{
    mComposerBase->identityCombo()->setEnabled(true);
    mExternalEditorWarning->hide();
}

void KMComposeWin::slotInsertShortUrl(const QString &url)
{
    mComposerBase->editor()->insertLink(url);
}

void KMComposeWin::slotShareLinkDone(const QString &link)
{
    mComposerBase->editor()->insertShareLink(link);
}

void KMComposeWin::slotTransportChanged()
{
    mComposerBase->editor()->document()->setModified(true);
}

void KMComposeWin::slotFollowUpMail(bool toggled)
{
    if (toggled) {
        QPointer<FollowUpReminderSelectDateDialog> dlg = new FollowUpReminderSelectDateDialog(this);
        if (dlg->exec()) {
            mComposerBase->setFollowUpDate(dlg->selectedDate());
            mComposerBase->setFollowUpCollection(dlg->collection());
        } else {
            mFollowUpToggleAction->setChecked(false);
        }
        delete dlg;
    } else {
        mComposerBase->clearFollowUp();
    }
}

void KMComposeWin::slotSnippetWidgetVisibilityChanged(bool b)
{
    mSnippetWidget->setVisible(b);
    mSnippetSplitterCollapser->setVisible(b);
}

void KMComposeWin::slotOverwriteModeWasChanged(bool state)
{
    mComposerBase->editor()->setCursorWidth( state ? 5 : 1 );
    mComposerBase->editor()->setOverwriteMode (state);
}
