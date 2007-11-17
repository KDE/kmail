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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
//  removed almost everything: Sven Radej <radej@kde.org>

//this could all go int kmkernel.h
#ifndef kmglobal_h
#define kmglobal_h

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
#define HDR_SUBJECT  0x20
#define HDR_NEWSGROUPS  0x40
#define HDR_FOLLOWUP_TO 0x80
#define HDR_IDENTITY 0x100
#define HDR_TRANSPORT 0x200
#define HDR_FCC       0x400
#define HDR_DICTIONARY 0x800
#define HDR_ALL      0xfff

#define HDR_STANDARD (HDR_SUBJECT)

#include <algorithm>

namespace KMail {
// List of prime numbers shamelessly stolen from GCC STL
  enum { num_primes = 29 };

  static const unsigned long prime_list[ num_primes ] =
  {
    31ul,        53ul,         97ul,         193ul,       389ul,
    769ul,       1543ul,       3079ul,       6151ul,      12289ul,
    24593ul,     49157ul,      98317ul,      196613ul,    393241ul,
    786433ul,    1572869ul,    3145739ul,    6291469ul,   12582917ul,
    25165843ul,  50331653ul,   100663319ul,  201326611ul, 402653189ul,
    805306457ul, 1610612741ul, 3221225473ul, 4294967291ul
  };

  inline unsigned long nextPrime( unsigned long n )
  {
    const unsigned long *first = prime_list;
    const unsigned long *last = prime_list + num_primes;
    const unsigned long *pos = std::lower_bound( first, last, n );
    return pos == last ? *( last - 1 ) : *pos;
  }

}

/** The "about KMail" text. */
extern const char* aboutText;
#endif
