/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _CONFIGURE_DIALOG_H_
#define _CONFIGURE_DIALOG_H_

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListViewItem;
class QMultiLineEdit;
class QPushButton;
class QRadioButton;
class KIntNumInput;
class KColorButton;
class KFontChooser;
class KURLRequester;
class ColorListBox;
class KMAccount;
class KMTransportInfo;
class KMFolder;
class KMFolderComboBox;

namespace Kpgp {
  class Config;
};

#include <klistview.h>
#include <kdialogbase.h>
#include <qguardedptr.h>
#include <qvaluelist.h>
#include <qcombobox.h>

#define DEFAULT_EDITOR_STR "kwrite %f"

class NewIdentityDialog : public KDialogBase
{
  Q_OBJECT

  public:
    enum DuplicateMode
    {
      Empty = 0,
      ControlCenter,
      ExistingEntry
    };

    NewIdentityDialog( QWidget *parent=0, const char *name=0, bool modal=true);
    void setIdentities( const QStringList &list );

    QString identityText( void );
    QString duplicateText( void );
    int     duplicateMode( void );

  private slots:
    void radioClicked( int id );

  protected slots:
    virtual void slotOk( void );

  private:
  int mDuplicateMode;
    QLineEdit  *mLineEdit;
    QLabel     *mComboLabel;
    QComboBox  *mComboBox;
};



class RenameIdentityDialog : public KDialogBase
{
  Q_OBJECT

  public:
    RenameIdentityDialog( QWidget *parent=0, const char *name=0,
			  bool modal=true );
    void setIdentities( const QString &current, const QStringList &list );
    QString identityText( void );

  protected:
    virtual void showEvent( QShowEvent * );

  protected slots:
    virtual void slotOk( void );

  private:
    QLineEdit   *mLineEdit;
    QLabel      *mCurrentNameLabel;
    QStringList mIdentityList;
};



class IdentityEntry
{
  public:
    IdentityEntry( void );
    QString identity() const;
    QString fullName() const;
    QString organization() const;
    QString pgpIdentity() const;
    QString emailAddress() const;
    QString replyToAddress() const;
    QString signatureFileName( bool exportIdentity = false ) const;
    QString signatureInlineText() const;
    bool    signatureFileIsAProgram() const;
    bool    useSignatureFile() const;
    QString transport() const;
    QString fcc() const;

    void setIdentity( const QString &identity );
    void setFullName( const QString &fullName );
    void setOrganization( const QString &organization );
    void setPgpIdentity( const QString &pgpIdentity );
    void setEmailAddress( const QString &emailAddress );
    void setReplyToAddress( const QString &replytoAddress );
    void setSignatureFileName( const QString &signatureFileName,
			       bool importIdentity=false );
    void setSignatureInlineText( const QString &signatureInlineText );
    void setSignatureFileIsAProgram( bool signatureFileIsAProgram );
    void setUseSignatureFile( bool useSignatureFile );
    void setTransport(const QString &transport);
    void setFcc( const QString &fcc );

  private:
    QString mIdentity;
    QString mFullName;
    QString mOrganization;
    QString mPgpIdentity;
    QString mEmailAddress;
    QString mReplytoAddress;
    QString mSignatureFileName;
    QString mSignatureInlineText;
    bool    mSignatureFileIsAProgram;
    bool    mUseSignatureFile;
    QString mTransport;
    QString mFcc;

};


class IdentityList
{
  public:
    IdentityList();

    QStringList identities( void );
    IdentityEntry *get( const QString &identity );
    IdentityEntry *get( uint index );

    /** Load system settings */
    void importData( void );
    /** Save state to system */
    void exportData( void );

    void add( const IdentityEntry &entry );
    void add( const QString &identity, const QString &copyFrom );
    void add( const QString &identity, QWidget *parent, bool useControlCenter);
    void update( const IdentityEntry &entry );

    void remove( const QString &identity );

  private:
    QPtrList<IdentityEntry> mList;
};

