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
#include "KEdit.h"
#include <ktopwidget.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kapp.h>
#include <kmsgbox.h>
#include "ktablistbox.h"
#include <kstatusbar.h>
#include "kmmsgpart.h"
#include <qpainter.h>
#include <drag.h>
#include <html.h>
#define FORWARD 0
#define REPLY 1
#define REPLYALL 2

class KMMessage;
class QGridLayout;


//-----------------------------------------------------------------------------
class KMComposeView : public QWidget
{
  Q_OBJECT
public:
  KMComposeView(QWidget *parent=0,const char *name=0,QString emailAddress=0,
		KMMessage *message=0, int action =0);
  ~KMComposeView();
  KEdit *editor;
  KDNDDropZone *zone;

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
  int indexAttachment;
  QStrList *urlList;
  KMMessagePart * createKMMsgPart(KMMessagePart *, QString);
  KHTMLWidget *attachmentWidget;
  KMMessage * prepareMessage();
  QFrame *frame;

public slots:
  void printIt();
  void find();
  void attachFile();
  void sendNow();
  void sendLater();
  void slotEncodingChanged();

private slots:
  void undoEvent();
  void copyText();
  void cutText();
  void pasteText();
  void markAll();
  void slotSelectFont();
  void toDo();
  void newComposer();
  void appendSignature();
  void parseConfiguration();
  void forwardMessage();
  void replyMessage();
  void replyAll();
  void detachFile(int,int);
  void insertFile();
  void getDNDAttachment();
  void insertNewAttachment(QString );
  void createAttachmentWidget();
  void deleteAttachmentWidget();
  void slotChangeHeading(const char *);
  void slotPopupMenu(const char *, const QPoint &);

protected:
  QGridLayout* grid;
};


//-----------------------------------------------------------------------------
class KMComposeWin : public KTopLevelWidget
{
  Q_OBJECT

public:
  KMComposeWin(QWidget *parent = 0, const char *name = 0, 
	       QString emailAddress=0, KMMessage *message=0, int action = 0);
  virtual void show();
  
protected:
  virtual void closeEvent(QCloseEvent *);
  friend class KMComposeView;

private slots:
  void abort();
  void about();
  void invokeHelp();
  void toDo();
  void parseConfiguration();
  void doNewMailReader();
  void toggleToolBar();
  void send();

private:
  void setupMenuBar();
  void setupToolBar();
  void setupStatusBar();

  KToolBar *toolBar;
  KMenuBar *menuBar;
  KStatusBar *statusBar;
  KMComposeView *composeView;
  bool toolBarStatus, sigStatus, sendButton;
  QWidget *setWidget;
  QRadioButton *isLater;
  QRadioButton *manualSig;
};
#endif


