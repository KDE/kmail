/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "configuresecuritypage.h"
#include <PimCommon/ConfigureImmutableWidgetUtils>
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "settings/kmailsettings.h"
#include <MailCommon/FolderSettings>
#include <MessageComposer/MessageComposerSettings>
#include <MessageViewer/MessageViewerSettings>
#include <MessageViewer/RemoteContentConfigureDialog>
#include <WebEngineViewer/CheckPhishingUrlCache>

#include "kmkernel.h"

#include <Libkleo/KeyRequester>
#include <Libkleo/KeySelectionDialog>
#include <QGpgME/CryptoConfig>
#include <QGpgME/Protocol>

#include "kcmutils_version.h"
#include "kmail_debug.h"
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginMetaData>

#include <QButtonGroup>
#include <QDBusConnection>
#include <QPointer>
#include <QWhatsThis>

QString SecurityPage::helpAnchor() const
{
    return QStringLiteral("configure-security");
}

SecurityPage::SecurityPage(QWidget *parent, const QVariantList &args)
    : ConfigModuleWithTabs(parent, args)
{
    //
    // "Reading" tab:
    //
    auto generalTab = new ReadingTab();
    addTab(generalTab, i18n("Reading"));

    addTab(new MDNTab(), i18n("Message Disposition Notifications"));

    //
    // "Composing" tab:
    //
    auto composerCryptoTab = new ComposerCryptoTab();
    addTab(composerCryptoTab, i18n("Composing"));

    //
    // "Warnings" tab:
    //
    auto warningTab = new WarningTab();
    addTab(warningTab, i18n("Miscellaneous"));

    //
    // "S/MIME Validation" tab:
    //
    auto sMimeTab = new SMimeTab();
    addTab(sMimeTab, i18n("S/MIME Validation"));
}

QString SecurityPage::ReadingTab::helpAnchor() const
{
    return QStringLiteral("configure-security-reading");
}

SecurityPageGeneralTab::SecurityPageGeneralTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mSGTab.setupUi(this);

    connect(mSGTab.mConfigureExternalReference, &QPushButton::clicked, this, &SecurityPageGeneralTab::slotOpenExternalReferenceExceptions);
    connect(mSGTab.mHtmlMailCheck, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);
    connect(mSGTab.mExternalReferences, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);
    connect(mSGTab.labelWarnHTML, &QLabel::linkActivated, this, &SecurityPageGeneralTab::slotLinkClicked);

    connect(mSGTab.mAlwaysDecrypt, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);

    connect(mSGTab.mAutomaticallyImportAttachedKeysCheck, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);

    connect(mSGTab.mScamDetection, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);

    connect(mSGTab.mCheckUrl, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);
    connect(mSGTab.mCheckMailUrlTracking, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);
    connect(mSGTab.scamWhiteList, &PimCommon::SimpleStringListEditor::changed, this, &ConfigModuleTab::slotEmitChanged);
    mSGTab.scamWhiteList->setAddDialogLabel(i18n("Email Address:"));
    mSGTab.scamWhiteList->setRemoveDialogLabel(i18n("Do you want to remove this email address?"));
}

void SecurityPageGeneralTab::slotOpenExternalReferenceExceptions()
{
    QPointer<MessageViewer::RemoteContentConfigureDialog> dlg = new MessageViewer::RemoteContentConfigureDialog(this);
    dlg->exec();
    delete dlg;
}

void SecurityPageGeneralTab::slotLinkClicked(const QString &link)
{
    if (link == QLatin1String("whatsthis1")) {
        QWhatsThis::showText(QCursor::pos(), mSGTab.mHtmlMailCheck->whatsThis());
    } else if (link == QLatin1String("whatsthis2")) {
        QWhatsThis::showText(QCursor::pos(), mSGTab.mExternalReferences->whatsThis());
    }
}

void SecurityPage::ReadingTab::doLoadOther()
{
    loadWidget(mSGTab.mHtmlMailCheck, MessageViewer::MessageViewerSettings::self()->htmlMailItem());
    loadWidget(mSGTab.mExternalReferences, MessageViewer::MessageViewerSettings::self()->htmlLoadExternalItem());
    loadWidget(mSGTab.mAutomaticallyImportAttachedKeysCheck, MessageViewer::MessageViewerSettings::self()->autoImportKeysItem());
    loadWidget(mSGTab.mAlwaysDecrypt, MessageViewer::MessageViewerSettings::self()->alwaysDecryptItem());

    loadWidget(mSGTab.mScamDetection, MessageViewer::MessageViewerSettings::self()->scamDetectionEnabledItem());
    loadWidget(mSGTab.scamWhiteList, MessageViewer::MessageViewerSettings::self()->scamDetectionWhiteListItem());
    loadWidget(mSGTab.mCheckUrl, MessageViewer::MessageViewerSettings::self()->checkPhishingUrlItem());
    loadWidget(mSGTab.mCheckMailUrlTracking, MessageViewer::MessageViewerSettings::self()->mailTrackingUrlEnabledItem());
}

