/*
 * This file is part of KMail.
 * Copyright (c) 2011-2019 Laurent Montel <montel@kde.org>
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
#include "kmcomposerwin.h"
// KMail includes
#include "attachment/attachmentcontroller.h"
#include "attachment/attachmentview.h"
#include "codec/codecaction.h"
#include "custommimeheader.h"
#include "editor/kmcomposereditorng.h"
#include "editor/plugininterface/kmailplugineditorcheckbeforesendmanagerinterface.h"
#include "editor/plugininterface/kmailplugineditorinitmanagerinterface.h"
#include "editor/plugininterface/kmailplugineditormanagerinterface.h"
#include "editor/plugininterface/kmailplugineditorconverttextmanagerinterface.h"
#include "editor/plugininterface/kmailplugingrammareditormanagerinterface.h"
#include "editor/potentialphishingemail/potentialphishingemailjob.h"
#include "editor/potentialphishingemail/potentialphishingemailwarning.h"
#include "editor/warningwidgets/incorrectidentityfolderwarning.h"
#include "job/addressvalidationjob.h"
#include "job/createnewcontactjob.h"
#include "job/saveasfilejob.h"
#include "job/savedraftjob.h"
#include "job/dndfromarkjob.h"
#include "kconfigwidgets_version.h"
#include "kmail_debug.h"
#include "kmcommands.h"
#include "kmcomposercreatenewcomposerjob.h"
#include "kmcomposerglobalaction.h"
#include "kmcomposerupdatetemplatejob.h"
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "kmmainwin.h"
#include "mailcomposeradaptor.h" // TODO port all D-Bus stuff...
#include "settings/kmailsettings.h"
#include "templatesconfiguration_kfg.h"
#include "util.h"
#include "validatesendmailshortcut.h"
#include "warningwidgets/attachmentmissingwarning.h"
#include "warningwidgets/externaleditorwarning.h"
#include "widgets/cryptostateindicatorwidget.h"
#include "widgets/kactionmenutransport.h"
#include "undosend/undosendmanager.h"
#include <Akonadi/Contact/ContactGroupExpandJob>
#include <Akonadi/KMime/MessageFlags>
#include <Akonadi/KMime/MessageStatus>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/Monitor>
#include <AkonadiCore/changerecorder.h>

#include <FollowupReminder/FollowUpReminderUtil>

#include <KContacts/VCardConverter>

#include <KIdentityManagement/Identity>
#include <KIdentityManagement/IdentityCombo>
#include <KIdentityManagement/IdentityManager>
#include <KIdentityManagement/Signature>

#include <KMime/Message>

#include <KPIMTextEdit/EditorUtil>
#include <KPIMTextEdit/RichTextComposerActions>
#include <KPIMTextEdit/RichTextComposerControler>
#include <KPIMTextEdit/RichTextComposerImages>
#include <KPIMTextEdit/RichTextEditorWidget>
#include <KPIMTextEdit/RichTextExternalComposer>

#include <Libkdepim/ProgressStatusBarWidget>
#include <Libkdepim/StatusbarProgressWidget>

#ifndef QT_NO_CURSOR
#include <Libkdepim/KCursorSaver>
#endif

#include <Libkleo/KeySelectionDialog>
#include <Libkleo/ProgressDialog>

#include <MailCommon/FolderCollectionMonitor>
#include <MailCommon/FolderRequester>
#include <MailCommon/FolderSettings>
#include <MailCommon/MailKernel>
#include <MailCommon/SnippetTreeView>
#include <MailCommon/SnippetsManager>

#include <MailTransport/Transport>
#include <MailTransport/TransportComboBox>
#include <MailTransport/TransportManager>

#include <MessageComposer/AttachmentModel>
#include <MessageComposer/Composer>
#include <MessageComposer/ComposerLineEdit>
#include <MessageComposer/FollowUpReminderSelectDateDialog>
#include <MessageComposer/FollowupReminderCreateJob>
#include <MessageComposer/GlobalPart>
#include <MessageComposer/InfoPart>
#include <MessageComposer/InsertTextFileJob>
#include <MessageComposer/Kleo_Util>
#include <MessageComposer/MessageComposerSettings>
#include <MessageComposer/MessageHelper>
#include <MessageComposer/PluginEditorCheckBeforeSendParams>
#include <MessageComposer/PluginEditorInterface>
#include <MessageComposer/PluginActionType>
#include <MessageComposer/RichTextComposerSignatures>
#include <MessageComposer/SignatureController>
#include <MessageComposer/TextPart>
#include <MessageComposer/Util>
#include <MessageComposer/StatusBarLabelToggledState>
#include <MessageComposer/ConvertSnippetVariablesJob>
#include <MessageComposer/ComposerViewInterface>

#include <Sonnet/DictionaryComboBox>

#include <MessageCore/AttachmentPart>
#include <MessageCore/MessageCoreSettings>
#include <MessageCore/NodeHelper>
#include <MessageCore/StringUtil>

#include <MessageViewer/MessageViewerSettings>
#include <MessageViewer/Stl_Util>

#include <MimeTreeParser/NodeHelper>
#include <MimeTreeParser/ObjectTreeParser>
#include <MimeTreeParser/SimpleObjectTreeSource>

#include <PimCommon/CustomToolsWidgetng>
#include <PimCommon/KActionMenuChangeCase>
#include <PimCommon/LineEditWithAutoCorrection>
#include <PimCommon/CustomToolsPluginManager>

#include <SendLater/SendLaterDialog>
#include <SendLater/SendLaterInfo>
#include <SendLater/SendLaterUtil>

#include <TemplateParser/TemplateParserJob>
#include <TemplateParser/TemplatesConfiguration>

#include <QGpgME/Protocol>
#include <QGpgME/ExportJob>
#include <QGpgME/KeyForMailboxJob>

// KDE Frameworks includes
#include <KActionCollection>
#include <KActionMenu>
#include <KCharsets>
#include <KConfigGroup>
#include <KEditToolBar>
#include <KEmailAddress>
#include <KEncodingFileDialog>
#include <KIO/JobUiDelegate>
#include <KIconUtils>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KRun>
#include <KShortcutsDialog>
#include <KSplitterCollapserButton>
#include <KStandardShortcut>
#include <KToggleAction>
#include <KToolBar>
#include <QDBusConnection>
#include <sonnet_version.h>
#include <KXMLGUIFactory>
// Qt includes
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QShortcut>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QMenuBar>
#include <MessageComposer/PluginEditorConverterInitialData>
#include <MessageComposer/PluginEditorConverterBeforeConvertingData>

// GPGME
#include <gpgme++/keylistresult.h>
#include <gpgme++/key.h>

using Sonnet::DictionaryComboBox;
using MailTransport::TransportManager;
using MailTransport::Transport;

Q_DECLARE_METATYPE(MessageComposer::Recipient::Ptr)

KMail::Composer *KMail::makeComposer(const KMime::Message::Ptr &msg, bool lastSignState, bool lastEncryptState, Composer::TemplateContext context, uint identity, const QString &textSelection,
                                     const QString &customTemplate)
{
    return KMComposerWin::create(msg, lastSignState, lastEncryptState, context, identity, textSelection, customTemplate);
}

KMail::Composer *KMComposerWin::create(const KMime::Message::Ptr &msg, bool lastSignState, bool lastEncryptState, Composer::TemplateContext context, uint identity, const QString &textSelection, const QString &customTemplate)
{
    return new KMComposerWin(msg, lastSignState, lastEncryptState, context, identity, textSelection, customTemplate);
}

int KMComposerWin::s_composerNumber = 0;

KMComposerWin::KMComposerWin(const KMime::Message::Ptr &aMsg, bool lastSignState, bool lastEncryptState, Composer::TemplateContext context, uint id, const QString &textSelection, const QString &customTemplate)
    : KMail::Composer(QStringLiteral("kmail-composer#"))
    , mTextSelection(textSelection)
    , mCustomTemplate(customTemplate)
    , mFolder(Akonadi::Collection(-1))
    , mId(id)
    , mContext(context)
{
    mGlobalAction = new KMComposerGlobalAction(this, this);
    mComposerBase = new MessageComposer::ComposerViewBase(this, this);
    mComposerBase->setIdentityManager(kmkernel->identityManager());

    mPluginEditorManagerInterface = new KMailPluginEditorManagerInterface(this);
    connect(mPluginEditorManagerInterface, &KMailPluginEditorManagerInterface::message, this, &KMComposerWin::slotMessage);
    connect(mPluginEditorManagerInterface, &KMailPluginEditorManagerInterface::insertText, this, &KMComposerWin::slotEditorPluginInsertText);
    mPluginEditorCheckBeforeSendManagerInterface = new KMailPluginEditorCheckBeforeSendManagerInterface(this);
    mPluginEditorInitManagerInterface = new KMailPluginEditorInitManagerInterface(this);
    mPluginEditorConvertTextManagerInterface = new KMailPluginEditorConvertTextManagerInterface(this);

    mPluginEditorGrammarManagerInterface = new KMailPluginGrammarEditorManagerInterface(this);

    connect(mComposerBase, &MessageComposer::ComposerViewBase::disableHtml, this, &KMComposerWin::disableHtml);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::enableHtml, this, &KMComposerWin::enableHtml);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::failed, this, &KMComposerWin::slotSendFailed);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::sentSuccessfully, this, &KMComposerWin::slotSendSuccessful);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::modified, this, &KMComposerWin::setModified);

    (void)new MailcomposerAdaptor(this);
    mdbusObjectPath = QLatin1String("/Composer_") + QString::number(++s_composerNumber);
    QDBusConnection::sessionBus().registerObject(mdbusObjectPath, this);

    MessageComposer::SignatureController *sigController = new MessageComposer::SignatureController(this);
    connect(sigController, &MessageComposer::SignatureController::enableHtml, this, &KMComposerWin::enableHtml);
    mComposerBase->setSignatureController(sigController);

    if (!kmkernel->xmlGuiInstanceName().isEmpty()) {
        setComponentName(kmkernel->xmlGuiInstanceName(), i18n("KMail2"));
    }
    mMainWidget = new QWidget(this);
    // splitter between the headers area and the actual editor
    mHeadersToEditorSplitter = new QSplitter(Qt::Vertical, mMainWidget);
    mHeadersToEditorSplitter->setObjectName(QStringLiteral("mHeadersToEditorSplitter"));
    mHeadersToEditorSplitter->setChildrenCollapsible(false);
    mHeadersArea = new QWidget(mHeadersToEditorSplitter);
    mHeadersArea->setSizePolicy(mHeadersToEditorSplitter->sizePolicy().horizontalPolicy(),
                                QSizePolicy::Expanding);
    mHeadersToEditorSplitter->addWidget(mHeadersArea);
    const QList<int> defaultSizes{
        0
    };
    mHeadersToEditorSplitter->setSizes(defaultSizes);

    QVBoxLayout *v = new QVBoxLayout(mMainWidget);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(mHeadersToEditorSplitter);
    KIdentityManagement::IdentityCombo *identity = new KIdentityManagement::IdentityCombo(kmkernel->identityManager(),
                                                                                          mHeadersArea);
    identity->setCurrentIdentity(mId);
    connect(identity, &KIdentityManagement::IdentityCombo::identityDeleted, this, &KMComposerWin::slotIdentityDeleted);
    connect(identity, &KIdentityManagement::IdentityCombo::invalidIdentity, this, &KMComposerWin::slotInvalidIdentity);
    mComposerBase->setIdentityCombo(identity);

    sigController->setIdentityCombo(identity);
    sigController->suspend(); // we have to do identity change tracking ourselves due to the template code

    Sonnet::DictionaryComboBox *dictionaryCombo = new DictionaryComboBox(mHeadersArea);
    dictionaryCombo->setToolTip(i18n("Select the dictionary to use when spell-checking this message"));
    mComposerBase->setDictionary(dictionaryCombo);

    mFccFolder = new MailCommon::FolderRequester(mHeadersArea);
    mFccFolder->setNotAllowToCreateNewFolder(true);
    mFccFolder->setMustBeReadWrite(true);

    mFccFolder->setToolTip(i18n("Select the sent-mail folder where a copy of this message will be saved"));
    connect(mFccFolder, &MailCommon::FolderRequester::folderChanged, this, &KMComposerWin::slotFccFolderChanged);
    connect(mFccFolder, &MailCommon::FolderRequester::invalidFolder, this, &KMComposerWin::slotFccIsInvalid);

    MailTransport::TransportComboBox *transport = new MailTransport::TransportComboBox(mHeadersArea);
    transport->setToolTip(i18n("Select the outgoing account to use for sending this message"));
    mComposerBase->setTransportCombo(transport);
    connect(transport, qOverload<int>(&MailTransport::TransportComboBox::activated), this, &KMComposerWin::slotTransportChanged);
    connect(transport, &MailTransport::TransportComboBox::transportRemoved, this, &KMComposerWin::slotTransportRemoved);
    mEdtFrom = new MessageComposer::ComposerLineEdit(false, mHeadersArea);
    mEdtFrom->setObjectName(QStringLiteral("fromLine"));
    mEdtFrom->setRecentAddressConfig(MessageComposer::MessageComposerSettings::self()->config());
    mEdtFrom->setToolTip(i18n("Set the \"From:\" email address for this message"));

    MessageComposer::RecipientsEditor *recipientsEditor = new MessageComposer::RecipientsEditor(mHeadersArea);
    recipientsEditor->setRecentAddressConfig(MessageComposer::MessageComposerSettings::self()->config());
    connect(recipientsEditor, &MessageComposer::RecipientsEditor::completionModeChanged, this, &KMComposerWin::slotCompletionModeChanged);
    connect(recipientsEditor, &MessageComposer::RecipientsEditor::sizeHintChanged, this, &KMComposerWin::recipientEditorSizeHintChanged);
    connect(recipientsEditor, &MessageComposer::RecipientsEditor::lineAdded, this, &KMComposerWin::slotRecipientEditorLineAdded);
    mComposerBase->setRecipientsEditor(recipientsEditor);

    mEdtSubject = new PimCommon::LineEditWithAutoCorrection(mHeadersArea, QStringLiteral("kmail2rc"));
    mEdtSubject->setActivateLanguageMenu(false);
    mEdtSubject->setToolTip(i18n("Set a subject for this message"));
    mEdtSubject->setAutocorrection(KMKernel::self()->composerAutoCorrection());
    mLblIdentity = new QLabel(i18n("&Identity:"), mHeadersArea);
    mDictionaryLabel = new QLabel(i18n("&Dictionary:"), mHeadersArea);
    mLblFcc = new QLabel(i18n("&Sent-Mail folder:"), mHeadersArea);
    mLblTransport = new QLabel(i18n("&Mail transport:"), mHeadersArea);
    mLblFrom = new QLabel(i18nc("sender address field", "&From:"), mHeadersArea);
    mLblSubject = new QLabel(i18nc("@label:textbox Subject of email.", "S&ubject:"), mHeadersArea);
    mShowHeaders = KMailSettings::self()->headers();
    mDone = false;
    mGrid = nullptr;
    mFixedFontAction = nullptr;
    // the attachment view is separated from the editor by a splitter
    mSplitter = new QSplitter(Qt::Vertical, mMainWidget);
    mSplitter->setObjectName(QStringLiteral("mSplitter"));
    mSplitter->setChildrenCollapsible(false);
    mSnippetSplitter = new QSplitter(Qt::Horizontal, mSplitter);
    mSnippetSplitter->setObjectName(QStringLiteral("mSnippetSplitter"));
    mSplitter->addWidget(mSnippetSplitter);

    QWidget *editorAndCryptoStateIndicators = new QWidget(mSplitter);
    mCryptoStateIndicatorWidget = new CryptoStateIndicatorWidget(this);
    mCryptoStateIndicatorWidget->setShowAlwaysIndicator(KMailSettings::self()->showCryptoLabelIndicator());

    QVBoxLayout *vbox = new QVBoxLayout(editorAndCryptoStateIndicators);
    vbox->setContentsMargins(0, 0, 0, 0);

    mPotentialPhishingEmailWarning = new PotentialPhishingEmailWarning(this);
    connect(mPotentialPhishingEmailWarning, &PotentialPhishingEmailWarning::sendNow, this, &KMComposerWin::slotCheckSendNowStep2);
    vbox->addWidget(mPotentialPhishingEmailWarning);

    mAttachmentMissing = new AttachmentMissingWarning(this);
    connect(mAttachmentMissing, &AttachmentMissingWarning::attachMissingFile, this, &KMComposerWin::slotAttachMissingFile);
    connect(mAttachmentMissing, &AttachmentMissingWarning::explicitClosedMissingAttachment, this, &KMComposerWin::slotExplicitClosedMissingAttachment);
    vbox->addWidget(mAttachmentMissing);

    KMComposerEditorNg *composerEditorNg = new KMComposerEditorNg(this, mCryptoStateIndicatorWidget);
    mRichTextEditorwidget = new KPIMTextEdit::RichTextEditorWidget(composerEditorNg, mCryptoStateIndicatorWidget);

    connect(composerEditorNg, &KMComposerEditorNg::insertEmoticon, mGlobalAction, &KMComposerGlobalAction::slotInsertEmoticon);
    //Don't use new connect api here. It crashes
    connect(composerEditorNg, SIGNAL(textChanged()), this, SLOT(slotEditorTextChanged()));
    connect(composerEditorNg, &KMComposerEditorNg::selectionChanged, this, &KMComposerWin::slotSelectionChanged);
    //connect(editor, &KMComposerEditor::textChanged, this, &KMComposeWin::slotEditorTextChanged);
    mComposerBase->setEditor(composerEditorNg);
    mIncorrectIdentityFolderWarning = new IncorrectIdentityFolderWarning(this);
    vbox->addWidget(mIncorrectIdentityFolderWarning);

    vbox->addWidget(mCryptoStateIndicatorWidget);
    vbox->addWidget(mRichTextEditorwidget);

    mSnippetSplitter->insertWidget(0, editorAndCryptoStateIndicators);
    mSnippetSplitter->setOpaqueResize(true);
    sigController->setEditor(composerEditorNg);

    mHeadersToEditorSplitter->addWidget(mSplitter);
    composerEditorNg->setAcceptDrops(true);
    connect(sigController, &MessageComposer::SignatureController::signatureAdded,
            mComposerBase->editor()->externalComposer(), &KPIMTextEdit::RichTextExternalComposer::startExternalEditor);

    connect(dictionaryCombo, &Sonnet::DictionaryComboBox::dictionaryChanged, this, &KMComposerWin::slotSpellCheckingLanguage);

    connect(composerEditorNg, &KMComposerEditorNg::languageChanged, this, &KMComposerWin::slotDictionaryLanguageChanged);
    connect(composerEditorNg, &KMComposerEditorNg::spellCheckStatus, this, &KMComposerWin::slotSpellCheckingStatus);
    connect(composerEditorNg, &KMComposerEditorNg::insertModeChanged, this, &KMComposerWin::slotOverwriteModeChanged);
    connect(composerEditorNg, &KMComposerEditorNg::spellCheckingFinished, this, &KMComposerWin::slotDelayedCheckSendNow);
    mSnippetWidget = new MailCommon::SnippetTreeView(actionCollection(), mSnippetSplitter);
    connect(mSnippetWidget, &MailCommon::SnippetTreeView::insertSnippetInfo, this, &KMComposerWin::insertSnippetInfo);
    connect(composerEditorNg, &KMComposerEditorNg::insertSnippet, mSnippetWidget->snippetsManager(), &MailCommon::SnippetsManager::insertSnippet);
    mSnippetWidget->setVisible(KMailSettings::self()->showSnippetManager());
    mSnippetSplitter->addWidget(mSnippetWidget);
    mSnippetSplitter->setCollapsible(0, false);
    mSnippetSplitterCollapser = new KSplitterCollapserButton(mSnippetWidget, mSnippetSplitter);
    mSnippetSplitterCollapser->setVisible(KMailSettings::self()->showSnippetManager());

    mSplitter->setOpaqueResize(true);

    setWindowTitle(i18nc("@title:window", "Composer"));
    setMinimumSize(200, 200);

    mCustomToolsWidget = new PimCommon::CustomToolsWidgetNg(this);
    mCustomToolsWidget->initializeView(actionCollection(), PimCommon::CustomToolsPluginManager::self()->pluginsList());
    mSplitter->addWidget(mCustomToolsWidget);
    connect(mCustomToolsWidget, &PimCommon::CustomToolsWidgetNg::insertText, this, &KMComposerWin::slotInsertShortUrl);

    MessageComposer::AttachmentModel *attachmentModel = new MessageComposer::AttachmentModel(this);
    KMail::AttachmentView *attachmentView = new KMail::AttachmentView(attachmentModel, mSplitter);
    attachmentView->hideIfEmpty();
    connect(attachmentView, &KMail::AttachmentView::modified, this, &KMComposerWin::setModified);
    KMail::AttachmentController *attachmentController = new KMail::AttachmentController(attachmentModel, attachmentView, this);

    mComposerBase->setAttachmentModel(attachmentModel);
    mComposerBase->setAttachmentController(attachmentController);

    if (KMailSettings::self()->showForgottenAttachmentWarning()) {
        mVerifyMissingAttachment = new QTimer(this);
        mVerifyMissingAttachment->setSingleShot(true);
        mVerifyMissingAttachment->setInterval(1000 * 5);
        connect(mVerifyMissingAttachment, &QTimer::timeout, this, &KMComposerWin::slotVerifyMissingAttachmentTimeout);
    }
    connect(attachmentController, &KMail::AttachmentController::fileAttached, mAttachmentMissing, &AttachmentMissingWarning::slotFileAttached);

    mExternalEditorWarning = new ExternalEditorWarning(this);
    v->addWidget(mExternalEditorWarning);

    mPluginEditorManagerInterface->setParentWidget(this);
    mPluginEditorManagerInterface->setRichTextEditor(mRichTextEditorwidget->editor());
    mPluginEditorManagerInterface->setActionCollection(actionCollection());
    mPluginEditorManagerInterface->setComposerInterface(mComposerBase);

    mPluginEditorCheckBeforeSendManagerInterface->setParentWidget(this);

    mPluginEditorInitManagerInterface->setParentWidget(this);
    mPluginEditorInitManagerInterface->setRichTextEditor(composerEditorNg);

    mPluginEditorConvertTextManagerInterface->setParentWidget(this);
    mPluginEditorConvertTextManagerInterface->setActionCollection(actionCollection());
    mPluginEditorConvertTextManagerInterface->setRichTextEditor(composerEditorNg);

    mPluginEditorGrammarManagerInterface->setParentWidget(this);
    mPluginEditorGrammarManagerInterface->setActionCollection(actionCollection());
    mPluginEditorGrammarManagerInterface->setRichTextEditor(composerEditorNg);
    mPluginEditorGrammarManagerInterface->setCustomToolsWidget(mCustomToolsWidget);

    setupStatusBar(attachmentView->widget());
    setupActions();
    setupEditor();
    rethinkFields();
    readConfig();

    updateSignatureAndEncryptionStateIndicators();

    applyMainWindowSettings(KMKernel::self()->config()->group("Composer"));

    mUpdateWindowTitleConnection = connect(mEdtSubject, &PimCommon::LineEditWithAutoCorrection::textChanged, this, &KMComposerWin::slotUpdateWindowTitle);
    mIdentityConnection = connect(identity, &KIdentityManagement::IdentityCombo::identityChanged, [this](uint val) {
        slotIdentityChanged(val);
    });
    connect(kmkernel->identityManager(), qOverload<uint>(&KIdentityManagement::IdentityManager::changed), this, [this](uint val) {
        if (mComposerBase->identityCombo()->currentIdentity() == val) {
            slotIdentityChanged(val);
        }
    });

    connect(mEdtFrom, &MessageComposer::ComposerLineEdit::completionModeChanged, this, &KMComposerWin::slotCompletionModeChanged);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionRemoved, this, &KMComposerWin::slotFolderRemoved);
    connect(kmkernel, &KMKernel::configChanged, this, &KMComposerWin::slotConfigChanged);

    mMainWidget->resize(800, 600);
    setCentralWidget(mMainWidget);

    if (KMailSettings::self()->useHtmlMarkup()) {
        enableHtml();
    } else {
        disableHtml(MessageComposer::ComposerViewBase::LetUserConfirm);
    }

    if (KMailSettings::self()->useExternalEditor()) {
        composerEditorNg->setUseExternalEditor(true);
        composerEditorNg->setExternalEditorPath(KMailSettings::self()->externalEditor());
    }

    const QList<KPIM::MultiplyingLine *> lstLines = recipientsEditor->lines();
    for (KPIM::MultiplyingLine *line : lstLines) {
        slotRecipientEditorLineAdded(line);
    }

    if (aMsg) {
        setMessage(aMsg, lastSignState, lastEncryptState);
    }

    mComposerBase->recipientsEditor()->setFocusBottom();
    composerEditorNg->composerActions()->updateActionStates(); // set toolbar buttons to correct values

    mDone = true;

    mDummyComposer = new MessageComposer::Composer(this);
    mDummyComposer->globalPart()->setParentWidgetForGui(this);

    KConfigGroup grp(KMKernel::self()->config()->group("Composer"));
    setAutoSaveSettings(grp, true);
}

KMComposerWin::~KMComposerWin()
{
    disconnect(mUpdateWindowTitleConnection);
    // When we have a collection set, store the message back to that collection.
    // Note that when we save the message or sent it, mFolder is set back to 0.
    // So this for example kicks in when opening a draft and then closing the window.
    if (mFolder.isValid() && mMsg && isModified()) {
        SaveDraftJob *saveDraftJob = new SaveDraftJob(mMsg, mFolder);
        saveDraftJob->start();
    }

    delete mComposerBase;
}

void KMComposerWin::insertSnippetInfo(const MailCommon::SnippetInfo &info)
{
    {
        if (!info.to.isEmpty()) {
            const QStringList lst = info.to.split(QLatin1Char(','));
            for (const QString &addr : lst) {
                mComposerBase->recipientsEditor()->addRecipient(addr, MessageComposer::Recipient::To);
            }
        }
    }
    {
        if (!info.cc.isEmpty()) {
            const QStringList lst = info.cc.split(QLatin1Char(','));
            for (const QString &addr : lst) {
                mComposerBase->recipientsEditor()->addRecipient(addr, MessageComposer::Recipient::Cc);
            }
        }
    }
    {
        if (!info.bcc.isEmpty()) {
            const QStringList lst = info.bcc.split(QLatin1Char(','));
            for (const QString &addr : lst) {
                mComposerBase->recipientsEditor()->addRecipient(addr, MessageComposer::Recipient::Bcc);
            }
        }
    }
    {
        if (!info.attachment.isEmpty()) {
            const QStringList lst = info.attachment.split(QLatin1Char(','));
            for (const QString &attach : lst) {
                const QUrl localUrl = QUrl::fromLocalFile(attach);
                addAttachment(localUrl, QString());
            }
        }
    }
    {
        if (!info.subject.isEmpty()) {
            //Convert subject
            MessageComposer::ConvertSnippetVariablesJob *job = new MessageComposer::ConvertSnippetVariablesJob(this);
            job->setText(info.subject);
            MessageComposer::ComposerViewInterface *interface = new MessageComposer::ComposerViewInterface(mComposerBase);
            job->setComposerViewInterface(interface);
            connect(job, &MessageComposer::ConvertSnippetVariablesJob::textConverted, this, [this](const QString &str) {
                if (!str.isEmpty()) {
                    if (mComposerBase->subject().isEmpty()) { //Add subject only if we don't have subject
                        mEdtSubject->setText(str);
                    }
                }
            });
            job->start();
        }
    }
    {
        if (!info.text.isEmpty()) {
            //Convert plain text
            MessageComposer::ConvertSnippetVariablesJob *job = new MessageComposer::ConvertSnippetVariablesJob(this);
            job->setText(info.text);
            MessageComposer::ComposerViewInterface *interface = new MessageComposer::ComposerViewInterface(mComposerBase);
            job->setComposerViewInterface(interface);
            connect(job, &MessageComposer::ConvertSnippetVariablesJob::textConverted, this, [this](const QString &str) {
                mComposerBase->editor()->insertPlainText(str);
            });
            job->start();
        }
    }
}

void KMComposerWin::slotSpellCheckingLanguage(const QString &language)
{
    mComposerBase->editor()->setSpellCheckingLanguage(language);
    mEdtSubject->setSpellCheckingLanguage(language);
}

QString KMComposerWin::dbusObjectPath() const
{
    return mdbusObjectPath;
}

void KMComposerWin::slotEditorTextChanged()
{
    const bool textIsNotEmpty = !mComposerBase->editor()->document()->isEmpty();
    mFindText->setEnabled(textIsNotEmpty);
    mFindNextText->setEnabled(textIsNotEmpty);
    mReplaceText->setEnabled(textIsNotEmpty);
    mSelectAll->setEnabled(textIsNotEmpty);
    if (mVerifyMissingAttachment && !mVerifyMissingAttachment->isActive()) {
        mVerifyMissingAttachment->start();
    }
}

void KMComposerWin::send(int how)
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

void KMComposerWin::addAttachmentsAndSend(const QList<QUrl> &urls, const QString &comment, int how)
{
    const int nbUrl = urls.count();
    for (int i = 0; i < nbUrl; ++i) {
        mComposerBase->addAttachment(urls.at(i), comment, true);
    }

    send(how);
}

void KMComposerWin::addAttachment(const QUrl &url, const QString &comment)
{
    mComposerBase->addAttachment(url, comment, false);
}

void KMComposerWin::addAttachment(const QString &name, KMime::Headers::contentEncoding cte, const QString &charset, const QByteArray &data, const QByteArray &mimeType)
{
    Q_UNUSED(cte);
    mComposerBase->addAttachment(name, name, charset, data, mimeType);
}

void KMComposerWin::readConfig(bool reload /* = false */)
{
    mEdtFrom->setCompletionMode((KCompletion::CompletionMode)KMailSettings::self()->completionMode());
    mComposerBase->recipientsEditor()->setCompletionMode((KCompletion::CompletionMode)KMailSettings::self()->completionMode());

    if (MessageCore::MessageCoreSettings::self()->useDefaultFonts()) {
        mBodyFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        mFixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    } else {
        mBodyFont = KMailSettings::self()->composerFont();
        mFixedFont = MessageViewer::MessageViewerSettings::self()->fixedFont();
    }

    slotUpdateFont();
    mEdtFrom->setFont(mBodyFont);
    mEdtSubject->setFont(mBodyFont);

    if (!reload) {
        QSize siz = KMailSettings::self()->composerSize();
        if (siz.width() < 200) {
            siz.setWidth(200);
        }
        if (siz.height() < 200) {
            siz.setHeight(200);
        }
        resize(siz);

        if (!KMailSettings::self()->snippetSplitterPosition().isEmpty()) {
            mSnippetSplitter->setSizes(KMailSettings::self()->snippetSplitterPosition());
        } else {
            QList<int> defaults;
            defaults << (int)(width() * 0.8) << (int)(width() * 0.2);
            mSnippetSplitter->setSizes(defaults);
        }
    }

    mComposerBase->identityCombo()->setCurrentIdentity(mId);
    qCDebug(KMAIL_LOG) << mComposerBase->identityCombo()->currentIdentityName();
    const KIdentityManagement::Identity &ident
        = kmkernel->identityManager()->identityForUoid(mId);

    mComposerBase->setAutoSaveInterval(KMailSettings::self()->autosaveInterval() * 1000 * 60);

    mComposerBase->dictionary()->setCurrentByDictionaryName(ident.dictionary());

    QString fccName;
    if (!ident.fcc().isEmpty()) {
        fccName = ident.fcc();
    }
    setFcc(fccName);
}

