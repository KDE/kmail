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
#define FORWARD 0
#define REPLY 1
#define REPLYALL 2

class KMMessage;
class QGridLayout;


class KMAttachmentItem  // for Attachment Widget
{
public:
  KMAttachmentItem(QString fileName =0, int index = 0);
  QString fileName;
  int index;
};


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
  int indexAttachment;
  QStrList *urlList;
  KMMessagePart * createKMMsgPart(KMMessagePart *, QString);

public slots:
  void sendIt();
  void printIt();
  void find();
  void attachFile();

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
  void invokeHelp();
  void toDo();
  void parseConfiguration();
  void doNewMailReader();
  void toggleToolBar();

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


