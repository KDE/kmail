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
#include "kmreaderwin.h"
#include "mclass.h"

class KMMainView : public QWidget   // separated now because of KTopLevelWidget
{
	Q_OBJECT
public:
	KMMainView(QWidget *parent=0,const char *name=0);
	KMFolderTree *folderTree;
private:
	KMReaderView *messageView;
	KPanner *horzPanner,*vertPanner;
	KMHeaders *headers;
	Folder *currentFolder;
	bool Integrated;
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
	void doDeleteMessage();
	void doForwardMessage();
	void doReplyMessage();
	void initIntegrated();
	void initSeparated();
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

