/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMComposeWin
#define __KMComposeWin

#ifndef KDE_USE_FINAL
# ifndef REALLY_WANT_KMCOMPOSEWIN_H
#  error Do not include kmcomposewin.h anymore. Include composer.h instead.
# endif
#endif

#include "composer.h"
#include "messagesender.h"

#include <set>

#include <qlabel.h>
#include <qlistview.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qclipboard.h>
#include <qpalette.h>
#include <qfont.h>
#include <qptrlist.h>
#include <qvaluevector.h>
#include <qsplitter.h>

#include <kio/job.h>
#include <kglobalsettings.h>
#include <kdeversion.h>
#include <keditcl.h>
#include <ktempdir.h>

#include "mailcomposerIface.h"

#include <libkdepim/addresseelineedit.h>
#include <mimelib/mediatyp.h>

#include <kleo/enum.h>

class QCloseEvent;
class QComboBox;
class QFrame;
class QGridLayout;
class QListView;
class QPopupMenu;
class QPushButton;
class QCString;
class KCompletion;
class KMEdit;
class KMComposeWin;
class KMFolderComboBox;
class KMFolder;
class KMMessage;
class KMMessagePart;
class KProcess;
class KDirWatch;
class KSelectAction;
class KFontAction;
class KFontSizeAction;
class KSelectAction;
class KStatusBar;
class KAction;
class KToggleAction;
class KTempFile;
class KToolBar;
class KToggleAction;
class KSelectColorAction;
class KURL;
class KRecentFilesAction;
class SpellingFilter;
class MessageComposer;
class RecipientsEditor;
class KMLineEdit;
class KMLineEditSpell;
class KMAtmListViewItem;
class SnippetWidget;

namespace KPIM {
  class IdentityCombo;
  class Identity;
}

namespace KMail {
  class AttachmentListView;
  class DictionaryComboBox;
  class EditorWatcher;
}

namespace GpgME {
  class Error;
}

//-----------------------------------------------------------------------------
class KMComposeWin : public KMail::Composer, virtual public MailComposerIface
{
  Q_OBJECT
  friend class ::KMEdit;
  friend class ::MessageComposer;

private: // mailserviceimpl, kmkernel, kmcommands, callback, kmmainwidget
  KMComposeWin( KMMessage* msg=0, uint identity=0 );
  ~KMComposeWin();
public:
  static Composer * create( KMMessage * msg = 0, uint identity = 0 );

  MailComposerIface * asMailComposerIFace() { return this; }
  const MailComposerIface * asMailComposerIFace() const { return this; }

public: // mailserviceimpl
  /**
   * From MailComposerIface
   */
  void send(int how);
  void addAttachmentsAndSend(const KURL::List &urls, const QString &comment, int how);
  void addAttachment(KURL url,QString comment);
  void addAttachment(const QString &name,
                    const QCString &cte,
                    const QByteArray &data,
                    const QCString &type,
                    const QCString &subType,
                    const QCString &paramAttr,
                    const QString &paramValue,
                    const QCString &contDisp);
public: // kmcommand
  void setBody (QString body);

private:
  /**
   * To catch palette changes
   */
  virtual bool event(QEvent *e);

  /**
   * update colors
   */
  void readColorConfig();

  /**
   * Write settings to app's config file.
   */
   void writeConfig(void);

  /**
   * If necessary increases the word wrap of the editor so that it will
   * not wrap the body string
   */
   void verifyWordWrapLengthIsAdequate(const QString&);

public: // kmkernel, kmcommands, callback
  /**
   * Set the message the composer shall work with. This discards
   * previous messages without calling applyChanges() on them before.
   */
   void setMsg(KMMessage* newMsg, bool mayAutoSign=TRUE,
	       bool allowDecryption=FALSE, bool isModified=FALSE);

   void disableWordWrap();

private: // kmedit
  /**
   * Returns message of the composer. To apply the user changes to the
   * message, call applyChanges() first.
   */
   KMMessage* msg() const { return mMsg; }

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
   * the composer is closed and the message was not sent. Default: FALSE
   */
   inline void setAutoDelete(bool f) { mAutoDeleteMsg = f; }

  /**
   * If this flag is set, the compose window will delete itself after
   * the window has been closed.
   */
  void setAutoDeleteWindow( bool f );

public: // kmcommand
  /**
   * If this folder is set, the original message is inserted back after
   * cancelling
   */
   void setFolder(KMFolder* aFolder) { mFolder = aFolder; }
public: // kmkernel, kmcommand, mailserviceimpl
  /**
   * Recode to the specified charset
   */
   void setCharset(const QCString& aCharset, bool forceDefault = FALSE);

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
   static QString prettyMimeType( const QString& type );
    QString quotePrefixName() const;

private: // kmedit:
  KMLineEditSpell *sujectLineWidget() const { return mEdtSubject;}
  void setSubjectTextWasSpellChecked( bool _spell ) {
    mSubjectTextWasSpellChecked = _spell;
  }
  bool subjectTextWasSpellChecked() const { return mSubjectTextWasSpellChecked; }

