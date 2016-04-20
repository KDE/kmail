/*
  Copyright (c) 2013-2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "configuresecuritypage.h"
#include "PimCommon/ConfigureImmutableWidgetUtils"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "messageviewer/messageviewersettings.h"
#include "MessageComposer/MessageComposerSettings"
#include "messageviewer/adblocksettingwidget.h"
#include <MailCommon/FolderCollection>
#include "settings/kmailsettings.h"

#include "kmkernel.h"

#include "Libkleo/CryptoConfig"
#include "Libkleo/CryptoBackendFactory"
#include "Libkleo/KeyRequester"
#include "Libkleo/KeySelectionDialog"

#include <KLocalizedString>
#include <KCMultiDialog>
#include <KMessageBox>
#include <QWhatsThis>
#include <QDBusConnection>
#include "kmail_debug.h"

QString SecurityPage::helpAnchor() const
{
    return QStringLiteral("configure-security");
}

SecurityPage::SecurityPage(QWidget *parent)
    : ConfigModuleWithTabs(parent)
{
    //
    // "Reading" tab:
    //
    GeneralTab *generalTab = new GeneralTab(); //  @TODO: rename
    addTab(generalTab, i18n("Reading"));

    addTab(new MDNTab(), i18n("Message Disposition Notifications"));

    //
    // "Composing" tab:
    //
    ComposerCryptoTab *composerCryptoTab = new ComposerCryptoTab();
    addTab(composerCryptoTab, i18n("Composing"));

    //
    // "Warnings" tab:
    //
    WarningTab *warningTab = new WarningTab();
    addTab(warningTab, i18n("Miscellaneous"));

    //
    // "S/MIME Validation" tab:
    //
    SMimeTab *sMimeTab = new SMimeTab();
    addTab(sMimeTab, i18n("S/MIME Validation"));
}

QString SecurityPage::GeneralTab::helpAnchor() const
{
    return QStringLiteral("configure-security-reading");
}

SecurityPageGeneralTab::SecurityPageGeneralTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    mSGTab.setupUi(this);

    connect(mSGTab.mHtmlMailCheck, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);
    connect(mSGTab.mExternalReferences, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);
    connect(mSGTab.labelWarnHTML, &QLabel::linkActivated, this, &SecurityPageGeneralTab::slotLinkClicked);

    connect(mSGTab.mAlwaysDecrypt, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);

    connect(mSGTab.mAutomaticallyImportAttachedKeysCheck, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);

    connect(mSGTab.mScamDetection, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);

    connect(mSGTab.scamWhiteList, &PimCommon::SimpleStringListEditor::changed, this, &ConfigModuleTab::slotEmitChanged);
    mSGTab.scamWhiteList->setAddDialogLabel(i18n("Email Address:"));
}

void SecurityPageGeneralTab::slotLinkClicked(const QString &link)
{
    if (link == QLatin1String("whatsthis1")) {
        QWhatsThis::showText(QCursor::pos(), mSGTab.mHtmlMailCheck->whatsThis());
    } else if (link == QLatin1String("whatsthis2")) {
        QWhatsThis::showText(QCursor::pos(), mSGTab.mExternalReferences->whatsThis());
    }
}

void SecurityPage::GeneralTab::doLoadOther()
{
    loadWidget(mSGTab.mHtmlMailCheck, MessageViewer::MessageViewerSettings::self()->htmlMailItem());
    loadWidget(mSGTab.mExternalReferences, MessageViewer::MessageViewerSettings::self()->htmlLoadExternalItem());
    loadWidget(mSGTab.mAutomaticallyImportAttachedKeysCheck, MessageViewer::MessageViewerSettings::self()->autoImportKeysItem());
    loadWidget(mSGTab.mAlwaysDecrypt, MessageViewer::MessageViewerSettings::self()->alwaysDecryptItem());

    loadWidget(mSGTab.mScamDetection, MessageViewer::MessageViewerSettings::self()->scamDetectionEnabledItem());
    loadWidget(mSGTab.scamWhiteList, MessageViewer::MessageViewerSettings::self()->scamDetectionWhiteListItem());
}

void SecurityPage::GeneralTab::save()
{
    if (MessageViewer::MessageViewerSettings::self()->htmlMail() != mSGTab.mHtmlMailCheck->isChecked()) {
        if (KMessageBox::warningContinueCancel(this, i18n("Changing the global "
                                               "HTML setting will override all folder specific values."), QString(),
                                               KStandardGuiItem::cont(), KStandardGuiItem::cancel(), QStringLiteral("htmlMailOverride")) == KMessageBox::Continue) {
            saveCheckBox(mSGTab.mHtmlMailCheck, MessageViewer::MessageViewerSettings::self()->htmlMailItem());
            if (kmkernel) {
                foreach (const Akonadi::Collection &collection, kmkernel->allFolders()) {
                    KConfigGroup config(KMKernel::self()->config(), MailCommon::FolderCollection::configGroupName(collection));
                    //Old config
                    config.deleteEntry("htmlMailOverride");
                    config.deleteEntry("displayFormatOverride");
                }
            }
        }
    }
    saveCheckBox(mSGTab.mExternalReferences, MessageViewer::MessageViewerSettings::self()->htmlLoadExternalItem());

    saveCheckBox(mSGTab.mAutomaticallyImportAttachedKeysCheck, MessageViewer::MessageViewerSettings::self()->autoImportKeysItem());
    saveCheckBox(mSGTab.mAlwaysDecrypt, MessageViewer::MessageViewerSettings::self()->alwaysDecryptItem());
    saveCheckBox(mSGTab.mScamDetection, MessageViewer::MessageViewerSettings::self()->scamDetectionEnabledItem());
    saveSimpleStringListEditor(mSGTab.scamWhiteList, MessageViewer::MessageViewerSettings::self()->scamDetectionWhiteListItem());
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
    connect(mMDNGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SecurityPageMDNTab::slotEmitChanged);
    mMDNGroup->addButton(mUi.radioIgnore, 0);
    mMDNGroup->addButton(mUi.radioAsk, 1);
    mMDNGroup->addButton(mUi.radioDeny, 2);
    mMDNGroup->addButton(mUi.radioAlways, 3);

    // "Original Message quote" radiobuttons
    mOrigQuoteGroup = new QButtonGroup(this);
    connect(mOrigQuoteGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SecurityPageMDNTab::slotEmitChanged);
    mOrigQuoteGroup->addButton(mUi.radioNothing, 0);
    mOrigQuoteGroup->addButton(mUi.radioFull, 1);
    mOrigQuoteGroup->addButton(mUi.radioHeaders, 2);

    connect(mUi.mNoMDNsWhenEncryptedCheck, &QAbstractButton::toggled, this, &ConfigModuleTab::slotEmitChanged);
    connect(mUi.labelWarning, SIGNAL(linkActivated(QString)), SLOT(slotLinkClicked(QString)));
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
    connect(mWidget->mAutoEncrypt, &QCheckBox::toggled, this, &SecurityPageComposerCryptoTab::slotEmitChanged);
    connect(mWidget->mNeverEncryptWhenSavingInDrafts, &QCheckBox::toggled, this, &SecurityPageComposerCryptoTab::slotEmitChanged);
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

    loadWidget(mWidget->mAutoEncrypt, MessageComposer::MessageComposerSettings::self()->pgpAutoEncryptItem());
    loadWidget(mWidget->mNeverEncryptWhenSavingInDrafts, KMailSettings::self()->neverEncryptDraftsItem());

    loadWidget(mWidget->mStoreEncrypted, KMailSettings::self()->cryptoStoreEncryptedItem());
    loadWidget(mWidget->mShowEncSignIndicator, KMailSettings::self()->showCryptoLabelIndicatorItem());
}

void SecurityPage::ComposerCryptoTab::save()
{
    saveCheckBox(mWidget->mEncToSelf, MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelfItem());
    saveCheckBox(mWidget->mShowKeyApprovalDlg, MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApprovalItem());

    saveCheckBox(mWidget->mAutoEncrypt, MessageComposer::MessageComposerSettings::self()->pgpAutoEncryptItem());
    saveCheckBox(mWidget->mNeverEncryptWhenSavingInDrafts, KMailSettings::self()->neverEncryptDraftsItem());
    saveCheckBox(mWidget->mStoreEncrypted, KMailSettings::self()->cryptoStoreEncryptedItem());
    saveCheckBox(mWidget->mShowEncSignIndicator, KMailSettings::self()->showCryptoLabelIndicatorItem());
}

void SecurityPage::ComposerCryptoTab::doLoadFromGlobalSettings()
{
    loadWidget(mWidget->mEncToSelf, MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelfItem());
    loadWidget(mWidget->mShowKeyApprovalDlg, MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApprovalItem());

    loadWidget(mWidget->mAutoEncrypt, MessageComposer::MessageComposerSettings::self()->pgpAutoEncryptItem());
    loadWidget(mWidget->mNeverEncryptWhenSavingInDrafts, KMailSettings::self()->neverEncryptDraftsItem());
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
    connect(mWidget->warnReceiverNotInCertificateCB, &QCheckBox::toggled, this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnSignKeyExpiresSB, static_cast<void (KPluralHandlingSpinBox::*)(int)>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnEncrKeyExpiresSB, static_cast<void (KPluralHandlingSpinBox::*)(int)>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnEncrChainCertExpiresSB, static_cast<void (KPluralHandlingSpinBox::*)(int)>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnSignChainCertExpiresSB, static_cast<void (KPluralHandlingSpinBox::*)(int)>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnSignRootCertExpiresSB, static_cast<void (KPluralHandlingSpinBox::*)(int)>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnEncrRootCertExpiresSB, static_cast<void (KPluralHandlingSpinBox::*)(int)>(&KPluralHandlingSpinBox::valueChanged), this, &SecurityPageWarningTab::slotEmitChanged);

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
    loadWidget(mWidget->warnReceiverNotInCertificateCB, MessageComposer::MessageComposerSettings::self()->cryptoWarnRecvNotInCertItem());

    // The "-int" part of the key name is because there used to be a separate boolean
    // config entry for enabling/disabling. This is done with the single bool value now.
    mWidget->warnGroupBox->setChecked(
        MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire());

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
    loadWidget(mWidget->warnReceiverNotInCertificateCB, MessageComposer::MessageComposerSettings::self()->cryptoWarnRecvNotInCertItem());

    // The "-int" part of the key name is because there used to be a separate boolean
    // config entry for enabling/disabling. This is done with the single bool value now.
    mWidget->warnGroupBox->setChecked(
        MessageComposer::MessageComposerSettings::self()->cryptoWarnWhenNearExpire());

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
    saveCheckBox(mWidget->warnReceiverNotInCertificateCB, MessageComposer::MessageComposerSettings::self()->cryptoWarnRecvNotInCertItem());

    MessageComposer::MessageComposerSettings::self()->setCryptoWarnWhenNearExpire(
        mWidget->warnGroupBox->isChecked());

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
    QPointer<KCMultiDialog> dlg(new KCMultiDialog(this));
    KPageWidgetItem *page = dlg->addModule(QStringLiteral("kleopatra_config_gnupgsystem"));
    if (!page) {
        QLabel *info = new QLabel(i18n("The module is missing. Please verify your installation. This module is provided by Kleopatra."), this);
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
    QButtonGroup *bg = new QButtonGroup(this);
    bg->addButton(mWidget->CRLRB);
    bg->addButton(mWidget->OCSPRB);

    // Settings for the keyrequester custom widget
    mWidget->OCSPResponderSignature->setAllowedKeys(
        Kleo::KeySelectionDialog::SMIMEKeys
        | Kleo::KeySelectionDialog::TrustedKeys
        | Kleo::KeySelectionDialog::ValidKeys
        | Kleo::KeySelectionDialog::SigningKeys
        | Kleo::KeySelectionDialog::PublicKeys);
    mWidget->OCSPResponderSignature->setMultipleKeysEnabled(false);

    mConfig = Kleo::CryptoBackendFactory::instance()->config();

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
    QButtonGroup *bgHTTPProxy = new QButtonGroup(this);
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

static void initializeDirmngrCheckbox(QCheckBox *cb, Kleo::CryptoConfigEntry *entry)
{
    if (entry) {
        cb->setChecked(entry->boolValue());
    } else {
        disableDirmngrWidget(cb);
    }
}

struct SMIMECryptoConfigEntries {
    SMIMECryptoConfigEntries(Kleo::CryptoConfig *config)
        : mConfig(config)
    {

        // Checkboxes
        mCheckUsingOCSPConfigEntry = configEntry(QStringLiteral("gpgsm"), QStringLiteral("Security"), QStringLiteral("enable-ocsp"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mEnableOCSPsendingConfigEntry = configEntry(QStringLiteral("dirmngr"), QStringLiteral("OCSP"), QStringLiteral("allow-ocsp"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mDoNotCheckCertPolicyConfigEntry = configEntry(QStringLiteral("gpgsm"), QStringLiteral("Security"), QStringLiteral("disable-policy-checks"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mNeverConsultConfigEntry = configEntry(QStringLiteral("gpgsm"), QStringLiteral("Security"), QStringLiteral("disable-crl-checks"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mFetchMissingConfigEntry = configEntry(QStringLiteral("gpgsm"), QStringLiteral("Security"), QStringLiteral("auto-issuer-key-retrieve"), Kleo::CryptoConfigEntry::ArgType_None, false);
        // dirmngr-0.9.0 options
        mIgnoreServiceURLEntry = configEntry(QStringLiteral("dirmngr"), QStringLiteral("OCSP"), QStringLiteral("ignore-ocsp-service-url"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mIgnoreHTTPDPEntry = configEntry(QStringLiteral("dirmngr"), QStringLiteral("HTTP"), QStringLiteral("ignore-http-dp"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mDisableHTTPEntry = configEntry(QStringLiteral("dirmngr"), QStringLiteral("HTTP"), QStringLiteral("disable-http"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mHonorHTTPProxy = configEntry(QStringLiteral("dirmngr"), QStringLiteral("HTTP"), QStringLiteral("honor-http-proxy"), Kleo::CryptoConfigEntry::ArgType_None, false);

        mIgnoreLDAPDPEntry = configEntry(QStringLiteral("dirmngr"), QStringLiteral("LDAP"), QStringLiteral("ignore-ldap-dp"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mDisableLDAPEntry = configEntry(QStringLiteral("dirmngr"), QStringLiteral("LDAP"), QStringLiteral("disable-ldap"), Kleo::CryptoConfigEntry::ArgType_None, false);
        // Other widgets
        mOCSPResponderURLConfigEntry = configEntry(QStringLiteral("dirmngr"), QStringLiteral("OCSP"), QStringLiteral("ocsp-responder"), Kleo::CryptoConfigEntry::ArgType_String, false);
        mOCSPResponderSignature = configEntry(QStringLiteral("dirmngr"), QStringLiteral("OCSP"), QStringLiteral("ocsp-signer"), Kleo::CryptoConfigEntry::ArgType_String, false);
        mCustomHTTPProxy = configEntry(QStringLiteral("dirmngr"), QStringLiteral("HTTP"), QStringLiteral("http-proxy"), Kleo::CryptoConfigEntry::ArgType_String, false);
        mCustomLDAPProxy = configEntry(QStringLiteral("dirmngr"), QStringLiteral("LDAP"), QStringLiteral("ldap-proxy"), Kleo::CryptoConfigEntry::ArgType_String, false);
    }

    Kleo::CryptoConfigEntry *configEntry(const QString &componentName,
                                         const QString &groupName,
                                         const QString &entryName,
                                         int argType,
                                         bool isList);

    // Checkboxes
    Kleo::CryptoConfigEntry *mCheckUsingOCSPConfigEntry;
    Kleo::CryptoConfigEntry *mEnableOCSPsendingConfigEntry;
    Kleo::CryptoConfigEntry *mDoNotCheckCertPolicyConfigEntry;
    Kleo::CryptoConfigEntry *mNeverConsultConfigEntry;
    Kleo::CryptoConfigEntry *mFetchMissingConfigEntry;
    Kleo::CryptoConfigEntry *mIgnoreServiceURLEntry;
    Kleo::CryptoConfigEntry *mIgnoreHTTPDPEntry;
    Kleo::CryptoConfigEntry *mDisableHTTPEntry;
    Kleo::CryptoConfigEntry *mHonorHTTPProxy;
    Kleo::CryptoConfigEntry *mIgnoreLDAPDPEntry;
    Kleo::CryptoConfigEntry *mDisableLDAPEntry;
    // Other widgets
    Kleo::CryptoConfigEntry *mOCSPResponderURLConfigEntry;
    Kleo::CryptoConfigEntry *mOCSPResponderSignature;
    Kleo::CryptoConfigEntry *mCustomHTTPProxy;
    Kleo::CryptoConfigEntry *mCustomLDAPProxy;

    Kleo::CryptoConfig *mConfig;
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
    bool enableProxySettings = !mWidget->disableHTTPCB->isChecked()
                               && mWidget->ignoreHTTPDPCB->isChecked();
    mWidget->systemHTTPProxy->setEnabled(enableProxySettings);
    mWidget->useCustomHTTPProxyRB->setEnabled(enableProxySettings);
    mWidget->honorHTTPProxyRB->setEnabled(enableProxySettings);
    mWidget->customHTTPProxy->setEnabled(enableProxySettings && mWidget->useCustomHTTPProxyRB->isChecked());

    if (!mWidget->useCustomHTTPProxyRB->isChecked() &&
            !mWidget->honorHTTPProxyRB->isChecked()) {
        mWidget->honorHTTPProxyRB->setChecked(true);
    }
}

static void saveCheckBoxToKleoEntry(QCheckBox *cb, Kleo::CryptoConfigEntry *entry)
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

    //dirmngr-0.9.0 options
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

Kleo::CryptoConfigEntry *SMIMECryptoConfigEntries::configEntry(const QString &componentName,
        const QString &groupName,
        const QString &entryName,
        int /*Kleo::CryptoConfigEntry::ArgType*/ argType,
        bool isList)
{
    Kleo::CryptoConfigEntry *entry = mConfig->entry(componentName, groupName, entryName);
    if (!entry) {
        qCWarning(KMAIL_LOG) << QStringLiteral("Backend error: gpgconf doesn't seem to know the entry for %1/%2/%3").arg(componentName, groupName, entryName);
        return Q_NULLPTR;
    }
    if (entry->argType() != argType || entry->isList() != isList) {
        qCWarning(KMAIL_LOG) << QStringLiteral("Backend error: gpgconf has wrong type for %1/%2/%3: %4 %5").arg(componentName, groupName, entryName).arg(entry->argType()).arg(entry->isList());
        return Q_NULLPTR;
    }
    return entry;
}
