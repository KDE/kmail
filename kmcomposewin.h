// KMComposeWin header file
#ifndef __KMComposeWin
#define __KMComposeWin
#include <iostream.h>
#include <qwidget.h>
#include <qlined.h>
#include <qmlined.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qlayout.h>
#include <qgrpbox.h>
#include <qbttngrp.h>
#include <qradiobt.h>
#include <qkeycode.h>
#include <qaccel.h>
#include <qprinter.h>
#include <qregexp.h>
#include <qlist.h>
#include <qframe.h>
#include <qevent.h>
#include "KEdit.h"
#include <ktopwidget.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kapp.h>
#include <kmsgbox.h>
#include "ktablistbox.h"
#include <kstatusbar.h>
#include "kmmsgpart.h"
#include "kmimemagic.h"
#include <qpainter.h>
#include <drag.h>
#include <html.h>
#include <sys/stat.h>
#include <unistd.h>

class KMMessage;
class QGridLayout;
enum Action { actNoOp =0, actForward=1, actReply=2, actReplyAll=3 };


//-----------------------------------------------------------------------------
class KMComposeView : public QWidget
{
  Q_OBJECT
public:
  KMComposeView(QWidget *parent=0,const char *name=0,QString emailAddress=0,
		KMMessage *message=0, Action ac = actNoOp);
  ~KMComposeView();
  KEdit *editor;


private:
  QLineEdit *fromLEdit;
  QLineEdit *toLEdit;
  QLineEdit *subjLEdit;
  QLineEdit *ccLEdit;
  KMMessage *currentMessage;
  KTabListBox *attWidget;
  QString SMTPServer;
  QString EMailAddress;
  QString ReplyToAddress;
  QStrList *urlList;
  KTabListBox *attachmentListBox;
  QFrame *frame;
  KMimeMagic *magic;

  void parseConfiguration();
  void forwardMessage();
  void replyMessage();
  void replyAll();
  void insertNewAttachment(QString );
  void createAttachmentWidget();
  void initKMimeMagic();
  KMMessagePart * createKMMsgPart(KMMessagePart *, QString);
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
	       QString emailAddress=0, KMMessage *message=0, Action action = actNoOp);
  virtual void show();
  QString encoding;
  friend class KMComposeView;  
protected:
  virtual void closeEvent(QCloseEvent *);
private slots:
  void abort();
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


