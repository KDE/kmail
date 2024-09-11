/*
    identitydialog.cpp

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2002 Marc Mutz <mutz@kde.org>
    SPDX-FileCopyrightText: 2014-2024 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "identitydialog.h"

#include "identityaddvcarddialog.h"
#include "identityeditvcarddialog.h"
#include "identityfolderrequester.h"
#include "identityinvalidfolder.h"

#include <QGpgME/Job>
#include <QGpgME/Protocol>

#include <KIdentityManagementCore/IdentityManager>

// other KMail headers:
#include "settings/kmailsettings.h"
#include "xfaceconfigurator.h"
#include <KEditListWidget>
#include <MailCommon/FolderRequester>

#include <MailCommon/MailKernel>

#include "job/addressvalidationjob.h"
#include <MessageComposer/Kleo_Util>
#include <MessageComposer/MessageComposerSettings>
#include <MessageCore/StringUtil>
#include <Sonnet/DictionaryComboBox>
#include <TemplateParser/TemplatesConfiguration>
#include <templateparser/templatesconfiguration_kfg.h>
// other kdepim headers:
#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementWidgets/SignatureConfigurator>

#include <PimCommon/PimUtil>
#include <TextAutoCorrectionWidgets/AutoCorrectionLanguage>

#include <KLineEditEventHandler>
#include <PimCommonAkonadi/AddresseeLineEdit>

// libkleopatra:
#include <Libkleo/DefaultKeyFilter>
#include <Libkleo/Formatting>
#include <Libkleo/KeyParameters>
#include <Libkleo/KeySelectionCombo>
#include <Libkleo/OpenPGPCertificateCreationDialog>
#include <Libkleo/ProgressDialog>

// gpgme++
#include <QGpgME/KeyGenerationJob>
#include <QGpgME/Protocol>
#include <gpgme++/context.h>
#include <gpgme++/interfaces/passphraseprovider.h>
#include <gpgme++/key.h>
#include <gpgme++/keygenerationresult.h>

#include <KEmailAddress>
#include <MailTransport/Transport>
#include <MailTransport/TransportComboBox>
#include <MailTransport/TransportManager>
using MailTransport::TransportManager;

// other KDE headers:
#include "kmail_debug.h"
#include <KEmailValidator>
#include <KLocalizedString>
#include <KMessageBox>
#include <QComboBox>
#include <QIcon>
#include <QPushButton>
#include <QTabWidget>

// Qt headers:
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHostInfo>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

// other headers:
#include <algorithm>
#include <iterator>

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/SpecialMailCollections>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QStandardPaths>

#if KMAIL_HAVE_ACTIVITY_SUPPORT
#include <PimCommonActivities/ActivitiesBaseManager>
#include <PimCommonActivities/ConfigureActivitiesWidget>
#endif

using namespace MailTransport;
using namespace MailCommon;
using namespace Qt::Literals::StringLiterals;

class EmptyPassphraseProvider : public GpgME::PassphraseProvider
{
public:
    char *getPassphrase(const char *useridHint, const char *description, bool previousWasBad, bool &canceled) override
    {
        Q_UNUSED(useridHint);
        Q_UNUSED(description);
        Q_UNUSED(previousWasBad);
        Q_UNUSED(canceled);
        return gpgrt_strdup("");
    }
};

namespace KMail
{
class KeySelectionCombo : public Kleo::KeySelectionCombo
{
    Q_OBJECT

public:
    enum KeyType {
        SigningKey,
        EncryptionKey,
    };

    explicit KeySelectionCombo(KeyType keyType, GpgME::Protocol protocol, QWidget *parent);
    ~KeySelectionCombo() override;

    void setIdentity(const QString &name, const QString &email);

    void init() override;

private:
    void onCustomItemSelected(const QVariant &type);
    QString mEmail;
    QString mName;
    const KeyType mKeyType;
    const GpgME::Protocol mProtocol;
};

class KMailKeyGenerationJob : public KJob
{
    Q_OBJECT

public:
    enum {
        GpgError = UserDefinedError,
    };
    explicit KMailKeyGenerationJob(const QString &name, const QString &email, KeySelectionCombo *parent);
    ~KMailKeyGenerationJob() override;

    bool doKill() override;
    void start() override;

private:
    void createCertificate(const Kleo::KeyParameters &keyParameters, bool protectKeyWithPassword);
    void keyGenerated(const GpgME::KeyGenerationResult &result);
    const QString mName;
    const QString mEmail;
    QGpgME::Job *mJob = nullptr;
    EmptyPassphraseProvider emptyPassphraseProvider;
};

KMailKeyGenerationJob::KMailKeyGenerationJob(const QString &name, const QString &email, KeySelectionCombo *parent)
    : KJob(parent)
    , mName(name)
    , mEmail(email)
{
}

KMailKeyGenerationJob::~KMailKeyGenerationJob() = default;

bool KMailKeyGenerationJob::doKill()
{
    if (mJob) {
        mJob->slotCancel();
    }
    return true;
}

void KMailKeyGenerationJob::start()
{
    auto dialog = new Kleo::OpenPGPCertificateCreationDialog(qobject_cast<KeySelectionCombo *>(parent()));
    dialog->setName(mName);
    dialog->setEmail(mEmail);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        const auto keyParameters = dialog->keyParameters();
        const auto protectKeyWithPassword = dialog->protectKeyWithPassword();

        QMetaObject::invokeMethod(
            this,
            [this, keyParameters, protectKeyWithPassword] {
                createCertificate(keyParameters, protectKeyWithPassword);
            },
            Qt::QueuedConnection);
    });
    connect(dialog, &QDialog::rejected, this, [this]() {
        emitResult();
    });

    dialog->show();
}

void KMailKeyGenerationJob::createCertificate(const Kleo::KeyParameters &keyParameters, bool protectKeyWithPassword)
{
    Q_ASSERT(keyParameters.protocol() == Kleo::KeyParameters::OpenPGP);

    auto keyGenJob = QGpgME::openpgp()->keyGenerationJob();
    if (!keyGenJob) {
        setError(GpgError);
        setErrorText(i18nc("@info:status", "Could not start OpenPGP certificate generation."));
        emitResult();
        return;
    }
    if (!protectKeyWithPassword) {
        auto ctx = QGpgME::Job::context(keyGenJob);
        ctx->setPassphraseProvider(&emptyPassphraseProvider);
        ctx->setPinentryMode(GpgME::Context::PinentryLoopback);
    }

    connect(keyGenJob, &QGpgME::KeyGenerationJob::result, this, &KMailKeyGenerationJob::keyGenerated);
    if (const GpgME::Error err = keyGenJob->start(keyParameters.toString())) {
        setError(GpgError);
        setErrorText(i18n("Could not start OpenPGP certificate generation: %1", Kleo::Formatting::errorAsString(err)));
        emitResult();
        return;
    } else {
        mJob = keyGenJob;
    }
    auto progressDialog = new QProgressDialog;
    progressDialog->setAttribute(Qt::WA_DeleteOnClose);
    progressDialog->setModal(true);
    progressDialog->setWindowTitle(i18nc("@title", "Generating an OpenPGP Certificate…"));
    progressDialog->setLabelText(
        i18n("The process of generating an OpenPGP certificate requires large amounts of random numbers. This may require several minutes…"));
    progressDialog->setRange(0, 0);
    connect(progressDialog, &QProgressDialog::canceled, this, [this]() {
        kill();
    });
    connect(mJob, &QGpgME::Job::done, this, [progressDialog]() {
        if (progressDialog) {
            progressDialog->accept();
        }
    });
    progressDialog->show();
}

void KMailKeyGenerationJob::keyGenerated(const GpgME::KeyGenerationResult &result)
{
    mJob = nullptr;
    if (result.error()) {
        setError(GpgError);
        setErrorText(i18n("Could not generate an OpenPGP certificate: %1", Kleo::Formatting::errorAsString(result.error())));
        emitResult();
        return;
    } else if (result.error().isCanceled()) {
        setError(GpgError);
        setErrorText(i18nc("@info:status", "Key generation was cancelled."));
        emitResult();
        return;
    }

    auto combo = qobject_cast<KeySelectionCombo *>(parent());
    combo->setDefaultKey(QLatin1StringView(result.fingerprint()));
    connect(combo, &KeySelectionCombo::keyListingFinished, this, &KMailKeyGenerationJob::emitResult);
    combo->refreshKeys();
}

KeySelectionCombo::KeySelectionCombo(KeyType keyType, GpgME::Protocol protocol, QWidget *parent)
    : Kleo::KeySelectionCombo(parent)
    , mKeyType(keyType)
    , mProtocol(protocol)
{
}

KeySelectionCombo::~KeySelectionCombo() = default;

void KeySelectionCombo::setIdentity(const QString &name, const QString &email)
{
    mName = name;
    mEmail = email;
    setIdFilter(email);
}

void KeySelectionCombo::init()
{
    Kleo::KeySelectionCombo::init();

    std::shared_ptr<Kleo::DefaultKeyFilter> keyFilter(new Kleo::DefaultKeyFilter);
    keyFilter->setIsOpenPGP(mProtocol == GpgME::OpenPGP ? Kleo::DefaultKeyFilter::Set : Kleo::DefaultKeyFilter::NotSet);
    if (mKeyType == SigningKey) {
        keyFilter->setCanSign(Kleo::DefaultKeyFilter::Set);
        keyFilter->setHasSecret(Kleo::DefaultKeyFilter::Set);
    } else {
        keyFilter->setCanEncrypt(Kleo::DefaultKeyFilter::Set);
    }
    setKeyFilter(keyFilter);
    prependCustomItem(QIcon(), i18n("No key"), QStringLiteral("no-key"));
    if (mProtocol == GpgME::OpenPGP) {
        appendCustomItem(QIcon::fromTheme(QStringLiteral("password-generate")), i18n("Generate a new OpenPGP certificate"), QStringLiteral("generate-new-key"));
    }

    connect(this, &KeySelectionCombo::customItemSelected, this, &KeySelectionCombo::onCustomItemSelected);
}

void KeySelectionCombo::onCustomItemSelected(const QVariant &type)
{
    if (type == "no-key"_L1) {
        return;
    } else if (type == "generate-new-key"_L1) {
        auto job = new KMailKeyGenerationJob(mName, mEmail, this);
        setEnabled(false);
        connect(job, &KMailKeyGenerationJob::finished, this, [this, job]() {
            if (job->error() != KJob::NoError) {
                KMessageBox::error(qobject_cast<QWidget *>(parent()), job->errorText(), i18nc("@title:window", "Key Generation Error"));
            }
            setEnabled(true);
        });
        job->start();
    }
}

IdentityDialog::IdentityDialog(QWidget *parent)
    : QDialog(parent)
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    , mConfigureActivitiesWidget(new PimCommonActivities::ConfigureActivitiesWidget(this))
#endif
{
    setWindowTitle(i18nc("@title:window", "Edit Identity"));
    auto mainLayout = new QVBoxLayout(this);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, this);
    connect(buttonBox->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &IdentityDialog::slotHelp);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IdentityDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IdentityDialog::reject);

    //
    // Tab Widget: General
    //
    auto page = new QWidget(this);
    mainLayout->addWidget(page);
    mainLayout->addWidget(buttonBox);
    auto vlay = new QVBoxLayout(page);
    vlay->setContentsMargins({});
    mTabWidget = new QTabWidget(page);
    mTabWidget->setObjectName("config-identity-tab"_L1);
    vlay->addWidget(mTabWidget);

    auto tab = new QWidget(mTabWidget);
    mTabWidget->addTab(tab, i18nc("@title:tab General identity settings.", "General"));

    auto formLayout = new QFormLayout(tab);

    // "Name" line edit and label:
    mNameEdit = new QLineEdit(tab);
    KLineEditEventHandler::catchReturnKey(mNameEdit);
    auto label = new QLabel(i18nc("@label:textbox", "&Your name:"), tab);
    formLayout->addRow(label, mNameEdit);
    label->setBuddy(mNameEdit);

    QString msg = i18n(
        "<qt><h3>Your name</h3>"
        "<p>This field should contain your name as you would like "
        "it to appear in the email header that is sent out;</p>"
        "<p>if you leave this blank your real name will not "
        "appear, only the email address.</p></qt>");
    label->setWhatsThis(msg);
    mNameEdit->setWhatsThis(msg);

    // "Organization" line edit and label:
    mOrganizationEdit = new QLineEdit(tab);
    KLineEditEventHandler::catchReturnKey(mOrganizationEdit);
    label = new QLabel(i18nc("@label:textbox", "Organi&zation:"), tab);
    formLayout->addRow(label, mOrganizationEdit);
    label->setBuddy(mOrganizationEdit);

    msg = i18n(
        "<qt><h3>Organization</h3>"
        "<p>This field should have the name of your organization "
        "if you would like it to be shown in the email header that "
        "is sent out.</p>"
        "<p>It is safe (and normal) to leave this blank.</p></qt>");
    label->setWhatsThis(msg);
    mOrganizationEdit->setWhatsThis(msg);

    // "Email Address" line edit and label:
    // (row 3: spacer)
    mEmailEdit = new QLineEdit(tab);
    KLineEditEventHandler::catchReturnKey(mEmailEdit);
    label = new QLabel(i18nc("@label:textbox", "&Email address:"), tab);
    formLayout->addRow(label, mEmailEdit);
    label->setBuddy(mEmailEdit);

    msg = i18n(
        "<qt><h3>Email address</h3>"
        "<p>This field should have your full email address.</p>"
        "<p>This address is the primary one, used for all outgoing mail. "
        "If you have more than one address, either create a new identity, "
        "or add additional alias addresses in the field below.</p>"
        "<p>If you leave this blank, or get it wrong, people "
        "will have trouble replying to you.</p></qt>");
    label->setWhatsThis(msg);
    mEmailEdit->setWhatsThis(msg);

    auto emailValidator = new KEmailValidator(this);
    mEmailEdit->setValidator(emailValidator);

    // "Email Aliases" string text edit and label:
    mAliasEdit = new KEditListWidget(tab);

    auto emailValidator1 = new KEmailValidator(this);
    mAliasEdit->lineEdit()->setValidator(emailValidator1);

    label = new QLabel(i18nc("@label:textbox", "Email a&liases:"), tab);
    formLayout->addRow(label, mAliasEdit);
    label->setBuddy(mAliasEdit);

    msg = i18n(
        "<qt><h3>Email aliases</h3>"
        "<p>This field contains alias addresses that should also "
        "be considered as belonging to this identity (as opposed "
        "to representing a different identity).</p>"
        "<p>Example:</p>"
        "<table>"
        "<tr><th>Primary address:</th><td>first.last@example.org</td></tr>"
        "<tr><th>Aliases:</th><td>first@example.org<br>last@example.org</td></tr>"
        "</table>"
        "<p>Type one alias address per line.</p></qt>");
    label->setToolTip(msg);
    mAliasEdit->setWhatsThis(msg);

    //
    // Tab Widget: Cryptography
    //
    mCryptographyTab = new QWidget(mTabWidget);
    mTabWidget->addTab(mCryptographyTab, i18n("Cryptography"));
    formLayout = new QFormLayout(mCryptographyTab);

    // "OpenPGP Signature Key" requester and label:
    mPGPSigningKeyRequester = new KeySelectionCombo(KeySelectionCombo::SigningKey, GpgME::OpenPGP, mCryptographyTab);
    msg = i18n(
        "<qt><p>The OpenPGP key you choose here will be used "
        "to digitally sign messages. You can also use GnuPG keys.</p>"
        "<p>You can leave this blank, but KMail will not be able "
        "to digitally sign emails using OpenPGP; "
        "normal mail functions will not be affected.</p>"
        "<p>You can find out more about keys at <a>https://www.gnupg.org</a></p></qt>");
    label = new QLabel(i18nc("@label:textbox", "OpenPGP signing key:"), mCryptographyTab);
    label->setBuddy(mPGPSigningKeyRequester);
    mPGPSigningKeyRequester->setWhatsThis(msg);
    label->setWhatsThis(msg);

    auto vbox = new QVBoxLayout;
    mPGPSameKey = new QCheckBox(i18nc("@option:check", "Use same key for encryption and signing"), this);
    vbox->addWidget(mPGPSigningKeyRequester);
    vbox->addWidget(mPGPSameKey);
    formLayout->addRow(label, vbox);

    connect(mPGPSameKey, &QCheckBox::toggled, this, [this, formLayout, vbox](bool checked) {
        mPGPEncryptionKeyRequester->setVisible(!checked);
        formLayout->labelForField(mPGPEncryptionKeyRequester)->setVisible(!checked);
        const auto label = qobject_cast<QLabel *>(formLayout->labelForField(vbox));
        if (checked) {
            label->setText(i18n("OpenPGP key:"));
            const auto key = mPGPSigningKeyRequester->currentKey();
            if (!key.isBad()) {
                mPGPEncryptionKeyRequester->setCurrentKey(key);
            } else if (mPGPSigningKeyRequester->currentData() == "no-key"_L1) {
                mPGPEncryptionKeyRequester->setCurrentIndex(mPGPSigningKeyRequester->currentIndex());
            }
        } else {
            label->setText(i18n("OpenPGP signing key:"));
        }
    });
    connect(mPGPSigningKeyRequester, &KeySelectionCombo::currentKeyChanged, this, [&](const GpgME::Key &key) {
        if (mPGPSameKey->isChecked()) {
            mPGPEncryptionKeyRequester->setCurrentKey(key);
        }
    });
    connect(mPGPSigningKeyRequester, &KeySelectionCombo::customItemSelected, this, [&](const QVariant &type) {
        if (mPGPSameKey->isChecked() && type == "no-key"_L1) {
            mPGPEncryptionKeyRequester->setCurrentIndex(mPGPSigningKeyRequester->currentIndex());
        }
    });

    // "OpenPGP Encryption Key" requester and label:
    mPGPEncryptionKeyRequester = new KeySelectionCombo(KeySelectionCombo::EncryptionKey, GpgME::OpenPGP, mCryptographyTab);
    msg = i18n(
        "<qt><p>The OpenPGP key you choose here will be used "
        "to encrypt messages to yourself and for the \"Attach My Public Key\" "
        "feature in the composer. You can also use GnuPG keys.</p>"
        "<p>You can leave this blank, but KMail will not be able "
        "to encrypt copies of outgoing messages to you using OpenPGP; "
        "normal mail functions will not be affected.</p>"
        "<p>You can find out more about keys at <a>https://www.gnupg.org</a></p></qt>");
    label = new QLabel(i18nc("@label:textbox", "OpenPGP encryption key:"), mCryptographyTab);
    label->setBuddy(mPGPEncryptionKeyRequester);
    label->setWhatsThis(msg);
    mPGPEncryptionKeyRequester->setWhatsThis(msg);

    formLayout->addRow(label, mPGPEncryptionKeyRequester);

    // "S/MIME Signature Key" requester and label:
    mSMIMESigningKeyRequester = new KeySelectionCombo(KeySelectionCombo::SigningKey, GpgME::CMS, mCryptographyTab);
    msg = i18n(
        "<qt><p>The S/MIME (X.509) certificate you choose here will be used "
        "to digitally sign messages.</p>"
        "<p>You can leave this blank, but KMail will not be able "
        "to digitally sign emails using S/MIME; "
        "normal mail functions will not be affected.</p></qt>");
    label = new QLabel(i18nc("@label:textbox", "S/MIME signing certificate:"), mCryptographyTab);
    label->setBuddy(mSMIMESigningKeyRequester);
    mSMIMESigningKeyRequester->setWhatsThis(msg);
    label->setWhatsThis(msg);
    formLayout->addRow(label, mSMIMESigningKeyRequester);

    const QGpgME::Protocol *smimeProtocol = QGpgME::smime();

    label->setEnabled(smimeProtocol);
    mSMIMESigningKeyRequester->setEnabled(smimeProtocol);

    // "S/MIME Encryption Key" requester and label:
    mSMIMEEncryptionKeyRequester = new KeySelectionCombo(KeySelectionCombo::EncryptionKey, GpgME::CMS, mCryptographyTab);
    msg = i18n(
        "<qt><p>The S/MIME certificate you choose here will be used "
        "to encrypt messages to yourself and for the \"Attach My Certificate\" "
        "feature in the composer.</p>"
        "<p>You can leave this blank, but KMail will not be able "
        "to encrypt copies of outgoing messages to you using S/MIME; "
        "normal mail functions will not be affected.</p></qt>");
    label = new QLabel(i18nc("@label:textbox", "S/MIME encryption certificate:"), mCryptographyTab);
    label->setBuddy(mSMIMEEncryptionKeyRequester);
    mSMIMEEncryptionKeyRequester->setWhatsThis(msg);
    label->setWhatsThis(msg);

    formLayout->addRow(label, mSMIMEEncryptionKeyRequester);

    label->setEnabled(smimeProtocol);
    mSMIMEEncryptionKeyRequester->setEnabled(smimeProtocol);

    // "Preferred Crypto Message Format" combobox and label:
    mPreferredCryptoMessageFormat = new QComboBox(mCryptographyTab);
    QStringList l;
    l << Kleo::cryptoMessageFormatToLabel(Kleo::AutoFormat) << Kleo::cryptoMessageFormatToLabel(Kleo::InlineOpenPGPFormat)
      << Kleo::cryptoMessageFormatToLabel(Kleo::OpenPGPMIMEFormat) << Kleo::cryptoMessageFormatToLabel(Kleo::SMIMEFormat)
      << Kleo::cryptoMessageFormatToLabel(Kleo::SMIMEOpaqueFormat);
    mPreferredCryptoMessageFormat->addItems(l);
    label = new QLabel(i18nc("preferred format of encrypted messages", "Preferred format:"), mCryptographyTab);
    label->setBuddy(mPreferredCryptoMessageFormat);

    formLayout->addRow(label, mPreferredCryptoMessageFormat);

    mAutocrypt = new QGroupBox(i18n("Enable Autocrypt"));
    mAutocrypt->setCheckable(true);
    mAutocrypt->setChecked(true);

    label = new QLabel(i18nc("@label:textbox", "Autocrypt:"), tab);
    formLayout->addRow(label, mAutocrypt);

    vlay = new QVBoxLayout(mAutocrypt);

    mAutocryptPrefer = new QCheckBox(i18nc("@option:check", "Let others know you prefer encryption"));
    vlay->addWidget(mAutocryptPrefer);

    mOverrideDefault = new QGroupBox(i18n("Overwrite global settings for security defaults"));
    mOverrideDefault->setCheckable(true);
    mOverrideDefault->setChecked(false);
    label = new QLabel(i18nc("@label:textbox", "Overwrite defaults:"), tab);
    formLayout->addRow(label, mOverrideDefault);

    vlay = new QVBoxLayout(mOverrideDefault);

    mAutoSign = new QCheckBox(i18nc("@option:check", "Sign messages"), tab);
    vlay->addWidget(mAutoSign);

    mAutoEncrypt = new QCheckBox(i18nc("@option:check", "Encrypt messages when possible"), tab);
    vlay->addWidget(mAutoEncrypt);

    mWarnNotSign = new QCheckBox(i18nc("@option:check", "Warn when trying to send unsigned messages"), tab);
    vlay->addWidget(mWarnNotSign);

    mWarnNotEncrypt = new QCheckBox(i18nc("@option:check", "Warn when trying to send unencrypted messages"), tab);
    vlay->addWidget(mWarnNotEncrypt);

    //
    // Tab Widget: Advanced
    //
    tab = new QWidget(mTabWidget);
    auto advancedMainLayout = new QVBoxLayout(tab);
    mIdentityInvalidFolder = new IdentityInvalidFolder(tab);
    advancedMainLayout->addWidget(mIdentityInvalidFolder);
    mTabWidget->addTab(tab, i18nc("@title:tab Advanced identity settings.", "Advanced"));
    formLayout = new QFormLayout;
    advancedMainLayout->addLayout(formLayout);

    // "Reply-To Address" line edit and label:
    mReplyToEdit = new PimCommon::AddresseeLineEdit(tab, true);
    mReplyToEdit->setClearButtonEnabled(true);
    mReplyToEdit->setObjectName("mReplyToEdit"_L1);
    label = new QLabel(i18nc("@label:textbox", "&Reply-To address:"), tab);
    label->setBuddy(mReplyToEdit);
    formLayout->addRow(label, mReplyToEdit);
    msg = i18n(
        "<qt><h3>Reply-To addresses</h3>"
        "<p>This sets the <tt>Reply-to:</tt> header to contain a "
        "different email address to the normal <tt>From:</tt> "
        "address.</p>"
        "<p>This can be useful when you have a group of people "
        "working together in similar roles. For example, you "
        "might want any emails sent to have your email in the "
        "<tt>From:</tt> field, but any responses to go to "
        "a group address.</p>"
        "<p>If in doubt, leave this field blank.</p></qt>");
    label->setWhatsThis(msg);
    mReplyToEdit->setWhatsThis(msg);
    KLineEditEventHandler::catchReturnKey(mReplyToEdit);

    // "CC addresses" line edit and label:
    mCcEdit = new PimCommon::AddresseeLineEdit(tab, true);
    mCcEdit->setClearButtonEnabled(true);
    mCcEdit->setObjectName("mCcEdit"_L1);
    label = new QLabel(i18nc("@label:textbox", "&CC addresses:"), tab);
    label->setBuddy(mCcEdit);
    formLayout->addRow(label, mCcEdit);
    KLineEditEventHandler::catchReturnKey(mCcEdit);

    msg = i18n(
        "<qt><h3>CC (Carbon Copy) addresses</h3>"
        "<p>The addresses that you enter here will be added to each "
        "outgoing mail that is sent with this identity.</p>"
        "<p>This is commonly used to send a copy of each sent message to "
        "another account of yours.</p>"
        "<p>To specify more than one address, use commas to separate "
        "the list of CC recipients.</p>"
        "<p>If in doubt, leave this field blank.</p></qt>");
    label->setWhatsThis(msg);
    mCcEdit->setWhatsThis(msg);

    // "BCC addresses" line edit and label:
    mBccEdit = new PimCommon::AddresseeLineEdit(tab, true);
    mBccEdit->setClearButtonEnabled(true);
    mBccEdit->setObjectName("mBccEdit"_L1);
    KLineEditEventHandler::catchReturnKey(mBccEdit);
    label = new QLabel(i18nc("@label:textbox", "&BCC addresses:"), tab);
    label->setBuddy(mBccEdit);
    formLayout->addRow(label, mBccEdit);
    msg = i18n(
        "<qt><h3>BCC (Blind Carbon Copy) addresses</h3>"
        "<p>The addresses that you enter here will be added to each "
        "outgoing mail that is sent with this identity. They will not "
        "be visible to other recipients.</p>"
        "<p>This is commonly used to send a copy of each sent message to "
        "another account of yours.</p>"
        "<p>To specify more than one address, use commas to separate "
        "the list of BCC recipients.</p>"
        "<p>If in doubt, leave this field blank.</p></qt>");
    label->setWhatsThis(msg);
    mBccEdit->setWhatsThis(msg);

    // "Dictionary" combo box and label:
    mDictionaryCombo = new Sonnet::DictionaryComboBox(tab);
    label = new QLabel(i18nc("@label:textbox", "D&ictionary:"), tab);
    label->setBuddy(mDictionaryCombo);
    formLayout->addRow(label, mDictionaryCombo);

    // "Sent-mail Folder" combo box and label:
    mFccFolderRequester = new IdentityFolderRequester(tab);
    mFccFolderRequester->setSelectFolderTitleDialog(i18n("Select Send-mail Folder"));
    mFccFolderRequester->setShowOutbox(false);
    mSentMailFolderCheck = new QCheckBox(i18nc("@option:check", "Sent-mail &folder:"), tab);
    connect(mSentMailFolderCheck, &QCheckBox::toggled, mFccFolderRequester, &MailCommon::FolderRequester::setEnabled);
    formLayout->addRow(mSentMailFolderCheck, mFccFolderRequester);

    // "Drafts Folder" combo box and label:
    mDraftsFolderRequester = new IdentityFolderRequester(tab);
    mDraftsFolderRequester->setSelectFolderTitleDialog(i18n("Select Draft Folder"));
    mDraftsFolderRequester->setShowOutbox(false);
    label = new QLabel(i18nc("@label:textbox", "&Drafts folder:"), tab);
    label->setBuddy(mDraftsFolderRequester);
    formLayout->addRow(label, mDraftsFolderRequester);

    // "Templates Folder" combo box and label:
    mTemplatesFolderRequester = new IdentityFolderRequester(tab);
    mTemplatesFolderRequester->setSelectFolderTitleDialog(i18n("Select Templates Folder"));
    mTemplatesFolderRequester->setShowOutbox(false);
    label = new QLabel(i18nc("@label:textbox", "&Templates folder:"), tab);
    label->setBuddy(mTemplatesFolderRequester);
    formLayout->addRow(label, mTemplatesFolderRequester);

    // "Special transport" combobox and label:
    mTransportCheck = new QCheckBox(i18nc("@option:check", "Outgoing Account:"), tab);
    mTransportCombo = new TransportComboBox(tab);
    mTransportCombo->setEnabled(false); // since !mTransportCheck->isChecked()
    formLayout->addRow(mTransportCheck, mTransportCombo);

    connect(mTransportCheck, &QCheckBox::toggled, mTransportCombo, &MailTransport::TransportComboBox::setEnabled);

    mAttachMyVCard = new QCheckBox(i18nc("@option:check", "Attach my vCard to message"), tab);
    mEditVCard = new QPushButton(i18nc("@action:button", "Create…"), tab);
    connect(mEditVCard, &QPushButton::clicked, this, &IdentityDialog::slotEditVcard);
    formLayout->addRow(mAttachMyVCard, mEditVCard);
    mAutoCorrectionLanguage = new TextAutoCorrectionWidgets::AutoCorrectionLanguage(tab);
    label = new QLabel(i18nc("@label:textbox", "Autocorrection language:"), tab);
    label->setBuddy(mAutoCorrectionLanguage);
    formLayout->addRow(label, mAutoCorrectionLanguage);

    // "default domain" input field:
    auto hbox = new QHBoxLayout;
    mDefaultDomainEdit = new QLineEdit(tab);
    KLineEditEventHandler::catchReturnKey(mDefaultDomainEdit);
    mDefaultDomainEdit->setClearButtonEnabled(true);
    hbox->addWidget(mDefaultDomainEdit);
    auto restoreDefaultDomainName = new QToolButton;
    restoreDefaultDomainName->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    restoreDefaultDomainName->setToolTip(i18nc("@info:tooltip", "Restore default domain name"));
    hbox->addWidget(restoreDefaultDomainName);
    connect(restoreDefaultDomainName, &QToolButton::clicked, this, &IdentityDialog::slotRefreshDefaultDomainName);
    label = new QLabel(i18nc("@label:textbox", "Defaul&t domain:"), tab);
    label->setBuddy(mDefaultDomainEdit);
    formLayout->addRow(label, mDefaultDomainEdit);

    // and now: add QWhatsThis:
    msg = i18n(
        "<qt><p>The default domain is used to complete email "
        "addresses that only consist of the user's name."
        "</p></qt>");
    label->setWhatsThis(msg);
    mDefaultDomainEdit->setWhatsThis(msg);

    // the last row is a spacer

    //
    // Tab Widget: Templates
    //
    tab = new QWidget(mTabWidget);
    vlay = new QVBoxLayout(tab);

    auto tlay = new QHBoxLayout();
    vlay->addLayout(tlay);

    mCustom = new QCheckBox(i18nc("@option:check", "&Use custom message templates for this identity"), tab);
    tlay->addWidget(mCustom, Qt::AlignLeft);

    mWidget = new TemplateParser::TemplatesConfiguration(tab, QStringLiteral("identity-templates"));
    mWidget->setEnabled(false);

    // Move the help label outside of the templates configuration widget,
    // so that the help can be read even if the widget is not enabled.
    tlay->addStretch(9);
    tlay->addWidget(mWidget->helpLabel(), Qt::AlignRight);

    vlay->addWidget(mWidget);

    auto btns = new QHBoxLayout();
    mCopyGlobal = new QPushButton(i18nc("@action:button", "&Copy Global Templates"), tab);
    mCopyGlobal->setEnabled(false);
    btns->addWidget(mCopyGlobal);
    vlay->addLayout(btns);
    connect(mCustom, &QCheckBox::toggled, mWidget, &TemplateParser::TemplatesConfiguration::setEnabled);
    connect(mCustom, &QCheckBox::toggled, mCopyGlobal, &QPushButton::setEnabled);
    connect(mCopyGlobal, &QPushButton::clicked, this, &IdentityDialog::slotCopyGlobal);
    mTabWidget->addTab(tab, i18n("Templates"));

    //
    // Tab Widget: Signature
    //
    mSignatureConfigurator = new KIdentityManagementWidgets::SignatureConfigurator(mTabWidget);
    mTabWidget->addTab(mSignatureConfigurator, i18n("Signature"));

    //
    // Tab Widget: Picture
    //

    mXFaceConfigurator = new XFaceConfigurator(mTabWidget);
    mTabWidget->addTab(mXFaceConfigurator, i18n("Picture"));

    resize(KMailSettings::self()->identityDialogSize());
    mNameEdit->setFocus();

#if KMAIL_HAVE_ACTIVITY_SUPPORT
    // TODO show/hide when we activate or not plasma activities
    mTabWidget->addTab(mConfigureActivitiesWidget, i18n("Activities"));
#endif
    connect(mTabWidget, &QTabWidget::currentChanged, this, &IdentityDialog::slotAboutToShow);
}

IdentityDialog::~IdentityDialog()
{
    KMailSettings::self()->setIdentityDialogSize(size());
}

void IdentityDialog::slotHelp()
{
    PimCommon::Util::invokeHelp(QStringLiteral("kmail2/configure-identity.html"));
}

void IdentityDialog::slotAboutToShow(int index)
{
    QWidget *w = mTabWidget->widget(index);
    if (w == mCryptographyTab) {
        // set the configured email address as initial query of the key
        // requesters:
        const QString name = mNameEdit->text().trimmed();
        const QString email = mEmailEdit->text().trimmed();

        mPGPEncryptionKeyRequester->setIdentity(name, email);
        mPGPSigningKeyRequester->setIdentity(name, email);
        mSMIMEEncryptionKeyRequester->setIdentity(name, email);
        mSMIMESigningKeyRequester->setIdentity(name, email);
    }
}

void IdentityDialog::slotCopyGlobal()
{
    mWidget->loadFromGlobal();
}

void IdentityDialog::slotRefreshDefaultDomainName()
{
    mDefaultDomainEdit->setText(QHostInfo::localHostName());
}

void IdentityDialog::slotAccepted()
{
    const QStringList aliases = mAliasEdit->items();
    for (const QString &alias : aliases) {
        if (alias.trimmed().isEmpty()) {
            continue;
        }
        if (!KEmailAddress::isValidSimpleAddress(alias)) {
            const QString errorMsg(KEmailAddress::simpleEmailAddressErrorMsg());
            KMessageBox::error(this, errorMsg, i18n("Invalid Email Alias \"%1\"", alias));
            return;
        }
    }

    // Validate email addresses
    const QString email = mEmailEdit->text().trimmed();
    if (email.isEmpty()) {
        KMessageBox::error(this, i18n("You must provide an email for this identity."), i18nc("@title:window", "Empty Email Address"));
        return;
    }
    if (!KEmailAddress::isValidSimpleAddress(email)) {
        const QString errorMsg(KEmailAddress::simpleEmailAddressErrorMsg());
        KMessageBox::error(this, errorMsg, i18nc("@title:window", "Invalid Email Address"));
        return;
    }

    // Check if the 'Reply to' and 'BCC' recipients are valid
    const QString recipients = mReplyToEdit->text().trimmed() + ", "_L1 + mBccEdit->text().trimmed() + ", "_L1 + mCcEdit->text().trimmed();
    auto job = new AddressValidationJob(recipients, this, this);
    // Use default Value
    job->setDefaultDomain(mDefaultDomainEdit->text());
    job->setProperty("email", email);
    connect(job, &AddressValidationJob::result, this, &IdentityDialog::slotDelayedButtonClicked);
    job->start();
    // TODO save activities
}

bool IdentityDialog::keyMatchesEmailAddress(const GpgME::Key &key, const QString &email_)
{
    if (key.isNull()) {
        return true;
    }
    const QString email = email_.trimmed().toLower();
    const auto uids = key.userIDs();
    for (const auto &uid : uids) {
        QString em = QString::fromUtf8(uid.email() ? uid.email() : uid.id());
        if (em.isEmpty()) {
            continue;
        }
        if (em[0] == QLatin1Char('<')) {
            em = em.mid(1, em.length() - 2);
        }
        if (em.toLower() == email) {
            return true;
        }
    }

    return false;
}

void IdentityDialog::slotDelayedButtonClicked(KJob *job)
{
    const AddressValidationJob *validationJob = qobject_cast<AddressValidationJob *>(job);

    // Abort if one of the recipient addresses is invalid
    if (!validationJob->isValid()) {
        return;
    }

    const QString email = validationJob->property("email").toString();

    const GpgME::Key &pgpSigningKey = mPGPSigningKeyRequester->currentKey();
    const GpgME::Key &pgpEncryptionKey = mPGPEncryptionKeyRequester->currentKey();
    const GpgME::Key &smimeSigningKey = mSMIMESigningKeyRequester->currentKey();
    const GpgME::Key &smimeEncryptionKey = mSMIMEEncryptionKeyRequester->currentKey();

    QString msg;
    bool err = false;
    if (!keyMatchesEmailAddress(pgpSigningKey, email)) {
        msg = i18n(
            "One of the configured OpenPGP signing keys does not contain "
            "any user ID with the configured email address for this "
            "identity (%1).\n"
            "This might result in warning messages on the receiving side "
            "when trying to verify signatures made with this configuration.",
            email);
        err = true;
    } else if (!keyMatchesEmailAddress(pgpEncryptionKey, email)) {
        msg = i18n(
            "One of the configured OpenPGP encryption keys does not contain "
            "any user ID with the configured email address for this "
            "identity (%1).",
            email);
        err = true;
    } else if (!keyMatchesEmailAddress(smimeSigningKey, email)) {
        msg = i18n(
            "One of the configured S/MIME signing certificates does not contain "
            "the configured email address for this "
            "identity (%1).\n"
            "This might result in warning messages on the receiving side "
            "when trying to verify signatures made with this configuration.",
            email);
        err = true;
    } else if (!keyMatchesEmailAddress(smimeEncryptionKey, email)) {
        msg = i18n(
            "One of the configured S/MIME encryption certificates does not contain "
            "the configured email address for this "
            "identity (%1).",
            email);
        err = true;
    }

    if (err) {
        if (KMessageBox::warningContinueCancel(this,
                                               msg,
                                               i18nc("@title:window", "Email Address Not Found in Key/Certificates"),
                                               KStandardGuiItem::cont(),
                                               KStandardGuiItem::cancel(),
                                               QStringLiteral("warn_email_not_in_certificate"))
            != KMessageBox::Continue) {
            return;
        }
    }

    if (mSignatureConfigurator->isSignatureEnabled() && mSignatureConfigurator->signatureType() == Signature::FromFile) {
        QFileInfo file(mSignatureConfigurator->filePath());
        if (!file.isReadable()) {
            KMessageBox::error(this, i18n("The signature file is not valid"));
            return;
        }
    }
    accept();
}

bool IdentityDialog::checkFolderExists(const QString &folderID)
{
    const Akonadi::Collection folder = CommonKernel->collectionFromId(folderID.toLongLong());
    return folder.isValid();
}

void IdentityDialog::setIdentity(KIdentityManagementCore::Identity &ident)
{
    setWindowTitle(i18nc("@title:window", "Edit Identity \"%1\"", ident.identityName()));

    // "General" tab:
    mNameEdit->setText(ident.fullName());
    mOrganizationEdit->setText(ident.organization());
    mEmailEdit->setText(ident.primaryEmailAddress());
    mAliasEdit->insertStringList(ident.emailAliases());

    // "Cryptography" tab:
    mPGPSigningKeyRequester->setDefaultKey(QLatin1StringView(ident.pgpSigningKey()));
    mPGPEncryptionKeyRequester->setDefaultKey(QLatin1StringView(ident.pgpEncryptionKey()));

    mPGPSameKey->setChecked(ident.pgpSigningKey() == ident.pgpEncryptionKey());

    mSMIMESigningKeyRequester->setDefaultKey(QLatin1StringView(ident.smimeSigningKey()));
    mSMIMEEncryptionKeyRequester->setDefaultKey(QLatin1StringView(ident.smimeEncryptionKey()));

    mPreferredCryptoMessageFormat->setCurrentIndex(format2cb(Kleo::stringToCryptoMessageFormat(ident.preferredCryptoMessageFormat())));
    mAutocrypt->setChecked(ident.autocryptEnabled());
    mAutocryptPrefer->setChecked(ident.autocryptPrefer());
    mOverrideDefault->setChecked(ident.encryptionOverride());
    if (!ident.encryptionOverride()) {
        mAutoSign->setChecked(ident.pgpAutoSign());
        mAutoEncrypt->setChecked(ident.pgpAutoEncrypt());
        mWarnNotSign->setChecked(ident.warnNotSign());
        mWarnNotEncrypt->setChecked(ident.warnNotEncrypt());
    } else {
        mAutoEncrypt->setChecked(MessageComposer::MessageComposerSettings::self()->cryptoAutoEncrypt());
        mAutoSign->setChecked(MessageComposer::MessageComposerSettings::self()->cryptoAutoSign());
        mWarnNotEncrypt->setChecked(MessageComposer::MessageComposerSettings::self()->cryptoWarningUnencrypted());
        mWarnNotSign->setChecked(MessageComposer::MessageComposerSettings::self()->cryptoWarningUnsigned());
    }

    // "Advanced" tab:
    mReplyToEdit->setText(ident.replyToAddr());
    mBccEdit->setText(ident.bcc());
    mCcEdit->setText(ident.cc());
    const int transportId = ident.transport().isEmpty() ? -1 : ident.transport().toInt();
    const Transport *transport = TransportManager::self()->transportById(transportId, true);
    mTransportCheck->setChecked(transportId != -1);
    mTransportCombo->setEnabled(transportId != -1);
    if (transport) {
        mTransportCombo->setCurrentTransport(transport->id());
    }
    mDictionaryCombo->setCurrentByDictionaryName(ident.dictionary());

    mSentMailFolderCheck->setChecked(!ident.disabledFcc());
    mFccFolderRequester->setEnabled(mSentMailFolderCheck->isChecked());
    bool foundNoExistingFolder = false;
    if (ident.fcc().isEmpty() || !checkFolderExists(ident.fcc())) {
        foundNoExistingFolder = true;
        mFccFolderRequester->setIsInvalidFolder(CommonKernel->sentCollectionFolder());
    } else {
        mFccFolderRequester->setCollection(Akonadi::Collection(ident.fcc().toLongLong()));
    }
    if (ident.drafts().isEmpty() || !checkFolderExists(ident.drafts())) {
        foundNoExistingFolder = true;
        mDraftsFolderRequester->setIsInvalidFolder(CommonKernel->draftsCollectionFolder());
    } else {
        mDraftsFolderRequester->setCollection(Akonadi::Collection(ident.drafts().toLongLong()));
    }

    if (ident.templates().isEmpty() || !checkFolderExists(ident.templates())) {
        foundNoExistingFolder = true;
        mTemplatesFolderRequester->setIsInvalidFolder(CommonKernel->templatesCollectionFolder());
    } else {
        mTemplatesFolderRequester->setCollection(Akonadi::Collection(ident.templates().toLongLong()));
    }
    if (foundNoExistingFolder) {
        mIdentityInvalidFolder->setErrorMessage(i18n("Some custom folder for identity does not exist (anymore); therefore, default folders will be used."));
    }
    mVcardFilename = ident.vCardFile();

    mAutoCorrectionLanguage->setLanguage(ident.autocorrectionLanguage());
    updateVcardButton();
    if (mVcardFilename.isEmpty()) {
        mVcardFilename = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QLatin1Char('/') + ident.identityName() + ".vcf"_L1;
        QFileInfo fileInfo(mVcardFilename);
        QDir().mkpath(fileInfo.absolutePath());
    } else {
        // Convert path.
        const QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QLatin1Char('/') + ident.identityName() + ".vcf"_L1;
        if (QFileInfo::exists(path) && (mVcardFilename != path)) {
            mVcardFilename = path;
        }
    }
    mAttachMyVCard->setChecked(ident.attachVcard());
    QString defaultDomainName = ident.defaultDomainName();
    if (defaultDomainName.isEmpty()) {
        defaultDomainName = QHostInfo::localHostName();
    }
    mDefaultDomainEdit->setText(defaultDomainName);

    // "Templates" tab:
    uint identity = ident.uoid();
    QString iid = TemplateParser::TemplatesConfiguration::configIdString(identity);
    TemplateParser::Templates t(iid);
    mCustom->setChecked(t.useCustomTemplates());
    mWidget->loadFromIdentity(identity);

    // "Signature" tab:
    mSignatureConfigurator->setImageLocation(ident);
    mSignatureConfigurator->setSignature(ident.signature());
    mXFaceConfigurator->setXFace(ident.xface());
    mXFaceConfigurator->setXFaceEnabled(ident.isXFaceEnabled());
    mXFaceConfigurator->setFace(ident.face());
    mXFaceConfigurator->setFaceEnabled(ident.isFaceEnabled());
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    const PimCommonActivities::ActivitiesBaseManager::ActivitySettings settings{ident.activities(), ident.enabledActivities()};
    mConfigureActivitiesWidget->setActivitiesSettings(settings);
#endif
}

void IdentityDialog::unregisterSpecialCollection(qint64 colId)
{
    // ID is not enough to unregister a special collection, we need the
    // resource set as well.
    auto fetch = new Akonadi::CollectionFetchJob(Akonadi::Collection(colId), Akonadi::CollectionFetchJob::Base, this);
    connect(fetch, &Akonadi::CollectionFetchJob::collectionsReceived, this, [](const Akonadi::Collection::List &cols) {
        if (cols.count() != 1) {
            return;
        }
        Akonadi::SpecialMailCollections::self()->unregisterCollection(cols.first());
    });
}

void IdentityDialog::updateIdentity(KIdentityManagementCore::Identity &ident)
{
    // TODO load plasma activities
    // "General" tab:
    ident.setFullName(mNameEdit->text());
    ident.setOrganization(mOrganizationEdit->text());
    QString email = mEmailEdit->text().trimmed();
    ident.setPrimaryEmailAddress(email);
    const QStringList aliases = mAliasEdit->items();
    QStringList result;
    for (const QString &alias : aliases) {
        const QString aliasTrimmed = alias.trimmed();
        if (aliasTrimmed.isEmpty()) {
            continue;
        }
        if (aliasTrimmed == email) {
            continue;
        }
        result.append(alias);
    }
    ident.setEmailAliases(result);
    // "Cryptography" tab:
    ident.setPGPSigningKey(mPGPSigningKeyRequester->currentKey().primaryFingerprint());
    ident.setPGPEncryptionKey(mPGPEncryptionKeyRequester->currentKey().primaryFingerprint());
    ident.setSMIMESigningKey(mSMIMESigningKeyRequester->currentKey().primaryFingerprint());
    ident.setSMIMEEncryptionKey(mSMIMEEncryptionKeyRequester->currentKey().primaryFingerprint());
    ident.setPreferredCryptoMessageFormat(QLatin1StringView(Kleo::cryptoMessageFormatToString(cb2format(mPreferredCryptoMessageFormat->currentIndex()))));
    ident.setAutocryptEnabled(mAutocrypt->isChecked());
    ident.setAutocryptPrefer(mAutocryptPrefer->isChecked());
    ident.setEncryptionOverride(mOverrideDefault->isChecked());
    ident.setPgpAutoSign(mAutoSign->isChecked());
    ident.setPgpAutoEncrypt(mAutoEncrypt->isChecked());
    ident.setWarnNotEncrypt(mWarnNotEncrypt->isChecked());
    ident.setWarnNotEncrypt(mWarnNotEncrypt->isChecked());
    // "Advanced" tab:
    ident.setReplyToAddr(mReplyToEdit->text());
    ident.setBcc(mBccEdit->text());
    ident.setCc(mCcEdit->text());
    ident.setTransport(mTransportCheck->isChecked() ? QString::number(mTransportCombo->currentTransportId()) : QString());
    ident.setDictionary(mDictionaryCombo->currentDictionaryName());
    ident.setDisabledFcc(!mSentMailFolderCheck->isChecked());
    Akonadi::Collection collection = mFccFolderRequester->collection();
    if (!ident.fcc().isEmpty()) {
        unregisterSpecialCollection(ident.fcc().toLongLong());
    }
    if (collection.isValid()) {
        ident.setFcc(QString::number(collection.id()));
        auto attribute = collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        attribute->setIconName(QStringLiteral("mail-folder-sent"));
        // It will also start a CollectionModifyJob
        Akonadi::SpecialMailCollections::self()->registerCollection(Akonadi::SpecialMailCollections::SentMail, collection);
    } else {
        ident.setFcc(QString());
    }

    collection = mDraftsFolderRequester->collection();
    if (!ident.drafts().isEmpty()) {
        unregisterSpecialCollection(ident.drafts().toLongLong());
    }
    if (collection.isValid()) {
        ident.setDrafts(QString::number(collection.id()));
        auto attribute = collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        attribute->setIconName(QStringLiteral("document-properties"));
        // It will also start a CollectionModifyJob
        Akonadi::SpecialMailCollections::self()->registerCollection(Akonadi::SpecialMailCollections::Drafts, collection);
    } else {
        ident.setDrafts(QString());
    }

    collection = mTemplatesFolderRequester->collection();
    if (ident.templates().isEmpty()) {
        unregisterSpecialCollection(ident.templates().toLongLong());
    }
    if (collection.isValid()) {
        ident.setTemplates(QString::number(collection.id()));
        auto attribute = collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        attribute->setIconName(QStringLiteral("document-new"));
        // It will also start a CollectionModifyJob
        Akonadi::SpecialMailCollections::self()->registerCollection(Akonadi::SpecialMailCollections::Templates, collection);
        new Akonadi::CollectionModifyJob(collection);
    } else {
        ident.setTemplates(QString());
    }
    ident.setVCardFile(mVcardFilename);
    ident.setAutocorrectionLanguage(mAutoCorrectionLanguage->language());
    updateVcardButton();
    ident.setAttachVcard(mAttachMyVCard->isChecked());
    // Add default ?
    ident.setDefaultDomainName(mDefaultDomainEdit->text());

    // "Templates" tab:
    uint identity = ident.uoid();
    QString iid = TemplateParser::TemplatesConfiguration::configIdString(identity);
    TemplateParser::Templates t(iid);
    qCDebug(KMAIL_LOG) << "use custom templates for identity" << identity << ":" << mCustom->isChecked();
    t.setUseCustomTemplates(mCustom->isChecked());
    t.save();
    mWidget->saveToIdentity(identity);

    // "Signature" tab:
    ident.setSignature(mSignatureConfigurator->signature());
    ident.setXFace(mXFaceConfigurator->xface());
    ident.setXFaceEnabled(mXFaceConfigurator->isXFaceEnabled());
    ident.setFace(mXFaceConfigurator->face());
    ident.setFaceEnabled(mXFaceConfigurator->isFaceEnabled());

#if KMAIL_HAVE_ACTIVITY_SUPPORT
    const PimCommonActivities::ActivitiesBaseManager::ActivitySettings settings = mConfigureActivitiesWidget->activitiesSettings();
    ident.setActivities(settings.activities);
    ident.setEnabledActivities(settings.enabled);
#endif
}

void IdentityDialog::slotEditVcard()
{
    if (QFileInfo::exists(mVcardFilename)) {
        editVcard(mVcardFilename);
    } else {
        if (!MailCommon::Kernel::self()->kernelIsRegistered()) {
            return;
        }
        KIdentityManagementCore::IdentityManager *manager = KernelIf->identityManager();

        QPointer<IdentityAddVcardDialog> dlg = new IdentityAddVcardDialog(manager->shadowIdentities(), this);
        if (dlg->exec()) {
            IdentityAddVcardDialog::DuplicateMode mode = dlg->duplicateMode();
            switch (mode) {
            case IdentityAddVcardDialog::Empty:
                editVcard(mVcardFilename);
                break;
            case IdentityAddVcardDialog::ExistingEntry: {
                KIdentityManagementCore::Identity ident = manager->modifyIdentityForName(dlg->duplicateVcardFromIdentity());
                const QString filename = ident.vCardFile();
                if (!filename.isEmpty()) {
                    QFile::copy(filename, mVcardFilename);
                }
                editVcard(mVcardFilename);
                break;
            }
            case IdentityAddVcardDialog::FromExistingVCard: {
                const QString filename = dlg->existingVCard().path();
                if (!filename.isEmpty()) {
                    mVcardFilename = filename;
                }
                editVcard(mVcardFilename);
                break;
            }
            }
        }
        delete dlg;
    }
}

void IdentityDialog::editVcard(const QString &filename)
{
    QPointer<IdentityEditVcardDialog> dlg = new IdentityEditVcardDialog(filename, this);
    connect(dlg.data(), &IdentityEditVcardDialog::vcardRemoved, this, &IdentityDialog::slotVCardRemoved);
    if (dlg->exec()) {
        mVcardFilename = dlg->saveVcard();
    }
    updateVcardButton();
    delete dlg;
}

void IdentityDialog::slotVCardRemoved()
{
    mVcardFilename.clear();
}

void IdentityDialog::updateVcardButton()
{
    if (mVcardFilename.isEmpty() || !QFileInfo::exists(mVcardFilename)) {
        mEditVCard->setText(i18n("Create…"));
    } else {
        mEditVCard->setText(i18n("Edit…"));
    }
}
}

#include "identitydialog.moc"

#include "moc_identitydialog.cpp"
