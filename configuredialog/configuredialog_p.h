// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include <config-enterprise.h>

#include "kmail_export.h"
#include "configmodule.h"

#include <QStringList>
#include <QList>
#include <QHash>
#include <QSharedPointer>
#include <QListWidgetItem>

#include <kcmodule.h>
#include <klocale.h>

#include <akonadi/agentinstance.h>

#include "ui_composercryptoconfiguration.h"
#include "ui_warningconfiguration.h"
#include "ui_smimeconfiguration.h"
#include "ui_customtemplates_base.h"
#include "ui_miscpagemaintab.h"
#include "ui_securitypagegeneraltab.h"
#include "ui_securitypagemdntab.h"
#include "ui_accountspagereceivingtab.h"
#include "tag.h"
class QPushButton;
class QLabel;
class QCheckBox;
class QFont;
class QPoint;
class QGroupBox;
class QSpinBox;
class QListWidget;

class KLineEdit;
class KButtonGroup;
class KUrlRequester;
class KFontChooser;
class KTabWidget;
class ListView;
class ConfigureDialog;
class KIntSpinBox;
class OrgFreedesktopAkonadiNewMailNotifierInterface;
class ConfigureAgentsWidget;
namespace PimCommon {
class SimpleStringListEditor;
}
class KComboBox;
class ColorListBox;
class KCModuleProxy;
class KIntNumInput;
namespace MessageComposer {
class ComposerAutoCorrectionWidget;
class ImageScalingWidget;
}

namespace MessageList {
namespace Utils {
class AggregationComboBox;
class ThemeComboBox;
}
}

namespace MessageViewer {
class ConfigureWidget;
class InvitationSettings;
class PrintingSettings;
class AdBlockSettingWidget;
}

namespace TemplateParser {
class CustomTemplates;
class TemplatesConfiguration;
}

namespace MailCommon {
class Tag;
typedef QSharedPointer<Tag> TagPtr;
}

namespace MailCommon {
class FolderRequester;
class TagWidget;
}

namespace Kleo {
class CryptoConfig;
}

// Individual tab of a ConfigModuleWithTabs
class ConfigModuleTab : public QWidget {
    Q_OBJECT
public:
    explicit ConfigModuleTab( QWidget *parent=0 )
        : QWidget( parent ),
          mEmitChanges( true )
    {}
    ~ConfigModuleTab() {}
    virtual void save() = 0;
    void defaults();
signals:
    // forwarded to the ConfigModule
    void changed(bool);
public slots:
    void slotEmitChanged();
    void load();
protected:
    bool mEmitChanges;
private:
    // reimplement this for loading values of settings which are available
    // via GlobalSettings
    virtual void doLoadFromGlobalSettings() {}
    // reimplement this for loading values of settings which are not available
    // via GlobalSettings
    virtual void doLoadOther() {}
    // reimplement this for loading default values of settings which are
    // not available via GlobalSettings (KConfigXT).
    virtual void doResetToDefaultsOther() {}
};


/*
 * ConfigModuleWithTabs represents a kcm with several tabs.
 * It simply forwards load and save operations to all tabs.
 */
class KMAIL_EXPORT ConfigModuleWithTabs : public ConfigModule {
    Q_OBJECT
public:
    explicit ConfigModuleWithTabs( const KComponentData &instance, QWidget *parent=0 );
    ~ConfigModuleWithTabs() {}

    // don't reimplement any of those methods
    virtual void load();
    virtual void save();
    virtual void defaults();

protected:
    virtual void showEvent ( QShowEvent * event );
    void addTab( ConfigModuleTab* tab, const QString & title );

private:
    KTabWidget *mTabWidget;
    bool mWasInitialized;
};


//
//
// AccountsPage
//
//

// subclasses: one class per tab:
class AccountsPageSendingTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AccountsPageSendingTab( QWidget * parent=0 );
    virtual ~AccountsPageSendingTab();
    QString helpAnchor() const;
    void save();

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    QCheckBox   *mConfirmSendCheck;
    QCheckBox   *mCheckSpellingBeforeSending;
    KComboBox   *mSendOnCheckCombo;
    KComboBox   *mSendMethodCombo;
    KLineEdit   *mDefaultDomainEdit;
};


class AccountsPageReceivingTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AccountsPageReceivingTab( QWidget * parent=0 );
    ~AccountsPageReceivingTab();
    QString helpAnchor() const;
    void save();

signals:
    void accountListChanged( const QStringList & );

