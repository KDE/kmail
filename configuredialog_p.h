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

#include <kdialog.h>
#include <kcmodule.h>
#include <klocale.h>

#include <akonadi/agentinstance.h>

#include "ui_composercryptoconfiguration.h"
#include "ui_warningconfiguration.h"
#include "ui_smimeconfiguration.h"
#include "ui_customtemplates_base.h"
#include "ui_miscpagemaintab.h"
#include "ui_securitypagegeneraltab.h"
#include "ui_accountspagereceivingtab.h"
#include "tag.h"
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
class TemplatesConfiguration;
class CustomTemplates;
class KColorCombo;
class KFontRequester;
class KIconButton;
class KKeySequenceWidget;
class KComboBox;
class ColorListBox;

namespace MessageList {
  namespace Utils {
    class AggregationComboBox;
    class ThemeComboBox;
  }
}

namespace MessageViewer {
  class ConfigureWidget;
class InvitationSettings;
}

namespace KMail {
  class Tag;
  typedef QSharedPointer<Tag> TagPtr;
}

namespace MailCommon {
  class FolderRequester;
}

namespace Kleo {
  class CryptoConfig;
}

// Individual tab of a ConfigModuleWithTabs
class ConfigModuleTab : public QWidget {
  Q_OBJECT
public:
   ConfigModuleTab( QWidget *parent=0 )
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
  void slotRestartSelectedAccount();
  void slotEditNotifications();
  void slotShowMailCheckMenu( const QString &, const QPoint & );
  void slotIncludeInCheckChanged( bool checked );
  void slotOfflineOnShutdownChanged( bool checked );
  void slotCheckOnStatupChanged( bool checked );

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();

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
  void updateFontSelector();
  virtual void doResetToDefaultsOther();

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
  AppearancePageColorsTab( QWidget * parent=0 );
  QString helpAnchor() const;
  void save();

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  virtual void doResetToDefaultsOther();
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
  QCheckBox     *mFolderQuickSearchCB;
  QButtonGroup  *mFolderToolTipsGroup;
  QGroupBox     *mFolderToolTipsGroupBox;
  QButtonGroup  *mFavoriteFoldersViewGroup;
  QGroupBox     *mFavoriteFoldersViewGroupBox;
};

class AppearancePageHeadersTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageHeadersTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();

private: // methods
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  // virtual void doResetToDefaultsOther();
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
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private: // data
  QCheckBox *mCloseAfterReplyOrForwardCheck;
  MessageViewer::ConfigureWidget *mViewerSettings;
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


class TagListWidgetItem : public QListWidgetItem
{
public:
  explicit TagListWidgetItem( QListWidget *parent = 0);
  explicit TagListWidgetItem( const QIcon & icon, const QString & text, QListWidget * parent = 0);

  ~TagListWidgetItem();
  void setKMailTag( const KMail::Tag::Ptr& tag );
  KMail::Tag::Ptr setKMailTag() const;
private:
  KMail::Tag::Ptr mTag;
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

  // List of all Tags currently in the list
  QList<KMail::TagPtr> mMsgTagList;

  // So we can compare to mMsgTagList and see if the user changed tags
  QList<KMail::TagPtr> mOriginalMsgTagList;

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
  ComposerPageGeneralTab( QWidget * parent=0 );
  QString helpAnchor() const;

  void save();
protected slots:
  void slotConfigureRecentAddresses();
  void slotConfigureCompletionOrder();

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doResetToDefaultsOther();

private:
  QCheckBox     *mAutoAppSignFileCheck;
  QCheckBox     *mTopQuoteCheck;
  QCheckBox     *mDashDashCheck;
  QCheckBox     *mReplyUsingHtml;
  QCheckBox     *mSmartQuoteCheck;
  QCheckBox     *mStripSignatureCheck;
  QCheckBox     *mQuoteSelectionOnlyCheck;
  QCheckBox     *mAutoRequestMDNCheck;
  QCheckBox	*mShowRecentAddressesInComposer;
  QCheckBox     *mWordWrapCheck;
  KIntSpinBox   *mWrapColumnSpin;
  KIntSpinBox   *mAutoSave;
  QCheckBox     *mExternalEditorCheck;
  KUrlRequester *mEditorRequester;
  KIntSpinBox   *mMaximumRecipients;
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
  virtual void doResetToDefaultsOther();
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
  virtual void doResetToDefaultsOther();

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
  virtual void doResetToDefaultsOther();

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
  virtual void doResetToDefaultsOther();

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
  explicit SecurityPageComposerCryptoTab( QWidget * parent=0 );
  ~SecurityPageComposerCryptoTab();

  QString helpAnchor() const;

  void save();

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
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
  void slotConfigureChiasmus();

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
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
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  Ui::SMimeConfiguration* mWidget;
  Kleo::CryptoConfig* mConfig;
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

private:
  GeneralTab    *mGeneralTab;
  ComposerCryptoTab *mComposerCryptoTab;
  WarningTab    *mWarningTab;
  SMimeTab      *mSMimeTab;
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
  MailCommon::FolderRequester *mOnStartupOpenFolder;
};

class MiscPageInviteTab : public ConfigModuleTab  {
  Q_OBJECT
public:
  MiscPageInviteTab( QWidget * parent=0 );
  void save();
  virtual void doResetToDefaultsOther();

private:
  virtual void doLoadFromGlobalSettings();

private:
  MessageViewer::InvitationSettings *mInvitationUi;
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
