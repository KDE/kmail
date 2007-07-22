/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef __KMComposeWin
#define __KMComposeWin

// doesn't work for the DBus adaptor...
/*#ifndef KDE_USE_FINAL
# ifndef REALLY_WANT_KMCOMPOSEWIN_H
#  error Do not include kmcomposewin.h anymore. Include composer.h instead.
# endif
#endif*/

#include "composer.h"
#include "messagesender.h"

#include <QByteArray>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QEvent>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QPalette>
#include <QPushButton>
#include <QSplitter>

#include <kio/job.h>
#include <kglobalsettings.h>
#include <kdeversion.h>
#include <keditcl.h>
#include <ktempdir.h>

#include <libkdepim/addresseelineedit.h>
#include <mimelib/mediatyp.h>

#include "libkleo/kleo/enum.h"

class QCloseEvent;
class QComboBox;
class QGridLayout;
class QPushButton;
class QByteArray;
class QTreeWidgetItem;
class KMEdit;
class KMComposeWin;
class KMFolderComboBox;
class KMFolder;
class KMMessage;
class KMMessagePart;
class KSelectAction;
class KFontAction;
class KFontSizeAction;
class KSelectAction;
class KAction;
class KToggleAction;
class KTemporaryFile;
class KToggleAction;
class KUrl;
class KRecentFilesAction;
class MessageComposer;
class RecipientsEditor;
class KMLineEdit;
class KMLineEditSpell;
class KMAtmListViewItem;

namespace KPIMIdentities {
  class IdentityCombo;
  class Identity;
}

namespace MailTransport {
  class TransportComboBox;
}

namespace KMail {
  class AttachmentListView;
  class DictionaryComboBox;
}

namespace GpgME {
  class Error;
}

//-----------------------------------------------------------------------------
class KMComposeWin : public KMail::Composer
{
  Q_OBJECT
  friend class ::KMEdit;
  friend class ::MessageComposer;

  private: // mailserviceimpl, kmkernel, kmcommands, callback, kmmainwidget
    KMComposeWin( KMMessage *msg=0, uint identity=0 );
    ~KMComposeWin();

  public:
    static Composer *create( KMMessage *msg = 0, uint identity = 0 );

  QString dbusObjectPath() const;
public: // mailserviceimpl
    /**
     * From MailComposerIface
     */
    void send( int how );
    void addAttachmentsAndSend( const KUrl::List &urls, const QString &comment, int how );
    void addAttachment( const KUrl &url, const QString &comment );
    void addAttachment( const QString &name,
                        const QByteArray &cte,
                        const QByteArray &data,
                        const QByteArray &type,
                        const QByteArray &subType,
                        const QByteArray &paramAttr,
                        const QString &paramValue,
                        const QByteArray &contDisp );
  public: // kmcommand
    void setBody ( const QString &body );

  private:
    /**
     * To catch palette changes
     */
    virtual bool event( QEvent *e );

    /**
     * update colors
     */
    void readColorConfig();

    /**
     * Write settings to app's config file.
     */
    void writeConfig( void );

    /**
     * If necessary increases the word wrap of the editor so that it will
     * not wrap the body string
     */
    void verifyWordWrapLengthIsAdequate( const QString & );

  public: // kmkernel, kmcommands, callback
    /**
     * Set the message the composer shall work with. This discards
     * previous messages without calling applyChanges() on them before.
     */
     void setMsg( KMMessage *newMsg, bool mayAutoSign=true,
                 bool allowDecryption=false, bool isModified=false );

  private: // kmedit
    /**
     * Returns message of the composer. To apply the user changes to the
     * message, call applyChanges() first.
     */
     KMMessage *msg() const { return mMsg; }

  public: // kmkernel
    /**
     * Set the filename which is used for autosaving.
     */
    void setAutoSaveFilename( const QString & filename );

  private:
    /**
     * Returns true if the message was modified by the user.
     */
    bool isModified() const;

    /**
     * Set whether the message should be treated as modified or not.
     */
    void setModified( bool modified );

  public: // kmkernel, callback
    /**
     * If this flag is set the message of the composer is deleted when
     * the composer is closed and the message was not sent. Default: false
     */
     inline void setAutoDelete( bool f ) { mAutoDeleteMsg = f; }

  public: // kmcommand
    /**
     * If this folder is set, the original message is inserted back after
     * canceling
     */
     void setFolder( KMFolder *aFolder ) { mFolder = aFolder; }

  public: // kmkernel, kmcommand, mailserviceimpl
    /**
     * Recode to the specified charset
     */
     void setCharset( const QByteArray &aCharset, bool forceDefault = false );

