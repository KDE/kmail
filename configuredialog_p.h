// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include <config-enterprise.h>

#include "kmail_export.h"

#include <QPointer>
#include <QString>
#include <QStringList>
#include <QLabel>
#include <QList>
#include <QShowEvent>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QHash>

#include <klineedit.h>
#include <kcombobox.h>
#include <kdialog.h>
#include <kcmodule.h>
#include <klocale.h>

#include <akonadi/agentinstance.h>

#include "ui_composercryptoconfiguration.h"
#include "ui_warningconfiguration.h"
#include "ui_smimeconfiguration.h"
#include "ui_customtemplates_base.h"
#include "ui_miscpagemaintab.h"
#include "ui_miscpageinvitetab.h"
#include "ui_securitypagegeneraltab.h"
#include "ui_identitypage.h"
#include "ui_accountspagereceivingtab.h"

class QPushButton;
class QLabel;
class QCheckBox;
class QFont;
class QRegExpValidator;
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
class SimpleStringListEditor;
class KConfig;
class SMimeConfiguration;
class TemplatesConfiguration;
class CustomTemplates;
class KMMessageTagDescription;
class KColorCombo;
class KFontRequester;
class KIconButton;
class KKeySequenceWidget;
class KComboBox;

namespace MessageList {
  namespace Utils {
    class AggregationComboBox;
    class ThemeComboBox;
  }
}
namespace KMail {
  class IdentityDialog;
  class IdentityListView;
  class IdentityListViewItem;
}
namespace Kleo {
  class BackendConfigWidget;
  class CryptoConfig;
  }

namespace KPIM {
  class ColorListBox;
}