class LanguageItem
{
  public:
    LanguageItem( const QString& language, const QString& reply,
      const QString& replyAll, const QString& forward,
      const QString& indentPrefix );
    QString mLanguage, mReply, mReplyAll, mForward, mIndentPrefix;
    LanguageItem *next;
};

class NewLanguageDialog : public KDialogBase
{
  Q_OBJECT

  public:
    NewLanguageDialog( QWidget *parent, const char *name, bool modal,
      LanguageItem *langList );
    QString language( void ) const;

  private:
    QComboBox *mComboBox;
};

class LanguageComboBox : public QComboBox
{
  Q_OBJECT

  public:
    LanguageComboBox( bool rw, QWidget *parent=0, const char *name=0 );
    int insertLanguage( const QString & language );
    QString language( void ) const;
    void setLanguage( const QString & language );
  private:
    QString *i18nPath;
};

class KScoringEditorWidget;

class ConfigureDialog : public KDialogBase
{
  Q_OBJECT

public:
    class ApplicationLaunch
    {
      public:
        ApplicationLaunch( const QString &cmd );
        void run( void );

      private:
        void doIt( void );

      private:
	QString mCmdline;
    };

    class ListView : public KListView
    {
      public:
        ListView( QWidget *parent=0, const char *name=0, int visibleItem=10 );
	void resizeColums( void );

	void setVisibleItem( int visibleItem, bool updateSize=true );
	virtual QSize sizeHint( void ) const;

      protected:
	virtual void resizeEvent( QResizeEvent *e );
	virtual void showEvent( QShowEvent *e );

      private:
	int mVisibleItem;
    };

private:
    struct IdentityWidget
    {
      int            pageIndex;
      QString        mActiveIdentity;
      QComboBox      *identityCombo;
      QPushButton    *removeIdentityButton;
      QPushButton    *renameIdentityButton;
      // "General" tab:
      QLineEdit      *nameEdit;
      QLineEdit      *organizationEdit;
      QLineEdit      *emailEdit;
      // "Advanced' tab:
      QLineEdit      *replytoEdit;
      QLabel         *pgpIdentityLabel;
      QCheckBox      *transportCheck;
      QComboBox      *transportCombo;
      KMFolderComboBox  *fccCombo;
      // "Signature" tab:
      QCheckBox      *signatureEnabled;
      QComboBox      *signatureSourceCombo;
      KURLRequester  *signatureFileRequester;
      QPushButton    *signatureEditButton;
      KURLRequester  *signatureCommandRequester;
      QMultiLineEdit *signatureTextEdit;
    };
    struct NetworkWidget
    {
      int          pageIndex;
      // "Sending" tab:
      ListView     *transportList;
      QPushButton  *addTransportButton;
      QPushButton  *modifyTransportButton;
      QPushButton  *removeTransportButton;
      QPushButton  *transportUpButton;
      QPushButton  *transportDownButton;
      // "Common options" group in "sending" tab:
      QComboBox    *sendMethodCombo;
      QComboBox    *messagePropertyCombo;
      QCheckBox    *confirmSendCheck;
      QCheckBox    *sendOutboxCheck;
      // "Receiving" tab:
      ListView     *accountList;
      QPushButton  *addAccountButton;
      QPushButton  *modifyAccountButton;
      QPushButton  *removeAccountButton;
      // "New Mail Notification" group box:
      QCheckBox   *beepNewMailCheck;
      QCheckBox   *showMessageBoxCheck;
      QCheckBox   *mailCommandCheck;
      QLineEdit   *mailCommandEdit;
      QPushButton *mailCommandChooseButton;
    };
    struct AppearanceWidget
    {
      AppearanceWidget( void )
      {
	activeFontIndex = -1;
      }

