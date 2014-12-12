/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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
#include "pimcommon/widgets/configureimmutablewidgetutils.h"
using namespace PimCommon::ConfigureImmutableWidgetUtils;
#include "messageviewer/settings/globalsettings.h"
#include "messagecomposer/settings/messagecomposersettings.h"
#include "messageviewer/adblock/adblocksettingwidget.h"
#include "mailcommon/folder/foldercollection.h"
#include "settings/globalsettings.h"

#include "kmkernel.h"

#include "kleo/cryptoconfig.h"
#include "kleo/cryptobackendfactory.h"
#include "libkleo/ui/keyrequester.h"
#include "libkleo/ui/keyselectiondialog.h"
#include "libkleo/ui/cryptoconfigdialog.h"

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
    mGeneralTab = new GeneralTab(); //  @TODO: rename
    addTab(mGeneralTab, i18n("Reading"));

    addTab(new MDNTab(), i18n("Message Disposition Notifications"));

    //
    // "Composing" tab:
    //
    mComposerCryptoTab = new ComposerCryptoTab();
    addTab(mComposerCryptoTab, i18n("Composing"));

    //
    // "Warnings" tab:
    //
    mWarningTab = new WarningTab();
    addTab(mWarningTab, i18n("Miscellaneous"));

    //
    // "S/MIME Validation" tab:
    //
    mSMimeTab = new SMimeTab();
    addTab(mSMimeTab, i18n("S/MIME Validation"));
#ifndef KDEPIM_NO_WEBKIT
    mSAdBlockTab = new SecurityPageAdBlockTab;
    addTab(mSAdBlockTab, i18n("Ad block"));
#endif
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
    connect(mSGTab.labelWarnHTML, SIGNAL(linkActivated(QString)), SLOT(slotLinkClicked(QString)));

    connect(mSGTab.mAlwaysDecrypt, &QCheckBox::stateChanged, this, &SecurityPageGeneralTab::slotEmitChanged);

    connect(mSGTab.mAutomaticallyImportAttachedKeysCheck, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()));

    connect(mSGTab.mScamDetection, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()));

    connect(mSGTab.scamWhiteList, SIGNAL(changed()), SLOT(slotEmitChanged()));
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
    loadWidget(mSGTab.mHtmlMailCheck, MessageViewer::GlobalSettings::self()->htmlMailItem());
    loadWidget(mSGTab.mExternalReferences, MessageViewer::GlobalSettings::self()->htmlLoadExternalItem());
    loadWidget(mSGTab.mAutomaticallyImportAttachedKeysCheck, MessageViewer::GlobalSettings::self()->autoImportKeysItem());
    loadWidget(mSGTab.mAlwaysDecrypt, MessageViewer::GlobalSettings::self()->alwaysDecryptItem());

    loadWidget(mSGTab.mScamDetection, MessageViewer::GlobalSettings::self()->scamDetectionEnabledItem());
    loadWidget(mSGTab.scamWhiteList, MessageViewer::GlobalSettings::self()->scamDetectionWhiteListItem());
}

