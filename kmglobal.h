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

#include <kglobalsettings.h> //for KGlobalSettings
#include <klocale.h> //for I18N_NOOP

#include "kmkernel.h"

typedef enum
{
    FCNTL,
    procmail_lockfile,
    mutt_dotlock,
    mutt_dotlock_privileged,
    lock_none
} LockType;

/*
 * Define the possible units to use for measuring message expiry.
 * expireNever is used to switch off message expiry, and expireMaxUnits
 * must always be the last in the list (for bounds checking).
 */
typedef enum {
  expireNever,
  expireDays,
  expireWeeks,
  expireMonths,
  expireMaxUnits
} ExpireUnits;

#define HDR_FROM     0x01
#define HDR_REPLY_TO 0x02
#define HDR_TO       0x04
#define HDR_CC       0x08
#define HDR_BCC      0x10
#define HDR_SUBJECT  0x20
#define HDR_NEWSGROUPS  0x40
#define HDR_FOLLOWUP_TO 0x80
#define HDR_IDENTITY 0x100
#define HDR_TRANSPORT 0x200
#define HDR_FCC       0x400
#define HDR_DICTIONARY 0x800
#define HDR_ALL      0xfff

#define HDR_STANDARD (HDR_SUBJECT|HDR_TO|HDR_CC)


/** The "about KMail" text. */
extern const char* aboutText;

/** Global Completion Modes*/
// This struct comes from configuredialog.cpp. It is now being used by
// kmcomposewin.cpp also.
namespace KMail {

static const struct {
  KGlobalSettings::Completion mode;
  const char* displayName;
} completionModes[] = {
  { KGlobalSettings::CompletionNone, I18N_NOOP("None") },
  { KGlobalSettings::CompletionShell, I18N_NOOP("Manual") },
  { KGlobalSettings::CompletionAuto, I18N_NOOP("Automatic") },
  { KGlobalSettings::CompletionPopup, I18N_NOOP("Dropdown List") },
  { KGlobalSettings::CompletionMan, I18N_NOOP("Short Automatic") },
  { KGlobalSettings::CompletionPopupAuto, I18N_NOOP("Dropdown List && Automatic") }
};

static const int numCompletionModes = sizeof KMail::completionModes / sizeof *KMail::completionModes;

}

#endif