private slots:
    void slotAccountSelected(const Akonadi::AgentInstance&);
    void slotAddAccount();
    void slotModifySelectedAccount();
    void slotRemoveSelectedAccount();
    void slotRestartSelectedAccount();
    void slotEditNotifications();
    void slotShowMailCheckMenu( const QString &, const QPoint & );
    void slotCustomizeAccountOrder();
    void slotIncludeInCheckChanged( bool checked );
    void slotOfflineOnShutdownChanged( bool checked );
    void slotCheckOnStatupChanged( bool checked );

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();

    struct RetrievalOptions {
        RetrievalOptions( bool manualCheck, bool offline, bool checkOnStartup )
            : IncludeInManualChecks( manualCheck )
            , OfflineOnShutdown( offline )
            ,CheckOnStartup( checkOnStartup ) {}
        bool IncludeInManualChecks;
        bool OfflineOnShutdown;
        bool CheckOnStartup;
    };

    QHash<QString, QSharedPointer<RetrievalOptions> > mRetrievalHash;
    //FIXME virtual void doResetToDefaultsOther();

private:
    QString mSpecialMailCollectionIdentifier;
    Ui_AccountsPageReceivingTab mAccountsReceiving;
    OrgFreedesktopAkonadiNewMailNotifierInterface *mNewMailNotifierInterface;
};

class KMAIL_EXPORT AccountsPage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit AccountsPage( const KComponentData &instance, QWidget *parent=0 );
    QString helpAnchor() const;


    // hrmpf. moc doesn't like nested classes with slots/signals...:
    typedef AccountsPageSendingTab SendingTab;
    typedef AccountsPageReceivingTab ReceivingTab;

signals:
    void accountListChanged( const QStringList & );

private:
    SendingTab   *mSendingTab;
    ReceivingTab *mReceivingTab;
};


//
//
// AppearancePage
//
//

class AppearancePageFontsTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageFontsTab( QWidget * parent=0 );
    QString helpAnchor() const;
    void save();

private slots:
    void slotFontSelectorChanged( int );

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    void updateFontSelector();
    void doResetToDefaultsOther();

private:
    QCheckBox    *mCustomFontCheck;
    KComboBox    *mFontLocationCombo;
    KFontChooser *mFontChooser;

    int          mActiveFontIndex;
    QFont        mFont[12];
};

class AppearancePageColorsTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageColorsTab( QWidget * parent=0 );
    QString helpAnchor() const;
    void save();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    void doResetToDefaultsOther();
    void loadColor( bool loadFromConfig );

private:
    QCheckBox    *mCustomColorCheck;
    ColorListBox *mColorList;
    QCheckBox    *mRecycleColorCheck;
    QSpinBox     *mCloseToQuotaThreshold;
};

class AppearancePageLayoutTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageLayoutTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private: // data
    QButtonGroup  *mFolderListGroup;
    QGroupBox     *mFolderListGroupBox;
    QButtonGroup  *mReaderWindowModeGroup;
    QGroupBox     *mReaderWindowModeGroupBox;
    QCheckBox     *mFolderQuickSearchCB;
    QButtonGroup  *mFolderToolTipsGroup;
    QGroupBox     *mFolderToolTipsGroupBox;
    QButtonGroup  *mFavoriteFoldersViewGroup;
    QGroupBox     *mFavoriteFoldersViewGroupBox;
};

class AppearancePageHeadersTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageHeadersTab( QWidget * parent=0 );

    QString helpAnchor() const;

    void save();

private: // methods
    void doLoadFromGlobalSettings();
    void doLoadOther();
    // virtual void doResetToDefaultsOther();
    void setDateDisplay( int id, const QString & format );

private: // data
    QCheckBox    *mDisplayMessageToolTips;
    QCheckBox    *mHideTabBarWithSingleTab;
    QCheckBox    *mTabsHaveCloseButton;
    MessageList::Utils::AggregationComboBox *mAggregationComboBox;
    MessageList::Utils::ThemeComboBox *mThemeComboBox;
    KButtonGroup *mDateDisplay;
    KLineEdit    *mCustomDateFormatEdit;
    QString       mCustomDateWhatsThis;

private slots:
    void slotLinkClicked( const QString & link );
    void slotSelectDefaultAggregation();
    void slotSelectDefaultTheme();
};

class AppearancePageReaderTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageReaderTab( QWidget * parent=0 );

    QString helpAnchor() const;

    void save();

private:
    void doLoadOther();
    void doResetToDefaultsOther();

private: // data
    QCheckBox *mCloseAfterReplyOrForwardCheck;
    MessageViewer::ConfigureWidget *mViewerSettings;
};


class AppearancePageSystemTrayTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageSystemTrayTab( QWidget * parent=0 );

    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();

private: // data
    QCheckBox    *mSystemTrayCheck;
    QCheckBox    *mSystemTrayShowUnreadMail;
    KButtonGroup *mSystemTrayGroup;
};