void KMComposerWin::writeConfig()
{
    KMailSettings::self()->setHeaders(mShowHeaders);
    KMailSettings::self()->setCurrentTransport(mComposerBase->transportComboBox()->currentText());
    KMailSettings::self()->setPreviousIdentity(mComposerBase->identityCombo()->currentIdentity());
    KMailSettings::self()->setPreviousFcc(QString::number(mFccFolder->collection().id()));
    KMailSettings::self()->setPreviousDictionary(mComposerBase->dictionary()->currentDictionaryName());
    KMailSettings::self()->setAutoSpellChecking(mAutoSpellCheckingAction->isChecked());
    MessageViewer::MessageViewerSettings::self()->setUseFixedFont(mFixedFontAction->isChecked());
    if (!mForceDisableHtml) {
        KMailSettings::self()->setUseHtmlMarkup(mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich);
    }
    KMailSettings::self()->setComposerSize(size());
    KMailSettings::self()->setShowSnippetManager(mSnippetAction->isChecked());

    if (mSnippetAction->isChecked()) {
        KMailSettings::setSnippetSplitterPosition(mSnippetSplitter->sizes());
    }

    // make sure config changes are written to disk, cf. bug 127538
    KMKernel::self()->slotSyncConfig();
}

MessageComposer::Composer *KMComposerWin::createSimpleComposer()
{
    QVector< QByteArray > charsets = mCodecAction->mimeCharsets();
    if (!mOriginalPreferredCharset.isEmpty()) {
        charsets.insert(0, mOriginalPreferredCharset);
    }
    mComposerBase->setFrom(from());
    mComposerBase->setSubject(subject());
    mComposerBase->setCharsets(charsets);
    return mComposerBase->createSimpleComposer();
}

bool KMComposerWin::canSignEncryptAttachments() const
{
    return cryptoMessageFormat() != Kleo::InlineOpenPGPFormat;
}

void KMComposerWin::slotUpdateView()
{
    if (!mDone) {
        return; // otherwise called from rethinkFields during the construction
        // which is not the intended behavior
    }

    //This sucks awfully, but no, I cannot get an activated(int id) from
    // actionContainer()
    KToggleAction *act = ::qobject_cast<KToggleAction *>(sender());
    if (!act) {
        return;
    }
    int id;

    if (act == mAllFieldsAction) {
        id = 0;
    } else if (act == mIdentityAction) {
        id = HDR_IDENTITY;
    } else if (act == mTransportAction) {
        id = HDR_TRANSPORT;
    } else if (act == mFromAction) {
        id = HDR_FROM;
    } else if (act == mSubjectAction) {
        id = HDR_SUBJECT;
    } else if (act == mFccAction) {
        id = HDR_FCC;
    } else if (act == mDictionaryAction) {
        id = HDR_DICTIONARY;
    } else {
        qCDebug(KMAIL_LOG) << "Something is wrong (Oh, yeah?)";
        return;
    }

    bool forceAllHeaders = false;
    // sanders There's a bug here this logic doesn't work if no
    // fields are shown and then show all fields is selected.
    // Instead of all fields being shown none are.
    if (!act->isChecked()) {
        // hide header
        if (id > 0) {
            mShowHeaders = mShowHeaders & ~id;
        } else {
            mShowHeaders = std::abs(mShowHeaders);
        }
    } else {
        // show header
        if (id > 0) {
            mShowHeaders |= id;
        } else {
            mShowHeaders = -std::abs(mShowHeaders);
            if (mShowHeaders == 0) {
                forceAllHeaders = true;
            }
        }
    }
    rethinkFields(true, forceAllHeaders);
}

int KMComposerWin::calcColumnWidth(int which, long allShowing, int width) const
{
    if ((allShowing & which) == 0) {
        return width;
    }

    QLabel *w = nullptr;
    if (which == HDR_IDENTITY) {
        w = mLblIdentity;
    } else if (which == HDR_DICTIONARY) {
        w = mDictionaryLabel;
    } else if (which == HDR_FCC) {
        w = mLblFcc;
    } else if (which == HDR_TRANSPORT) {
        w = mLblTransport;
    } else if (which == HDR_FROM) {
        w = mLblFrom;
    } else if (which == HDR_SUBJECT) {
        w = mLblSubject;
    } else {
        return width;
    }

    w->setBuddy(mComposerBase->editor());   // set dummy so we don't calculate width of '&' for this label.
    w->adjustSize();
    w->show();
    return qMax(width, w->sizeHint().width());
}