      int           pageIndex;
      QCheckBox     *customFontCheck;
      QLabel        *fontLocationLabel;
      QComboBox     *fontLocationCombo;
      KFontChooser  *fontChooser;
      QCheckBox     *customColorCheck;
      ColorListBox  *colorList;
      QCheckBox     *recycleColorCheck;
      QCheckBox     *longFolderCheck;
      QCheckBox     *showColorbarCheck;
      QCheckBox     *messageSizeCheck;
      QCheckBox     *nestedMessagesCheck;
      QRadioButton  *rdAlwaysOpen;
      QRadioButton  *rdDefaultOpen;
      QRadioButton  *rdDefaultClosed;
      QRadioButton  *rdUnreadOpen;
      QRadioButton *rdDateCtime;
      QRadioButton *rdDateLocalized;
      QRadioButton *rdDateFancy;
      int           activeFontIndex;
      QFont         font[7];
      ListView      *profileList;
      QListViewItem *mListItemDefault;
      QListViewItem *mListItemDefaultHtml;
      QListViewItem *mListItemContrast;
      QListViewItem *mListItemPurist;
      QPushButton  *profileDeleteButton;
      QComboBox     *addressbookCombo;
      QLabel        *addressbookLabel;
      QStringList   addressbookStrings;
    };
    struct ComposerWidget
    {
      int       pageIndex;
      LanguageComboBox *phraseLanguageCombo;
      QPushButton  *removeButton;
      QLineEdit    *phraseReplyEdit;
      QLineEdit    *phraseReplyAllEdit;
      QLineEdit    *phraseForwardEdit;
      QLineEdit    *phraseindentPrefixEdit;
      QCheckBox    *autoAppSignFileCheck;
      QCheckBox    *smartQuoteCheck;
      QCheckBox    *pgpAutoSignatureCheck;
      QCheckBox    *pgpAutoEncryptCheck;
      QCheckBox    *wordWrapCheck;
      KIntNumInput *wrapColumnSpin;
      QCheckBox   *externalEditorCheck;
      QLineEdit   *externalEditorEdit;
      QPushButton *externalEditorChooseButton;
      QLabel      *externalEditorLabel;
      QLabel      *externalEditorHelp;
      LanguageItem *LanguageList;
      LanguageItem *CurrentLanguage;
      QListBox *replyListBox;
      QPushButton *addReplyPrefixButton;
      QPushButton *removeReplyPrefixButton;
      QCheckBox *replaceReplyPrefixCheck;
      QListBox *forwardListBox;
      QPushButton *addForwardPrefixButton;
      QPushButton *removeForwardPrefixButton;
      QCheckBox *replaceForwardPrefixCheck;
      QListBox *charsetListBox;
      QPushButton *addCharsetButton;
      QPushButton *removeCharsetButton;
      QPushButton *charsetUpButton;
      QPushButton *charsetDownButton;
      QCheckBox* forceReplyCharsetCheck;
    };
    struct MimeWidget
    {
      MimeWidget( void )
      {
	currentTagItem = 0;
      }

      int           pageIndex;
      ListView      *tagList;
      QListViewItem *currentTagItem;
      QLineEdit     *tagNameEdit;
      QLineEdit     *tagValueEdit;
      QLabel        *tagNameLabel;
      QLabel        *tagValueLabel;
      QCheckBox     *createOwnMessageIdCheck;
      QLabel        *messageIdSuffixLabel;
      QLineEdit     *messageIdSuffixEdit;
      QLabel        *messageIdSuffixHintLabel;
    };
    struct SecurityWidget
    {
      int          pageIndex;
      Kpgp::Config *pgpConfig;
      QCheckBox    *htmlMailCheck;
      QCheckBox    *externalReferences;
      QCheckBox    *sendReceiptCheck;
    };
    struct MiscWidget
    {
      int         pageIndex;
      QCheckBox   *emptyTrashCheck;
      QCheckBox   *compactOnExitCheck;
      QCheckBox   *emptyFolderConfirmCheck;
      QButtonGroup *expBGroup;
      QRadioButton *manualExpiry;
      QRadioButton *expireAtExit;
      QRadioButton *expireAtStart;
      QRadioButton *expireDaily;
      QRadioButton *expireWeekly;
      QCheckBox   *warnBeforeExpire;
    };

    enum EPage
    {
      page_identity = 0,
      page_network,
      page_appearance,
      page_composer,
      page_mimeheader,
      page_security,
      page_misc,
      page_folder,
      page_max
    };

