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
class QVGroupBox;
class KMFolderComboBox;
class KMAccount;
class KMTransportInfo;
class ListView;
class ConfigureDialog;
class KIntSpinBox;
class SimpleStringListEditor;
class KConfig;
class QPoint;
class CryptPlugWrapperList;
namespace Kpgp {
  class Config;
}
namespace KMail {
  class IdentityDialog;
  class IdentityListView;
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
  LanguageItem() {};
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
// basic ConfigurationPage (inherit pages from this)
//
//

class ConfigurationPage : public QWidget {
  Q_OBJECT
public:
  ConfigurationPage( QWidget * parent=0, const char * name=0 )
    : QWidget( parent, name ) {}
  ~ConfigurationPage() {};

  /** Should return the help anchor for this page or tab */
  virtual QString helpAnchor() const = 0;

  /** Should set the page up (ie. read the setting from the @ref
      KConfig object into the widgets) after creating it in the
      constructor. Called from @ref ConfigureDialog. */
  virtual void setup() = 0;
  /** Called when the installation of a profile is
      requested. Reimplemenations of this method should do the
      equivalent of a @ref setup(), but with the given @ref KConfig
      object instead of KMKernel::config() and only for those entries that
      really have keys defined in the profile.

      The default implementation does nothing.
  */
  virtual void installProfile( KConfig * /*profile*/ ) {};
  /** Should apply the changed settings (ie. read the settings from
      the widgets into the @ref KConfig object). Called from @ref
      ConfigureDialog. */
  virtual void apply() = 0;
  /** Should cleanup any temporaries after cancel. The default
      implementation does nothing. Called from @ref ConfigureDialog. */
  virtual void dismiss() {}

  void setPageIndex( int aPageIndex ) { mPageIndex = aPageIndex; }
  int pageIndex() const { return mPageIndex; }
protected:
  int mPageIndex;
};

//
//
// TabbedConfigurationPage
//
//

class TabbedConfigurationPage : public ConfigurationPage {
  Q_OBJECT
public:
  TabbedConfigurationPage( QWidget * parent=0, const char * name=0 );

  void setup();
  void dismiss();
  void installProfile( KConfig * profile );
  void apply();

protected:
  void addTab( QWidget * tab, const QString & title );

private:
  ConfigurationPage * configTab( int index, const char * debugMsg ) const;

  QTabWidget *mTabWidget;

};

//
//
// IdentityPage
//
//

class IdentityPage : public ConfigurationPage {
  Q_OBJECT
public:
  IdentityPage( QWidget * parent=0, const char * name=0 );
  ~IdentityPage() {};

  static QString iconLabel();
  static QString title();
  static const char * iconName();
  QString helpAnchor() const;

  void setup();
  void apply();
  void dismiss();

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
      new name and sets it in the @ref IdentityManager */
  void slotRenameIdentity( QListViewItem *, const QString &, int );
  void slotContextMenu( KListView*, QListViewItem *, const QPoint & );
  void slotSetAsDefault();
  void slotIdentitySelectionChanged( QListViewItem * );

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
class NetworkPageSendingTab : public ConfigurationPage {
  Q_OBJECT
public:
  NetworkPageSendingTab( QWidget * parent=0, const char * name=0 );

  // no icon:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();

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
  QCheckBox   *mSendOutboxCheck;
  QComboBox   *mSendMethodCombo;
  QComboBox   *mMessagePropertyCombo;
  QLineEdit   *mDefaultDomainEdit;

  QPtrList< KMTransportInfo > mTransportInfoList;

};


class NetworkPageReceivingTab : public ConfigurationPage {
  Q_OBJECT
public:
  NetworkPageReceivingTab( QWidget * parent=0, const char * name=0 );

  // no icon:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
  void dismiss(); // needed for account list cleanup

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
  QCheckBox     *mSystrayCheck;
  QRadioButton  *mBlinkingSystray;
  QRadioButton  *mSystrayOnNew;
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

class NetworkPage : public TabbedConfigurationPage {
  Q_OBJECT
public:
  NetworkPage( QWidget * parent=0, const char * name=0 );

