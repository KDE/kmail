/* KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMComposeWin
#define __KMComposeWin


#include <qstring.h>
#include <qlabel.h>
#include <qlist.h>
#include <qevent.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qclipboard.h>
#include <qpalette.h>
#include <qfont.h>
#include <keditcl.h>

#include "kmmsgpart.h"


#include <qlineedit.h>
#include "kmtopwidget.h"



class QLineEdit;
class QGridLayout;
class QFrame;
class QPopupMenu;
class KEdit;
class KTabListBox;
class KMMessage;
class KDNDDropZone;
class KToolBar;
class KStatusBar;
class QPushButton;
class QCloseEvent;
class KSpell;
class KSpellConfig;
class KMComposeWin;

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
  void slotSpellResult(const char *newtext);
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
  virtual void cursorAtEnd();

signals:
  /** Emitted when Ctrl-. (period) is pressed. */
  void completion();

public slots:
  virtual void copy();
  virtual void cut();
  virtual void paste();
  virtual void markAll();
  virtual void slotCompletion();

protected:
  virtual bool eventFilter(QObject*, QEvent*);
  KMComposeWin* mComposer;
protected:
};


//-----------------------------------------------------------------------------
#define KMComposeWinInherited KMTopLevelWidget
class KMComposeWin : public KMTopLevelWidget
{
  Q_OBJECT

public:
  KMComposeWin(KMMessage* msg=NULL);
  virtual ~KMComposeWin();

  /** Read settings from app's config file. */
  virtual void readConfig(void);

  /** Write settings to app's config file. */
  virtual void writeConfig(void);

  /** If necessary increases the word wrap of the editor so that it will
      not wrap the body string */
  void verifyWordWrapLengthIsAdequate(const QString&);

  /** Set the message the composer shall work with. This discards
    previous messages without calling applyChanges() on them before. */
  virtual void setMsg(KMMessage* newMsg, bool mayAutoSign=TRUE);

  /** Returns message of the composer. To apply the user changes to the
    message, call applyChanges() first. */
  virtual KMMessage* msg(void) const { return mMsg; }

  /** Applies the user changes to the message object of the composer
    and signs/encrypts the message if activated. Returns FALSE in
    case of an error (e.g. if PGP encryption fails). */
  virtual bool applyChanges(void);

  /** If this flag is set the message of the composer is deleted when
    the composer is closed and the message was not sent. Default: FALSE */
  virtual void setAutoDelete(bool f);

public slots:
  void slotPrint();
  void slotAttachFile();
  void slotSend();
  void slotSendNow();
  void slotSendLater();
  void slotDropAction();
  void slotNewComposer();
  void slotNewMailReader();
  void slotClose();
  void slotHelp();

  /** editor functionality */
  void slotFind();
  void slotReplace();

  /** Do cut/copy/paste on the active line-edit */
  void slotCut();
  void slotCopy();
  void slotPaste();
  void slotMarkAll();

  /** Change window title to given string. */
  void slotUpdWinTitle(const QString& );

  /** Append signature file to the end of the text in the editor. */
  void slotAppendSignature();

  /** Insert a file to the end of the text in the editor. */
  void slotInsertFile();

  /** Insert sender's public key block in the editor. */
  void slotInsertMyPublicKey();

  /** Insert arbitary public key from public keyring in the editor. */
  void slotInsertPublicKey();

  /** Popup a nice "not implemented" message. */
  void slotToDo();

  /** Show/hide toolbar. */
  void slotToggleToolBar();

  /** Open a popup-menu in the attachments-listbox. */
  void slotAttachPopupMenu(int, int);

  /** Attachment operations. */
  void slotAttachView();
  void slotAttachRemove();
  void slotAttachSave();
  void slotAttachProperties();

  /** Message flags. */
  void slotToggleConfirmDelivery();
  void slotToggleConfirmRead();
  void slotToggleUrgent();

  /** Change visibility of a header field. */
  void slotMenuViewActivated(int id);

  /** Open addressbook editor dialog. */
  void slotAddrBook();

  /** Select an email from the addressbook and add it to the line
    the pressed button belongs to. */
  void slotAddrBookTo();
  void slotAddrBookCc();
  void slotAddrBookBcc();
  void slotAddrBookFrom();
  void slotAddrBookReplyTo();

