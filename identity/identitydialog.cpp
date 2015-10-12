/*
    identitydialog.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>
    Copyright (c) 2014-2015 Laurent Montel <montel@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "identitydialog.h"
#include "identityeditvcarddialog.h"
#include "identityaddvcarddialog.h"

#include "MessageComposer/MessageComposerSettings"

#include <KIdentityManagement/kidentitymanagement/identitymanager.h>

// other KMail headers:
#include "xfaceconfigurator.h"
#include <KEditListWidget>
#include "mailcommon/folderrequester.h"
#ifndef KCM_KPIMIDENTITIES_STANDALONE
#include "settings/kmailsettings.h"
#include "kmkernel.h"
#endif

#include "mailcommon/mailkernel.h"

#include "job/addressvalidationjob.h"
#include "MessageComposer/Kleo_Util"
#include "MessageCore/StringUtil"
#include "TemplateParser/TemplatesConfiguration"
#include "templatesconfiguration_kfg.h"
// other kdepim headers:
#include <KIdentityManagement/kidentitymanagement/identity.h>
#include <KIdentityManagement/kidentitymanagement/signatureconfigurator.h>

#include "PimCommon/AutoCorrectionLanguage"

#include <Libkdepim/AddresseeLineEdit>
// libkleopatra:
#include "Libkleo/KeyRequester"
#include "Libkleo/CryptoBackendFactory"

#include <KEmailAddress>
#include <Libkdepim/EmailValidator>
#include <MailTransport/mailtransport/transport.h>
#include <MailTransport/mailtransport/transportmanager.h>
#include <MailTransport/mailtransport/transportcombobox.h>
using MailTransport::TransportManager;

// other KDE headers:
#include <KLocalizedString>
#include <kmessagebox.h>
#include <kfileitem.h>
#include <qurl.h>
#include "kmail_debug.h"
#include <QPushButton>
#include <kcombobox.h>
#include <QTabWidget>
#include <QIcon>
#include <sonnet/dictionarycombobox.h>
#include <KHelpClient>

// Qt headers:
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFile>
#include <QHostInfo>
#include <QToolButton>
#include <QFileInfo>
#include <QDir>

// other headers:
#include <gpgme++/key.h>
#include <iterator>
#include <algorithm>

#include <AkonadiCore/entitydisplayattribute.h>
#include <AkonadiCore/collectionmodifyjob.h>
#include <QStandardPaths>
#include <KConfigGroup>
#include <QDialogButtonBox>

using namespace KPIM;
using namespace MailTransport;
using namespace MailCommon;

namespace KMail
{

IdentityDialog::IdentityDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Edit Identity"));
    QDialogButtonBox *buttonBox = Q_NULLPTR;
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    connect(buttonBox->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &IdentityDialog::slotHelp);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IdentityDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IdentityDialog::reject);

    okButton->setDefault(true);

    // tmp. vars:
    QWidget *tab;
    QLabel   *label;
    int row;
    QGridLayout *glay;
    QString msg;

    //
    // Tab Widget: General
    //
    row = -1;
    QWidget *page = new QWidget(this);
    mainLayout->addWidget(page);
    mainLayout->addWidget(buttonBox);
    QVBoxLayout *vlay = new QVBoxLayout(page);
    vlay->setMargin(0);
    mTabWidget = new QTabWidget(page);
    mTabWidget->setObjectName(QStringLiteral("config-identity-tab"));
    vlay->addWidget(mTabWidget);

    tab = new QWidget(mTabWidget);
    mTabWidget->addTab(tab, i18nc("@title:tab General identity settings.", "General"));
    glay = new QGridLayout(tab);
    glay->setRowStretch(3, 1);
    glay->setColumnStretch(1, 1);

    // "Name" line edit and label:
    ++row;
    mNameEdit = new KLineEdit(tab);
    glay->addWidget(mNameEdit, row, 1);
    label = new QLabel(i18n("&Your name:"), tab);
    label->setBuddy(mNameEdit);
    glay->addWidget(label, row, 0);
    msg = i18n("<qt><h3>Your name</h3>"
               "<p>This field should contain your name as you would like "
               "it to appear in the email header that is sent out;</p>"
               "<p>if you leave this blank your real name will not "
               "appear, only the email address.</p></qt>");
    label->setWhatsThis(msg);
    mNameEdit->setWhatsThis(msg);

    // "Organization" line edit and label:
    ++row;
    mOrganizationEdit = new KLineEdit(tab);
    glay->addWidget(mOrganizationEdit, row, 1);
    label =  new QLabel(i18n("Organi&zation:"), tab);
    label->setBuddy(mOrganizationEdit);
    glay->addWidget(label, row, 0);
    msg = i18n("<qt><h3>Organization</h3>"
               "<p>This field should have the name of your organization "
               "if you would like it to be shown in the email header that "
               "is sent out.</p>"
               "<p>It is safe (and normal) to leave this blank.</p></qt>");
    label->setWhatsThis(msg);
    mOrganizationEdit->setWhatsThis(msg);

    // "Email Address" line edit and label:
    // (row 3: spacer)
    ++row;
    mEmailEdit = new KLineEdit(tab);
    glay->addWidget(mEmailEdit, row, 1);
    label = new QLabel(i18n("&Email address:"), tab);
    label->setBuddy(mEmailEdit);
    glay->addWidget(label, row, 0);
    msg = i18n("<qt><h3>Email address</h3>"
               "<p>This field should have your full email address.</p>"
               "<p>This address is the primary one, used for all outgoing mail. "
               "If you have more than one address, either create a new identity, "
               "or add additional alias addresses in the field below.</p>"
               "<p>If you leave this blank, or get it wrong, people "
               "will have trouble replying to you.</p></qt>");
    label->setWhatsThis(msg);
    mEmailEdit->setWhatsThis(msg);

    KPIM::EmailValidator *emailValidator = new KPIM::EmailValidator(this);
    mEmailEdit->setValidator(emailValidator);

    // "Email Aliases" string text edit and label:
    ++row;
    mAliasEdit = new KEditListWidget(tab);
    glay->addWidget(mAliasEdit, row, 1);
    label = new QLabel(i18n("Email a&liases:"), tab);
    label->setBuddy(mAliasEdit);
    glay->addWidget(label, row, 0, Qt::AlignTop);
    msg = i18n("<qt><h3>Email aliases</h3>"
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
    row = -1;
    mCryptographyTab = tab = new QWidget(mTabWidget);
    mTabWidget->addTab(tab, i18n("Cryptography"));
    glay = new QGridLayout(tab);
    glay->setColumnStretch(1, 1);

    // "OpenPGP Signature Key" requester and label:
    ++row;
    mPGPSigningKeyRequester = new Kleo::SigningKeyRequester(false, Kleo::SigningKeyRequester::OpenPGP, tab);
    mPGPSigningKeyRequester->dialogButton()->setText(i18n("Chang&e..."));
    mPGPSigningKeyRequester->setDialogCaption(i18n("Your OpenPGP Signature Key"));
    msg = i18n("Select the OpenPGP key which should be used to "
               "digitally sign your messages.");
    mPGPSigningKeyRequester->setDialogMessage(msg);

    msg = i18n("<qt><p>The OpenPGP key you choose here will be used "
               "to digitally sign messages. You can also use GnuPG keys.</p>"
               "<p>You can leave this blank, but KMail will not be able "
               "to digitally sign emails using OpenPGP; "
               "normal mail functions will not be affected.</p>"
               "<p>You can find out more about keys at <a>http://www.gnupg.org</a></p></qt>");

    label = new QLabel(i18n("OpenPGP signing key:"), tab);
    label->setBuddy(mPGPSigningKeyRequester);
    mPGPSigningKeyRequester->setWhatsThis(msg);
    label->setWhatsThis(msg);

    glay->addWidget(label, row, 0);
    glay->addWidget(mPGPSigningKeyRequester, row, 1);

    // "OpenPGP Encryption Key" requester and label:
    ++row;
    mPGPEncryptionKeyRequester = new Kleo::EncryptionKeyRequester(false, Kleo::EncryptionKeyRequester::OpenPGP, tab);
    mPGPEncryptionKeyRequester->dialogButton()->setText(i18n("Chang&e..."));
    mPGPEncryptionKeyRequester->setDialogCaption(i18n("Your OpenPGP Encryption Key"));
    msg = i18n("Select the OpenPGP key which should be used when encrypting "
               "to yourself and for the \"Attach My Public Key\" "
               "feature in the composer.");
    mPGPEncryptionKeyRequester->setDialogMessage(msg);

    msg = i18n("<qt><p>The OpenPGP key you choose here will be used "
               "to encrypt messages to yourself and for the \"Attach My Public Key\" "
               "feature in the composer. You can also use GnuPG keys.</p>"
               "<p>You can leave this blank, but KMail will not be able "
               "to encrypt copies of outgoing messages to you using OpenPGP; "
               "normal mail functions will not be affected.</p>"
               "<p>You can find out more about keys at <a>http://www.gnupg.org</a></p></qt>");
    label = new QLabel(i18n("OpenPGP encryption key:"), tab);
    label->setBuddy(mPGPEncryptionKeyRequester);
    mPGPEncryptionKeyRequester->setWhatsThis(msg);
    label->setWhatsThis(msg);

    glay->addWidget(label, row, 0);
    glay->addWidget(mPGPEncryptionKeyRequester, row, 1);

    // "S/MIME Signature Key" requester and label:
    ++row;
    mSMIMESigningKeyRequester = new Kleo::SigningKeyRequester(false, Kleo::SigningKeyRequester::SMIME, tab);
    mSMIMESigningKeyRequester->dialogButton()->setText(i18n("Chang&e..."));
    mSMIMESigningKeyRequester->setDialogCaption(i18n("Your S/MIME Signature Certificate"));
    msg = i18n("Select the S/MIME certificate which should be used to "
               "digitally sign your messages.");
    mSMIMESigningKeyRequester->setDialogMessage(msg);

    msg = i18n("<qt><p>The S/MIME (X.509) certificate you choose here will be used "
               "to digitally sign messages.</p>"
               "<p>You can leave this blank, but KMail will not be able "
               "to digitally sign emails using S/MIME; "
               "normal mail functions will not be affected.</p></qt>");
    label = new QLabel(i18n("S/MIME signing certificate:"), tab);
    label->setBuddy(mSMIMESigningKeyRequester);
    mSMIMESigningKeyRequester->setWhatsThis(msg);
    label->setWhatsThis(msg);
    glay->addWidget(label, row, 0);
    glay->addWidget(mSMIMESigningKeyRequester, row, 1);

    const Kleo::CryptoBackend::Protocol *smimeProtocol
        = Kleo::CryptoBackendFactory::instance()->smime();

    label->setEnabled(smimeProtocol);
    mSMIMESigningKeyRequester->setEnabled(smimeProtocol);

    // "S/MIME Encryption Key" requester and label:
    ++row;
    mSMIMEEncryptionKeyRequester = new Kleo::EncryptionKeyRequester(false, Kleo::EncryptionKeyRequester::SMIME, tab);
    mSMIMEEncryptionKeyRequester->dialogButton()->setText(i18n("Chang&e..."));
    mSMIMEEncryptionKeyRequester->setDialogCaption(i18n("Your S/MIME Encryption Certificate"));
    msg = i18n("Select the S/MIME certificate which should be used when encrypting "
               "to yourself and for the \"Attach My Certificate\" "
               "feature in the composer.");
    mSMIMEEncryptionKeyRequester->setDialogMessage(msg);

    msg = i18n("<qt><p>The S/MIME certificate you choose here will be used "
               "to encrypt messages to yourself and for the \"Attach My Certificate\" "
               "feature in the composer.</p>"
               "<p>You can leave this blank, but KMail will not be able "
               "to encrypt copies of outgoing messages to you using S/MIME; "
               "normal mail functions will not be affected.</p></qt>");
    label = new QLabel(i18n("S/MIME encryption certificate:"), tab);
    label->setBuddy(mSMIMEEncryptionKeyRequester);
    mSMIMEEncryptionKeyRequester->setWhatsThis(msg);
    label->setWhatsThis(msg);

    glay->addWidget(label, row, 0);
    glay->addWidget(mSMIMEEncryptionKeyRequester, row, 1);

    label->setEnabled(smimeProtocol);
    mSMIMEEncryptionKeyRequester->setEnabled(smimeProtocol);

    // "Preferred Crypto Message Format" combobox and label:
    ++row;
    mPreferredCryptoMessageFormat = new KComboBox(tab);
    mPreferredCryptoMessageFormat->setEditable(false);
    QStringList l;
    l << Kleo::cryptoMessageFormatToLabel(Kleo::AutoFormat)
      << Kleo::cryptoMessageFormatToLabel(Kleo::InlineOpenPGPFormat)
      << Kleo::cryptoMessageFormatToLabel(Kleo::OpenPGPMIMEFormat)
      << Kleo::cryptoMessageFormatToLabel(Kleo::SMIMEFormat)
      << Kleo::cryptoMessageFormatToLabel(Kleo::SMIMEOpaqueFormat);
    mPreferredCryptoMessageFormat->addItems(l);
    label = new QLabel(i18nc("preferred format of encrypted messages", "Preferred format:"), tab);
    label->setBuddy(mPreferredCryptoMessageFormat);

    glay->addWidget(label, row, 0);
    glay->addWidget(mPreferredCryptoMessageFormat, row, 1);

    ++row;
    mAutoSign = new QCheckBox(i18n("Automatically sign messages"));
    glay->addWidget(mAutoSign, row, 0);

    ++row;
    glay->setRowStretch(row, 1);

    //
    // Tab Widget: Advanced
    //
    row = -1;
    tab = new QWidget(mTabWidget);
    mTabWidget->addTab(tab, i18nc("@title:tab Advanced identity settings.", "Advanced"));
    glay = new QGridLayout(tab);
    // the last (empty) row takes all the remaining space
    glay->setColumnStretch(1, 1);

    // "Reply-To Address" line edit and label:
    ++row;
    mReplyToEdit = new KPIM::AddresseeLineEdit(tab, true);
    mReplyToEdit->setClearButtonShown(true);
    mReplyToEdit->setObjectName(QStringLiteral("mReplyToEdit"));
    glay->addWidget(mReplyToEdit, row, 1);
    label = new QLabel(i18n("&Reply-To address:"), tab);
    label->setBuddy(mReplyToEdit);
    glay->addWidget(label, row, 0);
    msg = i18n("<qt><h3>Reply-To addresses</h3>"
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

    // "CC addresses" line edit and label:
    ++row;
    mCcEdit = new KPIM::AddresseeLineEdit(tab, true);
    mCcEdit->setClearButtonShown(true);
    mCcEdit->setObjectName(QStringLiteral("mCcEdit"));
    glay->addWidget(mCcEdit, row, 1);
    label = new QLabel(i18n("&CC addresses:"), tab);
    label->setBuddy(mCcEdit);
    glay->addWidget(label, row, 0);
    msg = i18n("<qt><h3>CC (Carbon Copy) addresses</h3>"
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
    ++row;
    mBccEdit = new KPIM::AddresseeLineEdit(tab, true);
    mBccEdit->setClearButtonShown(true);
    mBccEdit->setObjectName(QStringLiteral("mBccEdit"));
    glay->addWidget(mBccEdit, row, 1);
    label = new QLabel(i18n("&BCC addresses:"), tab);
    label->setBuddy(mBccEdit);
    glay->addWidget(label, row, 0);
    msg = i18n("<qt><h3>BCC (Blind Carbon Copy) addresses</h3>"
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
    ++row;
    mDictionaryCombo = new Sonnet::DictionaryComboBox(tab);
    glay->addWidget(mDictionaryCombo, row, 1);
    label = new QLabel(i18n("D&ictionary:"), tab);
    label->setBuddy(mDictionaryCombo);
    glay->addWidget(label, row, 0);

    // "Sent-mail Folder" combo box and label:
    ++row;
    mFccCombo = new FolderRequester(tab);
    mFccCombo->setShowOutbox(false);
    glay->addWidget(mFccCombo, row, 1);
    mSentMailFolderCheck = new QCheckBox(i18n("Sent-mail &folder:"), tab);
    glay->addWidget(mSentMailFolderCheck, row, 0);
    connect(mSentMailFolderCheck, &QCheckBox::toggled, mFccCombo, &MailCommon::FolderRequester::setEnabled);

    // "Drafts Folder" combo box and label:
    ++row;
    mDraftsCombo = new FolderRequester(tab);
    mDraftsCombo->setShowOutbox(false);
    glay->addWidget(mDraftsCombo, row, 1);
    label = new QLabel(i18n("&Drafts folder:"), tab);
    label->setBuddy(mDraftsCombo);
    glay->addWidget(label, row, 0);

    // "Templates Folder" combo box and label:
    ++row;
    mTemplatesCombo = new FolderRequester(tab);
    mTemplatesCombo->setShowOutbox(false);
    glay->addWidget(mTemplatesCombo, row, 1);
    label = new QLabel(i18n("&Templates folder:"), tab);
    label->setBuddy(mTemplatesCombo);
    glay->addWidget(label, row, 0);

    // "Special transport" combobox and label:
    ++row;
    mTransportCheck = new QCheckBox(i18n("Outgoing Account:"), tab);
    glay->addWidget(mTransportCheck, row, 0);
    mTransportCombo = new TransportComboBox(tab);
    mTransportCombo->setEnabled(false);   // since !mTransportCheck->isChecked()
    glay->addWidget(mTransportCombo, row, 1);
    connect(mTransportCheck, &QCheckBox::toggled, mTransportCombo, &MailTransport::TransportComboBox::setEnabled);

    ++row;
    mAttachMyVCard = new QCheckBox(i18n("Attach my vCard to message"), tab);
    glay->addWidget(mAttachMyVCard, row, 0);
    mEditVCard = new QPushButton(i18n("Create..."), tab);
    connect(mEditVCard, &QPushButton::clicked, this, &IdentityDialog::slotEditVcard);
    glay->addWidget(mEditVCard, row, 1);

    ++row;
    mAutoCorrectionLanguage = new PimCommon::AutoCorrectionLanguage(tab);
    glay->addWidget(mAutoCorrectionLanguage, row, 1);
    label = new QLabel(i18n("Autocorrection language:"), tab);
    label->setBuddy(mAutoCorrectionLanguage);
    glay->addWidget(label, row, 0);

    // "default domain" input field:
    ++row;
    QHBoxLayout *hbox = new QHBoxLayout;
    mDefaultDomainEdit = new KLineEdit(tab);
    mDefaultDomainEdit->setClearButtonShown(true);
    hbox->addWidget(mDefaultDomainEdit);
    QToolButton *restoreDefaultDomainName = new QToolButton;
    restoreDefaultDomainName->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    restoreDefaultDomainName->setToolTip(i18n("Restore default domain name"));
    hbox->addWidget(restoreDefaultDomainName);
    connect(restoreDefaultDomainName, &QToolButton::clicked, this, &IdentityDialog::slotRefreshDefaultDomainName);
    glay->addLayout(hbox, row, 1);
    label = new QLabel(i18n("Defaul&t domain:"), tab);
    label->setBuddy(mDefaultDomainEdit);
    glay->addWidget(label, row, 0);

    // and now: add QWhatsThis:
    msg = i18n("<qt><p>The default domain is used to complete email "
               "addresses that only consist of the user's name."
               "</p></qt>");
    label->setWhatsThis(msg);
    mDefaultDomainEdit->setWhatsThis(msg);

    ++row;
    glay->setRowStretch(row, 1);

    // the last row is a spacer

    //
    // Tab Widget: Templates
    //
    tab = new QWidget(mTabWidget);
    vlay = new QVBoxLayout(tab);

    QHBoxLayout *tlay = new QHBoxLayout();
    vlay->addLayout(tlay);

    mCustom = new QCheckBox(i18n("&Use custom message templates for this identity"), tab);
    tlay->addWidget(mCustom, Qt::AlignLeft);

    mWidget = new TemplateParser::TemplatesConfiguration(tab, QStringLiteral("identity-templates"));
    mWidget->setEnabled(false);

    // Move the help label outside of the templates configuration widget,
    // so that the help can be read even if the widget is not enabled.
    tlay->addStretch(9);
    tlay->addWidget(mWidget->helpLabel(), Qt::AlignRight);

    vlay->addWidget(mWidget);

    QHBoxLayout *btns = new QHBoxLayout();
    mCopyGlobal = new QPushButton(i18n("&Copy Global Templates"), tab);
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
    mSignatureConfigurator = new KIdentityManagement::SignatureConfigurator(mTabWidget);
    mTabWidget->addTab(mSignatureConfigurator, i18n("Signature"));

    //
    // Tab Widget: Picture
    //

    mXFaceConfigurator = new XFaceConfigurator(mTabWidget);
    mTabWidget->addTab(mXFaceConfigurator, i18n("Picture"));

#ifndef KCM_KPIMIDENTITIES_STANDALONE
    resize(KMailSettings::self()->identityDialogSize());
#endif
    mNameEdit->setFocus();

    connect(mTabWidget, &QTabWidget::currentChanged, this, &IdentityDialog::slotAboutToShow);
}

IdentityDialog::~IdentityDialog()
{
#ifndef KCM_KPIMIDENTITIES_STANDALONE
    KMailSettings::self()->setIdentityDialogSize(size());
#endif
}

void IdentityDialog::slotHelp()
{
    KHelpClient::invokeHelp(QStringLiteral("configure-identity"), QStringLiteral("kmail"));
}

void IdentityDialog::slotAboutToShow(int index)
{
    QWidget *w = mTabWidget->widget(index);
    if (w == mCryptographyTab) {
        // set the configured email address as initial query of the key
        // requesters:
        const QString email = mEmailEdit->text().trimmed();
        mPGPEncryptionKeyRequester->setInitialQuery(email);
        mPGPSigningKeyRequester->setInitialQuery(email);
        mSMIMEEncryptionKeyRequester->setInitialQuery(email);
        mSMIMESigningKeyRequester->setInitialQuery(email);
    }
}

void IdentityDialog::slotCopyGlobal()
{
    mWidget->loadFromGlobal();
}

namespace
{
struct DoesntMatchEMailAddress {
    explicit DoesntMatchEMailAddress(const QString &s)
        : email(s.trimmed().toLower()) {}
    bool operator()(const GpgME::Key &key) const;
private:
    bool checkForEmail(const char *email) const;
    static QString extractEmail(const char *email);
    const QString email;
};

bool DoesntMatchEMailAddress::operator()(const GpgME::Key &key) const
{
    const std::vector<GpgME::UserID> uids = key.userIDs();
    std::vector<GpgME::UserID>::const_iterator end = uids.end();
    for (std::vector<GpgME::UserID>::const_iterator it = uids.begin() ; it != end ; ++it)
        if (checkForEmail(it->email() ? it->email() : it->id())) {
            return false;
        }
    return true; // note the negation!
}

bool DoesntMatchEMailAddress::checkForEmail(const char *e) const
{
    const QString em = extractEmail(e);
    return !em.isEmpty() && email.toLower() == em.toLower();
}

QString DoesntMatchEMailAddress::extractEmail(const char *e)
{
    if (!e || !*e) {
        return QString();
    }
    const QString em = QString::fromUtf8(e);
    if (e[0] == '<') {
        return em.mid(1, em.length() - 2);
    } else {
        return em;
    }
}
}

void IdentityDialog::slotRefreshDefaultDomainName()
{
    mDefaultDomainEdit->setText(QHostInfo::localHostName());
}

void IdentityDialog::slotAccepted()
{
    const QStringList aliases = mAliasEdit->items();
    foreach (const QString &alias, aliases) {
        if (!KEmailAddress::isValidSimpleAddress(alias)) {
            const QString errorMsg(KEmailAddress::simpleEmailAddressErrorMsg());
            KMessageBox::sorry(this, errorMsg, i18n("Invalid Email Alias \"%1\"", alias));
            return;
        }
    }

    // Validate email addresses
    const QString email = mEmailEdit->text().trimmed();
    if (!KEmailAddress::isValidSimpleAddress(email)) {
        const QString errorMsg(KEmailAddress::simpleEmailAddressErrorMsg());
        KMessageBox::sorry(this, errorMsg, i18n("Invalid Email Address"));
        return;
    }

    // Check if the 'Reply to' and 'BCC' recipients are valid
    const QString recipients = mReplyToEdit->text().trimmed() + QLatin1String(", ") + mBccEdit->text().trimmed() + QLatin1String(", ") + mCcEdit->text().trimmed();
    AddressValidationJob *job = new AddressValidationJob(recipients, this, this);
    //Use default Value
    job->setDefaultDomain(mDefaultDomainEdit->text());
    job->setProperty("email", email);
    connect(job, &AddressValidationJob::result, this, &IdentityDialog::slotDelayedButtonClicked);
    job->start();
}

void IdentityDialog::slotDelayedButtonClicked(KJob *job)
{
    const AddressValidationJob *validationJob = qobject_cast<AddressValidationJob *>(job);

    // Abort if one of the recipient addresses is invalid
    if (!validationJob->isValid()) {
        return;
    }

    const QString email = validationJob->property("email").toString();

    const std::vector<GpgME::Key> &pgpSigningKeys =
        mPGPSigningKeyRequester->keys();
    const std::vector<GpgME::Key> &pgpEncryptionKeys =
        mPGPEncryptionKeyRequester->keys();
    const std::vector<GpgME::Key> &smimeSigningKeys =
        mSMIMESigningKeyRequester->keys();
    const std::vector<GpgME::Key> &smimeEncryptionKeys =
        mSMIMEEncryptionKeyRequester->keys();

    QString msg;
    bool err = false;
    if (std::find_if(pgpSigningKeys.begin(), pgpSigningKeys.end(),
                     DoesntMatchEMailAddress(email)) != pgpSigningKeys.end()) {
        msg = i18n("One of the configured OpenPGP signing keys does not contain "
                   "any user ID with the configured email address for this "
                   "identity (%1).\n"
                   "This might result in warning messages on the receiving side "
                   "when trying to verify signatures made with this configuration.", email);
        err = true;
    } else if (std::find_if(pgpEncryptionKeys.begin(), pgpEncryptionKeys.end(),
                            DoesntMatchEMailAddress(email)) != pgpEncryptionKeys.end()) {
        msg = i18n("One of the configured OpenPGP encryption keys does not contain "
                   "any user ID with the configured email address for this "
                   "identity (%1).", email);
        err = true;
    } else if (std::find_if(smimeSigningKeys.begin(), smimeSigningKeys.end(),
                            DoesntMatchEMailAddress(email)) != smimeSigningKeys.end()) {
        msg = i18n("One of the configured S/MIME signing certificates does not contain "
                   "the configured email address for this "
                   "identity (%1).\n"
                   "This might result in warning messages on the receiving side "
                   "when trying to verify signatures made with this configuration.", email);
        err = true;
    } else if (std::find_if(smimeEncryptionKeys.begin(), smimeEncryptionKeys.end(),
                            DoesntMatchEMailAddress(email)) != smimeEncryptionKeys.end()) {
        msg = i18n("One of the configured S/MIME encryption certificates does not contain "
                   "the configured email address for this "
                   "identity (%1).", email);
        err = true;
    }

    if (err) {
        if (KMessageBox::warningContinueCancel(this, msg,
                                               i18n("Email Address Not Found in Key/Certificates"),
                                               KStandardGuiItem::cont(),
                                               KStandardGuiItem::cancel(),
                                               QStringLiteral("warn_email_not_in_certificate"))
                != KMessageBox::Continue) {
            return;
        }
    }

    if (mSignatureConfigurator->isSignatureEnabled() &&
            mSignatureConfigurator->signatureType() == Signature::FromFile) {
        QUrl url(mSignatureConfigurator->fileURL());
        KFileItem signatureFile(url);
        if (!signatureFile.isFile() || !signatureFile.isReadable() || !signatureFile.isLocalFile()) {
            KMessageBox::error(this, i18n("The signature file is not valid"));
            return;
        }
    }

    accept();
}

bool IdentityDialog::checkFolderExists(const QString &folderID,
                                       const QString &msg)
{
    const Akonadi::Collection folder = CommonKernel->collectionFromId(folderID.toLongLong());
    if (!folder.isValid()) {
        KMessageBox::sorry(this, msg);
        return false;
    }

    return true;
}

void IdentityDialog::setIdentity(KIdentityManagement::Identity &ident)
{

    setWindowTitle(i18n("Edit Identity \"%1\"", ident.identityName()));

    // "General" tab:
    mNameEdit->setText(ident.fullName());
    mOrganizationEdit->setText(ident.organization());
    mEmailEdit->setText(ident.primaryEmailAddress());
    mAliasEdit->insertStringList(ident.emailAliases());

    // "Cryptography" tab:
    mPGPSigningKeyRequester->setFingerprint(QLatin1String(ident.pgpSigningKey()));
    mPGPEncryptionKeyRequester->setFingerprint(QLatin1String(ident.pgpEncryptionKey()));
    mSMIMESigningKeyRequester->setFingerprint(QLatin1String(ident.smimeSigningKey()));
    mSMIMEEncryptionKeyRequester->setFingerprint(QLatin1String(ident.smimeEncryptionKey()));

    mPreferredCryptoMessageFormat->setCurrentIndex(format2cb(
                Kleo::stringToCryptoMessageFormat(ident.preferredCryptoMessageFormat())));
    mAutoSign->setChecked(ident.pgpAutoSign());

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
    mFccCombo->setEnabled(mSentMailFolderCheck->isChecked());
    if (ident.fcc().isEmpty() ||
            !checkFolderExists(ident.fcc(),
                               i18n("The custom sent-mail folder for identity "
                                    "\"%1\" does not exist (anymore); "
                                    "therefore, the default sent-mail folder "
                                    "will be used.",
                                    ident.identityName()))) {
        mFccCombo->setCollection(CommonKernel->sentCollectionFolder());
    } else {
        mFccCombo->setCollection(Akonadi::Collection(ident.fcc().toLongLong()));
    }
    if (ident.drafts().isEmpty() ||
            !checkFolderExists(ident.drafts(),
                               i18n("The custom drafts folder for identity "
                                    "\"%1\" does not exist (anymore); "
                                    "therefore, the default drafts folder "
                                    "will be used.",
                                    ident.identityName()))) {
        mDraftsCombo->setCollection(CommonKernel->draftsCollectionFolder());
    } else {
        mDraftsCombo->setCollection(Akonadi::Collection(ident.drafts().toLongLong()));
    }

    if (ident.templates().isEmpty() ||
            !checkFolderExists(ident.templates(),
                               i18n("The custom templates folder for identity "
                                    "\"%1\" does not exist (anymore); "
                                    "therefore, the default templates folder "
                                    "will be used.", ident.identityName()))) {
        mTemplatesCombo->setCollection(CommonKernel->templatesCollectionFolder());

    } else {
        mTemplatesCombo->setCollection(Akonadi::Collection(ident.templates().toLongLong()));
    }

    mVcardFilename = ident.vCardFile();

    mAutoCorrectionLanguage->setLanguage(ident.autocorrectionLanguage());
    updateVcardButton();
    if (mVcardFilename.isEmpty()) {
        mVcardFilename = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + ident.identityName() + QLatin1String(".vcf");
        QFileInfo fileInfo(mVcardFilename);
        QDir().mkpath(fileInfo.absolutePath());
    } else {
        //Convert path.
        const QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + ident.identityName() + QLatin1String(".vcf");
        if (QFile(path).exists() && (mVcardFilename != path)) {
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
}

void IdentityDialog::updateIdentity(KIdentityManagement::Identity &ident)
{
    // "General" tab:
    ident.setFullName(mNameEdit->text());
    ident.setOrganization(mOrganizationEdit->text());
    QString email = mEmailEdit->text();
    ident.setPrimaryEmailAddress(email);
    ident.setEmailAliases(mAliasEdit->items());
    // "Cryptography" tab:
    ident.setPGPSigningKey(mPGPSigningKeyRequester->fingerprint().toLatin1());
    ident.setPGPEncryptionKey(mPGPEncryptionKeyRequester->fingerprint().toLatin1());
    ident.setSMIMESigningKey(mSMIMESigningKeyRequester->fingerprint().toLatin1());
    ident.setSMIMEEncryptionKey(mSMIMEEncryptionKeyRequester->fingerprint().toLatin1());
    ident.setPreferredCryptoMessageFormat(
        QLatin1String(Kleo::cryptoMessageFormatToString(cb2format(mPreferredCryptoMessageFormat->currentIndex()))));
    ident.setPgpAutoSign(mAutoSign->isChecked());
    // "Advanced" tab:
    ident.setReplyToAddr(mReplyToEdit->text());
    ident.setBcc(mBccEdit->text());
    ident.setCc(mCcEdit->text());
    ident.setTransport(mTransportCheck->isChecked() ? QString::number(mTransportCombo->currentTransportId())
                       : QString());
    ident.setDictionary(mDictionaryCombo->currentDictionaryName());
    ident.setDisabledFcc(!mSentMailFolderCheck->isChecked());
    Akonadi::Collection collection = mFccCombo->collection();
    if (collection.isValid()) {
        ident.setFcc(QString::number(collection.id()));
        Akonadi::EntityDisplayAttribute *attribute =  collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        attribute->setIconName(QStringLiteral("mail-folder-sent"));
        new Akonadi::CollectionModifyJob(collection);
    } else {
        ident.setFcc(QString());
    }

    collection = mDraftsCombo->collection();
    if (collection.isValid()) {
        ident.setDrafts(QString::number(collection.id()));
        Akonadi::EntityDisplayAttribute *attribute =  collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        attribute->setIconName(QStringLiteral("document-properties"));
        new Akonadi::CollectionModifyJob(collection);
    } else {
        ident.setDrafts(QString());
    }

    collection = mTemplatesCombo->collection();
    if (collection.isValid()) {
        ident.setTemplates(QString::number(collection.id()));
        Akonadi::EntityDisplayAttribute *attribute =  collection.attribute<Akonadi::EntityDisplayAttribute>(Akonadi::Collection::AddIfMissing);
        attribute->setIconName(QStringLiteral("document-new"));
        new Akonadi::CollectionModifyJob(collection);
    } else {
        ident.setTemplates(QString());
    }
    ident.setVCardFile(mVcardFilename);
    ident.setAutocorrectionLanguage(mAutoCorrectionLanguage->language());
    updateVcardButton();
    ident.setAttachVcard(mAttachMyVCard->isChecked());
    //Add default ?
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

}
void IdentityDialog::slotEditVcard()
{
    if (QFile(mVcardFilename).exists()) {
        editVcard(mVcardFilename);
    } else {
        if (!MailCommon::Kernel::self()->kernelIsRegistered()) {
            return;
        }
        KIdentityManagement::IdentityManager *manager = KernelIf->identityManager();

        QPointer<IdentityAddVcardDialog> dlg = new IdentityAddVcardDialog(manager->shadowIdentities(), this);
        if (dlg->exec()) {
            IdentityAddVcardDialog::DuplicateMode mode = dlg->duplicateMode();
            switch (mode) {
            case IdentityAddVcardDialog::Empty: {
                editVcard(mVcardFilename);
                break;
            }
            case IdentityAddVcardDialog::ExistingEntry: {
                KIdentityManagement::Identity ident = manager->modifyIdentityForName(dlg->duplicateVcardFromIdentity());
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
    if (mVcardFilename.isEmpty() || !QFile(mVcardFilename).exists()) {
        mEditVCard->setText(i18n("Create..."));
    } else {
        mEditVCard->setText(i18n("Edit..."));
    }
}

}