class TagListWidgetItem : public QListWidgetItem
{
public:
    explicit TagListWidgetItem( QListWidget *parent = 0);
    explicit TagListWidgetItem( const QIcon & icon, const QString & text, QListWidget * parent = 0);

    ~TagListWidgetItem();
    void setKMailTag( const MailCommon::Tag::Ptr& tag );
    MailCommon::Tag::Ptr kmailTag() const;
private:
    MailCommon::Tag::Ptr mTag;
};

/**Configuration tab in the appearance page for modifying the available set of
+message tags*/
class AppearancePageMessageTagTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit AppearancePageMessageTagTab( QWidget * parent=0);
    ~AppearancePageMessageTagTab();

    QString helpAnchor() const;

    void save();

public slots:
    /**Enables/disables Add button according to whether @p aText is empty.
  Connected to signal of the line edit widget for adding tags
  @param aText String to change add button according to
  */
    void slotAddLineTextChanged( const QString &aText );
    /**Creates a generic tag with the visible name from the line edit widget for
    adding tags. Adds it to the end of the list and selects. Empties the line
    edit widget*/
    void slotAddNewTag();
    /**Removes the currently selected text in the list box.*/
    void slotRemoveTag();
    /**Increases the currently selected tag's priority and handles related visual
  changes*/
    void slotMoveTagUp();
    /**Decreases the currently selected tag's priority and handles related visual
  changes*/
    void slotMoveTagDown();

private slots:
    /*Handles necessary processing when the selection in the edit box changes.
  Records the unselected tag's information, and applies visual changes
  necessary depending on the description of the new tag. Private since doesn't
  change the selection of the edit box itself*/
    void slotSelectionChanged();
    /*This slot is necessary so that apply button is not activated when we are
  only applying visual changes after selecting a new tag in the list box*/
    void slotEmitChangeCheck();
    /*Transfers the tag settings from the widgets to the internal data structures.
  Private since passing a wrong parameter modifies another tag's data*/
    void slotRecordTagSettings( int aIndex );
    /*Transfers the tag settings from the internal data structures to the widgets.
  Private since passing a wrong parameter visualizes another tag's data*/
    void slotUpdateTagSettingWidgets( int aIndex );
    /*Transfers changes in the tag name edit box to the list box for tags. Private
  since calling externally decouples the name in the list box from name edit box*/
    void slotNameLineTextChanged( const QString & );
    void slotIconNameChanged( const QString &iconName );
    void slotRowsMoved( const QModelIndex &,
                        int sourcestart, int sourceEnd,
                        const QModelIndex &, int destinationRow );

private:
    void doLoadFromGlobalSettings();
    void swapTagsInListBox( const int first, const int second );
    void updateButtons();

private: // data

    KLineEdit *mTagAddLineEdit;
    QPushButton *mTagAddButton, *mTagRemoveButton,
    *mTagUpButton, *mTagDownButton;

    QListWidget *mTagListBox;

    QGroupBox *mTagsGroupBox, *mTagSettingGroupBox;

    MailCommon::TagWidget *mTagWidget;

    // So we can compare to mMsgTagList and see if the user changed tags
    QList<MailCommon::TagPtr> mOriginalMsgTagList;

    /*Used to safely call slotRecordTagSettings when the selection in
    list box changes*/
    int mPreviousTag;
    bool mNepomukActive;
};
class KMAIL_EXPORT AppearancePage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit AppearancePage( const KComponentData &instance, QWidget *parent=0 );

    QString helpAnchor() const;

    // hrmpf. moc doesn't like nested classes with slots/signals...:
    typedef AppearancePageFontsTab FontsTab;
    typedef AppearancePageColorsTab ColorsTab;
    typedef AppearancePageLayoutTab LayoutTab;
    typedef AppearancePageHeadersTab HeadersTab;
    typedef AppearancePageReaderTab ReaderTab;
    typedef AppearancePageSystemTrayTab SystemTrayTab;
    typedef AppearancePageMessageTagTab MessageTagTab;

private:
    FontsTab      *mFontsTab;
    ColorsTab     *mColorsTab;
    LayoutTab     *mLayoutTab;
    HeadersTab    *mHeadersTab;
    ReaderTab     *mReaderTab;
    SystemTrayTab *mSystemTrayTab;
    MessageTagTab *mMessageTagTab;
};

//
//
// Composer Page
//
//

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
    MessageComposer::ComposerAutoCorrectionWidget *autocorrectionWidget;
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

//
//
// SecurityPage
//
//

class SecurityPageGeneralTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageGeneralTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui_SecurityPageGeneralTab mSGTab;

private slots:
    void slotLinkClicked( const QString & link );
};

class SecurityPageMDNTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageMDNTab( QWidget * parent=0 );
    QString helpAnchor() const;

    void save();

private:
    void doLoadOther();