class WarningConfiguration : public QWidget, public Ui::WarningConfiguration
{
public:
  WarningConfiguration( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class SMimeConfiguration : public QWidget, public Ui::SMimeConfiguration
{
public:
  SMimeConfiguration( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class ComposerCryptoConfiguration : public QWidget, public Ui::ComposerCryptoConfiguration
{
public:
  ComposerCryptoConfiguration( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class NewIdentityDialog : public KDialog
{
  Q_OBJECT

public:
  enum DuplicateMode { Empty, ControlCenter, ExistingEntry };

  explicit NewIdentityDialog( const QStringList & identities,
                     QWidget *parent=0 );

  QString identityName() const { return mLineEdit->text(); }
  QString duplicateIdentity() const { return mComboBox->currentText(); }
  DuplicateMode duplicateMode() const;

protected slots:
  virtual void slotEnableOK( const QString & );

private:
  KLineEdit  *mLineEdit;
  KComboBox  *mComboBox;
  QButtonGroup *mButtonGroup;
};

#include <kvbox.h>
class ConfigModule : public KCModule {
  Q_OBJECT
public:
  explicit ConfigModule( const KComponentData &instance, QWidget *parent=0 )
     : KCModule ( instance, parent )
     {}
  ~ConfigModule() {}

  virtual void load() = 0;
  virtual void save() = 0;
  virtual void defaults() {}

  /** Should return the help anchor for this page or tab */
  virtual QString helpAnchor() const = 0;
};


// Individual tab of a ConfigModuleWithTabs
class ConfigModuleTab : public QWidget {
  Q_OBJECT
public:
   ConfigModuleTab( QWidget *parent=0 )
      : QWidget( parent )
      {}
  ~ConfigModuleTab() {}
  void load();
  virtual void save() = 0;
  void defaults();
signals:
   // forwarded to the ConfigModule
  void changed(bool);
public slots:
  void slotEmitChanged();
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
  void addTab( ConfigModuleTab* tab, const QString & title );

private:
  KTabWidget *mTabWidget;

};


//
//
// IdentityPage
//
//

class KMAIL_EXPORT IdentityPage : public ConfigModule {
  Q_OBJECT
public:
  explicit IdentityPage( const KComponentData &instance, QWidget *parent = 0 );
  ~IdentityPage() {}

  QString helpAnchor() const;

  void load();
  void save();

private slots:
  void slotNewIdentity();
  void slotModifyIdentity();
  void slotRemoveIdentity();
  /** Connected to @p mRenameButton's clicked() signal. Just does a
      QTreeWidget::editItem on the selected item */
  void slotRenameIdentity();
  /** connected to @p mIdentityList's renamed() signal. Validates the
      new name and sets it in the KPIMIdentities::IdentityManager */
  void slotRenameIdentity( KMail::IdentityListViewItem *, const QString & );
  void slotContextMenu( KMail::IdentityListViewItem *, const QPoint & );
  void slotSetAsDefault();
  void slotIdentitySelectionChanged();

private: // methods
  void refreshList();

private: // data members
  KMail::IdentityDialog   *mIdentityDialog;
  int                      mOldNumberOfIdentities;

  Ui_IdentityPage mIPage;
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
  AccountsPageSendingTab( QWidget * parent=0 );
  virtual ~AccountsPageSendingTab();
  QString helpAnchor() const;
  void save();

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  QCheckBox   *mConfirmSendCheck;
  KComboBox   *mSendOnCheckCombo;
  KComboBox   *mSendMethodCombo;
  KComboBox   *mMessagePropertyCombo;
  KLineEdit   *mDefaultDomainEdit;
};


class AccountsPageReceivingTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AccountsPageReceivingTab( QWidget * parent=0 );
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
  void slotEditNotifications();

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  Ui_AccountsPageReceivingTab mAccountsReceiving;
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
  AppearancePageFontsTab( QWidget * parent=0 );
  QString helpAnchor() const;
  void save();

private slots:
  void slotFontSelectorChanged( int );

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();
  void updateFontSelector();

private:
  QCheckBox    *mCustomFontCheck;
  KComboBox    *mFontLocationCombo;
  KFontChooser *mFontChooser;

  int          mActiveFontIndex;
  QFont        mFont[13];
};

class AppearancePageColorsTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageColorsTab( QWidget * parent=0 );
  QString helpAnchor() const;
  void save();

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  QCheckBox    *mCustomColorCheck;
  KPIM::ColorListBox *mColorList;
  QCheckBox    *mRecycleColorCheck;
  QSpinBox     *mCloseToQuotaThreshold;
};

class AppearancePageLayoutTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageLayoutTab( QWidget * parent=0 );
  QString helpAnchor() const;

  void save();

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private: // data
  QButtonGroup  *mFolderListGroup;
  QGroupBox     *mFolderListGroupBox;
  QButtonGroup  *mReaderWindowModeGroup;
  QGroupBox     *mReaderWindowModeGroupBox;
  QCheckBox     *mFavoriteFolderViewCB;
#if 0
  QCheckBox     *mFolderQuickSearchCB;
#endif
  QButtonGroup  *mFolderToolTipsGroup;
  QGroupBox     *mFolderToolTipsGroupBox;
};

class AppearancePageHeadersTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageHeadersTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();

private: // methods
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();
  void setDateDisplay( int id, const QString & format );

private: // data
  QCheckBox    *mDisplayMessageToolTips;
  QCheckBox    *mHideTabBarWithSingleTab;
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
  AppearancePageReaderTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();
  void readCurrentFallbackCodec();
  void readCurrentOverrideCodec();

private: // data
  QCheckBox *mShowColorbarCheck;
  QCheckBox *mShowSpamStatusCheck;
  QCheckBox *mShowEmoticonsCheck;
  QCheckBox *mShowExpandQuotesMark;
  KIntSpinBox  *mCollapseQuoteLevelSpin;
  QCheckBox *mShrinkQuotesCheck;
  KComboBox *mCharsetCombo;
  KComboBox *mOverrideCharsetCombo;
};


class AppearancePageSystemTrayTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageSystemTrayTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();

private:
  virtual void doLoadFromGlobalSettings();

private: // data
  QCheckBox    *mSystemTrayCheck;
  KButtonGroup *mSystemTrayGroup;
};

/**Configuration tab in the appearance page for modifying the available set of
+message tags*/
class AppearancePageMessageTagTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageMessageTagTab( QWidget * parent=0);
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

private:
  virtual void doLoadFromGlobalSettings();
  void swapTagsInListBox( const int first, const int second );

private: // data
  KLineEdit *mTagNameLineEdit, *mTagAddLineEdit;
  QPushButton *mTagAddButton, *mTagRemoveButton,
              *mTagUpButton, *mTagDownButton;

