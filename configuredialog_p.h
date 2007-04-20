// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include <klineedit.h>
#include <QComboBox>
#include <QPointer>
#include <QString>

#include <QStringList>
//Added by qt3to4:
#include <QLabel>
#include <QList>
#include <QShowEvent>
#include <QResizeEvent>
#include <QStackedWidget>

#include <kdialog.h>
#include <k3listview.h>
#include <kcmodule.h>
#include <klocale.h>
#include <kdemacros.h>
#include "ui_composercryptoconfiguration.h"
#include "ui_warningconfiguration.h"
#include "ui_smimeconfiguration.h"
#include "ui_customtemplates_base.h"

class QPushButton;
class QLabel;
class QCheckBox;
class KUrlRequester;
class KFontChooser;
class QRadioButton;
class ColorListBox;
class QFont;
class Q3ListViewItem;
class QTabWidget;
class Q3ListBox;
class Q3ButtonGroup;
class QRegExpValidator;
class KVBox;
class KMAccount;
class KMTransportInfo;
class ListView;
class ConfigureDialog;
class KIntSpinBox;
class SimpleStringListEditor;
class KConfig;
class QPoint;
class SMimeConfiguration;
class TemplatesConfiguration;
class CustomTemplates;

namespace Kpgp {
  class Config;
}
namespace KMail {
  class IdentityDialog;
  class IdentityListView;
  class AccountComboBox;
  class FolderRequester;
}
namespace Kleo {
  class BackendConfigWidget;
  class CryptoConfig;
  class CryptoConfigEntry;
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

  NewIdentityDialog( const QStringList & identities,
                     QWidget *parent=0 );

  QString identityName() const { return mLineEdit->text(); }
  QString duplicateIdentity() const { return mComboBox->currentText(); }
  DuplicateMode duplicateMode() const;

protected slots:
  virtual void slotEnableOK( const QString & );

private:
  QLineEdit  *mLineEdit;
  QComboBox  *mComboBox;
  Q3ButtonGroup *mButtonGroup;
};


//
//
// Language item handling
//
//

struct LanguageItem
{
  LanguageItem() {}
  LanguageItem( const QString & language, const QString & reply=QString(),
                const QString & replyAll=QString(),
                const QString & forward=QString(),
                const QString & indentPrefix=QString() ) :
    mLanguage( language ), mReply( reply ), mReplyAll( replyAll ),
    mForward( forward ), mIndentPrefix( indentPrefix ) {}

  QString mLanguage, mReply, mReplyAll, mForward, mIndentPrefix;
};

typedef QList<LanguageItem> LanguageItemList;

class NewLanguageDialog : public KDialog
{
  Q_OBJECT

  public:
    NewLanguageDialog( LanguageItemList & suppressedLangs, QWidget *parent=0 );
    QString language() const;

  private:
    QComboBox *mComboBox;
};


class LanguageComboBox : public QComboBox
{
  Q_OBJECT

  public:
    LanguageComboBox( QWidget *parent=0 );
    int insertLanguage( const QString & language );
    QString language() const;
    void setLanguage( const QString & language );
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
  K3ListView   *mListView;
  QStringList mProfileList;
};

#include <kdialog.h>
#include <kvbox.h>
class ConfigModule : public KCModule {
  Q_OBJECT
public:
  ConfigModule( const KComponentData &instance, QWidget *parent=0, const QStringList &args=QStringList() )
     : KCModule ( instance, parent, args )
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
  virtual void installProfile(){}
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
class ConfigModuleWithTabs : public ConfigModule {
  Q_OBJECT
public:
  ConfigModuleWithTabs( const KComponentData &instance, QWidget *parent=0, const QStringList &args=QStringList() );
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

class KDE_EXPORT IdentityPage : public ConfigModule {
  Q_OBJECT
public:
  IdentityPage( const KComponentData &instance, QWidget *parent=0, const QStringList &args=QStringList() );
  ~IdentityPage() {}

  QString helpAnchor() const;

  void load();
  void save();

public slots:
  void slotUpdateTransportCombo( const QStringList & );

private slots:
  void slotNewIdentity();
  void slotModifyIdentity();
  void slotRemoveIdentity();
  /** Connected to @p mRenameButton's clicked() signal. Just does a
      K3ListView::rename on the selected item */
  void slotRenameIdentity();
  /** connected to @p mIdentityList's renamed() signal. Validates the
      new name and sets it in the KPIM::IdentityManager */
  void slotRenameIdentity( Q3ListViewItem *, const QString &, int );
  void slotContextMenu( K3ListView*, Q3ListViewItem *, const QPoint & );
  void slotSetAsDefault();
  void slotIdentitySelectionChanged();

private: // methods
  void refreshList();

private: // data members
  KMail::IdentityDialog   * mIdentityDialog;
  int            mOldNumberOfIdentities;

  KMail::IdentityListView * mIdentityList;
  QPushButton             * mModifyButton;
  QPushButton             * mRenameButton;
  QPushButton             * mRemoveButton;
  QPushButton             * mSetAsDefaultButton;
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

signals:
  void transportListChanged( const QStringList & );

private slots:
  void slotTransportSelected();
  void slotAddTransport();
  void slotModifySelectedTransport();
  void slotRemoveSelectedTransport();
  void slotSetDefaultTransport();

private:
  virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  ListView    *mTransportList;
  QPushButton *mModifyTransportButton;
  QPushButton *mRemoveTransportButton;
  QPushButton *mSetDefaultTransportButton;
  QCheckBox   *mConfirmSendCheck;
  QComboBox   *mSendOnCheckCombo;
  QComboBox   *mSendMethodCombo;
  QComboBox   *mMessagePropertyCombo;
  QLineEdit   *mDefaultDomainEdit;