void SecurityPage::ReadingTab::save()
{
    if (MessageViewer::MessageViewerSettings::self()->htmlMail() != mSGTab.mHtmlMailCheck->isChecked()) {
        if (KMessageBox::warningContinueCancel(this,
                                               i18n("Changing the global "
                                                    "HTML setting will override all folder specific values."),
                                               QString(),
                                               KStandardGuiItem::cont(),
                                               KStandardGuiItem::cancel(),
                                               QStringLiteral("htmlMailOverride"))
            == KMessageBox::Continue) {
            saveCheckBox(mSGTab.mHtmlMailCheck, MessageViewer::MessageViewerSettings::self()->htmlMailItem());
            if (kmkernel) {
                const auto allFolders = kmkernel->allFolders();
                for (const Akonadi::Collection &collection : allFolders) {
                    KConfigGroup config(KMKernel::self()->config(), MailCommon::FolderSettings::configGroupName(collection));
                    // Old config
                    config.deleteEntry("htmlMailOverride");
                    config.deleteEntry("displayFormatOverride");
                    MailCommon::FolderSettings::resetHtmlFormat();
                }
            }
        }
    }
    saveCheckBox(mSGTab.mExternalReferences, MessageViewer::MessageViewerSettings::self()->htmlLoadExternalItem());

    saveCheckBox(mSGTab.mAutomaticallyImportAttachedKeysCheck, MessageViewer::MessageViewerSettings::self()->autoImportKeysItem());
    saveCheckBox(mSGTab.mAlwaysDecrypt, MessageViewer::MessageViewerSettings::self()->alwaysDecryptItem());
    saveCheckBox(mSGTab.mScamDetection, MessageViewer::MessageViewerSettings::self()->scamDetectionEnabledItem());
    saveSimpleStringListEditor(mSGTab.scamWhiteList, MessageViewer::MessageViewerSettings::self()->scamDetectionWhiteListItem());
    saveCheckBox(mSGTab.mCheckUrl, MessageViewer::MessageViewerSettings::self()->checkPhishingUrlItem());
    saveCheckBox(mSGTab.mCheckMailUrlTracking, MessageViewer::MessageViewerSettings::self()->mailTrackingUrlEnabledItem());
    if (!mSGTab.mCheckUrl->isChecked()) {
        WebEngineViewer::CheckPhishingUrlCache::self()->clearCache();
    }
}

QString SecurityPage::MDNTab::helpAnchor() const
{
    return QStringLiteral("configure-security-mdn");
}

SecurityPageMDNTab::SecurityPageMDNTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mUi.setupUi(this);

    // "ignore", "ask", "deny", "always send" radiobuttons
    mMDNGroup = new QButtonGroup(this);
    connect(mMDNGroup, qOverload<QAbstractButton *>(&QButtonGroup::buttonClicked), this, &SecurityPageMDNTab::slotEmitChanged);
    mMDNGroup->addButton(mUi.radioIgnore, 0);
    mMDNGroup->addButton(mUi.radioAsk, 1);
    mMDNGroup->addButton(mUi.radioDeny, 2);
    mMDNGroup->addButton(mUi.radioAlways, 3);

    // "Original Message quote" radiobuttons
    mOrigQuoteGroup = new QButtonGroup(this);
    connect(mOrigQuoteGroup, qOverload<QAbstractButton *>(&QButtonGroup::buttonClicked), this, &SecurityPageMDNTab::slotEmitChanged);
    mOrigQuoteGroup->addButton(mUi.radioNothing, 0);
    mOrigQuoteGroup->addButton(mUi.radioFull, 1);
    mOrigQuoteGroup->addButton(mUi.radioHeaders, 2);

    connect(mUi.mNoMDNsWhenEncryptedCheck, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);
    connect(mUi.labelWarning, &QLabel::linkActivated, this, &SecurityPageMDNTab::slotLinkClicked);
}

void SecurityPageMDNTab::slotLinkClicked(const QString &link)
{
    if (link == QLatin1String("whatsthis-mdn")) {
        QWhatsThis::showText(QCursor::pos(), mUi.radioIgnore->whatsThis());
    }
}

void SecurityPage::MDNTab::doLoadOther()
{
    int num = MessageViewer::MessageViewerSettings::self()->defaultPolicy();
    if (num < 0 || num >= mMDNGroup->buttons().count()) {
        num = 0;
    }
    mMDNGroup->button(num)->setChecked(true);
    num = MessageViewer::MessageViewerSettings::self()->quoteMessage();
    if (num < 0 || num >= mOrigQuoteGroup->buttons().count()) {
        num = 0;
    }
    mOrigQuoteGroup->button(num)->setChecked(true);
    loadWidget(mUi.mNoMDNsWhenEncryptedCheck, MessageViewer::MessageViewerSettings::self()->notSendWhenEncryptedItem());
}

