// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include "kmail_export.h"

#include <klineedit.h>
#include <QComboBox>
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

#include <kdialog.h>
#include <kcmodule.h>
#include <klocale.h>
#include "ui_composercryptoconfiguration.h"
#include "ui_warningconfiguration.h"
#include "ui_smimeconfiguration.h"
#include "ui_customtemplates_base.h"

class QPushButton;
class QLabel;
class QCheckBox;
class QFont;
class QTabWidget;
class QRegExpValidator;
class QPoint;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QListWidget;

class KButtonGroup;
class KUrlRequester;
class KFontChooser;
class KVBox;
class KMAccount;
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

namespace KMail {
  class IdentityDialog;
  class IdentityListView;
  class IdentityListViewItem;
  class AccountComboBox;
  class FolderRequester;
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
  QLineEdit  *mLineEdit;
  QComboBox  *mComboBox;
  QButtonGroup *mButtonGroup;
};

//
//
// Profile dialog
//
//

class ProfileDialog : public KDialog {
  Q_OBJECT
public:
  ProfileDialog( QWidget * parent=0 );

signals:
  void profileSelected( KConfig * profile );

private slots:
  void slotSelectionChanged();
  void slotOk();

private:
  void setup();

private:
  QTreeWidget *mListView;
  QStringList mProfileList;
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

signals:
  /** Emitted when the installation of a profile is
      requested. All connected kcms should load the values
      from the profile only for those entries that
      really have keys defined in the profile.
   */
   void installProfile( KConfig * profile );

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
  // the below are optional
  virtual void installProfile( KConfig* ) {};
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
  virtual void installProfile( KConfig * profile );

protected:
  void addTab( ConfigModuleTab* tab, const QString & title );

private:
  QTabWidget *mTabWidget;

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

  KMail::IdentityListView *mIdentityList;
  QPushButton             *mModifyButton;
  QPushButton             *mRenameButton;
  QPushButton             *mRemoveButton;
  QPushButton             *mSetAsDefaultButton;
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
  QComboBox   *mSendOnCheckCombo;
  QComboBox   *mSendMethodCombo;
  QComboBox   *mMessagePropertyCombo;
  QLineEdit   *mDefaultDomainEdit;
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
  void slotAccountSelected();
  void slotAddAccount();
  void slotModifySelectedAccount();
  void slotRemoveSelectedAccount();
  void slotEditNotifications();

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();
  QStringList occupiedNames();

private:
  ListView      *mAccountList;
  QPushButton   *mModifyAccountButton;
  QPushButton   *mRemoveAccountButton;
  QCheckBox     *mBeepNewMailCheck;
  QCheckBox     *mVerboseNotificationCheck;
  QCheckBox     *mCheckmailStartupCheck;
  QPushButton   *mOtherNewMailActionsButton;

  QList< QPointer<KMAccount> > mAccountsToDelete;
  QList< QPointer<KMAccount> > mNewAccounts;
  struct ModifiedAccountsType {
    QPointer< KMAccount > oldAccount;
    QPointer< KMAccount > newAccount;
  };
  // ### make this a qptrlist:
  QList< ModifiedAccountsType* >  mModifiedAccounts;
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

  void installProfile( KConfig * profile );

private slots:
  void slotFontSelectorChanged( int );

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();
  void updateFontSelector();

private:
  QCheckBox    *mCustomFontCheck;
  QComboBox    *mFontLocationCombo;
  KFontChooser *mFontChooser;

  int          mActiveFontIndex;
  QFont        mFont[14];
};

class AppearancePageColorsTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageColorsTab( QWidget * parent=0 );
  QString helpAnchor() const;
  void save();

  void installProfile( KConfig * profile );

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
  void installProfile( KConfig * profile );

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private: // data
  QButtonGroup  *mFolderListGroup;
  QGroupBox     *mFolderListGroupBox;
  QButtonGroup  *mMIMETreeLocationGroup;
  QGroupBox     *mMIMETreeLocationGroupBox;
  QButtonGroup  *mMIMETreeModeGroup;
  QGroupBox     *mMIMETreeModeGroupBox;
  QButtonGroup  *mReaderWindowModeGroup;
  QGroupBox     *mReaderWindowModeGroupBox;
  QCheckBox     *mFavoriteFolderViewCB;
  QCheckBox     *mFolderQuickSearchCB;
};

class AppearancePageHeadersTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageHeadersTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();
  void installProfile( KConfig * profile );

private: // methods
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();
  void setDateDisplay( int id, const QString & format );

private: // data
  QCheckBox    *mShowQuickSearch;
  QCheckBox    *mMessageSizeCheck;
  QCheckBox    *mAttachmentCheck;
  QCheckBox    *mNestedMessagesCheck;
  QCheckBox    *mCryptoIconsCheck;
  KButtonGroup *mNestingPolicy;
  KButtonGroup *mDateDisplay;
  QLineEdit    *mCustomDateFormatEdit;
  QString      mCustomDateWhatsThis;

private slots:
  void slotLinkClicked( const QString & link );
};

class AppearancePageReaderTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageReaderTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();
  void installProfile( KConfig * profile );

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
  QComboBox *mCharsetCombo;
  QComboBox *mOverrideCharsetCombo;
};


class AppearancePageSystemTrayTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageSystemTrayTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();
  void installProfile( KConfig * profile );

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
  void installProfile( KConfig * profile );

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

private:
  virtual void doLoadFromGlobalSettings();
  void swapTagsInListBox( const int first, const int second );

private: // data
  QLineEdit *mTagNameLineEdit, *mTagAddLineEdit;
  QPushButton *mTagAddButton, *mTagRemoveButton,
              *mTagUpButton, *mTagDownButton;

  QListWidget *mTagListBox;

  QCheckBox *mTextColorCheck,
            *mTextFontCheck, *mInToolbarCheck;

  QGroupBox *mTagsGroupBox, *mTagSettingGroupBox;

  KColorCombo *mTextColorCombo;

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
  void installProfile( KConfig * profile );
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
  QLineEdit   *mMessageIdSuffixEdit;
  QRegExpValidator *mMessageIdSuffixValidator;
  ListView    *mTagList;
  QPushButton *mRemoveHeaderButton;
  QLineEdit   *mTagNameEdit;
  QLineEdit   *mTagValueEdit;
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
  void installProfile( KConfig * profile );

private:
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  QCheckBox    *mExternalReferences;
  QCheckBox    *mHtmlMailCheck;
  QCheckBox    *mNoMDNsWhenEncryptedCheck;
  QButtonGroup *mMDNGroup;
  QButtonGroup *mOrigQuoteGroup;
  QCheckBox    *mAutomaticallyImportAttachedKeysCheck;
  QCheckBox    *mAlwaysDecrypt;
  QString       mHtmlWhatsThis;
  QString       mExternalWhatsThis;
  QString       mReceiptWhatsThis;

private slots:
    void slotLinkClicked( const QString & link );
};


class SecurityPageComposerCryptoTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageComposerCryptoTab( QWidget * parent=0 );

  QString helpAnchor() const;

  void save();
  void installProfile( KConfig * profile );

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
  void installProfile( KConfig * profile );

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
  void installProfile( KConfig * profile );

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

  // OpenPGP tab is special:
  void installProfile( KConfig * profile );

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
  QCheckBox    *mEmptyFolderConfirmCheck;
  QCheckBox    *mExcludeImportantFromExpiry;
  QComboBox    *mLoopOnGotoUnread;
  QComboBox    *mMailboxPrefCombo;
  QComboBox    *mActionEnterFolder;
  QCheckBox    *mEmptyTrashCheck;
  QCheckBox    *mDelayedMarkAsRead;
  KIntSpinBox  *mDelayedMarkTime;
  QCheckBox    *mShowPopupAfterDnD;
  KMail::FolderRequester *mOnStartupOpenFolder;
  QComboBox    *mQuotaCmbBox;
};

class MiscPageGroupwareTab : public ConfigModuleTab  {
  Q_OBJECT
public:
  MiscPageGroupwareTab( QWidget * parent=0 );
  void save();
  QString helpAnchor() const;

private slots:
  void slotStorageFormatChanged( int );
  void slotLegacyBodyInvitesToggled( bool on );

private:
  virtual void doLoadFromGlobalSettings();

private:
  QCheckBox* mEnableGwCB;
  QCheckBox* mEnableImapResCB;

  QWidget* mBox;
  KVBox* gBox;

  QComboBox* mStorageFormatCombo;
  QComboBox* mLanguageCombo;

  QLabel* mFolderComboLabel;
  QStackedWidget* mFolderComboStack;
  KMail::FolderRequester* mFolderCombo; // in the widgetstack
  KMail::AccountComboBox* mAccountCombo; // in the widgetstack

  QCheckBox* mHideGroupwareFolders;
  QCheckBox* mOnlyShowGroupwareFolders;
  QCheckBox* mSyncImmediately;
  QCheckBox* mDeleteInvitations;

  QCheckBox* mLegacyMangleFromTo;
  QCheckBox* mLegacyBodyInvites;
  QCheckBox* mExchangeCompatibleInvitations;
  QCheckBox* mAutomaticSending;
};

class KMAIL_EXPORT MiscPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  explicit MiscPage( const KComponentData &instance, QWidget *parent=0 );
  QString helpAnchor() const;

  typedef MiscPageFolderTab FolderTab;
  typedef MiscPageGroupwareTab GroupwareTab;

private:
  FolderTab * mFolderTab;
  GroupwareTab * mGroupwareTab;
};

//
//
// further helper classes:
//
//

class ListView : public QTreeWidget {
  Q_OBJECT
public:
  explicit ListView( QWidget *parent=0 );
  void resizeColums();

  virtual QSize sizeHint() const;

protected:
  virtual void resizeEvent( QResizeEvent *e );
  virtual void showEvent( QShowEvent *e );

private:
  int mVisibleItem;
};


#endif // _CONFIGURE_DIALOG_PRIVATE_H_
