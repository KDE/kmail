// KMComposeWin header file

#ifndef __KMComposeWin
#define __KMComposeWin

#include <qwidget.h>
#include <qlined.h>
#include <qmlined.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <ktopwidget.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kapp.h>
#include <kmsgbox.h>

class KMComposeView : public QWidget
{
	Q_OBJECT
public:
	KMComposeView(QWidget *parent=0,const char *name=0,QString emailAddress=0);
private:
	QLineEdit *fromLEdit;
	QLineEdit *toLEdit;
	QLineEdit *subjLEdit;
	QLineEdit *ccLEdit;

	QMultiLineEdit  *editor;
private slots:
	void undoEvent();
	void copyText();
	void cutText();
	void pasteText();
	void markAll();
	void saveMail();
	void toDo();
protected:
	virtual void resizeEvent(QResizeEvent *);
};

class KMComposeWin : public KTopLevelWidget
{
	Q_OBJECT

public:
	KMComposeWin(QWidget *parent = 0, const char *name = 0, QString emailAddress=0);
private:
	KToolBar *toolBar;
	KMenuBar *menuBar;
	KMComposeView *composeView;

	void setupMenuBar();
	void setupToolBar();
private slots:
	void abort();
	void about();
	void invokeHelp();
	void newComposer();
	void newMailReader();
	void toDo();
protected:
	virtual void closeEvent(QCloseEvent *);
};

#endif