void SecurityPage::GeneralTab::save()
{
    if (MessageViewer::GlobalSettings::self()->htmlMail() != mSGTab.mHtmlMailCheck->isChecked()) {
        if (KMessageBox::warningContinueCancel(this, i18n("Changing the global "
                                               "HTML setting will override all folder specific values."), QString(),
                                               KStandardGuiItem::cont(), KStandardGuiItem::cancel(), QLatin1String("htmlMailOverride")) == KMessageBox::Continue) {
            saveCheckBox(mSGTab.mHtmlMailCheck, MessageViewer::GlobalSettings::self()->htmlMailItem());
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
    saveCheckBox(mSGTab.mExternalReferences, MessageViewer::GlobalSettings::self()->htmlLoadExternalItem());

    saveCheckBox(mSGTab.mAutomaticallyImportAttachedKeysCheck, MessageViewer::GlobalSettings::self()->autoImportKeysItem());
    saveCheckBox(mSGTab.mAlwaysDecrypt, MessageViewer::GlobalSettings::self()->alwaysDecryptItem());
    saveCheckBox(mSGTab.mScamDetection, MessageViewer::GlobalSettings::self()->scamDetectionEnabledItem());
    saveSimpleStringListEditor(mSGTab.scamWhiteList, MessageViewer::GlobalSettings::self()->scamDetectionWhiteListItem());
}

//Adblock

#ifndef KDEPIM_NO_WEBKIT
QString SecurityPageAdBlockTab::helpAnchor() const
{
    return QString();
}

SecurityPageAdBlockTab::SecurityPageAdBlockTab(QWidget *parent)
    : ConfigModuleTab(parent)
{
    QHBoxLayout *lay = new QHBoxLayout;
    lay->setMargin(0);

    mWidget = new MessageViewer::AdBlockSettingWidget;
    lay->addWidget(mWidget);
    connect(mWidget, SIGNAL(changed(bool)), SLOT(slotEmitChanged()));
    setLayout(lay);
}

SecurityPageAdBlockTab::~SecurityPageAdBlockTab()
{
}

void SecurityPageAdBlockTab::save()
{
    mWidget->save();
}

void SecurityPageAdBlockTab::doLoadFromGlobalSettings()
{
    mWidget->doLoadFromGlobalSettings();
}

void SecurityPageAdBlockTab::doLoadOther()
{

}

void SecurityPageAdBlockTab::doResetToDefaultsOther()
{
    mWidget->doResetToDefaultsOther();
}
#endif

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

    connect(mUi.mNoMDNsWhenEncryptedCheck, SIGNAL(toggled(bool)), SLOT(slotEmitChanged()));
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
    int num = MessageViewer::GlobalSettings::self()->defaultPolicy();
    if (num < 0 || num >= mMDNGroup->buttons().count()) {
        num = 0;
    }
    mMDNGroup->button(num)->setChecked(true);
    num = MessageViewer::GlobalSettings::self()->quoteMessage();
    if (num < 0 || num >= mOrigQuoteGroup->buttons().count()) {
        num = 0;
    }
    mOrigQuoteGroup->button(num)->setChecked(true);
    loadWidget(mUi.mNoMDNsWhenEncryptedCheck, MessageViewer::GlobalSettings::self()->notSendWhenEncryptedItem());
}

void SecurityPage::MDNTab::save()
{
    MessageViewer::GlobalSettings::self()->setDefaultPolicy(mMDNGroup->checkedId());
    MessageViewer::GlobalSettings::self()->setQuoteMessage(mOrigQuoteGroup->checkedId());
    saveCheckBox(mUi.mNoMDNsWhenEncryptedCheck, MessageViewer::GlobalSettings::self()->notSendWhenEncryptedItem());
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
    connect(mWidget->mShowEncryptionResult, &QCheckBox::toggled, this, &SecurityPageComposerCryptoTab::slotEmitChanged);
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
    mWidget->mShowEncryptionResult->setChecked(false);   //composer.readBoolEntry( "crypto-show-encryption-result", true ) );
    mWidget->mShowEncryptionResult->hide();
    loadWidget(mWidget->mShowKeyApprovalDlg, MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApprovalItem());

    loadWidget(mWidget->mAutoEncrypt, MessageComposer::MessageComposerSettings::self()->pgpAutoEncryptItem()) ;
    loadWidget(mWidget->mNeverEncryptWhenSavingInDrafts, GlobalSettings::self()->neverEncryptDraftsItem());

    loadWidget(mWidget->mStoreEncrypted, GlobalSettings::self()->cryptoStoreEncryptedItem());
    loadWidget(mWidget->mShowEncSignIndicator, GlobalSettings::self()->showCryptoLabelIndicatorItem());
}

void SecurityPage::ComposerCryptoTab::save()
{
    saveCheckBox(mWidget->mEncToSelf, MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelfItem());
    saveCheckBox(mWidget->mShowEncryptionResult, GlobalSettings::self()->cryptoShowEncryptionResultItem());
    saveCheckBox(mWidget->mShowKeyApprovalDlg, MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApprovalItem());

    saveCheckBox(mWidget->mAutoEncrypt, MessageComposer::MessageComposerSettings::self()->pgpAutoEncryptItem()) ;
    saveCheckBox(mWidget->mNeverEncryptWhenSavingInDrafts, GlobalSettings::self()->neverEncryptDraftsItem());
    saveCheckBox(mWidget->mStoreEncrypted, GlobalSettings::self()->cryptoStoreEncryptedItem());
    saveCheckBox(mWidget->mShowEncSignIndicator, GlobalSettings::self()->showCryptoLabelIndicatorItem());
}

void SecurityPage::ComposerCryptoTab::doLoadFromGlobalSettings()
{
    loadWidget(mWidget->mEncToSelf, MessageComposer::MessageComposerSettings::self()->cryptoEncryptToSelfItem());
    loadWidget(mWidget->mShowEncryptionResult, GlobalSettings::self()->cryptoShowEncryptionResultItem());
    loadWidget(mWidget->mShowKeyApprovalDlg, MessageComposer::MessageComposerSettings::self()->cryptoShowKeysForApprovalItem());

    loadWidget(mWidget->mAutoEncrypt, MessageComposer::MessageComposerSettings::self()->pgpAutoEncryptItem()) ;
    loadWidget(mWidget->mNeverEncryptWhenSavingInDrafts, GlobalSettings::self()->neverEncryptDraftsItem());
    loadWidget(mWidget->mStoreEncrypted, GlobalSettings::self()->cryptoStoreEncryptedItem());
    loadWidget(mWidget->mShowEncSignIndicator, GlobalSettings::self()->showCryptoLabelIndicatorItem());

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

    mWidget->chiasmusButton->hide();

    connect(mWidget->warnGroupBox, &QGroupBox::toggled, this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->mWarnUnsigned, &QCheckBox::toggled, this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->warnUnencryptedCB, &QCheckBox::toggled, this, &SecurityPageWarningTab::slotEmitChanged);
    connect(mWidget->warnReceiverNotInCertificateCB, &QCheckBox::toggled, this, &SecurityPageWarningTab::slotEmitChanged);

    connect(mWidget->gnupgButton, &QPushButton::clicked, this, &SecurityPageWarningTab::slotConfigureGnupg);
    //connect( mWidget->chiasmusButton, SIGNAL(clicked()), SLOT(slotConfigureChiasmus()) );
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
    dlg->addModule(QLatin1String("kleopatra_config_gnupgsystem"));
    dlg->exec();
    delete dlg;
}

#if 0
void SecurityPage::WarningTab::slotConfigureChiasmus()
{
    using namespace Kleo;
    // Find Chiasmus backend:
    if (const CryptoBackendFactory *const bf = Kleo::CryptoBackendFactory::instance())
        for (unsigned int i = 0 ; const CryptoBackend *const b = bf->backend(i) ; ++i)
            if (b->name() == QLatin1String("Chiasmus"))
                if (CryptoConfig *const c = b->config()) {
                    QPointer<CryptoConfigDialog> dlg(new CryptoConfigDialog(c, this));
                    dlg->exec();
                    delete dlg;
                    break;
                } else {
                    qCWarning(KMAIL_LOG) << "Found Chiasmus backend, but there doesn't seem to be a config object available from it.";
                }
            else {
                qCDebug(KMAIL_LOG) << "Skipping" << b->name() << "backend (not \"Chiasmus\")";
            }
    else {
        qCDebug(KMAIL_LOG) << "Kleo::CryptoBackendFactory::instance() returned NULL!";
    }
}
#endif
////

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

    QDBusConnection::sessionBus().connect(QString(), QString(), QLatin1String("org.kde.kleo.CryptoConfig"), QLatin1String("changed"), this, SLOT(load()));
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
        mCheckUsingOCSPConfigEntry = configEntry(QLatin1String("gpgsm"), QLatin1String("Security"), QLatin1String("enable-ocsp"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mEnableOCSPsendingConfigEntry = configEntry(QLatin1String("dirmngr"), QLatin1String("OCSP"), QLatin1String("allow-ocsp"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mDoNotCheckCertPolicyConfigEntry = configEntry(QLatin1String("gpgsm"), QLatin1String("Security"), QLatin1String("disable-policy-checks"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mNeverConsultConfigEntry = configEntry(QLatin1String("gpgsm"), QLatin1String("Security"), QLatin1String("disable-crl-checks"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mFetchMissingConfigEntry = configEntry(QLatin1String("gpgsm"), QLatin1String("Security"), QLatin1String("auto-issuer-key-retrieve"), Kleo::CryptoConfigEntry::ArgType_None, false);
        // dirmngr-0.9.0 options
        mIgnoreServiceURLEntry = configEntry(QLatin1String("dirmngr"), QLatin1String("OCSP"), QLatin1String("ignore-ocsp-service-url"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mIgnoreHTTPDPEntry = configEntry(QLatin1String("dirmngr"), QLatin1String("HTTP"), QLatin1String("ignore-http-dp"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mDisableHTTPEntry = configEntry(QLatin1String("dirmngr"), QLatin1String("HTTP"), QLatin1String("disable-http"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mHonorHTTPProxy = configEntry(QLatin1String("dirmngr"), QLatin1String("HTTP"), QLatin1String("honor-http-proxy"), Kleo::CryptoConfigEntry::ArgType_None, false);

        mIgnoreLDAPDPEntry = configEntry(QLatin1String("dirmngr"), QLatin1String("LDAP"), QLatin1String("ignore-ldap-dp"), Kleo::CryptoConfigEntry::ArgType_None, false);
        mDisableLDAPEntry = configEntry(QLatin1String("dirmngr"), QLatin1String("LDAP"), QLatin1String("disable-ldap"), Kleo::CryptoConfigEntry::ArgType_None, false);
        // Other widgets
        mOCSPResponderURLConfigEntry = configEntry(QLatin1String("dirmngr"), QLatin1String("OCSP"), QLatin1String("ocsp-responder"), Kleo::CryptoConfigEntry::ArgType_String, false);
        mOCSPResponderSignature = configEntry(QLatin1String("dirmngr"), QLatin1String("OCSP"), QLatin1String("ocsp-signer"), Kleo::CryptoConfigEntry::ArgType_String, false);
        mCustomHTTPProxy = configEntry(QLatin1String("dirmngr"), QLatin1String("HTTP"), QLatin1String("http-proxy"), Kleo::CryptoConfigEntry::ArgType_String, false);
        mCustomLDAPProxy = configEntry(QLatin1String("dirmngr"), QLatin1String("LDAP"), QLatin1String("ldap-proxy"), Kleo::CryptoConfigEntry::ArgType_String, false);
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
        return 0;
    }
    if (entry->argType() != argType || entry->isList() != isList) {
        qCWarning(KMAIL_LOG) << QStringLiteral("Backend error: gpgconf has wrong type for %1/%2/%3: %4 %5").arg(componentName, groupName, entryName).arg(entry->argType()).arg(entry->isList());
        return 0;
    }
    return entry;
}
