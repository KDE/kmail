/* KMail broadcast status message and related classes

   Copyright (C) 2000 Don Sanders <sanders@kde.org>

   License GPL
*/

#ifndef __km_broadcast_status_h
#define __km_broadcast_status_h

#include <qobject.h>
#include <qmap.h>

#undef None

namespace KMail {
  class ProgressItem;
}

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
                                          bool mLeaveOnServer = false,
                                          KMail::ProgressItem* progressItem = 0 ); // set the same status in this progress item
  void setStatusMsgTransmissionCompleted( const QString& account,
                                          int numMessages,
                                          int numBytes = -1,
                                          int numBytesRead = -1,
                                          int numBytesToRead = -1,
                                          bool mLeaveOnServer = false,
                                          KMail::ProgressItem* progressItem = 0 ); // set the same status in this progress item
  /** Emit an enable progress widget(s) in status bar(s) signal */
  void setStatusProgressEnable( const QString& id, bool enable );
  /** Emit an update progress widget(s) percent completed signal */
  void setStatusProgressPercent( const QString& id, unsigned long percent );

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

public slots:

  /** Set the state of the abort requested variable to return */
  void requestAbort();

protected:

  KMBroadcastStatus();
  static KMBroadcastStatus* instance_;
  bool abortRequested_;
  QMap<QString,unsigned long> ids;
};



#endif