void KMComposerWin::rethinkFields(bool fromSlot, bool forceAllHeaders)
{
    //This sucks even more but again no ids. sorry (sven)
    int mask, row;
    long showHeaders;

    if ((mShowHeaders < 0) || forceAllHeaders) {
        showHeaders = HDR_ALL;
    } else {
        showHeaders = mShowHeaders;
    }

    for (mask = 1, mNumHeaders = 0; mask <= showHeaders; mask <<= 1) {
        if ((showHeaders & mask) != 0) {
            ++mNumHeaders;
        }
    }
    delete mGrid;
    mGrid = new QGridLayout(mHeadersArea);
    mGrid->setColumnStretch(0, 1);
    mGrid->setColumnStretch(1, 100);
    mGrid->setRowStretch(mNumHeaders + 1, 100);

    row = 0;

    mLabelWidth = mComposerBase->recipientsEditor()->setFirstColumnWidth(0) + 2;
    if (std::abs(showHeaders)&HDR_IDENTITY) {
        mLabelWidth = calcColumnWidth(HDR_IDENTITY, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders)&HDR_DICTIONARY) {
        mLabelWidth = calcColumnWidth(HDR_DICTIONARY, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders)&HDR_FCC) {
        mLabelWidth = calcColumnWidth(HDR_FCC, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders)&HDR_TRANSPORT) {
        mLabelWidth = calcColumnWidth(HDR_TRANSPORT, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders)&HDR_FROM) {
        mLabelWidth = calcColumnWidth(HDR_FROM, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders)&HDR_SUBJECT) {
        mLabelWidth = calcColumnWidth(HDR_SUBJECT, showHeaders, mLabelWidth);
    }

    if (!fromSlot) {
        mAllFieldsAction->setChecked(showHeaders == HDR_ALL);
    }

    if (!fromSlot) {
        mIdentityAction->setChecked(std::abs(mShowHeaders)&HDR_IDENTITY);
    }
    rethinkHeaderLine(showHeaders, HDR_IDENTITY, row, mLblIdentity, mComposerBase->identityCombo());

    if (!fromSlot) {
        mDictionaryAction->setChecked(std::abs(mShowHeaders)&HDR_DICTIONARY);
    }
    rethinkHeaderLine(showHeaders, HDR_DICTIONARY, row, mDictionaryLabel,
                      mComposerBase->dictionary());

    if (!fromSlot) {
        mFccAction->setChecked(std::abs(mShowHeaders)&HDR_FCC);
    }
    rethinkHeaderLine(showHeaders, HDR_FCC, row, mLblFcc, mFccFolder);

    if (!fromSlot) {
        mTransportAction->setChecked(std::abs(mShowHeaders)&HDR_TRANSPORT);
    }
    rethinkHeaderLine(showHeaders, HDR_TRANSPORT, row, mLblTransport, mComposerBase->transportComboBox());

    if (!fromSlot) {
        mFromAction->setChecked(std::abs(mShowHeaders)&HDR_FROM);
    }
    rethinkHeaderLine(showHeaders, HDR_FROM, row, mLblFrom, mEdtFrom);

    QWidget *prevFocus = mEdtFrom;

    mGrid->addWidget(mComposerBase->recipientsEditor(), row, 0, 1, 2);
    ++row;
    connect(mEdtFrom, &MessageComposer::ComposerLineEdit::focusDown, mComposerBase->recipientsEditor(), &KPIM::MultiplyingLineEditor::setFocusTop);
    connect(mComposerBase->recipientsEditor(), &KPIM::MultiplyingLineEditor::focusUp, mEdtFrom, qOverload<>(&QWidget::setFocus));

    connect(mComposerBase->recipientsEditor(), &KPIM::MultiplyingLineEditor::focusDown, mEdtSubject, qOverload<>(&QWidget::setFocus));
    connect(mEdtSubject, &PimCommon::SpellCheckLineEdit::focusUp, mComposerBase->recipientsEditor(), &KPIM::MultiplyingLineEditor::setFocusBottom);

    prevFocus = mComposerBase->recipientsEditor();

    if (!fromSlot) {
        mSubjectAction->setChecked(std::abs(mShowHeaders)&HDR_SUBJECT);
    }
    rethinkHeaderLine(showHeaders, HDR_SUBJECT, row, mLblSubject, mEdtSubject);
    connectFocusMoving(mEdtSubject, mComposerBase->editor());

    assert(row <= mNumHeaders + 1);

    mHeadersArea->setMaximumHeight(mHeadersArea->sizeHint().height());

    mIdentityAction->setEnabled(!mAllFieldsAction->isChecked());
    mDictionaryAction->setEnabled(!mAllFieldsAction->isChecked());
    mTransportAction->setEnabled(!mAllFieldsAction->isChecked());
    mFromAction->setEnabled(!mAllFieldsAction->isChecked());
    mFccAction->setEnabled(!mAllFieldsAction->isChecked());
    mSubjectAction->setEnabled(!mAllFieldsAction->isChecked());
    mComposerBase->recipientsEditor()->setFirstColumnWidth(mLabelWidth);
}

QWidget *KMComposerWin::connectFocusMoving(QWidget *prev, QWidget *next)
{
    connect(prev, SIGNAL(focusDown()), next, SLOT(setFocus()));
    connect(next, SIGNAL(focusUp()), prev, SLOT(setFocus()));

    return next;
}

void KMComposerWin::rethinkHeaderLine(int aValue, int aMask, int &aRow, QLabel *aLbl, QWidget *aCbx)
{
    if (aValue & aMask) {
        aLbl->setBuddy(aCbx);
        aLbl->setFixedWidth(mLabelWidth);
        mGrid->addWidget(aLbl, aRow, 0);

        mGrid->addWidget(aCbx, aRow, 1);
        aCbx->show();
        aLbl->show();
        aRow++;
    } else {
        aLbl->hide();
        aCbx->hide();
    }
}

void KMComposerWin::slotUpdateComposer(const KIdentityManagement::Identity &ident, const KMime::Message::Ptr &msg, uint uoid, uint uoldId, bool wasModified)
{
    mComposerBase->updateTemplate(msg);
    updateSignature(uoid, uoldId);
    updateComposerAfterIdentityChanged(ident, uoid, wasModified);
}

void KMComposerWin::applyTemplate(uint uoid, uint uOldId, const KIdentityManagement::Identity &ident, bool wasModified)
{
    TemplateParser::TemplateParserJob::Mode mode;
    switch (mContext) {
    case New:
        mode = TemplateParser::TemplateParserJob::NewMessage;
        break;
    case Reply:
        mode = TemplateParser::TemplateParserJob::Reply;
        break;
    case ReplyToAll:
        mode = TemplateParser::TemplateParserJob::ReplyAll;
        break;
    case Forward:
        mode = TemplateParser::TemplateParserJob::Forward;
        break;
    case NoTemplate:
        updateComposerAfterIdentityChanged(ident, uoid, wasModified);
        return;
    }

    KMime::Headers::Generic *header = new KMime::Headers::Generic("X-KMail-Templates");
    header->fromUnicodeString(ident.templates(), "utf-8");
    mMsg->setHeader(header);

    if (mode == TemplateParser::TemplateParserJob::NewMessage) {
        KMComposerUpdateTemplateJob *job = new KMComposerUpdateTemplateJob;
        connect(job, &KMComposerUpdateTemplateJob::updateComposer, this, &KMComposerWin::slotUpdateComposer);
        job->setMsg(mMsg);
        job->setCustomTemplate(mCustomTemplate);
        job->setTextSelection(mTextSelection);
        job->setWasModified(wasModified);
        job->setUoldId(uOldId);
        job->setUoid(uoid);
        job->setIdent(ident);
        job->setCollection(mCollectionForNewMessage);
        job->start();
    } else {
        if (auto hrd = mMsg->headerByType("X-KMail-Link-Message")) {
            Akonadi::Item::List items;
            const QStringList serNums = hrd->asUnicodeString().split(QLatin1Char(','));
            items.reserve(serNums.count());
            for (const QString &serNumStr : serNums) {
                items << Akonadi::Item(serNumStr.toLongLong());
            }

            Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(items, this);
            job->fetchScope().fetchFullPayload(true);
            job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
            job->setProperty("mode", static_cast<int>(mode));
            job->setProperty("uoid", uoid);
            job->setProperty("uOldid", uOldId);
            connect(job, &Akonadi::ItemFetchJob::result, this, &KMComposerWin::slotDelayedApplyTemplate);
        }
        updateComposerAfterIdentityChanged(ident, uoid, wasModified);
    }
}

void KMComposerWin::slotDelayedApplyTemplate(KJob *job)
{
    const Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    const Akonadi::Item::List items = fetchJob->items();

    //Readd ? const TemplateParser::TemplateParserJob::Mode mode = static_cast<TemplateParser::TemplateParserJob::Mode>(fetchJob->property("mode").toInt());
    const uint uoid = fetchJob->property("uoid").toUInt();
    const uint uOldId = fetchJob->property("uOldid").toUInt();
#if 0 //FIXME template
    TemplateParser::TemplateParser parser(mMsg, mode);
    parser.setSelection(mTextSelection);
    parser.setAllowDecryption(true);
    parser.setWordWrap(MessageComposer::MessageComposerSettings::self()->wordWrap(), MessageComposer::MessageComposerSettings::self()->lineWrapWidth());
    parser.setIdentityManager(KMKernel::self()->identityManager());
    for (const Akonadi::Item &item : items) {
        if (!mCustomTemplate.isEmpty()) {
            parser.process(mCustomTemplate, MessageCore::Util::message(item));
        } else {
            parser.processWithIdentity(uoid, MessageCore::Util::message(item));
        }
    }
#else
    mComposerBase->updateTemplate(mMsg);
    updateSignature(uoid, uOldId);
    qCWarning(KMAIL_LOG) << " void KMComposerWin::slotDelayedApplyTemplate(KJob *job) is not implemented after removing qtwebkit";
#endif
}

void KMComposerWin::updateSignature(uint uoid, uint uOldId)
{
    const KIdentityManagement::Identity &ident = kmkernel->identityManager()->identityForUoid(uoid);
    const KIdentityManagement::Identity &oldIdentity = kmkernel->identityManager()->identityForUoid(uOldId);
    mComposerBase->identityChanged(ident, oldIdentity, true);
}

void KMComposerWin::setCollectionForNewMessage(const Akonadi::Collection &folder)
{
    mCollectionForNewMessage = folder;
}

void KMComposerWin::setQuotePrefix(uint uoid)
{
    QString quotePrefix;
    if (auto hrd = mMsg->headerByType("X-KMail-QuotePrefix")) {
        quotePrefix = hrd->asUnicodeString();
    }
    if (quotePrefix.isEmpty()) {
        // no quote prefix header, set quote prefix according in identity
        // TODO port templates to ComposerViewBase

        if (mCustomTemplate.isEmpty()) {
            const KIdentityManagement::Identity &identity = kmkernel->identityManager()->identityForUoidOrDefault(uoid);
            // Get quote prefix from template
            // ( custom templates don't specify custom quotes prefixes )
            TemplateParser::Templates quoteTemplate(
                TemplateParser::TemplatesConfiguration::configIdString(identity.uoid()));
            quotePrefix = quoteTemplate.quoteString();
        }
    }
    mComposerBase->editor()->setQuotePrefixName(MessageCore::StringUtil::formatQuotePrefix(quotePrefix,
                                                                                           mMsg->from()->displayString()));
}