  public:
    ConfigureDialog( QWidget *parent=0, const char *name=0, bool modal=true );
    ~ConfigureDialog( void );
    virtual void show( void );

  protected:
    virtual void slotDefault( void );
    virtual void slotOk( void );
    virtual void slotApply( void );
    virtual void slotDoApply( bool everything );
    void setup( void );

  private:
    void makeIdentityPage( void );
    void makeNetworkPage( void );
    void makeAppearancePage( void );
    void makeComposerPage( void );
    void makeMimePage( void );
    void makeSecurityPage( void );
    void makeMiscPage( void );

    void setupIdentityPage( void );
    void setupNetworkPage( void );
    void setupAppearancePage( void );
    void setupComposerPage( void );
    void setupMimePage( void );
    void setupSecurityPage( void );
    void setupMiscPage( void );
    void installProfile( void );

    void updateFontSelector( void );
    void saveActiveIdentity( void );
    void setIdentityInformation( const QString &identityName );
    QStringList identityStrings( void );
    QStringList occupiedNames( void );

  private slots:
    void slotCancelOrClose( void );
    void slotNewIdentity( void );
    void slotRenameIdentity( void );
    void slotRemoveIdentity( void );
    void slotIdentitySelectorChanged( void );
    void slotChangeDefaultPGPKey( void );
    void slotSignatureEdit( void );
    void slotEnableSignatureEditButton( const QString &filename );
    void slotTransportSelected( void );
    void slotUpdateTransportCombo( void );
    void slotAddTransport( void );
    void slotModifySelectedTransport( void );
    void slotRemoveSelectedTransport( void );
    void slotTransportUp( void );
    void slotTransportDown( void );
    void slotAccountSelected( void );
    void slotAddAccount( void );
    void slotModifySelectedAccount( void );
    void slotRemoveSelectedAccount( void );
    void slotCustomFontSelectionChanged( void );
    void slotCreateOwnMessageIdChanged( void );
    void slotFontSelectorChanged( int index );
    void slotAddressbookSelectorChanged( int index );
    void slotCustomColorSelectionChanged( void );
    void slotNewLanguage( void );
    void slotRemoveLanguage( void );
    void slotSaveOldPhrases( void );
    void slotLanguageChanged( const QString& );
    void slotAddNewLanguage( const QString& );
    void slotWordWrapSelectionChanged( void );
    void slotAddReplyPrefix( void );
    void slotRemoveSelReplyPrefix( void );
    void slotReplyPrefixSelected( void );
    void slotAddForwardPrefix( void );
    void slotRemoveSelForwardPrefix( void );
    void slotForwardPrefixSelected( void );
    void slotAddCharset( void );
    void slotRemoveSelCharset( void );
    void slotCharsetUp( void );
    void slotCharsetDown( void );
    void slotCharsetSelectionChanged( void );
    void slotMimeHeaderSelectionChanged( void );
    void slotMimeHeaderNameChanged( const QString &text );
    void slotMimeHeaderValueChanged( const QString &text );
    void slotNewMimeHeader( void );
    void slotDeleteMimeHeader( void );
    void slotExternalEditorSelectionChanged( void );
    void slotExternalEditorChooser( void );
    void slotMailCommandChooser( void );

  private:
    IdentityWidget   mIdentity;
    NetworkWidget    mNetwork;
    AppearanceWidget mAppearance;
    ComposerWidget   mComposer;
    MimeWidget       mMime;
    SecurityWidget   mSecurity;
    MiscWidget       mMisc;

    IdentityList     mIdentityList;
    QValueList<QGuardedPtr<KMAccount> > mAccountsToDelete;
    QValueList<QGuardedPtr<KMAccount> > mNewAccounts;
    struct mModifiedAccountsType
    {
      QGuardedPtr<KMAccount> oldAccount;
      QGuardedPtr<KMAccount> newAccount;
    };
    QValueList<mModifiedAccountsType*> mModifiedAccounts;
    QPtrList<KMTransportInfo> mTransportList;
    bool secondIdentity;
};

#endif