void SecurityPage::MDNTab::save()
{
    MessageViewer::MessageViewerSettings::self()->setDefaultPolicy(mMDNGroup->checkedId());
    MessageViewer::MessageViewerSettings::self()->setQuoteMessage(mOrigQuoteGroup->checkedId());
    saveCheckBox(mUi.mNoMDNsWhenEncryptedCheck, MessageViewer::MessageViewerSettings::self()->notSendWhenEncryptedItem());
}

QString SecurityPage::ComposerCryptoTab::helpAnchor() const
{
    return QStringLiteral("configure-security-composing");
}

SecurityPageComposerCryptoTab::SecurityPageComposerCryptoTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mWidget = new Ui::ComposerCryptoConfiguration;
    mWidget->setupUi(this);
    connect(mWidget->mEncToSelf, &QCheckBox::toggled, this, &SecurityPageComposerCryptoTab::slotEmitChanged);
    connect(mWidget->mShowKeyApprovalDlg, &QCheckBox::toggled, this, &SecurityPageComposerCryptoTab::slotEmitChanged);
    connect(mWidget->mAlwaysEncryptWhenSavingInDrafts, &QCheckBox::toggled, this, &SecurityPageComposerCryptoTab::slotEmitChanged);
    connect(mWidget->mStoreEncrypted, &QCheckBox::toggled, this, &SecurityPageComposerCryptoTab::slotEmitChanged);
    connect(mWidget->mShowEncSignIndicator, &QCheckBox::toggled, this, &SecurityPageComposerCryptoTab::slotEmitChanged);
}

SecurityPageComposerCryptoTab::~SecurityPageComposerCryptoTab()
{
    delete mWidget;
}

void SecurityPage::ComposerCryptoTab::doLoadOther()
{
    // If you change default values, sync messagecomposer.cpp too

    loadWidget(mWidget->mEncToSelf, MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelfItem());
    loadWidget(mWidget->mShowKeyApprovalDlg, MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApprovalItem());

    loadWidget(mWidget->mAlwaysEncryptWhenSavingInDrafts, KMailSettings::self()->alwaysEncryptDraftsItem());

    loadWidget(mWidget->mStoreEncrypted, KMailSettings::self()->cryptoStoreEncryptedItem());
    loadWidget(mWidget->mShowEncSignIndicator, KMailSettings::self()->showCryptoLabelIndicatorItem());
}

void SecurityPage::ComposerCryptoTab::save()
{
    saveCheckBox(mWidget->mEncToSelf, MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelfItem());
    saveCheckBox(mWidget->mShowKeyApprovalDlg, MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApprovalItem());

    saveCheckBox(mWidget->mAlwaysEncryptWhenSavingInDrafts, KMailSettings::self()->alwaysEncryptDraftsItem());
    saveCheckBox(mWidget->mStoreEncrypted, KMailSettings::self()->cryptoStoreEncryptedItem());
    saveCheckBox(mWidget->mShowEncSignIndicator, KMailSettings::self()->showCryptoLabelIndicatorItem());
}

void SecurityPage::ComposerCryptoTab::doLoadFromGlobalSettings()
{
    loadWidget(mWidget->mEncToSelf, MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelfItem());
    loadWidget(mWidget->mShowKeyApprovalDlg, MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApprovalItem());

    loadWidget(mWidget->mAlwaysEncryptWhenSavingInDrafts, KMailSettings::self()->alwaysEncryptDraftsItem());
    loadWidget(mWidget->mStoreEncrypted, KMailSettings::self()->cryptoStoreEncryptedItem());
    loadWidget(mWidget->mShowEncSignIndicator, KMailSettings::self()->showCryptoLabelIndicatorItem());
}

QString SecurityPage::WarningTab::helpAnchor() const
{
    return QStringLiteral("configure-security-warnings");
}

SecurityPageWarningTab::SecurityPageWarningTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mWidget = new Ui::WarningConfiguration;
    mWidget->setupUi(this);

    connect(mWidget->warnGroupBox, &QGroupBox::toggled, this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnUnsigned, &QCheckBox::toggled, this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->warnUnencryptedCB, &QCheckBox::toggled, this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnSignKeyExpiresSB, qOverload<int>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnEncrKeyExpiresSB, qOverload<int>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnEncrChainCertExpiresSB, qOverload<int>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnSignChainCertExpiresSB, qOverload<int>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnSignRootCertExpiresSB, qOverload<int>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnEncrRootCertExpiresSB, qOverload<int>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);

    connect(mWidget->gnupgButton, &QPushButton::clicked, this, &SecurityPageWarningTab::slotConfigureGnupg);
    connect(mWidget->enableAllWarningsPB, &QPushButton::clicked, this, &SecurityPageWarningTab::slotReenableAllWarningsClicked);
}