  void paste( QClipboard::Mode mode );

public: // callback
  /** Disabled signing and encryption completely for this composer window. */
  void setSigningAndEncryptionDisabled( bool v )
  {
    mSigningAndEncryptionExplicitlyDisabled = v;
  }

private slots:
  void polish();
  /**
   * Actions:
   */
  void slotPrint();
  void slotAttachFile();
  void slotInsertRecentFile(const KURL&);
  void slotAttachedFile(const KURL&);
public slots: // kmkernel, callback
  void slotSendNow();
private slots:
  void slotSendNowVia( int item );
  void slotSendLater();
  void slotSendLaterVia( int item );

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
  void slotPasteClipboard();
  void slotPasteClipboardAsQuotation();
  void slotPasteClipboardAsAttachment();
  void slotAddQuotes();
  void slotRemoveQuotes();
  void slotAttachPNGImageData(const QByteArray &image);

  void slotMarkAll();

  void slotFolderRemoved(KMFolder*);

  void slotEditDone( KMail::EditorWatcher* watcher );

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
  void slotStatusMessage(const QString &message);
  void slotEditToolbars();
  void slotUpdateToolbars();
  void slotEditKeys();
  /**
   * Read settings from app's config file.
   */
  void readConfig(void);
  /**
   * Change window title to given string.
   */
  void slotUpdWinTitle(const QString& );

  /**
   * Switch the icon to lock or unlock respectivly.
   * Change states of all encrypt check boxes in the attachments listview
   */
  void slotEncryptToggled(bool);

  /**
   * Change states of all sign check boxes in the attachments listview
   */
  void slotSignToggled(bool);

public slots: // kmkernel, callback
  /**
   * Switch wordWrap on/off
   */
  void slotWordWrapToggled(bool);

private slots:
  /**
   * Append signature file to the end of the text in the editor.
   */
  void slotAppendSignature();

  /**
   * Prepend signature file at the beginning of the text in the editor.
   */
  void slotPrependSignature();

  /**
   * Insert signature file at the cursor position of the text in the editor.
   */
  void slotInsertSignatureAtCursor();

  /**
   * Attach sender's public key.
   */
  void slotInsertMyPublicKey();

  /**
   * Insert arbitary public key from public keyring in the editor.
   */
  void slotInsertPublicKey();

  /**
   * Enable/disable some actions in the Attach menu
   */
  void slotUpdateAttachActions();

  /**
   * Open a popup-menu in the attachments-listbox.
   */
  void slotAttachPopupMenu(QListViewItem *, const QPoint &, int);

  /**
   * Returns the number of the current attachment in the listbox,
   * or -1 if there is no current attachment
   */
  int currentAttachmentNum();

  /**
   * Attachment operations.
   */
  void slotAttachOpen();
  void slotAttachView();
  void slotAttachRemove();
  void slotAttachSave();
  void slotAttachProperties();
  void slotAttachOpenWith();
  void slotAttachEdit();
  void slotAttachEditWith();
  void slotAttachmentDragStarted();

  /**
   * Select an email from the addressbook and add it to the line
   * the pressed button belongs to.
   */
  void slotAddrBookTo();
  void slotAddrBookFrom();
  void slotAddrBookReplyTo();

  void slotCleanSpace();

  void slotToggleMarkup();
  void toggleMarkup(bool markup);
  void htmlToolBarVisibilityChanged( bool visible );

//  void slotSpellConfigure();
  void slotSpellcheckDone(int result);
  void slotSpellcheckDoneClearStatus();

public slots: // kmkernel
  void autoSaveMessage();

private slots:
  void updateCursorPosition();

  void slotView();

  /**
   * Update composer field to reflect new identity
   */
  void slotIdentityChanged(uint);

  /**
   * KIO slots for attachment insertion
   */
  void slotAttachFileData(KIO::Job *, const QByteArray &);
  void slotAttachFileResult(KIO::Job *);

