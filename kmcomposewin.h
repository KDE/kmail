/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMComposeWin
#define __KMComposeWin

#include "secondarywindow.h"

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

#include "kmmsgpart.h"
#include "kmmsgbase.h"
#include "mailcomposerIface.h"

#include <libkdepim/addresseelineedit.h>
#include <mimelib/mediatyp.h>

#include <kleo/enum.h>

class _StringPair {
 public:
   QString name;
   QString value;
};

class QCloseEvent;
class QComboBox;
class QFrame;
class QGridLayout;
class QListView;
class QPopupMenu;
class QPushButton;
class QCString;
class KCompletion;
class KEdit;
class KMComposeWin;
class KMFolderComboBox;
class KMMessage;
class KProcess;
class KDirWatch;
class KSelectAction;
class KFontAction;
class KFontSizeAction;
class KSelectAction;
class KSpell;
class KSpellConfig;
class KDictSpellingHighlighter;
class KStatusBar;
class KAction;
class KToggleAction;
class KTempFile;
class KToolBar;
class KToggleAction;
class KSelectColorAction;
class KURL;
class SpellingFilter;
class MessageComposer;

namespace KPIM {
  class IdentityCombo;
  class Identity;
}

namespace KMail {
  class AttachmentListView;
  class DictionaryComboBox;
}

namespace GpgME {
  class Error;
}


//-----------------------------------------------------------------------------
class KMEdit: public KEdit
{
  Q_OBJECT
public:
  KMEdit(QWidget *parent=0,KMComposeWin* composer=0,
         KSpellConfig* spellConfig = 0,
	 const char *name=0);
  virtual ~KMEdit();

  /**
   * Start the spell checker.
   */
  void spellcheck();

  /**
   * Text with lines breaks inserted after every row
   */
  QString brokenText();

   /**
   * Toggle automatic spellchecking
   */
  int autoSpellChecking( bool );

  /**
   * For the external editor
   */
  void setUseExternalEditor( bool use ) { mUseExtEditor = use; }
  void setExternalEditorPath( const QString & path ) { mExtEditor = path; }

  /**
   * Check that the external editor has finished and output a warning
   * if it hasn't.
   * @return false if the user chose to cancel whatever operation
   * called this method.
   */
  bool checkExternalEditorFinished();

  QPopupMenu* KMEdit::createPopupMenu(const QPoint&);
  void setSpellCheckingActive(bool spellCheckingActive);

  /** Drag and drop methods */
  void contentsDragEnterEvent(QDragEnterEvent *e);
  void contentsDragMoveEvent(QDragMoveEvent *e);
  void contentsDropEvent(QDropEvent *e);

  void initializeAutoSpellChecking( KSpellConfig* autoSpellConfig );
  void deleteAutoSpellChecking();

signals:
  void spellcheck_done(int result);
  void pasteImage();
public slots:
  void slotSpellcheck2(KSpell*);
  void slotSpellResult(const QString&);
  void slotSpellDone();
  void slotExternalEditorDone(KProcess*);
  void slotMisspelling(const QString &, const QStringList &, unsigned int);
  void slotCorrected (const QString &, const QString &, unsigned int);
  void addSuggestion(const QString& text, const QStringList& lst, unsigned int );
  virtual void cut();
  virtual void clear();
  virtual void del();
  virtual void paste();
protected:
  /**
   * Event filter that does Tab-key handling.
   */
  virtual bool eventFilter(QObject*, QEvent*);
  virtual void keyPressEvent( QKeyEvent* );

  KMComposeWin* mComposer;

private slots:
  void slotExternalEditorTempFileChanged( const QString & fileName );

private:
  void killExternalEditor();

private:
  KSpell *mKSpell;
  QMap<QString,QStringList> mReplacements;
  SpellingFilter* mSpellingFilter;
  KTempFile *mExtEditorTempFile;
  KDirWatch *mExtEditorTempFileWatcher;
  KProcess  *mExtEditorProcess;
  bool      mUseExtEditor;
  QString   mExtEditor;
  bool      mWasModifiedBeforeSpellCheck;
  KDictSpellingHighlighter *mSpellChecker;
  bool mSpellLineEdit;
};


//-----------------------------------------------------------------------------
class KMLineEdit : public KPIM::AddresseeLineEdit
{
    Q_OBJECT
public:
    KMLineEdit(KMComposeWin* composer, bool useCompletion, QWidget *parent = 0,
               const char *name = 0);
protected:
    // Inherited. Always called by the parent when this widget is created.
    virtual void loadContacts();

    virtual void keyPressEvent(QKeyEvent*);

    virtual QPopupMenu *createPopupMenu();

private slots:
    void editRecentAddresses();

private:
    KMComposeWin* mComposer;
};