SecurityPageWarningTab::~SecurityPageWarningTab()
{
    delete mWidget;
}

void SecurityPage::WarningTab::doLoadFromGlobalSettings()
{
    loadWidget(mWidget->warnUnencryptedCB, MessageComposer::MessageComposerSettings::self()->cryptoWarningUnencryptedItem());
    loadWidget(mWidget->mWarnUnsigned, MessageComposer::MessageComposerSettings::self()->cryptoWarningUnsignedItem());

    // The "-int" part of the key name is because there used to be a separate boolean
    // config entry for enabling/disabling. This is done with the single bool value now.
    mWidget->warnGroupBox->setChecked(MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire());

    loadWidget(mWidget->mWarnSignKeyExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignKeyNearExpiryThresholdDaysItem());
    loadWidget(mWidget->mWarnSignChainCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignChaincertNearExpiryThresholdDaysItem());
    loadWidget(mWidget->mWarnSignRootCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignRootNearExpiryThresholdDaysItem());
    loadWidget(mWidget->mWarnEncrKeyExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrKeyNearExpiryThresholdDaysItem());
    loadWidget(mWidget->mWarnEncrChainCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrChaincertNearExpiryThresholdDaysItem());
    loadWidget(mWidget->mWarnEncrRootCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrRootNearExpiryThresholdDaysItem());
}

void SecurityPage::WarningTab::doLoadOther()
{
    loadWidget(mWidget->warnUnencryptedCB, MessageComposer::MessageComposerSettings::self()->cryptoWarningUnencryptedItem());
    loadWidget(mWidget->mWarnUnsigned, MessageComposer::MessageComposerSettings::self()->cryptoWarningUnsignedItem());

    // The "-int" part of the key name is because there used to be a separate boolean
    // config entry for enabling/disabling. This is done with the single bool value now.
    mWidget->warnGroupBox->setChecked(MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire());

    loadWidget(mWidget->mWarnSignKeyExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignKeyNearExpiryThresholdDaysItem());
    mWidget->mWarnSignKeyExpiresSB->setSuffix(ki18np(" day", " days"));

    loadWidget(mWidget->mWarnSignChainCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignChaincertNearExpiryThresholdDaysItem());
    mWidget->mWarnSignChainCertExpiresSB->setSuffix(ki18np(" day", " days"));
    loadWidget(mWidget->mWarnSignRootCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignRootNearExpiryThresholdDaysItem());
    mWidget->mWarnSignRootCertExpiresSB->setSuffix(ki18np(" day", " days"));
    loadWidget(mWidget->mWarnEncrKeyExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrKeyNearExpiryThresholdDaysItem());
    mWidget->mWarnEncrKeyExpiresSB->setSuffix(ki18np(" day", " days"));

    loadWidget(mWidget->mWarnEncrChainCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrChaincertNearExpiryThresholdDaysItem());
    mWidget->mWarnEncrChainCertExpiresSB->setSuffix(ki18np(" day", " days"));
    loadWidget(mWidget->mWarnEncrRootCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrRootNearExpiryThresholdDaysItem());
    mWidget->mWarnEncrRootCertExpiresSB->setSuffix(ki18np(" day", " days"));

    mWidget->enableAllWarningsPB->setEnabled(true);
}

void SecurityPage::WarningTab::save()
{
    saveCheckBox(mWidget->warnUnencryptedCB, MessageComposer::MessageComposerSettings::self()->cryptoWarningUnencryptedItem());
    saveCheckBox(mWidget->mWarnUnsigned, MessageComposer::MessageComposerSettings::self()->cryptoWarningUnsignedItem());

    MessageComposer::MessageComposerSettings::self()->setCryptoWarnWhenNearExpire(mWidget->warnGroupBox->isChecked());

    saveSpinBox(mWidget->mWarnSignKeyExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignKeyNearExpiryThresholdDaysItem());
    saveSpinBox(mWidget->mWarnSignChainCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignChaincertNearExpiryThresholdDaysItem());
    saveSpinBox(mWidget->mWarnSignRootCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnSignRootNearExpiryThresholdDaysItem());
    saveSpinBox(mWidget->mWarnEncrKeyExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrKeyNearExpiryThresholdDaysItem());
    saveSpinBox(mWidget->mWarnEncrChainCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrChaincertNearExpiryThresholdDaysItem());
    saveSpinBox(mWidget->mWarnEncrRootCertExpiresSB, MessageComposer::MessageComposerSettings::self()->cryptoWarnEncrRootNearExpiryThresholdDaysItem());
}

