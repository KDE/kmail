/*  KMail Global Objects
 *
 *  These objects are all created in main.cpp during application start
 *  before anything else is done.
 *
 *  If you add anything here do not forget to define and create the
 *  object in main.cpp !
 *
 *  Author: Stefan Taferner <taferner@alpin.or.at>
 *
 *  removed almost everything: Sven Radej <radej@kde.org>
 */

//this could all go int kmkernel.h
#ifndef kmglobal_h
#define kmglobal_h

#include "kmkernel.h"

typedef enum 
{
    FCNTL,
    procmail_lockfile,
    mutt_dotlock,
    mutt_dotlock_privileged,
    None
} LockType;

/** The "about KMail" text. */
extern const char* aboutText;
#endif