class KMLineEditSpell : public KMLineEdit
{
    Q_OBJECT
public:
    KMLineEditSpell(KMComposeWin* composer, bool useCompletion, QWidget *parent = 0,
               const char *name = 0);
    void highLightWord( unsigned int length, unsigned int pos );
    void spellCheckDone( const QString &s );
    void spellCheckerMisspelling( const QString &text, const QStringList &, unsigned int pos);
    void spellCheckerCorrected( const QString &old, const QString &corr, unsigned int pos);

 signals:
  void subjectTextSpellChecked();
};


//-----------------------------------------------------------------------------
class KMAtmListViewItem : public QObject, public QListViewItem
{
  Q_OBJECT
  friend class KMComposeWin;
  friend class MessageComposer;

public:
  KMAtmListViewItem(QListView * parent);
  virtual ~KMAtmListViewItem();
  virtual void paintCell( QPainter * p, const QColorGroup & cg,
                          int column, int width, int align );

protected:
  void enableCryptoCBs(bool on);
  void setEncrypt(bool on);
  bool isEncrypt();
  void setSign(bool on);
  bool isSign();

private:
  QListView* mListview;
  QCheckBox* mCBEncrypt;
  QCheckBox* mCBSign;
  bool mCBSignEnabled, mCBEncryptEnabled;
};


class KMHeaders;

//-----------------------------------------------------------------------------
class KMComposeWin : public KMail::SecondaryWindow, virtual public MailComposerIface
{
  Q_OBJECT
  friend class KMHeaders;         // needed for the digest forward
  friend class MessageComposer;

public:
  KMComposeWin( KMMessage* msg=0, uint identity=0 );
  ~KMComposeWin();

  /**
   * From MailComposerIface
   */
  void send(int how);
  void addAttachment(KURL url,QString comment);
  void addAttachment(const QString &name,
                    const QCString &cte,
                    const QByteArray &data,
                    const QCString &type,
                    const QCString &subType,
                    const QCString &paramAttr,
                    const QString &paramValue,
                    const QCString &contDisp);
  void setBody (QString body);

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

  /**
   * Set the message the composer shall work with. This discards
   * previous messages without calling applyChanges() on them before.
   */
   void setMsg(KMMessage* newMsg, bool mayAutoSign=TRUE,
	       bool allowDecryption=FALSE, bool isModified=FALSE);

  /**
   * Returns message of the composer. To apply the user changes to the
   * message, call applyChanges() first.
   */
   KMMessage* msg(void) const { return mMsg; }

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

  /**
   * If this folder is set, the original message is inserted back after
   * cancelling
   */
   void setFolder(KMFolder* aFolder) { mFolder = aFolder; }

  /**
   * Recode to the specified charset
   */
   void setCharset(const QCString& aCharset, bool forceDefault = FALSE);

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

    KMLineEditSpell *sujectLineWidget() const { return mEdtSubject;}
  void setSubjectTextWasSpellChecked( bool _spell ) {
    mSubjectTextWasSpellChecked = _spell;
  }
  bool subjectTextWasSpellChecked() const { return mSubjectTextWasSpellChecked; }


  /** Disabled signing and encryption completely for this composer window. */
  void setSigningAndEncryptionDisabled( bool v )
  {
    mSigningAndEncryptionExplicitlyDisabled = v;
  }

public slots:
  void polish();
  /**
   * Actions:
   */
  void slotPrint();
  void slotAttachFile();
  void slotSendNow();
  void slotSendLater();
  /**
   * Returns true when saving was successful.
   */
  void slotSaveDraft();
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

  void slotMarkAll();

  void slotFolderRemoved(KMFolder*);

  /**
     Tell the composer to always send the message, even if the user
     hasn't changed the next. This is useful if a message is
     autogenerated (e.g., via a DCOP call), and the user should
     simply be able to confirm the message and send it.
  */
  void slotSetAlwaysSend( bool bAlwaysSend );

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
  void slotSelectCryptoModule();

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

  /**
   * Switch wordWrap on/off
   */
  void slotWordWrapToggled(bool);

  /**
   * Append signature file to the end of the text in the editor.
   */
  void slotAppendSignature();

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

//  void slotSpellConfigure();
  void slotSpellcheckDone(int result);
  void slotSpellcheckDoneClearStatus();

  /**
   * Append current message to ~/dead.letter
   */
  void deadLetter(void);

  void updateCursorPosition();

  void slotView();

  /**
   * Move focus to next/prev edit widget
   */
  void focusNextPrevEdit(const QWidget* current, bool next);

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

  void addAttach(const KURL url);