  QListWidget *mTagListBox;

  QCheckBox *mTextColorCheck, *mBackgroundColorCheck,
            *mTextFontCheck, *mInToolbarCheck;

  QGroupBox *mTagsGroupBox, *mTagSettingGroupBox;

  KColorCombo *mTextColorCombo, *mBackgroundColorCombo;

  KFontRequester *mFontRequester;

  KIconButton *mIconButton;

  KKeySequenceWidget *mKeySequenceWidget;

  QHash<QString,KMMessageTagDescription*> *mMsgTagDict;
  QList<KMMessageTagDescription*> *mMsgTagList;
  /*If true, changes to the widgets activate the Apply button*/
  bool mEmitChanges;
  /*Used to safely call slotRecordTagSettings when the selection in
    list box changes*/
  int mPreviousTag;
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
  ComposerPageGeneralTab( QWidget * parent=0 );
  QString helpAnchor() const;

  void save();
protected slots:
  void slotConfigureRecentAddresses();
  void slotConfigureCompletionOrder();

private:
  virtual void doLoadFromGlobalSettings();

private:
  QCheckBox     *mAutoAppSignFileCheck;
  QCheckBox     *mTopQuoteCheck;
  QCheckBox     *mDashDashCheck;
  QCheckBox     *mSmartQuoteCheck;
  QCheckBox     *mAutoRequestMDNCheck;
  QCheckBox	*mShowRecentAddressesInComposer;
  QCheckBox     *mWordWrapCheck;
  KIntSpinBox   *mWrapColumnSpin;
  KIntSpinBox   *mAutoSave;
  QCheckBox     *mExternalEditorCheck;
  KUrlRequester *mEditorRequester;
#ifdef KDEPIM_ENTERPRISE_BUILD
  KComboBox     *mForwardTypeCombo;
  QCheckBox     *mRecipientCheck;
  KIntSpinBox   *mRecipientSpin;
#endif
};

class ComposerPageTemplatesTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageTemplatesTab( QWidget * parent = 0 );
  QString helpAnchor() const;

  void save();

private slots:

private:
  virtual void doLoadFromGlobalSettings();

private:
    TemplatesConfiguration* mWidget;
};

class ComposerPageCustomTemplatesTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageCustomTemplatesTab( QWidget * parent = 0 );
  QString helpAnchor() const;

  void save();

private slots:

private:
  virtual void doLoadFromGlobalSettings();

private:
    CustomTemplates* mWidget;
};

class ComposerPageSubjectTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageSubjectTab( QWidget * parent = 0 );
  QString helpAnchor() const;

  void save();

private:
  virtual void doLoadFromGlobalSettings();

private:
  SimpleStringListEditor *mReplyListEditor;
  QCheckBox              *mReplaceReplyPrefixCheck;
  SimpleStringListEditor *mForwardListEditor;
  QCheckBox              *mReplaceForwardPrefixCheck;
};

class ComposerPageCharsetTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageCharsetTab( QWidget * parent=0 );
  QString helpAnchor() const;

  void save();

private slots:
  void slotVerifyCharset(QString&);

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  SimpleStringListEditor *mCharsetListEditor;
  QCheckBox              *mKeepReplyCharsetCheck;
};

class ComposerPageHeadersTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageHeadersTab( QWidget * parent=0 );
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
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  QCheckBox   *mCreateOwnMessageIdCheck;
  KLineEdit   *mMessageIdSuffixEdit;
  QRegExpValidator *mMessageIdSuffixValidator;
  ListView    *mTagList;
  QPushButton *mRemoveHeaderButton;
  KLineEdit   *mTagNameEdit;
  KLineEdit   *mTagValueEdit;
  QLabel      *mTagNameLabel;
  QLabel      *mTagValueLabel;
};

class ComposerPageAttachmentsTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageAttachmentsTab( QWidget * parent=0 );
  QString helpAnchor() const;

  void save();

