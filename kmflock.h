/* flock emulation
 */
#ifndef kmflock_h
#define kmflock_h

#ifndef HAVE_FLOCK
#ifndef _FLOCK_EMULATE_INCLUDED
#define _FLOCK_EMULATE_INCLUDED

#include <fcntl.h>

#if defined(F_SETLK) && defined(F_SETLKW)
# define FCNTL_FLOCK
#else
# define LOCKF_FLOCK
#endif /* F_SETLK && F_SETLKW */

/* These definitions are in <sys/file.h> on BSD 4.3 */

/*
 * Flock call.
 */
#define LOCK_SH  1 /* shared lock */
#define LOCK_EX  2 /* exclusive lock */
#define LOCK_NB  4 /* don't block when locking */
#define LOCK_UN  8 /* unlock */

#if defined(__cplusplus)
extern "C" {
extern int kmflock(int fd, int operation);
}
#else
extern int kmflock(int fd, int operation);
#endif

#endif /* _FLOCK_EMULATE_INCLUDED */
#else  /* HAVE_FLOCK */
#define kmflock flock
#endif /* HAVE_FLOCK */
#endif /*kmflock_h*/
