// configuredialog_p.h: classes internal to ConfigureDialog
// see configuredialog.h for details.

#ifndef _CONFIGURE_DIALOG_PRIVATE_H_
#define _CONFIGURE_DIALOG_PRIVATE_H_

#define DEFAULT_EDITOR_STR "kate %f"


#include "kmheaders.h"
#include "kmime_util.h"

#include <qlineedit.h>
#include <qcombobox.h>
#include <qguardedptr.h>
#include <qwidget.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>

#include <kdialogbase.h>
#include <klistview.h>
#include <klocale.h> // for i18n

class QPushButton;
class QLabel;
class QCheckBox;
class KURLRequester;
class QMultiLineEdit;
class KFontChooser;
class QRadioButton;
class ColorListBox;
class QFont;
class QListViewItem;
class QTabWidget;
class QListBox;
class QButtonGroup;
class QRegExpValidator;
class KMFolderComboBox;
class KMAccount;
class KMTransportInfo;
class ListView;
class ConfigureDialog;
class KIntSpinBox;
class SimpleStringListEditor;
class KConfig;
namespace Kpgp {
  class Config;
};

//
//
// Identity handling
//
//

class IdentityEntry
{
public:
  IdentityEntry() : mSignatureFileIsAProgram( false ),
    mUseSignatureFile( true ), mIsDefault( false ) {}
  /** Convenience constructor for the most-often used settings */
  IdentityEntry( const QString & fullName,
		 const QString & email,
		 const QString & organization=QString::null,
		 const QString & replyTo=QString::null )
    : mFullName( fullName ), mOrganization( organization),
    mEmailAddress( email ), mReplyToAddress( replyTo ),
    mSignatureFileIsAProgram( false ),
    mUseSignatureFile( true ), mIsDefault( false ) {}

  /** Constructs an IdentituEntry from the control center default
      settings */
  static IdentityEntry fromControlCenter();

  /** used for sorting */
  bool operator<( const IdentityEntry & other ) const {
    // default is lower than any other, so it is always sorted to the
    // top:
    if ( isDefault() ) return true;
    if ( other.isDefault() ) return false;
    return identityName() < other.identityName();
  }
  // begin stupid stl headers workaround
  bool operator>( const IdentityEntry & other ) const {
    if ( isDefault() ) return false;
    if ( other.isDefault() ) return true;
    return identityName() > other.identityName();
  }
  bool operator<=( const IdentityEntry & other ) const {
    return !operator>( other );
  }
  bool operator>=( const IdentityEntry & other ) const {
    return !operator<( other );
  }
  // end stupid stl headers workaround
  /** used for searching */
  bool operator==( const IdentityEntry & other ) const {
    return identityName() == other.identityName();
  }

  QString identityName() const { return mIdentityName; }
  QString fullName() const { return mFullName; }
  QString organization() const { return mOrganization; }
  QString pgpIdentity() const { return mPgpIdentity ; }
  QString emailAddress() const { return mEmailAddress; }
  QString replyToAddress() const { return mReplyToAddress; }
  QString signatureFileName() const { return mSignatureFileName; }
  QString signatureInlineText() const { return mSignatureInlineText; }
  bool    signatureFileIsAProgram() const { return mSignatureFileIsAProgram; }
  bool    useSignatureFile() const { return mUseSignatureFile; }
  bool    isDefault() const { return mIsDefault; }
  QString transport() const { return mTransport; }
  QString fcc() const { return mFcc; }
  QString drafts() const { return mDrafts; }
  
  void setIdentityName( const QString & i ) { mIdentityName = i; }
  void setFullName( const QString & f ) { mFullName = f; }
  void setOrganization( const QString & o ) { mOrganization = o; }
  void setPgpIdentity( const QString & p ) { mPgpIdentity = p; }
  void setEmailAddress( const QString & e ) { mEmailAddress = e; }
  void setReplyToAddress( const QString & r ) { mReplyToAddress = r; }
  void setSignatureFileName( const QString & s ) { mSignatureFileName = s; }
  void setSignatureInlineText( const QString & t ) { mSignatureInlineText = t; }
  void setSignatureFileIsAProgram( bool b ) { mSignatureFileIsAProgram = b; }
  void setUseSignatureFile( bool b ) { mUseSignatureFile = b; }
  void setIsDefault( bool b ) { mIsDefault = b; }
  void setTransport( const QString & t ) { mTransport = t; }
  void setFcc( const QString & f ) { mFcc = f; }
  void setDrafts( const QString & f ) { mDrafts = f; }

private:
  // only add members that have an operator= defined or where the
  // compiler can generate one by itself!!
  QString mIdentityName;
  QString mFullName;
  QString mOrganization;
  QString mPgpIdentity;
  QString mEmailAddress;
  QString mReplyToAddress;
  QString mSignatureFileName;
  QString mSignatureInlineText;
  QString mTransport;
  QString mFcc;
  QString mDrafts;
  bool    mSignatureFileIsAProgram;
  bool    mUseSignatureFile;
  bool    mIsDefault;
};