private slots:
  void slotOutlookCompatibleClicked();

private:
  virtual void doLoadFromGlobalSettings();
  //FIXME virtual void doResetToDefaultsOther();

private:
  QCheckBox   *mOutlookCompatibleCheck;
  QCheckBox   *mMissingAttachmentDetectionCheck;
  SimpleStringListEditor *mAttachWordsListEditor;
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

private:
  GeneralTab  *mGeneralTab;
  TemplatesTab  *mTemplatesTab;
  CustomTemplatesTab  *mCustomTemplatesTab;
  SubjectTab  *mSubjectTab;
  CharsetTab  *mCharsetTab;
  HeadersTab  *mHeadersTab;
  AttachmentsTab  *mAttachmentsTab;
};

//
//
// SecurityPage
//
//

class SecurityPageGeneralTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageGeneralTab( QWidget * parent=0 );
  QString helpAnchor() const;

  void save();

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  QButtonGroup *mMDNGroup;
  QButtonGroup *mOrigQuoteGroup;
  Ui_SecurityPageGeneralTab mSGTab;

private slots:
    void slotLinkClicked( const QString & link );
};


class SecurityPageComposerCryptoTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageComposerCryptoTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  ComposerCryptoConfiguration* mWidget;
};

class SecurityPageWarningTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageWarningTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();

private slots:
  void slotReenableAllWarningsClicked();

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  WarningConfiguration* mWidget;
};

class SecurityPageSMimeTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageSMimeTab( QWidget * parent=0 );
  ~SecurityPageSMimeTab();

  QString helpAnchor() const;

  void save();

private slots:
  void slotUpdateHTTPActions();

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  SMimeConfiguration* mWidget;
  Kleo::CryptoConfig* mConfig;
};

class SecurityPageCryptPlugTab : public ConfigModuleTab
{
  Q_OBJECT
public:
  SecurityPageCryptPlugTab( QWidget * parent = 0 );
  ~SecurityPageCryptPlugTab();

  QString helpAnchor() const;

  void save();

private:
  virtual void doLoadOther();
  //virtual void doResetToDefaultsOther();

private:
  Kleo::BackendConfigWidget * mBackendConfig;
};

class KMAIL_EXPORT SecurityPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  explicit SecurityPage( const KComponentData &instance, QWidget *parent=0 );

  QString helpAnchor() const;

  typedef SecurityPageGeneralTab GeneralTab;
  typedef SecurityPageComposerCryptoTab ComposerCryptoTab;
  typedef SecurityPageWarningTab WarningTab;
  typedef SecurityPageSMimeTab SMimeTab;
  typedef SecurityPageCryptPlugTab CryptPlugTab;

private:
  GeneralTab    *mGeneralTab;
  ComposerCryptoTab *mComposerCryptoTab;
  WarningTab    *mWarningTab;
  SMimeTab      *mSMimeTab;
  CryptPlugTab  *mCryptPlugTab;
};


//
//
// MiscPage
//
//

class MiscPageFolderTab : public ConfigModuleTab {
  Q_OBJECT
public:
  MiscPageFolderTab( QWidget * parent=0 );

  void save();
 QString helpAnchor() const;

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  Ui_MiscMainTab mMMTab;
};

class MiscPageInviteTab : public ConfigModuleTab  {
  Q_OBJECT
public:
  MiscPageInviteTab( QWidget * parent=0 );
  void save();
  QString helpAnchor() const;

private slots:
  void slotLegacyBodyInvitesToggled( bool on );

private:
  virtual void doLoadFromGlobalSettings();

private:
  Ui_MiscInviteTab mMITab;
};

class KMAIL_EXPORT MiscPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  explicit MiscPage( const KComponentData &instance, QWidget *parent=0 );
  QString helpAnchor() const;

  typedef MiscPageFolderTab FolderTab;
  typedef MiscPageInviteTab InviteTab;

private:
  FolderTab * mFolderTab;
  InviteTab * mInviteTab;
};

#endif // _CONFIGURE_DIALOG_PRIVATE_H_
