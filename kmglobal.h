/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
//  removed almost everything: Sven Radej <radej@kde.org>

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
#endif
