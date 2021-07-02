/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "configuredialog_p.h"
#include "kmail_export.h"
#include <config-enterprise.h>
class QCheckBox;
class QSpinBox;
class QSpinBox;
class QComboBox;
class QLineEdit;
class ListView;
class QPushButton;
class QLabel;
class KPluralHandlingSpinBox;
namespace TemplateParser
{
class CustomTemplates;
class TemplatesConfiguration;
}
namespace PimCommon
{
class AutoCorrectionWidget;
class SimpleStringListEditor;
}
namespace MessageComposer
{
class ImageScalingWidget;
}

class ComposerPageGeneralTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageGeneralTab(QWidget *parent = nullptr);
    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void slotConfigureAddressCompletion();
    void doLoadFromGlobalSettings() override;
    void doResetToDefaultsOther() override;

private:
    QCheckBox *mShowAkonadiSearchAddressesInComposer = nullptr;
    QCheckBox *mAutoAppSignFileCheck = nullptr;
    QCheckBox *mTopQuoteCheck = nullptr;
    QCheckBox *mDashDashCheck = nullptr;
    QCheckBox *mReplyUsingVisualFormat = nullptr;
    QCheckBox *mSmartQuoteCheck = nullptr;
    QCheckBox *mStripSignatureCheck = nullptr;
    QCheckBox *mQuoteSelectionOnlyCheck = nullptr;
    QCheckBox *mAutoRequestMDNCheck = nullptr;
    QCheckBox *mShowRecentAddressesInComposer = nullptr;
    QCheckBox *mWordWrapCheck = nullptr;
    QSpinBox *mWrapColumnSpin = nullptr;
    KPluralHandlingSpinBox *mAutoSave = nullptr;
    QSpinBox *mMaximumRecipients = nullptr;
    QCheckBox *mImprovePlainTextOfHtmlMessage = nullptr;
    QSpinBox *mMaximumRecentAddress = nullptr;
#if KDEPIM_ENTERPRISE_BUILD
    QComboBox *mForwardTypeCombo = nullptr;
    QCheckBox *mRecipientCheck = nullptr;
    QSpinBox *mRecipientSpin = nullptr;
#endif
};

class ComposerPageTemplatesTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageTemplatesTab(QWidget *parent = nullptr);
    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void doLoadFromGlobalSettings() override;
    void doResetToDefaultsOther() override;

private:
    TemplateParser::TemplatesConfiguration *mWidget = nullptr;
};

class ComposerPageCustomTemplatesTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageCustomTemplatesTab(QWidget *parent = nullptr);
    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void doLoadFromGlobalSettings() override;

private:
    TemplateParser::CustomTemplates *mWidget = nullptr;
};

class ComposerPageSubjectTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageSubjectTab(QWidget *parent = nullptr);
    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void doLoadFromGlobalSettings() override;
    void doResetToDefaultsOther() override;

private:
    PimCommon::SimpleStringListEditor *mReplyListEditor = nullptr;
    QCheckBox *mReplaceReplyPrefixCheck = nullptr;
    PimCommon::SimpleStringListEditor *mForwardListEditor = nullptr;
    QCheckBox *mReplaceForwardPrefixCheck = nullptr;
};

class ComposerPageCharsetTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageCharsetTab(QWidget *parent = nullptr);
    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private:
    void slotVerifyCharset(QString &);
    void doLoadOther() override;
    void doResetToDefaultsOther() override;

private:
    PimCommon::SimpleStringListEditor *mCharsetListEditor = nullptr;
    QCheckBox *mKeepReplyCharsetCheck = nullptr;
};

class ComposerPageHeadersTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageHeadersTab(QWidget *parent = nullptr);
    Q_REQUIRED_RESULT QString helpAnchor() const;

    void save() override;

private Q_SLOTS:
    void slotMimeHeaderSelectionChanged();
    void slotMimeHeaderNameChanged(const QString &);
    void slotMimeHeaderValueChanged(const QString &);
    void slotNewMimeHeader();
    void slotRemoveMimeHeader();

private:
    void doLoadOther() override;
    void doResetToDefaultsOther() override;

private:
    QCheckBox *mCreateOwnMessageIdCheck = nullptr;
    QLineEdit *mMessageIdSuffixEdit = nullptr;
    ListView *mHeaderList = nullptr;
    QPushButton *mRemoveHeaderButton = nullptr;
    QLineEdit *mTagNameEdit = nullptr;
    QLineEdit *mTagValueEdit = nullptr;
    QLabel *mTagNameLabel = nullptr;
    QLabel *mTagValueLabel = nullptr;
};

class ComposerPageAttachmentsTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageAttachmentsTab(QWidget *parent = nullptr);
    QString helpAnchor() const;

    void save() override;

private Q_SLOTS:
    void slotOutlookCompatibleClicked();

private:
    void doLoadFromGlobalSettings() override;

private:
    QCheckBox *mOutlookCompatibleCheck = nullptr;
    QCheckBox *mMissingAttachmentDetectionCheck = nullptr;
    PimCommon::SimpleStringListEditor *mAttachWordsListEditor = nullptr;
    QSpinBox *mMaximumAttachmentSize = nullptr;
};

class ComposerPageAutoCorrectionTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageAutoCorrectionTab(QWidget *parent = nullptr);
    QString helpAnchor() const;

    void save() override;

private:
    void doLoadFromGlobalSettings() override;
    void doResetToDefaultsOther() override;

private:
    PimCommon::AutoCorrectionWidget *autocorrectionWidget = nullptr;
};

class ComposerPageAutoImageResizeTab : public ConfigModuleTab
{
    Q_OBJECT
public:
    explicit ComposerPageAutoImageResizeTab(QWidget *parent = nullptr);
    QString helpAnchor() const;

    void save() override;

private:
    void doLoadFromGlobalSettings() override;
    void doResetToDefaultsOther() override;

private:
    MessageComposer::ImageScalingWidget *autoResizeWidget = nullptr;
};

class KMAIL_EXPORT ComposerPage : public ConfigModuleWithTabs
{
    Q_OBJECT
public:
    explicit ComposerPage(QWidget *parent = nullptr, const QVariantList &args = {});

    QString helpAnchor() const override;

    // hrmpf. moc doesn't like nested classes with slots/signals...:
    using GeneralTab = ComposerPageGeneralTab;
    using TemplatesTab = ComposerPageTemplatesTab;
    using CustomTemplatesTab = ComposerPageCustomTemplatesTab;
    using SubjectTab = ComposerPageSubjectTab;
    using CharsetTab = ComposerPageCharsetTab;
    using HeadersTab = ComposerPageHeadersTab;
    using AttachmentsTab = ComposerPageAttachmentsTab;
    using AutoCorrectionTab = ComposerPageAutoCorrectionTab;
    using AutoImageResizeTab = ComposerPageAutoImageResizeTab;
};

