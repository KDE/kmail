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

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListViewItem;
class QMultiLineEdit;
class QPushButton;
class QRadioButton;
class KColorButton;
class KFontChooser;
class KpgpConfig;
class ColorListBox;

#include <klistview.h>
#include <kdialogbase.h>


class NewIdentityDialog : public KDialogBase
{
  Q_OBJECT

  public:
    NewIdentityDialog( QWidget *parent=0, const char *name=0, bool modal=true);
    void setIdentities( const QStringList &list );

    QString identityText( void );
    QString duplicateText( void );
      
  private slots:
    void radioClicked( int id );

  private:
    QLineEdit *mLineEdit; 
    QLabel    *mComboLabel;
    QComboBox *mComboBox;
};




class IdentityEntry
{
  public:
    QString identity() const;
    QString fullName() const;
    QString organization() const;
    QString emailAddress() const;
    QString replyToAddress() const;
    QString signatureFileName() const;
    QString signatureInlineText() const;
    bool    signatureFileIsAProgram() const;
    bool    useSignatureFile() const;

    void setIdentity( const QString &identity );
    void setFullName( const QString &fullName );
    void setOrganization( const QString &organization );
    void setEmailAddress( const QString &emailAddress );
    void setReplyToAddress( const QString &replytoAddress );
    void setSignatureFileName( const QString &signatureFileName );
    void setSignatureInlineText( const QString &signatureInlineText );
    void setSignatureFileIsAProgram( bool signatureFileIsAProgram );
    void setUseSignatureFile( bool useSignatureFile );

  private:
    QString mIdentity;
    QString mFullName;
    QString mOrganization;
    QString mEmailAddress;
    QString mReplytoAddress;
    QString mSignatureFileName;
    QString mSignatureInlineText;
    bool    mSignatureFileIsAProgram;
    bool    mUseSignatureFile;
};


class IdentityList
{
  public:
    IdentityList();

    QStringList identities( void );
    IdentityEntry *get( const QString &identity );
 
    void import( void ); // Load system settings 
    void save( void ); // Save state to system

    void add( const IdentityEntry &entry );
    void add( const QString &identity, const QString &copyFrom );
    void update( const IdentityEntry &entry );

    void remove( const QString &identity );

  private:
    QList<IdentityEntry> mList;
};



class ConfigureDialog : public KDialogBase
{
  Q_OBJECT

  private:
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

    struct IdentityWidget
    {
      int            pageIndex;
      QComboBox      *identityCombo;
      QPushButton    *removeIdentityButton;
      QLineEdit      *nameEdit;
      QLineEdit      *organizationEdit;
      QLineEdit      *emailEdit;
      QLineEdit      *replytoEdit;
      QLineEdit      *signatureFileEdit;
      QLabel         *signatureFileLabel;
      QCheckBox      *signatureExecCheck;
      QPushButton    *signatureBrowseButton;
      QPushButton    *signatureEditButton;
      QRadioButton   *signatureFileRadio;
      QRadioButton   *signatureTextRadio;
      QMultiLineEdit *signatureTextEdit;
      QString        mActiveIdentity;
    };
    struct NetworkWidget
    {
      int          pageIndex;
      QRadioButton *sendmailRadio;
      QRadioButton *smtpRadio;
      QPushButton  *sendmailChooseButton;
      QLineEdit    *sendmailLocationEdit;
      QLineEdit    *smtpServerEdit;
      QLineEdit    *smtpPortEdit;
      ListView     *accountList;
      QPushButton  *addAccountButton;
      QPushButton  *modifyAccountButton;
      QPushButton  *removeAccountButton;

      QComboBox    *sendMethodCombo;
      QComboBox    *messagePropertyCombo;
      QCheckBox    *confirmSendCheck;
    };
    struct AppearanceWidget
    {
      AppearanceWidget( void )
      {
	activeFontIndex = -1;
      }

