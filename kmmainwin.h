/* kmail main window
 * Maintained by Stefan Taferner <taferner@kde.org>
 * This code is under the GPL
 */
#ifndef __KMMAINWIN
#define __KMMAINWIN

#include "kmtopwidget.h"
#include "qstring.h"

class KMMainWidget;
class KMLittleProgressDlg;
class KToggleAction;

class KMMainWin : public KMTopLevelWidget
{
  Q_OBJECT

public:
  // the main window needs to have a name since else restoring the window
  // settings by kwin doesn't work
  KMMainWin(QWidget *parent = 0);
  virtual ~KMMainWin();
  KMMainWidget *mainKMWidget() { return mKMMainWidget; };
  KMLittleProgressDlg* progressDialog() const { return littleProgress; }

  /** Read configuration options after widgets are created. */
  virtual void readConfig(void);

  /** Write configuration options. */
  virtual void writeConfig(void);


public slots:
  void statusMsg(const QString&);
  void htmlStatusMsg(const QString&);
  void displayStatusMsg(const QString&);
  void slotToggleToolBar();
  void slotToggleStatusBar();
  void slotEditToolbars();
  void slotUpdateToolbars();
  void setupStatusBar();

protected slots:
  void slotQuit();

private:
  KToggleAction *mToolbarAction;
  KToggleAction *mStatusbarAction;
  KMMainWidget *mKMMainWidget;
  QString      mLastStatusMsg;
  KMLittleProgressDlg *littleProgress;
  int mMessageStatusId;
};

#endif
