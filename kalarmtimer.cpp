// kalarmtimer.cpp

#include "kalarmtimer.h"
#include <sys/time.h>
#include <signal.h>
#include <sys/signal.h>
#include <stdio.h>

#undef DEBUG

#ifdef DEBUG
#define dprintf printf
#else
void dprintf(const char*,...) {}
#endif


static KAlarmTimerId KAlarmAddTimeout(KAlarmTimer* aObj, unsigned long aTime);
static void KAlarmRemoveTimeout (KAlarmTimerId);

typedef struct _KATimeoutRec
{
  struct _KATimeoutRec*	next;
  KAlarmTimerId		id;
  KAlarmTimer*		obj;
  struct timeval	time;
  bool			valid;
} KATimeoutRec;
typedef KATimeoutRec* KATimeoutPtr;

static KATimeoutPtr head = NULL;
static KAlarmTimerId idCounter = 1;

#define ONE_SECOND 1000000


//-----------------------------------------------------------------------------
static char* time2str(struct timeval& tv)
{
  static char str[32];
  sprintf (str, "%ld.%ld", (long)tv.tv_sec, (long)tv.tv_usec);
  return str;
}


//-----------------------------------------------------------------------------
KAlarmTimer :: KAlarmTimer(): QObject()
{
  tid  = 0;
  msec = 0;
}


//-----------------------------------------------------------------------------
KAlarmTimer :: ~KAlarmTimer()
{
}


//-----------------------------------------------------------------------------
void KAlarmTimer :: start (int aMsec, bool aOnce)
{
  msec = aMsec;
  once = aOnce;

  tid = KAlarmAddTimeout(this, msec);
  dprintf ("start %d\n", msec);
}


//-----------------------------------------------------------------------------
void KAlarmTimer :: stop (void)
{
  dprintf ("stop %d\n", msec);
  once = TRUE;
  KAlarmRemoveTimeout(tid);
}


//-----------------------------------------------------------------------------
void KAlarmTimer :: internalTimerEvent (KAlarmTimerId id)
{
  struct timeval tod;

  gettimeofday(&tod,NULL);

  emit timeout(id);
  timerEvent();
  dprintf ("%s: event #%ld %d\n", time2str(tod), id, msec);

  if (!once) tid = KAlarmAddTimeout(this, msec);
  else tid = 0;
}


//-----------------------------------------------------------------------------
void KAlarmTimer :: timerEvent (void)
{
}


//-----------------------------------------------------------------------------
void KAlarmTimeoutHandler(int)
{
  KATimeoutPtr		cur, next, todo;
  struct timeval	outTime;
  struct itimerval	timerVal;

  if (!head) return;
  outTime = head->time;

  todo = head;
  head = head->next;
  todo->next = NULL;

  for (cur=head; cur; cur=next)
  {
    next = cur->next;

    if (!cur->valid || cur->time.tv_sec < 0 ||
	(cur->time.tv_sec == 0 && cur->time.tv_usec <= 0))
    {
      head = next;
      cur->next = todo;
      todo = cur;
    }
    else break;
  }

  for (cur=todo; cur; cur=next)
  {
    next = cur->next;
    if (cur->valid) cur->obj->internalTimerEvent(cur->id);
    else dprintf ("ignoring event #%ld\n", cur->id);
    delete cur;
  }

  if (head)
  {
    signal (SIGALRM, KAlarmTimeoutHandler);
    timerVal.it_value = head->time;
    timerVal.it_interval.tv_sec  = 0;
    timerVal.it_interval.tv_usec = 0;
    setitimer (ITIMER_REAL, &timerVal, NULL);
    dprintf ("next timeout #%ld in %s\n",head->id, time2str(timerVal.it_value));
  }
}

//-----------------------------------------------------------------------------
static KAlarmTimerId KAlarmAddTimeout (KAlarmTimer* aObj, unsigned long aTime)
{
  KATimeoutPtr		cur, last, newTR;
  struct itimerval	timerVal;
  struct timeval	timeVal;

  timeVal.tv_sec  = (aTime>=1000 ? aTime/1000 : 0);
  timeVal.tv_usec = (aTime<1000  ? aTime : aTime%1000)*1000;

  for (cur=head, last=NULL; cur; cur=cur->next)
  {
    if ((cur->time.tv_sec  > timeVal.tv_sec) ||
	(cur->time.tv_sec == timeVal.tv_sec &&
	 cur->time.tv_usec > timeVal.tv_usec)) break;

    timeVal.tv_sec  -= cur->time.tv_sec;
    timeVal.tv_usec -= cur->time.tv_usec;
    if (timeVal.tv_usec < 0)
    {
      timeVal.tv_sec--;
      timeVal.tv_usec += ONE_SECOND;
    }
    last = cur;
  }
  newTR = new KATimeoutRec;
  newTR->next  = cur;
  newTR->id    = idCounter;
  newTR->obj   = aObj;
  newTR->time  = timeVal;
  newTR->valid = TRUE;

  if (!head) signal (SIGALRM, KAlarmTimeoutHandler);

  if (cur)
  {
    cur->time.tv_sec  -= timeVal.tv_sec;
    cur->time.tv_usec -= timeVal.tv_usec;
    if (cur->time.tv_usec < 0)
    {
      cur->time.tv_sec--;
      cur->time.tv_usec += ONE_SECOND;
    }
    dprintf ("changed timeout of #%ld to %s\n", cur->id,
	    time2str(cur->time));
  }

  if (last)
  {
    last->next = newTR;
    dprintf ("new timeout #%d with %s after #%d\n", newTR->id, 
	    time2str(newTR->time), last->id);
  }
  else 
  {
    if (head)
    { // store state of running timer
      getitimer (ITIMER_REAL, &timerVal);
      head->time = timerVal.it_value;
    }
    head = newTR;
    //timerVal.it_interval = newTR->time;
    timerVal.it_value = newTR->time;
    timerVal.it_interval.tv_sec  = 0;
    timerVal.it_interval.tv_usec = 0;
    setitimer (ITIMER_REAL, &timerVal, NULL);
    dprintf ("starting timer for #%ld with %s\n", newTR->id,
	    time2str(newTR->time));
  }

  idCounter++;

  return (newTR->id);
}


//-----------------------------------------------------------------------------
static void KAlarmRemoveTimeout (KAlarmTimerId aId)
{
  KATimeoutPtr cur;

  for (cur=head; cur; cur=cur->next)
  {
    if (cur->id == aId)
    {
      cur->valid = FALSE;
      break;
    }
  }
}


#include "kalarmtimer.moc"