  void slotCleanSpace();

  /** Check spelling of text. */
  void slotSpellcheck();
  void slotSpellcheckConfig();
//  void slotSpellConfigure();
  void slotSpellcheckDone();

  /** Append current message to ~/dead.letter */
  virtual void deadLetter(void);

  void updateCursorPosition();

  void slotConfigureCharsets();
  void slotSetCharsets(const char *message,const char *composer,
		       bool ascii,bool quote,bool def);

  /** Move focus to next/prev edit widget */
  virtual void focusNextPrevEdit(const QLineEdit* current, bool next);

protected:
  /** Install grid management and header fields. If fields exist that
    should not be there they are removed. Those that are needed are
    created if necessary. */
  virtual void rethinkFields(void);

  /** Show or hide header lines */
  virtual void rethinkHeaderLine(int value, int mask, int& row, 
				 const QString labelStr, QLabel* lbl,
				 QLineEdit* edt, QPushButton* btn=NULL);
  /** Initialization methods */
  virtual void setupMenuBar(void);
  virtual void setupToolBar(void);
  virtual void setupStatusBar(void);
  virtual void setupEditor(void);

  /** Header fields. */
  virtual const QString subject(void) const { return mEdtSubject.text(); }
  virtual const QString to(void) const { return mEdtTo.text(); }
  virtual const QString cc(void) const { return mEdtCc.text(); }
  virtual const QString bcc(void) const { return mEdtBcc.text(); }
  virtual const QString from(void) const { return mEdtFrom.text(); }
  virtual const QString replyTo(void) const { return mEdtReplyTo.text(); }
#ifdef KRN
  virtual const QString newsgroups(void) const { return mEdtNewsgroups.text(); }
  virtual const QString followupTo(void) const { return mEdtFollowupTo.text(); }
#endif

  /** Ask for confirmation if the message was changed.
    Save settings upon close. */
  virtual void closeEvent(QCloseEvent*);

  /** Add an attachment to the list. */
  virtual void addAttach(const QString url);
  virtual void addAttach(const KMMessagePart* msgPart);

  /** Remove an attachment from the list. */
  virtual void removeAttach(const QString url);
  virtual void removeAttach(int idx);

  /** Returns a string suitable for the attachment listbox that describes
    the given message part. */
  virtual const QString msgPartLbxString(const KMMessagePart* msgPart) const;

  /** Open addressbook and append selected addresses to the given
    edit field. */
  virtual void addrBookSelInto(KMLineEdit* destEdit);
  
private:
  /** Get message including signing and encrypting it */
  virtual const QString pgpProcessedMsg(void);
  
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
  KMLineEdit mEdtFrom, mEdtReplyTo, mEdtTo, mEdtCc, mEdtBcc, mEdtSubject;
  QLabel    mLblFrom, mLblReplyTo, mLblTo, mLblCc, mLblBcc, mLblSubject;
  QPushButton mBtnTo, mBtnCc, mBtnBcc, mBtnFrom, mBtnReplyTo;
  bool mSpellCheckInProgress;
#ifdef KRN
  KMLineEdit mEdtNewsgroups, mEdtFollowupTo;
  QLabel     mLblNewsgroups, mLblFollowupTo;
#endif

  QPopupMenu *mMnuView, *mMnuOptions;
  KMEdit* mEditor;
  QGridLayout* mGrid;
  KDNDDropZone *mDropZone;
  KMMessage *mMsg;
  KToolBar *mToolBar;
  KMenuBar *mMenuBar;
  KStatusBar *mStatusBar;
  KTabListBox *mAtmListBox;
  KMMsgPartList mAtmList;
  bool mAutoSign, mAutoPgpSign, mShowToolBar, mAutoDeleteMsg;
  long mShowHeaders;
  QString mDefEncoding;
#ifndef KRN
  QString mExtEditor;
  bool useExtEditor;
#endif
  int mNumHeaders;
  int mLineBreak;
  int mWordWrap;
  short mBtnIdSign, mBtnIdEncrypt;
  short mMnuIdUrgent, mMnuIdConfDeliver, mMnuIdConfRead;
  QFont mBodyFont;
  QList<QLineEdit> mEdtList;
  static QString mPathAttach;
  QPalette mPalette;

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
};
#endif

