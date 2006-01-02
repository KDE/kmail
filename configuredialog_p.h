// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include <klineedit.h>
#include <qcombobox.h>
#include <qpointer.h>
#include <q3ptrlist.h>
#include <qstring.h>

#include <qstringlist.h>
//Added by qt3to4:
#include <QLabel>
#include <QShowEvent>
#include <QResizeEvent>
#include <QStackedWidget>
#include <dcopobject.h>

#include <kdialogbase.h>
#include <klistview.h>
#include <kcmodule.h>
#include <klocale.h>
#include <kdepimmacros.h>

class QPushButton;
class QLabel;
class QCheckBox;
class KURLRequester;
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
class ComposerCryptoConfiguration;
class WarningConfiguration;
class SMimeConfiguration;

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

class NewIdentityDialog : public KDialogBase
{
  Q_OBJECT

public:
  enum DuplicateMode { Empty, ControlCenter, ExistingEntry };

  NewIdentityDialog( const QStringList & identities,
                     QWidget *parent=0, const char *name=0, bool modal=true );

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

class NewLanguageDialog : public KDialogBase
{
  Q_OBJECT

  public:
    NewLanguageDialog( LanguageItemList & suppressedLangs, QWidget *parent=0,
                       const char *name=0, bool modal=true );
    QString language() const;

  private:
    QComboBox *mComboBox;
};


class LanguageComboBox : public QComboBox
{
  Q_OBJECT

  public:
    LanguageComboBox( bool rw, QWidget *parent=0 );
    int insertLanguage( const QString & language );
    QString language() const;
    void setLanguage( const QString & language );
};

//
//
// Profile dialog
//
//

class ProfileDialog : public KDialogBase {
  Q_OBJECT
public:
  ProfileDialog( QWidget * parent=0, const char * name=0, bool modal=false );

signals:
  void profileSelected( KConfig * profile );

private slots:
  void slotSelectionChanged();
  void slotOk();

private:
  void setup();

private:
  KListView   *mListView;
  QStringList mProfileList;
};

#include <kdialog.h>
#include <kvbox.h>
class ConfigModule : public KCModule {
  Q_OBJECT
public:
  ConfigModule( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() )
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
  ConfigModuleWithTabs( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() );
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
  IdentityPage( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() );
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
      KListView::rename on the selected item */
  void slotRenameIdentity();
  /** connected to @p mIdentityList's renamed() signal. Validates the
      new name and sets it in the KPIM::IdentityManager */
  void slotRenameIdentity( Q3ListViewItem *, const QString &, int );
  void slotContextMenu( KListView*, Q3ListViewItem *, const QPoint & );
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

  Q3PtrList< KMTransportInfo > mTransportInfoList;
};


class AccountsPageReceivingTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AccountsPageReceivingTab( QWidget * parent=0 );
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
  AccountsPage( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() );
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
  Q3ButtonGroup *mFolderListGroup;
  Q3ButtonGroup *mMIMETreeLocationGroup;
  Q3ButtonGroup *mMIMETreeModeGroup;
  Q3ButtonGroup *mReaderWindowModeGroup;
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
  AppearancePage( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() );

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
  KURLRequester *mEditorRequester;
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

class ComposerPageSubjectTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageSubjectTab( QWidget * parent=0 );
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
  //virtual void doLoadFromGlobalSettings();
  virtual void doLoadOther();
  //FIXME virtual void doResetToDefaultsOther();

private:
  QCheckBox   *mOutlookCompatibleCheck;
  QCheckBox   *mMissingAttachmentDetectionCheck;
  SimpleStringListEditor *mAttachWordsListEditor;
};

class KDE_EXPORT ComposerPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  ComposerPage( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() );

  QString helpAnchor() const;

  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef ComposerPageGeneralTab GeneralTab;
  typedef ComposerPagePhrasesTab PhrasesTab;
  typedef ComposerPageSubjectTab SubjectTab;
  typedef ComposerPageCharsetTab CharsetTab;
  typedef ComposerPageHeadersTab HeadersTab;
  typedef ComposerPageAttachmentsTab AttachmentsTab;

private:
  GeneralTab  *mGeneralTab;
  PhrasesTab  *mPhrasesTab;
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

class SecurityPageSMimeTab : public ConfigModuleTab, public DCOPObject {
  Q_OBJECT
  K_DCOP
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
  SecurityPage( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() );

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
  MiscPage( KInstance *instance, QWidget *parent=0, const QStringList &args=QStringList() );
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

class ListView : public KListView {
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
