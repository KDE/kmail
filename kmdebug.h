// -*- c++ -*- convenience wrappers around kDebug/kWarning/etc

#ifndef __KMAIL_KMDEBUG_H__
#define __KMAIL_KMDEBUG_H__

#include <kdebug.h>

// Enable this to debug timing
// #define DEBUG_TIMING

// timing utilities
#if !defined( NDEBUG ) && defined( DEBUG_TIMING )
#include <QDateTime>
#define CREATE_TIMER(x) int x=0, x ## _tmp=0; QTime x ## _tmp2
#define START_TIMER(x) x ## _tmp2 = QTime::currentTime()
#define GRAB_TIMER(x) x ## _tmp2.msecsTo(QTime::currentTime())
#define END_TIMER(x) x += GRAB_TIMER(x); x ## _tmp++
#define SHOW_TIMER(x) kDebug() << #x" ==" << x <<"(" << x ## _tmp <<")"
#else
#define CREATE_TIMER(x)
#define START_TIMER(x)
#define GRAB_TIMER(x)
#define END_TIMER(x)
#define SHOW_TIMER(x)
#endif

// profiling utilities
#if !defined( NDEBUG )
#define CREATE_COUNTER(x) int x ## _cnt=0
#define RESET_COUNTER(x) x ## _cnt=0
#define INC_COUNTER(x) x ## _cnt++
#define SHOW_COUNTER(x) kDebug() << #x" ==" << x ## _cnt
#else
#define CREATE_COUNTER(x)
#define RESET_COUNTER(x)
#define INC_COUNTER(x)
#define SHOW_COUNTER(x)
#endif

#endif // __KMAIL_KMDEBUG_H__