void SecurityPage::WarningTab::slotReenableAllWarningsClicked()
{
    KMessageBox::enableAllMessages();
    mWidget->enableAllWarningsPB->setEnabled(false);
}

void SecurityPage::WarningTab::slotConfigureGnupg()
{
    QPointer<GpgSettingsDialog> dlg(new GpgSettingsDialog(this));
    KPageWidgetItem *page = nullptr;
#if KCMUTILS_VERSION >= QT_VERSION_CHECK(5, 84, 0)
    const auto plugin = KPluginMetaData::findPluginById(QStringLiteral("pim/kcms/kleopatra"), QStringLiteral("kleopatra_config_gnupgsystem"));
    if (plugin.isValid()) {
        page = dlg->addModule(plugin);
    }
#else
    page = dlg->addModule(QStringLiteral("kleopatra_config_gnupgsystem"));
#endif
    if (!page) {
        auto info = new QLabel(i18n("The module is missing. Please verify your installation. This module is provided by Kleopatra."), this);
        QFont font = info->font();
        font.setBold(true);
        info->setFont(font);
        info->setWordWrap(true);
        dlg->addPage(info, i18n("GnuPG Configure Module Error"));
    }
    dlg->exec();
    delete dlg;
}

QString SecurityPage::SMimeTab::helpAnchor() const
{
    return QStringLiteral("configure-security-smime-validation");
}

SecurityPageSMimeTab::SecurityPageSMimeTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mWidget = new Ui::SMimeConfiguration;
    mWidget->setupUi(this);

    // Button-group for exclusive radiobuttons
    auto bg = new QButtonGroup(this);
    bg->addButton(mWidget->CRLRB);
    bg->addButton(mWidget->OCSPRB);

    // Settings for the keyrequester custom widget
    mWidget->OCSPResponderSignature->setAllowedKeys(Kleo::KeySelectionDialog::SMIMEKeys | Kleo::KeySelectionDialog::TrustedKeys
                                                    | Kleo::KeySelectionDialog::ValidKeys | Kleo::KeySelectionDialog::SigningKeys
                                                    | Kleo::KeySelectionDialog::PublicKeys);
    mWidget->OCSPResponderSignature->setMultipleKeysEnabled(false);

    mConfig = QGpgME::cryptoConfig();

    connect(mWidget->CRLRB, &QRadioButton::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->OCSPRB, &QRadioButton::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->OCSPResponderURL, &QLineEdit::textChanged, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->OCSPResponderSignature, &Kleo::KeyRequester::changed, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->doNotCheckCertPolicyCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->neverConsultCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->fetchMissingCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);

    connect(mWidget->ignoreServiceURLCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->ignoreHTTPDPCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->disableHTTPCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->honorHTTPProxyRB, &QRadioButton::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->useCustomHTTPProxyRB, &QRadioButton::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->customHTTPProxy, &QLineEdit::textChanged, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->ignoreLDAPDPCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->disableLDAPCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotEmitChanged);
    connect(mWidget->customLDAPProxy, &QLineEdit::textChanged, this, &SecurityPageSMimeTab::slotEmitChanged);

    connect(mWidget->disableHTTPCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotUpdateHTTPActions);
    connect(mWidget->ignoreHTTPDPCB, &QCheckBox::toggled, this, &SecurityPageSMimeTab::slotUpdateHTTPActions);

    // Button-group for exclusive radiobuttons
    auto bgHTTPProxy = new QButtonGroup(this);
    bgHTTPProxy->addButton(mWidget->honorHTTPProxyRB);
    bgHTTPProxy->addButton(mWidget->useCustomHTTPProxyRB);

    QDBusConnection::sessionBus().connect(QString(), QString(), QStringLiteral("org.kde.kleo.CryptoConfig"), QStringLiteral("changed"), this, SLOT(load()));
}

SecurityPageSMimeTab::~SecurityPageSMimeTab()
{
    delete mWidget;
}

static void disableDirmngrWidget(QWidget *w)
{
    w->setEnabled(false);
    w->setWhatsThis(i18n("This option requires dirmngr >= 0.9.0"));
}

static void initializeDirmngrCheckbox(QCheckBox *cb, QGpgME::CryptoConfigEntry *entry)
{
    if (entry) {
        cb->setChecked(entry->boolValue());
    } else {
        disableDirmngrWidget(cb);
    }
}

