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
#include <qlineedit.h>
#include <kio/job.h>

#include "kmmsgpart.h"
#include "kmmsgbase.h"

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
class KDNDDropZone;
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

typedef QList<KMMessagePart> KMMsgPartList;


//-----------------------------------------------------------------------------
#define KMEditInherited KEdit
class KMEdit: public KEdit
{
  Q_OBJECT
public:
  KMEdit(QWidget *parent=NULL,KMComposeWin* composer=NULL,
	 const char *name=NULL);
  virtual ~KMEdit();

  /**
   * Start the spell checker.
   **/
  void spellcheck();

  // Text with lines breaks inserted after every row
  QString brokenText() const;

  // For the external editor
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
  /** Event filter that does Tab-key handling. */
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
#define KMLineEditInherited QLineEdit
class KMLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  KMLineEdit(KMComposeWin* composer = NULL, QWidget *parent = NULL,
	     const char *name = NULL);
  virtual ~KMLineEdit();

  /** Set cursor to end of line. */
   void cursorAtEnd();

signals:
  /** Emitted when Ctrl-. (period) is pressed. */
  void completion();

public slots:
   void undo();
   void copy();
   void cut();
   void paste();
   void markAll();
   void slotCompletion();

protected:
  virtual bool eventFilter(QObject*, QEvent*);
  virtual void dropEvent(QDropEvent *e);
  KMComposeWin* mComposer;
protected:
};


class KMHeaders;

//-----------------------------------------------------------------------------
class KMComposeWin : public KMTopLevelWidget
{
  Q_OBJECT
  friend class KMHeaders;         // needed for the digest forward

public:
  KMComposeWin(KMMessage* msg=NULL, QString id = "unknown" );
  ~KMComposeWin();

  /** Add descriptions to the encodings in the list */
  static void makeDescriptiveNames(QStringList &encodings);

  /** To catch palette changes */
  virtual bool event(QEvent *e);

  /** update colors */
  void readColorConfig();

  /** Write settings to app's config file. */
   void writeConfig(void);

  /** If necessary increases the word wrap of the editor so that it will
      not wrap the body string */
   void verifyWordWrapLengthIsAdequate(const QString&);

  /** Set the message the composer shall work with. This discards
    previous messages without calling applyChanges() on them before. */
   void setMsg(KMMessage* newMsg, bool mayAutoSign=TRUE);

  /** Returns message of the composer. To apply the user changes to the
    message, call applyChanges() first. */
   KMMessage* msg(void) const { return mMsg; }

  /** Applies the user changes to the message object of the composer
    and signs/encrypts the message if activated. Returns FALSE in
    case of an error (e.g. if PGP encryption fails). */
   bool applyChanges(void);

  /** If this flag is set the message of the composer is deleted when
    the composer is closed and the message was not sent. Default: FALSE */
   inline void setAutoDelete(bool f) { mAutoDeleteMsg = f; }

  /** If this folder is set, the original message is inserted back after
    cancelling */
   void setFolder(KMFolder* aFolder) { mFolder = aFolder; }

  /** Recode to the specified charset */
   void setCharset(const QString& aCharset, bool forceDefault = FALSE);

public slots:
 //Actions:
  void slotPrint();
  void slotAttachFile();
  void slotSendNow();
  void slotSendLater();
  void slotSaveDraft();
  void slotDropAction(QDropEvent *e);
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
  void slotAddrBook(); // Open addressbook editor dialog.
  void slotInsertFile(); // Insert a file to the end of the text in the editor.

  void slotSetCharset();

  void slotSpellcheck(); // Check spelling of text.
  void slotSpellcheckConfig();

  // XML-GUI stuff
  void slotToggleToolBar();
  void slotToggleStatusBar();
  void slotEditToolbars();
  void slotEditKeys();

  void readConfig(void); // Read settings from app's config file.

  void slotUpdWinTitle(const QString& ); // Change window title to given string.

  /** Switch the icon to lock or unlock respectivly. */
  void slotEncryptToggled(bool);

  /** Switch wordWrap on/off */
  void slotWordWrapToggled(bool);

  /** Append signature file to the end of the text in the editor. */
  void slotAppendSignature();

  /** Insert sender's public key block in the editor. */
  void slotInsertMyPublicKey();

  /** Insert arbitary public key from public keyring in the editor. */
  void slotInsertPublicKey();

  /** Popup a nice "not implemented" message. */
  void slotToDo();

  /** Open a popup-menu in the attachments-listbox. */
  void slotAttachPopupMenu(QListViewItem *, const QPoint &, int);

  /** Returns the number of the current attachment in the listbox,
  or -1 if there is no current attachment */
  int currentAttachmentNum();

  /** Attachment operations. */
  void slotAttachView();
  void slotAttachRemove();
  void slotAttachSave();
  void slotAttachProperties();


  /** Select an email from the addressbook and add it to the line
    the pressed button belongs to. */
  void slotAddrBookTo();
  void slotAddrBookCc();
  void slotAddrBookBcc();
  void slotAddrBookFrom();
  void slotAddrBookReplyTo();

  void slotCleanSpace();


//  void slotSpellConfigure();
  void slotSpellcheckDone();