void KMComposerWin::setupActions()
{
    KActionMenuTransport *actActionNowMenu = nullptr;
    KActionMenuTransport *actActionLaterMenu = nullptr;

    if (MessageComposer::MessageComposerSettings::self()->sendImmediate()) {
        //default = send now, alternative = queue
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("mail-send")), i18n("&Send Mail"), this);
        actionCollection()->addAction(QStringLiteral("send_default"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Return));
        connect(action, &QAction::triggered, this, &KMComposerWin::slotSendNowByShortcut);

        actActionNowMenu = new KActionMenuTransport(this);
        actActionNowMenu->setIcon(QIcon::fromTheme(QStringLiteral("mail-send")));
        actActionNowMenu->setText(i18n("&Send Mail Via"));

        actActionNowMenu->setIconText(i18n("Send"));
        actionCollection()->addAction(QStringLiteral("send_default_via"), actActionNowMenu);

        action = new QAction(QIcon::fromTheme(QStringLiteral("mail-queue")), i18n("Send &Later"), this);
        actionCollection()->addAction(QStringLiteral("send_alternative"), action);
        connect(action, &QAction::triggered, this, &KMComposerWin::slotSendLater);

        actActionLaterMenu = new KActionMenuTransport(this);
        actActionLaterMenu->setIcon(QIcon::fromTheme(QStringLiteral("mail-queue")));
        actActionLaterMenu->setText(i18n("Send &Later Via"));

        actActionLaterMenu->setIconText(i18nc("Queue the message for sending at a later date", "Queue"));
        actionCollection()->addAction(QStringLiteral("send_alternative_via"), actActionLaterMenu);
    } else {
        //default = queue, alternative = send now
        QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("mail-queue")), i18n("Send &Later"), this);
        actionCollection()->addAction(QStringLiteral("send_default"), action);
        connect(action, &QAction::triggered, this, &KMComposerWin::slotSendLater);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Return));

        actActionLaterMenu = new KActionMenuTransport(this);
        actActionLaterMenu->setIcon(QIcon::fromTheme(QStringLiteral("mail-queue")));
        actActionLaterMenu->setText(i18n("Send &Later Via"));
        actionCollection()->addAction(QStringLiteral("send_default_via"), actActionLaterMenu);

        action = new QAction(QIcon::fromTheme(QStringLiteral("mail-send")), i18n("&Send Mail"), this);
        actionCollection()->addAction(QStringLiteral("send_alternative"), action);
        connect(action, &QAction::triggered, this, &KMComposerWin::slotSendNow);

        actActionNowMenu = new KActionMenuTransport(this);
        actActionNowMenu->setIcon(QIcon::fromTheme(QStringLiteral("mail-send")));
        actActionNowMenu->setText(i18n("&Send Mail Via"));
        actionCollection()->addAction(QStringLiteral("send_alternative_via"), actActionNowMenu);
    }

    connect(actActionNowMenu, &QAction::triggered, this,
            &KMComposerWin::slotSendNow);
    connect(actActionLaterMenu, &QAction::triggered, this,
            &KMComposerWin::slotSendLater);
    connect(actActionNowMenu, &KActionMenuTransport::transportSelected, this,
            &KMComposerWin::slotSendNowVia);
    connect(actActionLaterMenu, &KActionMenuTransport::transportSelected, this,
            &KMComposerWin::slotSendLaterVia);

    QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save as &Draft"), this);
    actionCollection()->addAction(QStringLiteral("save_in_drafts"), action);
    KMail::Util::addQActionHelpText(action, i18n("Save email in Draft folder"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_S));
    connect(action, &QAction::triggered, this, &KMComposerWin::slotSaveDraft);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save as &Template"), this);
    KMail::Util::addQActionHelpText(action, i18n("Save email in Template folder"));
    actionCollection()->addAction(QStringLiteral("save_in_templates"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotSaveTemplate);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save as &File"), this);
    KMail::Util::addQActionHelpText(action, i18n("Save email as text or html file"));
    actionCollection()->addAction(QStringLiteral("save_as_file"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotSaveAsFile);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-open")), i18n("&Insert Text File..."), this);
    actionCollection()->addAction(QStringLiteral("insert_file"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotInsertFile);

    mRecentAction = new KRecentFilesAction(QIcon::fromTheme(QStringLiteral("document-open")),
                                           i18n("&Insert Recent Text File"), this);
    actionCollection()->addAction(QStringLiteral("insert_file_recent"), mRecentAction);
    connect(mRecentAction, &KRecentFilesAction::urlSelected, this, &KMComposerWin::slotInsertRecentFile);
    connect(mRecentAction, &KRecentFilesAction::recentListCleared, this, &KMComposerWin::slotRecentListFileClear);

    const QStringList urls = KMailSettings::self()->recentUrls();
    for (const QString &url : urls) {
        mRecentAction->addUrl(QUrl(url));
    }

    action = new QAction(QIcon::fromTheme(QStringLiteral("x-office-address-book")), i18n("&Address Book"), this);
    KMail::Util::addQActionHelpText(action, i18n("Open Address Book"));
    actionCollection()->addAction(QStringLiteral("addressbook"), action);
    if (QStandardPaths::findExecutable(QStringLiteral("kaddressbook")).isEmpty()) {
        action->setEnabled(false);
    } else {
        connect(action, &QAction::triggered, this, &KMComposerWin::slotAddressBook);
    }
    action = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new")), i18n("&New Composer"), this);
    actionCollection()->addAction(QStringLiteral("new_composer"), action);

    connect(action, &QAction::triggered, this, &KMComposerWin::slotNewComposer);
    actionCollection()->setDefaultShortcuts(action, KStandardShortcut::shortcut(KStandardShortcut::New));

    action = new QAction(i18n("Select &Recipients..."), this);
    actionCollection()->addAction(QStringLiteral("select_recipients"), action);
    connect(action, &QAction::triggered,
            mComposerBase->recipientsEditor(), &MessageComposer::RecipientsEditor::selectRecipients);
    action = new QAction(i18n("Save &Distribution List..."), this);
    actionCollection()->addAction(QStringLiteral("save_distribution_list"), action);
    connect(action, &QAction::triggered,
            mComposerBase->recipientsEditor(), &MessageComposer::RecipientsEditor::saveDistributionList);

    KStandardAction::print(this, &KMComposerWin::slotPrint, actionCollection());
    KStandardAction::printPreview(this, &KMComposerWin::slotPrintPreview, actionCollection());
    KStandardAction::close(this, &KMComposerWin::slotClose, actionCollection());

    KStandardAction::undo(mGlobalAction, &KMComposerGlobalAction::slotUndo, actionCollection());
    KStandardAction::redo(mGlobalAction, &KMComposerGlobalAction::slotRedo, actionCollection());
    KStandardAction::cut(mGlobalAction, &KMComposerGlobalAction::slotCut, actionCollection());
    KStandardAction::copy(mGlobalAction, &KMComposerGlobalAction::slotCopy, actionCollection());
    KStandardAction::paste(mGlobalAction, &KMComposerGlobalAction::slotPaste, actionCollection());
    mSelectAll = KStandardAction::selectAll(mGlobalAction, &KMComposerGlobalAction::slotMarkAll, actionCollection());

    mFindText = KStandardAction::find(mRichTextEditorwidget, &KPIMTextEdit::RichTextEditorWidget::slotFind, actionCollection());
    mFindNextText = KStandardAction::findNext(mRichTextEditorwidget, &KPIMTextEdit::RichTextEditorWidget::slotFindNext, actionCollection());

    mReplaceText = KStandardAction::replace(mRichTextEditorwidget, &KPIMTextEdit::RichTextEditorWidget::slotReplace, actionCollection());
    actionCollection()->addAction(KStandardAction::Spelling, QStringLiteral("spellcheck"),
                                  mComposerBase->editor(), SLOT(slotCheckSpelling()));

    action = new QAction(i18n("Paste as Attac&hment"), this);
    actionCollection()->addAction(QStringLiteral("paste_att"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotPasteAsAttachment);

    action = new QAction(i18n("Cl&ean Spaces"), this);
    actionCollection()->addAction(QStringLiteral("clean_spaces"), action);
    connect(action, &QAction::triggered, mComposerBase->signatureController(), &MessageComposer::SignatureController::cleanSpace);

    mFixedFontAction = new KToggleAction(i18n("Use Fi&xed Font"), this);
    actionCollection()->addAction(QStringLiteral("toggle_fixedfont"), mFixedFontAction);
    connect(mFixedFontAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateFont);
    mFixedFontAction->setChecked(MessageViewer::MessageViewerSettings::self()->useFixedFont());

    //these are checkable!!!
    mUrgentAction = new KToggleAction(
        i18nc("@action:inmenu Mark the email as urgent.", "&Urgent"), this);
    actionCollection()->addAction(QStringLiteral("urgent"), mUrgentAction);
    mRequestMDNAction = new KToggleAction(i18n("&Request Disposition Notification"), this);
    actionCollection()->addAction(QStringLiteral("options_request_mdn"), mRequestMDNAction);
    mRequestMDNAction->setChecked(KMailSettings::self()->requestMDN());

    mRequestDeliveryConfirmation = new KToggleAction(i18n("&Request Delivery Confirmation"), this);
    actionCollection()->addAction(QStringLiteral("options_request_delivery_confirmation"), mRequestDeliveryConfirmation);
    //TOOD mRequestDeliveryConfirmation->setChecked(KMailSettings::self()->requestMDN());

    //----- Message-Encoding Submenu
    mCodecAction = new CodecAction(CodecAction::ComposerMode, this);
    actionCollection()->addAction(QStringLiteral("charsets"), mCodecAction);
    mWordWrapAction = new KToggleAction(i18n("&Wordwrap"), this);
    actionCollection()->addAction(QStringLiteral("wordwrap"), mWordWrapAction);
    mWordWrapAction->setChecked(MessageComposer::MessageComposerSettings::self()->wordWrap());
    connect(mWordWrapAction, &KToggleAction::toggled, this, &KMComposerWin::slotWordWrapToggled);

    mSnippetAction = new KToggleAction(i18n("&Snippets"), this);
    actionCollection()->addAction(QStringLiteral("snippets"), mSnippetAction);
    connect(mSnippetAction, &KToggleAction::toggled, this, &KMComposerWin::slotSnippetWidgetVisibilityChanged);
    mSnippetAction->setChecked(KMailSettings::self()->showSnippetManager());

    mAutoSpellCheckingAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("tools-check-spelling")),
                                                 i18n("&Automatic Spellchecking"),
                                                 this);
    actionCollection()->addAction(QStringLiteral("options_auto_spellchecking"), mAutoSpellCheckingAction);
    const bool spellChecking = KMailSettings::self()->autoSpellChecking();
    const bool useKmailEditor = !KMailSettings::self()->useExternalEditor();
    const bool spellCheckingEnabled = useKmailEditor && spellChecking;
    mAutoSpellCheckingAction->setEnabled(useKmailEditor);

    mAutoSpellCheckingAction->setChecked(spellCheckingEnabled);
    slotAutoSpellCheckingToggled(spellCheckingEnabled);
    connect(mAutoSpellCheckingAction, &KToggleAction::toggled, this, &KMComposerWin::slotAutoSpellCheckingToggled);
    connect(mComposerBase->editor(), &KPIMTextEdit::RichTextEditor::checkSpellingChanged, this, &KMComposerWin::slotAutoSpellCheckingToggled);

    connect(mComposerBase->editor(), &MessageComposer::RichTextComposerNg::textModeChanged, this, &KMComposerWin::slotTextModeChanged);
    connect(mComposerBase->editor(), &MessageComposer::RichTextComposerNg::externalEditorClosed, this, &KMComposerWin::slotExternalEditorClosed);
    connect(mComposerBase->editor(), &MessageComposer::RichTextComposerNg::externalEditorStarted, this, &KMComposerWin::slotExternalEditorStarted);
    //these are checkable!!!
    mMarkupAction = new KToggleAction(i18n("Rich Text Editing"), this);
    mMarkupAction->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-font")));
    mMarkupAction->setIconText(i18n("Rich Text"));
    mMarkupAction->setToolTip(i18n("Toggle rich text editing mode"));
    actionCollection()->addAction(QStringLiteral("html"), mMarkupAction);
    connect(mMarkupAction, &KToggleAction::triggered, this, &KMComposerWin::slotToggleMarkup);

    mAllFieldsAction = new KToggleAction(i18n("&All Fields"), this);
    actionCollection()->addAction(QStringLiteral("show_all_fields"), mAllFieldsAction);
    connect(mAllFieldsAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mIdentityAction = new KToggleAction(i18n("&Identity"), this);
    actionCollection()->addAction(QStringLiteral("show_identity"), mIdentityAction);
    connect(mIdentityAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mDictionaryAction = new KToggleAction(i18n("&Dictionary"), this);
    actionCollection()->addAction(QStringLiteral("show_dictionary"), mDictionaryAction);
    connect(mDictionaryAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mFccAction = new KToggleAction(i18n("&Sent-Mail Folder"), this);
    actionCollection()->addAction(QStringLiteral("show_fcc"), mFccAction);
    connect(mFccAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mTransportAction = new KToggleAction(i18n("&Mail Transport"), this);
    actionCollection()->addAction(QStringLiteral("show_transport"), mTransportAction);
    connect(mTransportAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mFromAction = new KToggleAction(i18n("&From"), this);
    actionCollection()->addAction(QStringLiteral("show_from"), mFromAction);
    connect(mFromAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mSubjectAction = new KToggleAction(
        i18nc("@action:inmenu Show the subject in the composer window.", "S&ubject"), this);
    actionCollection()->addAction(QStringLiteral("show_subject"), mSubjectAction);
    connect(mSubjectAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    //end of checkable

    mAppendSignature = new QAction(i18n("Append S&ignature"), this);
    actionCollection()->addAction(QStringLiteral("append_signature"), mAppendSignature);
    connect(mAppendSignature, &QAction::triggered, mComposerBase->signatureController(), &MessageComposer::SignatureController::appendSignature);

    mPrependSignature = new QAction(i18n("Pr&epend Signature"), this);
    actionCollection()->addAction(QStringLiteral("prepend_signature"), mPrependSignature);
    connect(mPrependSignature, &QAction::triggered, mComposerBase->signatureController(), &MessageComposer::SignatureController::prependSignature);

    mInsertSignatureAtCursorPosition = new QAction(i18n("Insert Signature At C&ursor Position"), this);
    actionCollection()->addAction(QStringLiteral("insert_signature_at_cursor_position"), mInsertSignatureAtCursorPosition);
    connect(mInsertSignatureAtCursorPosition, &QAction::triggered, mComposerBase->signatureController(), &MessageComposer::SignatureController::insertSignatureAtCursor);

    mComposerBase->attachmentController()->createActions();

    setStandardToolBarMenuEnabled(true);

    KStandardAction::keyBindings(this, &KMComposerWin::slotEditKeys, actionCollection());
    KStandardAction::configureToolbars(this, &KMComposerWin::slotEditToolbars, actionCollection());
    KStandardAction::preferences(kmkernel, &KMKernel::slotShowConfigurationDialog, actionCollection());

    action = new QAction(i18n("&Spellchecker..."), this);
    action->setIconText(i18n("Spellchecker"));
    actionCollection()->addAction(QStringLiteral("setup_spellchecker"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotSpellcheckConfig);

    mEncryptAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("document-encrypt")), i18n("&Encrypt Message"), this);
    mEncryptAction->setIconText(i18n("Encrypt"));
    actionCollection()->addAction(QStringLiteral("encrypt_message"), mEncryptAction);
    mSignAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("document-sign")), i18n("&Sign Message"), this);
    mSignAction->setIconText(i18n("Sign"));
    actionCollection()->addAction(QStringLiteral("sign_message"), mSignAction);
    const KIdentityManagement::Identity &ident
        = KMKernel::self()->identityManager()->identityForUoidOrDefault(mComposerBase->identityCombo()->currentIdentity());
    // PENDING(marc): check the uses of this member and split it into
    // smime/openpgp and or enc/sign, if necessary:
    mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
    mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

    mLastEncryptActionState = false;
    mLastSignActionState = ident.pgpAutoSign();

    changeCryptoAction();

    connect(mEncryptAction, &KToggleAction::triggered, this, &KMComposerWin::slotEncryptToggled);
    connect(mSignAction, &KToggleAction::triggered, this, &KMComposerWin::slotSignToggled);

    QStringList listCryptoFormat;
    listCryptoFormat.reserve(numCryptoMessageFormats);
    for (int i = 0; i < numCryptoMessageFormats; ++i) {
        listCryptoFormat.push_back(Kleo::cryptoMessageFormatToLabel(cryptoMessageFormats[i]));
    }

    mCryptoModuleAction = new KSelectAction(i18n("&Cryptographic Message Format"), this);
    actionCollection()->addAction(QStringLiteral("options_select_crypto"), mCryptoModuleAction);
    connect(mCryptoModuleAction, qOverload<int>(&KSelectAction::triggered), this, &KMComposerWin::slotCryptoModuleSelected);
    mCryptoModuleAction->setToolTip(i18n("Select a cryptographic format for this message"));
    mCryptoModuleAction->setItems(listCryptoFormat);

    mComposerBase->editor()->createActions(actionCollection());

    mFollowUpToggleAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("appointment-new")), i18n("Create Follow Up Reminder..."), this);
    actionCollection()->addAction(QStringLiteral("follow_up_mail"), mFollowUpToggleAction);
    connect(mFollowUpToggleAction, &KToggleAction::triggered, this, &KMComposerWin::slotFollowUpMail);
    mFollowUpToggleAction->setEnabled(FollowUpReminder::FollowUpReminderUtil::followupReminderAgentEnabled());

    mPluginEditorManagerInterface->initializePlugins();
    mPluginEditorCheckBeforeSendManagerInterface->initializePlugins();
    mPluginEditorInitManagerInterface->initializePlugins();
    mPluginEditorConvertTextManagerInterface->initializePlugins();
    mPluginEditorGrammarManagerInterface->initializePlugins();

    mHideMenuBarAction = KStandardAction::showMenubar(this, &KMComposerWin::slotToggleMenubar, actionCollection());
    mHideMenuBarAction->setChecked(KMailSettings::self()->composerShowMenuBar());
    slotToggleMenubar(true);

    createGUI(QStringLiteral("kmcomposerui.rc"));
    initializePluginActions();
    connect(toolBar(QStringLiteral("htmlToolBar"))->toggleViewAction(), &QAction::toggled,
            this, &KMComposerWin::htmlToolBarVisibilityChanged);

    // In Kontact, this entry would read "Configure Kontact", but bring
    // up KMail's config dialog. That's sensible, though, so fix the label.
    QAction *configureAction = actionCollection()->action(QStringLiteral("options_configure"));
    if (configureAction) {
        configureAction->setText(i18n("Configure KMail..."));
    }
}

void KMComposerWin::slotToggleMenubar(bool dontShowWarning)
{
    if (menuBar()) {
        if (mHideMenuBarAction->isChecked()) {
            menuBar()->show();
        } else {
            if (!dontShowWarning) {
                const QString accel = mHideMenuBarAction->shortcut().toString();
                KMessageBox::information(this,
                                         i18n("<qt>This will hide the menu bar completely."
                                              " You can show it again by typing %1.</qt>", accel),
                                         i18n("Hide menu bar"), QStringLiteral("HideMenuBarWarning"));
            }
            menuBar()->hide();
        }
        KMailSettings::self()->setComposerShowMenuBar(mHideMenuBarAction->isChecked());
    }
}

void KMComposerWin::initializePluginActions()
{
    if (guiFactory()) {
        QHash<QString, QList<QAction *> > hashActions;
        QHashIterator<MessageComposer::PluginActionType::Type, QList<QAction *> > localEditorManagerActionsType(mPluginEditorManagerInterface->actionsType());
        while (localEditorManagerActionsType.hasNext()) {
            localEditorManagerActionsType.next();
            QList<QAction *> lst = localEditorManagerActionsType.value();
            if (!lst.isEmpty()) {
                const QString actionlistname = QLatin1String("kmaileditor") + MessageComposer::PluginActionType::actionXmlExtension(localEditorManagerActionsType.key());
                hashActions.insert(actionlistname, lst);
            }
        }
        QHashIterator<MessageComposer::PluginActionType::Type, QList<QAction *> > localEditorConvertTextManagerActionsType(mPluginEditorConvertTextManagerInterface->actionsType());
        while (localEditorConvertTextManagerActionsType.hasNext()) {
            localEditorConvertTextManagerActionsType.next();
            QList<QAction *> lst = localEditorConvertTextManagerActionsType.value();
            if (!lst.isEmpty()) {
                const QString actionlistname = QLatin1String("kmaileditor") + MessageComposer::PluginActionType::actionXmlExtension(localEditorConvertTextManagerActionsType.key());
                if (hashActions.contains(actionlistname)) {
                    lst = hashActions.value(actionlistname) + lst;
                    hashActions.remove(actionlistname);
                }
                hashActions.insert(actionlistname, lst);
            }
        }

        const QList<KToggleAction *> customToolsWidgetActionList = mCustomToolsWidget->actionList();
        const QString actionlistname = QLatin1String("kmaileditor") + MessageComposer::PluginActionType::actionXmlExtension(MessageComposer::PluginActionType::Tools);
        for (KToggleAction *act : customToolsWidgetActionList) {
            QList<QAction *> lst;
            lst << act;
            if (hashActions.contains(actionlistname)) {
                lst = hashActions.value(actionlistname) + lst;
                hashActions.remove(actionlistname);
            }
            hashActions.insert(actionlistname, lst);
        }

        QHash<QString, QList<QAction *> >::const_iterator i = hashActions.constBegin();

        while (i != hashActions.constEnd()) {
            const auto lst = guiFactory()->clients();
            for (KXMLGUIClient *client : lst) {
                client->unplugActionList(i.key());
                client->plugActionList(i.key(), i.value());
            }
            ++i;
        }
        //Initialize statusbar
        const QList<QWidget *> statusbarWidgetList = mPluginEditorManagerInterface->statusBarWidgetList();
        for (int i = 0; i < statusbarWidgetList.count(); ++i) {
            statusBar()->addPermanentWidget(statusbarWidgetList.at(i), 0);
        }
        const QList<QWidget *> statusbarWidgetListConverter = mPluginEditorConvertTextManagerInterface->statusBarWidgetList();
        for (int i = 0; i < statusbarWidgetListConverter.count(); ++i) {
            statusBar()->addPermanentWidget(statusbarWidgetListConverter.at(i), 0);
        }
    }
}

void KMComposerWin::changeCryptoAction()
{
    const KIdentityManagement::Identity &ident
        = KMKernel::self()->identityManager()->identityForUoidOrDefault(mComposerBase->identityCombo()->currentIdentity());

    if (!QGpgME::openpgp() && !QGpgME::smime()) {
        // no crypto whatsoever
        mEncryptAction->setEnabled(false);
        setEncryption(false);
        mSignAction->setEnabled(false);
        setSigning(false);
    } else {
        const bool canOpenPGPSign = QGpgME::openpgp()
                                    && !ident.pgpSigningKey().isEmpty();
        const bool canSMIMESign = QGpgME::smime()
                                  && !ident.smimeSigningKey().isEmpty();

        setEncryption(false);
        setSigning((canOpenPGPSign || canSMIMESign) && ident.pgpAutoSign());
    }
}

void KMComposerWin::setupStatusBar(QWidget *w)
{
    statusBar()->addWidget(w);
    mStatusbarLabel = new QLabel(this);
    mStatusbarLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusBar()->addPermanentWidget(mStatusbarLabel);

    mCursorLineLabel = new QLabel(this);
    mCursorLineLabel->setTextFormat(Qt::PlainText);
    mCursorLineLabel->setText(i18nc("Shows the linenumber of the cursor position.", " Line: %1 ",
                                    QStringLiteral("     ")));
    statusBar()->addPermanentWidget(mCursorLineLabel);

    mCursorColumnLabel = new QLabel(i18n(" Column: %1 ", QStringLiteral("     ")));
    mCursorColumnLabel->setTextFormat(Qt::PlainText);
    statusBar()->addPermanentWidget(mCursorColumnLabel);

    mStatusBarLabelToggledOverrideMode = new MessageComposer::StatusBarLabelToggledState(this);
    mStatusBarLabelToggledOverrideMode->setStateString(i18n("OVR"), i18n("INS"));
    statusBar()->addPermanentWidget(mStatusBarLabelToggledOverrideMode, 0);
    connect(mStatusBarLabelToggledOverrideMode, &MessageComposer::StatusBarLabelToggledState::toggleModeChanged, this, &KMComposerWin::slotOverwriteModeWasChanged);

    mStatusBarLabelSpellCheckingChangeMode = new MessageComposer::StatusBarLabelToggledState(this);
    mStatusBarLabelSpellCheckingChangeMode->setStateString(i18n("Spellcheck: on"), i18n("Spellcheck: off"));
    statusBar()->addPermanentWidget(mStatusBarLabelSpellCheckingChangeMode, 0);
    connect(mStatusBarLabelSpellCheckingChangeMode, &MessageComposer::StatusBarLabelToggledState::toggleModeChanged, this, &KMComposerWin::slotAutoSpellCheckingToggled);
}

void KMComposerWin::setupEditor()
{
    QFontMetrics fm(mBodyFont);
    mComposerBase->editor()->setTabStopDistance(fm.boundingRect(QLatin1Char(' ')).width() * 8);

    slotWordWrapToggled(MessageComposer::MessageComposerSettings::self()->wordWrap());

    // Font setup
    slotUpdateFont();

    connect(mComposerBase->editor(), &QTextEdit::cursorPositionChanged,
            this, &KMComposerWin::slotCursorPositionChanged);
    slotCursorPositionChanged();
}

QString KMComposerWin::subject() const
{
    return MessageComposer::Util::cleanedUpHeaderString(mEdtSubject->toPlainText());
}

QString KMComposerWin::from() const
{
    return MessageComposer::Util::cleanedUpHeaderString(mEdtFrom->text());
}

void KMComposerWin::slotInvalidIdentity()
{
    mIncorrectIdentityFolderWarning->identityInvalid();
}

void KMComposerWin::slotFccIsInvalid()
{
    mIncorrectIdentityFolderWarning->fccIsInvalid();
}

void KMComposerWin::setCurrentTransport(int transportId)
{
    if (!mComposerBase->transportComboBox()->setCurrentTransport(transportId)) {
        mIncorrectIdentityFolderWarning->mailTransportIsInvalid();
    }
}

uint KMComposerWin::currentIdentity() const
{
    return mComposerBase->identityCombo()->currentIdentity();
}

void KMComposerWin::setMessage(const KMime::Message::Ptr &newMsg, bool lastSignState, bool lastEncryptState, bool mayAutoSign, bool allowDecryption, bool isModified)
{
    if (!newMsg) {
        qCDebug(KMAIL_LOG) << "newMsg == 0!";
        return;
    }

    if (lastSignState) {
        mLastSignActionState = true;
    }

    if (lastEncryptState) {
        mLastEncryptActionState = true;
    }

    mComposerBase->setMessage(newMsg, allowDecryption);
    mMsg = newMsg;

    //Add initial data.
    MessageComposer::PluginEditorConverterInitialData data;
    data.setMewMsg(mMsg);
    data.setNewMessage(mContext == TemplateContext::New);
    mPluginEditorConvertTextManagerInterface->setInitialData(data);

    KIdentityManagement::IdentityManager *im = KMKernel::self()->identityManager();

    mEdtFrom->setText(mMsg->from()->asUnicodeString());
    mEdtSubject->setText(mMsg->subject()->asUnicodeString());

    // Restore the quote prefix. We can't just use the global quote prefix here,
    // since the prefix is different for each message, it might for example depend
    // on the original sender in a reply.
    if (auto hdr = mMsg->headerByType("X-KMail-QuotePrefix")) {
        mComposerBase->editor()->setQuotePrefixName(hdr->asUnicodeString());
    }

    if (auto hrd = newMsg->headerByType("X-KMail-Identity")) {
        const QString identityStr = hrd->asUnicodeString();
        if (!identityStr.isEmpty()) {
            const KIdentityManagement::Identity &ident = KMKernel::self()->identityManager()->identityForUoid(identityStr.toUInt());
            if (ident.isNull()) {
                if (auto hrd = newMsg->headerByType("X-KMail-Identity-Name")) {
                    const QString identityStrName = hrd->asUnicodeString();
                    const KIdentityManagement::Identity id = KMKernel::self()->identityManager()->modifyIdentityForName(identityStrName);
                    if (!id.isNull()) {
                        mId = id.uoid();
                    } else {
                        mId = 0;
                    }
                } else {
                    mId = 0;
                }
            } else {
                mId = identityStr.toUInt();
            }
        }
    } else {
        if (auto hrd = newMsg->headerByType("X-KMail-Identity-Name")) {
            const QString identityStrName = hrd->asUnicodeString();
            const KIdentityManagement::Identity id = KMKernel::self()->identityManager()->modifyIdentityForName(identityStrName);
            if (!id.isNull()) {
                mId = id.uoid();
            } else {
                mId = 0;
            }
        } else {
            mId = 0;
        }
    }

    // don't overwrite the header values with identity specific values
    disconnect(mIdentityConnection);

    // load the mId into the gui, sticky or not, without emitting
    mComposerBase->identityCombo()->setCurrentIdentity(mId);
    mIdentityConnection = connect(mComposerBase->identityCombo(), &KIdentityManagement::IdentityCombo::identityChanged, [this](uint val) {
        slotIdentityChanged(val);
    });

    // manually load the identity's value into the fields; either the one from the
    // message, where appropriate, or the one from the sticky identity. What's in
    // mId might have changed meanwhile, thus the save value
    slotIdentityChanged(mId, true /*initalChange*/);
    // Fixing the identities with auto signing activated
    mLastSignActionState = mSignAction->isChecked();

    const KIdentityManagement::Identity &ident = im->identityForUoid(mComposerBase->identityCombo()->currentIdentity());

    // check for the presence of a DNT header, indicating that MDN's were requested
    if (auto hdr = newMsg->headerByType("Disposition-Notification-To")) {
        const QString mdnAddr = hdr->asUnicodeString();
        mRequestMDNAction->setChecked((!mdnAddr.isEmpty()
                                       && im->thatIsMe(mdnAddr))
                                      || KMailSettings::self()->requestMDN());
    }
    if (auto hdr = newMsg->headerByType("Return-Receipt-To")) {
        const QString returnReceiptToAddr = hdr->asUnicodeString();
        mRequestDeliveryConfirmation->setChecked((!returnReceiptToAddr.isEmpty()
                                                  && im->thatIsMe(returnReceiptToAddr))
                                                 /*TODO || KMailSettings::self()->requestMDN()*/);
    }
    // check for presence of a priority header, indicating urgent mail:
    if (newMsg->headerByType("X-PRIORITY") && newMsg->headerByType("Priority")) {
        const QString xpriority = newMsg->headerByType("X-PRIORITY")->asUnicodeString();
        const QString priority = newMsg->headerByType("Priority")->asUnicodeString();
        if (xpriority == QLatin1String("2 (High)") && priority == QLatin1String("urgent")) {
            mUrgentAction->setChecked(true);
        }
    }

    if (!ident.isXFaceEnabled() || ident.xface().isEmpty()) {
        mMsg->removeHeader("X-Face");
    } else {
        QString xface = ident.xface();
        if (!xface.isEmpty()) {
            int numNL = (xface.length() - 1) / 70;
            for (int i = numNL; i > 0; --i) {
                xface.insert(i * 70, QStringLiteral("\n\t"));
            }
            auto header = new KMime::Headers::Generic("X-Face");
            header->fromUnicodeString(xface, "utf-8");
            mMsg->setHeader(header);
        }
    }

    // if these headers are present, the state of the message should be overruled
    if (auto hdr = mMsg->headerByType("X-KMail-SignatureActionEnabled")) {
        mLastSignActionState = (hdr->as7BitString(false).contains("true"));
    }
    if (auto hdr = mMsg->headerByType("X-KMail-EncryptActionEnabled")) {
        mLastEncryptActionState = (hdr->as7BitString(false).contains("true"));
    }
    if (auto hdr = mMsg->headerByType("X-KMail-CryptoMessageFormat")) {
        mCryptoModuleAction->setCurrentItem(format2cb(static_cast<Kleo::CryptoMessageFormat>(
                                                          hdr->asUnicodeString().toInt())));
    }

    mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
    mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

    if (QGpgME::openpgp() || QGpgME::smime()) {
        const bool canOpenPGPSign = QGpgME::openpgp() && !ident.pgpSigningKey().isEmpty();
        const bool canSMIMESign = QGpgME::smime() && !ident.smimeSigningKey().isEmpty();

        setEncryption(mLastEncryptActionState);
        setSigning((canOpenPGPSign || canSMIMESign) && mLastSignActionState);
    }
    updateSignatureAndEncryptionStateIndicators();

    QString kmailFcc;
    if (auto hdr = mMsg->headerByType("X-KMail-Fcc")) {
        kmailFcc = hdr->asUnicodeString();
    }
    if (kmailFcc.isEmpty()) {
        setFcc(ident.fcc());
    } else {
        setFcc(kmailFcc);
    }
    if (auto hdr = mMsg->headerByType("X-KMail-Dictionary")) {
        const QString dictionary = hdr->asUnicodeString();
        if (!dictionary.isEmpty()) {
            if (!mComposerBase->dictionary()->assignByDictionnary(dictionary)) {
                mIncorrectIdentityFolderWarning->dictionaryInvalid();
            }
        }
    } else {
        mComposerBase->dictionary()->setCurrentByDictionaryName(ident.dictionary());
    }

    KMime::Content *msgContent = new KMime::Content;
    msgContent->setContent(mMsg->encodedContent());
    msgContent->parse();
    MimeTreeParser::SimpleObjectTreeSource emptySource;
    MimeTreeParser::ObjectTreeParser otp(&emptySource);  //All default are ok
    emptySource.setDecryptMessage(allowDecryption);
    otp.parseObjectTree(msgContent);

    bool shouldSetCharset = false;
    if ((mContext == Reply || mContext == ReplyToAll || mContext == Forward) && MessageComposer::MessageComposerSettings::forceReplyCharset()) {
        shouldSetCharset = true;
    }
    if (shouldSetCharset && !otp.plainTextContentCharset().isEmpty()) {
        mOriginalPreferredCharset = otp.plainTextContentCharset();
    }
    // always set auto charset, but prefer original when composing if force reply is set.
    mCodecAction->setAutoCharset();

    delete msgContent;

    if ((MessageComposer::MessageComposerSettings::self()->autoTextSignature() == QLatin1String("auto")) && mayAutoSign) {
        //
        // Espen 2000-05-16
        // Delay the signature appending. It may start a fileseletor.
        // Not user friendly if this modal fileseletor opens before the
        // composer.
        //
        if (MessageComposer::MessageComposerSettings::self()->prependSignature()) {
            QTimer::singleShot(0, mComposerBase->signatureController(), &MessageComposer::SignatureController::prependSignature);
        } else {
            QTimer::singleShot(0, mComposerBase->signatureController(), &MessageComposer::SignatureController::appendSignature);
        }
    } else {
        mComposerBase->editor()->externalComposer()->startExternalEditor();
    }

    setModified(isModified);

    // honor "keep reply in this folder" setting even when the identity is changed later on
    mPreventFccOverwrite = (!kmailFcc.isEmpty() && ident.fcc() != kmailFcc);
    QTimer::singleShot(0, this, &KMComposerWin::forceAutoSaveMessage);   //Force autosaving to make sure this composer reappears if a crash happens before the autosave timer kicks in.
}

void KMComposerWin::setAutoSaveFileName(const QString &fileName)
{
    mComposerBase->setAutoSaveFileName(fileName);
}

void KMComposerWin::setSigningAndEncryptionDisabled(bool v)
{
    mSigningAndEncryptionExplicitlyDisabled = v;
}

void KMComposerWin::setFolder(const Akonadi::Collection &aFolder)
{
    mFolder = aFolder;
}

void KMComposerWin::setFcc(const QString &idString)
{
    // check if the sent-mail folder still exists
    Akonadi::Collection col;
    if (idString.isEmpty()) {
        col = CommonKernel->sentCollectionFolder();
    } else {
        col = Akonadi::Collection(idString.toLongLong());
    }
    if (col.isValid()) {
        mComposerBase->setFcc(col);
        mFccFolder->setCollection(col);
        mIncorrectIdentityFolderWarning->clearFccInvalid();
    } else {
        mIncorrectIdentityFolderWarning->fccIsInvalid();
        qCWarning(KMAIL_LOG) << "setFcc: collection invalid " << idString;
    }
}

bool KMComposerWin::isComposerModified() const
{
    return mComposerBase->editor()->document()->isModified()
           || mEdtFrom->isModified()
           || mComposerBase->recipientsEditor()->isModified()
           || mEdtSubject->document()->isModified();
}

bool KMComposerWin::isModified() const
{
    return mWasModified || isComposerModified();
}

void KMComposerWin::setModified(bool modified)
{
    mWasModified = modified;
    changeModifiedState(modified);
}

void KMComposerWin::changeModifiedState(bool modified)
{
    mComposerBase->editor()->document()->setModified(modified);
    if (!modified) {
        mEdtFrom->setModified(false);
        mComposerBase->recipientsEditor()->clearModified();
        mEdtSubject->document()->setModified(false);
    }
}

bool KMComposerWin::queryClose()
{
    if (!mComposerBase->editor()->checkExternalEditorFinished()) {
        return false;
    }
    if (kmkernel->shuttingDown() || qApp->isSavingSession()) {
        writeConfig();
        return true;
    }

    if (isModified()) {
        const bool istemplate = (mFolder.isValid() && CommonKernel->folderIsTemplates(mFolder));
        const QString savebut = (istemplate
                                 ? i18n("Re&save as Template")
                                 : i18n("&Save as Draft"));
        const QString savetext = (istemplate
                                  ? i18n("Resave this message in the Templates folder. "
                                         "It can then be used at a later time.")
                                  : i18n("Save this message in the Drafts folder. "
                                         "It can then be edited and sent at a later time."));

        const int rc = KMessageBox::warningYesNoCancel(this,
                                                       i18n("Do you want to save the message for later or discard it?"),
                                                       i18n("Close Composer"),
                                                       KGuiItem(savebut, QStringLiteral("document-save"), QString(), savetext),
                                                       KStandardGuiItem::discard(),
                                                       KStandardGuiItem::cancel());
        if (rc == KMessageBox::Cancel) {
            return false;
        } else if (rc == KMessageBox::Yes) {
            // doSend will close the window. Just return false from this method
            if (istemplate) {
                slotSaveTemplate();
            } else {
                slotSaveDraft();
            }
            return false;
        }
        //else fall through: return true
    }
    mComposerBase->cleanupAutoSave();

    if (!mMiscComposers.isEmpty()) {
        qCWarning(KMAIL_LOG) << "Tried to close while composer was active";
        return false;
    }
    writeConfig();
    return true;
}

MessageComposer::ComposerViewBase::MissingAttachment KMComposerWin::userForgotAttachment()
{
    bool checkForForgottenAttachments = mCheckForForgottenAttachments && KMailSettings::self()->showForgottenAttachmentWarning();

    if (!checkForForgottenAttachments) {
        return MessageComposer::ComposerViewBase::NoMissingAttachmentFound;
    }

    mComposerBase->setSubject(subject());   //be sure the composer knows the subject
    MessageComposer::ComposerViewBase::MissingAttachment missingAttachments = mComposerBase->checkForMissingAttachments(KMailSettings::self()->attachmentKeywords());

    return missingAttachments;
}

void KMComposerWin::forceAutoSaveMessage()
{
    autoSaveMessage(true);
}

void KMComposerWin::autoSaveMessage(bool force)
{
    if (isComposerModified() || force) {
        applyComposerSetting(mComposerBase);
        mComposerBase->saveMailSettings();
        mComposerBase->autoSaveMessage();
        if (!force) {
            mWasModified = true;
            changeModifiedState(false);
        }
    } else {
        mComposerBase->updateAutoSave();
    }
}

bool KMComposerWin::encryptToSelf() const
{
    return MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelf();
}

void KMComposerWin::slotSendFailed(const QString &msg, MessageComposer::ComposerViewBase::FailedType type)
{
    setEnabled(true);
    if (!msg.isEmpty()) {
        KMessageBox::sorry(mMainWidget, msg,
                           (type == MessageComposer::ComposerViewBase::AutoSave) ? i18n("Autosave Message Failed") : i18n("Sending Message Failed"));
    }
}

void KMComposerWin::slotSendSuccessful(Akonadi::Item::Id id)
{
    if (id != -1) {
        UndoSendManager::self()->addItem(id, subject(), KMailSettings::self()->undoSendDelay());
    }
    setModified(false);
    mComposerBase->cleanupAutoSave();
    mFolder = Akonadi::Collection(); // see dtor
    close();
}

const KIdentityManagement::Identity &KMComposerWin::identity() const
{
    return KMKernel::self()->identityManager()->identityForUoidOrDefault(mComposerBase->identityCombo()->currentIdentity());
}

Kleo::CryptoMessageFormat KMComposerWin::cryptoMessageFormat() const
{
    if (!mCryptoModuleAction) {
        return Kleo::AutoFormat;
    }
    return cb2format(mCryptoModuleAction->currentItem());
}

void KMComposerWin::addAttach(KMime::Content *msgPart)
{
    mComposerBase->addAttachmentPart(msgPart);
    setModified(true);
}

void KMComposerWin::slotAddressBook()
{
    KRun::runCommand(QStringLiteral("kaddressbook"), window());
}

void KMComposerWin::slotInsertFile()
{
    const QUrl u = insertFile();
    if (u.isEmpty()) {
        return;
    }

    mRecentAction->addUrl(u);
    // Prevent race condition updating list when multiple composers are open
    {
        QUrlQuery query(u);
        const QString encoding = MimeTreeParser::NodeHelper::encodingForName(query.queryItemValue(QStringLiteral("charset")));
        QStringList urls = KMailSettings::self()->recentUrls();
        QStringList encodings = KMailSettings::self()->recentEncodings();
        // Prevent config file from growing without bound
        // Would be nicer to get this constant from KRecentFilesAction
        const int mMaxRecentFiles = 30;
        while (urls.count() > mMaxRecentFiles) {
            urls.removeLast();
        }
        while (encodings.count() > mMaxRecentFiles) {
            encodings.removeLast();
        }
        // sanity check
        if (urls.count() != encodings.count()) {
            urls.clear();
            encodings.clear();
        }
        urls.prepend(u.toDisplayString());
        encodings.prepend(encoding);
        KMailSettings::self()->setRecentUrls(urls);
        KMailSettings::self()->setRecentEncodings(encodings);
        KMailSettings::self()->save();
    }
    slotInsertRecentFile(u);
}

void KMComposerWin::slotRecentListFileClear()
{
    KSharedConfig::Ptr config = KMKernel::self()->config();
    KConfigGroup group(config, "Composer");
    group.deleteEntry("recent-urls");
    group.deleteEntry("recent-encoding");
    KMailSettings::self()->save();
}

void KMComposerWin::slotInsertRecentFile(const QUrl &u)
{
    if (u.fileName().isEmpty()) {
        return;
    }

    // Get the encoding previously used when inserting this file
    QString encoding;
    const QStringList urls = KMailSettings::self()->recentUrls();
    const QStringList encodings = KMailSettings::self()->recentEncodings();
    const int index = urls.indexOf(u.toDisplayString());
    if (index != -1) {
        encoding = encodings[ index ];
    } else {
        qCDebug(KMAIL_LOG) << " encoding not found so we can't insert text"; //see InsertTextFileJob
        return;
    }
    MessageComposer::InsertTextFileJob *job = new MessageComposer::InsertTextFileJob(mComposerBase->editor(), u);
    job->setEncoding(encoding);
    connect(job, &KJob::result, this, &KMComposerWin::slotInsertTextFile);
    job->start();
}

bool KMComposerWin::showErrorMessage(KJob *job)
{
    if (job->error()) {
        if (auto uiDelegate = static_cast<KIO::Job *>(job)->uiDelegate()) {
            uiDelegate->showErrorMessage();
        } else {
            qCDebug(KMAIL_LOG) << " job->errorString() :" << job->errorString();
        }
        return true;
    }
    return false;
}

void KMComposerWin::slotInsertTextFile(KJob *job)
{
    showErrorMessage(job);
}

void KMComposerWin::slotCryptoModuleSelected()
{
    slotSelectCryptoModule(false);
}

void KMComposerWin::slotSelectCryptoModule(bool init)
{
    if (!init) {
        setModified(true);
    }

    mComposerBase->attachmentModel()->setEncryptEnabled(canSignEncryptAttachments());
    mComposerBase->attachmentModel()->setSignEnabled(canSignEncryptAttachments());
}

void KMComposerWin::slotUpdateFont()
{
    if (!mFixedFontAction) {
        return;
    }
    const QFont plaintextFont = mFixedFontAction->isChecked() ? mFixedFont : mBodyFont;
    mComposerBase->editor()->composerControler()->setFontForWholeText(plaintextFont);
    mComposerBase->editor()->setDefaultFontSize(plaintextFont.pointSize());
}

QUrl KMComposerWin::insertFile()
{
    const KEncodingFileDialog::Result result = KEncodingFileDialog::getOpenUrlAndEncoding(QString(),
                                                                                          QUrl(),
                                                                                          QString(),
                                                                                          this,
                                                                                          i18nc("@title:window", "Insert File"));
    QUrl url;
    if (!result.URLs.isEmpty()) {
        url = result.URLs.constFirst();
        if (url.isValid()) {
            MessageCore::StringUtil::setEncodingFile(url, MimeTreeParser::NodeHelper::fixEncoding(result.encoding));
        }
    }
    return url;
}

QString KMComposerWin::smartQuote(const QString &msg)
{
    return MessageCore::StringUtil::smartQuote(msg, MessageComposer::MessageComposerSettings::self()->lineWrapWidth());
}

void KMComposerWin::insertUrls(const QMimeData *source, const QList<QUrl> &urlList)
{
    QStringList urlAdded;
    for (const QUrl &url : urlList) {
        QString urlStr;
        if (url.scheme() == QLatin1String("mailto")) {
            urlStr = KEmailAddress::decodeMailtoUrl(url);
        } else {
            urlStr = url.toDisplayString();
            // Workaround #346370
            if (urlStr.isEmpty()) {
                urlStr = source->text();
            }
        }
        if (!urlAdded.contains(urlStr)) {
            mComposerBase->editor()->composerControler()->insertLink(urlStr);
            urlAdded.append(urlStr);
        }
    }
}

bool KMComposerWin::insertFromMimeData(const QMimeData *source, bool forceAttachment)
{
    // If this is a PNG image, either add it as an attachment or as an inline image
    if (source->hasHtml() && mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich) {
        const QString html = QString::fromUtf8(source->data(QStringLiteral("text/html")));
        mComposerBase->editor()->insertHtml(html);
        return true;
    } else if (source->hasHtml()
               && (mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Plain)
               && source->hasText()
               && !forceAttachment) {
        mComposerBase->editor()->insertPlainText(source->text());
        return true;
    } else if (source->hasImage() && source->hasFormat(QStringLiteral("image/png"))) {
        // Get the image data before showing the dialog, since that processes events which can delete
        // the QMimeData object behind our back
        const QByteArray imageData = source->data(QStringLiteral("image/png"));
        if (imageData.isEmpty()) {
            return true;
        }
        if (!forceAttachment) {
            if (mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich /*&& mComposerBase->editor()->isEnableImageActions() Necessary ?*/) {
                QImage image = qvariant_cast<QImage>(source->imageData());
                QFileInfo fi(source->text());

                QMenu menu(this);
                const QAction *addAsInlineImageAction = menu.addAction(i18n("Add as &Inline Image"));
                /*const QAction *addAsAttachmentAction = */ menu.addAction(i18n("Add as &Attachment"));
                const QAction *selectedAction = menu.exec(QCursor::pos());
                if (selectedAction == addAsInlineImageAction) {
                    // Let the textedit from kdepimlibs handle inline images
                    mComposerBase->editor()->composerControler()->composerImages()->insertImage(image, fi);
                    return true;
                } else if (!selectedAction) {
                    return true;
                }
                // else fall through
            }
        }
        // Ok, when we reached this point, the user wants to add the image as an attachment.
        // Ask for the filename first.
        bool ok;
        QString attName
            = QInputDialog::getText(this, i18n("KMail"), i18n("Name of the attachment:"), QLineEdit::Normal, QString(), &ok);
        if (!ok) {
            return true;
        }
        attName = attName.trimmed();
        if (attName.isEmpty()) {
            KMessageBox::sorry(this,
                               i18n("Attachment name can't be empty"),
                               i18n("Invalid Attachment Name"));

            return true;
        }
        addAttachment(attName, KMime::Headers::CEbase64, QString(), imageData, "image/png");
        return true;
    } else {
        DndFromArkJob *job = new DndFromArkJob(this);
        job->setComposerWin(this);
        if (job->extract(source)) {
            return true;
        }
    }

    // If this is a URL list, add those files as attachments or text
    // but do not offer this if we are pasting plain text containing an url, e.g. from a browser
    const QList<QUrl> urlList = source->urls();
    if (!urlList.isEmpty()) {
        //Search if it's message items.
        Akonadi::Item::List items;
        Akonadi::Collection::List collections;
        bool allLocalURLs = true;

        for (const QUrl &url : urlList) {
            if (!url.isLocalFile()) {
                allLocalURLs = false;
            }
            const Akonadi::Item item = Akonadi::Item::fromUrl(url);
            if (item.isValid()) {
                items << item;
            } else {
                const Akonadi::Collection collection = Akonadi::Collection::fromUrl(url);
                if (collection.isValid()) {
                    collections << collection;
                }
            }
        }

        if (items.isEmpty() && collections.isEmpty()) {
            if (allLocalURLs || forceAttachment) {
                for (const QUrl &url : urlList) {
                    addAttachment(url, QString());
                }
            } else {
                QMenu p;
                const int sizeUrl(urlList.size());
                const QAction *addAsTextAction = p.addAction(i18np("Add URL into Message", "Add URLs into Message", sizeUrl));
                const QAction *addAsAttachmentAction = p.addAction(i18np("Add File as &Attachment", "Add Files as &Attachment", sizeUrl));
                const QAction *selectedAction = p.exec(QCursor::pos());

                if (selectedAction == addAsTextAction) {
                    insertUrls(source, urlList);
                } else if (selectedAction == addAsAttachmentAction) {
                    for (const QUrl &url : urlList) {
                        if (url.isValid()) {
                            addAttachment(url, QString());
                        }
                    }
                }
            }
            return true;
        } else {
            if (!items.isEmpty()) {
                Akonadi::ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(items, this);
                itemFetchJob->fetchScope().fetchFullPayload(true);
                itemFetchJob->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
                connect(itemFetchJob, &Akonadi::ItemFetchJob::result, this, &KMComposerWin::slotFetchJob);
            }
            if (!collections.isEmpty()) {
                qCDebug(KMAIL_LOG) << "Collection dnd not supported";
            }
            return true;
        }
    }
    return false;
}

void KMComposerWin::slotPasteAsAttachment()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData) {
        return;
    }
    if (insertFromMimeData(mimeData, true)) {
        return;
    }
    if (mimeData->hasText()) {
        bool ok;
        const QString attName = QInputDialog::getText(this,
                                                      i18n("Insert clipboard text as attachment"),
                                                      i18n("Name of the attachment:"), QLineEdit::Normal,
                                                      QString(), &ok);
        if (ok) {
            mComposerBase->addAttachment(attName, attName, QStringLiteral("utf-8"), QApplication::clipboard()->text().toUtf8(), "text/plain");
        }
        return;
    }
}

void KMComposerWin::slotFetchJob(KJob *job)
{
    if (showErrorMessage(job)) {
        return;
    }
    Akonadi::ItemFetchJob *fjob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (!fjob) {
        return;
    }
    const Akonadi::Item::List items = fjob->items();

    if (items.isEmpty()) {
        return;
    }

    if (items.first().mimeType() == KMime::Message::mimeType()) {
        uint identity = 0;
        if (items.at(0).isValid()) {
            const Akonadi::Collection parentCollection = items.at(0).parentCollection();
            if (parentCollection.isValid()) {
                const QString resourceName = parentCollection.resource();
                if (!resourceName.isEmpty()) {
                    QSharedPointer<MailCommon::FolderSettings> fd(MailCommon::FolderSettings::forCollection(parentCollection, false));
                    if (fd) {
                        identity = fd->identity();
                    }
                }
            }
        }
        KMCommand *command = new KMForwardAttachedCommand(this, items, identity, this);
        command->start();
    } else {
        for (const Akonadi::Item &item : items) {
            QString attachmentName = QStringLiteral("attachment");
            if (item.hasPayload<KContacts::Addressee>()) {
                const KContacts::Addressee contact = item.payload<KContacts::Addressee>();
                attachmentName = contact.realName() + QLatin1String(".vcf");
                //Workaround about broken kaddressbook fields.
                QByteArray data = item.payloadData();
                KContacts::adaptIMAttributes(data);
                addAttachment(attachmentName, KMime::Headers::CEbase64, QString(), data, "text/x-vcard");
            } else if (item.hasPayload<KContacts::ContactGroup>()) {
                const KContacts::ContactGroup group = item.payload<KContacts::ContactGroup>();
                attachmentName = group.name() + QLatin1String(".vcf");
                Akonadi::ContactGroupExpandJob *expandJob = new Akonadi::ContactGroupExpandJob(group, this);
                expandJob->setProperty("groupName", attachmentName);
                connect(expandJob, &KJob::result, this, &KMComposerWin::slotExpandGroupResult);
                expandJob->start();
            } else {
                addAttachment(attachmentName, KMime::Headers::CEbase64, QString(), item.payloadData(), item.mimeType().toLatin1());
            }
        }
    }
}

void KMComposerWin::slotExpandGroupResult(KJob *job)
{
    Akonadi::ContactGroupExpandJob *expandJob = qobject_cast<Akonadi::ContactGroupExpandJob *>(job);
    Q_ASSERT(expandJob);

    const QString attachmentName = expandJob->property("groupName").toString();
    KContacts::VCardConverter converter;
    const QByteArray groupData = converter.exportVCards(expandJob->contacts(), KContacts::VCardConverter::v3_0);
    if (!groupData.isEmpty()) {
        addAttachment(attachmentName, KMime::Headers::CEbase64, QString(), groupData, "text/x-vcard");
    }
}

void KMComposerWin::slotClose()
{
    close();
}

void KMComposerWin::slotNewComposer()
{
    KMComposerCreateNewComposerJob *job = new KMComposerCreateNewComposerJob;
    job->setCollectionForNewMessage(mCollectionForNewMessage);

    job->setCurrentIdentity(currentIdentity());
    job->start();
}

void KMComposerWin::slotUpdateWindowTitle()
{
    QString s(mEdtSubject->toPlainText());
    mComposerBase->setSubject(s);
    // Remove characters that show badly in most window decorations:
    // newlines tend to become boxes.
    if (s.isEmpty()) {
        setWindowTitle(QLatin1Char('(') + i18n("unnamed") + QLatin1Char(')'));
    } else {
        setWindowTitle(s.replace(QLatin1Char('\n'), QLatin1Char(' ')));
    }
}

void KMComposerWin::slotEncryptToggled(bool on)
{
    setEncryption(on, true);
    updateSignatureAndEncryptionStateIndicators();
}

void KMComposerWin::setEncryption(bool encrypt, bool setByUser)
{
    bool wasModified = isModified();
    if (setByUser) {
        setModified(true);
    }
    if (!mEncryptAction->isEnabled()) {
        encrypt = false;
    }
    // check if the user wants to encrypt messages to himself and if he defined
    // an encryption key for the current identity
    else if (encrypt && encryptToSelf() && !mLastIdentityHasEncryptionKey) {
        if (setByUser) {
            KMessageBox::sorry(this,
                               i18n("<qt><p>You have requested that messages be "
                                    "encrypted to yourself, but the currently selected "
                                    "identity does not define an (OpenPGP or S/MIME) "
                                    "encryption key to use for this.</p>"
                                    "<p>Please select the key(s) to use "
                                    "in the identity configuration.</p>"
                                    "</qt>"),
                               i18n("Undefined Encryption Key"));
            setModified(wasModified);
        }
        encrypt = false;
    }

    // make sure the mEncryptAction is in the right state
    mEncryptAction->setChecked(encrypt);
    mEncryptAction->setProperty("setByUser", setByUser);
    if (!setByUser) {
        updateSignatureAndEncryptionStateIndicators();
    }

    // show the appropriate icon
    if (encrypt) {
        mEncryptAction->setIcon(QIcon::fromTheme(QStringLiteral("document-encrypt")));
    } else {
        mEncryptAction->setIcon(QIcon::fromTheme(QStringLiteral("document-decrypt")));
    }

    if (setByUser) {
        // User has toggled encryption, go over all recipients
        const auto lst = mComposerBase->recipientsEditor()->lines();
        for (auto line : lst) {
            if (encrypt) {
                // Encryption was enabled, update encryption status of all recipients
                slotRecipientAdded(qobject_cast<MessageComposer::RecipientLineNG *>(line));
            } else {
                // Encryption was disabled, remove the encryption indicator
                auto edit = qobject_cast<MessageComposer::RecipientLineNG *>(line);
                edit->setIcon(QIcon());
                auto recipient = edit->data().dynamicCast<MessageComposer::Recipient>();
                recipient->setEncryptionAction(Kleo::Impossible);
                recipient->setKey(GpgME::Key());
            }
        }
    }

    // mark the attachments for (no) encryption
    if (canSignEncryptAttachments()) {
        mComposerBase->attachmentModel()->setEncryptSelected(encrypt);
    }
}

void KMComposerWin::slotSignToggled(bool on)
{
    setSigning(on, true);
    updateSignatureAndEncryptionStateIndicators();
}

void KMComposerWin::setSigning(bool sign, bool setByUser)
{
    bool wasModified = isModified();
    if (setByUser) {
        setModified(true);
    }
    if (!mSignAction->isEnabled()) {
        sign = false;
    }

    // check if the user defined a signing key for the current identity
    if (sign && !mLastIdentityHasSigningKey) {
        if (setByUser) {
            KMessageBox::sorry(this,
                               i18n("<qt><p>In order to be able to sign "
                                    "this message you first have to "
                                    "define the (OpenPGP or S/MIME) signing key "
                                    "to use.</p>"
                                    "<p>Please select the key to use "
                                    "in the identity configuration.</p>"
                                    "</qt>"),
                               i18n("Undefined Signing Key"));
            setModified(wasModified);
        }
        sign = false;
    }

    // make sure the mSignAction is in the right state
    mSignAction->setChecked(sign);

    if (!setByUser) {
        updateSignatureAndEncryptionStateIndicators();
    }
    // mark the attachments for (no) signing
    if (canSignEncryptAttachments()) {
        mComposerBase->attachmentModel()->setSignSelected(sign);
    }
}

void KMComposerWin::slotWordWrapToggled(bool on)
{
    if (on) {
        mComposerBase->editor()->enableWordWrap(validateLineWrapWidth());
    } else {
        disableWordWrap();
    }
}

int KMComposerWin::validateLineWrapWidth()
{
    int lineWrap = MessageComposer::MessageComposerSettings::self()->lineWrapWidth();
    if ((lineWrap == 0) || (lineWrap > 78)) {
        lineWrap = 78;
    } else if (lineWrap < 30) {
        lineWrap = 30;
    }
    return lineWrap;
}

void KMComposerWin::disableWordWrap()
{
    mComposerBase->editor()->disableWordWrap();
}

void KMComposerWin::forceDisableHtml()
{
    mForceDisableHtml = true;
    disableHtml(MessageComposer::ComposerViewBase::NoConfirmationNeeded);
    mMarkupAction->setEnabled(false);
    // FIXME: Remove the toggle toolbar action somehow
}

bool KMComposerWin::isComposing() const
{
    return mComposerBase && mComposerBase->isComposing();
}

void KMComposerWin::disableForgottenAttachmentsCheck()
{
    mCheckForForgottenAttachments = false;
}

void KMComposerWin::slotPrint()
{
    printComposer(false);
}

void KMComposerWin::slotPrintPreview()
{
    printComposer(true);
}

void KMComposerWin::printComposer(bool preview)
{
    MessageComposer::Composer *composer = createSimpleComposer();
    mMiscComposers.append(composer);
    composer->setProperty("preview", preview);
    connect(composer, &MessageComposer::Composer::result, this, &KMComposerWin::slotPrintComposeResult);
    composer->start();
}

void KMComposerWin::slotPrintComposeResult(KJob *job)
{
    const bool preview = job->property("preview").toBool();
    printComposeResult(job, preview);
}

void KMComposerWin::printComposeResult(KJob *job, bool preview)
{
    Q_ASSERT(dynamic_cast< MessageComposer::Composer * >(job));
    MessageComposer::Composer *composer = dynamic_cast< MessageComposer::Composer * >(job);
    Q_ASSERT(mMiscComposers.contains(composer));
    mMiscComposers.removeAll(composer);

    if (composer->error() == MessageComposer::Composer::NoError) {
        Q_ASSERT(composer->resultMessages().size() == 1);
        Akonadi::Item printItem;
        printItem.setPayload<KMime::Message::Ptr>(composer->resultMessages().constFirst());
        Akonadi::MessageFlags::copyMessageFlags(*(composer->resultMessages().constFirst()), printItem);
        const bool isHtml = mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich;
        const MessageViewer::Viewer::DisplayFormatMessage format = isHtml ? MessageViewer::Viewer::Html : MessageViewer::Viewer::Text;
        KMPrintCommandInfo commandInfo;
        commandInfo.mMsg = printItem;
        commandInfo.mFormat = format;
        commandInfo.mHtmlLoadExtOverride = isHtml;
        commandInfo.mPrintPreview = preview;
        KMPrintCommand *command = new KMPrintCommand(this, commandInfo);
        command->start();
    } else {
        showErrorMessage(job);
    }
}

void KMComposerWin::doSend(MessageComposer::MessageSender::SendMethod method, MessageComposer::MessageSender::SaveIn saveIn, bool willSendItWithoutReediting)
{
    if (saveIn == MessageComposer::MessageSender::SaveInNone) {
        const MessageComposer::ComposerViewBase::MissingAttachment forgotAttachment = userForgotAttachment();
        if ((forgotAttachment == MessageComposer::ComposerViewBase::FoundMissingAttachmentAndAddedAttachment)
            || (forgotAttachment == MessageComposer::ComposerViewBase::FoundMissingAttachmentAndCancel)) {
            return;
        }
    }

    //TODO generate new message from plugins.
    MessageComposer::PluginEditorConverterBeforeConvertingData data;
    data.setNewMessage(mContext == TemplateContext::New);
    mPluginEditorConvertTextManagerInterface->setDataBeforeConvertingText(data);

    //TODO converttext if necessary

    // TODO integrate with MDA online status
    if (method == MessageComposer::MessageSender::SendImmediate) {
        if (!MessageComposer::Util::sendMailDispatcherIsOnline()) {
            method = MessageComposer::MessageSender::SendLater;
        }
        if (KMailSettings::self()->enabledUndoSend()) {
            mComposerBase->setSendLaterInfo(nullptr);
            const bool wasRegistered = sendLaterRegistered();
            if (wasRegistered) {
                SendLater::SendLaterInfo *info = new SendLater::SendLaterInfo;
                info->setRecurrence(false);
                info->setSubject(subject());
                info->setDateTime(QDateTime::currentDateTime().addSecs(KMailSettings::self()->undoSendDelay()));
                mComposerBase->setSendLaterInfo(info);
            }
            method = MessageComposer::MessageSender::SendLater;
            willSendItWithoutReediting = true;
            saveIn = MessageComposer::MessageSender::SaveInOutbox;
        }
    }

    if (saveIn == MessageComposer::MessageSender::SaveInNone || willSendItWithoutReediting) {   // don't save as draft or template, send immediately
        if (KEmailAddress::firstEmailAddress(from()).isEmpty()) {
            if (!(mShowHeaders & HDR_FROM)) {
                mShowHeaders |= HDR_FROM;
                rethinkFields(false);
            }
            mEdtFrom->setFocus();
            KMessageBox::sorry(this,
                               i18n("You must enter your email address in the "
                                    "From: field. You should also set your email "
                                    "address for all identities, so that you do "
                                    "not have to enter it for each message."));
            return;
        }
        if (mComposerBase->to().isEmpty()) {
            if (mComposerBase->cc().isEmpty() && mComposerBase->bcc().isEmpty()) {
                KMessageBox::information(this,
                                         i18n("You must specify at least one receiver, "
                                              "either in the To: field or as CC or as BCC."));

                return;
            } else {
                int rc = KMessageBox::questionYesNo(this,
                                                    i18n("To: field is empty. "
                                                         "Send message anyway?"),
                                                    i18n("No To: specified"),
                                                    KStandardGuiItem::yes(),
                                                    KStandardGuiItem::no());
                if (rc == KMessageBox::No) {
                    return;
                }
            }
        }

        if (subject().isEmpty()) {
            mEdtSubject->setFocus();
            int rc
                = KMessageBox::questionYesNo(this,
                                             i18n("You did not specify a subject. "
                                                  "Send message anyway?"),
                                             i18n("No Subject Specified"),
                                             KGuiItem(i18n("S&end as Is")),
                                             KGuiItem(i18n("&Specify the Subject")));
            if (rc == KMessageBox::No) {
                return;
            }
        }

        MessageComposer::PluginEditorCheckBeforeSendParams params;
        params.setSubject(subject());
        params.setHtmlMail(mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich);
        params.setIdentity(mComposerBase->identityCombo()->currentIdentity());
        params.setHasAttachment(mComposerBase->attachmentModel()->rowCount() > 0);
        params.setTransportId(mComposerBase->transportComboBox()->currentTransportId());
        const KIdentityManagement::Identity &ident = KMKernel::self()->identityManager()->identityForUoid(mComposerBase->identityCombo()->currentIdentity());
        QString defaultDomainName;
        if (!ident.isNull()) {
            defaultDomainName = ident.defaultDomainName();
        }
        const QString composerBaseBccTrimmed = mComposerBase->bcc().trimmed();
        const QString composerBaseToTrimmed = mComposerBase->to().trimmed();
        const QString composerBaseCcTrimmed = mComposerBase->cc().trimmed();
        params.setBccAddresses(composerBaseBccTrimmed);
        params.setToAddresses(composerBaseToTrimmed);
        params.setCcAddresses(composerBaseCcTrimmed);
        params.setDefaultDomain(defaultDomainName);

        if (!mPluginEditorCheckBeforeSendManagerInterface->execute(params)) {
            return;
        }
        const QStringList recipients = {composerBaseToTrimmed, composerBaseCcTrimmed, composerBaseBccTrimmed};

        setEnabled(false);

        // Validate the To:, CC: and BCC fields
        AddressValidationJob *job = new AddressValidationJob(recipients.join(QLatin1String(", ")), this, this);
        job->setDefaultDomain(defaultDomainName);
        job->setProperty("method", static_cast<int>(method));
        job->setProperty("saveIn", static_cast<int>(saveIn));
        connect(job, &Akonadi::ItemFetchJob::result, this, &KMComposerWin::slotDoDelayedSend);
        job->start();

        // we'll call send from within slotDoDelaySend
    } else {
        if (saveIn == MessageComposer::MessageSender::SaveInDrafts && mEncryptAction->isChecked()
            && KMailSettings::self()->alwaysEncryptDrafts()
            && mComposerBase->to().isEmpty() && mComposerBase->cc().isEmpty()) {
            KMessageBox::information(this, i18n("You must specify at least one receiver "
                                                "in order to be able to encrypt a draft."));
            return;
        }
        doDelayedSend(method, saveIn);
    }
}

void KMComposerWin::slotDoDelayedSend(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(this, job->errorText());
        setEnabled(true);
        return;
    }

    const AddressValidationJob *validateJob = qobject_cast<AddressValidationJob *>(job);

    // Abort sending if one of the recipient addresses is invalid ...
    if (!validateJob->isValid()) {
        setEnabled(true);
        return;
    }

    // ... otherwise continue as usual
    const MessageComposer::MessageSender::SendMethod method = static_cast<MessageComposer::MessageSender::SendMethod>(job->property("method").toInt());
    const MessageComposer::MessageSender::SaveIn saveIn = static_cast<MessageComposer::MessageSender::SaveIn>(job->property("saveIn").toInt());

    doDelayedSend(method, saveIn);
}

