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
