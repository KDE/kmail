/* kalarmtimer: timeout handler using the system's alarm timer
 *
 * The alarm timer uses the alarm signal for timeout events. The
 * advantage of this signal is that the event will also occur when
 * the application is busy. Qt's timer events only occur when
 * the application is idle.
 *
 * Author: Stefan Taferner <taferner@salzburg.co.at>
 * This code is under GPL.
 */
#ifndef kalarmtimer_h_
#define kalarmtimer_h_

#include <qobject.h>

typedef unsigned long KAlarmTimerId;

class KAlarmTimer: public QObject
{
  Q_OBJECT

public:
  KAlarmTimer();
  virtual ~KAlarmTimer();

  /** Start the timer. If 'once' is TRUE then the timer periodically
   * sends the timeout signal until stoped. */
  void start (int msec, bool once=FALSE);

  /** Stop the timer. */
  void stop (void);

signals:
  /** Emitted when a timer event occurs. Either connect to this signal
   or inherit the alarmtimer class and write your own `timerEvent'
   method. */
  void timeout(int timerId);

protected:
  /** Virtual method that can be overloaded and is empty per default. 
   You do not need to call `KAlarmTimer::timerEvent' in inherited
   methods. */
  virtual void timerEvent(void);

private:
  void internalTimerEvent(KAlarmTimerId id);

  KAlarmTimerId tid;
  int msec;
  bool once;

  friend void KAlarmTimeoutHandler(int);
};

#endif /*kalarmtimer_h_*/