private:
    QButtonGroup *mMDNGroup;
    QButtonGroup *mOrigQuoteGroup;
    Ui_SecurityPageMDNTab mUi;

private slots:
    void slotLinkClicked( const QString & link );
};

class SecurityPageComposerCryptoTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageComposerCryptoTab( QWidget * parent=0 );
    ~SecurityPageComposerCryptoTab();

    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::ComposerCryptoConfiguration* mWidget;
};

class SecurityPageWarningTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageWarningTab( QWidget * parent=0 );
    ~SecurityPageWarningTab();

    QString helpAnchor() const;

    void save();

private Q_SLOTS:
    void slotReenableAllWarningsClicked();
    void slotConfigureGnupg();
    //void slotConfigureChiasmus();

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::WarningConfiguration* mWidget;
};

class SecurityPageSMimeTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageSMimeTab( QWidget * parent=0 );
    ~SecurityPageSMimeTab();

    QString helpAnchor() const;

    void save();

private slots:
    void slotUpdateHTTPActions();

private:
    //virtual void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui::SMimeConfiguration* mWidget;
    Kleo::CryptoConfig* mConfig;
};

#ifndef KDEPIM_NO_WEBKIT
class SecurityPageAdBlockTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit SecurityPageAdBlockTab( QWidget * parent=0 );
    ~SecurityPageAdBlockTab();

    QString helpAnchor() const;

    void save();

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    void doResetToDefaultsOther();

private:
    MessageViewer::AdBlockSettingWidget *mWidget;
};
#endif


class KMAIL_EXPORT SecurityPage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit SecurityPage( const KComponentData &instance, QWidget *parent=0 );

    QString helpAnchor() const;

    typedef SecurityPageGeneralTab GeneralTab;
    typedef SecurityPageMDNTab MDNTab;
    typedef SecurityPageComposerCryptoTab ComposerCryptoTab;
    typedef SecurityPageWarningTab WarningTab;
    typedef SecurityPageSMimeTab SMimeTab;

private:
    GeneralTab    *mGeneralTab;
    ComposerCryptoTab *mComposerCryptoTab;
    WarningTab    *mWarningTab;
    SMimeTab      *mSMimeTab;
#ifndef KDEPIM_NO_WEBKIT
    SecurityPageAdBlockTab *mSAdBlockTab;
#endif
};


//
//
// MiscPage
//
//

class MiscPageFolderTab : public ConfigModuleTab {
    Q_OBJECT
public:
    explicit MiscPageFolderTab( QWidget * parent=0 );

    void save();
    QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings();
    void doLoadOther();
    //FIXME virtual void doResetToDefaultsOther();

private:
    Ui_MiscMainTab mMMTab;
    MailCommon::FolderRequester *mOnStartupOpenFolder;
};

class MiscPageInviteTab : public ConfigModuleTab  {
    Q_OBJECT
public:
    explicit MiscPageInviteTab( QWidget * parent=0 );
    void save();
    void doResetToDefaultsOther();

private:
    void doLoadFromGlobalSettings();

private:
    MessageViewer::InvitationSettings *mInvitationUi;
};


class MiscPageProxyTab : public ConfigModuleTab  {
    Q_OBJECT
public:
    explicit MiscPageProxyTab( QWidget * parent=0 );
    void save();
private:
    KCModuleProxy *mProxyModule;
};


class MiscPageAgentSettingsTab : public ConfigModuleTab  {
    Q_OBJECT
public:
    explicit MiscPageAgentSettingsTab( QWidget * parent=0 );
    void save();
    void doResetToDefaultsOther();

    QString helpAnchor() const;

private:
    void doLoadFromGlobalSettings();

private:
    ConfigureAgentsWidget *mConfigureAgent;
};

class MiscPagePrintingTab : public ConfigModuleTab  {
    Q_OBJECT
public:
    explicit MiscPagePrintingTab( QWidget * parent=0 );
    void save();
    void doResetToDefaultsOther();

private:
    void doLoadFromGlobalSettings();

private:
    MessageViewer::PrintingSettings* mPrintingUi;
};


class KMAIL_EXPORT MiscPage : public ConfigModuleWithTabs {
    Q_OBJECT
public:
    explicit MiscPage( const KComponentData &instance, QWidget *parent=0 );
    QString helpAnchor() const;

    typedef MiscPageFolderTab FolderTab;
    typedef MiscPageInviteTab InviteTab;
    typedef MiscPageProxyTab ProxyTab;

private:
    FolderTab * mFolderTab;
    InviteTab * mInviteTab;
    ProxyTab * mProxyTab;
    MiscPageAgentSettingsTab *mAgentSettingsTab;
    MiscPagePrintingTab *mPrintingTab;
};

#endif // _CONFIGURE_DIALOG_PRIVATE_H_