  QList< KMTransportInfo* > mTransportInfoList;
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
  void slotTweakAccountList();

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

class KDE_EXPORT AccountsPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  AccountsPage( const KComponentData &instance, QWidget *parent=0, const QStringList &args=QStringList() );
  QString helpAnchor() const;


  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef AccountsPageSendingTab SendingTab;
  typedef AccountsPageReceivingTab ReceivingTab;

signals:
  void transportListChanged( const QStringList & );
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
  ColorListBox *mColorList;
  QCheckBox    *mRecycleColorCheck;
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
  QCheckBox    *mMessageSizeCheck;
  QCheckBox    *mAttachmentCheck;
  QCheckBox    *mNestedMessagesCheck;
  QCheckBox    *mCryptoIconsCheck;
  Q3ButtonGroup *mNestingPolicy;
  Q3ButtonGroup *mDateDisplay;
  QLineEdit    *mCustomDateFormatEdit;
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
  Q3ButtonGroup *mSystemTrayGroup;
};

class KDE_EXPORT AppearancePage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  AppearancePage( const KComponentData &instance, QWidget *parent=0, const QStringList &args=QStringList() );

  QString helpAnchor() const;

  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef AppearancePageFontsTab FontsTab;
  typedef AppearancePageColorsTab ColorsTab;
  typedef AppearancePageLayoutTab LayoutTab;
  typedef AppearancePageHeadersTab HeadersTab;
  typedef AppearancePageReaderTab ReaderTab;
  typedef AppearancePageSystemTrayTab SystemTrayTab;

private:
  FontsTab      *mFontsTab;
  ColorsTab     *mColorsTab;
  LayoutTab     *mLayoutTab;
  HeadersTab    *mHeadersTab;
  ReaderTab     *mReaderTab;
  SystemTrayTab *mSystemTrayTab;
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
  QCheckBox     *mSmartQuoteCheck;
  QCheckBox     *mAutoRequestMDNCheck;
  QCheckBox	*mShowRecentAddressesInComposer;
  QCheckBox     *mWordWrapCheck;
  KIntSpinBox   *mWrapColumnSpin;
  KIntSpinBox   *mAutoSave;
  QCheckBox     *mExternalEditorCheck;
  KUrlRequester *mEditorRequester;
};

class ComposerPagePhrasesTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPagePhrasesTab( QWidget * parent=0 );
  QString helpAnchor() const;

  void save();

private slots:
  void slotNewLanguage();
  void slotRemoveLanguage();
  void slotLanguageChanged( const QString& );
  void slotAddNewLanguage( const QString& );

private:
  virtual void doLoadFromGlobalSettings();
  void setLanguageItemInformation( int index );
  void saveActiveLanguageItem();

private:
  LanguageComboBox *mPhraseLanguageCombo;
  QPushButton      *mRemoveButton;
  QLineEdit        *mPhraseReplyEdit;
  QLineEdit        *mPhraseReplyAllEdit;
  QLineEdit        *mPhraseForwardEdit;
  QLineEdit        *mPhraseIndentPrefixEdit;

  int              mActiveLanguageItem;
  LanguageItemList mLanguageList;
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
  Q3ListView   *mTagList;
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

class KDE_EXPORT ComposerPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  ComposerPage( const KComponentData &instance, QWidget *parent=0, const QStringList &args=QStringList() );

  QString helpAnchor() const;

  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef ComposerPageGeneralTab GeneralTab;
  typedef ComposerPagePhrasesTab PhrasesTab;
  typedef ComposerPageTemplatesTab TemplatesTab;
  typedef ComposerPageCustomTemplatesTab CustomTemplatesTab;
  typedef ComposerPageSubjectTab SubjectTab;
  typedef ComposerPageCharsetTab CharsetTab;
  typedef ComposerPageHeadersTab HeadersTab;
  typedef ComposerPageAttachmentsTab AttachmentsTab;

private:
  GeneralTab  *mGeneralTab;
  PhrasesTab  *mPhrasesTab;
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
  Q3ButtonGroup *mMDNGroup;
  Q3ButtonGroup *mOrigQuoteGroup;
  QCheckBox    *mAutomaticallyImportAttachedKeysCheck;
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

#ifdef __GNUC__
#warning This was a DCOPObject, so we probably need to port it to DBus!
#endif
class SecurityPageSMimeTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageSMimeTab( QWidget * parent=0 );
  ~SecurityPageSMimeTab();

  QString helpAnchor() const;

  // Can't use k_dcop here. dcopidl can't parse this file, dcopidlng has a namespace bug.
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

class KDE_EXPORT SecurityPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  SecurityPage( const KComponentData &instance, QWidget *parent=0, const QStringList &args=QStringList() );

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
#ifdef HAVE_INDEXLIB
  QCheckBox    *mIndexingEnabled;
#endif
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
  QCheckBox* mAutoResCB;
  QCheckBox* mAutoDeclConflCB;
  QCheckBox* mAutoDeclRecurCB;

  QCheckBox* mLegacyMangleFromTo;
  QCheckBox* mLegacyBodyInvites;
  QCheckBox* mAutomaticSending;
};

class KDE_EXPORT MiscPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  MiscPage( const KComponentData &instance, QWidget *parent=0, const QStringList &args=QStringList() );
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

class ListView : public K3ListView {
  Q_OBJECT
public:
  ListView( QWidget *parent=0, int visibleItem=10 );
  void resizeColums();

  void setVisibleItem( int visibleItem, bool updateSize=true );
  virtual QSize sizeHint() const;

protected:
  virtual void resizeEvent( QResizeEvent *e );
  virtual void showEvent( QShowEvent *e );

private:
  int mVisibleItem;
};


#endif // _CONFIGURE_DIALOG_PRIVATE_H_