void KMComposerWin::applyComposerSetting(MessageComposer::ComposerViewBase *mComposerBase)
{
    QVector< QByteArray > charsets = mCodecAction->mimeCharsets();
    if (!mOriginalPreferredCharset.isEmpty()) {
        charsets.insert(0, mOriginalPreferredCharset);
    }
    mComposerBase->setFrom(from());
    mComposerBase->setSubject(subject());
    mComposerBase->setCharsets(charsets);
    mComposerBase->setUrgent(mUrgentAction->isChecked());
    mComposerBase->setMDNRequested(mRequestMDNAction->isChecked());
    mComposerBase->setRequestDeleveryConfirmation(mRequestDeliveryConfirmation->isChecked());
}

void KMComposerWin::doDelayedSend(MessageComposer::MessageSender::SendMethod method, MessageComposer::MessageSender::SaveIn saveIn)
{
#ifndef QT_NO_CURSOR
    KPIM::KCursorSaver busy(KPIM::KBusyPtr::busy());
#endif
    applyComposerSetting(mComposerBase);
    if (mForceDisableHtml) {
        disableHtml(MessageComposer::ComposerViewBase::NoConfirmationNeeded);
    }
    bool sign = mSignAction->isChecked();
    bool encrypt = mEncryptAction->isChecked();

    mComposerBase->setCryptoOptions(sign, encrypt, cryptoMessageFormat(),
                                    ((saveIn != MessageComposer::MessageSender::SaveInNone && !KMailSettings::self()->alwaysEncryptDrafts())
                                     || mSigningAndEncryptionExplicitlyDisabled));

    const int num = KMailSettings::self()->customMessageHeadersCount();
    QMap<QByteArray, QString> customHeader;
    for (int ix = 0; ix < num; ++ix) {
        CustomMimeHeader customMimeHeader(QString::number(ix));
        customMimeHeader.load();
        customHeader.insert(customMimeHeader.custHeaderName().toLatin1(), customMimeHeader.custHeaderValue());
    }

    QMap<QByteArray, QString>::const_iterator extraCustomHeader = mExtraHeaders.constBegin();
    while (extraCustomHeader != mExtraHeaders.constEnd()) {
        customHeader.insert(extraCustomHeader.key(), extraCustomHeader.value());
        ++extraCustomHeader;
    }

    mComposerBase->setCustomHeader(customHeader);
    mComposerBase->send(method, saveIn, false);
}

