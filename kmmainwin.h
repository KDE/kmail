/* kmail main window
 * Maintained by Stefan Taferner <taferner@kde.org>
 * This code is under the GPL
 */
#ifndef __KMMAINWIN
#define __KMMAINWIN

#include <kmainwindow.h>
#include "kdeversion.h"
#include "qstring.h"

class KMMainWidget;
class StatusbarProgressWidget;

class KMMainWin : public KMainWindow
{
  Q_OBJECT

public:
  // the main window needs to have a name since else restoring the window
  // settings by kwin doesn't work
  KMMainWin(QWidget *parent = 0);
  virtual ~KMMainWin();
  KMMainWidget *mainKMWidget() const { return mKMMainWidget; };
  StatusbarProgressWidget* progressDialog() const { return littleProgress; }

  /** Read configuration options after widgets are created. */
  virtual void readConfig(void);

  /** Write configuration options. */
  virtual void writeConfig(void);

public slots:
  void statusMsg(const QString&);
  void htmlStatusMsg(const QString&);
  void displayStatusMsg(const QString&);
  void slotEditToolbars();
  void slotUpdateToolbars();
  void setupStatusBar();

protected:
  virtual bool queryClose ();

protected slots:
  void slotQuit();
  void slotConfigChanged();

private:
  KMMainWidget *mKMMainWidget;
  QString      mLastStatusMsg;
  StatusbarProgressWidget *littleProgress;
  int mMessageStatusId;
  bool mReallyClose;
};

#endif
