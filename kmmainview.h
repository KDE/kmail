/* KMail Main View of Main Window
 *
 */
#ifndef kmmainview_h
#define kmmainview_h

#include <qwidget.h>

class KMMainWin;
class KPanner;
class KMHeaders;
class KMAcctFolder;
class KMReaderView;
class KMFolderTree;
class KMFolder;

#define KMMainViewInherited QWidget

class KMMainView : public QWidget
{
  Q_OBJECT
public:
  KMMainView(QWidget *parent=0, const char *name=0);
  KMFolderTree *folderTree;

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
  void doReplyAllToMessage();
  void doPrintMessage();
  void initIntegrated();
  void initSeparated();

private:
  KMReaderView *messageView;
  KPanner *horzPanner,*vertPanner;
  KMHeaders *headers;
  KMAcctFolder *currentFolder;
  bool Integrated;
};

#endif /*kmmainview_h*/
