// KMMainWin header file

#ifndef __KMMAINWIN
#define __KMMAINWIN

#include <qwidget.h>
#include <qmlined.h>
#include <kpanner.h>
#include <ktopwidget.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include "kmfoldertree.h"
#include "kmheaders.h"

class KMMainView : public QWidget   // separated now because of KTopLevelWidget
{
	Q_OBJECT
public:
	KMMainView(QWidget *parent=0,const char *name=0);

	KMFolderTree *folderTree;
private:
	QMultiLineEdit *messageView;
	KPanner *horzPanner,*vertPanner;
	KMHeaders *headers;
private slots:
	void doAddFolder();
	void doCheckMail();
	void doCompose();
	void doModifyFolder();
	void doRemoveFolder();
	void folderSelected(QDir *);
	void messageSelected(Message *);
	void pannerHasChanged();
	void resizeEvent(QResizeEvent *);
};

class KMMainWin : public KTopLevelWidget
{
	Q_OBJECT
public:
	KMMainWin(QWidget *parent = 0, char *name = 0);
private:
	KMMainView *mainView;
	KMenuBar *menuBar;
	KToolBar *toolBar;
	KStatusBar *statusBar;

	void parseConfiguration();
	void setupMenuBar();
	void setupToolBar();
	void setupStatusBar();
private slots:
	void doAbout();
	void doClose();
	void doHelp();
	void doNewMailReader();
	void doSettings();
	void doUnimplemented();
protected:
	virtual void closeEvent(QCloseEvent *);
};

#endif

