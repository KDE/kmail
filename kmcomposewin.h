/* KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMComposeWin
#define __KMComposeWin

#include <ktopwidget.h>
#include <qstring.h>

class QLineEdit;
class QGridLayout;
class QStrList;
class QFrame;
class QPopupMenu;
class KEdit;
class KTabListBox;
class KMMessage;
class KMMessagePart;
class KMimeMagic;
class KDNDDropZone;
class KToolBar;
class KStatusBar;

enum Action { actNoOp =0, actForward=1, actReply=2, actReplyAll=3 };


//-----------------------------------------------------------------------------
class KMComposeView : public QWidget
{
  Q_OBJECT
public:
  KMComposeView(QWidget *parent=0,const char *name=0,QString emailAddress=0,
		KMMessage *message=0, Action ac = actNoOp);
  virtual ~KMComposeView();

  virtual const QString to(void) const;
  virtual void setTo(const QString _str);

  virtual const QString cc(void) const;
  virtual void setCc(const QString _str);

  virtual const QString subject(void) const;
  virtual void setSubject(const QString _str);

  virtual const QString text(void) const;
  virtual void setText(const QString _str);
  virtual void appendText(const QString _str);
  virtual void insertText(const QString _str);
  virtual void insertTextAt(const QString _str, int line, int col);
  virtual int textLines(void) const;

  const QString emailAddress(void) const
    {return EMailAddress;}
    
  const QString replyToAddress(void) const
    {return ReplyToAddress;}

  KEdit * getEditor(void);

private:
  KEdit *editor;
  QLineEdit *fromLEdit;
  QLineEdit *toLEdit;
  QLineEdit *subjLEdit;
  QLineEdit *ccLEdit;
  KMMessage *currentMessage;
  KTabListBox *attWidget;
  QString EMailAddress;
  QString ReplyToAddress;
  QStrList *urlList;
  KTabListBox *attachmentListBox;
  QFrame *frame;
  KMimeMagic *magic;

  void parseConfiguration();

  // fill composer from contents of given message
  void fromMsg(KMMessage* msg);

  void forwardMessage();
  void replyMessage();
  void replyAll();
  void insertNewAttachment(QString );
  void createAttachmentWidget();
  void initKMimeMagic();
  bool loadMsgPart(KMMessagePart* msgPart, const QString fileName);
  KMMessage * prepareMessage();

public slots:
  void slotPrintIt();
  void slotFind();
  void slotAttachFile();
  void slotSendNow();
  void slotSendLater();

private slots:
  void slotUndoEvent();
  void slotCopyText();
  void slotCutText();
  void slotPasteText();
  void slotMarkAll();
  void slotSelectFont();
  void slotToDo();
  void slotNewComposer();
  void slotAppendSignature();
  void slotInsertFile();
  void slotGetDNDObject();
  void slotUpdateHeading(const char *);
  void slotPopupMenu(int, int);
  void slotOpenAttachment();
  void slotRemoveAttachment();
  void slotShowProperties();
  

protected:
  QGridLayout* grid;
  KDNDDropZone *zone;
  virtual void resizeEvent(QResizeEvent*);
};


//-----------------------------------------------------------------------------
class KMComposeWin : public KTopLevelWidget
{
  Q_OBJECT

public:
  KMComposeWin(QWidget *parent = 0, const char *name = 0, 
	       QString emailAddress=0, KMMessage *message=0,
	       Action action = actNoOp);
  virtual void show();
  QString encoding;
  friend class KMComposeView;  
protected:
  virtual void closeEvent(QCloseEvent *);
private slots:
  void abort();
  void aboutQt(); 
  void about();
  void invokeHelp();
  void toDo();
  void parseConfiguration();
  void doNewMailReader();
  void toggleToolBar();
  void send();
  void slotEncodingChanged();
private:
  void setupMenuBar();
  void setupToolBar();
  void setupStatusBar();

  KToolBar *toolBar;
  KMenuBar *menuBar;
  KStatusBar *statusBar;
  KMComposeView *composeView;
  bool toolBarStatus, sigStatus, sendButton;
  QPopupMenu *menu;

};
#endif