class IdentityList : public QValueList<IdentityEntry>
{
public:
  // Let the compiler generate c'tors, d'tor and operator=. They are
  // sufficient.

  /** generates a list of display names of identies for use in
      widgets. */
  QStringList names() const;
  /** Gets and @ref IdentityEntry by name */
  IdentityEntry & getByName( const QString & identity );

  // remove when no longer commented out in qvaluelist.h:
  void sort() { qHeapSort( *this ); }

  /** Load system settings */
  void importData();
  /** Save state to system */
  void exportData() const;
};


class NewIdentityDialog : public KDialogBase
{
  Q_OBJECT

public:
  enum DuplicateMode { Empty, ControlCenter, ExistingEntry };

  NewIdentityDialog( const QStringList & identities,
		     QWidget *parent=0, const char *name=0, bool modal=true );
  
  QString identityName() const { return mLineEdit->text(); }
  int duplicateIdentity() const { return mComboBox->currentItem(); }
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

  /** Should set the page up (ie. read the setting from the @ref
      KConfig object into the widgets) after creating it in the
      constructor. Called from @ref ConfigureDialog. */
  virtual void setup() = 0;
  /** Called when the installation of a profile is
      requested. Reimplemenations of this method should do the
      equivalent of a @ref setup(), but with the given @ref KConfig
      object instead of kapp->config() and only for those entries that
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

protected:
  void addTab( QWidget * tab, const QString & title );

private:
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
  static QString helpAnchor();

  void setup();
  void apply();

public slots:
  void slotUpdateTransportCombo( const QStringList & );

protected slots:
  void slotNewIdentity();
  void slotRenameIdentity();
  void slotRemoveIdentity();
  void slotIdentitySelectorChanged();
  void slotChangeDefaultPGPKey();
  void slotSignatureEdit();
  void slotEnableSignatureEditButton( const QString &filename );

protected: // methods
  void saveActiveIdentity();
  void setIdentityInformation( const QString & identityName );
  QStringList identityStrings() const;

protected: // data members
  QString        mActiveIdentity;
  IdentityList   mIdentities;
  int            mOldNumberOfIdentities;

  // Widgets:
  // outside tabwidget:
  QComboBox        *mIdentityCombo;
  QPushButton      *mRenameButton;
  QPushButton      *mRemoveButton;

  // "general" tab:
  QLineEdit        *mNameEdit;
  QLineEdit        *mOrganizationEdit;
  QLineEdit        *mEmailEdit;
  // "advanced" tab:
  QLineEdit        *mReplyToEdit;
  QLabel           *mPgpIdentityLabel;
  KMFolderComboBox *mFccCombo;
  KMFolderComboBox *mDraftsCombo;
  QCheckBox        *mTransportCheck;
  QComboBox        *mTransportCombo; // should be a KMTransportCombo...
  // "signature" tab:
  QCheckBox        *mSignatureEnabled;
  QComboBox        *mSignatureSourceCombo;
  KURLRequester    *mSignatureFileRequester;
  QPushButton      *mSignatureEditButton;
  KURLRequester    *mSignatureCommandRequester;
  QMultiLineEdit   *mSignatureTextEdit;
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
  static QString helpAnchor();
  
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
  static QString helpAnchor();
  
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

protected:
  QStringList occupiedNames();

protected:
  ListView      *mAccountList;
  QPushButton   *mModifyAccountButton;
  QPushButton   *mRemoveAccountButton;
  QCheckBox     *mBeepNewMailCheck;
  QCheckBox     *mShowMessageBoxCheck;
  QCheckBox     *mMailCommandCheck;
  KURLRequester *mMailCommandRequester;
  
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
  static QString helpAnchor();

  void setup();
  void apply();
  void dismiss(); // needed for account list cleanup.
  void installProfile( KConfig * profile );

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
  static QString helpAnchor();
  
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
  static QString helpAnchor();
  
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
  static QString helpAnchor();

  void setup();
  void apply();
  void installProfile( KConfig * profile );
  
protected:
  QCheckBox    *mLongFolderCheck;
  QCheckBox    *mShowColorbarCheck;
  QCheckBox    *mMessageSizeCheck;
  QCheckBox    *mNestedMessagesCheck;
  QButtonGroup *mNestingPolicy;
  QButtonGroup *mDateDisplay;
  
  enum { numDateDisplayConfig = 4 };
  static const struct dateDisplayConfigType {
    const char *  displayName;
    KMime::DateFormatter::FormatType dateDisplay;
  } dateDisplayConfig[ numDateDisplayConfig ];

};

class AppearancePageProfileTab : public ConfigurationPage {
  Q_OBJECT
public:
  AppearancePageProfileTab( QWidget * parent=0, const char * name=0 );
  
  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }
  
  static QString title();
  static QString helpAnchor();
  
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
  static QString helpAnchor();

  void setup();
  void apply();
  void installProfile( KConfig * profile );

  // hrmpf. moc doesn't like nested classes with slots/signals...:
  typedef AppearancePageFontsTab FontsTab;
  typedef AppearancePageColorsTab ColorsTab;
  typedef AppearancePageLayoutTab LayoutTab;
  typedef AppearancePageProfileTab ProfileTab;

signals:
  /** Used to route @p mProfileTab's signal on to the
      configuredialog's slotInstallProfile() */
  void profileSelected( KConfig * profile );

protected:
  FontsTab   *mFontsTab;
  ColorsTab  *mColorsTab; 
  LayoutTab  *mLayoutTab;
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
  static QString helpAnchor();

