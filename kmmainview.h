/* KMail Main View of Main Window
 *
 */
#ifndef kmmainview_h
#define kmmainview_h

#include <qwidget.h>

class KMMainWin;
class KPanner;
class KMHeaders;
class KMFolder;
class KMReaderView;
class KMFolderTree;

#define KMMainViewInherited QWidget

class KMMainView : public QWidget
{
	Q_OBJECT
public:
	KMMainView(QWidget *parent=0, const char *name=0);
	KMFolderTree *folderTree;
private:
	KMReaderView *messageView;
	KPanner *horzPanner,*vertPanner;
	KMHeaders *headers;
	KMFolder *currentFolder;
	bool Integrated;
private slots:
	void doAddFolder();
	void doCheckMail();
	void doCompose();
	void doModifyFolder();
	void doRemoveFolder();
	void doEmptyFolder();
	void folderSelected(KMFolder*);
	void messageSelected(KMMessage*);
	void pannerHasChanged();
	void resizeEvent(QResizeEvent*);
	void doDeleteMessage();
	void doForwardMessage();
	void doReplyMessage();
	void initIntegrated();
	void initSeparated();
};

#endif /*kmmainview_h*/
