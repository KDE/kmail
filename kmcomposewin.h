/* KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMComposeWin
#define __KMComposeWin

#include "kmtopwidget.h"

//#include <qstring.h>
#include <qlabel.h>

//#include <qlist.h>
//#include <qevent.h>
//#include <qwidget.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qclipboard.h>
#include <qpalette.h>
#include <qfont.h>
#include <keditcl.h>
#include <qlineedit.h>
#include <ktempfile.h>
#include <kio/job.h>
#include <kurl.h>


#include "kmmsgpart.h"





#ifndef KRN
class _StringPair {
 public:
   QString name;
   QString value;
};
#endif



class QLineEdit;
class QComboBox;
class QGridLayout;
class QFrame;
class QPopupMenu;
class KEdit;
class QListView;
class QListViewItem;
class KMMessage;
class KDNDDropZone;
class KToolBar;
class KStatusBar;
class QPushButton;
class QCloseEvent;
class KSpell;
class KSpellConfig;
class KMComposeWin;
class KToggleAction;

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

#ifndef KRN
  // For the external editor
  inline void setExternalEditor(bool extEd) { extEditor=extEd; }
  inline void setExternalEditorPath(QString path) { mExtEditor=path; }
#endif

signals:
  void spellcheck_done();
public slots:
  void slotSpellcheck2(KSpell*);
  void slotSpellResult(const QString &newtext);
  void slotSpellDone();

protected:
  /** Event filter that does Tab-key handling. */
  virtual bool eventFilter(QObject*, QEvent*);

  KMComposeWin* mComposer;
private:
  KSpell *mKSpell;
#ifndef KRN
  bool extEditor;
  QString mExtEditor;
#endif
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

public slots:
 //Actions:
  void slotPrint();
  void slotAttachFile();
  void slotSendNow();
  void slotSendLater();
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

  void slotSpellcheck(); // Check spelling of text.
  void slotSpellcheckConfig();

  // XML-GUI stuff
  void slotToggleToolBar();
  void slotToggleStatusBar();
  void slotEditToolbars();
  void slotEditKeys();

  void readConfig(void); // Read settings from app's config file.

  void slotUpdWinTitle(const QString& ); // Change window title to given string.

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

  void slotConfigureCharsets();
  void slotSetCharsets(const char *message,const char *composer,
		       bool ascii,bool quote,bool def);
  void slotView();

  /** Move focus to next/prev edit widget */
  void focusNextPrevEdit(const QWidget* current, bool next);

  /** Update composer field to reflect new identity  */
  void slotIdentityActivated(int idx);

  /** KIO slots for attachment insertion */
  void slotAttachFileData(KIO::Job *, const QByteArray &);
  void slotAttachFileResult(KIO::Job *);

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
   const QString cc(void) const { return mEdtCc.text(); }
   const QString bcc(void) const { return mEdtBcc.text(); }
   const QString from(void) const { return mEdtFrom.text(); }
   const QString replyTo(void) const { return mEdtReplyTo.text(); }
#ifdef KRN
   const QString newsgroups(void) const { return mEdtNewsgroups.text(); }
   const QString followupTo(void) const { return mEdtFollowupTo.text(); }
#endif

  /** Ask for confirmation if the message was changed before close. */
  virtual bool queryClose ();
  /** prevent kmail from exiting when last window is deleted (kernel rules)*/
  virtual bool queryExit ();

  /** Add an attachment to the list. */
   void addAttach(const QString url);
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
   const QString pgpProcessedMsg(void);

#if defined CHARSETS
  /** Converts message text for sending. */
  void convertToSend(const QString str);

  /** Test if string has any 8-bit characters */
  bool is8Bit(const QString str);

  /** Set edit widget charset */
  void setEditCharset();
#endif

  /** Send the message */
  void doSend(int sendNow=-1);

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
#ifdef KRN
  KMLineEdit mEdtNewsgroups, mEdtFollowupTo;
  QLabel     mLblNewsgroups, mLblFollowupTo;
#endif

  KMEdit* mEditor;
  QGridLayout* mGrid;
  //KDNDDropZone *mDropZone;
  KMMessage *mMsg;
  QListView *mAtmListBox;
  QList<QListViewItem> mAtmItemList;
  KMMsgPartList mAtmList;
  bool mAutoSign, mAutoPgpSign, mAutoDeleteMsg;
  long mShowHeaders;
  QString mDefEncoding;
#ifndef KRN
  QString mExtEditor;
  bool useExtEditor;
  QList<_StringPair> mCustHeaders;
  bool mConfirmSend;
#endif
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

#if defined CHARSETS
  int m7BitAscii;
  QString mDefaultCharset;
  QString mCharset;
  QString mDefComposeCharset;
  QString mComposeCharset;
  int mQuoteUnknownCharacters;
  QFont mSavedEditorFont;
#endif

private:
  QColor foreColor,backColor;
  struct atmLoadData
  {
    KURL url;
    QByteArray data;
    bool insert;
  };
  QMap<KIO::Job *, atmLoadData> mapAtmLoadData;
};
#endif

