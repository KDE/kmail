/* KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMComposeWin
#define __KMComposeWin

#include <ktopwidget.h>
#include <qstring.h>
#include <qlined.h>
#include <qlabel.h>
#include <qlist.h>
#include <qevent.h>
#include <qwidget.h>
#include <qclipbrd.h>
#include <kmsgbox.h>
#include "kmmsgpart.h"

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

typedef QList<KMMessagePart> KMMsgPartList;

class KMLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  KMLineEdit(QWidget *parent = NULL, const char *name = NULL);

public slots:
  void copy();
  void cut();
  void paste();
  void markAll();

private:
  /*int cursorPos;
  int offset;
  QString tbuf;*/
protected:
  virtual void mousePressEvent(QMouseEvent *);
};

//-----------------------------------------------------------------------------
#define KMComposeWinInherited KTopLevelWidget
class KMComposeWin : public KTopLevelWidget
{
  Q_OBJECT

public:
  KMComposeWin(KMMessage* msg=NULL);
  virtual ~KMComposeWin();

  /** Read settings from app's config file. */
  virtual void readConfig(void);

  /** Write settings to app's config file. Calls sync() if withSync is TRUE. */
  virtual void writeConfig(bool withSync=TRUE);

  /** Set the message the composer shall work with. This discards
    previous messages without calling applyChanges() on them before. */
  virtual void setMsg(KMMessage* newMsg);

  /** Returns message of the composer. To apply the user changes to the
    message, call applyChanges() first. */
  virtual KMMessage* msg(void) const { return mMsg; }

  /** Applies the user changes to the message object of the composer. */
  virtual void applyChanges(void);

  /** If this flag is set the message of the composer is deleted when
    the composer is closed and the message was not sent. Default: FALSE */
  virtual void setAutoDelete(bool f);

public slots:
  void slotPrint();
  void slotAttachFile();
  void slotSend();
  void slotSendLater();
  void slotDropAction();
  void slotNewComposer();
  void slotClose();

  /** Do cut/copy/paste on the active line-edit */
  void slotCut();
  void slotCopy();
  void slotPaste();
  void slotHelp();

  /** Change window title to given string. */
  void slotUpdWinTitle(const char *);

  /** Append signature file to the end of the text in the editor. */
  void slotAppendSignature();

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

  /** Change visibility of a header field. */
  void slotMenuViewActivated(int id);

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

  /** Save settings upon close. */
  virtual void closeEvent(QCloseEvent*);

  /** Add an attachment to the list. */
  virtual void addAttach(QString url);
  virtual void addAttach(KMMessagePart* msgPart);

  /** Remove an attachment from the list. */
  virtual void removeAttach(QString url);
  virtual void removeAttach(int idx);

  /** Returns a string suitable for the attachment listbox that describes
    the given message part. */
  virtual const QString msgPartLbxString(KMMessagePart* msgPart) const;

protected:
  QWidget   mMainWidget;
  KMLineEdit mEdtFrom, mEdtReplyTo, mEdtTo, mEdtCc, mEdtBcc, mEdtSubject;
  QLabel    mLblFrom, mLblReplyTo, mLblTo, mLblCc, mLblBcc, mLblSubject;

  QPopupMenu* mMnuView;
  KEdit* mEditor;
  QGridLayout* mGrid;
  KDNDDropZone *mDropZone;
  KMMessage *mMsg;
  KToolBar *mToolBar;
  KMenuBar *mMenuBar;
  KStatusBar *mStatusBar;
  KTabListBox *mAtmListBox;
  KMMsgPartList mAtmList;
  bool mAutoSign, mShowToolBar, mAutoDeleteMsg;
  int  mSendImmediate;
  long mShowHeaders;
  QString mDefEncoding;
  int mNumHeaders;
};
#endif

