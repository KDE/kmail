#ifndef __KPIM_STATUSBARPROGRESSWIDGET_H
#define __KPIM_STATUSBARPROGRESSWIDGET_H
/*
  statusbarprogresswidget.h

  This file is part of KMail, the KDE mail client.

  (C) 2004 KMail Authors

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt.  If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.
*/
/**
  *  A specialized progress widget class, heavily based on
  *  kio_littleprogress_dlg (it looks similar)
  */

class KMMainWidget;
class KProgress;
class QPushButton;
class QWidgetStack;
class QBoxLayout;
class QLabel;
class QTimer;

namespace KPIM {
class SSLLabel;
class ProgressItem;
class ProgressDialog;

class StatusbarProgressWidget : public QFrame {

  Q_OBJECT

public:

  StatusbarProgressWidget( ProgressDialog* progressDialog, QWidget* parent, bool button = true );

public slots:

  void slotClean();
  void slotSetSSL( bool );

  void slotProgressItemAdded( ProgressItem *i );
  void slotProgressItemCompleted( ProgressItem *i );
  void slotProgressItemProgress( ProgressItem *i, unsigned int value );

protected slots:
  void slotProgressDialogVisible( bool );
  void slotShowItemDelayed();
  void slotBusyIndicator();

protected:
  void setMode();
  void connectSingleItem();
  void activateSingleItemMode();

  virtual bool eventFilter( QObject *, QEvent * );

private:
  KProgress* m_pProgressBar;
  QLabel* m_pLabel;
  SSLLabel* m_sslLabel;
  QPushButton* m_pButton;

  enum Mode { None, /*Label,*/ Progress };

  uint mode;
  bool m_bShowButton;

  QBoxLayout *box;
  QWidgetStack *stack;
  ProgressItem *mCurrentItem;
  ProgressDialog* mProgressDialog;
  QTimer *mDelayTimer;
  QTimer *mBusyTimer;
};

} // namespace

#endif