  static QString iconLabel();
  static QString title();
  static const char * iconName();
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

class AppearancePageFontsTab : public ConfigurationPage {
  Q_OBJECT
public:
  AppearancePageFontsTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
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

class AppearancePageColorsTab : public ConfigurationPage {
  Q_OBJECT
public:
  AppearancePageColorsTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
  void installProfile( KConfig * profile );

protected:
  QCheckBox    *mCustomColorCheck;
  ColorListBox *mColorList;
  QCheckBox    *mRecycleColorCheck;
};

class AppearancePageLayoutTab : public ConfigurationPage {
  Q_OBJECT
public:
  AppearancePageLayoutTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
  void installProfile( KConfig * profile );


protected slots:
  /** Used to switch the Layout preview items if Mime Tree Viewer is enabled
      or disabled */
  void showMIMETreeClicked( int id );

protected: // methods
  QPixmap pixmapFor( int num, int type );

protected: // data
  QCheckBox    *mShowColorbarCheck;
  QButtonGroup *mShowMIMETreeMode;
  QButtonGroup *mWindowLayoutBG;
  int mShowMIMETreeModeLastValue;
};

class AppearancePageHeadersTab : public ConfigurationPage {
  Q_OBJECT
public:
  AppearancePageHeadersTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
  void installProfile( KConfig * profile );

protected: // methods
  void setDateDisplay( int id, const QString & format );

protected: // data
  QCheckBox    *mMessageSizeCheck;
  QCheckBox    *mNestedMessagesCheck;
  QCheckBox    *mCryptoIconsCheck;
  QButtonGroup *mNestingPolicy;
  QButtonGroup *mDateDisplay;
  QLineEdit    *mCustomDateFormatEdit;
};

class AppearancePageProfileTab : public ConfigurationPage {
  Q_OBJECT
public:
  AppearancePageProfileTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();

signals:
  void profileSelected( KConfig * profile );

protected:
  KListView   *mListView;
  QStringList mProfileList;
};

class AppearancePage : public TabbedConfigurationPage {
  Q_OBJECT
public:
  AppearancePage( QWidget * parent=0, const char * name=0 );

  static QString iconLabel();
  static QString title();
  static const char * iconName();
  QString helpAnchor() const;

  void apply(); // is special

  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef AppearancePageFontsTab FontsTab;
  typedef AppearancePageColorsTab ColorsTab;
  typedef AppearancePageLayoutTab LayoutTab;
  typedef AppearancePageHeadersTab HeadersTab;
  typedef AppearancePageProfileTab ProfileTab;

signals:
  /** Used to route @p mProfileTab's signal on to the
      configuredialog's slotInstallProfile() */
  void profileSelected( KConfig * profile );

protected:
  FontsTab   *mFontsTab;
  ColorsTab  *mColorsTab;
  LayoutTab  *mLayoutTab;
  HeadersTab *mHeadersTab;
  ProfileTab *mProfileTab;
};

//
//
// Composer Page
//
//

class ComposerPageGeneralTab : public ConfigurationPage {
  Q_OBJECT
public:
  ComposerPageGeneralTab( QWidget * parent=0, const char * name=0 );

  // no icon
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
  void installProfile( KConfig * profile );

protected:
  QCheckBox     *mAutoAppSignFileCheck;
  QCheckBox     *mSmartQuoteCheck;
  QCheckBox     *mAutoRequestMDNCheck;
  QCheckBox     *mWordWrapCheck;
  KIntSpinBox   *mWrapColumnSpin;
  QCheckBox     *mExternalEditorCheck;
  KURLRequester *mEditorRequester;
};

class ComposerPagePhrasesTab : public ConfigurationPage {
  Q_OBJECT
public:
  ComposerPagePhrasesTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();

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

class ComposerPageSubjectTab : public ConfigurationPage {
  Q_OBJECT
public:
  ComposerPageSubjectTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();


protected:
  SimpleStringListEditor *mReplyListEditor;
  QCheckBox              *mReplaceReplyPrefixCheck;
  SimpleStringListEditor *mForwardListEditor;
  QCheckBox              *mReplaceForwardPrefixCheck;
};

class ComposerPageCharsetTab : public ConfigurationPage {
  Q_OBJECT
public:
  ComposerPageCharsetTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();

protected slots:
  void slotVerifyCharset(QString&);

protected:
  SimpleStringListEditor *mCharsetListEditor;
  QCheckBox              *mKeepReplyCharsetCheck;
};

class ComposerPageHeadersTab : public ConfigurationPage {
  Q_OBJECT
public:
  ComposerPageHeadersTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();

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

class ComposerPage : public TabbedConfigurationPage {
  Q_OBJECT
public:
  ComposerPage( QWidget * parent=0, const char * name=0 );