struct SMIMECryptoConfigEntries {
    explicit SMIMECryptoConfigEntries(QGpgME::CryptoConfig *config)
        : mConfig(config)
    {
        // Checkboxes
        mCheckUsingOCSPConfigEntry =
            configEntry(QStringLiteral("gpgsm"), QStringLiteral("Security"), QStringLiteral("enable-ocsp"), QGpgME::CryptoConfigEntry::ArgType_None, false);
        mEnableOCSPsendingConfigEntry =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("OCSP"), QStringLiteral("allow-ocsp"), QGpgME::CryptoConfigEntry::ArgType_None, false);
        mDoNotCheckCertPolicyConfigEntry = configEntry(QStringLiteral("gpgsm"),
                                                       QStringLiteral("Security"),
                                                       QStringLiteral("disable-policy-checks"),
                                                       QGpgME::CryptoConfigEntry::ArgType_None,
                                                       false);
        mNeverConsultConfigEntry = configEntry(QStringLiteral("gpgsm"),
                                               QStringLiteral("Security"),
                                               QStringLiteral("disable-crl-checks"),
                                               QGpgME::CryptoConfigEntry::ArgType_None,
                                               false);
        mFetchMissingConfigEntry = configEntry(QStringLiteral("gpgsm"),
                                               QStringLiteral("Security"),
                                               QStringLiteral("auto-issuer-key-retrieve"),
                                               QGpgME::CryptoConfigEntry::ArgType_None,
                                               false);
        // dirmngr-0.9.0 options
        mIgnoreServiceURLEntry = configEntry(QStringLiteral("dirmngr"),
                                             QStringLiteral("OCSP"),
                                             QStringLiteral("ignore-ocsp-service-url"),
                                             QGpgME::CryptoConfigEntry::ArgType_None,
                                             false);
        mIgnoreHTTPDPEntry =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("HTTP"), QStringLiteral("ignore-http-dp"), QGpgME::CryptoConfigEntry::ArgType_None, false);
        mDisableHTTPEntry =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("HTTP"), QStringLiteral("disable-http"), QGpgME::CryptoConfigEntry::ArgType_None, false);
        mHonorHTTPProxy =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("HTTP"), QStringLiteral("honor-http-proxy"), QGpgME::CryptoConfigEntry::ArgType_None, false);

        mIgnoreLDAPDPEntry =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("LDAP"), QStringLiteral("ignore-ldap-dp"), QGpgME::CryptoConfigEntry::ArgType_None, false);
        mDisableLDAPEntry =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("LDAP"), QStringLiteral("disable-ldap"), QGpgME::CryptoConfigEntry::ArgType_None, false);
        // Other widgets
        mOCSPResponderURLConfigEntry =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("OCSP"), QStringLiteral("ocsp-responder"), QGpgME::CryptoConfigEntry::ArgType_String, false);
        mOCSPResponderSignature =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("OCSP"), QStringLiteral("ocsp-signer"), QGpgME::CryptoConfigEntry::ArgType_String, false);
        mCustomHTTPProxy =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("HTTP"), QStringLiteral("http-proxy"), QGpgME::CryptoConfigEntry::ArgType_String, false);
        mCustomLDAPProxy =
            configEntry(QStringLiteral("dirmngr"), QStringLiteral("LDAP"), QStringLiteral("ldap-proxy"), QGpgME::CryptoConfigEntry::ArgType_String, false);
    }

    QGpgME::CryptoConfigEntry *configEntry(const QString &componentName, const QString &groupName, const QString &entryName, int argType, bool isList);

    // Checkboxes
    QGpgME::CryptoConfigEntry *mCheckUsingOCSPConfigEntry = nullptr;
    QGpgME::CryptoConfigEntry *mEnableOCSPsendingConfigEntry = nullptr;
    QGpgME::CryptoConfigEntry *mDoNotCheckCertPolicyConfigEntry = nullptr;
    QGpgME::CryptoConfigEntry *mNeverConsultConfigEntry = nullptr;
    QGpgME::CryptoConfigEntry *mFetchMissingConfigEntry = nullptr;
    QGpgME::CryptoConfigEntry *mIgnoreServiceURLEntry = nullptr;
    QGpgME::CryptoConfigEntry *mIgnoreHTTPDPEntry = nullptr;
    QGpgME::CryptoConfigEntry *mDisableHTTPEntry = nullptr;
    QGpgME::CryptoConfigEntry *mHonorHTTPProxy = nullptr;
    QGpgME::CryptoConfigEntry *mIgnoreLDAPDPEntry = nullptr;
    QGpgME::CryptoConfigEntry *mDisableLDAPEntry = nullptr;
    // Other widgets
    QGpgME::CryptoConfigEntry *mOCSPResponderURLConfigEntry = nullptr;
    QGpgME::CryptoConfigEntry *mOCSPResponderSignature = nullptr;
    QGpgME::CryptoConfigEntry *mCustomHTTPProxy = nullptr;
    QGpgME::CryptoConfigEntry *mCustomLDAPProxy = nullptr;

    QGpgME::CryptoConfig *mConfig = nullptr;
};