bool KMComposerWin::sendLaterRegistered() const
{
    return SendLater::SendLaterUtil::sentLaterAgentWasRegistered() && SendLater::SendLaterUtil::sentLaterAgentEnabled();
}

void KMComposerWin::slotSendLater()
{
    if (!TransportManager::self()->showTransportCreationDialog(this, TransportManager::IfNoTransportExists)) {
        return;
    }
    if (!checkRecipientNumber()) {
        return;
    }
    mComposerBase->setSendLaterInfo(nullptr);
    if (mComposerBase->editor()->checkExternalEditorFinished()) {
        const bool wasRegistered = sendLaterRegistered();
        if (wasRegistered) {
            SendLater::SendLaterInfo *info = nullptr;
            QPointer<SendLater::SendLaterDialog> dlg = new SendLater::SendLaterDialog(info, this);
            if (dlg->exec()) {
                info = dlg->info();
                const SendLater::SendLaterDialog::SendLaterAction action = dlg->action();
                delete dlg;
                switch (action) {
                case SendLater::SendLaterDialog::Unknown:
                    qCDebug(KMAIL_LOG) << "Sendlater action \"Unknown\": Need to fix it.";
                    break;
                case SendLater::SendLaterDialog::Canceled:
                    return;
                    break;
                case SendLater::SendLaterDialog::PutInOutbox:
                    doSend(MessageComposer::MessageSender::SendLater);
                    break;
                case SendLater::SendLaterDialog::SendDeliveryAtTime:
                    mComposerBase->setSendLaterInfo(info);
                    if (info->isRecurrence()) {
                        doSend(MessageComposer::MessageSender::SendLater, MessageComposer::MessageSender::SaveInTemplates, true);
                    } else {
                        doSend(MessageComposer::MessageSender::SendLater, MessageComposer::MessageSender::SaveInDrafts, true);
                    }
                    break;
                }
            } else {
                delete dlg;
            }
        } else {
            doSend(MessageComposer::MessageSender::SendLater);
        }
    }
}