  public: // kmcommand
    /**
     * Sets the focus to the edit-widget and the cursor below the
     * "On ... you wrote" line when hasMessage is true.
     * Make sure you call this _after_ setMsg().
     */
     void setReplyFocus( bool hasMessage = true );

    /**
     * Sets the focus to the subject line edit. For use when creating a
     * message to a known recipient.
     */
     void setFocusToSubject();

  private:
    /**
     * determines whether inline signing/encryption is selected
     */
     bool inlineSigningEncryptionSelected();

     /**
      * Tries to find the given mimetype @p type in the KDE Mimetype registry.
      * If found, returns its localized description, otherwise the @p type
      * in lowercase.
      */
     static QString prettyMimeType( const QString &type );
    QString quotePrefixName() const;

  private: // kmedit:
    KMLineEditSpell *sujectLineWidget() const { return mEdtSubject;}
    void setSubjectTextWasSpellChecked( bool _spell ) {
      mSubjectTextWasSpellChecked = _spell;
    }
    bool subjectTextWasSpellChecked() const {
      return mSubjectTextWasSpellChecked;
    }

  public: // callback
    /** Disabled signing and encryption completely for this composer window. */
    void setSigningAndEncryptionDisabled( bool v )
    {
      mSigningAndEncryptionExplicitlyDisabled = v;
    }

  private slots:
    void ensurePolished();

    /**
     * Actions:
     */
    void slotPrint();
    void slotAttachFile();
    void slotInsertRecentFile( const KUrl & );
    void slotAttachedFile( const KUrl & );

  public slots: // kmkernel, callback
    void slotSendNow();

  private slots:
    void slotSendNowVia( QAction * );
    void slotSendLater();
    void slotSendLaterVia( QAction * );
    void getTransportMenu();

    /**
     * Returns true when saving was successful.
     */
    void slotSaveDraft();
    void slotSaveTemplate();
    void slotNewComposer();
    void slotNewMailReader();
    void slotClose();
    void slotHelp();

    void slotFind();
    void slotSearchAgain();
    void slotReplace();
    void slotUndo();
    void slotRedo();
    void slotCut();
    void slotCopy();
    void slotPaste();
    void slotPasteAsQuotation();
    void slotPasteAsAttachment();
    void slotAddQuotes();
    void slotRemoveQuotes();
    void slotAttachPNGImageData( const QByteArray &image );

    void slotMarkAll();

    void slotFolderRemoved( KMFolder * );

  public slots: // kmkernel
    /**
       Tell the composer to always send the message, even if the user
       hasn't changed the next. This is useful if a message is
       autogenerated (e.g., via a DCOP call), and the user should
       simply be able to confirm the message and send it.
    */
    void slotSetAlwaysSend( bool bAlwaysSend );

  private slots:
    /**
     * toggle fixed width font.
     */
    void slotUpdateFont();

    /**
     * Open addressbook editor dialog.
     */
    void slotAddrBook();

    /**
     * Insert a file to the end of the text in the editor.
     */
    void slotInsertFile();

    void slotSetCharset();
    /**
     * Check spelling of text.
     */
    void slotSpellcheck();
    void slotSpellcheckConfig();
    void slotSubjectTextSpellChecked();

    /**
     * Change crypto plugin to be used for signing/encrypting messages,
     * or switch to built-in OpenPGP code.
     */
    void slotSelectCryptoModule( bool init = false );

    /**
     * XML-GUI stuff
     */
    void slotStatusMessage( const QString &message );
    void slotEditToolbars();
    void slotUpdateToolbars();
    void slotEditKeys();

    /**
     * Read settings from app's config file.
     */
    void readConfig( void );

    /**
     * Change window title to given string.
     */
    void slotUpdWinTitle( const QString & );

    /**
     * Switch the icon to lock or unlock respectivly.
     * Change states of all encrypt check boxes in the attachments listview
     */
    void slotEncryptToggled( bool );

    /**
     * Change states of all sign check boxes in the attachments listview
     */
    void slotSignToggled( bool );

  public slots: // kmkernel, callback
    /**
     * Switch wordWrap on/off
     */
    void slotWordWrapToggled( bool );

  private slots:
    /**
     * Append signature file to the end of the text in the editor.
     */
    void slotAppendSignature();

    /**
     * Attach sender's public key.
     */
    void slotInsertMyPublicKey();

    /**
     * Insert arbitrary public key from public keyring in the editor.
     */
    void slotInsertPublicKey();

    /**
     * Enable/disable some actions in the Attach menu
     */
    void slotUpdateAttachActions();