void SecurityPage::SMimeTab::doLoadOther()
{
    if (!mConfig) {
        setEnabled(false);
        return;
    }

    // Force re-parsing gpgconf data, in case e.g. kleopatra or "configure backend" was used
    // (which ends up calling us via D-Bus)
    mConfig->clear();

    // Create config entries
    // Don't keep them around, they'll get deleted by clear(), which could be
    // done by the "configure backend" button even before we save().
    SMIMECryptoConfigEntries e(mConfig);

    // Initialize GUI items from the config entries

    if (e.mCheckUsingOCSPConfigEntry) {
        bool b = e.mCheckUsingOCSPConfigEntry->boolValue();
        mWidget->OCSPRB->setChecked(b);
        mWidget->CRLRB->setChecked(!b);
        mWidget->OCSPGroupBox->setEnabled(b);
    } else {
        mWidget->OCSPGroupBox->setEnabled(false);
    }
    if (e.mDoNotCheckCertPolicyConfigEntry) {
        mWidget->doNotCheckCertPolicyCB->setChecked(e.mDoNotCheckCertPolicyConfigEntry->boolValue());
    }
    if (e.mNeverConsultConfigEntry) {
        mWidget->neverConsultCB->setChecked(e.mNeverConsultConfigEntry->boolValue());
    }
    if (e.mFetchMissingConfigEntry) {
        mWidget->fetchMissingCB->setChecked(e.mFetchMissingConfigEntry->boolValue());
    }

    if (e.mOCSPResponderURLConfigEntry) {
        mWidget->OCSPResponderURL->setText(e.mOCSPResponderURLConfigEntry->stringValue());
    }
    if (e.mOCSPResponderSignature) {
        mWidget->OCSPResponderSignature->setFingerprint(e.mOCSPResponderSignature->stringValue());
    }

    // dirmngr-0.9.0 options
    initializeDirmngrCheckbox(mWidget->ignoreServiceURLCB, e.mIgnoreServiceURLEntry);
    initializeDirmngrCheckbox(mWidget->ignoreHTTPDPCB, e.mIgnoreHTTPDPEntry);
    initializeDirmngrCheckbox(mWidget->disableHTTPCB, e.mDisableHTTPEntry);
    initializeDirmngrCheckbox(mWidget->ignoreLDAPDPCB, e.mIgnoreLDAPDPEntry);
    initializeDirmngrCheckbox(mWidget->disableLDAPCB, e.mDisableLDAPEntry);
    if (e.mCustomHTTPProxy) {
        QString systemProxy = QString::fromLocal8Bit(qgetenv("http_proxy"));
        if (systemProxy.isEmpty()) {
            systemProxy = i18n("no proxy");
        }
        mWidget->systemHTTPProxy->setText(i18n("(Current system setting: %1)", systemProxy));
        bool honor = e.mHonorHTTPProxy && e.mHonorHTTPProxy->boolValue();
        mWidget->honorHTTPProxyRB->setChecked(honor);
        mWidget->useCustomHTTPProxyRB->setChecked(!honor);
        mWidget->customHTTPProxy->setText(e.mCustomHTTPProxy->stringValue());
    } else {
        disableDirmngrWidget(mWidget->honorHTTPProxyRB);
        disableDirmngrWidget(mWidget->useCustomHTTPProxyRB);
        disableDirmngrWidget(mWidget->systemHTTPProxy);
        disableDirmngrWidget(mWidget->customHTTPProxy);
    }
    if (e.mCustomLDAPProxy) {
        mWidget->customLDAPProxy->setText(e.mCustomLDAPProxy->stringValue());
    } else {
        disableDirmngrWidget(mWidget->customLDAPProxy);
        disableDirmngrWidget(mWidget->customLDAPLabel);
    }
    slotUpdateHTTPActions();
}

void SecurityPage::SMimeTab::slotUpdateHTTPActions()
{
    mWidget->ignoreHTTPDPCB->setEnabled(!mWidget->disableHTTPCB->isChecked());

    // The proxy settings only make sense when "Ignore HTTP CRL DPs of certificate" is checked.
    bool enableProxySettings = !mWidget->disableHTTPCB->isChecked() && mWidget->ignoreHTTPDPCB->isChecked();
    mWidget->systemHTTPProxy->setEnabled(enableProxySettings);
    mWidget->useCustomHTTPProxyRB->setEnabled(enableProxySettings);
    mWidget->honorHTTPProxyRB->setEnabled(enableProxySettings);
    mWidget->customHTTPProxy->setEnabled(enableProxySettings && mWidget->useCustomHTTPProxyRB->isChecked());

    if (!mWidget->useCustomHTTPProxyRB->isChecked() && !mWidget->honorHTTPProxyRB->isChecked()) {
        mWidget->honorHTTPProxyRB->setChecked(true);
    }
}

static void saveCheckBoxToKleoEntry(QCheckBox *cb, QGpgME::CryptoConfigEntry *entry)
{
    const bool b = cb->isChecked();
    if (entry && entry->boolValue() != b) {
        entry->setBoolValue(b);
    }
}