void KMComposerWin::slotSaveDraft()
{
    if (mComposerBase->editor()->checkExternalEditorFinished()) {
        doSend(MessageComposer::MessageSender::SendLater, MessageComposer::MessageSender::SaveInDrafts);
    }
}

void KMComposerWin::slotSaveTemplate()
{
    if (mComposerBase->editor()->checkExternalEditorFinished()) {
        doSend(MessageComposer::MessageSender::SendLater, MessageComposer::MessageSender::SaveInTemplates);
    }
}

void KMComposerWin::slotSendNowVia(MailTransport::Transport *transport)
{
    if (transport) {
        mComposerBase->transportComboBox()->setCurrentTransport(transport->id());
        slotSendNow();
    }
}

void KMComposerWin::slotSendLaterVia(MailTransport::Transport *transport)
{
    if (transport) {
        mComposerBase->transportComboBox()->setCurrentTransport(transport->id());
        slotSendLater();
    }
}

void KMComposerWin::sendNow(bool shortcutUsed)
{
    if (!mComposerBase->editor()->checkExternalEditorFinished()) {
        return;
    }
    if (!TransportManager::self()->showTransportCreationDialog(this, TransportManager::IfNoTransportExists)) {
        return;
    }
    if (!checkRecipientNumber()) {
        return;
    }
    mSendNowByShortcutUsed = shortcutUsed;
    if (KMailSettings::self()->checkSpellingBeforeSend()) {
        mComposerBase->editor()->forceSpellChecking();
    } else {
        slotCheckSendNow();
    }
}

void KMComposerWin::slotSendNowByShortcut()
{
    sendNow(true);
}

void KMComposerWin::slotSendNow()
{
    sendNow(false);
}

void KMComposerWin::confirmBeforeSend()
{
    const int rc = KMessageBox::warningYesNoCancel(mMainWidget,
                                                   i18n("About to send email..."),
                                                   i18n("Send Confirmation"),
                                                   KGuiItem(i18n("&Send Now")),
                                                   KGuiItem(i18n("Send &Later")));

    if (rc == KMessageBox::Yes) {
        doSend(MessageComposer::MessageSender::SendImmediate);
    } else if (rc == KMessageBox::No) {
        doSend(MessageComposer::MessageSender::SendLater);
    }
}

void KMComposerWin::slotCheckSendNowStep2()
{
    if (KMailSettings::self()->confirmBeforeSend()) {
        confirmBeforeSend();
    } else {
        if (mSendNowByShortcutUsed) {
            if (!KMailSettings::self()->checkSendDefaultActionShortcut()) {
                ValidateSendMailShortcut validateShortcut(actionCollection(), this);
                if (!validateShortcut.validate()) {
                    return;
                }
            }
            if (KMailSettings::self()->confirmBeforeSendWhenUseShortcut()) {
                confirmBeforeSend();
                return;
            }
        }
        doSend(MessageComposer::MessageSender::SendImmediate);
    }
}

void KMComposerWin::slotDelayedCheckSendNow()
{
    QTimer::singleShot(0, this, &KMComposerWin::slotCheckSendNow);
}

void KMComposerWin::slotCheckSendNow()
{
    QStringList lst {mComposerBase->to()};
    const QString ccStr = mComposerBase->cc();
    if (!ccStr.isEmpty()) {
        lst << ccStr.split(QLatin1Char(','));
    }
    const QString bccStr = mComposerBase->bcc();
    if (!bccStr.isEmpty()) {
        lst << bccStr.split(QLatin1Char(','));
    }
    if (lst.isEmpty()) {
        slotCheckSendNowStep2();
    } else {
        PotentialPhishingEmailJob *job = new PotentialPhishingEmailJob(this);
        KConfigGroup group(KSharedConfig::openConfig(), "PotentialPhishing");
        const QStringList whiteList = group.readEntry("whiteList", QStringList());
        job->setEmailWhiteList(whiteList);
        job->setPotentialPhishingEmails(lst);
        connect(job, &PotentialPhishingEmailJob::potentialPhishingEmailsFound, this, &KMComposerWin::slotPotentialPhishingEmailsFound);
        job->start();
    }
}

void KMComposerWin::slotPotentialPhishingEmailsFound(const QStringList &list)
{
    if (list.isEmpty()) {
        slotCheckSendNowStep2();
    } else {
        mPotentialPhishingEmailWarning->setPotentialPhisingEmail(list);
    }
}

bool KMComposerWin::checkRecipientNumber() const
{
    const int thresHold = KMailSettings::self()->recipientThreshold();
    if (KMailSettings::self()->tooManyRecipients() && mComposerBase->recipientsEditor()->recipients().count() > thresHold) {
        if (KMessageBox::questionYesNo(mMainWidget,
                                       i18n("You are trying to send the mail to more than %1 recipients. Send message anyway?", thresHold),
                                       i18n("Too many recipients"),
                                       KGuiItem(i18n("&Send as Is")),
                                       KGuiItem(i18n("&Edit Recipients"))) == KMessageBox::No) {
            return false;
        }
    }
    return true;
}

void KMComposerWin::enableHtml()
{
    if (mForceDisableHtml) {
        disableHtml(MessageComposer::ComposerViewBase::NoConfirmationNeeded);
        return;
    }

    mComposerBase->editor()->activateRichText();
    if (!toolBar(QStringLiteral("htmlToolBar"))->isVisible()) {
        // Use singleshot, as we we might actually be called from a slot that wanted to disable the
        // toolbar (but the messagebox in disableHtml() prevented that and called us).
        // The toolbar can't correctly deal with being enabled right in a slot called from the "disabled"
        // signal, so wait one event loop run for that.
        QTimer::singleShot(0, toolBar(QStringLiteral("htmlToolBar")), &QWidget::show);
    }
    if (!mMarkupAction->isChecked()) {
        mMarkupAction->setChecked(true);
    }

    mComposerBase->editor()->composerActions()->updateActionStates();
    mComposerBase->editor()->composerActions()->setActionsEnabled(true);
}