  void setup();
  void apply();
  void installProfile( KConfig * profile );

protected:
  QCheckBox     *mAutoAppSignFileCheck;
  QCheckBox     *mSmartQuoteCheck;
  QCheckBox     *mPgpAutoSignatureCheck;
  QCheckBox     *mPgpAutoEncryptCheck;
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
  static QString helpAnchor();
  
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
  static QString helpAnchor();
  
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
  static QString helpAnchor();

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
  static QString helpAnchor();
  
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
  static QString helpAnchor();

  void setup();
  void apply();
  void installProfile( KConfig * profile );

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
  static QString helpAnchor();

  void setup();
  void apply();
  void installProfile( KConfig * profile );

protected:
  QCheckBox *mExternalReferences;
  QCheckBox *mHtmlMailCheck;
  QCheckBox *mSendReceiptCheck;
};

class SecurityPage : public TabbedConfigurationPage {
  Q_OBJECT
public:
  SecurityPage( QWidget * parent=0, const char * name=0 );

  static QString iconLabel();
  static const char * iconName();
  static QString title();
  static QString helpAnchor();

  void setup();
  void apply();
  void installProfile( KConfig * profile );

  typedef SecurityPageGeneralTab GeneralTab;

protected:
  GeneralTab   *mGeneralTab;
  Kpgp::Config *mPgpTab;
};


//
//
// MiscPage
//
//

class MiscPageFoldersTab : public ConfigurationPage {
  Q_OBJECT
public:
  MiscPageFoldersTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  static QString helpAnchor();

  void setup();
  void apply();

protected:
  QCheckBox  *mEmptyFolderConfirmCheck;
  QCheckBox  *mWarnBeforeExpire;
  QCheckBox  *mLoopOnGotoUnread;
  QComboBox  *mMailboxPrefCombo;
  QCheckBox  *mCompactOnExitCheck;
  QCheckBox  *mEmptyTrashCheck;
  QCheckBox  *mExpireAtExit;
};

class MiscPageAddressbookTab : public ConfigurationPage {
  Q_OBJECT
public:
  MiscPageAddressbookTab( QWidget * parent=0, const char * name=0 );

  // no icons:
  static QString iconLabel() { return QString::null; }
  static const char * iconName() { return 0; }

  static QString title();
  static QString helpAnchor();

  void setup();
  void apply();

protected:
  QComboBox *mAddressbookCombo;
};


class MiscPage : public TabbedConfigurationPage {
  Q_OBJECT
public:
  MiscPage( QWidget * parent=0, const char * name=0 );

  static QString iconLabel();
  static const char * iconName();
  static QString title();
  static QString helpAnchor();

  void setup();
  void apply();
  void installProfile( KConfig * profile );

  typedef MiscPageFoldersTab FoldersTab;
  typedef MiscPageAddressbookTab AddressbookTab;

protected:
  FoldersTab     *mFoldersTab;
  AddressbookTab *mAddressbookTab;
};

//
//
// PluginPage
//
//

class PluginPage : public ConfigurationPage {
  Q_OBJECT
public:
  PluginPage( QWidget * parent=0, const char * name=0 );

  static QString iconLabel();
  static const char * iconName();
  static QString title();
  static QString helpAnchor();

  void setup();
  void apply();
  void installProfile( KConfig * profile );

protected:
};

//
//
// further helper classes:
//
//

class ApplicationLaunch {
public:
  ApplicationLaunch( const QString &cmd ) : mCmdLine( cmd ) {}
  void run();
  
private:
  void doIt();
  
private:
  QString mCmdLine;
};

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
