#ifndef KMREADERWIN_H
#define KMREADERWIN_H

#include <stdio.h>
#include <string.h>
#include <qaccel.h>
#include <qstring.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qpushbt.h>
#include <qmlined.h>
#include <qregexp.h>
#include <qradiobt.h>
#include <qbttngrp.h>
#include <mclass.h>
#undef write
#include <html.h>
#include <kapp.h>
#include <ktopwidget.h>
#include <ktoolbar.h>
#include <kmenubar.h>
#include <kmsgbox.h>
#include "kmcomposewin.h"

class KMReaderView: public QWidget
{
Q_OBJECT
public:
	KMReaderView(QWidget *parent =0, const char *name=0, int msgno = 0, Folder *f =0);
	KHTMLWidget *messageCanvas;
	KHTMLWidget *headerCanvas;
private:
	Message *currentMessage;
	int currentIndex;
	bool displayFull;
	QScrollBar *vert;
	QScrollBar *horz;
	QFrame *separator;
	Folder *currentFolder;
	long allMessages;
public slots:
 	void updateDisplay();
	void clearCanvas();
	void parseMessage(Message*);
private slots:
	void slotScrollVert(int);
	void slotScrollHorz(int);
	void slotScrollLeRi();
	void slotScrollUpDo();
	void slotDocumentDone();
	void slotDocumentChanged();
	void saveMail();
 	void replyMessage();
	void replyAll();
	void forwardMessage();
	void deleteMessage();
	void nextMessage();
	void previousMessage();
	void openURL(const char *, int);
	void saveURL(int);
protected:
	void resizeEvent(QResizeEvent *);
};

class KMReaderWin : public KTopLevelWidget
{
Q_OBJECT
public:
	KMReaderWin(QWidget *parent = 0, const char *name =0, int msgno = 0, Folder *f =0);
	KMReaderView *newView;
	KToolBar *toolBar;
	KMenuBar *menuBar;
	bool displayFull;	
private:
	bool showToolBar;
	QRadioButton *fullHeader;
	QRadioButton *halfHeader;
	QWidget *setWidget;
	Folder *tempFolder;
public slots:
	void parseConfiguration();
private slots:
	//void applySettings();
	//void cancelSettings();
	void abort();
	void invokeHelp();
	void about();
	void toDo();
	void newComposer();
	void newReader();
	//void setSettings();
	void toggleToolBar();
	void setupMenuBar();
	void setupToolBar();
protected:
	virtual void closeEvent(QCloseEvent*);
};
#endif