  /**
   * Add an attachment to the list.
   */
   void addAttach(const KMMessagePart* msgPart);

  /**
   * Add an image from the clipboard as attachment
   */
   void addImageFromClipboard();

public:
  const KPIM::Identity & identity() const;
  Kleo::CryptoMessageFormat cryptoMessageFormat() const;
  bool encryptToSelf() const;

signals:
  /**
   * A message has been queued or saved in the drafts folder
   */
  void messageQueuedOrDrafted();

  void applyChangesDone( bool );

protected:
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
   void removeAttach(const QString &url);
   void removeAttach(int idx);

   /**
    * Updates an item in the QListView to represnet a given message part
    */
   void msgPartToItem(const KMMessagePart* msgPart, KMAtmListViewItem *lvi);

  /**
   * Open addressbook and append selected addresses to the given
   * edit field.
   */
  void addrBookSelInto();

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
   * Send the message. Returns true if the message was sent successfully.
   */
  void doSend(int sendNow=-1, bool saveInDrafts = false);

protected:
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
  int mAtmEncryptColWidth;
  int mAtmSignColWidth;
  QPtrList<QListViewItem> mAtmItemList;
  QPtrList<KMMessagePart> mAtmList;
  QPopupMenu *mAttachMenu;
  int mOpenId, mViewId, mRemoveId, mSaveAsId, mPropertiesId;
  bool mAutoSign, mAutoPgpSign, mAutoPgpEncrypt, mAutoDeleteMsg;
  bool mNeverSignWhenSavingInDrafts, mNeverEncryptWhenSavingInDrafts;
  bool mSigningAndEncryptionExplicitlyDisabled;
  bool mAutoRequestMDN;
  bool mLastSignActionState, mLastEncryptActionState;
  bool mLastIdentityHasSigningKey, mLastIdentityHasEncryptionKey;
  KMFolder *mFolder;
  long mShowHeaders;
  QString mExtEditor;
  bool mUseExtEditor;
  QPtrList<_StringPair> mCustHeaders;
  bool mConfirmSend;
  bool mDisableBreaking; // Move
  int mNumHeaders;
  int mLineBreak;
  int mWordWrap;
  bool mUseFixedFont;
  bool mUseHTMLEditor;
  bool mHtmlMarkup;
  QFont mBodyFont, mFixedFont;
  //  QList<QLineEdit> mEdtList;
  QPtrList<QWidget> mEdtList;
  QPtrList<KTempFile> mAtmTempList;
  QPalette mPalette;
  uint mId;
  QString mOldSigText;
  QStringList mTransportHistory;

  KAction *mAttachPK, *mAttachMPK,
          *mAttachRemoveAction, *mAttachSaveAction, *mAttachPropertiesAction;

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

  QCString mCharset;
  QCString mDefCharset;
  QStringList mCharsets;
  bool mAutoCharset;

  bool mAlwaysSend;
  bool mOutlookCompatible;

  QStringList mFolderNames;
  QValueList<QGuardedPtr<KMFolder> > mFolderList;

private:
  // helper method for slotInsert(My)PublicKey()
  void startPublicKeyExport();
  bool canSignEncryptAttachments() const {
    return cryptoMessageFormat() != Kleo::InlineOpenPGPFormat;
  }

  bool mSubjectTextWasSpellChecked;

private slots:
  void slotCompletionModeChanged( KGlobalSettings::Completion );
  void slotConfigChanged();

  void slotComposerDone( bool );

  void slotContinueDoSend( bool );
  void slotContinuePrint( bool );
  void slotContinueDeadLetter( bool );

  /**
   * Helper method (you could call is a bottom-half :) for
   * startPublicKeyExport()
   */
  void slotPublicKeyExportResult( const GpgME::Error & err, const QByteArray & keydata );

  /**
   *  toggle automatic spellchecking
   */
  void slotAutoSpellCheckingToggled(bool);

private:
  QColor mForeColor,mBackColor;
  QFont mSaveFont;
  QSplitter *mSplitter;
  struct atmLoadData
  {
    KURL url;
    QByteArray data;
    bool insert;
    QCString encoding;
  };
  QMap<KIO::Job *, atmLoadData> mMapAtmLoadData;
  bool mForceReplyCharset;

  // These are for passing on methods over the applyChanges calls
  int mSendNow;
  bool mSaveInDrafts;

  // This is the temporary object that constructs the message out of the
  // window
  MessageComposer* mComposer;

  // Temp var for slotPrint:
  bool mMessageWasModified;

  // Temp var for slotInsert(My)PublicKey():
  QString mFingerprint;

  // Temp ptr for saving image from clipboard
  KTempDir *mTempDir;
};

#endif

