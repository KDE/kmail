/* KMail broadcast status message and related classes

   Copyright (C) 2000 Don Sanders <sanders@kde.org>

   License GPL
*/

#ifndef __km_broadcast_status_h
#define __km_broadcast_status_h

#include <qwidget.h>
#include <qframe.h>

class KMMainWidget;
class KProgress;
class QPushButton;
class QWidgetStack;
class QBoxLayout;
class QLabel;
namespace KMail {
  class SSLLabel;
  class ProgressItem;
}
using KMail::SSLLabel;
using KMail::ProgressItem;
#undef None

/** When KMail is running it is possible to have multiple KMMainWin open
    at the same time. We want certain messages/information to be displayed
    in all active KMMainWins. KMBroadcastStatus make this possible, it
    defines a singleton object that broadcasts status messages by emitting
    signals. All KMMainWins connect up these signals to appropriate slots
    for updating their status bar. */

class KMBroadcastStatus : public QObject
{

  Q_OBJECT

public:

  /** Return the instance of the singleton object for this class */
  static KMBroadcastStatus *instance();
  /** Emit an update status bar signal */
  void setStatusMsg( const QString& message );
  /** Sets a status bar message with timestamp */
  void setStatusMsgWithTimestamp( const QString& message );
  /** Sets a transmission completed status bar message */
  void setStatusMsgTransmissionCompleted( int numMessages,
                                          int numBytes = -1,
                                          int numBytesRead = -1,
                                          int numBytesToRead = -1,
                                          bool mLeaveOnServer = false );
  void setStatusMsgTransmissionCompleted( const QString& account,
                                          int numMessages,
                                          int numBytes = -1,
                                          int numBytesRead = -1,
                                          int numBytesToRead = -1,
                                          bool mLeaveOnServer = false );
  /** Emit an enable progress widget(s) in status bar(s) signal */
  void setStatusProgressEnable( const QString& id, bool enable );
  /** Emit an update progress widget(s) percent completed signal */
  void setStatusProgressPercent( const QString& id, unsigned long percent );
  /** Set if the acccount checking is using ssl */
  void setUsingSSL( bool isUsing );

  /** Returns true IFF the user has requested the current operation
      (the one whose progress is being shown) should be aborted.
      Needs to be periodically polled in the implementation of the
      operation. */
  bool abortRequested();
  /**  Set the state of the abort requested variable to false */
  void reset();
  /**  Set the state of the abort requested variable to true,
   * without emitting the signal.
   * (to let the current jobs run, but stop when possible).
   * This is only for exiting gracefully, don't use.
   */
  void setAbortRequested();
signals:

  /** Emitted when setStatusMsg is called. */
  void statusMsg( const QString& );
  /** Emitted when setStatusProgressEnable is called. */
  void statusProgressEnable( bool );
  /** Emitted when setStatusProgressPercent is called. */
  void statusProgressPercent( unsigned long );
  /** Emitted when reset is called. */
  void resetRequested();
  /** Emitted when user wants to abort the connection. */
  void signalAbortRequested();
  void signalUsingSSL( bool );

public slots:

  /** Set the state of the abort requested variable to return */
  void requestAbort();

protected:

  KMBroadcastStatus();
  static KMBroadcastStatus* instance_;
  bool abortRequested_;
  QMap<QString,unsigned long> ids;
};

/** A specialized progress widget class, heavily based on
    kio_littleprogress_dlg (it looks similar) */
class KMLittleProgressDlg : public QFrame {

  Q_OBJECT

public:

  KMLittleProgressDlg( KMMainWidget* mainWidget, QWidget* parent, bool button = true );

public slots:

  virtual void slotEnable( bool );
  virtual void slotJustPercent( unsigned long );
  virtual void slotClean();
  virtual void slotSetSSL( bool );

  virtual void slotProgressItemAdded( ProgressItem *i );
  virtual void slotProgressItemCompleted( ProgressItem *i );
  virtual void slotProgressItemProgress( ProgressItem *i, unsigned int value );

protected slots:
  void slotProgressDialogVisible( bool );

protected:
  KMMainWidget* m_mainWidget;
  KProgress* m_pProgressBar;
  QLabel* m_pLabel;
  SSLLabel* m_sslLabel;
  QPushButton* m_pButton;

  enum Mode { None, Clean, Label, Progress };

  uint mode;
  bool m_bShowButton;

  void setMode();


  virtual bool eventFilter( QObject *, QEvent * );
  QBoxLayout *box;
  QWidgetStack *stack;
  ProgressItem *mCurrentItem;
};

#endif
