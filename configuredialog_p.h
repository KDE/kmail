// -*- c++ -*-
// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#include <klineedit.h>
#include <qcombobox.h>
#include <qguardedptr.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>

#include <kdialogbase.h>
#include <klistview.h>
#include <kcmodule.h>
#include <klocale.h>

class QPushButton;
class QLabel;
class QCheckBox;
class KURLRequester;
class KFontChooser;
class QRadioButton;
class ColorListBox;
class QFont;
class QListViewItem;
class QTabWidget;
class QListBox;
class QButtonGroup;
class QRegExpValidator;
class QVBox;
class KMFolderComboBox;
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
  QButtonGroup *mButtonGroup;
};


//
//
// Language item handling
//
//

struct LanguageItem
{
  LanguageItem() {}
  LanguageItem( const QString & language, const QString & reply=QString::null,
		const QString & replyAll=QString::null,
		const QString & forward=QString::null,
		const QString & indentPrefix=QString::null ) :
    mLanguage( language ), mReply( reply ), mReplyAll( replyAll ),
    mForward( forward ), mIndentPrefix( indentPrefix ) {}

  QString mLanguage, mReply, mReplyAll, mForward, mIndentPrefix;
};

typedef QValueList<LanguageItem> LanguageItemList;

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
    LanguageComboBox( bool rw, QWidget *parent=0, const char *name=0 );
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
class ConfigModule : public KCModule {
  Q_OBJECT
public:
  ConfigModule( QWidget * parent=0, const char * name=0 )
     : KCModule ( parent, name )
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
   ConfigModuleTab( QWidget *parent=0, const char* name=0 )
      :QWidget( parent, name )
      {}
  ~ConfigModuleTab() {}
  virtual void load() = 0;
  virtual void save() = 0;
  // the below are optional
  virtual void defaults() {}
  virtual void installProfile(){}
signals:
   // forwarded to the ConfigModule
  void changed(bool);
public slots:
  void slotEmitChanged();
};


/*
 * ConfigModuleWithTabs represents a kcm with several tabs.
 * It simply forwards load and save operations to all tabs.
 */
class ConfigModuleWithTabs : public ConfigModule {
  Q_OBJECT
public:
  ConfigModuleWithTabs( QWidget * parent=0, const char * name=0 );
   ~ConfigModuleWithTabs() {}

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

class IdentityPage : public ConfigModule {
  Q_OBJECT
public:
  IdentityPage( QWidget * parent=0, const char * name=0 );
  ~IdentityPage() {}

  QString helpAnchor() const;

  void load();
  void save();

public slots:
  void slotUpdateTransportCombo( const QStringList & );

protected slots:
  void slotNewIdentity();
  void slotModifyIdentity();
  void slotRemoveIdentity();
  /** Connected to @p mRenameButton's clicked() signal. Just does a
      KListView::rename on the selected item */
  void slotRenameIdentity();
  /** connected to @p mIdentityList's renamed() signal. Validates the
      new name and sets it in the @ref KPIM::IdentityManager */
  void slotRenameIdentity( QListViewItem *, const QString &, int );
  void slotContextMenu( KListView*, QListViewItem *, const QPoint & );
  void slotSetAsDefault();
  void slotIdentitySelectionChanged();

protected: // methods
  void refreshList();

protected: // data members
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
// NetworkPage
//
//

// subclasses: one class per tab:
class NetworkPageSendingTab : public ConfigModuleTab {
  Q_OBJECT
public:
  NetworkPageSendingTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;
  void load();
  void save();
  void defaults() {}

signals:
  void transportListChanged( const QStringList & );

protected slots:
  void slotTransportSelected();
  void slotAddTransport();
  void slotModifySelectedTransport();
  void slotRemoveSelectedTransport();
  void slotTransportUp();
  void slotTransportDown();

protected:
  ListView    *mTransportList;
  QPushButton *mModifyTransportButton;
  QPushButton *mRemoveTransportButton;
  QPushButton *mTransportUpButton;
  QPushButton *mTransportDownButton;
  QCheckBox   *mConfirmSendCheck;
  QComboBox   *mSendOnCheckCombo;
  QComboBox   *mSendMethodCombo;
  QComboBox   *mMessagePropertyCombo;
  QLineEdit   *mDefaultDomainEdit;