      int          pageIndex;
      QCheckBox    *customFontCheck;
      QLabel       *fontLocationLabel;
      QComboBox    *fontLocationCombo;
      KFontChooser *fontChooser;
      QCheckBox    *customColorCheck;
      ColorListBox *colorList;
      QCheckBox    *longFolderCheck;
      int          activeFontIndex;
      QString      fontString[6];
    };
    struct ComposerWidget
    {
      int       pageIndex;
      QLineEdit *phraseReplyEdit;
      QLineEdit *phraseReplyAllEdit;
      QLineEdit *phraseForwardEdit;
      QLineEdit *phraseindentPrefixEdit;
      QCheckBox *autoAppSignFileCheck;
      QCheckBox *smartQuoteCheck;
      QCheckBox *pgpAutoSignatureCheck;
      QCheckBox *monoSpaceFontCheck;
      QCheckBox *wordWrapCheck;
      QLineEdit *wrapColumnEdit;
      QComboBox *sendMethodCombo;
      QComboBox *messagePropertyCombo;
      QCheckBox *confirmSendCheck; 
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
    };
    struct SecurityWidget
    {
      int        pageIndex;
      KpgpConfig *pgpConfig;
    };
    struct MiscWidget
    {
      int         pageIndex;
      QCheckBox   *emptyTrashCheck;
      QCheckBox   *sendOutboxCheck;
      QCheckBox   *sendReceiptCheck;
      QCheckBox   *compactOnExitCheck;
      QCheckBox   *externalEditorCheck;
      QLineEdit   *externalEditorEdit;
      QPushButton *externalEditorChooseButton;
      QLabel      *externalEditorLabel;
      QLabel      *externalEditorHelp;
      QCheckBox   *beepNewMailCheck;
      QCheckBox   *showMessageBoxCheck;
      QCheckBox   *mailCommandCheck;
      QLineEdit   *mailCommandEdit;
      QPushButton *mailCommandChooseButton;
      QLabel      *mailCommandLabel;
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
      page_max
    };

  public:
    ConfigureDialog( QWidget *parent=0, const char *name=0, bool modal=true );
    ~ConfigureDialog( void );
    void setup( void );

  protected:
    virtual void slotDefault( void );
    virtual void slotOk( void );
    virtual void slotApply( void );

  private:
    void makeIdentityPage( void );
    void makeNetworkPage( void );
    void makeApperancePage( void );
    void makeComposerPage( void );
    void makeMimePage( void );
    void makeSecurityPage( void );
    void makeMiscPage( void );

    void setupIdentityPage( void );
    void setupNetworkPage( void );
    void setupApperancePage( void );
    void setupComposerPage( void );
    void setupMimePage( void );
    void setupSecurityPage( void );
    void setupMiscPage( void );
    void saveActiveIdentity( void );
    void setIdentityInformation( const QString &identityName );
    QStringList identityStrings( void );

  private slots:
    void slotNewIdentity( void );
    void slotRemoveIdentity( void );
    void slotIdentitySelectorChanged( void );
    void slotSignatureType( int id );
    void slotSignatureChooser( void );
    void slotSignatureEdit( void );
    void slotSignatureFile( const QString &filename );
    void slotSignatureExecMode( bool state );
    void slotSendmailType( int id );
    void slotSendmailChooser( void );
    void slotAccountSelected( void );
    void slotAddAccount( void );
    void slotModifySelectedAccount( void );
    void slotRemoveSelectedAccount( void );
    void slotCustomFontSelectionChanged( void );
    void slotFontSelectorChanged( int index );
    void slotCustomColorSelectionChanged( void );
    void slotWordWrapSelectionChanged( void );
    void slotMimeHeaderSelectionChanged( void );
    void slotMimeHeaderNameChanged( const QString &text );
    void slotMimeHeaderValueChanged( const QString &text );
    void slotNewMimeHeader( void );
    void slotDeleteMimeHeader( void );
    void slotExternalEditorSelectionChanged( void );
    void slotMailCommandSelectionChanged( void );
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
};


#endif