  void slotListAction(const QString &);
  void slotFontAction(const QString &);
  void slotSizeAction(int);
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
  bool addAttach(const KURL url);

public: // kmcommand
  /**
   * Add an attachment to the list.
   */
  void addAttach(const KMMessagePart* msgPart);

private:
  const KPIM::Identity & identity() const;
  uint identityUid() const;
  Kleo::CryptoMessageFormat cryptoMessageFormat() const;
  bool encryptToSelf() const;

signals:
  void applyChangesDone( bool );
  void attachmentAdded( const KURL&, bool success );

private:
  /**
   * Applies the user changes to the message object of the composer
   * and signs/encrypts the message if activated. Returns FALSE in
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
  void rethinkFields(bool fromslot=false);

  /**
    Connect signals for moving focus by arrow keys. Returns next edit.
  */
  QWidget *connectFocusMoving( QWidget *prev, QWidget *next );

  /**
   * Show or hide header lines
   */

  void rethinkHeaderLine( int aValue, int aMask, int& aRow,
                          const QString &aLabelStr, QLabel* aLbl,
                          QLineEdit* aEdt, QPushButton* aBtn = 0,
                          const QString &toolTip = QString::null,
                          const QString &whatsThis = QString::null );

  void rethinkHeaderLine( int value, int mask, int& row,
                          const QString& labelStr, QLabel* lbl,
                          QComboBox* cbx, QCheckBox *chk );

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
  virtual bool queryClose ();
  /**
   * prevent kmail from exiting when last window is deleted (kernel rules)
  */
  virtual bool queryExit ();

  /**
   * Open the attachment with the given index and with ("Open with")
   */
  void openAttach( int index, bool with );

  /**
   * View the attachment with the given index.
   */
  void viewAttach( int index );

  /**
    Edit the attachment with the given index.
  */
  void editAttach( int index, bool openWith );

  /**
   * Remove an attachment from the list.
   */
   void removeAttach(const QString &url);
   void removeAttach(int idx);

   /**
    * Updates an item in the QListView to represnet a given message part
    */
   void msgPartToItem(const KMMessagePart* msgPart, KMAtmListViewItem *lvi,
        bool loadDefaults = true );

  /**
   * Open addressbook and append selected addresses to the given
   * edit field.
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
  bool encryptFlagOfAttachment(int idx);

  /**
   * Retrieve sign flag of an attachment
   * ( == state of it's check box in the attachments list view )
   */
  bool signFlagOfAttachment(int idx);


  /**
   * Decrypt an OpenPGP block or strip off the OpenPGP envelope of a text
   * block with a clear text signature. This is only done if the given
   * string contains exactly one OpenPGP block.
   * This function is for example used to restore the unencrypted/unsigned
   * message text for editting.
   */
  static void decryptOrStripOffCleartextSignature( QCString& );

  /**
   * Save the message into the Drafts or Templates folder.
   */
  bool saveDraftOrTemplate( const QString &folderName, KMMessage *msg );

  /**
   * Send the message. Returns true if the message was sent successfully.
   */
  enum SaveIn { None, Drafts, Templates };
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
  static bool validateAddresses( QWidget * parent, const QString & addresses );

  /**
   * Sets the transport combobox to @p transport. If @p transport is empty
   * then the combobox remains unchanged. If @p transport is neither a known transport
   * nor a custom transport then the combobox is set to the default transport.
   * @param transport the transport the combobox should be set to
   */
  void setTransport( const QString & transport );

  /**
   * Helper to insert the signature of the current identy at the
   * beginning or end of the editor.
   */
  void insertSignature( bool append = true, int pos = 0 );
private slots:
   /**
    * Compress an attachemnt with the given index
    */
    void compressAttach(int idx);
    void uncompressAttach(int idx);
    void editorFocusChanged(bool gained);
    void recipientEditorSizeHintChanged();
    void setMaximumHeaderSize();

private:
  QWidget   *mMainWidget;
  QComboBox *mTransport;
  KMail::DictionaryComboBox *mDictionaryCombo;
  KPIM::IdentityCombo    *mIdentity;
  KMFolderComboBox *mFcc;
  KMLineEdit *mEdtFrom, *mEdtReplyTo, *mEdtTo, *mEdtCc, *mEdtBcc;
  KMLineEditSpell *mEdtSubject;
  QLabel    *mLblIdentity, *mLblTransport, *mLblFcc;
  QLabel    *mLblFrom, *mLblReplyTo, *mLblTo, *mLblCc, *mLblBcc, *mLblSubject;
  QLabel    *mDictionaryLabel;
  QCheckBox *mBtnIdentity, *mBtnTransport, *mBtnFcc;
  QPushButton *mBtnTo, *mBtnCc, *mBtnBcc, /* *mBtnFrom, */ *mBtnReplyTo;
  bool mSpellCheckInProgress;
  bool mDone;
  bool mAtmModified;

