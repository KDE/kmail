/*
    identitydialog.h

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2002 Marc Mutz <mutz@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once
#include "config-kmail.h"
#include <QDialog>
class QCheckBox;

class KEditListWidget;
class QComboBox;
class QGroupBox;
class KJob;
class QLineEdit;
class QPushButton;
class QTabWidget;

namespace GpgME
{
class Key;
}
namespace KIdentityManagementCore
{
class Identity;
}
namespace KIdentityManagementWidgets
{
class SignatureConfigurator;
}
namespace KMail
{
class XFaceConfigurator;
}

namespace MailCommon
{
class FolderRequester;
}
namespace Sonnet
{
class DictionaryComboBox;
}

namespace MailTransport
{
class TransportComboBox;
}

namespace TemplateParser
{
class TemplatesConfiguration;
}
namespace TextAutoCorrectionWidgets
{
class AutoCorrectionLanguage;
}

#if KMAIL_HAVE_ACTIVITY_SUPPORT
namespace PimCommonActivities
{
class ConfigureActivitiesWidget;
}
#endif

namespace KMail
{
class IdentityFolderRequester;
class IdentityInvalidFolder;
class KeySelectionCombo;

class IdentityDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IdentityDialog(QWidget *parent = nullptr);
    ~IdentityDialog() override;

    void setIdentity(/*_not_ const*/ KIdentityManagementCore::Identity &ident);

    void updateIdentity(KIdentityManagementCore::Identity &ident);

private:
    void slotAboutToShow(int);
    // copy default templates to identity templates
    void slotCopyGlobal();
    void slotAccepted();
    void slotDelayedButtonClicked(KJob *);
    void slotEditVcard();
    void slotRefreshDefaultDomainName();
    void slotVCardRemoved();
    void slotHelp();

    [[nodiscard]] bool keyMatchesEmailAddress(const GpgME::Key &key, const QString &email);
    [[nodiscard]] bool checkFolderExists(const QString &folder);
    void updateVcardButton();
    void editVcard(const QString &filename);
    void unregisterSpecialCollection(qint64 id);

    QString mVcardFilename;

    // "general" tab:
    QLineEdit *mNameEdit = nullptr;
    QLineEdit *mOrganizationEdit = nullptr;
    QLineEdit *mEmailEdit = nullptr;
    KEditListWidget *mAliasEdit = nullptr;
    // "cryptography" tab:
    QWidget *mCryptographyTab = nullptr;
    KeySelectionCombo *mPGPSigningKeyRequester = nullptr;
    KeySelectionCombo *mPGPEncryptionKeyRequester = nullptr;
    KeySelectionCombo *mSMIMESigningKeyRequester = nullptr;
    KeySelectionCombo *mSMIMEEncryptionKeyRequester = nullptr;
    QComboBox *mPreferredCryptoMessageFormat = nullptr;
    QGroupBox *mAutocrypt = nullptr;
    QCheckBox *mAutocryptPrefer = nullptr;
    QGroupBox *mOverrideDefault = nullptr;
    QCheckBox *mPGPSameKey = nullptr;
    QCheckBox *mAutoSign = nullptr;
    QCheckBox *mAutoEncrypt = nullptr;
    QCheckBox *mWarnNotEncrypt = nullptr;
    QCheckBox *mWarnNotSign = nullptr;
    // "advanced" tab:
    QLineEdit *mReplyToEdit = nullptr;
    QLineEdit *mBccEdit = nullptr;
    QLineEdit *mCcEdit = nullptr;
    Sonnet::DictionaryComboBox *mDictionaryCombo = nullptr;
    IdentityFolderRequester *mFccFolderRequester = nullptr;
    QCheckBox *mSentMailFolderCheck = nullptr;
    IdentityFolderRequester *mDraftsFolderRequester = nullptr;
    IdentityFolderRequester *mTemplatesFolderRequester = nullptr;
    QCheckBox *mSpamFolderCheck = nullptr;
    IdentityFolderRequester *mSpamFolderRequester = nullptr;
    QCheckBox *mTransportCheck = nullptr;
    MailTransport::TransportComboBox *mTransportCombo = nullptr;
    QCheckBox *mAttachMyVCard = nullptr;
    QPushButton *mEditVCard = nullptr;
    TextAutoCorrectionWidgets::AutoCorrectionLanguage *mAutoCorrectionLanguage = nullptr;
    QLineEdit *mDefaultDomainEdit = nullptr;

    // "templates" tab:
    TemplateParser::TemplatesConfiguration *mWidget = nullptr;
    QCheckBox *mCustom = nullptr;
    QPushButton *mCopyGlobal = nullptr;
    // "signature" tab:
    KIdentityManagementWidgets::SignatureConfigurator *mSignatureConfigurator = nullptr;
    // "X-Face" tab:
    KMail::XFaceConfigurator *mXFaceConfigurator = nullptr;
    QTabWidget *mTabWidget = nullptr;
    IdentityInvalidFolder *mIdentityInvalidFolder = nullptr;
#if KMAIL_HAVE_ACTIVITY_SUPPORT
    PimCommonActivities::ConfigureActivitiesWidget *const mConfigureActivitiesWidget;
#endif
};
} // namespace KMail
