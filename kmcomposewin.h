/* KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMComposeWin
#define __KMComposeWin

#include "kmtopwidget.h"

#include <qlabel.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qclipboard.h>
#include <qpalette.h>
#include <qfont.h>

#include <keditcl.h>
#include <klineedit.h>
#include <kio/job.h>
#include <kglobalsettings.h>

#include "kmmsgpart.h"
#include "kmmsgbase.h"
#include "mailcomposerIface.h"

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
class QListViewItem;
class QPopupMenu;
class QPushButton;
class KCompletion;
class KEdit;
class KMComposeWin;
class KMMessage;
class KProcess;
class KSelectAction;
class KSpell;
class KSpellConfig;
class KStatusBar;
class KTempFile;
class KToolBar;
class KToggleAction;
class KURL;

typedef QPtrList<KMMessagePart> KMMsgPartList;

/* default key to toggle between fixed width font and default font */
#define DEFAULT_FIXEDFONTS_KEY (CTRL+Key_E)

//-----------------------------------------------------------------------------
#define KMEditInherited KEdit
class KMEdit: public KEdit
{
  Q_OBJECT
public:
  KMEdit(QWidget *parent=0L,KMComposeWin* composer=0L,
	 const char *name=0L);
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
  inline void setExternalEditor(bool extEd) { extEditor=extEd; }
  inline void setExternalEditorPath(QString path) { mExtEditor=path; }

signals:
  void spellcheck_done();
public slots:
  void slotSpellcheck2(KSpell*);
  void slotSpellResult(const QString &newtext);
  void slotSpellDone();
  void slotExternalEditorDone(KProcess*);

protected:
  /**
   * Event filter that does Tab-key handling.
   */
  virtual bool eventFilter(QObject*, QEvent*);

  KMComposeWin* mComposer;
private:
  KSpell *mKSpell;
  KTempFile *mTempFile;
  KProcess  *mExtEditorProcess;
  bool      extEditor;
  QString   mExtEditor;
};


//-----------------------------------------------------------------------------
#define KMLineEditInherited KLineEdit
class KMLineEdit : public KLineEdit
{
  Q_OBJECT

public:
  KMLineEdit(KMComposeWin* composer, bool useCompletion, QWidget *parent = 0L,
             const char *name = 0L);
  virtual ~KMLineEdit();

  virtual void setFont( const QFont& );

public slots:
  void undo();
  /**
   * Set cursor to end of line.
   */
  void cursorAtEnd();

protected:
  virtual bool eventFilter(QObject*, QEvent*);
  virtual void dropEvent(QDropEvent *e);
  void doCompletion(bool ctrlT);
  KMComposeWin* mComposer;

private slots:
  void slotCompletion() { doCompletion(false); }
  void slotPopupCompletion( const QString& );

private:
  void loadAddresses();

  QString m_previousAddresses;
  bool m_useCompletion;

  static bool s_addressesDirty;
  static KCompletion *s_completion;

};


class KMHeaders;

//-----------------------------------------------------------------------------
class KMComposeWin : public KMTopLevelWidget, virtual public MailComposerIface
{
  Q_OBJECT
  friend class KMHeaders;         // needed for the digest forward

public:
  KMComposeWin(KMMessage* msg=0L, QString id = "unknown");
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
   void setMsg(KMMessage* newMsg, bool mayAutoSign=TRUE, bool allowDecryption=FALSE);

  /**
   * Returns message of the composer. To apply the user changes to the
   * message, call applyChanges() first.
   */
   KMMessage* msg(void) const { return mMsg; }

  /**
   * Applies the user changes to the message object of the composer
   * and signs/encrypts the message if activated. Returns FALSE in
   * case of an error (e.g. if PGP encryption fails).
   */
   bool applyChanges(void);

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

public slots:
  /**
   * Actions:
   */
  void slotPrint();
  void slotAttachFile();
  void slotSendNow();
  void slotSendLater();
  void slotSaveDraft();
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
  void slotMarkAll();
  
  /**
   * toggle fixed width font.
   */
  void slotToggleFixedFont();

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
   * XML-GUI stuff
   */
  void slotToggleToolBar();
  void slotToggleStatusBar();
  void slotEditToolbars();
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
   */
  void slotEncryptToggled(bool);

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
  void slotAddrBookCc();
  void slotAddrBookBcc();
  void slotAddrBookFrom();
  void slotAddrBookReplyTo();