  QPtrList< KMTransportInfo > mTransportInfoList;
};


class NetworkPageReceivingTab : public ConfigModuleTab {
  Q_OBJECT
public:
  NetworkPageReceivingTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;
  void load();
  void save();
  void defaults() {}

signals:
  void accountListChanged( const QStringList & );

protected slots:
  void slotAccountSelected();
  void slotAddAccount();
  void slotModifySelectedAccount();
  void slotRemoveSelectedAccount();
  void slotEditNotifications();

protected:
  QStringList occupiedNames();

protected:
  ListView      *mAccountList;
  QPushButton   *mModifyAccountButton;
  QPushButton   *mRemoveAccountButton;
  QCheckBox     *mBeepNewMailCheck;
  QCheckBox     *mVerboseNotificationCheck;
  QCheckBox     *mCheckmailStartupCheck;
  QPushButton   *mOtherNewMailActionsButton;

  QValueList< QGuardedPtr<KMAccount> > mAccountsToDelete;
  QValueList< QGuardedPtr<KMAccount> > mNewAccounts;
  struct ModifiedAccountsType {
    QGuardedPtr< KMAccount > oldAccount;
    QGuardedPtr< KMAccount > newAccount;
  };
  // ### make this a qptrlist:
  QValueList< ModifiedAccountsType* >  mModifiedAccounts;
};

class NetworkPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  NetworkPage( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;


  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef NetworkPageSendingTab SendingTab;
  typedef NetworkPageReceivingTab ReceivingTab;

signals:
  void transportListChanged( const QStringList & );
  void accountListChanged( const QStringList & );

protected:
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
  AppearancePageFontsTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;
  void load();
  void save();
  void defaults() {}

  void installProfile( KConfig * profile );

protected slots:
  void slotFontSelectorChanged( int );

protected:
  void updateFontSelector();

protected:
  QCheckBox    *mCustomFontCheck;
  QComboBox    *mFontLocationCombo;
  KFontChooser *mFontChooser;

  int          mActiveFontIndex;
  QFont        mFont[10];
};

class AppearancePageColorsTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageColorsTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;
  void load();
  void save();
  void defaults() {}

  void installProfile( KConfig * profile );

protected:
  QCheckBox    *mCustomColorCheck;
  ColorListBox *mColorList;
  QCheckBox    *mRecycleColorCheck;
};

class AppearancePageLayoutTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageLayoutTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}
  void installProfile( KConfig * profile );

protected: // data
  QCheckBox    *mShowColorbarCheck;
  QButtonGroup *mFolderListGroup;
  QButtonGroup *mMIMETreeLocationGroup;
  QButtonGroup *mMIMETreeModeGroup;
  QButtonGroup *mReaderWindowModeGroup;
};

class AppearancePageHeadersTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageHeadersTab( QWidget * parent=0, const char * name=0 );

  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}
  void installProfile( KConfig * profile );

protected: // methods
  void setDateDisplay( int id, const QString & format );

protected: // data
  QCheckBox    *mMessageSizeCheck;
  QCheckBox    *mAttachmentCheck;
  QCheckBox    *mNestedMessagesCheck;
  QCheckBox    *mCryptoIconsCheck;
  QButtonGroup *mNestingPolicy;
  QButtonGroup *mDateDisplay;
  QLineEdit    *mCustomDateFormatEdit;
};

class AppearancePageSystemTrayTab : public ConfigModuleTab {
  Q_OBJECT
public:
  AppearancePageSystemTrayTab( QWidget * parent=0, const char * name=0 );

  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}
  void installProfile( KConfig * profile );

private: // data
  QCheckBox    *mSystemTrayCheck;
  QButtonGroup *mSystemTrayGroup;
};

class AppearancePage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  AppearancePage( QWidget * parent=0, const char * name=0 );

  QString helpAnchor() const;

  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef AppearancePageFontsTab FontsTab;
  typedef AppearancePageColorsTab ColorsTab;
  typedef AppearancePageLayoutTab LayoutTab;
  typedef AppearancePageHeadersTab HeadersTab;
  typedef AppearancePageSystemTrayTab SystemTrayTab;

