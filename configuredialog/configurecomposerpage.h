/*
  Copyright (c) 2013-2015 Montel Laurent <montel@kde.org>

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

#ifndef CONFIGURECOMPOSERPAGE_H
#define CONFIGURECOMPOSERPAGE_H

#include "kmail_export.h"
#include <config-enterprise.h>
#include "configuredialog_p.h"
#include "ui_customtemplates_base.h"
class QCheckBox;
class KIntSpinBox;
class KIntNumInput;
class KComboBox;
class KUrlRequester;
class KLineEdit;
class ListView;
class QPushButton;
class QLabel;
class ConfigureStorageServiceWidget;

namespace TemplateParser {
class CustomTemplates;
class TemplatesConfiguration;
}
namespace PimCommon {
class AutoCorrectionWidget;
}
namespace MessageComposer {
class ImageScalingWidget;
}

namespace PimCommon {
class SimpleStringListEditor;
}

class ComposerPageGeneralTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageGeneralTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();
protected slots:
    void slotConfigureRecentAddresses();
    void slotConfigureCompletionOrder();

private:
    void doLoadFromGlobalSettings();
    void doResetToDefaultsOther();

private:
    QCheckBox     *mShowBalooSearchAddressesInComposer;
    QCheckBox     *mAutoAppSignFileCheck;
    QCheckBox     *mTopQuoteCheck;
    QCheckBox     *mDashDashCheck;
    QCheckBox     *mReplyUsingHtml;
    QCheckBox     *mSmartQuoteCheck;
    QCheckBox     *mStripSignatureCheck;
    QCheckBox     *mQuoteSelectionOnlyCheck;
    QCheckBox     *mAutoRequestMDNCheck;
    QCheckBox        *mShowRecentAddressesInComposer;
    QCheckBox     *mWordWrapCheck;
    KIntSpinBox   *mWrapColumnSpin;
    KIntSpinBox   *mAutoSave;
    KIntSpinBox   *mMaximumRecipients;
    QCheckBox     *mImprovePlainTextOfHtmlMessage;
    KIntNumInput  *mMaximumRecentAddress;
#ifdef KDEPIM_ENTERPRISE_BUILD
    KComboBox     *mForwardTypeCombo;
    QCheckBox     *mRecipientCheck;
    KIntSpinBox   *mRecipientSpin;
#endif
};

class ComposerPageExternalEditorTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageExternalEditorTab( QWidget * parent = 0 );
    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();

private:
    QCheckBox     *mExternalEditorCheck;
    KUrlRequester *mEditorRequester;
};

class ComposerPageTemplatesTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageTemplatesTab( QWidget * parent = 0 );
    QString helpAnchor() const;

    void save();

private slots:

private:
    void doLoadFromGlobalSettings();
    void doResetToDefaultsOther();
private:
    TemplateParser::TemplatesConfiguration* mWidget;
};

class ComposerPageCustomTemplatesTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageCustomTemplatesTab( QWidget * parent = 0 );
    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();

private:
    TemplateParser::CustomTemplates* mWidget;
};

class ComposerPageSubjectTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageSubjectTab( QWidget * parent = 0 );
    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();
    void doResetToDefaultsOther();

private:
    PimCommon::SimpleStringListEditor *mReplyListEditor;
    QCheckBox              *mReplaceReplyPrefixCheck;
    PimCommon::SimpleStringListEditor *mForwardListEditor;
    QCheckBox              *mReplaceForwardPrefixCheck;
};

class ComposerPageCharsetTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageCharsetTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private slots:
    void slotVerifyCharset(QString&);

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    void doResetToDefaultsOther();

private:
    PimCommon::SimpleStringListEditor *mCharsetListEditor;
    QCheckBox              *mKeepReplyCharsetCheck;
};

class ComposerPageHeadersTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageHeadersTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private slots:
    void slotMimeHeaderSelectionChanged();
    void slotMimeHeaderNameChanged( const QString & );
    void slotMimeHeaderValueChanged( const QString & );
    void slotNewMimeHeader();
    void slotRemoveMimeHeader();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    void doResetToDefaultsOther();

private:
    QCheckBox   *mCreateOwnMessageIdCheck;
    KLineEdit   *mMessageIdSuffixEdit;
    ListView    *mHeaderList;
    QPushButton *mRemoveHeaderButton;
    KLineEdit   *mTagNameEdit;
    KLineEdit   *mTagValueEdit;
    QLabel      *mTagNameLabel;
    QLabel      *mTagValueLabel;
};

class ComposerPageAttachmentsTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageAttachmentsTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private slots:
    void slotOutlookCompatibleClicked();

private:
    void doLoadFromGlobalSettings();
    //FIXME virtual void doResetToDefaultsOther();

private:
    QCheckBox   *mOutlookCompatibleCheck;
    QCheckBox   *mMissingAttachmentDetectionCheck;
    PimCommon::SimpleStringListEditor *mAttachWordsListEditor;
    KIntNumInput *mMaximumAttachmentSize;
    ConfigureStorageServiceWidget *mStorageServiceWidget;
};

class ComposerPageAutoCorrectionTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageAutoCorrectionTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();
    void doResetToDefaultsOther();

private:
    PimCommon::AutoCorrectionWidget *autocorrectionWidget;
};

class ComposerPageAutoImageResizeTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit ComposerPageAutoImageResizeTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private:
    virtual void doLoadFromGlobalSettings();
    virtual void doResetToDefaultsOther();

private:
    MessageComposer::ImageScalingWidget *autoResizeWidget;
};


class KMAIL_EXPORT ComposerPage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit ComposerPage( const KComponentData &instance, QWidget *parent=0 );

    QString helpAnchor() const;

    // hrmpf. moc doesn't like nested classes with slots/signals...:
    typedef ComposerPageGeneralTab GeneralTab;
    typedef ComposerPageTemplatesTab TemplatesTab;
    typedef ComposerPageCustomTemplatesTab CustomTemplatesTab;
    typedef ComposerPageSubjectTab SubjectTab;
    typedef ComposerPageCharsetTab CharsetTab;
    typedef ComposerPageHeadersTab HeadersTab;
    typedef ComposerPageAttachmentsTab AttachmentsTab;
    typedef ComposerPageAutoCorrectionTab AutoCorrectionTab;
    typedef ComposerPageAutoImageResizeTab AutoImageResizeTab;
    typedef ComposerPageExternalEditorTab ExternalEditorTab;
private:
    GeneralTab  *mGeneralTab;
    TemplatesTab  *mTemplatesTab;
    CustomTemplatesTab  *mCustomTemplatesTab;
    SubjectTab  *mSubjectTab;
    CharsetTab  *mCharsetTab;
    HeadersTab  *mHeadersTab;
    AttachmentsTab  *mAttachmentsTab;
    AutoCorrectionTab *mAutoCorrectionTab;
    AutoImageResizeTab *mAutoImageResizeTab;
    ExternalEditorTab *mExternalEditorTab;
};

#endif // CONFIGURECOMPOSERPAGE_H