    /**
     * Open a popup-menu in the attachments-listbox.
     */
    void slotAttachPopupMenu( QTreeWidgetItem* );

    /**
     * Attachment operations.
     */
    void slotAttachOpen();
    void slotAttachView();
    void slotAttachRemove();
    void slotAttachSave();
    void slotAttachProperties();

    /**
     * Select an email from the addressbook and add it to the line
     * the pressed button belongs to.
     */
    void slotAddrBookTo();
    void slotAddrBookFrom();
    void slotAddrBookReplyTo();
    void slotCleanSpace();
    void slotToggleMarkup();
    void toggleMarkup( bool markup );
    void htmlToolBarVisibilityChanged( bool visible );
    void slotSpellcheckDone( int result );
    void slotSpellcheckDoneClearStatus();

  public slots: // kmkernel
    void autoSaveMessage();

  private slots:
    void updateCursorPosition();
    void slotView();

    /**
     * Update composer field to reflect new identity
     */
    void slotIdentityChanged( uint );

    /**
     * KIO slots for attachment insertion
     */
    void slotAttachFileData( KIO::Job *, const QByteArray & );
    void slotAttachFileResult( KJob * );
    void slotListAction( const QString & );
    void slotFontAction( const QString & );
    void slotSizeAction( int );
    void slotAlignLeft();
    void slotAlignCenter();
    void slotAlignRight();
    void slotTextBold();
    void slotTextItalic();
    void slotTextUnder();
    void slotFormatReset();
    void slotTextColor();
    void fontChanged( const QFont & );
    void alignmentChanged( int );

  public: // kmkernel, attachmentlistview
    bool addAttach( const KUrl &url );

  public: // kmcommand
    /**
     * Add an attachment to the list.
     */
    void addAttach( KMMessagePart *msgPart );

  private:
    const KPIMIdentities::Identity & identity() const;
    uint identityUid() const;
    Kleo::CryptoMessageFormat cryptoMessageFormat() const;
    bool encryptToSelf() const;

  signals:
    void applyChangesDone( bool );
    void attachmentAdded( const KUrl &, bool success );

  private:
    /**
     * Applies the user changes to the message object of the composer
     * and signs/encrypts the message if activated. Returns false in
     * case of an error (e.g. if PGP encryption fails).
     * Disables the controls of the composer window unless @dontDisable
     * is true.
     */
    void applyChanges( bool dontSignNorEncrypt, bool dontDisable=false );

    /**
     * Install grid management and header fields. If fields exist that
     * should not be there they are removed. Those that are needed are
     * created if necessary.
     */
    void rethinkFields( bool fromslot=false );

    /**
      Connect signals for moving focus by arrow keys. Returns next edit.
    */
    QWidget *connectFocusMoving( QWidget *prev, QWidget *next );

    /**
     * Show or hide header lines
     */

    void rethinkHeaderLine( int aValue, int aMask, int &aRow,
                            const QString &aLabelStr, QLabel *aLbl,
                            QLineEdit *aEdt, QPushButton *aBtn = 0,
                            const QString &toolTip = QString(),
                            const QString &whatsThis = QString() );

    void rethinkHeaderLine( int value, int mask, int &row,
                            const QString &labelStr, QLabel *lbl,
                            QComboBox *cbx, QCheckBox *chk );

    /**
     * Initialization methods
     */
    void setupActions();
    void setupStatusBar();
    void setupEditor();

    /**
     * Header fields.
     */
    QString subject() const;
    QString to() const;
    QString cc() const;
    QString bcc() const;
    QString from() const;
    QString replyTo() const;

    /**
     * Use the given folder as sent-mail folder if the given folder exists.
     * Else show an error message and use the default sent-mail folder as
     * sent-mail folder.
     */
    void setFcc( const QString &idString );

    /**
     * Ask for confirmation if the message was changed before close.
     */
    virtual bool queryClose();

    /**
     * prevent kmail from exiting when last window is deleted (kernel rules)
     */
    virtual bool queryExit();

    /**
     * Open the attachment with the given index.
     */
    void openAttach( int index );

    /**
     * View the attachment with the given index.
     */
    void viewAttach( int index );

    /**
     * Remove an attachment from the list.
     */
    void removeAttach( const QString &url );
    void removeAttach( int idx );

    /**
     * Updates an item in the QListView to represnet a given message part
     */
    void msgPartToItem( const KMMessagePart *msgPart, KMAtmListViewItem *lvi,
                        bool loadDefaults = true );

    /**
     * Open addressbook and append selected addresses to the given edit field.
     */
    void addrBookSelInto();
    void addrBookSelIntoOld();
    void addrBookSelIntoNew();