  KMEdit* mEditor;
  QGridLayout* mGrid;
  KMMessage *mMsg;
  QValueVector<KMMessage*> mComposedMessages;
  KMail::AttachmentListView* mAtmListView;
  int mAtmColEncrypt;
  int mAtmColSign;
  int mAtmColCompress;
  int mAtmEncryptColWidth;
  int mAtmSignColWidth;
  int mAtmCompressColWidth;
  QPtrList<QListViewItem> mAtmItemList;
  QPtrList<KMMessagePart> mAtmList;
  QPopupMenu *mAttachMenu;
  int mOpenId, mOpenWithId, mViewId, mRemoveId, mSaveAsId, mPropertiesId, mEditId, mEditWithId;
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
  QPtrList<KTempFile> mAtmTempList;
  QPalette mPalette;
  uint mId;
  QString mOldSigText;

  KAction *mAttachPK, *mAttachMPK,
          *mAttachRemoveAction, *mAttachSaveAction, *mAttachPropertiesAction,
          *mPasteQuotation, *mAddQuoteChars, *mRemQuoteChars;
  KRecentFilesAction *mRecentAction;

  KAction *mAppendSignatureAction, *mPrependSignatureAction, *mInsertSignatureAction;

  KToggleAction *mSignAction, *mEncryptAction, *mRequestMDNAction;
  KToggleAction *mUrgentAction, *mAllFieldsAction, *mFromAction;
  KToggleAction *mReplyToAction, *mToAction, *mCcAction, *mBccAction;
  KToggleAction *mSubjectAction;
  KToggleAction *mIdentityAction, *mTransportAction, *mFccAction;
  KToggleAction *mWordWrapAction, *mFixedFontAction, *mAutoSpellCheckingAction;
  KToggleAction *mDictionaryAction, *mSnippetAction;

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

  QCString mCharset;
  QCString mDefCharset;
  QStringList mCharsets;
  bool mAutoCharset;

  bool mAlwaysSend;

  QStringList mFolderNames;
  QValueList<QGuardedPtr<KMFolder> > mFolderList;
  QMap<KIO::Job*, KURL> mAttachJobs;
  KURL::List mAttachFilesPending;
  int mAttachFilesSend;

private:
  // helper method for slotInsert(My)PublicKey()
  void startPublicKeyExport();
  bool canSignEncryptAttachments() const {
    return cryptoMessageFormat() != Kleo::InlineOpenPGPFormat;
  }

  bool mSubjectTextWasSpellChecked;

  QString addQuotesToText( const QString &inputText );
  QString removeQuotesFromText( const QString &inputText );
  // helper method for rethinkFields
  int calcColumnWidth(int which, long allShowing, int width);

private slots:
  void slotCompletionModeChanged( KGlobalSettings::Completion );
  void slotConfigChanged();

  void slotComposerDone( bool );

  void slotContinueDoSend( bool );
  void slotContinuePrint( bool );
  void slotContinueAutoSave();

  void slotEncryptChiasmusToggled( bool );

  /**
   * Helper method (you could call is a bottom-half :) for
   * startPublicKeyExport()
   */
  void slotPublicKeyExportResult( const GpgME::Error & err, const QByteArray & keydata );

  /**
   *  toggle automatic spellchecking
   */
  void slotAutoSpellCheckingToggled(bool);

  /**
   *  Updates signature actions when identity changes.
   */
  void slotUpdateSignatureActions();

  /**
   * Updates the visibility and text of the signature and encryption state indicators.
   */
  void slotUpdateSignatureAndEncrypionStateIndicators();
private:
  QColor mForeColor,mBackColor;
  QFont mSaveFont;
  QSplitter *mHeadersToEditorSplitter;
  QWidget* mHeadersArea;
  QSplitter *mSplitter;
  QSplitter *mSnippetSplitter;
  struct atmLoadData
  {
    KURL url;
    QByteArray data;
    bool insert;
    QCString encoding;
  };
  QMap<KIO::Job *, atmLoadData> mMapAtmLoadData;

  // These are for passing on methods over the applyChanges calls
  KMail::MessageSender::SendMethod mSendMethod;
  KMComposeWin::SaveIn mSaveIn;

  KToggleAction *mEncryptChiasmusAction;
  bool mEncryptWithChiasmus;

  // This is the temporary object that constructs the message out of the
  // window
  MessageComposer* mComposer;

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

  QPopupMenu *mActNowMenu;
  QPopupMenu *mActLaterMenu;

  QMap<KMail::EditorWatcher*, KMMessagePart*> mEditorMap;
  QMap<KMail::EditorWatcher*, KTempFile*> mEditorTempFiles;

  QLabel *mSignatureStateIndicator;
  QLabel *mEncryptionStateIndicator;

  SnippetWidget *mSnippetWidget;
  std::set<KTempDir*> mTempDirs;
};

#endif

