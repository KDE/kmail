/* kalarmtimer: timeout handler using the system's alarm timer
 *
 * The alarm timer uses the alarm signal for timeout events. The
 * advantage of this signal is that the event will also occur when
 * the application is busy. Qt's timer events only occur when
 * the application is idle.
 *
 * $Id$
 */
#ifndef kalarmtimer_h_
#define kalarmtimer_h_

#include <qobject.h>

typedef unsigned long KAlarmTimerId;

class KAlarmTimer: public QObject
{
  Q_OBJECT;
public:
  KAlarmTimer();
  virtual ~KAlarmTimer();

  void start (int msec, bool once=FALSE);
  void stop (void);

protected:
  virtual void timerEvent(void);

private:
  void internalTimerEvent(KAlarmTimerId id);

  KAlarmTimerId tid;
  int msec;
  bool once;

  friend void KAlarmTimeoutHandler(int);
};

#endif /*kalarmtimer_h_*/
