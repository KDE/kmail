/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMComposeWin
#define __KMComposeWin

#include "kmtopwidget.h"

#include <qlabel.h>
#include <qlistview.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qclipboard.h>
#include <qpalette.h>
#include <qfont.h>
#include <qptrlist.h>

#include <klineedit.h>
#include <kio/job.h>
#include <kglobalsettings.h>
#include <kdeversion.h>
#include <keditcl.h>

#include <kpgp.h>

#include "kmmsgpart.h"
#include "kmmsgbase.h"
#include "mailcomposerIface.h"

#include "cryptplugwrapper.h"
#include <kabc/addresslineedit.h>

class _StringPair {
 public:
   QString name;
   QString value;
};

class QCloseEvent;
class QComboBox;
class QFrame;
class QGridLayout;
class QLineEdit;
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
class KSpell;
class KSpellConfig;
class KDictSpellingHighlighter;
class KStatusBar;
class KAction;
class KToggleAction;
class KTempFile;
class KToolBar;
class KToggleAction;
class KURL;
class IdentityCombo;
class SpellingFilter;
class  CryptPlugWrapperList;

namespace KMail {
class AttachmentListView;
}

typedef QPtrList<KMMessagePart> KMMsgPartList;


//-----------------------------------------------------------------------------
class KMEdit: public KEdit
{
  Q_OBJECT
public:
  KMEdit(QWidget *parent=0,KMComposeWin* composer=0,
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


  /** Drag and drop methods */
  void contentsDragEnterEvent(QDragEnterEvent *e);
  void contentsDragMoveEvent(QDragMoveEvent *e);
  void contentsDropEvent(QDropEvent *e);

signals:
  void spellcheck_done(int result);
public slots:
  void slotAutoSpellCheckingToggled( bool );
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
  void initializeAutoSpellChecking();

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
class KMLineEdit : public KABC::AddressLineEdit
{
    Q_OBJECT
public:
    KMLineEdit(KMComposeWin* composer, bool useCompletion, QWidget *parent = 0,
               const char *name = 0);
protected:
    // Inherited. Always called by the parent when this widget is created.
    virtual void loadAddresses();
    /**
     * Smart insertion of email addresses. If @p pos is -1 then
     * @p str is inserted at the end of the current contents of this
     * lineedit. Else @p str is inserted at @p pos.
     * Features:
     * - Automatically adds ',' if necessary to separate email addresses
     * - Correctly decodes mailto URLs
     * - Recognizes email addresses which are protected against address
     *   harvesters, i.e. "name at kde dot org" and "name(at)kde.org"

    void smartInsert( const QString &str, int pos = -1 );
    virtual void dropEvent(QDropEvent *e);
*/

    virtual void keyPressEvent(QKeyEvent*);
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
};


//-----------------------------------------------------------------------------
class KMAtmListViewItem : public QObject, public QListViewItem
{
  Q_OBJECT
  friend class KMComposeWin;

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
};


class KMHeaders;

//-----------------------------------------------------------------------------
class KMComposeWin : public KMTopLevelWidget, virtual public MailComposerIface
{
  Q_OBJECT
  friend class KMHeaders;         // needed for the digest forward

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
   * Applies the user changes to the message object of the composer
   * and signs/encrypts the message if activated. Returns FALSE in
   * case of an error (e.g. if PGP encryption fails).
   * If backgroundMode is true then no functions which might require
   * user interaction (like signing/encrypting) are performed
   */
   bool applyChanges( bool backgroundMode = false );

  /**
   * Internal helper function called from applyChanges(void) to allow
   * processing several messages (encrypted or unencrypted) based on
   * the same composer content.
   * That's useful for storing decrypted versions of messages which
   * were sent in encrypted form.                  (khz, 2002/06/24)
   */
   Kpgp::Result composeMessage( QCString pgpUserId,
                                KMMessage& theMessage,
                                bool doSign,
                                bool doEncrypt,
                                bool ignoreBcc,
                                QCString& signCertFingerprint );

  /**
   * If this flag is set the message of the composer is deleted when
   * the composer is closed and the message was not sent. Default: FALSE
   */
   inline void setAutoDelete(bool f) { mAutoDeleteMsg = f; }

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
    * Tries to find the given mimetype @p type in the KDE Mimetype registry.
    * If found, returns its localized description, otherwise the @p type
    * in lowercase.
    */
   static QString prettyMimeType( const QString& type );
    QString quotePrefixName() const;

    KMLineEditSpell *sujectLineWidget() const { return mEdtSubject;}
public slots:
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
  bool slotSaveDraft();
  void slotNewComposer();
  void slotNewMailReader();
  void slotClose();
  void slotHelp();

  void slotFind();
  void slotReplace();
  void slotUndo();
  void slotRedo();
  void slotCut();
  void slotCopy();
  void slotPaste();
  void slotPasteAsQuotation();
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
   * Insert sender's public key block in the editor.
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

  void addAttach(const KURL url);

  /**
   * Add an attachment to the list.
   */
   void addAttach(const KMMessagePart* msgPart);

signals:
  /**
   * A message has been queued or saved in the drafts folder
   */
  void messageQueuedOrDrafted();

protected:
  /**
   * Install grid management and header fields. If fields exist that
   * should not be there they are removed. Those that are needed are
   * created if necessary.
   */
  void rethinkFields(bool fromslot=false);

  /**
   * Show or hide header lines
   */
  void rethinkHeaderLine(int value, int mask, int& row,
				 const QString& labelStr, QLabel* lbl,
				 QLineEdit* edt, QPushButton* btn=0);
  void rethinkHeaderLine(int value, int mask, int& row,
				 const QString& labelStr, QLabel* lbl,
				 QComboBox* cbx, QCheckBox *chk);

  /**
   * Initialization methods
   */
  void setupActions();
  void setupStatusBar();
  void setupEditor();


  /**
   * Header fields.
   */
    QString subject(void) const { return mEdtSubject->text(); }
    QString to(void) const { return mEdtTo->text(); }
    QString cc(void) const
   { return (mEdtCc->isHidden()) ? QString::null : mEdtCc->text(); }
    QString bcc(void) const
   { return (mEdtBcc->isHidden()) ? QString::null : mEdtBcc->text(); }
    QString from(void) const { return mEdtFrom->text(); }
    QString replyTo(void) const { return mEdtReplyTo->text(); }

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
   * Get message ready for sending or saving.
   * This must be done _before_ signing and/or encrypting it.
   *
   */
  QCString breakLinesAndApplyCodec();

  /**
   * Get signature for a message.
   * To build nice S/MIME objects signing and encoding must be separeted.
   *
   */
  QByteArray pgpSignedMsg( QCString cText,
                           StructuringInfoWrapper& structuring,
                           QCString& signCertFingerprint );

  /**
   * Get encrypted message.
   * To build nice S/MIME objects signing and encrypting must be separat.
   *
   */
  Kpgp::Result pgpEncryptedMsg( QByteArray & rEncryptedBody,
				QCString cText, const QStringList& recipients,
				StructuringInfoWrapper& structuring,
				QCString& encryptCertFingerprints );

  /**
   * Get encryption certificate for a recipient (the Aegypten way).
   */
  QCString getEncryptionCertificate( const QString& recipient );

  /**
   * Check for expiry of various certificates.
   */
  bool checkForEncryptCertificateExpiry( const QString& recipient,
                                         const QCString& certFingerprint );

  /**
   * Build a MIME object (or a flat text resp.) based upon
   * structuring information returned by a crypto plugin that was
   * called via pgpSignedMsg() (or pgpEncryptedMsg(), resp.).
   *
   * NOTE: The c string representation of the MIME object (or the
   *       flat text, resp.) is returned in resultingData, so just
   *       use this string as body text of the surrounding MIME object.
   *       This string *is* encoded according to contentTEncClear
   *       and thus should be ready for neing sended via SMTP.
   */
  bool processStructuringInfo( const QString   bugURL,
                               uint            boundaryLevel,
                               const QString   contentDescriptionClear,
                               const QCString  contentTypeClear,
                               const QCString  contentSubtypeClear,
                               const QCString  contentDispClear,
                               const QCString  contentTEncClear,
                               const QCString& bodytext,
                               const QString   contentDescriptionCiph,
                               const QByteArray& ciphertext,
                               const StructuringInfoWrapper& structuring,
                               KMMessagePart&  resultingPart );

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


  Kpgp::Result encryptMessage( KMMessage* msg,
                       const QStringList& recipients, bool doSign, bool doEncrypt,
                       const QCString& encodedBody,int previousBoundaryLevel,
                       const KMMessagePart& oldBodyPart,
                       bool earlyAddAttachments, bool allAttachmentsAreInBody,
                       KMMessagePart newBodyPart,
                       QCString& signCertFingerprint );

  /**
   * Decrypt an OpenPGP block or strip off the OpenPGP envelope of a text
   * block with a clear text signature. This is only done if the given
   * string contains exactly one OpenPGP block.
   * This function is for example used to restore the unencrypted/unsigned
   * message text for editting.
   */
   static void decryptOrStripOffCleartextSignature( QCString& );

  /**
   * Get message including signing and encrypting it
   */
  QCString pgpProcessedMsg(void);

  /**
   * Send the message. Returns true if the message was sent successfully.
   */
  bool doSend(int sendNow=-1, bool saveInDrafts = false);

protected:
  QWidget   *mMainWidget;
  QComboBox *mTransport;
  IdentityCombo    *mIdentity;
  KMFolderComboBox *mFcc;
  KMLineEdit *mEdtFrom, *mEdtReplyTo, *mEdtTo, *mEdtCc, *mEdtBcc;
  KMLineEditSpell *mEdtSubject;
  QLabel    *mLblIdentity, *mLblTransport, *mLblFcc;
  QLabel    *mLblFrom, *mLblReplyTo, *mLblTo, *mLblCc, *mLblBcc, *mLblSubject;
  QCheckBox *mBtnIdentity, *mBtnTransport, *mBtnFcc;
  QPushButton *mBtnTo, *mBtnCc, *mBtnBcc, *mBtnFrom, *mBtnReplyTo;
  bool mSpellCheckInProgress;
  bool mDone;
  bool mAtmModified;

  KMEdit* mEditor;
  QGridLayout* mGrid;
  KMMessage *mMsg;
  QPtrList<KMMessage> mBccMsgList;
  KMail::AttachmentListView* mAtmListView;
  int mAtmColEncrypt;
  int mAtmColSign;
  int mAtmEncryptColWidth;
  int mAtmSignColWidth;
  QPtrList<QListViewItem> mAtmItemList;
  KMMsgPartList mAtmList;
  QPopupMenu *mAttachMenu;
  int mSaveAsId, mPropertiesId;
  bool mAutoSign, mAutoPgpSign, mAutoPgpEncrypt, mAutoDeleteMsg;
  bool mAutoRequestMDN;
  bool mLastSignActionState, mLastEncryptActionState;
  bool mLastIdentityHasOpenPgpKey;
  KMFolder *mFolder;
  long mShowHeaders;
  QString mExtEditor;
  bool mUseExtEditor;
  QPtrList<_StringPair> mCustHeaders;
  bool mConfirmSend;
  bool mDisableBreaking;
  int mNumHeaders;
  int mLineBreak;
  int mWordWrap;
  short mBtnIdSign, mBtnIdEncrypt;
  short mMnuIdUrgent, mMnuIdConfDeliver, mMnuIdConfRead;
  QFont mBodyFont, mFixedFont;
  //  QList<QLineEdit> mEdtList;
  QPtrList<QWidget> mEdtList;
  QPtrList<KTempFile> mAtmTempList;
  QPalette mPalette;
  uint mId;
  QString mOldSigText;
  QStringList mTransportHistory;
  QString mBcc;

  KAction *mAttachPK, *mAttachMPK,
          *mAttachRemoveAction, *mAttachSaveAction, *mAttachPropertiesAction;

  KToggleAction *mSignAction, *mEncryptAction, *mRequestMDNAction;
  KToggleAction *mUrgentAction, *mAllFieldsAction, *mFromAction;
  KToggleAction *mReplyToAction, *mToAction, *mCcAction, *mBccAction;
  KToggleAction *mSubjectAction;
  KToggleAction *mIdentityAction, *mTransportAction, *mFccAction;
  KToggleAction *mWordWrapAction, *mFixedFontAction, *mAutoSpellCheckingAction;

  KSelectAction *mEncodingAction;
  KSelectAction *mCryptoModuleAction;

  QCString mCharset;
  QCString mDefCharset;
  QStringList mCharsets;
  bool mAutoCharset;

  bool mAlwaysSend;

  QStringList mFolderNames;
  QValueList<QGuardedPtr<KMFolder> > mFolderList;

private slots:
  void slotCompletionModeChanged( KGlobalSettings::Completion );

private:
  QColor mForeColor,mBackColor;
  struct atmLoadData
  {
    KURL url;
    QByteArray data;
    bool insert;
    QCString encoding;
  };
  QMap<KIO::Job *, atmLoadData> mMapAtmLoadData;
  bool mForceReplyCharset;

  QString mErrorProcessingStructuringInfo;
  QString mErrorNoCryptPlugAndNoBuildIn;

  /**
   * Store the cryptplug that was selected for signing and/or encrypting.
   *
   * This is either the 'active' cryptplug of the global wrapper list
   * or another plugin (or 0 if none selected) if the user decided to
   * override the global setting using KDComposeWin's options_select_crypto action.
   */
  CryptPlugWrapper* mSelectedCryptPlug;

public:
  bool mDebugComposerCrypto;
  bool mNeverSign;     // use this to completely disable signing and warnings
  bool mNeverEncrypt;  // use this to completely disable encrypting and warnings
};
#endif