  private:
    /**
     * Turn encryption on/off. If setByUser is true then a message box is shown
     * in case encryption isn't possible.
     */
    void setEncryption( bool encrypt, bool setByUser = false );

    /**
     * Turn signing on/off. If setByUser is true then a message box is shown
     * in case signing isn't possible.
     */
    void setSigning( bool sign, bool setByUser = false );

    /**
      Returns true if the user forgot to attach something.
    */
    bool userForgotAttachment();

    /**
     * Retrieve encrypt flag of an attachment
     * ( == state of it's check box in the attachments list view )
     */
    bool encryptFlagOfAttachment( int idx );

    /**
     * Retrieve sign flag of an attachment
     * ( == state of it's check box in the attachments list view )
     */
    bool signFlagOfAttachment( int idx );

    /**
     * Decrypt an OpenPGP block or strip off the OpenPGP envelope of a text
     * block with a clear text signature. This is only done if the given
     * string contains exactly one OpenPGP block.
     * This function is for example used to restore the unencrypted/unsigned
     * message text for editting.
     */
    static void decryptOrStripOffCleartextSignature( QByteArray & );

    /**
     * Save the message into the Drafts or Templates folder.
     */
    bool saveDraftOrTemplate( const QString &folderName, KMMessage *msg );

    /**
     * Send the message. Returns true if the message was sent successfully.
     */
    enum SaveIn {
      None,
      Drafts,
      Templates
    };

    void doSend( KMail::MessageSender::SendMethod method=KMail::MessageSender::SendDefault,
                 KMComposeWin::SaveIn saveIn = KMComposeWin::None );

    /**
     * Returns the autosave interval in milliseconds (as needed for QTimer).
     */
    int autoSaveInterval() const;

    /**
     * Initialize autosaving (timer and filename).
     */
    void initAutoSave();

    /**
     * Enables/disables autosaving depending on the value of the autosave
     * interval.
     */
    void updateAutoSave();

    /**
     * Stop autosaving and delete the autosaved message.
     */
    void cleanupAutoSave();

    /**
     * Validates a list of email addresses.
     * @return true if all addresses are valid.
     * @return false if one or several addresses are invalid.
     */
    static bool validateAddresses( QWidget *parent, const QString &addresses );

  private slots:
    /**
     * Compress an attachemnt with the given index
     */
    void compressAttach( KMAtmListViewItem *attachmentItem );
    void uncompressAttach( KMAtmListViewItem *attachmentItem );
    void editorFocusChanged( bool gained );

  private:
    QWidget   *mMainWidget;
    MailTransport::TransportComboBox *mTransport;
    KMail::DictionaryComboBox *mDictionaryCombo;
    KPIMIdentities::IdentityCombo    *mIdentity;
    KMFolderComboBox *mFcc;
    KMLineEdit *mEdtFrom, *mEdtReplyTo, *mEdtTo, *mEdtCc, *mEdtBcc;
    KMLineEditSpell *mEdtSubject;
    QLabel    *mLblIdentity, *mLblTransport, *mLblFcc;
    QLabel    *mLblFrom, *mLblReplyTo, *mLblTo, *mLblCc, *mLblBcc;
    QLabel    *mLblSubject;
    QLabel    *mDictionaryLabel;
    QCheckBox *mBtnIdentity, *mBtnTransport, *mBtnFcc;
    QPushButton *mBtnTo, *mBtnCc, *mBtnBcc, *mBtnReplyTo;
    bool mSpellCheckInProgress;
    bool mDone;
    bool mAtmModified;

    KMEdit *mEditor;
    QGridLayout *mGrid;
    KMMessage *mMsg;
    QVector<KMMessage*> mComposedMessages;
    KMail::AttachmentListView *mAtmListView;
    QList<KMAtmListViewItem*> mAtmItemList;
    QList<KMMessagePart*> mAtmList;
    QMenu *mAttachMenu;
    QAction *mOpenId, *mViewId, *mRemoveId, *mSaveAsId, *mPropertiesId;
    bool mAutoDeleteMsg;
    bool mSigningAndEncryptionExplicitlyDisabled;
    bool mLastSignActionState, mLastEncryptActionState;
    bool mLastIdentityHasSigningKey, mLastIdentityHasEncryptionKey;
    KMFolder *mFolder;
    long mShowHeaders;
    bool mConfirmSend;
    bool mDisableBreaking; // Move
    int mNumHeaders;
    bool mUseHTMLEditor;
    bool mHtmlMarkup;
    QFont mBodyFont, mFixedFont;
    QList<KTemporaryFile*> mAtmTempList;
    QPalette mPalette;
    uint mId;
    QString mOldSigText;

