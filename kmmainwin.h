/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef __KMMAINWIN
#define __KMMAINWIN

#include <kmainwindow.h>
#include "kdeversion.h"
#include "qstring.h"

class KMMainWidget;
namespace KPIM {
   class StatusbarProgressWidget;
   class ProgressDialog;
}
using KPIM::StatusbarProgressWidget;
using KPIM::ProgressDialog;

class KMMainWin : public KMainWindow
{
  Q_OBJECT

public:
  // the main window needs to have a name since else restoring the window
  // settings by kwin doesn't work
  KMMainWin(QWidget *parent = 0);
  virtual ~KMMainWin();
  KMMainWidget *mainKMWidget() const { return mKMMainWidget; };
  StatusbarProgressWidget* progressWidget() const { return mLittleProgress; }
  ProgressDialog* progressDialog() const { return mProgressDialog; }


  /** Read configuration options after widgets are created. */
  virtual void readConfig(void);

  /** Write configuration options. */
  virtual void writeConfig(void);

public slots:
  void displayStatusMsg(const QString&);
  void slotEditToolbars();
  void slotUpdateToolbars();
  void setupStatusBar();

protected:
  virtual bool queryClose ();

protected slots:
  void slotQuit();
  void slotConfigChanged();
  void slotShowTipOnStart();

private:
  KMMainWidget *mKMMainWidget;
  StatusbarProgressWidget *mLittleProgress;
  ProgressDialog *mProgressDialog;
  int mMessageStatusId;
  bool mReallyClose;
};

#endif