void SecurityPage::SMimeTab::save()
{
    if (!mConfig) {
        return;
    }
    // Create config entries
    // Don't keep them around, they'll get deleted by clear(), which could be done by the
    // "configure backend" button.
    SMIMECryptoConfigEntries e(mConfig);

    bool b = mWidget->OCSPRB->isChecked();
    if (e.mCheckUsingOCSPConfigEntry && e.mCheckUsingOCSPConfigEntry->boolValue() != b) {
        e.mCheckUsingOCSPConfigEntry->setBoolValue(b);
    }
    // Set allow-ocsp together with enable-ocsp
    if (e.mEnableOCSPsendingConfigEntry && e.mEnableOCSPsendingConfigEntry->boolValue() != b) {
        e.mEnableOCSPsendingConfigEntry->setBoolValue(b);
    }

    saveCheckBoxToKleoEntry(mWidget->doNotCheckCertPolicyCB, e.mDoNotCheckCertPolicyConfigEntry);
    saveCheckBoxToKleoEntry(mWidget->neverConsultCB, e.mNeverConsultConfigEntry);
    saveCheckBoxToKleoEntry(mWidget->fetchMissingCB, e.mFetchMissingConfigEntry);

    QString txt = mWidget->OCSPResponderURL->text();
    if (e.mOCSPResponderURLConfigEntry && e.mOCSPResponderURLConfigEntry->stringValue() != txt) {
        e.mOCSPResponderURLConfigEntry->setStringValue(txt);
    }

    txt = mWidget->OCSPResponderSignature->fingerprint();
    if (e.mOCSPResponderSignature && e.mOCSPResponderSignature->stringValue() != txt) {
        e.mOCSPResponderSignature->setStringValue(txt);
    }

    // dirmngr-0.9.0 options
    saveCheckBoxToKleoEntry(mWidget->ignoreServiceURLCB, e.mIgnoreServiceURLEntry);
    saveCheckBoxToKleoEntry(mWidget->ignoreHTTPDPCB, e.mIgnoreHTTPDPEntry);
    saveCheckBoxToKleoEntry(mWidget->disableHTTPCB, e.mDisableHTTPEntry);
    saveCheckBoxToKleoEntry(mWidget->ignoreLDAPDPCB, e.mIgnoreLDAPDPEntry);
    saveCheckBoxToKleoEntry(mWidget->disableLDAPCB, e.mDisableLDAPEntry);
    if (e.mCustomHTTPProxy) {
        const bool honor = mWidget->honorHTTPProxyRB->isChecked();
        if (e.mHonorHTTPProxy && e.mHonorHTTPProxy->boolValue() != honor) {
            e.mHonorHTTPProxy->setBoolValue(honor);
        }

        QString chosenProxy = mWidget->customHTTPProxy->text();
        if (chosenProxy != e.mCustomHTTPProxy->stringValue()) {
            e.mCustomHTTPProxy->setStringValue(chosenProxy);
        }
    }
    txt = mWidget->customLDAPProxy->text();
    if (e.mCustomLDAPProxy && e.mCustomLDAPProxy->stringValue() != txt) {
        e.mCustomLDAPProxy->setStringValue(mWidget->customLDAPProxy->text());
    }

    mConfig->sync(true);
}

QGpgME::CryptoConfigEntry *SMIMECryptoConfigEntries::configEntry(const QString &componentName,
                                                                 const QString &groupName,
                                                                 const QString &entryName,
                                                                 int /*Kleo::CryptoConfigEntry::ArgType*/ argType,
                                                                 bool isList)
{
    QGpgME::CryptoConfigEntry *entry = mConfig->entry(componentName, groupName, entryName);
    if (!entry) {
        qCWarning(KMAIL_LOG) << QStringLiteral("Backend error: gpgconf doesn't seem to know the entry for %1/%2/%3").arg(componentName, groupName, entryName);
        return nullptr;
    }
    if (entry->argType() != argType || entry->isList() != isList) {
        qCWarning(KMAIL_LOG) << QStringLiteral("Backend error: gpgconf has wrong type for %1/%2/%3: %4 %5")
                                    .arg(componentName, groupName, entryName)
                                    .arg(entry->argType())
                                    .arg(entry->isList());
        return nullptr;
    }
    return entry;
}

GpgSettingsDialog::GpgSettingsDialog(QWidget *parent)
    : KCMultiDialog(parent)
{
    readConfig();
}

GpgSettingsDialog::~GpgSettingsDialog()
{
    saveConfig();
}

void GpgSettingsDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), "GpgSettingsDialog");
    const QSize size = group.readEntry("Size", QSize(600, 400));
    if (size.isValid()) {
        resize(size);
    }
}

void GpgSettingsDialog::saveConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), "GpgSettingsDialog");
    group.writeEntry("Size", size());
    group.sync();
}