  /** Append current message to ~/dead.letter */
  void deadLetter(void);

  void updateCursorPosition();

  void slotView();

  /** Move focus to next/prev edit widget */
  void focusNextPrevEdit(const QWidget* current, bool next);

  /** Update composer field to reflect new identity  */
  void slotIdentityActivated(int idx);

  /** KIO slots for attachment insertion */
  void slotAttachFileData(KIO::Job *, const QByteArray &);
  void slotAttachFileResult(KIO::Job *);

signals:
  /** A message has been queued or saved in the drafts folder */
  void messageQueuedOrDrafted();

protected:
  /** Install grid management and header fields. If fields exist that
    should not be there they are removed. Those that are needed are
    created if necessary. */
  void rethinkFields(bool fromslot=false);

  /** Show or hide header lines */
  void rethinkHeaderLine(int value, int mask, int& row,
				 const QString labelStr, QLabel* lbl,
				 QLineEdit* edt, QPushButton* btn=NULL);
  void rethinkHeaderLine(int value, int mask, int& row,
				 const QString labelStr, QLabel* lbl,
				 QComboBox* cbx, QCheckBox *chk);

  /** Initialization methods */
  void setupActions();
  void setupStatusBar();
  void setupEditor();


  /** Header fields. */
   const QString subject(void) const { return mEdtSubject.text(); }
   const QString to(void) const { return mEdtTo.text(); }
   const QString cc(void) const
   { return (mEdtCc.isHidden()) ? QString() : mEdtCc.text(); }
   const QString bcc(void) const
   { return (mEdtBcc.isHidden()) ? QString() : mEdtBcc.text(); }
   const QString from(void) const { return mEdtFrom.text(); }
   const QString replyTo(void) const { return mEdtReplyTo.text(); }

  /** Ask for confirmation if the message was changed before close. */
  virtual bool queryClose ();
  /** prevent kmail from exiting when last window is deleted (kernel rules)*/
  virtual bool queryExit ();

  /** Add an attachment to the list. */
   void addAttach(const KURL url);
   void addAttach(const KMMessagePart* msgPart);

  /** Remove an attachment from the list. */
   void removeAttach(const QString url);
   void removeAttach(int idx);

   /* Updates an item in the QListView to represnet a given message part */
   void msgPartToItem(const KMMessagePart* msgPart, QListViewItem *lvi);

  /** Open addressbook and append selected addresses to the given
    edit field. */
   void addrBookSelInto(KMLineEdit* destEdit);

private:
  /** Get message including signing and encrypting it */
  const QCString pgpProcessedMsg(void);

  /** Test if string has any 8-bit characters */
  bool is8Bit(const QString str);

  /** Set edit widget charset */
  void setEditCharset();

  /** Send the message */
  void doSend(int sendNow=-1, bool saveInDrafts = false);

  /** get default charset from locale settings */
  const QString defaultCharset(void);

protected:
  QWidget   mMainWidget;
  QComboBox mIdentity, mTransport;
  KMLineEdit mEdtFrom, mEdtReplyTo, mEdtTo, mEdtCc, mEdtBcc, mEdtSubject;
  QLabel    mLblIdentity, mLblTransport;
  QLabel    mLblFrom, mLblReplyTo, mLblTo, mLblCc, mLblBcc, mLblSubject;
  QCheckBox mBtnIdentity, mBtnTransport;
  QPushButton mBtnTo, mBtnCc, mBtnBcc, mBtnFrom, mBtnReplyTo;
  bool mSpellCheckInProgress;
  bool mDone;

  KMEdit* mEditor;
  QGridLayout* mGrid;
  //KDNDDropZone *mDropZone;
  KMMessage *mMsg;
  QListView *mAtmListBox;
  QList<QListViewItem> mAtmItemList;
  KMMsgPartList mAtmList;
  bool mAutoSign, mAutoPgpSign, mAutoDeleteMsg;
  KMFolder *mFolder;
  long mShowHeaders;
  QString mDefEncoding;
  QString mExtEditor;
  bool useExtEditor;
  QList<_StringPair> mCustHeaders;
  bool mConfirmSend;
  bool disableBreaking;
  int mNumHeaders;
  int mLineBreak;
  int mWordWrap;
  short mBtnIdSign, mBtnIdEncrypt;
  short mMnuIdUrgent, mMnuIdConfDeliver, mMnuIdConfRead;
  QFont mBodyFont;
  //  QList<QLineEdit> mEdtList;
  QList<QWidget> mEdtList;
  QList<KTempFile> mAtmTempList;
  QPalette mPalette;
  QString mId, mOldSigText;
  QStringList mTransportHistory;

  KToggleAction *signAction, *encryptAction, *confirmDeliveryAction;
  KToggleAction *confirmReadAction, *urgentAction, *allFieldsAction, *fromAction;
  KToggleAction *replyToAction, *toAction, *ccAction, *bccAction, *subjectAction;
  KToggleAction *identityAction, *transportAction;
  KToggleAction *toolbarAction, *statusbarAction;
  KToggleAction *wordWrapAction;

  KSelectAction *encodingAction;

  QString mCharset;
  QString mDefCharset;
  QFont mSavedEditorFont;

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