void KMComposerWin::disableHtml(MessageComposer::ComposerViewBase::Confirmation confirmation)
{
    bool forcePlainTextMarkup = false;
    if (confirmation == MessageComposer::ComposerViewBase::LetUserConfirm && mComposerBase->editor()->composerControler()->isFormattingUsed() && !mForceDisableHtml) {
        int choice = KMessageBox::warningYesNoCancel(this, i18n("Turning HTML mode off "
                                                                "will cause the text to lose the formatting. Are you sure?"),
                                                     i18n("Lose the formatting?"), KGuiItem(i18n("Lose Formatting")), KGuiItem(i18n("Add Markup Plain Text")), KStandardGuiItem::cancel(),
                                                     QStringLiteral("LoseFormattingWarning"));

        switch (choice) {
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
    mComposerBase->editor()->composerActions()->setActionsEnabled(false);

    slotUpdateFont();
    if (toolBar(QStringLiteral("htmlToolBar"))->isVisible()) {
        // See the comment in enableHtml() why we use a singleshot timer, similar situation here.
        QTimer::singleShot(0, toolBar(QStringLiteral("htmlToolBar")), &QWidget::hide);
    }
    if (mMarkupAction->isChecked()) {
        mMarkupAction->setChecked(false);
    }
}

void KMComposerWin::slotToggleMarkup()
{
    htmlToolBarVisibilityChanged(mMarkupAction->isChecked());
}

void KMComposerWin::slotTextModeChanged(MessageComposer::RichTextComposerNg::Mode mode)
{
    if (mode == MessageComposer::RichTextComposerNg::Plain) {
        disableHtml(MessageComposer::ComposerViewBase::NoConfirmationNeeded);    // ### Can this happen at all?
    } else {
        enableHtml();
    }
    enableDisablePluginActions(mode == MessageComposer::RichTextComposerNg::Rich);
}

void KMComposerWin::enableDisablePluginActions(bool richText)
{
    mPluginEditorConvertTextManagerInterface->enableDisablePluginActions(richText);
}

void KMComposerWin::htmlToolBarVisibilityChanged(bool visible)
{
    if (visible) {
        enableHtml();
    } else {
        disableHtml(MessageComposer::ComposerViewBase::LetUserConfirm);
    }
}

void KMComposerWin::slotAutoSpellCheckingToggled(bool on)
{
    mAutoSpellCheckingAction->setChecked(on);
    if (on != mComposerBase->editor()->checkSpellingEnabled()) {
        mComposerBase->editor()->setCheckSpellingEnabled(on);
    }
    if (on != mEdtSubject->checkSpellingEnabled()) {
        mEdtSubject->setCheckSpellingEnabled(on);
    }
    mStatusBarLabelSpellCheckingChangeMode->setToggleMode(on);
}

void KMComposerWin::slotSpellCheckingStatus(const QString &status)
{
    mStatusbarLabel->setText(status);
    QTimer::singleShot(2000, this, &KMComposerWin::slotSpellcheckDoneClearStatus);
}

void KMComposerWin::slotSpellcheckDoneClearStatus()
{
    mStatusbarLabel->clear();
}

void KMComposerWin::slotIdentityChanged(uint uoid, bool initialChange)
{
    if (!mMsg) {
        qCDebug(KMAIL_LOG) << "Trying to change identity but mMsg == 0!";
        return;
    }
    const KIdentityManagement::Identity &ident
        = KMKernel::self()->identityManager()->identityForUoid(uoid);
    if (ident.isNull()) {
        return;
    }
    bool wasModified(isModified());
    Q_EMIT identityChanged(identity());
    if (!ident.fullEmailAddr().isNull()) {
        mEdtFrom->setText(ident.fullEmailAddr());
    }

    // make sure the From field is shown if it does not contain a valid email address
    if (KEmailAddress::firstEmailAddress(from()).isEmpty()) {
        mShowHeaders |= HDR_FROM;
    }

    // remove BCC of old identity and add BCC of new identity (if they differ)
    const KIdentityManagement::Identity &oldIdentity
        = KMKernel::self()->identityManager()->identityForUoidOrDefault(mId);

    if (ident.organization().isEmpty()) {
        mMsg->removeHeader<KMime::Headers::Organization>();
    } else {
        KMime::Headers::Organization *const organization = new KMime::Headers::Organization;
        organization->fromUnicodeString(ident.organization(), "utf-8");
        mMsg->setHeader(organization);
    }
    if (!ident.isXFaceEnabled() || ident.xface().isEmpty()) {
        mMsg->removeHeader("X-Face");
    } else {
        QString xface = ident.xface();
        if (!xface.isEmpty()) {
            int numNL = (xface.length() - 1) / 70;
            for (int i = numNL; i > 0; --i) {
                xface.insert(i * 70, QStringLiteral("\n\t"));
            }
            KMime::Headers::Generic *header = new KMime::Headers::Generic("X-Face");
            header->fromUnicodeString(xface, "utf-8");
            mMsg->setHeader(header);
        }
    }

    if (initialChange) {
        if (auto hrd = mMsg->headerByType("X-KMail-Transport")) {
            const QString mailtransportStr = hrd->asUnicodeString();
            if (!mailtransportStr.isEmpty()) {
                int transportId = mailtransportStr.toInt();
                const Transport *transport = TransportManager::self()->transportById(transportId, false); /*don't return default transport */
                if (transport) {
                    KMime::Headers::Generic *header = new KMime::Headers::Generic("X-KMail-Transport");
                    header->fromUnicodeString(QString::number(transport->id()), "utf-8");
                    mMsg->setHeader(header);
                    mComposerBase->transportComboBox()->setCurrentTransport(transport->id());
                } else {
                    if (auto hrd = mMsg->headerByType("X-KMail-Transport-Name")) {
                        const QString identityStrName = hrd->asUnicodeString();
                        const Transport *transport = TransportManager::self()->transportByName(identityStrName, true);
                        if (transport) {
                            KMime::Headers::Generic *header = new KMime::Headers::Generic("X-KMail-Transport");
                            header->fromUnicodeString(QString::number(transport->id()), "utf-8");
                            mMsg->setHeader(header);
                            mComposerBase->transportComboBox()->setCurrentTransport(transport->id());
                        } else {
                            mComposerBase->transportComboBox()->setCurrentTransport(TransportManager::self()->defaultTransportId());
                        }
                    } else {
                        mComposerBase->transportComboBox()->setCurrentTransport(TransportManager::self()->defaultTransportId());
                    }
                }
            }
        } else {
            const int transportId = ident.transport().isEmpty() ? -1 : ident.transport().toInt();
            const Transport *transport = TransportManager::self()->transportById(transportId, true);
            if (transport) {
                KMime::Headers::Generic *header = new KMime::Headers::Generic("X-KMail-Transport");
                header->fromUnicodeString(QString::number(transport->id()), "utf-8");
                mMsg->setHeader(header);
                mComposerBase->transportComboBox()->setCurrentTransport(transport->id());
            } else {
                mComposerBase->transportComboBox()->setCurrentTransport(TransportManager::self()->defaultTransportId());
            }
        }
    } else {
        const int transportId = ident.transport().isEmpty() ? -1 : ident.transport().toInt();
        const Transport *transport = TransportManager::self()->transportById(transportId, true);
        if (!transport) {
            mMsg->removeHeader("X-KMail-Transport");
            mComposerBase->transportComboBox()->setCurrentTransport(TransportManager::self()->defaultTransportId());
        } else {
            KMime::Headers::Generic *header = new KMime::Headers::Generic("X-KMail-Transport");
            header->fromUnicodeString(QString::number(transport->id()), "utf-8");
            mMsg->setHeader(header);
            mComposerBase->transportComboBox()->setCurrentTransport(transport->id());
        }
    }

    const bool fccIsDisabled = ident.disabledFcc();
    if (fccIsDisabled) {
        KMime::Headers::Generic *header = new KMime::Headers::Generic("X-KMail-FccDisabled");
        header->fromUnicodeString(QStringLiteral("true"), "utf-8");
        mMsg->setHeader(header);
    } else {
        mMsg->removeHeader("X-KMail-FccDisabled");
    }
    mFccFolder->setEnabled(!fccIsDisabled);

    mComposerBase->dictionary()->setCurrentByDictionaryName(ident.dictionary());
    slotSpellCheckingLanguage(mComposerBase->dictionary()->currentDictionary());
    if (!mPreventFccOverwrite) {
        setFcc(ident.fcc());
    }
    // if unmodified, apply new template, if one is set
    if (!wasModified && !(ident.templates().isEmpty() && mCustomTemplate.isEmpty())
        && !initialChange) {
        applyTemplate(uoid, mId, ident, wasModified);
    } else {
        mComposerBase->identityChanged(ident, oldIdentity, false);
        mEdtSubject->setAutocorrectionLanguage(ident.autocorrectionLanguage());
        updateComposerAfterIdentityChanged(ident, uoid, wasModified);
    }
}

void KMComposerWin::updateComposerAfterIdentityChanged(const KIdentityManagement::Identity &ident, uint uoid, bool wasModified)
{
    // disable certain actions if there is no PGP user identity set
    // for this profile
    bool bNewIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
    bool bNewIdentityHasEncryptionKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
    // save the state of the sign and encrypt button
    if (!bNewIdentityHasEncryptionKey && mLastIdentityHasEncryptionKey) {
        mLastEncryptActionState = mEncryptAction->isChecked();
        setEncryption(false);
    }
    if (!bNewIdentityHasSigningKey && mLastIdentityHasSigningKey) {
        mLastSignActionState = mSignAction->isChecked();
        setSigning(false);
    }
    // restore the last state of the sign and encrypt button
    if (bNewIdentityHasEncryptionKey && !mLastIdentityHasEncryptionKey) {
        setEncryption(mLastEncryptActionState);
    }
    if (bNewIdentityHasSigningKey && !mLastIdentityHasSigningKey) {
        setSigning(mLastSignActionState);
    }

    mCryptoModuleAction->setCurrentItem(format2cb(
                                            Kleo::stringToCryptoMessageFormat(ident.preferredCryptoMessageFormat())));
    slotSelectCryptoModule(true);

    mLastIdentityHasSigningKey = bNewIdentityHasSigningKey;
    mLastIdentityHasEncryptionKey = bNewIdentityHasEncryptionKey;
    const KIdentityManagement::Signature sig = const_cast<KIdentityManagement::Identity &>(ident).signature();
    bool isEnabledSignature = sig.isEnabledSignature();
    mAppendSignature->setEnabled(isEnabledSignature);
    mPrependSignature->setEnabled(isEnabledSignature);
    mInsertSignatureAtCursorPosition->setEnabled(isEnabledSignature);

    mId = uoid;
    changeCryptoAction();
    // make sure the From and BCC fields are shown if necessary
    rethinkFields(false);
    setModified(wasModified);
}

void KMComposerWin::slotSpellcheckConfig()
{
    static_cast<KMComposerEditorNg *>(mComposerBase->editor())->showSpellConfigDialog(QStringLiteral("kmail2rc"));
}

void KMComposerWin::slotEditToolbars()
{
    QPointer<KEditToolBar> dlg = new KEditToolBar(guiFactory(), this);

    connect(dlg.data(), &KEditToolBar::newToolBarConfig, this, &KMComposerWin::slotUpdateToolbars);

    dlg->exec();
    delete dlg;
}

void KMComposerWin::slotUpdateToolbars()
{
    createGUI(QStringLiteral("kmcomposerui.rc"));
    applyMainWindowSettings(KMKernel::self()->config()->group("Composer"));
}

void KMComposerWin::slotEditKeys()
{
    KShortcutsDialog::configure(actionCollection(),
                                KShortcutsEditor::LetterShortcutsDisallowed);
}

void KMComposerWin::setFocusToEditor()
{
    // The cursor position is already set by setMsg(), so we only need to set the
    // focus here.
    mComposerBase->editor()->setFocus();
}

void KMComposerWin::setFocusToSubject()
{
    mEdtSubject->setFocus();
}

void KMComposerWin::slotCompletionModeChanged(KCompletion::CompletionMode mode)
{
    KMailSettings::self()->setCompletionMode((int)mode);

    // sync all the lineedits to the same completion mode
    mEdtFrom->setCompletionMode(mode);
    mComposerBase->recipientsEditor()->setCompletionMode(mode);
}

void KMComposerWin::slotConfigChanged()
{
    readConfig(true /*reload*/);
    mComposerBase->updateAutoSave();
    rethinkFields();
    slotWordWrapToggled(mWordWrapAction->isChecked());
}

/*
 * checks if the drafts-folder has been deleted
 * that is not nice so we set the system-drafts-folder
 */
void KMComposerWin::slotFolderRemoved(const Akonadi::Collection &col)
{
    qCDebug(KMAIL_LOG) << "you killed me.";
    // TODO: need to handle templates here?
    if ((mFolder.isValid()) && (col.id() == mFolder.id())) {
        mFolder = CommonKernel->draftsCollectionFolder();
        qCDebug(KMAIL_LOG) << "restoring drafts to" << mFolder.id();
    } else if (col.id() == mFccFolder->collection().id()) {
        qCDebug(KMAIL_LOG) << "FCC was removed " << col.id();
        mFccFolder->setCollection(CommonKernel->sentCollectionFolder());
        mIncorrectIdentityFolderWarning->fccIsInvalid();
    }
}

void KMComposerWin::slotOverwriteModeChanged()
{
    const bool overwriteMode = mComposerBase->editor()->overwriteMode();
    mComposerBase->editor()->setCursorWidth(overwriteMode ? 5 : 1);
    mStatusBarLabelToggledOverrideMode->setToggleMode(overwriteMode);
}

void KMComposerWin::slotCursorPositionChanged()
{
    // Change Line/Column info in status bar
    const int line = mComposerBase->editor()->linePosition() + 1;
    const int col = mComposerBase->editor()->columnNumber() + 1;
    QString temp = i18nc("Shows the linenumber of the cursor position.", " Line: %1 ", line);
    mCursorLineLabel->setText(temp);
    temp = i18n(" Column: %1 ", col);
    mCursorColumnLabel->setText(temp);

    // Show link target in status bar
    if (mComposerBase->editor()->textCursor().charFormat().isAnchor()) {
        const QString text = mComposerBase->editor()->composerControler()->currentLinkText() + QLatin1String(" -> ") + mComposerBase->editor()->composerControler()->currentLinkUrl();
        mStatusbarLabel->setText(text);
    } else {
        mStatusbarLabel->clear();
    }
}

void KMComposerWin::recipientEditorSizeHintChanged()
{
    QTimer::singleShot(1, this, &KMComposerWin::setMaximumHeaderSize);
}

void KMComposerWin::setMaximumHeaderSize()
{
    mHeadersArea->setMaximumHeight(mHeadersArea->sizeHint().height());
}

void KMComposerWin::updateSignatureAndEncryptionStateIndicators()
{
    mCryptoStateIndicatorWidget->updateSignatureAndEncrypionStateIndicators(mSignAction->isChecked(), mEncryptAction->isChecked());
}

void KMComposerWin::slotDictionaryLanguageChanged(const QString &language)
{
    mComposerBase->dictionary()->setCurrentByDictionary(language);
}

void KMComposerWin::slotFccFolderChanged(const Akonadi::Collection &collection)
{
    mComposerBase->setFcc(collection);
    mComposerBase->editor()->document()->setModified(true);
}

void KMComposerWin::slotSaveAsFile()
{
    SaveAsFileJob *job = new SaveAsFileJob(this);
    job->setParentWidget(this);
    job->setHtmlMode(mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich);
    job->setTextDocument(mComposerBase->editor()->document());
    job->start();
    //not necessary to delete it. It done in SaveAsFileJob
}

void KMComposerWin::slotAttachMissingFile()
{
    mComposerBase->attachmentController()->showAddAttachmentFileDialog();
}

void KMComposerWin::slotVerifyMissingAttachmentTimeout()
{
    if (mComposerBase->hasMissingAttachments(KMailSettings::self()->attachmentKeywords())) {
        mAttachmentMissing->animatedShow();
    }
}

void KMComposerWin::slotExplicitClosedMissingAttachment()
{
    if (mVerifyMissingAttachment) {
        mVerifyMissingAttachment->stop();
        delete mVerifyMissingAttachment;
        mVerifyMissingAttachment = nullptr;
    }
}

void KMComposerWin::addExtraCustomHeaders(const QMap<QByteArray, QString> &headers)
{
    mExtraHeaders = headers;
}

bool KMComposerWin::processModifyText(QKeyEvent *event)
{
    return mPluginEditorManagerInterface->processProcessKeyEvent(event);
}

MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus KMComposerWin::convertPlainText(MessageComposer::TextPart *textPart)
{
    return mPluginEditorConvertTextManagerInterface->convertTextToFormat(textPart);
}

void KMComposerWin::slotExternalEditorStarted()
{
    mComposerBase->identityCombo()->setEnabled(false);
    mExternalEditorWarning->show();
}

void KMComposerWin::slotExternalEditorClosed()
{
    mComposerBase->identityCombo()->setEnabled(true);
    mExternalEditorWarning->hide();
}

void KMComposerWin::slotInsertShortUrl(const QString &url)
{
    mComposerBase->editor()->composerControler()->insertLink(url);
}

void KMComposerWin::slotTransportChanged()
{
    mComposerBase->editor()->document()->setModified(true);
}

void KMComposerWin::slotFollowUpMail(bool toggled)
{
    if (toggled) {
        QPointer<MessageComposer::FollowUpReminderSelectDateDialog> dlg = new MessageComposer::FollowUpReminderSelectDateDialog(this);
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

void KMComposerWin::slotSnippetWidgetVisibilityChanged(bool b)
{
    mSnippetWidget->setVisible(b);
    mSnippetSplitterCollapser->setVisible(b);
}

void KMComposerWin::slotOverwriteModeWasChanged(bool state)
{
    mComposerBase->editor()->setCursorWidth(state ? 5 : 1);
    mComposerBase->editor()->setOverwriteMode(state);
}

QList<KToggleAction *> KMComposerWin::customToolsList() const
{
    return mCustomToolsWidget->actionList();
}

QList<QAction *> KMComposerWin::pluginToolsActionListForPopupMenu() const
{
    return mPluginEditorManagerInterface->actionsType(MessageComposer::PluginActionType::PopupMenu) + mPluginEditorConvertTextManagerInterface->actionsType(MessageComposer::PluginActionType::PopupMenu);
}

void KMComposerWin::slotRecipientEditorLineAdded(KPIM::MultiplyingLine *line_)
{
    auto line = qobject_cast<MessageComposer::RecipientLineNG *>(line_);
    Q_ASSERT(line);

    connect(line, &MessageComposer::RecipientLineNG::countChanged,
            this, [this, line]() {
        this->slotRecipientAdded(line);
    });
    connect(line, &MessageComposer::RecipientLineNG::iconClicked,
            this, [this, line]() {
        this->slotRecipientLineIconClicked(line);
    });
    connect(line, &MessageComposer::RecipientLineNG::destroyed,
            this, &KMComposerWin::slotRecipientEditorFocusChanged,
            Qt::QueuedConnection);
    connect(line, &MessageComposer::RecipientLineNG::activeChanged,
            this, [this, line]() {
        this->slotRecipientFocusLost(line);
    });

    slotRecipientEditorFocusChanged();
}

void KMComposerWin::slotRecipientEditorFocusChanged()
{
    // Already disabled
    if (mEncryptAction->property("setByUser").toBool()) {
        return;
    }

    // Focus changed, which basically means that user "committed" a new recipient.
    // If we have at least one recipient that does not have a key, disable encryption
    // (unless user enabled it manually), because we want to encrypt by default,
    // but not by force
    bool encrypt = false;
    const auto lst = mComposerBase->recipientsEditor()->lines();
    for (auto line_ : lst) {
        auto line = qobject_cast<MessageComposer::RecipientLineNG *>(line_);

        // There's still a lookup job running, so wait, slotKeyForMailBoxResult()
        // will call us if the job returns empty key
        if (line->property("keyLookupJob").isValid()) {
            return;
        }

        const auto keyStatus = static_cast<CryptoKeyState>(line->property("keyStatus").toInt());
        if (keyStatus == NoState) {
            continue;
        }

        if (!line->recipient()->isEmpty() && keyStatus != KeyOk) {
            setEncryption(false, false);
            return;
        }

        encrypt = true;
    }

    if (encrypt) {
        setEncryption(true, false);
    }
}

void KMComposerWin::slotRecipientLineIconClicked(MessageComposer::RecipientLineNG *line)
{
    const auto data = line->data().dynamicCast<MessageComposer::Recipient>();

    if (!data->key().isNull()) {
        QProcess::startDetached(QStringLiteral("kleopatra"), {
            QStringLiteral("--query"),
            QString::fromLatin1(data->key().primaryFingerprint()),
            QStringLiteral("--parent-windowid"),
            QString::number(winId())
        });
    }
}

void KMComposerWin::slotRecipientAdded(MessageComposer::RecipientLineNG *line)
{
    // User has disabled encryption, don't bother checking the key...
    if (!mEncryptAction->isChecked() && mEncryptAction->property("setByUser").toBool()) {
        return;
    }

    // Same if auto-encryption is not enabled in current identity settings
    if (!identity().pgpAutoEncrypt() || identity().pgpEncryptionKey().isEmpty()) {
        return;
    }

    if (line->recipientsCount() == 0) {
        return;
    }

    const auto protocol = QGpgME::openpgp();
    // If we don't have gnupg we can't look for keys
    if (!protocol) {
        return;
    }

    auto recipient = line->data().dynamicCast<MessageComposer::Recipient>();
    // check if is an already running key lookup job and if so, cancel it
    // this is to prevent a slower job overwriting results of the job that we
    // are about to start now.
    const auto runningJob = line->property("keyLookupJob").value<QPointer<QGpgME::KeyForMailboxJob> >();
    if (runningJob) {
        disconnect(runningJob.data(), &QGpgME::KeyForMailboxJob::result, this, &KMComposerWin::slotKeyForMailBoxResult);
        runningJob->slotCancel();
        line->setProperty("keyLookupJob", QVariant());
    }

    QGpgME::KeyForMailboxJob *job = protocol->keyForMailboxJob();
    if (!job) {
        line->setProperty("keyStatus", NoKey);
        recipient->setEncryptionAction(Kleo::Impossible);
        return;
    }

    QString dummy, addrSpec;
    if (KEmailAddress::splitAddress(recipient->email(), dummy, addrSpec, dummy) != KEmailAddress::AddressOk) {
        addrSpec = recipient->email();
    }
    line->setProperty("keyLookupJob", QVariant::fromValue(QPointer<QGpgME::KeyForMailboxJob>(job)));
    job->setProperty("recipient", QVariant::fromValue(recipient));
    job->setProperty("line", QVariant::fromValue(QPointer<MessageComposer::RecipientLineNG>(line)));
    connect(job, &QGpgME::KeyForMailboxJob::result, this, &KMComposerWin::slotKeyForMailBoxResult);
    job->start(addrSpec, true);
}

void KMComposerWin::slotRecipientFocusLost(MessageComposer::RecipientLineNG *line)
{
    if (mEncryptAction->property("setByUser").toBool()) {
        return;
    }

    // Same if auto-encryption is not enabled in current identity settings
    if (!identity().pgpAutoEncrypt() || identity().pgpEncryptionKey().isEmpty()) {
        return;
    }

    if (line->recipientsCount() == 0) {
        return;
    }

    if (line->property("keyLookupJob").toBool()) {
        return;
    }

    if (static_cast<CryptoKeyState>(line->property("keyStatus").toInt()) != KeyOk) {
        line->setProperty("keyStatus", NoKey);
        setEncryption(false, false);
    }
}

void KMComposerWin::slotKeyForMailBoxResult(const GpgME::KeyListResult &, const GpgME::Key &key, const GpgME::UserID &userID)
{
    QObject *job = sender();
    Q_ASSERT(job);

    // Check if the encryption was explicitly disabled while the job was running
    if (!mEncryptAction->isChecked() && mEncryptAction->property("setByUser").toBool()) {
        return;
    }

    auto recipient = job->property("recipient").value<MessageComposer::Recipient::Ptr>();
    auto line = job->property("line").value<QPointer<MessageComposer::RecipientLineNG> >();

    if (!recipient || !line) {
        return;
    }

    line->setProperty("keyLookupJob", QVariant());
    if (key.isNull()) {
        recipient->setEncryptionAction(Kleo::Impossible); // no key
        line->setIcon(QIcon());
        line->setProperty("keyStatus", InProgress);
    } else {
        recipient->setEncryptionAction(Kleo::DoIt);
        recipient->setKey(key);

        const QIcon icon = QIcon::fromTheme(QStringLiteral("gpg"));
        QIcon overlay;
        QString tooltip;
        switch (userID.validity()) {
        case GpgME::UserID::Ultimate:
        case GpgME::UserID::Full:
            overlay = QIcon::fromTheme(QStringLiteral("emblem-favorite"));
            tooltip = i18n("High security encryption will be used for this recipient (the encryption key is fully trusted). "
                           "Click the icon for details.");
            break;
        case GpgME::UserID::Marginal:
            overlay = QIcon::fromTheme(QStringLiteral("emblem-success"));
            tooltip = i18n("Medium security encryption will be used for this recipient (the encryption key is marginally trusted). "
                           "Click the icon for details.");
            break;
        case GpgME::UserID::Never:
            overlay = QIcon::fromTheme(QStringLiteral("emblem-error"));
            tooltip = i18n("Low security encryption will be used for this recipient (the encryption key is untrusted). "
                           "Click the icon for details.");
            break;
        case GpgME::UserID::Undefined:
        case GpgME::UserID::Unknown:
            overlay = QIcon::fromTheme(QStringLiteral("emblem-information"));
            tooltip = i18n("The email to this recipient will be encrypted, but the security of the encryption is unknown "
                           "(the encryption key could not be verified). Click the icon for details.");
            break;
        }

        line->setProperty("keyStatus", KeyOk);
        line->setIcon(KIconUtils::addOverlay(icon, overlay, Qt::BottomRightCorner), tooltip);

        slotRecipientEditorFocusChanged();
    }
}

void KMComposerWin::slotIdentityDeleted(uint uoid)
{
    if (mComposerBase->identityCombo()->currentIdentity() == uoid) {
        mIncorrectIdentityFolderWarning->identityInvalid();
    }
}

void KMComposerWin::slotTransportRemoved(int id, const QString &name)
{
    Q_UNUSED(name);
    if (mComposerBase->transportComboBox()->currentTransportId() == id) {
        mIncorrectIdentityFolderWarning->mailTransportIsInvalid();
    }
}

void KMComposerWin::slotSelectionChanged()
{
    Q_EMIT mPluginEditorManagerInterface->textSelectionChanged(mRichTextEditorwidget->editor()->textCursor().hasSelection());
}

void KMComposerWin::slotMessage(const QString &str)
{
    KMessageBox::information(this, str, i18n("Plugin Editor Information"));
}

void KMComposerWin::slotEditorPluginInsertText(const QString &str)
{
    mGlobalAction->slotInsertText(str);
}