protected:
  FontsTab      *mFontsTab;
  ColorsTab     *mColorsTab;
  LayoutTab     *mLayoutTab;
  HeadersTab    *mHeadersTab;
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
  ComposerPageGeneralTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;

  void load();
  void save();
  void defaults();
  void installProfile( KConfig * profile );

protected:
  QCheckBox     *mAutoAppSignFileCheck;
  QCheckBox     *mSmartQuoteCheck;
  QCheckBox     *mAutoRequestMDNCheck;
  QCheckBox     *mWordWrapCheck;
  KIntSpinBox   *mWrapColumnSpin;
  KIntSpinBox   *mAutoSave;
  QCheckBox     *mExternalEditorCheck;
  KURLRequester *mEditorRequester;
};

class ComposerPagePhrasesTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPagePhrasesTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}

protected slots:
  void slotNewLanguage();
  void slotRemoveLanguage();
  void slotLanguageChanged( const QString& );
  void slotAddNewLanguage( const QString& );

protected:
  void setLanguageItemInformation( int index );
  void saveActiveLanguageItem();

protected:
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
  ComposerPageSubjectTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}


protected:
  SimpleStringListEditor *mReplyListEditor;
  QCheckBox              *mReplaceReplyPrefixCheck;
  SimpleStringListEditor *mForwardListEditor;
  QCheckBox              *mReplaceForwardPrefixCheck;
};

class ComposerPageCharsetTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageCharsetTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}

protected slots:
  void slotVerifyCharset(QString&);

protected:
  SimpleStringListEditor *mCharsetListEditor;
  QCheckBox              *mKeepReplyCharsetCheck;
};

class ComposerPageHeadersTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageHeadersTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}

protected slots:
  void slotMimeHeaderSelectionChanged();
  void slotMimeHeaderNameChanged( const QString & );
  void slotMimeHeaderValueChanged( const QString & );
  void slotNewMimeHeader();
  void slotRemoveMimeHeader();

protected:
  QCheckBox   *mCreateOwnMessageIdCheck;
  QLineEdit   *mMessageIdSuffixEdit;
  QRegExpValidator *mMessageIdSuffixValidator;
  QListView   *mTagList;
  QPushButton *mRemoveHeaderButton;
  QLineEdit   *mTagNameEdit;
  QLineEdit   *mTagValueEdit;
  QLabel      *mTagNameLabel;
  QLabel      *mTagValueLabel;
};

class ComposerPageAttachmentsTab : public ConfigModuleTab {
  Q_OBJECT
public:
  ComposerPageAttachmentsTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}

protected slots:
  void slotOutlookCompatibleClicked();

protected:
  QCheckBox   *mOutlookCompatibleCheck;
  QCheckBox   *mMissingAttachmentDetectionCheck;
  SimpleStringListEditor *mAttachWordsListEditor;
};

class ComposerPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  ComposerPage( QWidget * parent=0, const char * name=0 );

  QString helpAnchor() const;

  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef ComposerPageGeneralTab GeneralTab;
  typedef ComposerPagePhrasesTab PhrasesTab;
  typedef ComposerPageSubjectTab SubjectTab;
  typedef ComposerPageCharsetTab CharsetTab;
  typedef ComposerPageHeadersTab HeadersTab;
  typedef ComposerPageAttachmentsTab AttachmentsTab;

protected:
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
  SecurityPageGeneralTab( QWidget * parent=0, const char * name=0 );
  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}

  void installProfile( KConfig * profile );

protected:
  QCheckBox    *mExternalReferences;
  QCheckBox    *mHtmlMailCheck;
  QCheckBox    *mNoMDNsWhenEncryptedCheck;
  QButtonGroup *mMDNGroup;
  QButtonGroup *mOrigQuoteGroup;
  QCheckBox    *mAutomaticallyImportAttachedKeysCheck;
};


class SecurityPageComposerCryptoTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageComposerCryptoTab( QWidget * parent=0, const char * name=0 );

  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}
  void installProfile( KConfig * profile );

protected:
  ComposerCryptoConfiguration* mWidget;
};

class SecurityPageWarningTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageWarningTab( QWidget * parent=0, const char * name=0 );

  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}
  void installProfile( KConfig * profile );

private slots:
  void slotReenableAllWarningsClicked();

protected:
  WarningConfiguration* mWidget;
};