    KAction *mAttachPK, *mAttachMPK,
      *mAttachRemoveAction, *mAttachSaveAction, *mAttachPropertiesAction,
      *mPasteQuotation, *mAddQuoteChars, *mRemQuoteChars;
    KRecentFilesAction *mRecentAction;

    KToggleAction *mSignAction, *mEncryptAction, *mRequestMDNAction;
    KToggleAction *mUrgentAction, *mAllFieldsAction, *mFromAction;
    KToggleAction *mReplyToAction, *mToAction, *mCcAction, *mBccAction;
    KToggleAction *mSubjectAction;
    KToggleAction *mIdentityAction, *mTransportAction, *mFccAction;
    KToggleAction *mWordWrapAction, *mFixedFontAction, *mAutoSpellCheckingAction;
    KToggleAction *mDictionaryAction;

    KSelectAction *listAction;
    KFontAction *fontAction;
    KFontSizeAction *fontSizeAction;
    KToggleAction *alignLeftAction, *alignCenterAction, *alignRightAction;
    KToggleAction *textBoldAction, *textItalicAction, *textUnderAction;
    KToggleAction *plainTextAction, *markupAction;
    KAction *actionFormatColor, *actionFormatReset;
    KAction *mHtmlToolbar;

    KSelectAction *mEncodingAction;
    KSelectAction *mCryptoModuleAction;

    QByteArray mCharset;
    QByteArray mDefCharset;
    QStringList mCharsets;
    bool mAutoCharset;

    bool mAlwaysSend;

    QStringList mFolderNames;
    QList<QPointer<KMFolder> > mFolderList;
    QMap<KJob*, KUrl> mAttachJobs;
    KUrl::List mAttachFilesPending;
    int mAttachFilesSend;

  private:
    // helper method for slotInsert(My)PublicKey()
    void startPublicKeyExport();
    bool canSignEncryptAttachments() const {
      return cryptoMessageFormat() != Kleo::InlineOpenPGPFormat;
    }

    bool mSubjectTextWasSpellChecked;

    QString addQuotesToText( const QString &inputText ) const;
    QString removeQuotesFromText( const QString &inputText ) const;
    // helper method for rethinkFields
    int calcColumnWidth( int which, long allShowing, int width ) const;

  private slots:
    void slotCompletionModeChanged( KGlobalSettings::Completion );
    void slotConfigChanged();

    void slotComposerDone( bool );

    void slotContinueDoSend( bool );
    void slotContinuePrint( bool );
    void slotContinueAutoSave( bool );

    void slotEncryptChiasmusToggled( bool );

    /**
     * Helper method (you could call is a bottom-half :) for
     * startPublicKeyExport()
     */
    void slotPublicKeyExportResult( const GpgME::Error &err,
                                    const QByteArray &keydata );

    /**
     *  toggle automatic spellchecking
     */
    void slotAutoSpellCheckingToggled( bool );

  private:
    QColor mForeColor, mBackColor;
    QFont mSaveFont;
    QSplitter *mSplitter;
    struct atmLoadData
    {
      KUrl url;
      QByteArray data;
      bool insert;
      QByteArray encoding;
    };
    QMap<KIO::Job *, atmLoadData> mMapAtmLoadData;

    // These are for passing on methods over the applyChanges calls
    KMail::MessageSender::SendMethod mSendMethod;
    KMComposeWin::SaveIn mSaveIn;

    KToggleAction *mEncryptChiasmusAction;
    bool mEncryptWithChiasmus;

    // The temporary object that constructs the message out of the window
    MessageComposer *mComposer;

    // Temp var for slotPrint:
    bool mMessageWasModified;

    // Temp var for slotInsert(My)PublicKey():
    QString mFingerprint;

    // Temp ptr for saving image from clipboard
    KTempDir *mTempDir;

    bool mClassicalRecipients;

    RecipientsEditor *mRecipientsEditor;
    int mLabelWidth;

    QTimer *mAutoSaveTimer;
    QString mAutoSaveFilename;
    int mLastAutoSaveErrno; // holds the errno of the last try to autosave

    QMenu *mActNowMenu;
    QMenu *mActLaterMenu;

  /** If the message in this composer has a cursor position set (for
   *   instance because it comes from a template containing %CURSOR)
   *   then we need to preserve that cursor position even when auto-
   *   appending (or prepending) the signature during composer setup.
   *   Set to true *once* (and only in setMsg() at that) to avoid
   *   accidentally moving the cursor.
   */
  bool mPreserveUserCursorPosition;

  QString mdbusObjectPath;
  static int s_composerNumber;
};

#endif
