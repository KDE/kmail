// -*- c++ -*- convenience wrappers around kdDebug/kdWarning/etc

#ifndef __KMAIL_KMDEBUG_H__
#define __KMAIL_KMDEBUG_H__

#include <kdebug.h>

// Enable this to debug timing
// #define DEBUG_TIMING

// return type of kmDebug() depends on NDEBUG define:
#ifdef NDEBUG
# define kmail_dbgstream kndbgstream
#else
# define kmail_dbgstream kdbgstream
#endif

/** KMail's debug area code */
static const int kmail_debug_area = 5006;

static inline kmail_dbgstream kmDebug() { return kdDebug( kmail_debug_area ); }
static inline kmail_dbgstream kmDebug( bool cond ) { return kdDebug( cond, kmail_debug_area ); }

static inline kdbgstream kmWarning() { return kdWarning( kmail_debug_area ); }
static inline kdbgstream kmWarning( bool cond ) { return kdWarning( cond, kmail_debug_area ); }

static inline kdbgstream kmError() { return kdError( kmail_debug_area ); }
static inline kdbgstream kmError( bool cond ) { return kdError( cond, kmail_debug_area ); }

static inline kdbgstream kmFatal() { return kdFatal( kmail_debug_area ); }
static inline kdbgstream kmFatal( bool cond ) { return kdFatal( cond, kmail_debug_area ); }

// timing utilities
#if !defined( NDEBUG ) && defined( DEBUG_TIMING )
#include <qdatetime.h>
#define CREATE_TIMER(x) int x=0, x ## _tmp=0; QTime x ## _tmp2
#define START_TIMER(x) x ## _tmp2 = QTime::currentTime()
#define GRAB_TIMER(x) x ## _tmp2.msecsTo(QTime::currentTime())
#define END_TIMER(x) x += GRAB_TIMER(x); x ## _tmp++
#define SHOW_TIMER(x) kdDebug(5006) << #x " == " << x << "(" << x ## _tmp << ")\n"
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
#define SHOW_COUNTER(x) kdDebug(5006) << #x " == " << x ## _cnt << endl
#else
#define CREATE_COUNTER(x)
#define RESET_COUNTER(x)
#define INC_COUNTER(x)
#define SHOW_COUNTER(x)
#endif

#endif // __KMAIL_KMDEBUG_H__
