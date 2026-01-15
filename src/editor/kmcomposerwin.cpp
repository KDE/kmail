/*
 * This file is part of KMail.
 * SPDX-FileCopyrightText: 2011-2025 Laurent Montel <montel@kde.org>
 *
 * SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Based on KMail code by:
 * SPDX-FileCopyrightText: 1997 Markus Wuebben <markus.wuebben@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kmcomposerwin.h"
#include "attachment/attachmentcontroller.h"
#include "attachment/attachmentview.h"
#include "config-kmail.h"
#include "custommimeheader.h"
#include "editor/kmcomposereditorng.h"
#include "editor/plugininterface/kmailplugineditorcheckbeforesendmanagerinterface.h"
#include "editor/plugininterface/kmailplugineditorconverttextmanagerinterface.h"
#include "editor/plugininterface/kmailplugineditorinitmanagerinterface.h"
#include "editor/plugininterface/kmailplugineditormanagerinterface.h"
#include "editor/plugininterface/kmailplugingrammareditormanagerinterface.h"
#include "editor/potentialphishingemail/potentialphishingemailjob.h"
#include "editor/potentialphishingemail/potentialphishingemailwarning.h"
#include "editor/warningwidgets/attachmentaddedfromexternalwarning.h"
#include "editor/warningwidgets/incorrectidentityfolderwarning.h"
#include "editor/warningwidgets/nearexpirywarning.h"
#include "editor/warningwidgets/toomanyrecipientswarning.h"
#include "job/addressvalidationjob.h"
#include "job/dndfromarkjob.h"
#include "job/saveasfilejob.h"
#include "job/savedraftjob.h"
#include "kmail_debug.h"
#include "kmcommands.h"
#include "kmcomposercreatenewcomposerjob.h"
#include "kmcomposerglobalaction.h"
#include "kmcomposerupdatetemplatejob.h"
#include "kmkernel.h"
#include "mailcomposeradaptor.h" // TODO port all D-Bus stuff…
#include "settings/kmailsettings.h"
#include "subjectlineeditwithautocorrection.h"
#include "undosend/undosendmanager.h"
#include "util.h"
#include "validatesendmailshortcut.h"
#include "warningwidgets/attachmentmissingwarning.h"
#include "warningwidgets/externaleditorwarning.h"
#include "widgets/cryptostateindicatorwidget.h"
#include "widgets/kactionmenutransport.h"
#include <TextAddonsWidgets/ExecutableUtils>

#if KMAIL_HAVE_ACTIVITY_SUPPORT
#include "activities/activitiesmanager.h"
#include "activities/identityactivities.h"
#include "activities/transportactivities.h"
#endif
#include <templateparser/templatesconfiguration_kfg.h>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/ContactGroupExpandJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/MessageFlags>
#include <Akonadi/MessageStatus>
#include <Akonadi/Monitor>

#include <KContacts/VCardConverter>

#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementCore/IdentityManager>
#include <KIdentityManagementCore/Signature>
#include <KIdentityManagementWidgets/IdentityCombo>

#include <KPIMTextEdit/RichTextComposerActions>
#include <KPIMTextEdit/RichTextComposerControler>
#include <KPIMTextEdit/RichTextComposerImages>
#include <KPIMTextEdit/RichTextExternalComposer>
#include <TextCustomEditor/RichTextEditorWidget>

#include <Libkdepim/ProgressStatusBarWidget>
#include <Libkdepim/StatusbarProgressWidget>

#include <KCursorSaver>

#include <Libkleo/ExpiryChecker>
#include <Libkleo/KeyCache>
#include <Libkleo/KeySelectionDialog>

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
#include <MessageComposer/ComposerJob>
#include <MessageComposer/ComposerLineEdit>
#include <MessageComposer/ComposerViewInterface>
#include <MessageComposer/ConvertSnippetVariablesJob>
#include <MessageComposer/DraftStatus>
#include <MessageComposer/FollowUpReminderSelectDateDialog>
#include <MessageComposer/FollowupReminder>
#include <MessageComposer/FollowupReminderCreateJob>
#include <MessageComposer/GlobalPart>
#include <MessageComposer/InfoPart>
#include <MessageComposer/InsertTextFileJob>
#include <MessageComposer/Kleo_Util>
#include <MessageComposer/MessageComposerSettings>
#include <MessageComposer/MessageHelper>
#include <MessageComposer/PluginActionType>
#include <MessageComposer/PluginEditorCheckBeforeSendParams>
#include <MessageComposer/PluginEditorConverterBeforeConvertingData>
#include <MessageComposer/PluginEditorConverterInitialData>
#include <MessageComposer/PluginEditorInterface>
#include <MessageComposer/RecipientsEditor>
#include <MessageComposer/RichTextComposerSignatures>
#include <MessageComposer/SendLaterDialog>
#include <MessageComposer/SendLaterInfo>
#include <MessageComposer/SendLaterUtil>
#include <MessageComposer/SignatureController>
#include <MessageComposer/StatusBarLabelToggledState>
#include <MessageComposer/TextPart>
#include <MessageComposer/Util>

#include <Sonnet/DictionaryComboBox>

#include <MessageCore/AttachmentPart>
#include <MessageCore/AutocryptStorage>
#include <MessageCore/MessageCoreSettings>
#include <MessageCore/StringUtil>

#include <MessageViewer/MessageViewerSettings>
#include <MessageViewer/Stl_Util>

#include <MimeTreeParser/NodeHelper>
#include <MimeTreeParser/ObjectTreeParser>
#include <MimeTreeParser/SimpleObjectTreeSource>

#include <PimCommon/CustomToolsPluginManager>
#include <PimCommon/CustomToolsWidgetng>
#include <PimCommon/KActionMenuChangeCase>
#include <PimCommon/LineEditWithAutoCorrection>
#include <PimCommon/PurposeMenuMessageWidget>

#include <TemplateParser/TemplateParserJob>
#include <TemplateParser/TemplatesConfiguration>

#include <QGpgME/Protocol>

// KDE Frameworks includes
#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KEditToolBar>
#include <KEmailAddress>
#include <KEncodingFileDialog>
#include <KFileWidget>
#include <KIO/JobUiDelegate>
#include <KIconUtils>
#include <KMessageBox>
#include <KRecentDirs>
#include <KRecentFilesAction>
#include <KShortcutsDialog>
#include <KSplitterCollapserButton>
#include <KStandardShortcut>
#include <KToggleAction>
#include <KToolBar>
#include <KXMLGUIFactory>

#include <QDBusConnection>
// Qt includes
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPointer>
#include <QShortcut>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QUrlQuery>

// GPGME
#include <gpgme++/key.h>
#include <gpgme++/keylistresult.h>
#include <gpgme++/tofuinfo.h>

#include <KDialogJobUiDelegate>
#include <KIO/CommandLauncherJob>
#include <chrono>

using namespace std::chrono_literals;

using MailTransport::Transport;
using MailTransport::TransportManager;
using Sonnet::DictionaryComboBox;
using namespace Qt::Literals::StringLiterals;
Q_DECLARE_METATYPE(MessageComposer::Recipient::Ptr)

KMail::Composer *KMail::makeComposer(const std::shared_ptr<KMime::Message> &msg,
                                     bool lastSignState,
                                     bool lastEncryptState,
                                     Composer::TemplateContext context,
                                     uint identity,
                                     const QString &textSelection,
                                     const QString &customTemplate)
{
    return KMComposerWin::create(msg, lastSignState, lastEncryptState, context, identity, textSelection, customTemplate);
}

KMail::Composer *KMComposerWin::create(const std::shared_ptr<KMime::Message> &msg,
                                       bool lastSignState,
                                       bool lastEncryptState,
                                       Composer::TemplateContext context,
                                       uint identity,
                                       const QString &textSelection,
                                       const QString &customTemplate)
{
    return new KMComposerWin(msg, lastSignState, lastEncryptState, context, identity, textSelection, customTemplate);
}

int KMComposerWin::s_composerNumber = 0;

KMComposerWin::KMComposerWin(const std::shared_ptr<KMime::Message> &aMsg,
                             bool lastSignState,
                             bool lastEncryptState,
                             Composer::TemplateContext context,
                             uint id,
                             const QString &textSelection,
                             const QString &customTemplate)
    : KMail::Composer(QStringLiteral("kmail-composer#"))
    , mTextSelection(textSelection)
    , mCustomTemplate(customTemplate)
    , mFolder(Akonadi::Collection(-1))
    , mId(id)
    , mContext(context)
    , mAttachmentMissing(new AttachmentMissingWarning(this))
    , mNearExpiryWarning(new NearExpiryWarning(this))
    , mCryptoStateIndicatorWidget(new CryptoStateIndicatorWidget(this))
    , mIncorrectIdentityFolderWarning(new IncorrectIdentityFolderWarning(this))
    , mPluginEditorManagerInterface(new KMailPluginEditorManagerInterface(this))
    , mPluginEditorGrammarManagerInterface(new KMailPluginGrammarEditorManagerInterface(this))
    , mPluginEditorMessageWidget(new PimCommon::PurposeMenuMessageWidget(this))
    , mKeyCache(Kleo::KeyCache::mutableInstance())
{
    mGlobalAction = new KMComposerGlobalAction(this, this);
    mComposerBase = new MessageComposer::ComposerViewBase(this, this);
    mComposerBase->setIdentityManager(kmkernel->identityManager());

    connect(mPluginEditorManagerInterface, &KMailPluginEditorManagerInterface::message, this, &KMComposerWin::slotMessage);
    connect(mPluginEditorManagerInterface, &KMailPluginEditorManagerInterface::insertText, this, &KMComposerWin::slotEditorPluginInsertText);
    mPluginEditorCheckBeforeSendManagerInterface = new KMailPluginEditorCheckBeforeSendManagerInterface(this);
    mPluginEditorInitManagerInterface = new KMailPluginEditorInitManagerInterface(this);
    mPluginEditorConvertTextManagerInterface = new KMailPluginEditorConvertTextManagerInterface(this);

    connect(mPluginEditorManagerInterface,
            &KMailPluginEditorManagerInterface::errorMessage,
            mPluginEditorMessageWidget,
            &PimCommon::PurposeMenuMessageWidget::slotShareError);
    connect(mPluginEditorManagerInterface,
            &KMailPluginEditorManagerInterface::successMessage,
            mPluginEditorMessageWidget,
            &PimCommon::PurposeMenuMessageWidget::slotShareSuccess);

    connect(mComposerBase, &MessageComposer::ComposerViewBase::disableHtml, this, &KMComposerWin::disableHtml);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::enableHtml, this, &KMComposerWin::enableHtml);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::failed, this, &KMComposerWin::slotSendFailed);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::sentSuccessfully, this, &KMComposerWin::slotSendSuccessful);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::modified, this, &KMComposerWin::setModified);
    connect(mComposerBase, &MessageComposer::ComposerViewBase::tooManyRecipient, this, &KMComposerWin::slotTooManyRecipients);

    (void)new MailcomposerAdaptor(this);

    connect(mComposerBase->expiryChecker().get(),
            &Kleo::ExpiryChecker::expiryMessage,
            this,
            [&](const GpgME::Key &key, QString msg, Kleo::ExpiryChecker::ExpiryInformation info, [[maybe_unused]] bool isNewMessage) {
                if (info == Kleo::ExpiryChecker::OwnKeyExpired || info == Kleo::ExpiryChecker::OwnKeyNearExpiry) {
                    const auto plainMsg = msg.replace("<p>"_L1, " "_L1).replace("</p>"_L1, " "_L1).replace("<p align=center>"_L1, " "_L1);
                    mNearExpiryWarning->addInfo(plainMsg);
                    mNearExpiryWarning->setWarning(info == Kleo::ExpiryChecker::OwnKeyExpired);
                    mNearExpiryWarning->animatedShow();
                }
                const QList<KPIM::MultiplyingLine *> lstLines = mComposerBase->recipientsEditor()->lines();
                for (KPIM::MultiplyingLine *line : lstLines) {
                    auto recipient = line->data().dynamicCast<MessageComposer::Recipient>();
                    if (recipient->key().primaryFingerprint() == key.primaryFingerprint()) {
                        auto recipientLine = qobject_cast<MessageComposer::RecipientLineNG *>(line);
                        QString iconname = QStringLiteral("emblem-warning");
                        if (info == Kleo::ExpiryChecker::OtherKeyExpired) {
                            mEncryptionState.setAcceptedSolution(false);
                            iconname = QStringLiteral("emblem-error");
                            const auto showCryptoIndicator = KMailSettings::self()->showCryptoLabelIndicator();
                            const auto hasOverride = mEncryptionState.hasOverride();
                            const auto encrypt = mEncryptionState.encrypt();

                            const bool showAllIcons = showCryptoIndicator && hasOverride && encrypt;
                            if (!showAllIcons) {
                                recipientLine->setIcon(QIcon(), msg);
                                return;
                            }
                        }

                        recipientLine->setIcon(QIcon::fromTheme(iconname), msg);
                        return;
                    }
                }
            });

    mdbusObjectPath = "/Composer_"_L1 + QString::number(++s_composerNumber);
    QDBusConnection::sessionBus().registerObject(mdbusObjectPath, this);

    auto sigController = new MessageComposer::SignatureController(this);
    connect(sigController, &MessageComposer::SignatureController::enableHtml, this, &KMComposerWin::enableHtml);
    mComposerBase->setSignatureController(sigController);

    if (!kmkernel->xmlGuiInstanceName().isEmpty()) {
        setComponentName(kmkernel->xmlGuiInstanceName(), i18n("KMail2"));
    }
    mMainWidget = new QWidget(this);
    // splitter between the headers area and the actual editor
    mHeadersToEditorSplitter = new QSplitter(Qt::Vertical, mMainWidget);
    mHeadersToEditorSplitter->setObjectName("mHeadersToEditorSplitter"_L1);
    mHeadersToEditorSplitter->setChildrenCollapsible(false);
    mHeadersArea = new QWidget(mHeadersToEditorSplitter);
    mHeadersArea->setSizePolicy(mHeadersToEditorSplitter->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    mHeadersToEditorSplitter->addWidget(mHeadersArea);
    const QList<int> defaultSizes{0};
    mHeadersToEditorSplitter->setSizes(defaultSizes);

    mMainlayoutMainWidget = new QVBoxLayout(mMainWidget);
    mMainlayoutMainWidget->setContentsMargins({});
    mMainlayoutMainWidget->addWidget(mHeadersToEditorSplitter);
    auto identity = new KIdentityManagementWidgets::IdentityCombo(kmkernel->identityManager(), mHeadersArea);
    identity->setCurrentIdentity(mId);
    identity->setObjectName("identitycombo"_L1);
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    identity->setIdentityActivitiesAbstract(ActivitiesManager::self()->identityActivities());
    identity->setEnablePlasmaActivities(KMailSettings::self()->plasmaActivitySupport());
#endif
    connect(identity, &KIdentityManagementWidgets::IdentityCombo::identityDeleted, this, &KMComposerWin::slotIdentityDeleted);
    connect(identity, &KIdentityManagementWidgets::IdentityCombo::invalidIdentity, this, &KMComposerWin::slotInvalidIdentity);
    mComposerBase->setIdentityCombo(identity);

    sigController->setIdentityCombo(identity);
    sigController->suspend(); // we have to do identity change tracking ourselves due to the template code

    auto dictionaryCombo = new DictionaryComboBox(mHeadersArea);
    dictionaryCombo->setToolTip(i18nc("@info:tooltip", "Select the dictionary to use when spell-checking this message"));
    mComposerBase->setDictionary(dictionaryCombo);

    mFccFolder = new MailCommon::FolderRequester(mHeadersArea);
    mFccFolder->setNotAllowToCreateNewFolder(true);
    mFccFolder->setMustBeReadWrite(true);

    mFccFolder->setToolTip(i18nc("@info:tooltip", "Select the sent-mail folder where a copy of this message will be saved"));
    connect(mFccFolder, &MailCommon::FolderRequester::folderChanged, this, &KMComposerWin::slotFccFolderChanged);
    connect(mFccFolder, &MailCommon::FolderRequester::invalidFolder, this, &KMComposerWin::slotFccIsInvalid);

    auto transport = new MailTransport::TransportComboBox(mHeadersArea);
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    transport->setTransportActivitiesAbstract(ActivitiesManager::self()->transportActivities());
    transport->setEnablePlasmaActivities(KMailSettings::self()->plasmaActivitySupport());
#endif
    transport->setToolTip(i18nc("@info:tooltip", "Select the outgoing account to use for sending this message"));
    mComposerBase->setTransportCombo(transport);
    connect(transport, &MailTransport::TransportComboBox::activated, this, &KMComposerWin::slotTransportChanged);
    connect(transport, &MailTransport::TransportComboBox::transportRemoved, this, &KMComposerWin::slotTransportRemoved);
    mEdtFrom = new MessageComposer::ComposerLineEdit(false, mHeadersArea);
    // TODO add ldapactivities
    mEdtFrom->installEventFilter(this);
    mEdtFrom->setObjectName("fromLine"_L1);
    mEdtFrom->setRecentAddressConfig(MessageComposer::MessageComposerSettings::self()->config());
    mEdtFrom->setToolTip(i18nc("@info:tooltip", "Set the \"From:\" email address for this message"));

    // TODO add ldapactivities
    auto recipientsEditor = new MessageComposer::RecipientsEditor(mHeadersArea);
    recipientsEditor->setRecentAddressConfig(MessageComposer::MessageComposerSettings::self()->config());
    connect(recipientsEditor, &MessageComposer::RecipientsEditor::completionModeChanged, this, &KMComposerWin::slotCompletionModeChanged);
    connect(recipientsEditor, &MessageComposer::RecipientsEditor::sizeHintChanged, this, &KMComposerWin::recipientEditorSizeHintChanged);
    connect(recipientsEditor, &MessageComposer::RecipientsEditor::lineAdded, this, &KMComposerWin::slotRecipientEditorLineAdded);
    connect(recipientsEditor, &MessageComposer::RecipientsEditor::focusInRecipientLineEdit, this, &KMComposerWin::slotRecipientEditorLineFocused);
    mComposerBase->setRecipientsEditor(recipientsEditor);

    mEdtSubject = new SubjectLineEditWithAutoCorrection(mHeadersArea, QStringLiteral("kmail2rc"));
    mEdtSubject->installEventFilter(this);
    mEdtSubject->setAutocorrection(KMKernel::self()->composerAutoCorrection());
    connect(mEdtSubject, &SubjectLineEditWithAutoCorrection::handleMimeData, this, [this](const QMimeData *mimeData) {
        insertFromMimeData(mimeData, false);
    });

    connect(&mEncryptionState, &EncryptionState::encryptChanged, this, &KMComposerWin::slotEncryptionButtonIconUpdate);
    connect(&mEncryptionState, &EncryptionState::encryptChanged, this, &KMComposerWin::updateSignatureAndEncryptionStateIndicators);
    connect(&mEncryptionState, &EncryptionState::overrideChanged, this, &KMComposerWin::slotEncryptionButtonIconUpdate);
    connect(&mEncryptionState, &EncryptionState::overrideChanged, this, &KMComposerWin::runKeyResolver);
    connect(&mEncryptionState, &EncryptionState::acceptedSolutionChanged, this, &KMComposerWin::slotEncryptionButtonIconUpdate);

    mRunKeyResolverTimer = new QTimer(this);
    mRunKeyResolverTimer->setSingleShot(true);
    mRunKeyResolverTimer->setInterval(500ms);
    connect(mRunKeyResolverTimer, &QTimer::timeout, this, &KMComposerWin::runKeyResolver);

    mLblIdentity = new QLabel(i18nc("@label:textbox", "&Identity:"), mHeadersArea);
    mDictionaryLabel = new QLabel(i18nc("@label:textbox", "&Dictionary:"), mHeadersArea);
    mLblFcc = new QLabel(i18nc("@label:textbox", "&Sent-Mail folder:"), mHeadersArea);
    mLblTransport = new QLabel(i18nc("@label:textbox", "&Mail transport:"), mHeadersArea);
    mLblFrom = new QLabel(i18nc("sender address field", "&From:"), mHeadersArea);
    mLblSubject = new QLabel(i18nc("@label:textbox Subject of email.", "S&ubject:"), mHeadersArea);
    mShowHeaders = KMailSettings::self()->headers();
    mDone = false;
    // the attachment view is separated from the editor by a splitter
    mSplitter = new QSplitter(Qt::Vertical, mMainWidget);
    mSplitter->setObjectName("mSplitter"_L1);
    mSplitter->setChildrenCollapsible(false);
    mSnippetSplitter = new QSplitter(Qt::Horizontal, mSplitter);
    mSnippetSplitter->setObjectName("mSnippetSplitter"_L1);
    mSplitter->addWidget(mSnippetSplitter);

    auto editorAndCryptoStateIndicators = new QWidget(mSplitter);

    mEditorAndCryptoStateIndicatorsLayout = new QVBoxLayout(editorAndCryptoStateIndicators);
    mEditorAndCryptoStateIndicatorsLayout->setSpacing(0);
    mEditorAndCryptoStateIndicatorsLayout->setContentsMargins({});

    connect(mAttachmentMissing, &AttachmentMissingWarning::attachMissingFile, this, &KMComposerWin::slotAttachMissingFile);
    connect(mAttachmentMissing, &AttachmentMissingWarning::explicitClosedMissingAttachment, this, &KMComposerWin::slotExplicitClosedMissingAttachment);
    mEditorAndCryptoStateIndicatorsLayout->addWidget(mAttachmentMissing);

    auto composerEditorNg = new KMComposerEditorNg(this, mCryptoStateIndicatorWidget);
    composerEditorNg->setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::TopEdge}));
    mRichTextEditorWidget = new TextCustomEditor::RichTextEditorWidget(composerEditorNg, mCryptoStateIndicatorWidget);
    composerEditorNg->installEventFilter(this);

    connect(composerEditorNg, &KMComposerEditorNg::insertEmoticon, mGlobalAction, &KMComposerGlobalAction::slotInsertEmoticon);
    // Don't use new connect api here. It crashes
    connect(composerEditorNg, SIGNAL(textChanged()), this, SLOT(slotEditorTextChanged()));
    connect(composerEditorNg, &KMComposerEditorNg::selectionChanged, this, &KMComposerWin::slotSelectionChanged);
    // connect(editor, &KMComposerEditor::textChanged, this, &KMComposeWin::slotEditorTextChanged);
    mComposerBase->setEditor(composerEditorNg);

    mEditorAndCryptoStateIndicatorsLayout->addWidget(mIncorrectIdentityFolderWarning);

    mEditorAndCryptoStateIndicatorsLayout->addWidget(mPluginEditorMessageWidget);
    mEditorAndCryptoStateIndicatorsLayout->addWidget(mNearExpiryWarning);

    mEditorAndCryptoStateIndicatorsLayout->addWidget(mCryptoStateIndicatorWidget);
    mEditorAndCryptoStateIndicatorsLayout->addWidget(mRichTextEditorWidget);

    mSnippetSplitter->insertWidget(0, editorAndCryptoStateIndicators);
    mSnippetSplitter->setOpaqueResize(true);
    sigController->setEditor(composerEditorNg);

    mHeadersToEditorSplitter->addWidget(mSplitter);
    composerEditorNg->setAcceptDrops(true);
    connect(sigController,
            &MessageComposer::SignatureController::signatureAdded,
            mComposerBase->editor()->externalComposer(),
            &KPIMTextEdit::RichTextExternalComposer::startExternalEditor);

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

    auto attachmentModel = new MessageComposer::AttachmentModel(this);
    auto attachmentView = new KMail::AttachmentView(attachmentModel, mSplitter);
    attachmentView->hideIfEmpty();
    connect(attachmentView, &KMail::AttachmentView::modified, this, &KMComposerWin::setModified);
    auto attachmentController = new KMail::AttachmentController(attachmentModel, attachmentView, this);

    mComposerBase->setAttachmentModel(attachmentModel);
    mComposerBase->setAttachmentController(attachmentController);

    if (KMailSettings::self()->showForgottenAttachmentWarning()) {
        mVerifyMissingAttachment = new QTimer(this);
        mVerifyMissingAttachment->setSingleShot(true);
        mVerifyMissingAttachment->setInterval(5s);
        connect(mVerifyMissingAttachment, &QTimer::timeout, this, &KMComposerWin::slotVerifyMissingAttachmentTimeout);
    }
    connect(attachmentController, &KMail::AttachmentController::fileAttached, mAttachmentMissing, &AttachmentMissingWarning::slotFileAttached);

    mPluginEditorManagerInterface->setParentWidget(this);
    mPluginEditorManagerInterface->setRichTextEditor(mRichTextEditorWidget->editor());
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

    mUpdateWindowTitleConnection = connect(mEdtSubject, &PimCommon::LineEditWithAutoCorrection::textChanged, this, &KMComposerWin::slotUpdateWindowTitle);
    mIdentityConnection = connect(identity, &KIdentityManagementWidgets::IdentityCombo::identityChanged, this, [this](uint val) {
        slotIdentityChanged(val);
    });
    connect(kmkernel->identityManager(), qOverload<uint>(&KIdentityManagementCore::IdentityManager::changed), this, [this](uint val) {
        if (currentIdentity() == val) {
            slotIdentityChanged(val);
        }
    });

    connect(mEdtFrom, &MessageComposer::ComposerLineEdit::completionModeChanged, this, &KMComposerWin::slotCompletionModeChanged);
    connect(kmkernel->folderCollectionMonitor(), &Akonadi::Monitor::collectionRemoved, this, &KMComposerWin::slotFolderRemoved);
    connect(kmkernel, &KMKernel::configChanged, this, [this]() {
        slotConfigChanged();
        runKeyResolver();
    });

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

    mDummyComposer = new MessageComposer::ComposerJob(this);
    mDummyComposer->globalPart()->setParentWidgetForGui(this);

    setStateConfigGroup(QStringLiteral("Composer"));
    setAutoSaveSettings(stateConfigGroup(), true);
}

KMComposerWin::~KMComposerWin()
{
    disconnect(mUpdateWindowTitleConnection);
    // When we have a collection set, store the message back to that collection.
    // Note that when we save the message or sent it, mFolder is set back to 0.
    // So this for example kicks in when opening a draft and then closing the window.
    if (mFolder.isValid() && mMsg && isModified()) {
        auto saveDraftJob = new SaveDraftJob(mMsg, mFolder);
        saveDraftJob->start();
    }

    delete mComposerBase;
}

void KMComposerWin::createAttachmentFromExternalMissing()
{
    if (!mAttachmentFromExternalMissing) {
        mAttachmentFromExternalMissing = new AttachmentAddedFromExternalWarning(this);
        mEditorAndCryptoStateIndicatorsLayout->insertWidget(0, mAttachmentFromExternalMissing);
    }
}

void KMComposerWin::createExternalEditorWarning()
{
    if (!mExternalEditorWarning) {
        mExternalEditorWarning = new ExternalEditorWarning(this);
        mMainlayoutMainWidget->addWidget(mExternalEditorWarning);
    }
}

void KMComposerWin::createTooMyRecipientWarning()
{
    if (!mTooMyRecipientWarning) {
        mTooMyRecipientWarning = new TooManyRecipientsWarning(this);
        mEditorAndCryptoStateIndicatorsLayout->insertWidget(0, mTooMyRecipientWarning);
    }
}

void KMComposerWin::slotTooManyRecipients(bool b)
{
    if (b) {
        createTooMyRecipientWarning();
        mTooMyRecipientWarning->animatedShow();
    } else {
        createTooMyRecipientWarning();
        mTooMyRecipientWarning->animatedHide();
    }
}

void KMComposerWin::slotRecipientEditorLineFocused()
{
    mPluginEditorManagerInterface->setStatusBarWidgetEnabled(MessageComposer::PluginEditorInterface::ApplyOnFieldType::EmailFields);
}

KMComposerWin::ModeType KMComposerWin::modeType() const
{
    return mModeType;
}

void KMComposerWin::setModeType(KMComposerWin::ModeType modeType)
{
    mModeType = modeType;
}

bool KMComposerWin::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        if (obj == mEdtSubject) {
            mPluginEditorManagerInterface->setStatusBarWidgetEnabled(MessageComposer::PluginEditorInterface::ApplyOnFieldType::SubjectField);
        } else if (obj == mComposerBase->recipientsEditor()) {
            mPluginEditorManagerInterface->setStatusBarWidgetEnabled(MessageComposer::PluginEditorInterface::ApplyOnFieldType::EmailFields);
        } else if (obj == mEdtFrom) {
            mPluginEditorManagerInterface->setStatusBarWidgetEnabled(MessageComposer::PluginEditorInterface::ApplyOnFieldType::EmailFields);
        }
    }
    return KMail::Composer::eventFilter(obj, event);
}

void KMComposerWin::insertSnippetInfo(const MailCommon::SnippetInfo &info)
{
    {
        if (!info.to.isEmpty()) {
            const QStringList lst = KEmailAddress::splitAddressList(info.to);
            for (const QString &addr : lst) {
                if (!mComposerBase->recipientsEditor()->addRecipient(addr, MessageComposer::Recipient::To)) {
                    qCWarning(KMAIL_LOG) << "Impossible to add to entry";
                }
            }
        }
    }
    {
        if (!info.cc.isEmpty()) {
            const QStringList lst = KEmailAddress::splitAddressList(info.cc);
            for (const QString &addr : lst) {
                if (!mComposerBase->recipientsEditor()->addRecipient(addr, MessageComposer::Recipient::Cc)) {
                    qCWarning(KMAIL_LOG) << "Impossible to add cc entry";
                }
            }
        }
    }
    {
        if (!info.bcc.isEmpty()) {
            const QStringList lst = KEmailAddress::splitAddressList(info.bcc);
            for (const QString &addr : lst) {
                if (!mComposerBase->recipientsEditor()->addRecipient(addr, MessageComposer::Recipient::Bcc)) {
                    qCWarning(KMAIL_LOG) << "Impossible to add bcc entry";
                }
            }
        }
    }
    {
        if (!info.attachment.isEmpty()) {
            const QStringList lst = info.attachment.split(QLatin1Char(','));
            for (const QString &attach : lst) {
                auto job = new MessageComposer::ConvertSnippetVariablesJob(this);
                job->setText(attach);
                auto interface = new MessageComposer::ComposerViewInterface(mComposerBase);
                job->setComposerViewInterface(interface);
                connect(job, &MessageComposer::ConvertSnippetVariablesJob::textConverted, this, [this](const QString &str) {
                    if (!str.isEmpty()) {
                        const QUrl localUrl = QUrl::fromLocalFile(str);
                        AttachmentInfo info;
                        info.url = localUrl;
                        addAttachment(QList<AttachmentInfo>() << info, false);
                    }
                });
                job->start();
            }
        }
    }
    {
        if (!info.subject.isEmpty()) {
            // Convert subject
            auto job = new MessageComposer::ConvertSnippetVariablesJob(this);
            job->setText(info.subject);
            auto interface = new MessageComposer::ComposerViewInterface(mComposerBase);
            job->setComposerViewInterface(interface);
            connect(job, &MessageComposer::ConvertSnippetVariablesJob::textConverted, this, [this](const QString &str) {
                if (!str.isEmpty()) {
                    if (mComposerBase->subject().isEmpty()) { // Add subject only if we don't have subject
                        mEdtSubject->setPlainText(str);
                    }
                }
            });
            job->start();
        }
    }
    {
        if (!info.text.isEmpty()) {
            // Convert plain text
            auto job = new MessageComposer::ConvertSnippetVariablesJob(this);
            job->setText(info.text);
            auto interface = new MessageComposer::ComposerViewInterface(mComposerBase);
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

void KMComposerWin::addAttachment(const QList<KMail::Composer::AttachmentInfo> &infos, bool showWarning)
{
    QStringList lst;
    for (const AttachmentInfo &info : infos) {
        if (showWarning) {
            lst.append(info.url.toDisplayString());
        }
        mComposerBase->addAttachment(info.url, info.comment, false);
    }
    if (showWarning) {
        createAttachmentFromExternalMissing();
        mAttachmentFromExternalMissing->setAttachmentNames(lst);
        mAttachmentFromExternalMissing->animatedShow();
    }
}

void KMComposerWin::addAttachment(const QString &name,
                                  [[maybe_unused]] KMime::Headers::contentEncoding cte,
                                  const QString &charset,
                                  const QByteArray &data,
                                  const QByteArray &mimeType)
{
    mComposerBase->addAttachment(name, name, charset, data, mimeType);
}

void KMComposerWin::readConfig(bool reload)
{
    mEdtFrom->setCompletionMode(static_cast<KCompletion::CompletionMode>(KMailSettings::self()->completionMode()));
    mComposerBase->recipientsEditor()->setCompletionMode(static_cast<KCompletion::CompletionMode>(KMailSettings::self()->completionMode()));
    mCryptoStateIndicatorWidget->setShowAlwaysIndicator(KMailSettings::self()->showCryptoLabelIndicator());

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
        QSize composerSize = KMailSettings::self()->composerSize();
        if (composerSize.width() < 200) {
            composerSize.setWidth(200);
        }
        if (composerSize.height() < 200) {
            composerSize.setHeight(200);
        }
        resize(composerSize);

        if (!KMailSettings::self()->snippetSplitterPosition().isEmpty()) {
            mSnippetSplitter->setSizes(KMailSettings::self()->snippetSplitterPosition());
        } else {
            const QList<int> defaults{(int)(width() * 0.8), (int)(width() * 0.2)};
            mSnippetSplitter->setSizes(defaults);
        }
    }

    mComposerBase->identityCombo()->setCurrentIdentity(mId);
    qCDebug(KMAIL_LOG) << mComposerBase->identityCombo()->currentIdentityName();
    const KIdentityManagementCore::Identity &ident = kmkernel->identityManager()->identityForUoid(mId);

    mComposerBase->setAutoSaveInterval(KMailSettings::self()->autosaveInterval() * 1000 * 60);

    mComposerBase->dictionary()->setCurrentByDictionaryName(ident.dictionary());

    const QString fccName = ident.fcc();
    setFcc(fccName);
}

void KMComposerWin::writeConfig()
{
    KMailSettings::self()->setHeaders(mShowHeaders);
    KMailSettings::self()->setCurrentTransport(mComposerBase->transportComboBox()->currentText());
    KMailSettings::self()->setPreviousIdentity(currentIdentity());
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

MessageComposer::ComposerJob *KMComposerWin::createSimpleComposer()
{
    mComposerBase->setFrom(from());
    mComposerBase->setSubject(subject());
    auto composer = new MessageComposer::ComposerJob();
    mComposerBase->fillComposer(composer);
    return composer;
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

    // This sucks awfully, but no, I cannot get an activated(int id) from
    // actionContainer()
    auto act = ::qobject_cast<KToggleAction *>(sender());
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

    w->setBuddy(mComposerBase->editor()); // set dummy so we don't calculate width of '&' for this label.
    w->adjustSize();
    w->show();
    return qMax(width, w->sizeHint().width());
}

void KMComposerWin::rethinkFields(bool fromSlot, bool forceAllHeaders)
{
    // This sucks even more but again no ids. sorry (sven)
    int mask;
    int row;
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
    if (std::abs(showHeaders) & HDR_IDENTITY) {
        mLabelWidth = calcColumnWidth(HDR_IDENTITY, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders) & HDR_DICTIONARY) {
        mLabelWidth = calcColumnWidth(HDR_DICTIONARY, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders) & HDR_FCC) {
        mLabelWidth = calcColumnWidth(HDR_FCC, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders) & HDR_TRANSPORT) {
        mLabelWidth = calcColumnWidth(HDR_TRANSPORT, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders) & HDR_FROM) {
        mLabelWidth = calcColumnWidth(HDR_FROM, showHeaders, mLabelWidth);
    }
    if (std::abs(showHeaders) & HDR_SUBJECT) {
        mLabelWidth = calcColumnWidth(HDR_SUBJECT, showHeaders, mLabelWidth);
    }

    if (!fromSlot) {
        mAllFieldsAction->setChecked(showHeaders == HDR_ALL);
        mIdentityAction->setChecked(std::abs(mShowHeaders) & HDR_IDENTITY);
    }
    rethinkHeaderLine(showHeaders, HDR_IDENTITY, row, mLblIdentity, mComposerBase->identityCombo());

    if (!fromSlot) {
        mDictionaryAction->setChecked(std::abs(mShowHeaders) & HDR_DICTIONARY);
    }
    rethinkHeaderLine(showHeaders, HDR_DICTIONARY, row, mDictionaryLabel, mComposerBase->dictionary());

    if (!fromSlot) {
        mFccAction->setChecked(std::abs(mShowHeaders) & HDR_FCC);
    }
    rethinkHeaderLine(showHeaders, HDR_FCC, row, mLblFcc, mFccFolder);

    if (!fromSlot) {
        mTransportAction->setChecked(std::abs(mShowHeaders) & HDR_TRANSPORT);
    }
    rethinkHeaderLine(showHeaders, HDR_TRANSPORT, row, mLblTransport, mComposerBase->transportComboBox());

    if (!fromSlot) {
        mFromAction->setChecked(std::abs(mShowHeaders) & HDR_FROM);
    }
    rethinkHeaderLine(showHeaders, HDR_FROM, row, mLblFrom, mEdtFrom);

    mGrid->addWidget(mComposerBase->recipientsEditor(), row, 0, 1, 2);
    ++row;
    connect(mEdtFrom, &MessageComposer::ComposerLineEdit::focusDown, mComposerBase->recipientsEditor(), &KPIM::MultiplyingLineEditor::setFocusTop);
    connect(mComposerBase->recipientsEditor(), &KPIM::MultiplyingLineEditor::focusUp, mEdtFrom, qOverload<>(&QWidget::setFocus));

    connect(mComposerBase->recipientsEditor(), &KPIM::MultiplyingLineEditor::focusDown, mEdtSubject, qOverload<>(&QWidget::setFocus));
    connect(mEdtSubject, &PimCommon::SpellCheckLineEdit::focusUp, mComposerBase->recipientsEditor(), &KPIM::MultiplyingLineEditor::setFocusBottom);

    if (!fromSlot) {
        mSubjectAction->setChecked(std::abs(mShowHeaders) & HDR_SUBJECT);
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

void KMComposerWin::slotUpdateComposer(const KIdentityManagementCore::Identity &ident,
                                       const std::shared_ptr<KMime::Message> &msg,
                                       uint uoid,
                                       uint uoldId,
                                       bool wasModified)
{
    mComposerBase->updateTemplate(msg);
    updateSignature(uoid, uoldId);
    updateComposerAfterIdentityChanged(ident, uoid, wasModified);
}

void KMComposerWin::applyTemplate(uint uoid, uint uOldId, const KIdentityManagementCore::Identity &ident, bool wasModified)
{
    TemplateParser::TemplateParserJob::Mode mode;
    switch (mContext) {
    case TemplateContext::New:
        mode = TemplateParser::TemplateParserJob::NewMessage;
        break;
    case TemplateContext::Reply:
        mode = TemplateParser::TemplateParserJob::Reply;
        break;
    case TemplateContext::ReplyToAll:
        mode = TemplateParser::TemplateParserJob::ReplyAll;
        break;
    case TemplateContext::Forward:
        mode = TemplateParser::TemplateParserJob::Forward;
        break;
    case TemplateContext::NoTemplate:
        updateComposerAfterIdentityChanged(ident, uoid, wasModified);
        return;
    }

    auto header = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("X-KMail-Templates"));
    header->fromUnicodeString(ident.templates());
    mMsg->setHeader(std::move(header));

    if (mode == TemplateParser::TemplateParserJob::NewMessage) {
        auto job = new KMComposerUpdateTemplateJob;
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

            auto job = new Akonadi::ItemFetchJob(items, this);
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
    // const Akonadi::Item::List items = fetchJob->items();

    // Re-add ? const TemplateParser::TemplateParserJob::Mode mode = static_cast<TemplateParser::TemplateParserJob::Mode>(fetchJob->property("mode").toInt());
    const uint uoid = fetchJob->property("uoid").toUInt();
    const uint uOldId = fetchJob->property("uOldid").toUInt();
    mComposerBase->updateTemplate(mMsg);
    updateSignature(uoid, uOldId);
    qCWarning(KMAIL_LOG) << " void KMComposerWin::slotDelayedApplyTemplate(KJob *job) is not implemented after removing qtwebkit";
}

void KMComposerWin::updateSignature(uint uoid, uint uOldId)
{
    const KIdentityManagementCore::Identity &ident = kmkernel->identityManager()->identityForUoid(uoid);
    const KIdentityManagementCore::Identity &oldIdentity = kmkernel->identityManager()->identityForUoid(uOldId);
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
            const KIdentityManagementCore::Identity &identity = kmkernel->identityManager()->identityForUoidOrDefault(uoid);
            // Get quote prefix from template
            // ( custom templates don't specify custom quotes prefixes )
            TemplateParser::Templates quoteTemplate(TemplateParser::TemplatesConfiguration::configIdString(identity.uoid()));
            quotePrefix = quoteTemplate.quoteString();
        }
    }
    QString fromStr;
    if (auto h = mMsg->from(KMime::CreatePolicy::DontCreate)) {
        fromStr = h->displayString();
    }
    mComposerBase->editor()->setQuotePrefixName(MessageCore::StringUtil::formatQuotePrefix(quotePrefix, fromStr));
}

void KMComposerWin::setupActions()
{
    KActionMenuTransport *actActionNowMenu = nullptr;
    KActionMenuTransport *actActionLaterMenu = nullptr;

    if (MessageComposer::MessageComposerSettings::self()->sendImmediate()) {
        // default = send now, alternative = queue
        auto action = new QAction(QIcon::fromTheme(QStringLiteral("mail-send")), i18n("&Send Mail"), this);
        actionCollection()->addAction(QStringLiteral("send_default"), action);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_Return));
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
        // default = queue, alternative = send now
        auto action = new QAction(QIcon::fromTheme(QStringLiteral("mail-queue")), i18n("Send &Later"), this);
        actionCollection()->addAction(QStringLiteral("send_default"), action);
        connect(action, &QAction::triggered, this, &KMComposerWin::slotSendLater);
        actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_Return));

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

    connect(actActionNowMenu, &QAction::triggered, this, &KMComposerWin::slotSendNow);
    connect(actActionLaterMenu, &QAction::triggered, this, &KMComposerWin::slotSendLater);
    connect(actActionNowMenu, &KActionMenuTransport::transportSelected, this, &KMComposerWin::slotSendNowVia);
    connect(actActionLaterMenu, &KActionMenuTransport::transportSelected, this, &KMComposerWin::slotSendLaterVia);

    auto action = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save as &Draft"), this);
    actionCollection()->addAction(QStringLiteral("save_in_drafts"), action);
    KMail::Util::addQActionHelpText(action, i18n("Save email in Draft folder"));
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(action, &QAction::triggered, this, &KMComposerWin::slotSaveDraft);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save as &Template"), this);
    KMail::Util::addQActionHelpText(action, i18n("Save email in Template folder"));
    actionCollection()->addAction(QStringLiteral("save_in_templates"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotSaveTemplate);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save as &File"), this);
    KMail::Util::addQActionHelpText(action, i18n("Save email as text or html file"));
    actionCollection()->addAction(QStringLiteral("save_as_file"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotSaveAsFile);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-open")), i18n("&Insert Text File…"), this);
    actionCollection()->addAction(QStringLiteral("insert_file"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotInsertFile);

    mRecentAction = new KRecentFilesAction(QIcon::fromTheme(QStringLiteral("document-open")), i18n("&Insert Recent Text File"), this);
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
    const QString path = TextAddonsWidgets::ExecutableUtils::findExecutable(QStringLiteral("kaddressbook"));
    if (path.isEmpty()) {
        action->setEnabled(false);
    } else {
        connect(action, &QAction::triggered, this, &KMComposerWin::slotAddressBook);
    }
    action = new QAction(QIcon::fromTheme(QStringLiteral("mail-message-new")), i18n("&New Composer"), this);
    actionCollection()->addAction(QStringLiteral("new_composer"), action);

    connect(action, &QAction::triggered, this, &KMComposerWin::slotNewComposer);
    actionCollection()->setDefaultShortcuts(action, KStandardShortcut::shortcut(KStandardShortcut::New));

    action = new QAction(i18nc("@action", "Select &Recipients…"), this);
    actionCollection()->addAction(QStringLiteral("select_recipients"), action);
    connect(action, &QAction::triggered, mComposerBase->recipientsEditor(), &MessageComposer::RecipientsEditor::selectRecipients);
    action = new QAction(i18nc("@action", "Save &Distribution List…"), this);
    actionCollection()->addAction(QStringLiteral("save_distribution_list"), action);
    connect(action, &QAction::triggered, mComposerBase->recipientsEditor(), &MessageComposer::RecipientsEditor::saveDistributionList);

    KStandardActions::print(this, &KMComposerWin::slotPrint, actionCollection());
    KStandardActions::printPreview(this, &KMComposerWin::slotPrintPreview, actionCollection());
    KStandardActions::close(this, &KMComposerWin::close, actionCollection());

    KStandardActions::undo(mGlobalAction, &KMComposerGlobalAction::slotUndo, actionCollection());
    KStandardActions::redo(mGlobalAction, &KMComposerGlobalAction::slotRedo, actionCollection());
    KStandardActions::cut(mGlobalAction, &KMComposerGlobalAction::slotCut, actionCollection());
    KStandardActions::copy(mGlobalAction, &KMComposerGlobalAction::slotCopy, actionCollection());
    KStandardActions::paste(mGlobalAction, &KMComposerGlobalAction::slotPaste, actionCollection());
    mSelectAll = KStandardActions::selectAll(mGlobalAction, &KMComposerGlobalAction::slotMarkAll, actionCollection());

    mFindText = KStandardActions::find(mRichTextEditorWidget, &TextCustomEditor::RichTextEditorWidget::slotFind, actionCollection());
    mFindNextText = KStandardActions::findNext(mRichTextEditorWidget, &TextCustomEditor::RichTextEditorWidget::slotFindNext, actionCollection());

    mReplaceText = KStandardActions::replace(mRichTextEditorWidget, &TextCustomEditor::RichTextEditorWidget::slotReplace, actionCollection());
    actionCollection()->addAction(KStandardActions::Spelling,
                                  QStringLiteral("spellcheck"),
                                  mComposerBase->editor(),
                                  &MessageComposer::RichTextComposerNg::slotCheckSpelling);

    action = new QAction(i18nc("@action", "Paste as Attac&hment"), this);
    actionCollection()->addAction(QStringLiteral("paste_att"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotPasteAsAttachment);

    action = new QAction(i18nc("@action", "Cl&ean Spaces"), this);
    actionCollection()->addAction(QStringLiteral("clean_spaces"), action);
    connect(action, &QAction::triggered, mComposerBase->signatureController(), &MessageComposer::SignatureController::cleanSpace);

    mFixedFontAction = new KToggleAction(i18nc("@action", "Use Fi&xed Font"), this);
    actionCollection()->addAction(QStringLiteral("toggle_fixedfont"), mFixedFontAction);
    connect(mFixedFontAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateFont);
    mFixedFontAction->setChecked(MessageViewer::MessageViewerSettings::self()->useFixedFont());

    // these are checkable!!!
    mUrgentAction = new KToggleAction(i18nc("@action:inmenu Mark the email as urgent.", "&Urgent"), this);
    actionCollection()->addAction(QStringLiteral("urgent"), mUrgentAction);
    mRequestMDNAction = new KToggleAction(i18nc("@action", "&Request Disposition Notification"), this);
    actionCollection()->addAction(QStringLiteral("options_request_mdn"), mRequestMDNAction);
    mRequestMDNAction->setChecked(KMailSettings::self()->requestMDN());

    mRequestDeliveryConfirmation = new KToggleAction(i18nc("@action", "&Request Delivery Confirmation"), this);
    actionCollection()->addAction(QStringLiteral("options_request_delivery_confirmation"), mRequestDeliveryConfirmation);
    // TODO mRequestDeliveryConfirmation->setChecked(KMailSettings::self()->requestMDN());

    //----- Message-Encoding Submenu
    mWordWrapAction = new KToggleAction(i18nc("@action", "&Wordwrap"), this);
    actionCollection()->addAction(QStringLiteral("wordwrap"), mWordWrapAction);
    mWordWrapAction->setChecked(MessageComposer::MessageComposerSettings::self()->wordWrap());
    connect(mWordWrapAction, &KToggleAction::toggled, this, &KMComposerWin::slotWordWrapToggled);

    mSnippetAction = new KToggleAction(i18nc("@action", "&Snippets"), this);
    actionCollection()->addAction(QStringLiteral("snippets"), mSnippetAction);
    connect(mSnippetAction, &KToggleAction::toggled, this, &KMComposerWin::slotSnippetWidgetVisibilityChanged);
    mSnippetAction->setChecked(KMailSettings::self()->showSnippetManager());

    mAutoSpellCheckingAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("tools-check-spelling")), i18nc("@action", "&Automatic Spellchecking"), this);
    actionCollection()->addAction(QStringLiteral("options_auto_spellchecking"), mAutoSpellCheckingAction);
    const bool spellChecking = KMailSettings::self()->autoSpellChecking();
    const bool useKmailEditor = !KMailSettings::self()->useExternalEditor();
    const bool spellCheckingEnabled = useKmailEditor && spellChecking;
    mAutoSpellCheckingAction->setEnabled(useKmailEditor);

    mAutoSpellCheckingAction->setChecked(spellCheckingEnabled);
    slotAutoSpellCheckingToggled(spellCheckingEnabled);
    connect(mAutoSpellCheckingAction, &KToggleAction::toggled, this, &KMComposerWin::slotAutoSpellCheckingToggled);
    connect(mComposerBase->editor(), &TextCustomEditor::RichTextEditor::checkSpellingChanged, this, &KMComposerWin::slotAutoSpellCheckingToggled);

    connect(mComposerBase->editor(), &MessageComposer::RichTextComposerNg::textModeChanged, this, &KMComposerWin::slotTextModeChanged);
    connect(mComposerBase->editor(), &MessageComposer::RichTextComposerNg::externalEditorClosed, this, &KMComposerWin::slotExternalEditorClosed);
    connect(mComposerBase->editor(), &MessageComposer::RichTextComposerNg::externalEditorStarted, this, &KMComposerWin::slotExternalEditorStarted);
    // these are checkable!!!
    mMarkupAction = new KToggleAction(i18nc("@action", "Rich Text Editing"), this);
    mMarkupAction->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-font")));
    mMarkupAction->setIconText(i18n("Rich Text"));
    mMarkupAction->setToolTip(i18nc("@info:tooltip", "Toggle rich text editing mode"));
    actionCollection()->addAction(QStringLiteral("html"), mMarkupAction);
    connect(mMarkupAction, &KToggleAction::triggered, this, &KMComposerWin::slotToggleMarkup);

    mAllFieldsAction = new KToggleAction(i18nc("@action", "&All Fields"), this);
    actionCollection()->addAction(QStringLiteral("show_all_fields"), mAllFieldsAction);
    connect(mAllFieldsAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mIdentityAction = new KToggleAction(i18nc("@action", "&Identity"), this);
    actionCollection()->addAction(QStringLiteral("show_identity"), mIdentityAction);
    connect(mIdentityAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mDictionaryAction = new KToggleAction(i18nc("@action", "&Dictionary"), this);
    actionCollection()->addAction(QStringLiteral("show_dictionary"), mDictionaryAction);
    connect(mDictionaryAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mFccAction = new KToggleAction(i18nc("@action", "&Sent-Mail Folder"), this);
    actionCollection()->addAction(QStringLiteral("show_fcc"), mFccAction);
    connect(mFccAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mTransportAction = new KToggleAction(i18nc("@action", "&Mail Transport"), this);
    actionCollection()->addAction(QStringLiteral("show_transport"), mTransportAction);
    connect(mTransportAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mFromAction = new KToggleAction(i18nc("@action", "&From"), this);
    actionCollection()->addAction(QStringLiteral("show_from"), mFromAction);
    connect(mFromAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    mSubjectAction = new KToggleAction(i18nc("@action:inmenu Show the subject in the composer window.", "S&ubject"), this);
    actionCollection()->addAction(QStringLiteral("show_subject"), mSubjectAction);
    connect(mSubjectAction, &KToggleAction::triggered, this, &KMComposerWin::slotUpdateView);
    // end of checkable

    mAppendSignature = new QAction(i18nc("@action", "Append S&ignature"), this);
    actionCollection()->addAction(QStringLiteral("append_signature"), mAppendSignature);
    connect(mAppendSignature, &QAction::triggered, mComposerBase->signatureController(), &MessageComposer::SignatureController::appendSignature);

    mPrependSignature = new QAction(i18nc("@action", "Pr&epend Signature"), this);
    actionCollection()->addAction(QStringLiteral("prepend_signature"), mPrependSignature);
    connect(mPrependSignature, &QAction::triggered, mComposerBase->signatureController(), &MessageComposer::SignatureController::prependSignature);

    mInsertSignatureAtCursorPosition = new QAction(i18nc("@action", "Insert Signature At C&ursor Position"), this);
    actionCollection()->addAction(QStringLiteral("insert_signature_at_cursor_position"), mInsertSignatureAtCursorPosition);
    connect(mInsertSignatureAtCursorPosition,
            &QAction::triggered,
            mComposerBase->signatureController(),
            &MessageComposer::SignatureController::insertSignatureAtCursor);

    mComposerBase->attachmentController()->createActions();

    setStandardToolBarMenuEnabled(true);

    KStandardActions::keyBindings(this, &KMComposerWin::slotEditKeys, actionCollection());
    KStandardActions::configureToolbars(this, &KMComposerWin::slotEditToolbars, actionCollection());
    KStandardActions::preferences(kmkernel, &KMKernel::slotShowConfigurationDialog, actionCollection());

    action = new QAction(QIcon::fromTheme(QStringLiteral("tools-check-spelling")), i18n("&Spellchecker…"), this);
    action->setIconText(i18n("Spellchecker"));
    actionCollection()->addAction(QStringLiteral("setup_spellchecker"), action);
    connect(action, &QAction::triggered, this, &KMComposerWin::slotSpellcheckConfig);

    mEncryptAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("document-encrypt")), i18n("&Encrypt Message"), this);
    mEncryptAction->setIconText(i18n("Encrypt"));
    actionCollection()->addAction(QStringLiteral("encrypt_message"), mEncryptAction);
    mSignAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("document-sign")), i18n("&Sign Message"), this);
    mSignAction->setIconText(i18n("Sign"));
    actionCollection()->addAction(QStringLiteral("sign_message"), mSignAction);
    const auto ident = identity();
    // PENDING(marc): check the uses of this member and split it into
    // smime/openpgp and or enc/sign, if necessary:
    mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
    mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

    mLastEncryptActionState = false;
    mLastSignActionState = pgpAutoSign();

    changeCryptoAction();

    connect(&mEncryptionState, &EncryptionState::possibleEncryptChanged, mEncryptAction, &KToggleAction::setEnabled);
    connect(&mEncryptionState, &EncryptionState::encryptChanged, this, [this](bool enabled) {
        mEncryptAction->setChecked(enabled);
        mComposerBase->attachmentModel()->setEncryptSelected(enabled);
    });
    connect(mEncryptAction, &KToggleAction::triggered, &mEncryptionState, &EncryptionState::toggleOverride);
    connect(mSignAction, &KToggleAction::triggered, this, &KMComposerWin::slotSignToggled);

    QStringList listCryptoFormat;
    listCryptoFormat.reserve(numCryptoMessageFormats);
    for (int i = 0; i < numCryptoMessageFormats; ++i) {
        listCryptoFormat.push_back(Kleo::cryptoMessageFormatToLabel(cryptoMessageFormats[i]));
    }

    mCryptoModuleAction = new KSelectAction(i18n("&Cryptographic Message Format"), this);
    actionCollection()->addAction(QStringLiteral("options_select_crypto"), mCryptoModuleAction);
    connect(mCryptoModuleAction, &KSelectAction::indexTriggered, this, &KMComposerWin::slotCryptoModuleSelected);
    mCryptoModuleAction->setToolTip(i18nc("@info:tooltip", "Select a cryptographic format for this message"));
    mCryptoModuleAction->setItems(listCryptoFormat);

    mComposerBase->editor()->createActions(actionCollection());

    mFollowUpToggleAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("appointment-new")), i18n("Create Follow Up Reminder…"), this);
    actionCollection()->addAction(QStringLiteral("follow_up_mail"), mFollowUpToggleAction);
    connect(mFollowUpToggleAction, &KToggleAction::triggered, this, &KMComposerWin::slotFollowUpMail);
    mFollowUpToggleAction->setEnabled(MessageComposer::FollowUpReminder::isAvailableAndEnabled());

    mPluginEditorManagerInterface->initializePlugins();
    mPluginEditorCheckBeforeSendManagerInterface->initializePlugins();
    mPluginEditorInitManagerInterface->initializePlugins();
    mPluginEditorConvertTextManagerInterface->initializePlugins();
    mPluginEditorGrammarManagerInterface->initializePlugins();

    mShowMenuBarAction = KStandardAction::showMenubar(this, &KMComposerWin::slotToggleMenubar, actionCollection());
    mShowMenuBarAction->setChecked(KMailSettings::self()->composerShowMenuBar());
    slotToggleMenubar(true);

    mHamburgerMenu = KStandardAction::hamburgerMenu(nullptr, nullptr, actionCollection());
    mHamburgerMenu->setShowMenuBarAction(mShowMenuBarAction);
    mHamburgerMenu->setMenuBar(menuBar());
    connect(mHamburgerMenu, &KHamburgerMenu::aboutToShowMenu, this, [this]() {
        updateHamburgerMenu();
        // Immediately disconnect. We only need to run this once, but on demand.
        // NOTE: The nullptr at the end disconnects all connections between
        // q and mHamburgerMenu's aboutToShowMenu signal.
        disconnect(mHamburgerMenu, &KHamburgerMenu::aboutToShowMenu, this, nullptr);
    });

    createGUI(QStringLiteral("kmcomposerui.rc"));
    initializePluginActions();
    connect(toolBar(QStringLiteral("htmlToolBar"))->toggleViewAction(), &QAction::toggled, this, &KMComposerWin::htmlToolBarVisibilityChanged);

    // In Kontact, this entry would read "Configure Kontact", but bring
    // up KMail's config dialog. That's sensible, though, so fix the label.
    QAction *configureAction = actionCollection()->action(QStringLiteral("options_configure"));
    if (configureAction) {
        configureAction->setText(i18n("Configure KMail…"));
    }
}

void KMComposerWin::updateHamburgerMenu()
{
    delete mHamburgerMenu->menu();
    auto menu = new QMenu(this);
    menu->addAction(actionCollection()->action(QStringLiteral("new_composer")));
    menu->addSeparator();
    menu->addAction(actionCollection()->action(KStandardActions::name(KStandardActions::Undo)));
    menu->addAction(actionCollection()->action(KStandardActions::name(KStandardActions::Redo)));
    menu->addSeparator();
    menu->addAction(actionCollection()->action(KStandardActions::name(KStandardActions::Print)));
    menu->addAction(actionCollection()->action(KStandardActions::name(KStandardActions::PrintPreview)));
    menu->addSeparator();
    menu->addAction(actionCollection()->action(QStringLiteral("attach_menu")));
    menu->addSeparator();
    menu->addAction(actionCollection()->action(KStandardActions::name(KStandardActions::Close)));
    mHamburgerMenu->setMenu(menu);
}

void KMComposerWin::slotToggleMenubar(bool dontShowWarning)
{
    if (menuBar()) {
        if (mShowMenuBarAction->isChecked()) {
            menuBar()->show();
        } else {
            if (!dontShowWarning && (!toolBar()->isVisible() || !toolBar()->actions().contains(mHamburgerMenu))) {
                const QString accel = mShowMenuBarAction->shortcut().toString(QKeySequence::NativeText);
                KMessageBox::information(this,
                                         i18n("<qt>This will hide the menu bar completely."
                                              " You can show it again by typing %1.</qt>",
                                              accel),
                                         i18nc("@title:window", "Hide menu bar"),
                                         QStringLiteral("HideMenuBarWarning"));
            }
            menuBar()->hide();
        }
        KMailSettings::self()->setComposerShowMenuBar(mShowMenuBarAction->isChecked());
    }
}

void KMComposerWin::initializePluginActions()
{
    if (guiFactory()) {
        QHash<QString, QList<QAction *>> hashActions;
        QHashIterator<MessageComposer::PluginActionType::Type, QList<QAction *>> localEditorManagerActionsType(mPluginEditorManagerInterface->actionsType());
        while (localEditorManagerActionsType.hasNext()) {
            localEditorManagerActionsType.next();
            QList<QAction *> lst = localEditorManagerActionsType.value();
            if (!lst.isEmpty()) {
                const QString actionlistname = "kmaileditor"_L1 + MessageComposer::PluginActionType::actionXmlExtension(localEditorManagerActionsType.key());
                hashActions.insert(actionlistname, lst);
            }
        }
        QHashIterator<MessageComposer::PluginActionType::Type, QList<QAction *>> localEditorConvertTextManagerActionsType(
            mPluginEditorConvertTextManagerInterface->actionsType());
        while (localEditorConvertTextManagerActionsType.hasNext()) {
            localEditorConvertTextManagerActionsType.next();
            QList<QAction *> lst = localEditorConvertTextManagerActionsType.value();
            if (!lst.isEmpty()) {
                const QString actionlistname =
                    "kmaileditor"_L1 + MessageComposer::PluginActionType::actionXmlExtension(localEditorConvertTextManagerActionsType.key());
                if (hashActions.contains(actionlistname)) {
                    lst = hashActions.value(actionlistname) + lst;
                    hashActions.remove(actionlistname);
                }
                hashActions.insert(actionlistname, lst);
            }
        }

        const QList<KToggleAction *> customToolsWidgetActionList = mCustomToolsWidget->actionList();
        const QString actionlistname = "kmaileditor"_L1 + MessageComposer::PluginActionType::actionXmlExtension(MessageComposer::PluginActionType::Tools);
        for (KToggleAction *act : customToolsWidgetActionList) {
            QList<QAction *> lst{act};
            if (hashActions.contains(actionlistname)) {
                lst = hashActions.value(actionlistname) + lst;
                hashActions.remove(actionlistname);
            }
            hashActions.insert(actionlistname, lst);
        }

        QHash<QString, QList<QAction *>>::const_iterator i = hashActions.constBegin();

        while (i != hashActions.constEnd()) {
            const auto lst = guiFactory()->clients();
            for (KXMLGUIClient *client : lst) {
                client->unplugActionList(i.key());
                client->plugActionList(i.key(), i.value());
            }
            ++i;
        }
        // Initialize statusbar
        const QList<QWidget *> statusbarWidgetList = mPluginEditorManagerInterface->statusBarWidgetList();
        for (int index = 0; index < statusbarWidgetList.count(); ++index) {
            statusBar()->addPermanentWidget(statusbarWidgetList.at(index), 0);
        }
        const QList<QWidget *> statusbarWidgetListConverter = mPluginEditorConvertTextManagerInterface->statusBarWidgetList();
        for (int index = 0; index < statusbarWidgetListConverter.count(); ++index) {
            statusBar()->addPermanentWidget(statusbarWidgetListConverter.at(index), 0);
        }
    }
}

void KMComposerWin::changeCryptoAction()
{
    const auto ident = identity();

    if (!QGpgME::openpgp() && !QGpgME::smime()) {
        // no crypto whatsoever
        mEncryptAction->setEnabled(false);
        mEncryptionState.setPossibleEncrypt(false);
        mSignAction->setEnabled(false);
        setSigning(false);
    } else {
        const bool canOpenPGPSign = QGpgME::openpgp() && !ident.pgpSigningKey().isEmpty();
        const bool canSMIMESign = QGpgME::smime() && !ident.smimeSigningKey().isEmpty();

        // Must use KMComposerWin::pgpAutoSign() here and not Identity::pgpAutoSign(),
        // because the former takes account of both the global defaults and the
        // identity-specific override.
        setSigning((canOpenPGPSign || canSMIMESign) && pgpAutoSign());
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
    mCursorLineLabel->setText(i18nc("Shows the linenumber of the cursor position.", " Line: %1 ", QStringLiteral("     ")));
    statusBar()->addPermanentWidget(mCursorLineLabel);

    mCursorColumnLabel = new QLabel(i18nc("@label:textbox", " Column: %1 ", QStringLiteral("     ")));
    mCursorColumnLabel->setTextFormat(Qt::PlainText);
    statusBar()->addPermanentWidget(mCursorColumnLabel);

    mStatusBarLabelToggledOverrideMode = new MessageComposer::StatusBarLabelToggledState(this);
    mStatusBarLabelToggledOverrideMode->setStateString(i18n("OVR"), i18n("INS"));
    statusBar()->addPermanentWidget(mStatusBarLabelToggledOverrideMode, 0);
    connect(mStatusBarLabelToggledOverrideMode,
            &MessageComposer::StatusBarLabelToggledState::toggleModeChanged,
            this,
            &KMComposerWin::slotOverwriteModeWasChanged);

    mStatusBarLabelSpellCheckingChangeMode = new MessageComposer::StatusBarLabelToggledState(this);
    mStatusBarLabelSpellCheckingChangeMode->setStateString(i18n("Spellcheck: on"), i18n("Spellcheck: off"));
    statusBar()->addPermanentWidget(mStatusBarLabelSpellCheckingChangeMode, 0);
    connect(mStatusBarLabelSpellCheckingChangeMode,
            &MessageComposer::StatusBarLabelToggledState::toggleModeChanged,
            this,
            &KMComposerWin::slotAutoSpellCheckingToggled);
}

void KMComposerWin::setupEditor()
{
    QFontMetrics fm(mBodyFont);
    mComposerBase->editor()->setTabStopDistance(fm.boundingRect(QLatin1Char(' ')).width() * 8);

    slotWordWrapToggled(MessageComposer::MessageComposerSettings::self()->wordWrap());

    // Font setup
    slotUpdateFont();

    connect(mComposerBase->editor(), &QTextEdit::cursorPositionChanged, this, &KMComposerWin::slotCursorPositionChanged);
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

void KMComposerWin::addFaceHeaders(const KIdentityManagementCore::Identity &ident, const std::shared_ptr<KMime::Message> &msg)
{
    if (!ident.isXFaceEnabled() || ident.xface().isEmpty()) {
        msg->removeHeader("X-Face");
    } else {
        QString xface = ident.xface();
        if (!xface.isEmpty()) {
            int numNL = (xface.length() - 1) / 70;
            for (int i = numNL; i > 0; --i) {
                xface.insert(i * 70, QStringLiteral("\n\t"));
            }
            auto header = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("X-Face"));
            header->fromUnicodeString(xface);
            msg->setHeader(std::move(header));
        }
    }

    if (!ident.isFaceEnabled() || ident.face().isEmpty()) {
        msg->removeHeader("Face");
    } else {
        QString face = ident.face();
        if (!face.isEmpty()) {
            // The first line of data is 72 lines long to account for the
            // header name, the following lines are 76 lines long, like in
            // https://quimby.gnus.org/circus/face/
            if (face.length() > 72) {
                int numNL = (face.length() - 73) / 76;

                for (int i = numNL; i > 0; --i) {
                    face.insert(72 + i * 76, QStringLiteral("\n\t"));
                }

                face.insert(72, QStringLiteral("\n\t"));
            }

            auto header = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("Face"));
            header->fromUnicodeString(face);
            msg->setHeader(std::move(header));
        }
    }
}

void KMComposerWin::setMessage(const std::shared_ptr<KMime::Message> &newMsg,
                               bool lastSignState,
                               bool lastEncryptState,
                               bool mayAutoSign,
                               bool allowDecryption,
                               bool isModified)
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

    const auto im = KMKernel::self()->identityManager();

    if (auto hrd = newMsg->headerByType("X-KMail-Identity")) {
        const QString identityStr = hrd->asUnicodeString();
        if (!identityStr.isEmpty()) {
            const auto ident = im->identityForUoid(identityStr.toUInt());
            if (ident.isNull()) {
                if (auto hrdIdentityName = newMsg->headerByType("X-KMail-Identity-Name")) {
                    const QString identityStrName = hrdIdentityName->asUnicodeString();
                    const auto id = im->modifyIdentityForName(identityStrName);
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
        if (auto hrdIdentityName = newMsg->headerByType("X-KMail-Identity-Name")) {
            const QString identityStrName = hrdIdentityName->asUnicodeString();
            const auto id = im->modifyIdentityForName(identityStrName);
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
    mIdentityConnection = connect(mComposerBase->identityCombo(), &KIdentityManagementWidgets::IdentityCombo::identityChanged, this, [this](uint val) {
        slotIdentityChanged(val);
    });

    mMsg = newMsg;

    // manually load the identity's value into the fields; either the one from the
    // message, where appropriate, or the one from the sticky identity. What's in
    // mId might have changed meanwhile, thus the save value
    if (mId == 0) {
        mId = mComposerBase->identityCombo()->currentIdentity();
    }
    slotIdentityChanged(mId, true /*initalChange*/);
    // Fixing the identities with auto signing activated
    mLastSignActionState = sign();

    // Add initial data.
    MessageComposer::PluginEditorConverterInitialData data;
    data.setMewMsg(mMsg);
    data.setNewMessage(mContext == TemplateContext::New);
    mPluginEditorConvertTextManagerInterface->setInitialData(data);

    if (auto msgFrom = mMsg->from(KMime::CreatePolicy::DontCreate)) {
        mEdtFrom->setText(msgFrom->asUnicodeString());
    }
    if (auto msgSubject = mMsg->subject(KMime::CreatePolicy::DontCreate)) {
        mEdtSubject->setPlainText(msgSubject->asUnicodeString());
    }

    // Restore the quote prefix. We can't just use the global quote prefix here,
    // since the prefix is different for each message, it might for example depend
    // on the original sender in a reply.
    if (auto hdr = mMsg->headerByType("X-KMail-QuotePrefix")) {
        mComposerBase->editor()->setQuotePrefixName(hdr->asUnicodeString());
    }

    // check for the presence of a DNT header, indicating that MDN's were requested
    if (auto hdr = newMsg->headerByType("Disposition-Notification-To")) {
        const QString mdnAddr = hdr->asUnicodeString();
        mRequestMDNAction->setChecked((!mdnAddr.isEmpty() && im->thatIsMe(mdnAddr)) || KMailSettings::self()->requestMDN());
    }
    if (auto hdr = newMsg->headerByType("Return-Receipt-To")) {
        const QString returnReceiptToAddr = hdr->asUnicodeString();
        mRequestDeliveryConfirmation->setChecked((!returnReceiptToAddr.isEmpty() && im->thatIsMe(returnReceiptToAddr))
                                                 /*TODO || KMailSettings::self()->requestMDN()*/);
    }
    // check for presence of a priority header, indicating urgent mail:
    auto xPriorityHeader = newMsg->headerByType("X-PRIORITY");
    auto priorityHeader = newMsg->headerByType("Priority");
    if (xPriorityHeader && priorityHeader) {
        const QString xpriority = xPriorityHeader->asUnicodeString();
        const QString priority = priorityHeader->asUnicodeString();
        if (xpriority == "2 (High)"_L1 && priority == "urgent"_L1) {
            mUrgentAction->setChecked(true);
        }
    }

    const auto &ident = identity();

    addFaceHeaders(ident, mMsg);

    // if these headers are present, the state of the message should be overruled
    MessageComposer::DraftSignatureState signState(mMsg);
    if (signState.isDefined()) {
        mLastSignActionState = signState.signState();
    }
    MessageComposer::DraftEncryptionState encState(mMsg);
    if (encState.isDefined()) {
        mLastEncryptActionState = encState.encryptionState();
    }
    MessageComposer::DraftCryptoMessageFormatState formatState(mMsg);
    if (formatState.isDefined()) {
        mCryptoModuleAction->setCurrentItem(format2cb(formatState.cryptoMessageFormatState()));
    }

    mLastIdentityHasSigningKey = !ident.pgpSigningKey().isEmpty() || !ident.smimeSigningKey().isEmpty();
    mLastIdentityHasEncryptionKey = !ident.pgpEncryptionKey().isEmpty() || !ident.smimeEncryptionKey().isEmpty();

    if (QGpgME::openpgp() || QGpgME::smime()) {
        const bool canOpenPGPSign = QGpgME::openpgp() && !ident.pgpSigningKey().isEmpty();
        const bool canSMIMESign = QGpgME::smime() && !ident.smimeSigningKey().isEmpty();

        setSigning((canOpenPGPSign || canSMIMESign) && mLastSignActionState);
    } else {
        mEncryptionState.setPossibleEncrypt(false);
    }
    updateSignatureAndEncryptionStateIndicators();

    // We need to set encryption/signing first before adding content.
    // This is important if we are in auto detect the encrypt mode.
    mComposerBase->setMessage(mMsg, allowDecryption);

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

    auto msgContent = new KMime::Content;
    msgContent->setContent(mMsg->encodedContent());
    msgContent->parse();
    MimeTreeParser::SimpleObjectTreeSource emptySource;
    MimeTreeParser::ObjectTreeParser otp(&emptySource); // All default are ok
    emptySource.setDecryptMessage(allowDecryption);
    otp.parseObjectTree(msgContent);

    delete msgContent;

    if ((MessageComposer::MessageComposerSettings::self()->autoTextSignature() == "auto"_L1) && mayAutoSign) {
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
    QTimer::singleShot(
        0,
        this,
        &KMComposerWin::forceAutoSaveMessage); // Force autosaving to make sure this composer reappears if a crash happens before the autosave timer kicks in.
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
    return mComposerBase->editor()->document()->isModified() || mEdtFrom->isModified() || mComposerBase->recipientsEditor()->isModified()
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
        const QString savebut = (istemplate ? i18n("Re&save as Template") : i18n("&Save as Draft"));
        const QString savetext = (istemplate ? i18n("Resave this message in the Templates folder. "
                                                    "It can then be used at a later time.")
                                             : i18n("Save this message in the Drafts folder. "
                                                    "It can then be edited and sent at a later time."));

        const int rc = KMessageBox::warningTwoActionsCancel(this,
                                                            i18n("Do you want to save the message for later or discard it?"),
                                                            i18nc("@title:window", "Close Composer"),
                                                            KGuiItem(savebut, "document-save"_L1, QString(), savetext),
                                                            KStandardGuiItem::discard(),
                                                            KStandardGuiItem::cancel());
        if (rc == KMessageBox::Cancel) {
            return false;
        } else if (rc == KMessageBox::ButtonCode::PrimaryAction) {
            // doSend will close the window. Just return false from this method
            if (istemplate) {
                slotSaveTemplate();
            } else {
                slotSaveDraft();
            }
            return false;
        }
        // else fall through: return true
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
    const bool checkForForgottenAttachments = mCheckForForgottenAttachments && KMailSettings::self()->showForgottenAttachmentWarning();

    if (!checkForForgottenAttachments) {
        return MessageComposer::ComposerViewBase::NoMissingAttachmentFound;
    }

    mComposerBase->setSubject(subject()); // be sure the composer knows the subject
    const MessageComposer::ComposerViewBase::MissingAttachment missingAttachments =
        mComposerBase->checkForMissingAttachments(KMailSettings::self()->attachmentKeywords());

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

void KMComposerWin::slotSendFailed(const QString &msg, MessageComposer::ComposerViewBase::FailedType type)
{
    setEnabled(true);
    if (!msg.isEmpty()) {
        KMessageBox::error(mMainWidget,
                           msg,
                           (type == MessageComposer::ComposerViewBase::AutoSave) ? i18nc("@title:window", "Autosave Message Failed")
                                                                                 : i18nc("@title:window", "Sending Message Failed"));
    }
}

void KMComposerWin::slotSendSuccessful(Akonadi::Item::Id id)
{
    if (id != -1) {
        UndoSendManager::UndoSendManagerInfo info;
        info.subject = MessageCore::StringUtil::quoteHtmlChars(subject());
        info.index = id;
        info.delay = KMailSettings::self()->undoSendDelay();
        info.to = MessageCore::StringUtil::quoteHtmlChars(mComposerBase->to());

        UndoSendManager::self()->addItem(std::move(info));
    }
    setModified(false);
    mComposerBase->cleanupAutoSave();
    mFolder = Akonadi::Collection(); // see dtor
    close();
}

const KIdentityManagementCore::Identity &KMComposerWin::identity() const
{
    return KMKernel::self()->identityManager()->identityForUoidOrDefault(currentIdentity());
}

bool KMComposerWin::pgpAutoEncrypt() const
{
    const auto ident = identity();
    if (ident.encryptionOverride()) {
        return ident.pgpAutoEncrypt();
    } else {
        return MessageComposer::MessageComposerSettings::self()->cryptoAutoEncrypt();
    }
}

bool KMComposerWin::pgpAutoSign() const
{
    const auto ident = identity();
    if (ident.encryptionOverride()) {
        return ident.pgpAutoSign();
    } else {
        return MessageComposer::MessageComposerSettings::self()->cryptoAutoSign();
    }
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
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kaddressbook"), {}, this);
    job->setDesktopName(QStringLiteral("org.kde.kaddressbook"));
    job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    job->start();
#else
    const QString path = TextAddonsWidgets::ExecutableUtils::findExecutable(QStringLiteral("kaddressbook"));
    if (path.isEmpty() || !QProcess::startDetached(path)) {
        KMessageBox::error(this,
                           i18n("Could not start \"KAddressbook\" program. "
                                "Please check your installation."),
                           i18nc("@title:window", "Unable to start \"KAddressbook\" program"));
    }
#endif
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
    KConfigGroup group(config, QStringLiteral("Composer"));
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
        encoding = encodings[index];
    } else {
        qCDebug(KMAIL_LOG) << " encoding not found so we can't insert text"; // see InsertTextFileJob
        return;
    }
    auto job = new MessageComposer::InsertTextFileJob(mComposerBase->editor(), u);
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
    QString recentDirClass;
    QUrl startUrl = KFileWidget::getStartUrl(QUrl(QStringLiteral("kfiledialog:///InsertFile")), recentDirClass);

    const auto url = QFileDialog::getOpenFileUrl(this, i18nc("@title:window", "Insert File"), startUrl);
    if (url.isValid()) {
        std::optional<QStringConverter::Encoding> encoding;

        QFile file(url.toLocalFile());
        if (file.open(QIODeviceBase::ReadOnly)) {
            auto content = file.read(1024 * 1024); // only read the first 1MB
            if (content.isEmpty()) {
                encoding = QStringConverter::System;
            } else if (url.toLocalFile().endsWith(QStringLiteral("html"))) {
                encoding = QStringConverter::encodingForHtml(content);
            } else {
                encoding = QStringConverter::encodingForData(content);
            }
        }

        auto encodingName = QStringConverter::nameForEncoding(encoding.value_or(QStringConverter::System));
        if (!encodingName) {
            encodingName = "UTF-8";
        }

        if (strcmp(encodingName, "Locale") == 0) {
            encodingName = "UTF-8";
        }

        QUrl urlWithEncoding = url;
        MessageCore::StringUtil::setEncodingFile(urlWithEncoding, QLatin1StringView(encodingName));
        if (!recentDirClass.isEmpty()) {
            KRecentDirs::add(recentDirClass, urlWithEncoding.path());
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
        if (url.scheme() == "mailto"_L1) {
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
    } else if (source->hasHtml() && (mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Plain) && source->hasText()
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
            if (mComposerBase->editor()->textMode()
                == MessageComposer::RichTextComposerNg::Rich /*&& mComposerBase->editor()->isEnableImageActions() Necessary ?*/) {
                auto image = qvariant_cast<QImage>(source->imageData());
                QFileInfo fi(source->text());

                QMenu menu(this);
                const QAction *addAsInlineImageAction = menu.addAction(i18nc("@action", "Add as &Inline Image"));
                menu.addAction(i18nc("@action", "Add as &Attachment"));
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
        QString attName = QInputDialog::getText(this, i18nc("@title:window", "KMail"), i18n("Name of the attachment:"), QLineEdit::Normal, QString(), &ok);
        if (!ok) {
            return true;
        }
        attName = attName.trimmed();
        if (attName.isEmpty()) {
            KMessageBox::error(this, i18n("Attachment name can't be empty"), i18nc("@title:window", "Invalid Attachment Name"));

            return true;
        }
        addAttachment(attName, KMime::Headers::CEbase64, QString(), imageData, "image/png");
        return true;
    } else {
        auto job = new DndFromArkJob(this);
        job->setComposerWin(this);
        if (job->extract(source)) {
            return true;
        }
    }

    // If this is a URL list, add those files as attachments or text
    // but do not offer this if we are pasting plain text containing an url, e.g. from a browser
    const QList<QUrl> urlList = source->urls();
    if (!urlList.isEmpty()) {
        // Search if it's message items.
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
                QList<AttachmentInfo> infoList;
                infoList.reserve(urlList.count());
                for (const QUrl &url : urlList) {
                    AttachmentInfo info;
                    info.url = url;
                    infoList.append(std::move(info));
                }
                addAttachment(infoList, false);
            } else {
                QMenu p;
                const int sizeUrl(urlList.size());
                const QAction *addAsTextAction = p.addAction(i18np("Add URL into Message", "Add URLs into Message", sizeUrl));
                const QAction *addAsAttachmentAction = p.addAction(i18np("Add File as &Attachment", "Add Files as &Attachment", sizeUrl));
                const QAction *selectedAction = p.exec(QCursor::pos());

                if (selectedAction == addAsTextAction) {
                    insertUrls(source, urlList);
                } else if (selectedAction == addAsAttachmentAction) {
                    QList<AttachmentInfo> infoList;
                    for (const QUrl &url : urlList) {
                        if (url.isValid()) {
                            AttachmentInfo info;
                            info.url = url;
                            infoList.append(std::move(info));
                        }
                    }
                    addAttachment(infoList, false);
                }
            }
            return true;
        } else {
            if (!items.isEmpty()) {
                auto itemFetchJob = new Akonadi::ItemFetchJob(items, this);
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
                                                      i18nc("@title:window", "Insert clipboard text as attachment"),
                                                      i18n("Name of the attachment:"),
                                                      QLineEdit::Normal,
                                                      QString(),
                                                      &ok);
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
    auto fjob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (!fjob) {
        return;
    }
    const Akonadi::Item::List items = fjob->items();

    if (items.isEmpty()) {
        return;
    }

    if (items.constFirst().mimeType() == KMime::Message::mimeType()) {
        uint identity = 0;
        if (items.at(0).isValid()) {
            const Akonadi::Collection parentCollection = items.at(0).parentCollection();
            if (parentCollection.isValid()) {
                const QString resourceName = parentCollection.resource();
                if (!resourceName.isEmpty()) {
                    QSharedPointer<MailCommon::FolderSettings> fd(MailCommon::FolderSettings::forCollection(parentCollection, false));
                    if (!fd.isNull()) {
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
                const auto contact = item.payload<KContacts::Addressee>();
                attachmentName = contact.realName() + ".vcf"_L1;
                // Workaround about broken kaddressbook fields.
                QByteArray data = item.payloadData();
                KContacts::adaptIMAttributes(data);
                addAttachment(attachmentName, KMime::Headers::CEbase64, QString(), data, "text/x-vcard");
            } else if (item.hasPayload<KContacts::ContactGroup>()) {
                const auto group = item.payload<KContacts::ContactGroup>();
                attachmentName = group.name() + ".vcf"_L1;
                auto expandJob = new Akonadi::ContactGroupExpandJob(group, this);
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
    auto expandJob = qobject_cast<Akonadi::ContactGroupExpandJob *>(job);
    Q_ASSERT(expandJob);

    const QString attachmentName = expandJob->property("groupName").toString();
    KContacts::VCardConverter converter;
    const QByteArray groupData = converter.exportVCards(expandJob->contacts(), KContacts::VCardConverter::v4_0);
    if (!groupData.isEmpty()) {
        addAttachment(attachmentName, KMime::Headers::CEbase64, QString(), groupData, "text/x-vcard");
    }
}

void KMComposerWin::slotNewComposer()
{
    auto job = new KMComposerCreateNewComposerJob;
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

void KMComposerWin::slotSignToggled(bool on)
{
    setSigning(on, true);
    updateSignatureAndEncryptionStateIndicators();
}

void KMComposerWin::setSigning(bool sign, bool setByUser)
{
    const bool wasModified = isModified();
    if (setByUser) {
        setModified(true);
    }
    if (!mSignAction->isEnabled()) {
        sign = false;
    }

    // check if the user defined a signing key for the current identity
    if (sign && !mLastIdentityHasSigningKey) {
        if (setByUser) {
            KMessageBox::error(this,
                               i18n("<qt><p>In order to be able to sign "
                                    "this message you first have to "
                                    "define the (OpenPGP or S/MIME) signing key "
                                    "to use.</p>"
                                    "<p>Please select the key to use "
                                    "in the identity configuration.</p>"
                                    "</qt>"),
                               i18nc("@title:window", "Undefined Signing Key"));
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

int KMComposerWin::validateLineWrapWidth() const
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

void KMComposerWin::forceEnableHtml()
{
    enableHtml();
    // TODO
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
    auto composer = new MessageComposer::ComposerJob();
    mComposerBase->fillComposer(composer);
    mMiscComposers.append(composer);
    composer->setProperty("preview", preview);
    connect(composer, &MessageComposer::ComposerJob::result, this, &KMComposerWin::slotPrintComposeResult);
    composer->start();
}

void KMComposerWin::slotPrintComposeResult(KJob *job)
{
    const bool preview = job->property("preview").toBool();
    printComposeResult(job, preview);
}

void KMComposerWin::printComposeResult(KJob *job, bool preview)
{
    Q_ASSERT(dynamic_cast<MessageComposer::ComposerJob *>(job));
    auto composer = qobject_cast<MessageComposer::ComposerJob *>(job);
    Q_ASSERT(mMiscComposers.contains(composer));
    mMiscComposers.removeAll(composer);

    if (composer->error() == MessageComposer::ComposerJob::NoError) {
        Q_ASSERT(composer->resultMessages().size() == 1);
        Akonadi::Item printItem;
        printItem.setPayload<std::shared_ptr<KMime::Message>>(composer->resultMessages().constFirst());
        Akonadi::MessageFlags::copyMessageFlags(*(composer->resultMessages().constFirst()), printItem);
        const bool isHtml = mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich;
        const MessageViewer::Viewer::DisplayFormatMessage format = isHtml ? MessageViewer::Viewer::Html : MessageViewer::Viewer::Text;
        KMPrintCommandInfo commandInfo;
        commandInfo.mMsg = printItem;
        commandInfo.mFormat = format;
        commandInfo.mHtmlLoadExtOverride = isHtml;
        commandInfo.mPrintPreview = preview;
        auto command = new KMPrintCommand(this, commandInfo);
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

    // TODO generate new message from plugins.
    MessageComposer::PluginEditorConverterBeforeConvertingData data;
    data.setNewMessage(mContext == TemplateContext::New);
    mPluginEditorConvertTextManagerInterface->setDataBeforeConvertingText(data);

    // TODO converttext if necessary

    // TODO integrate with MDA online status
    if (method == MessageComposer::MessageSender::SendImmediate) {
        if (!MessageComposer::Util::sendMailDispatcherIsOnline()) {
            method = MessageComposer::MessageSender::SendLater;
        }
        if (KMailSettings::self()->enabledUndoSend()) {
            mComposerBase->setSendLaterInfo(nullptr);
            const bool wasRegistered = sendLaterRegistered();
            if (wasRegistered) {
                auto info = new MessageComposer::SendLaterInfo;
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

    if (saveIn == MessageComposer::MessageSender::SaveInNone || willSendItWithoutReediting) { // don't save as draft or template, send immediately
        if (KEmailAddress::firstEmailAddress(from()).isEmpty()) {
            if (!(mShowHeaders & HDR_FROM)) {
                mShowHeaders |= HDR_FROM;
                rethinkFields(false);
            }
            mEdtFrom->setFocus();
            KMessageBox::error(this,
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
                const int rc = KMessageBox::questionTwoActions(this,
                                                               i18n("To: field is empty. "
                                                                    "Send message anyway?"),
                                                               i18nc("@title:window", "No To: specified"),
                                                               KGuiItem(i18nc("@action:button", "S&end as Is"), "mail-send"_L1),
                                                               KGuiItem(i18nc("@action:button", "&Specify the To field"), "edit-rename"_L1));
                if (rc == KMessageBox::ButtonCode::SecondaryAction) {
                    return;
                }
            }
        }

        if (subject().isEmpty()) {
            mEdtSubject->setFocus();
            const int rc = KMessageBox::questionTwoActions(this,
                                                           i18n("You did not specify a subject. "
                                                                "Send message anyway?"),
                                                           i18nc("@title:window", "No Subject Specified"),
                                                           KGuiItem(i18nc("@action:button", "S&end as Is"), "mail-send"_L1),
                                                           KGuiItem(i18nc("@action:button", "&Specify the Subject"), "edit-rename"_L1));
            if (rc == KMessageBox::ButtonCode::SecondaryAction) {
                return;
            }
        }

        MessageComposer::PluginEditorCheckBeforeSendParams params;
        params.setSubject(subject());
        params.setHtmlMail(mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich);
        params.setIdentity(currentIdentity());
        params.setHasAttachment(mComposerBase->attachmentModel()->rowCount() > 0);
        params.setTransportId(mComposerBase->transportComboBox()->currentTransportId());
        const auto ident = identity();
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
        auto job = new AddressValidationJob(recipients.join(", "_L1), this, this);
        job->setDefaultDomain(defaultDomainName);
        job->setProperty("method", static_cast<int>(method));
        job->setProperty("saveIn", static_cast<int>(saveIn));
        connect(job, &Akonadi::ItemFetchJob::result, this, &KMComposerWin::slotDoDelayedSend);
        job->start();

        // we'll call send from within slotDoDelaySend
    } else {
        if (saveIn == MessageComposer::MessageSender::SaveInDrafts && mEncryptionState.encrypt() && KMailSettings::self()->alwaysEncryptDrafts()
            && mComposerBase->to().isEmpty() && mComposerBase->cc().isEmpty()) {
            KMessageBox::information(this,
                                     i18n("You must specify at least one receiver "
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

    // Abort sending if one of the recipient addresses is invalid …
    if (!validateJob->isValid()) {
        setEnabled(true);
        return;
    }

    // … otherwise continue as usual
    const MessageComposer::MessageSender::SendMethod method = static_cast<MessageComposer::MessageSender::SendMethod>(job->property("method").toInt());
    const MessageComposer::MessageSender::SaveIn saveIn = static_cast<MessageComposer::MessageSender::SaveIn>(job->property("saveIn").toInt());

    doDelayedSend(method, saveIn);
}

void KMComposerWin::applyComposerSetting(MessageComposer::ComposerViewBase *composerBase)
{
    composerBase->setFrom(from());
    composerBase->setSubject(subject());
    composerBase->setUrgent(mUrgentAction->isChecked());
    composerBase->setMDNRequested(mRequestMDNAction->isChecked());
    composerBase->setRequestDeleveryConfirmation(mRequestDeliveryConfirmation->isChecked());
}

void KMComposerWin::doDelayedSend(MessageComposer::MessageSender::SendMethod method, MessageComposer::MessageSender::SaveIn saveIn)
{
    KCursorSaver saver(Qt::WaitCursor);
    applyComposerSetting(mComposerBase);
    if (mForceDisableHtml) {
        disableHtml(MessageComposer::ComposerViewBase::NoConfirmationNeeded);
    }
    const bool encrypt = mEncryptionState.encrypt();

    mComposerBase->setCryptoOptions(
        sign(),
        encrypt,
        cryptoMessageFormat(),
        ((saveIn != MessageComposer::MessageSender::SaveInNone && !KMailSettings::self()->alwaysEncryptDrafts()) || mSigningAndEncryptionExplicitlyDisabled));

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
    return MessageComposer::SendLaterUtil::sentLaterAgentWasRegistered() && MessageComposer::SendLaterUtil::sentLaterAgentEnabled();
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
            MessageComposer::SendLaterInfo *info = nullptr;
            QPointer<MessageComposer::SendLaterDialog> dlg = new MessageComposer::SendLaterDialog(info, this);
            if (dlg->exec()) {
                info = dlg->info();
                const MessageComposer::SendLaterDialog::SendLaterAction action = dlg->action();
                delete dlg;
                switch (action) {
                case MessageComposer::SendLaterDialog::Unknown:
                    qCDebug(KMAIL_LOG) << "Sendlater action \"Unknown\": Need to fix it.";
                    break;
                case MessageComposer::SendLaterDialog::Canceled:
                    return;
                    break;
                case MessageComposer::SendLaterDialog::PutInOutbox:
                    doSend(MessageComposer::MessageSender::SendLater);
                    break;
                case MessageComposer::SendLaterDialog::SendDeliveryAtTime:
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
    const int rc = KMessageBox::warningTwoActionsCancel(mMainWidget,
                                                        i18n("About to send email…"),
                                                        i18nc("@title:window", "Send Confirmation"),
                                                        KGuiItem(i18nc("@action:button", "&Send Now"), "mail-send"_L1),
                                                        KGuiItem(i18nc("@action:button", "Send &Later"), "mail-queue"_L1));

    if (rc == KMessageBox::ButtonCode::PrimaryAction) {
        doSend(MessageComposer::MessageSender::SendImmediate);
    } else if (rc == KMessageBox::ButtonCode::SecondaryAction) {
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
    QStringList lst{mComposerBase->to()};
    const QString ccStr = mComposerBase->cc();
    if (!ccStr.isEmpty()) {
        lst << KEmailAddress::splitAddressList(ccStr);
    }
    const QString bccStr = mComposerBase->bcc();
    if (!bccStr.isEmpty()) {
        lst << KEmailAddress::splitAddressList(bccStr);
    }
    if (lst.isEmpty()) {
        slotCheckSendNowStep2();
    } else {
        auto job = new PotentialPhishingEmailJob(this);
        KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("PotentialPhishing"));
        const QStringList whiteList = group.readEntry("whiteList", QStringList());
        job->setEmailWhiteList(whiteList);
        job->setPotentialPhishingEmails(lst);
        connect(job, &PotentialPhishingEmailJob::potentialPhishingEmailsFound, this, &KMComposerWin::slotPotentialPhishingEmailsFound);
        if (!job->start()) {
            qCWarning(KMAIL_LOG) << "PotentialPhishingEmailJob can't start";
        }
    }
}

void KMComposerWin::slotPotentialPhishingEmailsFound(const QStringList &list)
{
    if (list.isEmpty()) {
        slotCheckSendNowStep2();
    } else {
        if (!mPotentialPhishingEmailWarning) {
            mPotentialPhishingEmailWarning = new PotentialPhishingEmailWarning(this);
            connect(mPotentialPhishingEmailWarning, &PotentialPhishingEmailWarning::sendNow, this, &KMComposerWin::slotCheckSendNowStep2);
            mEditorAndCryptoStateIndicatorsLayout->insertWidget(0, mPotentialPhishingEmailWarning);
        }
        mPotentialPhishingEmailWarning->setPotentialPhisingEmail(list);
    }
}

bool KMComposerWin::checkRecipientNumber() const
{
    const int thresHold = KMailSettings::self()->recipientThreshold();
    if (KMailSettings::self()->tooManyRecipients() && mComposerBase->recipientsEditor()->recipients().count() > thresHold) {
        if (KMessageBox::questionTwoActions(mMainWidget,
                                            i18n("You are trying to send the mail to more than %1 recipients. Send message anyway?", thresHold),
                                            i18nc("@title:window", "Too many recipients"),
                                            KGuiItem(i18nc("@action:button", "&Send as Is")),
                                            KGuiItem(i18nc("@action:button", "&Edit Recipients")))
            == KMessageBox::ButtonCode::SecondaryAction) {
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
    if (confirmation == MessageComposer::ComposerViewBase::LetUserConfirm && mComposerBase->editor()->composerControler()->isFormattingUsed()
        && !mForceDisableHtml) {
        int choice = KMessageBox::warningTwoActionsCancel(this,
                                                          i18n("Turning HTML mode off "
                                                               "will cause the text to lose the formatting. Are you sure?"),
                                                          i18nc("@title:window", "Lose the formatting?"),
                                                          KGuiItem(i18nc("@action:button", "Lose Formatting")),
                                                          KGuiItem(i18nc("@action:button", "Add Markup Plain Text")),
                                                          KStandardGuiItem::cancel(),
                                                          QStringLiteral("LoseFormattingWarning"));

        switch (choice) {
        case KMessageBox::Cancel:
            enableHtml();
            return;
        case KMessageBox::ButtonCode::SecondaryAction:
            forcePlainTextMarkup = true;
            break;
        case KMessageBox::ButtonCode::PrimaryAction:
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
        disableHtml(MessageComposer::ComposerViewBase::NoConfirmationNeeded); // ### Can this happen at all?
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

void KMComposerWin::showAndActivateComposer()
{
    show();
    raise();
    activateWindow();
}

void KMComposerWin::slotSpellCheckingStatus(const QString &status)
{
    mStatusbarLabel->setText(status);
    QTimer::singleShot(2s, this, &KMComposerWin::slotSpellcheckDoneClearStatus);
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
    const KIdentityManagementCore::Identity &ident = KMKernel::self()->identityManager()->identityForUoid(uoid);
    if (ident.isNull()) {
        return;
    }
    const bool wasModified(isModified());
    Q_EMIT identityChanged(identity());
    if (!ident.fullEmailAddr().isNull()) {
        mEdtFrom->setText(ident.fullEmailAddr());
    }

    // make sure the From field is shown if it does not contain a valid email address
    if (KEmailAddress::firstEmailAddress(from()).isEmpty()) {
        mShowHeaders |= HDR_FROM;
    }

    // remove BCC of old identity and add BCC of new identity (if they differ)
    const KIdentityManagementCore::Identity &oldIdentity = KMKernel::self()->identityManager()->identityForUoidOrDefault(mId);

    if (ident.organization().isEmpty()) {
        mMsg->removeHeader<KMime::Headers::Organization>();
    } else {
        auto organization = std::unique_ptr<KMime::Headers::Organization>(new KMime::Headers::Organization);
        organization->fromUnicodeString(ident.organization());
        mMsg->setHeader(std::move(organization));
    }

    addFaceHeaders(ident, mMsg);

    if (initialChange) {
        if (auto hrd = mMsg->headerByType("X-KMail-Transport")) {
            const QString mailtransportStr = hrd->asUnicodeString();
            if (!mailtransportStr.isEmpty()) {
                int transportId = mailtransportStr.toInt();
                const Transport *transport = TransportManager::self()->transportById(transportId, false); /*don't return default transport */
                if (transport) {
                    auto header = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("X-KMail-Transport"));
                    header->fromUnicodeString(QString::number(transport->id()));
                    mMsg->setHeader(std::move(header));
                    mComposerBase->transportComboBox()->setCurrentTransport(transport->id());
                } else {
                    if (auto hrdTransportName = mMsg->headerByType("X-KMail-Transport-Name")) {
                        const QString identityStrName = hrdTransportName->asUnicodeString();
                        const Transport *transportFromStrName = TransportManager::self()->transportByName(identityStrName, true);
                        if (transportFromStrName) {
                            auto header = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("X-KMail-Transport"));
                            header->fromUnicodeString(QString::number(transportFromStrName->id()));
                            mMsg->setHeader(std::move(header));
                            mComposerBase->transportComboBox()->setCurrentTransport(transportFromStrName->id());
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
                auto header = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("X-KMail-Transport"));
                header->fromUnicodeString(QString::number(transport->id()));
                mMsg->setHeader(std::move(header));
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
            auto header = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("X-KMail-Transport"));
            header->fromUnicodeString(QString::number(transport->id()));
            mMsg->setHeader(std::move(header));
            mComposerBase->transportComboBox()->setCurrentTransport(transport->id());
        }
    }

    const bool fccIsDisabled = ident.disabledFcc();
    if (fccIsDisabled) {
        auto header = std::unique_ptr<KMime::Headers::Generic>(new KMime::Headers::Generic("X-KMail-FccDisabled"));
        header->fromUnicodeString(QStringLiteral("true"));
        mMsg->setHeader(std::move(header));
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
    if (!wasModified && !(ident.templates().isEmpty() && mCustomTemplate.isEmpty()) && !initialChange) {
        applyTemplate(uoid, mId, ident, wasModified);
    } else {
        mComposerBase->identityChanged(ident, oldIdentity, false);
        mEdtSubject->setAutocorrectionLanguage(ident.autocorrectionLanguage());
        updateComposerAfterIdentityChanged(ident, uoid, wasModified);
    }
}

void KMComposerWin::checkOwnKeyExpiry(const KIdentityManagementCore::Identity &ident)
{
    mNearExpiryWarning->clearInfo();
    mNearExpiryWarning->hide();

    if (cryptoMessageFormat() & Kleo::AnyOpenPGP) {
        if (!ident.pgpEncryptionKey().isEmpty()) {
            auto const key = mKeyCache->findByKeyIDOrFingerprint(ident.pgpEncryptionKey().constData());
            if (key.isNull() || !key.canEncrypt()) {
                mNearExpiryWarning->addInfo(i18nc("The argument is as PGP fingerprint",
                                                  "Your selected PGP key (%1) doesn't exist in your keyring or is not suitable for encryption.",
                                                  QString::fromLatin1(ident.pgpEncryptionKey())));
                mNearExpiryWarning->setWarning(true);
                mNearExpiryWarning->show();
            } else {
                mComposerBase->expiryChecker()->checkKey(key, Kleo::ExpiryChecker::OwnEncryptionKey);
            }
        }
        if (!ident.pgpSigningKey().isEmpty()) {
            if (ident.pgpSigningKey() != ident.pgpEncryptionKey()) {
                auto const key = mKeyCache->findByKeyIDOrFingerprint(ident.pgpSigningKey().constData());
                if (key.isNull() || !key.canSign()) {
                    mNearExpiryWarning->addInfo(i18nc("The argument is as PGP fingerprint",
                                                      "Your selected PGP signing key (%1) doesn't exist in your keyring or is not suitable for signing.",
                                                      QString::fromLatin1(ident.pgpSigningKey())));
                    mNearExpiryWarning->setWarning(true);
                    mNearExpiryWarning->show();
                } else {
                    mComposerBase->expiryChecker()->checkKey(key, Kleo::ExpiryChecker::OwnSigningKey);
                }
            }
        }
    }

    if (cryptoMessageFormat() & Kleo::AnySMIME) {
        if (!ident.smimeEncryptionKey().isEmpty()) {
            auto const key = mKeyCache->findByKeyIDOrFingerprint(ident.smimeEncryptionKey().constData());
            if (key.isNull() || !key.canEncrypt()) {
                mNearExpiryWarning->addInfo(i18nc("The argument is as SMIME fingerprint",
                                                  "Your selected SMIME key (%1) doesn't exist in your keyring or is not suitable for encryption.",
                                                  QString::fromLatin1(ident.smimeEncryptionKey())));
                mNearExpiryWarning->setWarning(true);
                mNearExpiryWarning->show();
            } else {
                mComposerBase->expiryChecker()->checkKey(key, Kleo::ExpiryChecker::OwnEncryptionKey);
            }
        }
        if (!ident.smimeSigningKey().isEmpty()) {
            if (ident.smimeSigningKey() != ident.smimeEncryptionKey()) {
                auto const key = mKeyCache->findByKeyIDOrFingerprint(ident.smimeSigningKey().constData());
                if (key.isNull() || !key.canSign()) {
                    mNearExpiryWarning->addInfo(i18nc("The argument is as SMIME fingerprint",
                                                      "Your selected SMIME signing key (%1) doesn't exist in your keyring or is not suitable for signing.",
                                                      QString::fromLatin1(ident.smimeSigningKey())));
                    mNearExpiryWarning->setWarning(true);
                    mNearExpiryWarning->show();
                } else {
                    mComposerBase->expiryChecker()->checkKey(key, Kleo::ExpiryChecker::OwnSigningKey);
                }
            }
        }
    }
}

void KMComposerWin::updateComposerAfterIdentityChanged(const KIdentityManagementCore::Identity &ident, uint uoid, bool wasModified)
{
    // disable certain actions if there is no PGP user identity set
    // for this profile
    bool bPGPEncryptionKey = !ident.pgpEncryptionKey().isEmpty();
    bool bPGPSigningKey = !ident.pgpSigningKey().isEmpty();
    bool bSMIMEEncryptionKey = !ident.smimeEncryptionKey().isEmpty();
    bool bSMIMESigningKey = !ident.smimeSigningKey().isEmpty();
    if (cryptoMessageFormat() & Kleo::AnyOpenPGP) {
        if (bPGPEncryptionKey) {
            auto const key = mKeyCache->findByKeyIDOrFingerprint(ident.pgpEncryptionKey().constData());
            if (key.isNull() || !key.canEncrypt()) {
                bPGPEncryptionKey = false;
            }
        }
        if (bPGPSigningKey) {
            auto const key = mKeyCache->findByKeyIDOrFingerprint(ident.pgpSigningKey().constData());
            if (key.isNull() || !key.canSign()) {
                bPGPSigningKey = false;
            }
        }
    } else {
        bPGPEncryptionKey = false;
        bPGPSigningKey = false;
    }

    if (cryptoMessageFormat() & Kleo::AnySMIME) {
        if (bSMIMEEncryptionKey) {
            auto const key = mKeyCache->findByKeyIDOrFingerprint(ident.smimeEncryptionKey().constData());
            if (key.isNull() || !key.canEncrypt()) {
                bSMIMEEncryptionKey = false;
            }
        }
        if (bSMIMESigningKey) {
            auto const key = mKeyCache->findByKeyIDOrFingerprint(ident.smimeSigningKey().constData());
            if (key.isNull() || !key.canSign()) {
                bSMIMESigningKey = false;
            }
        }
    } else {
        bSMIMEEncryptionKey = false;
        bSMIMESigningKey = false;
    }

    const bool bNewIdentityHasSigningKey = bPGPSigningKey || bSMIMESigningKey;
    const bool bNewIdentityHasEncryptionKey = bPGPEncryptionKey || bSMIMEEncryptionKey;

    if (!mKeyCache->initialized()) {
        // We need to start key listing on our own othweise KMail will crash and we want to wait till the cache is populated.
        mKeyCache->startKeyListing();
        connect(mKeyCache.get(), &Kleo::KeyCache::keyListingDone, this, [this, &ident]() {
            checkOwnKeyExpiry(ident);
            runKeyResolver();
        });
    } else {
        checkOwnKeyExpiry(ident);
    }

    // save the state of the sign and encrypt button
    if (!bNewIdentityHasEncryptionKey && mLastIdentityHasEncryptionKey) {
        mLastEncryptActionState = mEncryptionState.encrypt();
    }

    mSignAction->setEnabled(bNewIdentityHasSigningKey);

    if (!bNewIdentityHasSigningKey && mLastIdentityHasSigningKey) {
        mLastSignActionState = sign();
        setSigning(false);
    }
    // restore the last state of the sign and encrypt button
    if (bNewIdentityHasSigningKey && !mLastIdentityHasSigningKey) {
        setSigning(mLastSignActionState);
    }

    mCryptoModuleAction->setCurrentItem(format2cb(Kleo::stringToCryptoMessageFormat(ident.preferredCryptoMessageFormat())));
    slotSelectCryptoModule(true);

    mLastIdentityHasSigningKey = bNewIdentityHasSigningKey;
    mLastIdentityHasEncryptionKey = bNewIdentityHasEncryptionKey;
    const KIdentityManagementCore::Signature sig = const_cast<KIdentityManagementCore::Identity &>(ident).signature();
    bool isEnabledSignature = sig.isEnabledSignature();
    mAppendSignature->setEnabled(isEnabledSignature);
    mPrependSignature->setEnabled(isEnabledSignature);
    mInsertSignatureAtCursorPosition->setEnabled(isEnabledSignature);

    mId = uoid;

    mEncryptionState.setPossibleEncrypt(true);
    changeCryptoAction();
    mEncryptionState.unsetOverride();
    mEncryptionState.setPossibleEncrypt(mEncryptionState.possibleEncrypt() && bNewIdentityHasEncryptionKey);
    mEncryptionState.setAutoEncrypt(ident.pgpAutoEncrypt());

    // make sure the From and BCC fields are shown if necessary
    rethinkFields(false);
    setModified(wasModified);

    if (ident.pgpAutoEncrypt() && mKeyCache->initialized()) {
        runKeyResolver();
    }
}

auto findSendersUid(const std::string &addrSpec, const std::vector<GpgME::UserID> &userIds)
{
    return std::find_if(userIds.cbegin(), userIds.cend(), [&addrSpec](const auto &uid) {
        return uid.addrSpec() == addrSpec || (uid.addrSpec().empty() && std::string(uid.email()) == addrSpec)
            || (uid.addrSpec().empty() && (!uid.email() || !*uid.email()) && uid.name() == addrSpec);
    });
}

std::unique_ptr<Kleo::KeyResolverCore> KMComposerWin::fillKeyResolver()
{
    const auto ident = identity();
    auto keyResolverCore = std::make_unique<Kleo::KeyResolverCore>(true, sign());

    keyResolverCore->setMinimumValidity(GpgME::UserID::Unknown);

    QStringList signingKeys, encryptionKeys;

    if (cryptoMessageFormat() & Kleo::AnyOpenPGP) {
        if (!ident.pgpSigningKey().isEmpty()) {
            signingKeys.push_back(QLatin1StringView(ident.pgpSigningKey()));
        }
        if (!ident.pgpEncryptionKey().isEmpty()) {
            encryptionKeys.push_back(QLatin1StringView(ident.pgpEncryptionKey()));
        }
    }

    if (cryptoMessageFormat() & Kleo::AnySMIME) {
        if (!ident.smimeSigningKey().isEmpty()) {
            signingKeys.push_back(QLatin1StringView(ident.smimeSigningKey()));
        }
        if (!ident.smimeEncryptionKey().isEmpty()) {
            encryptionKeys.push_back(QLatin1StringView(ident.smimeEncryptionKey()));
        }
    }

    keyResolverCore->setSender(ident.fullEmailAddr());
    keyResolverCore->setSigningKeys(signingKeys);
    keyResolverCore->setOverrideKeys({{GpgME::UnknownProtocol, {{keyResolverCore->normalizedSender(), encryptionKeys}}}});

    QStringList recipients;
    const auto lst = mComposerBase->recipientsEditor()->lines();
    for (auto line : lst) {
        auto recipient = line->data().dynamicCast<MessageComposer::Recipient>();
        recipients.push_back(recipient->email());
    }

    keyResolverCore->setRecipients(recipients);
    return keyResolverCore;
}

void KMComposerWin::slotEncryptionButtonIconUpdate()
{
    const auto state = mEncryptionState.encrypt();
    const auto setByUser = mEncryptionState.override();
    const auto acceptedSolution = mEncryptionState.acceptedSolution();

    auto icon = QIcon::fromTheme(QStringLiteral("document-encrypt"));
    QString tooltip;
    if (state) {
        tooltip = i18nc("@info:tooltip", "Encrypt");
    } else {
        tooltip = i18nc("@info:tooltip", "Not Encrypt");
        icon = QIcon::fromTheme(QStringLiteral("document-decrypt"));
    }

    if (acceptedSolution) {
        auto overlay = QIcon::fromTheme(QStringLiteral("emblem-added"));
        if (state) {
            overlay = QIcon::fromTheme(QStringLiteral("emblem-checked"));
        }
        icon = KIconUtils::addOverlay(icon, overlay, Qt::BottomRightCorner);
    } else {
        if (state && setByUser) {
            auto overlay = QIcon::fromTheme(QStringLiteral("emblem-warning"));
            icon = KIconUtils::addOverlay(icon, overlay, Qt::BottomRightCorner);
        }
    }
    mEncryptAction->setIcon(icon);
    mEncryptAction->setToolTip(tooltip);
}

void KMComposerWin::runKeyResolver()
{
    const auto ident = identity();
    auto keyResolverCore = fillKeyResolver();
    auto result = keyResolverCore->resolve();

    QStringList autocryptKeys;
    QStringList gossipKeys;

    if (!(result.flags & Kleo::KeyResolverCore::AllResolved)) {
        if (result.flags & Kleo::KeyResolverCore::OpenPGPOnly && ident.autocryptEnabled()) {
            bool allResolved = true;
            const auto storage = MessageCore::AutocryptStorage::self();
            for (const auto &recipient : result.solution.encryptionKeys.keys()) {
                const auto key = result.solution.encryptionKeys[recipient];
                if (!key.empty()) { // There are already keys found
                    continue;
                }
                if (recipient == keyResolverCore->normalizedSender()) { // Don't care about own key as we show warnings in another way
                    continue;
                }
                const auto rec = storage->getRecipient(recipient.toUtf8());
                GpgME::Key autocryptKey;
                if (rec) {
                    const auto gpgKey = rec->gpgKey();
                    if (!gpgKey.isBad() && gpgKey.canEncrypt()) {
                        autocryptKey = gpgKey;
                    } else {
                        const auto gossipKey = rec->gossipKey();
                        if (!gossipKey.isBad() && gossipKey.canEncrypt()) {
                            gossipKeys.push_back(recipient);
                            autocryptKey = gossipKey;
                        }
                    }
                }
                if (!autocryptKey.isNull()) {
                    autocryptKeys.push_back(recipient);
                    result.solution.encryptionKeys[recipient].push_back(autocryptKey);
                } else {
                    allResolved = false;
                }
            }
            if (allResolved) {
                result.flags = Kleo::KeyResolverCore::SolutionFlags(result.flags | Kleo::KeyResolverCore::AllResolved);
            }
        }
    }

    const auto lst = mComposerBase->recipientsEditor()->lines();

    if (lst.size() == 1) {
        const auto line = qobject_cast<MessageComposer::RecipientLineNG *>(lst.first());
        if (line->recipientsCount() == 0) {
            mEncryptionState.setAcceptedSolution(false);
            return;
        }
    }

    mEncryptionState.setAcceptedSolution(result.flags & Kleo::KeyResolverCore::AllResolved);

    for (auto line_ : lst) {
        auto line = qobject_cast<MessageComposer::RecipientLineNG *>(line_);
        Q_ASSERT(line);
        auto recipient = line->data().dynamicCast<MessageComposer::Recipient>();

        QString dummy;
        QString addrSpec;
        if (KEmailAddress::splitAddress(recipient->email(), dummy, addrSpec, dummy) != KEmailAddress::AddressOk) {
            addrSpec = recipient->email();
        }

        auto resolvedKeys = result.solution.encryptionKeys[addrSpec];
        GpgME::Key key;
        if (resolvedKeys.size() == 0) { // no key found for recipient
            // Search for any key, also for not accepted ones, to at least give the user more info.
            key = Kleo::KeyCache::instance()->findBestByMailBox(addrSpec.toUtf8().constData(), GpgME::UnknownProtocol, Kleo::KeyCache::KeyUsage::Encrypt);
            key.update(); // We need tofu information for key.
            recipient->setKey(key);
        } else { // A key was found for recipient
            key = resolvedKeys.front();
            if (recipient->key().primaryFingerprint() != key.primaryFingerprint()) {
                key.update(); // We need tofu information for key.
                recipient->setKey(key);
            }
        }

        const bool autocryptKey = autocryptKeys.contains(addrSpec);
        const bool gossipKey = gossipKeys.contains(addrSpec);
        annotateRecipientEditorLineWithCryptoInfo(line, autocryptKey, gossipKey);

        if (!key.isNull()) {
            mComposerBase->expiryChecker()->checkKey(key, Kleo::ExpiryChecker::EncryptionKey);
        }
    }
}

void KMComposerWin::annotateRecipientEditorLineWithCryptoInfo(MessageComposer::RecipientLineNG *line, bool autocryptKey, bool gossipKey)
{
    auto recipient = line->data().dynamicCast<MessageComposer::Recipient>();
    const auto key = recipient->key();

    const auto showCryptoIndicator = KMailSettings::self()->showCryptoLabelIndicator();
    const auto hasOverride = mEncryptionState.hasOverride();
    const auto encrypt = mEncryptionState.encrypt();

    const bool showPositiveIcons = showCryptoIndicator && encrypt;
    const bool showAllIcons = showCryptoIndicator && hasOverride && encrypt;

    QString dummy;
    QString addrSpec;
    bool invalidEmail = false;
    if (KEmailAddress::splitAddress(recipient->email(), dummy, addrSpec, dummy) != KEmailAddress::AddressOk) {
        invalidEmail = true;
        addrSpec = recipient->email();
    }

    if (key.isNull()) {
        recipient->setEncryptionAction(Kleo::Impossible);
        if (showAllIcons && !invalidEmail) {
            const auto icon = QIcon::fromTheme(QStringLiteral("emblem-error"));
            line->setIcon(icon, i18nc("@info:tooltip", "No key found for the recipient."));
        } else {
            line->setIcon(QIcon());
        }
        line->setProperty("keyStatus", invalidEmail ? QVariant::fromValue(CryptoKeyState::InProgress) : QVariant::fromValue(CryptoKeyState::NoKey));
        return;
    }

    KMComposerWin::CryptoKeyState keyState = CryptoKeyState::KeyOk;

    if (recipient->encryptionAction() != Kleo::DoIt) {
        recipient->setEncryptionAction(Kleo::DoIt);
    }

    if (autocryptKey) { // We found an Autocrypt key for recipient
        QIcon icon = QIcon::fromTheme(QStringLiteral("emblem-success"));
        QString tooltip;
        const auto storage = MessageCore::AutocryptStorage::self();
        const auto rec = storage->getRecipient(addrSpec.toUtf8());
        if (gossipKey) { // We found an Autocrypt gossip key for recipient
            icon = QIcon::fromTheme(QStringLiteral("emblem-informations"));
            tooltip = i18nc("@info:tooltip",
                            "Autocrypt gossip key is used for this recipient. We got this key from 3rd party recipients. "
                            "This key is not verified.");
        } else if (rec->prefer_encrypt()) {
            tooltip = i18nc("@info:tooltip",
                            "Autocrypt key is used for this recipient. "
                            "This key is not verified. The recipient prefers encrypted replies.");
        } else {
            tooltip = i18nc("@info:tooltip",
                            "Autocrypt key is used for this recipient. "
                            "This key is not verified. The recipient does not prefer encrypted replies.");
        }
        if (showAllIcons) {
            line->setIcon(icon, tooltip);
        } else {
            line->setIcon(QIcon());
        }

    } else {
        QIcon icon;
        QString tooltip;

        const auto uids = key.userIDs();
        const auto _uid = findSendersUid(addrSpec.toStdString(), uids);
        GpgME::UserID uid;
        if (_uid == uids.cend()) {
            uid = key.userID(0);
        } else {
            uid = *_uid;
        }

        const auto trustLevel = Kleo::trustLevel(uid);
        switch (trustLevel) {
        case Kleo::Level0:
            if (uid.tofuInfo().isNull()) {
                tooltip = i18nc("@info:tooltip",
                                "The encryption key is not trusted. It hasn't enough validity. "
                                "You can sign the key, if you communicated the fingerprint by another channel. "
                                "Click the icon for details.");
                keyState = CryptoKeyState::NoKey;
            } else {
                switch (uid.tofuInfo().validity()) {
                case GpgME::TofuInfo::NoHistory:
                    tooltip = i18nc("@info:tooltip",
                                    "The encryption key is not trusted. "
                                    "It hasn't been used anywhere to guarantee it belongs to the stated person. "
                                    "By using the key will be trusted more. "
                                    "Or you can sign the key, if you communicated the fingerprint by another channel. "
                                    "Click the icon for details.");
                    break;
                case GpgME::TofuInfo::Conflict:
                    tooltip = i18nc("@info:tooltip",
                                    "The encryption key is not trusted. It has conflicting TOFU data. "
                                    "Click the icon for details.");
                    keyState = CryptoKeyState::NoKey;
                    break;
                case GpgME::TofuInfo::ValidityUnknown:
                    tooltip = i18nc("@info:tooltip",
                                    "The encryption key is not trusted. It has unknown validity in TOFU data. "
                                    "Click the icon for details.");
                    keyState = CryptoKeyState::NoKey;
                    break;
                default:
                    tooltip = i18nc("@info:tooltip",
                                    "The encryption key is not trusted. The key is marked as bad. "
                                    "Click the icon for details.");
                    keyState = CryptoKeyState::NoKey;
                }
            }
            break;
        case Kleo::Level1:
            tooltip = i18nc("@info:tooltip",
                            "The encryption key is only marginally trusted and hasn't been used enough time to guarantee it belongs to the stated person. "
                            "By using the key will be trusted more. "
                            "Or you can sign the key, if you communicated the fingerprint by another channel. "
                            "Click the icon for details.");
            break;
        case Kleo::Level2:
            if (uid.tofuInfo().isNull()) {
                tooltip = i18nc("@info:tooltip",
                                "The encryption key is only marginally trusted. "
                                "You can sign the key, if you communicated the fingerprint by another channel. "
                                "Click the icon for details.");
            } else {
                tooltip =
                    i18nc("@info:tooltip",
                          "The encryption key is only marginally trusted, but has been used enough times to be very likely controlled by the stated person. "
                          "By using the key will be trusted more. "
                          "Or you can sign the key, if you communicated the fingerprint by another channel. "
                          "Click the icon for details.");
            }
            break;
        case Kleo::Level3:
            tooltip = i18nc("@info:tooltip",
                            "The encryption key is fully trusted. You can raise the security level, by signing the key. "
                            "Click the icon for details.");
            break;
        case Kleo::Level4:
            tooltip = i18nc("@info:tooltip",
                            "The encryption key is ultimately trusted or is signed by another ultimately trusted key. "
                            "Click the icon for details.");
            break;
        default:
            Q_UNREACHABLE();
        }

        if (keyState == CryptoKeyState::NoKey) {
            mEncryptionState.setAcceptedSolution(false);
            if (showAllIcons) {
                line->setIcon(QIcon::fromTheme(QStringLiteral("emblem-error")), tooltip);
            } else {
                line->setIcon(QIcon());
            }
        } else if (trustLevel == Kleo::Level0 && encrypt) {
            line->setIcon(QIcon::fromTheme(QStringLiteral("emblem-warning")), tooltip);
        } else if (showPositiveIcons) {
            // Magically, the icon name maps precisely to each trust level
            // line->setIcon(QIcon::fromTheme(QStringLiteral("gpg-key-trust-level-%1").arg(trustLevel)), tooltip);
            line->setIcon(QIcon::fromTheme(QStringLiteral("emblem-success")), tooltip);
        } else {
            line->setIcon(QIcon());
        }
    }

    if (line->property("keyStatus").value<CryptoKeyState>() != keyState) {
        line->setProperty("keyStatus", QVariant::fromValue(keyState));
    }
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
    applyMainWindowSettings(stateConfigGroup());
}

void KMComposerWin::slotEditKeys()
{
    KShortcutsDialog::showDialog(actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this);
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
    KMailSettings::self()->setCompletionMode(static_cast<int>(mode));

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
        const QString text =
            mComposerBase->editor()->composerControler()->currentLinkText() + " -> "_L1 + mComposerBase->editor()->composerControler()->currentLinkUrl();
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
    mCryptoStateIndicatorWidget->updateSignatureAndEncrypionStateIndicators(sign(), mEncryptionState.encrypt());
    if (sign() || mEncryptionState.encrypt()) {
        mComposerBase->editor()->setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::TopEdge}));
        mComposerBase->editor()->setProperty("_breeze_force_frame", true);
    } else {
        mComposerBase->editor()->setProperty("_breeze_borders_sides", QVariant{});
        mComposerBase->editor()->setProperty("_breeze_force_frame", false);
    }
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
    auto job = new SaveAsFileJob(this);
    job->setParentWidget(this);
    job->setHtmlMode(mComposerBase->editor()->textMode() == MessageComposer::RichTextComposerNg::Rich);
    job->setTextDocument(mComposerBase->editor()->document());
    job->start();
    // not necessary to delete it. It done in SaveAsFileJob
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
    createExternalEditorWarning();
    mComposerBase->identityCombo()->setEnabled(false);
    mExternalEditorWarning->show();
}

void KMComposerWin::slotExternalEditorClosed()
{
    createExternalEditorWarning();
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
    return mPluginEditorManagerInterface->actionsType(MessageComposer::PluginActionType::PopupMenu)
        + mPluginEditorConvertTextManagerInterface->actionsType(MessageComposer::PluginActionType::PopupMenu);
}

void KMComposerWin::slotRecipientEditorLineAdded(KPIM::MultiplyingLine *line_)
{
    auto line = qobject_cast<MessageComposer::RecipientLineNG *>(line_);
    Q_ASSERT(line);

    connect(line, &MessageComposer::RecipientLineNG::countChanged, this, [this, line]() {
        slotRecipientAdded(line);
    });
    connect(line, &MessageComposer::RecipientLineNG::iconClicked, this, [this, line]() {
        slotRecipientLineIconClicked(line);
    });
    connect(line, &MessageComposer::RecipientLineNG::destroyed, this, &KMComposerWin::slotRecipientEditorFocusChanged, Qt::QueuedConnection);
    connect(
        line,
        &MessageComposer::RecipientLineNG::activeChanged,
        this,
        [this, line]() {
            slotRecipientFocusLost(line);
        },
        Qt::QueuedConnection);

    slotRecipientEditorFocusChanged();
}

bool KMComposerWin::sign() const
{
    return mSignAction->isChecked();
}

void KMComposerWin::slotRecipientEditorFocusChanged()
{
    if (!mEncryptionState.possibleEncrypt()) {
        return;
    }

    if (mKeyCache->initialized()) {
        mRunKeyResolverTimer->stop();
        runKeyResolver();
    }
}

void KMComposerWin::slotRecipientLineIconClicked(MessageComposer::RecipientLineNG *line)
{
    const auto recipient = line->data().dynamicCast<MessageComposer::Recipient>();

    if (!recipient->key().isNull()) {
        const QString exec = TextAddonsWidgets::ExecutableUtils::findExecutable(QStringLiteral("kleopatra"));
        if (exec.isEmpty()
            || !QProcess::startDetached(exec,
                                        {QStringLiteral("--query"),
                                         QString::fromLatin1(recipient->key().primaryFingerprint()),
                                         QStringLiteral("--parent-windowid"),
                                         QString::number(winId())})) {
            qCWarning(KMAIL_LOG) << "Unable to execute kleopatra";
        }
        return;
    }

    const auto msg = i18nc(
        "if in your language something like "
        "'certificate(s)' is not possible please "
        "use the plural in the translation",
        "<qt>No valid and trusted encryption certificate was "
        "found for \"%1\".<br/><br/>"
        "Select the certificate(s) which should "
        "be used for this recipient. If there is no suitable certificate in the list "
        "you can also search for external certificates by clicking the button: "
        "search for external certificates.</qt>",
        recipient->name().isEmpty() ? recipient->email() : recipient->name());

    const bool opgp = containsOpenPGP(cryptoMessageFormat());
    const bool x509 = containsSMIME(cryptoMessageFormat());

    QPointer<Kleo::KeySelectionDialog> dlg = new Kleo::KeySelectionDialog(
        i18n("Encryption Key Selection"),
        msg,
        recipient->email(),
        {},
        Kleo::KeySelectionDialog::ValidEncryptionKeys | (opgp ? Kleo::KeySelectionDialog::OpenPGPKeys : 0) | (x509 ? Kleo::KeySelectionDialog::SMIMEKeys : 0),
        false, // multi-selection
        false); // "remember choice" box;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->open();

    connect(dlg, &QDialog::accepted, this, [dlg, recipient, line, this]() {
        auto key = dlg->selectedKey();
        key.update(); // We need tofu information for key.
        recipient->setKey(key);
        annotateRecipientEditorLineWithCryptoInfo(line, false, false);
    });
}

void KMComposerWin::slotRecipientAdded(MessageComposer::RecipientLineNG *line)
{
    // Encryption is possible not possible to find encryption keys.
    if (!mEncryptionState.possibleEncrypt()) {
        return;
    }

    if (line->recipientsCount() == 0) {
        return;
    }

    if (!mKeyCache->initialized()) {
        if (line->property("keyLookupJob").toBool()) {
            return;
        }

        line->setProperty("keyLookupJob", true);
        // We need to start key listing on our own othweise KMail will crash and we want to wait till the cache is populated.
        connect(mKeyCache.get(), &Kleo::KeyCache::keyListingDone, this, [this, line]() {
            slotRecipientAdded(line);
        });
        return;
    }

    if (mKeyCache->initialized()) {
        mRunKeyResolverTimer->start();
    }
}

void KMComposerWin::slotRecipientFocusLost(MessageComposer::RecipientLineNG *line)
{
    // Not possible to find encryption keys.
    if (!mEncryptionState.possibleEncrypt()) {
        return;
    }

    if (line->recipientsCount() == 0) {
        return;
    }

    if (mKeyCache->initialized()) {
        mRunKeyResolverTimer->start();
    }
}

void KMComposerWin::slotIdentityDeleted(uint uoid)
{
    if (currentIdentity() == uoid) {
        mIncorrectIdentityFolderWarning->identityInvalid();
    }
}

void KMComposerWin::slotTransportRemoved(int id, [[maybe_unused]] const QString &name)
{
    if (mComposerBase->transportComboBox()->currentTransportId() == id) {
        mIncorrectIdentityFolderWarning->mailTransportIsInvalid();
    }
}

void KMComposerWin::slotSelectionChanged()
{
    Q_EMIT mPluginEditorManagerInterface->textSelectionChanged(mRichTextEditorWidget->editor()->textCursor().hasSelection());
}

void KMComposerWin::slotMessage(const QString &str)
{
    KMessageBox::information(this, str, i18nc("@title:window", "Plugin Editor Information"));
}

void KMComposerWin::slotEditorPluginInsertText(const QString &str)
{
    mGlobalAction->slotInsertText(str);
}

#include "moc_kmcomposerwin.cpp"
