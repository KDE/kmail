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
#undef write
#include <html.h>
#include <kapp.h>
#include <ktopwidget.h>
#include <ktoolbar.h>
#include <kmenubar.h>
#include <kmsgbox.h>
#include "kmcomposewin.h"

class KMFolder;
class KMMessage;

class KMReaderView: public QWidget
{
Q_OBJECT
public:
	KMReaderView(QWidget *parent=0, const char *name=0, int msgno=0, KMFolder *f=0);
	KHTMLWidget *messageCanvas;
	KHTMLWidget *headerCanvas;
	QString selectedText;
private:
	KMMessage *currentMessage;
	int currentIndex;
	QScrollBar *vert;
	QScrollBar *horz;
	QFrame *separator;
	KMFolder *currentFolder;
	long allMessages;
	char *kdeDir;
	QString picsDir;
	int currentAtmnt;
public slots:
 	void updateDisplay();
	void clearCanvas();
	void parseMessage(KMMessage*);
	void copy();
	void markAll();
	void printMail();
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
        void popupMenu(const char *, const QPoint &);	        
	void saveURL(int);
protected:
	void resizeEvent(QResizeEvent *);
};

class KMReaderWin : public KTopLevelWidget
{
Q_OBJECT
public:
	KMReaderWin(QWidget *parent=0, const char *name=0, int msgno=0, KMFolder *f=0);
        virtual void show();
	KMReaderView *newView;
	KToolBar *toolBar;
	KMenuBar *menuBar;
private:
	bool showToolBar;
	QRadioButton *fullHeader;
	QRadioButton *halfHeader;
	QWidget *setWidget;
	KMFolder *tempFolder;
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
