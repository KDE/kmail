/*
 * Copyright (c) 1989 Mark Davies
 * Copyright (c) 1990 Andy Linton
 * Copyright (c) 1990 Victoria University of Wellington.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * Victoria University of Wellington, New Zealand and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(SYSV) || defined(ISC)

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#ifdef ISC
# include <net/errno.h>
#endif
#include "flock.h"               /* defines normally in <sys/file.h> */


#if defined(F_SETLK) && defined(F_SETLKW)

kmflock(int fd, int operation)
{
 int op, ret;
 struct flock arg;
 extern int errno;

 op = (LOCK_NB & operation) ? F_SETLK : F_SETLKW;

 arg.l_type = (LOCK_EX & operation) ? F_WRLCK :
  (LOCK_SH & operation) ? F_RDLCK : F_UNLCK;
 arg.l_whence = 0;
 arg.l_start = 0;
 arg.l_len = 0;
 arg.l_pid = 0;

 if ((ret = fcntl(fd, op, &arg)) == -1) {
  if (errno == EACCES || errno == EAGAIN)
   errno = EWOULDBLOCK;
 }
 return (ret);
}

#else

kmflock(int fd, int operation)
{
 int op, ret;
 extern int errno;

 op = (LOCK_UN & operation) ? F_ULOCK :
   (LOCK_NB & operation) ? F_TLOCK : F_LOCK;

 if ((ret = lockf(fd, op, 0)) == -1) {
  if (errno == EACCES || errno == EAGAIN)
   errno = EWOULDBLOCK;
 }

 return (ret);
}

#endif /* F_SETLK && F_SETLKW */

#endif /* SYSV || ISC */