  void slotCleanSpace();


//  void slotSpellConfigure();
  void slotSpellcheckDone();

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
  void slotIdentityActivated(int idx);

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
				 QLineEdit* edt, QPushButton* btn=NULL);
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
   { return (mEdtCc->isHidden()) ? QString() : mEdtCc->text(); }
    QString bcc(void) const
   { return (mEdtBcc->isHidden()) ? QString() : mEdtBcc->text(); }
    QString from(void) const { return mEdtFrom->text(); }
    QString replyTo(void) const { return mEdtReplyTo->text(); }

  /**
   * Ask for confirmation if the message was changed before close.
   */
  virtual bool queryClose ();
  /**
   * prevent kmail from exiting when last window is deleted (kernel rules)
  */
  virtual bool queryExit ();

  /**
   * Remove an attachment from the list.
   */
   void removeAttach(const QString &url);
   void removeAttach(int idx);

   /**
    * Updates an item in the QListView to represnet a given message part
    */
   void msgPartToItem(const KMMessagePart* msgPart, QListViewItem *lvi);

  /**
   * Open addressbook and append selected addresses to the given
   * edit field.
   */
   void addrBookSelInto(KMLineEdit* destEdit);

private:
  /**
   * Get message including signing and encrypting it
   */
  QCString pgpProcessedMsg(void);

  /**
   * Set edit widget charset
   */
  void setEditCharset();

  /**
   * Send the message
   */
  void doSend(int sendNow=-1, bool saveInDrafts = false);

  /**
   * get default charset from locale settings
   */
  QCString defaultCharset(void) const;

protected:
  QWidget   *mMainWidget;
  QComboBox *mIdentity, *mTransport, *mFcc;
  KMLineEdit *mEdtFrom, *mEdtReplyTo, *mEdtTo, *mEdtCc, *mEdtBcc, *mEdtSubject;
  QLabel    *mLblIdentity, *mLblTransport, *mLblFcc;
  QLabel    *mLblFrom, *mLblReplyTo, *mLblTo, *mLblCc, *mLblBcc, *mLblSubject;
  QCheckBox *mBtnIdentity, *mBtnTransport, *mBtnFcc;
  QPushButton *mBtnTo, *mBtnCc, *mBtnBcc, *mBtnFrom, *mBtnReplyTo;
  bool mSpellCheckInProgress;
  bool mDone;

  KMEdit* mEditor;
  QGridLayout* mGrid;
  KMMessage *mMsg;
  QListView *mAtmListBox;
  QPtrList<QListViewItem> mAtmItemList;
  KMMsgPartList mAtmList;
  bool mAutoSign, mAutoPgpSign, mAutoDeleteMsg;
  KMFolder *mFolder;
  long mShowHeaders;
  QString mExtEditor;
  bool useExtEditor;
  QPtrList<_StringPair> mCustHeaders;
  bool mConfirmSend;
  bool disableBreaking;
  int mNumHeaders;
  int mLineBreak;
  int mWordWrap;
  short mBtnIdSign, mBtnIdEncrypt;
  short mMnuIdUrgent, mMnuIdConfDeliver, mMnuIdConfRead;
  QFont mBodyFont;
  //  QList<QLineEdit> mEdtList;
  QPtrList<QWidget> mEdtList;
  QPtrList<KTempFile> mAtmTempList;
  QPalette mPalette;
  QString mId, mOldSigText;
  QStringList mTransportHistory;

  KAction *attachPK, *attachMPK;

  KToggleAction *signAction, *encryptAction, *confirmDeliveryAction;
  KToggleAction *confirmReadAction, *urgentAction, *allFieldsAction, *fromAction;
  KToggleAction *replyToAction, *toAction, *ccAction, *bccAction, *subjectAction;
  KToggleAction *identityAction, *transportAction, *fccAction;
  KToggleAction *toolbarAction, *statusbarAction;
  KToggleAction *wordWrapAction;

  KSelectAction *encodingAction;

  QCString mCharset;
  QCString mDefCharset;

  QStringList mFolderNames;
  QValueList<QGuardedPtr<KMFolder> > mFolderList;

private slots:
  void slotCompletionModeChanged( KGlobalSettings::Completion );

private:
  QColor foreColor,backColor;
  struct atmLoadData
  {
    KURL url;
    QByteArray data;
    bool insert;
  };
  QMap<KIO::Job *, atmLoadData> mapAtmLoadData;
  bool mUnicodeFont;
  bool mForceReplyCharset;
};
#endif