  static QString iconLabel();
  static QString title();
  static const char * iconName();
  QString helpAnchor() const;

  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef ComposerPageGeneralTab GeneralTab;
  typedef ComposerPagePhrasesTab PhrasesTab;
  typedef ComposerPageSubjectTab SubjectTab;
  typedef ComposerPageCharsetTab CharsetTab;
  typedef ComposerPageHeadersTab HeadersTab;

protected:
  GeneralTab  *mGeneralTab;
  PhrasesTab  *mPhrasesTab;
  SubjectTab  *mSubjectTab;
  CharsetTab  *mCharsetTab;
  HeadersTab  *mHeadersTab;
};

//
//
// SecurityPage
//
//

class SecurityPageGeneralTab : public ConfigurationPage {
  Q_OBJECT
public:
  SecurityPageGeneralTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
  void installProfile( KConfig * profile );

protected:
  QCheckBox    *mExternalReferences;
  QCheckBox    *mHtmlMailCheck;
  QCheckBox    *mSendReceivedReceiptCheck;
  QButtonGroup *mMDNGroup;
  QButtonGroup *mOrigQuoteGroup;
};


class SecurityPageOpenPgpTab : public ConfigurationPage {
  Q_OBJECT
public:
  SecurityPageOpenPgpTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
  void installProfile( KConfig * profile );

protected:
  Kpgp::Config *mPgpConfig;
  QCheckBox    *mPgpAutoSignatureCheck;
  QCheckBox    *mPgpAutoEncryptCheck;
};


class SecurityPageCryptPlugTab : public ConfigurationPage
{
  Q_OBJECT
public:
  SecurityPageCryptPlugTab( QWidget * parent = 0, const char* name = 0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();
  //void savePluginsConfig( bool silent );

public slots:
  void slotNewPlugIn();
  void slotDeletePlugIn();
  void slotActivatePlugIn();
  void slotConfigurePlugIn();
  void slotPlugNameChanged( const QString& );
  void slotPlugLocationChanged( const QString& );
  void slotPlugUpdateURLChanged( const QString& );

protected slots:
  void slotPlugSelectionChanged();

private:
  KListView     *mPlugList;

  QPushButton   *mNewButton;
  QPushButton   *mRemoveButton;
  QPushButton   *mActivateButton;
  QPushButton   *mConfigureButton;

  QLineEdit     *mNameEdit;
  KURLRequester *mLocationRequester;
  QLineEdit     *mUpdateURLEdit;
};

class SecurityPage : public TabbedConfigurationPage {
  Q_OBJECT
public:
  SecurityPage(	QWidget * parent=0, const char * name=0 );

  static QString iconLabel();
  static const char * iconName();
  static QString title();
  QString helpAnchor() const;

  // OpenPGP tab is special:
  void setup();
  void apply();
  void dismiss() {}
  void installProfile( KConfig * profile );

  typedef SecurityPageGeneralTab GeneralTab;
  typedef SecurityPageOpenPgpTab OpenPgpTab;
  typedef SecurityPageCryptPlugTab CryptPlugTab;

protected:
  GeneralTab   *mGeneralTab;
  OpenPgpTab   *mOpenPgpTab;
  CryptPlugTab *mCryptPlugTab;
};


//
//
// FolderPage
//
//

class FolderPage : public ConfigurationPage {
  Q_OBJECT
public:
  FolderPage( QWidget * parent=0, const char * name=0 );

  static QString iconLabel();
  static const char * iconName();
  static QString title();
  QString helpAnchor() const;

  void setup();
  void apply();

protected:
  QCheckBox    *mEmptyFolderConfirmCheck;
  QCheckBox    *mWarnBeforeExpire;
  QComboBox    *mLoopOnGotoUnread;
  QCheckBox    *mJumpToUnread;
  QComboBox    *mMailboxPrefCombo;
  QCheckBox    *mCompactOnExitCheck;
  QCheckBox    *mEmptyTrashCheck;
  QCheckBox    *mExpireAtExit;
  QCheckBox    *mDelayedMarkAsRead;
  KIntSpinBox  *mDelayedMarkTime;
  QCheckBox    *mShowPopupAfterDnD;
  KMFolderComboBox *mOnStartupOpenFolder;
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

//
//
// Groupware config page
//
//

class GroupwarePage: public ConfigurationPage  {
  Q_OBJECT
public:
  GroupwarePage( QWidget * parent=0, const char * name=0 );
  ~GroupwarePage() {};

  static QString iconLabel();
  static const char * iconName();

  static QString title();

  /** Should return the help anchor for this page or tab */
  virtual QString helpAnchor() const;

  /** Should set the page up (ie. read the setting from the @ref
      KConfig object into the widgets) after creating it in the
      constructor. Called from @ref ConfigureDialog. */
  virtual void setup();
  /** Called when the installation of a profile is
      requested. Reimplemenations of this method should do the
      equivalent of a @ref setup(), but with the given @ref KConfig
      object instead of KMKernel::config() and only for those entries that
      really have keys defined in the profile.

      The default implementation does nothing.
  */
  virtual void installProfile( KConfig * profile );
  /** Should apply the changed settings (ie. read the settings from
      the widgets into the @ref KConfig object). Called from @ref
      ConfigureDialog. */
  virtual void apply();
private:
  QCheckBox* mEnableGwCB;
  QCheckBox* mEnableImapResCB;

  QVGroupBox* mBox;

  QComboBox* mLanguageCombo;
  KMFolderComboBox* mFolderCombo;

  QCheckBox* mAutoResCB;
  QCheckBox* mAutoDeclConflCB;
  QCheckBox* mAutoDeclRecurCB;

    QCheckBox* mLegacyMangleFromTo;
};


#endif // _CONFIGURE_DIALOG_PRIVATE_H_