class SecurityPageSMimeTab : public ConfigModuleTab {
  Q_OBJECT
public:
  SecurityPageSMimeTab( QWidget * parent=0, const char * name=0 );

  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}
  void installProfile( KConfig * profile );

protected:
  Kleo::CryptoConfigEntry* configEntry( const char* componentName,
                                        const char* groupName,
                                        const char* entryName,
                                        int argType,
                                        bool isList );

private:
  SMimeConfiguration* mWidget;
  // Checkboxes
  Kleo::CryptoConfigEntry* mCheckUsingOCSPConfigEntry;
  Kleo::CryptoConfigEntry* mEnableOCSPsendingConfigEntry;
  Kleo::CryptoConfigEntry* mDoNotCheckCertPolicyConfigEntry;
  Kleo::CryptoConfigEntry* mNeverConsultConfigEntry;
  Kleo::CryptoConfigEntry* mFetchMissingConfigEntry;
  // Other widgets
  Kleo::CryptoConfigEntry* mOCSPResponderURLConfigEntry;
  Kleo::CryptoConfigEntry* mOCSPResponderSignature;

  Kleo::CryptoConfig* mConfig;
};

class SecurityPageCryptPlugTab : public ConfigModuleTab
{
  Q_OBJECT
public:
  SecurityPageCryptPlugTab( QWidget * parent = 0, const char* name = 0 );
  ~SecurityPageCryptPlugTab();

  QString helpAnchor() const;

  void load();
  void save();
  void defaults() {}

private:
  Kleo::BackendConfigWidget * mBackendConfig;
};

class SecurityPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  SecurityPage(	QWidget * parent=0, const char * name=0 );

  QString helpAnchor() const;

  // OpenPGP tab is special:
  void installProfile( KConfig * profile );

  typedef SecurityPageGeneralTab GeneralTab;
  typedef SecurityPageComposerCryptoTab ComposerCryptoTab;
  typedef SecurityPageWarningTab WarningTab;
  typedef SecurityPageSMimeTab SMimeTab;
  typedef SecurityPageCryptPlugTab CryptPlugTab;

protected:
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
  MiscPageFolderTab( QWidget * parent=0, const char * name=0 );

  void load();
  void save();
  void defaults() {}
 QString helpAnchor() const;

protected:
  QCheckBox    *mEmptyFolderConfirmCheck;
  QCheckBox    *mExcludeImportantFromExpiry;
  QComboBox    *mLoopOnGotoUnread;
  QCheckBox    *mJumpToUnread;
  QComboBox    *mMailboxPrefCombo;
  QCheckBox    *mEmptyTrashCheck;
  QCheckBox    *mDelayedMarkAsRead;
  KIntSpinBox  *mDelayedMarkTime;
  QCheckBox    *mShowPopupAfterDnD;
  KMFolderComboBox *mOnStartupOpenFolder;
};

class MiscPageGroupwareTab : public ConfigModuleTab  {
  Q_OBJECT
public:
  MiscPageGroupwareTab( QWidget * parent=0, const char * name=0 );
  void load();
  void save();
  void defaults() {}
  QString helpAnchor() const;
private slots:
  void slotStorageFormatChanged( int );
  void slotLegacyBodyInvitesToggled( bool on );
private:
  QCheckBox* mEnableGwCB;
  QCheckBox* mEnableImapResCB;

  QWidget* mBox;
  QVBox* gBox;

  QComboBox* mStorageFormatCombo;
  QComboBox* mLanguageCombo;

  QLabel* mFolderComboLabel;
  QWidgetStack* mFolderComboStack;
  KMFolderComboBox* mFolderCombo; // in the widgetstack
  KMail::AccountComboBox* mAccountCombo; // in the widgetstack

  QCheckBox* mHideGroupwareFolders;
  QCheckBox* mAutoResCB;
  QCheckBox* mAutoDeclConflCB;
  QCheckBox* mAutoDeclRecurCB;

  QCheckBox* mLegacyMangleFromTo;
  QCheckBox* mLegacyBodyInvites;
  QCheckBox* mAutomaticSending;
};

class MiscPage : public ConfigModuleWithTabs {
  Q_OBJECT
public:
  MiscPage( QWidget * parent=0, const char * name=0 );
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
  ListView( QWidget *parent=0, const char *name=0, int visibleItem=10 );
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
