/*  -*- c++ -*-
    kleo_util.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KDEPIM_KMAIL_KLEO_UTIL_H__
#define __KDEPIM_KMAIL_KLEO_UTIL_H__

#include <kleo/enum.h>

static const Kleo::CryptoMessageFormat cryptoMessageFormats[] = {
  Kleo::AutoFormat,
  Kleo::InlineOpenPGPFormat,
  Kleo::OpenPGPMIMEFormat,
  Kleo::SMIMEFormat,
  Kleo::SMIMEOpaqueFormat
};
static const int numCryptoMessageFormats = sizeof cryptoMessageFormats / sizeof *cryptoMessageFormats ;

static const Kleo::CryptoMessageFormat concreteCryptoMessageFormats[] = {
  Kleo::OpenPGPMIMEFormat,
  Kleo::SMIMEFormat,
  Kleo::SMIMEOpaqueFormat,
  Kleo::InlineOpenPGPFormat
};
static const unsigned int numConcreteCryptoMessageFormats
  = sizeof concreteCryptoMessageFormats / sizeof *concreteCryptoMessageFormats ;

static inline Kleo::CryptoMessageFormat cb2format( int idx ) {
  return cryptoMessageFormats[ idx >= 0 && idx < numCryptoMessageFormats ? idx : 0 ];
}

static inline int format2cb( Kleo::CryptoMessageFormat f ) {
  for ( int i = 0 ; i < numCryptoMessageFormats ; ++i )
    if ( f == cryptoMessageFormats[i] )
      return i;
  return 0;
}


//
// some helper functions indicating the need for CryptoMessageFormat
// to be a class type :)
//

static inline bool isSMIME( Kleo::CryptoMessageFormat f ) {
  return f ==  Kleo::SMIMEFormat || f == Kleo::SMIMEOpaqueFormat ;
}

static inline bool isOpenPGP( Kleo::CryptoMessageFormat f ) {
  return f == Kleo::InlineOpenPGPFormat || f == Kleo::OpenPGPMIMEFormat ;
}


#endif // __KDEPIM_KMAIL_KLEO_UTIL_H__
