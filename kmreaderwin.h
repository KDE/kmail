// Header for kmreaderwin the kmail reader
// written by Markus Wuebben <markus.wuebben@kde.org>

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
#include <qtabdlg.h>
#include <html.h>
#include <kapp.h>
#include <ktopwidget.h>
#include <ktoolbar.h>
#include <kmenubar.h>
#include <kmsgbox.h>
#include "kmcomposewin.h"
#include "kmimemagic.h"
#include "kmmsgpart.h"
#include <mimelib/utility.h>
#include <mimelib/string.h>


class KMReaderView : public QWidget
{
Q_OBJECT
public:
	KMReaderView(QWidget *p=0,const char *n=0, int n=0, KMFolder *f=0);
        KHTMLWidget *messageCanvas;
	bool isInline();
	void setInline(bool);

private:
	KMMessage *currentMessage;
        KMFolder *currentFolder;
        QScrollBar *vert;
        QScrollBar *horz;
        QFrame *separator;
        char *kdeDir;
        QString picsDir;
        long allMessages;
        int currentAtmnt;
        int currentIndex;
	void initKMimeMagic();                                              
	void parseConfiguration();
	KMimeMagic *magic;
	u_int MAX_LINES;
	bool showInline;
        QString selectedText;	

public slots:
	void clearCanvas();
	void parseMessage(KMMessage*);
	void saveMail();
	void printMail();
	void copy();
	void markAll();
	void viewSource();

private slots:
	void replyMessage();
	void replyAll();
	void forwardMessage();
	void nextMessage();
	void previousMessage();
	void deleteMessage();
	void slotScrollVert( int _y );
        void slotScrollHorz( int _x );
        void slotScrollUpDo();
        void slotScrollLeRi();
        void slotDocumentChanged();
        void slotDocumentDone();
	void slotOpenAtmnt();
	void slotSaveAtmnt();
	void slotPrintAtmnt();
	void slotViewAtmnt();
        void openURL(const char *, int);
        void popupHeaderMenu(const char *, const QPoint &);
        void popupMenu(const char *, const QPoint &);
        void saveURL(int);
	void updateDisplay();
	QString parseEAddress(QString);
	QString parseBodyPart(KMMessagePart *,int);
	QString decodeString(KMMessagePart*, QString); 
	QString scanURL(QString);
                                                
protected:
	void resizeEvent(QResizeEvent *);
};

class KMReaderWin : public KTopLevelWidget
{
Q_OBJECT
public:
	KMReaderWin(QWidget *parent=0, const char *name=0, int n=0, KMFolder *f=0);
        virtual void show();
        KMReaderView *newView;
        KToolBar *toolBar;
        KMenuBar *menuBar;
                                
private:
        bool showToolBar;
        KMFolder *tempFolder;
                                        
public slots:
	void parseConfiguration();
        void toDo(); 
private slots:
	void setupToolBar();
	void setupMenuBar();
	void invokeHelp();
	void newComposer();
	void newReader();
	void about();
	void toggleToolBar();
	void abort();

protected:
	virtual void closeEvent(QCloseEvent *);
	
};

class KMGeneral : public QDialog
{
Q_OBJECT
public:
	KMGeneral(QWidget *p=0, const char *n=0, KMMessage *m=0);
	KMMessage *tempMes;
protected:
	void paintEvent(QPaintEvent *);	

};

class KMSource : public QDialog
{
Q_OBJECT
public:
	KMSource(QWidget *p=0, const char *n=0, QString s=0);
	QMultiLineEdit *edit;	
};

class KMProperties: public QDialog
{
Q_OBJECT
public:
	KMProperties(QWidget *p=0, const char *n=0, KMMessage *m=0);
	QTabDialog *tabDialog;
	KMGeneral *topLevel;
	KMSource  *sourceWidget;

};
#endif
	
		

	  
