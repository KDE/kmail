#ifndef _UTIL_H
#define _UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

extern "C" {
#include <env.h>
#include <fs.h>
}

#define ULONG unsigned long 
#define BADFILE 0
#define TEXT 1
#define BINARY 2

#define CHKBYTES 512

const char *getName();
const char *getLocalName(char *);
char *unCRLF(char *);
long writeFile(const char *, void *, unsigned long);
void *readFile(const char *, unsigned long *);
const char *basename(const char *); 
int fileType(const char *);

//////////////////////////////////////////////////////////////////////////////
// Some useful inline functions
//////////////////////////////////////////////////////////////////////////////
inline char *getHostName()
{
    return mylocalhost();
}

inline char *getHomeDir()
{
    return myhomedir();
}

extern char *sysinbox();

inline char *getDefaultInbox()
{
    return sysinbox();
}

#endif
