// KMMainWin header file

#ifndef __KMMAINWIN
#define __KMMAINWIN

#include <ktopwidget.h>

class KMFolder;
class KMMainView;
class KMenuBar;
class KToolBar;
class KStatusBar;

#define KMMainWinInherited KTopLevelWidget

class KMMainWin : public KTopLevelWidget
{
  Q_OBJECT
public:
  KMMainWin(QWidget *parent = 0, char *name = 0);
  virtual ~KMMainWin();
  virtual void show();

protected:
  virtual void closeEvent(QCloseEvent *);

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

};

#endif

