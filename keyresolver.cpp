/*  -*- c++ -*-
    keyresolver.cpp

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klarälvdalens Datakonsult AB

    Based on kpgp.cpp
    Copyright (C) 2001,2002 the KPGP authors
    See file libkdenetwork/AUTHORS.kpgp for details

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "keyresolver.h"

#include "kcursorsaver.h"
#include "kleo_util.h"

#include <ui/keyselectiondialog.h>
#include <kleo/cryptobackendfactory.h>
#include <kleo/keylistjob.h>
#include <kleo/dn.h>

#include <gpgmepp/key.h>
#include <gpgmepp/keylistresult.h>

#include <kabc/stdaddressbook.h>
#include <klocale.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <kmessagebox.h>

#include <qstringlist.h>
#include <qtl.h>

#include <algorithm>
#include <memory>
#include <iterator>
#include <functional>
#include <map>
#include <set>
#include <iostream>

#include <time.h>

//
// some predicates to be used in STL algorithms:
//

static inline bool EmptyKeyList( const Kleo::KeyApprovalDialog::Item & item ) {
  return item.keys.empty();
}

static inline QString ItemDotAddress( const Kleo::KeyResolver::Item & item ) {
  return item.address;
}

static inline bool ApprovalNeeded( const Kleo::KeyResolver::Item & item ) {
  return item.pref == Kleo::UnknownPreference || item.pref == Kleo::NeverEncrypt || item.keys.empty() ;
}

static inline Kleo::KeyResolver::Item
CopyKeysAndEncryptionPreferences( const Kleo::KeyResolver::Item & oldItem,
				  const Kleo::KeyApprovalDialog::Item & newItem ) {
  return Kleo::KeyResolver::Item( oldItem.address, newItem.keys, newItem.pref, oldItem.signPref, oldItem.format );
}

static inline bool ByKeyID( const GpgME::Key & left, const GpgME::Key & right ) {
  return qstrcmp( left.keyID(), right.keyID() ) < 0 ;
}

static inline bool WithRespectToKeyID( const GpgME::Key & left, const GpgME::Key & right ) {
  return qstrcmp( left.keyID(), right.keyID() ) == 0 ;
}

static bool ValidTrustedOpenPGPEncryptionKey( const GpgME::Key & key ) {
  if ( key.protocol() != GpgME::Context::OpenPGP ) {
    return false;
  }
#if 0
  if ( key.isRevoked() )
    kdWarning() << " is revoked" << endl;
  if ( key.isExpired() )
    kdWarning() << " is expired" << endl;
  if ( key.isDisabled() )
    kdWarning() << " is disabled" << endl;
  if ( !key.canEncrypt() )
    kdWarning() << " can't encrypt" << endl;
#endif
  if ( key.isRevoked() || key.isExpired() || key.isDisabled() || !key.canEncrypt() )
    return false;
  const std::vector<GpgME::UserID> uids = key.userIDs();
  for ( std::vector<GpgME::UserID>::const_iterator it = uids.begin() ; it != uids.end() ; ++it ) {
    if ( !it->isRevoked() && it->validity() >= GpgME::UserID::Marginal )
      return true;
#if 0
    else
      if ( it->isRevoked() )
        kdWarning() << "a userid is revoked" << endl;
      else
        kdWarning() << "bad validity " << it->validity() << endl;
#endif
  }
  return false;
}

static bool ValidTrustedSMIMEEncryptionKey( const GpgME::Key & key ) {
  if ( key.protocol() != GpgME::Context::CMS )
    return false;
  if ( key.isRevoked() || key.isExpired() || key.isDisabled() || !key.canEncrypt() )
    return false;
  return true;
}

static inline bool ValidTrustedEncryptionKey( const GpgME::Key & key ) {
  switch ( key.protocol() ) {
  case GpgME::Context::OpenPGP:
    return ValidTrustedOpenPGPEncryptionKey( key );
  case GpgME::Context::CMS:
    return ValidTrustedSMIMEEncryptionKey( key );
  default:
    return false;
  }
}

static inline bool ValidSigningKey( const GpgME::Key & key ) {
  if ( key.isRevoked() || key.isExpired() || key.isDisabled() || !key.canSign() )
    return false;
  return key.hasSecret();
}

static inline bool ValidOpenPGPSigningKey( const GpgME::Key & key ) {
  return key.protocol() == GpgME::Context::OpenPGP && ValidSigningKey( key );
}

static inline bool ValidSMIMESigningKey( const GpgME::Key & key ) {
  return key.protocol() == GpgME::Context::CMS && ValidSigningKey( key );
}

static inline bool NotValidTrustedOpenPGPEncryptionKey( const GpgME::Key & key ) {
  return !ValidTrustedOpenPGPEncryptionKey( key );
}

static inline bool NotValidTrustedSMIMEEncryptionKey( const GpgME::Key & key ) {
  return !ValidTrustedSMIMEEncryptionKey( key );
}

static inline bool NotValidTrustedEncryptionKey( const GpgME::Key & key ) {
  return !ValidTrustedEncryptionKey( key );
}

static inline bool NotValidSigningKey( const GpgME::Key & key ) {
  return !ValidSigningKey( key );
}

static inline bool NotValidOpenPGPSigningKey( const GpgME::Key & key ) {
  return !ValidOpenPGPSigningKey( key );
}

static inline bool NotValidSMIMESigningKey( const GpgME::Key & key ) {
  return !ValidSMIMESigningKey( key );
}

namespace {
  struct IsNotForFormat : public std::unary_function<GpgME::Key,bool> {
    IsNotForFormat( Kleo::CryptoMessageFormat f ) : format( f ) {}

    bool operator()( const GpgME::Key & key ) const {
      return
	( isOpenPGP( format ) && key.protocol() != GpgME::Context::OpenPGP ) ||
	( isSMIME( format )   && key.protocol() != GpgME::Context::CMS );
    }

    const Kleo::CryptoMessageFormat format;
  };
}



class Kleo::KeyResolver::SigningPreferenceCounter : public std::unary_function<Kleo::KeyResolver::Item,void> {
public:
  SigningPreferenceCounter()
    : mTotal( 0 ),
      mUnknownSigningPreference( 0 ),
      mNeverSign( 0 ),
      mAlwaysSign( 0 ),
      mAlwaysSignIfPossible( 0 ),
      mAlwaysAskForSigning( 0 ),
      mAskSigningWheneverPossible( 0 )
  {

  }
  void operator()( const Kleo::KeyResolver::Item & item );
#define make_int_accessor(x) unsigned int num##x() const { return m##x; }
  make_int_accessor(UnknownSigningPreference)
  make_int_accessor(NeverSign)
  make_int_accessor(AlwaysSign)
  make_int_accessor(AlwaysSignIfPossible)
  make_int_accessor(AlwaysAskForSigning)
  make_int_accessor(AskSigningWheneverPossible)
  make_int_accessor(Total)
#undef make_int_accessor
private:
  unsigned int mTotal;
  unsigned int mUnknownSigningPreference, mNeverSign, mAlwaysSign,
    mAlwaysSignIfPossible, mAlwaysAskForSigning, mAskSigningWheneverPossible;
};

void Kleo::KeyResolver::SigningPreferenceCounter::operator()( const Kleo::KeyResolver::Item & item ) {
  switch ( item.signPref ) {
#define CASE(x) case x: ++m##x; break
    CASE(UnknownSigningPreference);
    CASE(NeverSign);
    CASE(AlwaysSign);
    CASE(AlwaysSignIfPossible);
    CASE(AlwaysAskForSigning);
    CASE(AskSigningWheneverPossible);
#undef CASE
  }
  ++mTotal;
}



class Kleo::KeyResolver::EncryptionPreferenceCounter : public std::unary_function<Item,void> {
  const Kleo::KeyResolver * _this;
public:
  EncryptionPreferenceCounter( const Kleo::KeyResolver * kr, EncryptionPreference defaultPreference )
    : _this( kr ),
      mDefaultPreference( defaultPreference ),
      mTotal( 0 ),
      mNoKey( 0 ),
      mNeverEncrypt( 0 ),
      mUnknownPreference( 0 ),
      mAlwaysEncrypt( 0 ),
      mAlwaysEncryptIfPossible( 0 ),
      mAlwaysAskForEncryption( 0 ),
      mAskWheneverPossible( 0 )
  {

  }
  void operator()( Item & item );

#define make_int_accessor(x) unsigned int num##x() const { return m##x; }
  make_int_accessor(NoKey)
  make_int_accessor(NeverEncrypt)
  make_int_accessor(UnknownPreference)
  make_int_accessor(AlwaysEncrypt)
  make_int_accessor(AlwaysEncryptIfPossible)
  make_int_accessor(AlwaysAskForEncryption)
  make_int_accessor(AskWheneverPossible)
  make_int_accessor(Total)
#undef make_int_accessor
private:
  EncryptionPreference mDefaultPreference;
  unsigned int mTotal;
  unsigned int mNoKey;
  unsigned int mNeverEncrypt, mUnknownPreference, mAlwaysEncrypt,
    mAlwaysEncryptIfPossible, mAlwaysAskForEncryption, mAskWheneverPossible;
};

void Kleo::KeyResolver::EncryptionPreferenceCounter::operator()( Item & item ) {
  if ( item.needKeys )
    item.keys = _this->getEncryptionKeys( item.address, true );
  if ( item.keys.empty() ) {
    ++mNoKey;
    return;
  }
  switch ( !item.pref ? mDefaultPreference : item.pref ) {
#define CASE(x) case Kleo::x: ++m##x; break
    CASE(NeverEncrypt);
    CASE(UnknownPreference);
    CASE(AlwaysEncrypt);
    CASE(AlwaysEncryptIfPossible);
    CASE(AlwaysAskForEncryption);
    CASE(AskWheneverPossible);
#undef CASE
  }
  ++mTotal;
}

namespace {

  class FormatPreferenceCounterBase : public std::unary_function<Kleo::KeyResolver::Item,void> {
  public:
    FormatPreferenceCounterBase()
      : mTotal( 0 ),
	mInlineOpenPGP( 0 ),
	mOpenPGPMIME( 0 ),
	mSMIME( 0 ),
	mSMIMEOpaque( 0 )
    {

    }

#define make_int_accessor(x) unsigned int num##x() const { return m##x; }
    make_int_accessor(Total)
    make_int_accessor(InlineOpenPGP)
    make_int_accessor(OpenPGPMIME)
    make_int_accessor(SMIME)
    make_int_accessor(SMIMEOpaque)
#undef make_int_accessor

    unsigned int numOf( Kleo::CryptoMessageFormat f ) const {
      switch ( f ) {
#define CASE(x) case Kleo::x##Format: return m##x
	CASE(InlineOpenPGP);
	CASE(OpenPGPMIME);
	CASE(SMIME);
	CASE(SMIMEOpaque);
#undef CASE
      default: return 0;
      }
    }

  protected:
    unsigned int mTotal;
    unsigned int mInlineOpenPGP, mOpenPGPMIME, mSMIME, mSMIMEOpaque;
  };

  class EncryptionFormatPreferenceCounter : public FormatPreferenceCounterBase {
  public:
    EncryptionFormatPreferenceCounter() : FormatPreferenceCounterBase() {}
    void operator()( const Kleo::KeyResolver::Item & item );
  };

  class SigningFormatPreferenceCounter : public FormatPreferenceCounterBase {
  public:
    SigningFormatPreferenceCounter() : FormatPreferenceCounterBase() {}
    void operator()( const Kleo::KeyResolver::Item & item );
  };

#define CASE(x) if ( item.format & Kleo::x##Format ) ++m##x;
  void EncryptionFormatPreferenceCounter::operator()( const Kleo::KeyResolver::Item & item ) {
    if ( item.format & (Kleo::InlineOpenPGPFormat|Kleo::OpenPGPMIMEFormat) &&
	 std::find_if( item.keys.begin(), item.keys.end(),
		       ValidTrustedOpenPGPEncryptionKey ) != item.keys.end() ) {
      CASE(OpenPGPMIME);
      CASE(InlineOpenPGP);
    }
    if ( item.format & (Kleo::SMIMEFormat|Kleo::SMIMEOpaqueFormat) &&
	 std::find_if( item.keys.begin(), item.keys.end(),
		       ValidTrustedSMIMEEncryptionKey ) != item.keys.end() ) {
      CASE(SMIME);
      CASE(SMIMEOpaque);
    }
    ++mTotal;
  }

  void SigningFormatPreferenceCounter::operator()( const Kleo::KeyResolver::Item & item ) {
    CASE(InlineOpenPGP);
    CASE(OpenPGPMIME);
    CASE(SMIME);
    CASE(SMIMEOpaque);
    ++mTotal;
  };
#undef CASE

} // anon namespace

static QString canonicalAddress( const QString & _address ) {
  int openAngle, atSign, closeAngle;

  const QString address = _address.simplifyWhiteSpace().stripWhiteSpace();

  // just leave pure e-mail address.
  if((openAngle = address.find('<')) != -1)
    if((atSign = address.find('@',openAngle+1)) != -1)
      if((closeAngle = address.find('>',atSign+1)) != -1)
	return address.mid(openAngle+1,closeAngle-openAngle-1);

  if((atSign = address.find('@')) == -1)
  {
    // local address
    //char hostname[1024];
    //gethostname(hostname,1024);
    //return address + '@' + hostname;
    return address + "@localdomain";
  }
  else
  {
    int index1 = address.findRev(' ',openAngle);
    int index2 = address.find(' ',openAngle);
    if(index2 == -1) index2 = address.length();
    return address.mid(index1+1 ,index2-index1-1);
  }
}


struct FormatInfo {
  std::vector<Kleo::KeyResolver::SplitInfo> splitInfos;
  std::vector<GpgME::Key> signKeys;
};

struct Kleo::KeyResolver::Private {
  std::set<QCString> alreadyWarnedFingerprints;

  std::vector<GpgME::Key> mOpenPGPSigningKeys; // signing
  std::vector<GpgME::Key> mSMIMESigningKeys; // signing

  std::vector<GpgME::Key> mOpenPGPEncryptToSelfKeys; // encryption to self
  std::vector<GpgME::Key> mSMIMEEncryptToSelfKeys; // encryption to self

  std::vector<Item> mPrimaryEncryptionKeys; // encryption to To/CC
  std::vector<Item> mSecondaryEncryptionKeys; // encryption to BCC

  std::map<CryptoMessageFormat,FormatInfo> mFormatInfoMap;

  // key=email address, value=crypto preferences for this contact (from kabc)
  typedef std::map<QString, ContactPreferences> ContactPreferencesMap;
  ContactPreferencesMap mContactPreferencesMap;
};


Kleo::KeyResolver::KeyResolver( bool encToSelf, bool showApproval, bool oppEncryption,
				unsigned int f,
				int encrWarnThresholdKey, int signWarnThresholdKey,
				int encrWarnThresholdRootCert, int signWarnThresholdRootCert,
				int encrWarnThresholdChainCert, int signWarnThresholdChainCert )
  : mEncryptToSelf( encToSelf ),
    mShowApprovalDialog( showApproval ),
    mOpportunisticEncyption( oppEncryption ),
    mCryptoMessageFormats( f ),
    mEncryptKeyNearExpiryWarningThreshold( encrWarnThresholdKey ),
    mSigningKeyNearExpiryWarningThreshold( signWarnThresholdKey ),
    mEncryptRootCertNearExpiryWarningThreshold( encrWarnThresholdRootCert ),
    mSigningRootCertNearExpiryWarningThreshold( signWarnThresholdRootCert ),
    mEncryptChainCertNearExpiryWarningThreshold( encrWarnThresholdChainCert ),
    mSigningChainCertNearExpiryWarningThreshold( signWarnThresholdChainCert )
{
  d = new Private();
}

Kleo::KeyResolver::~KeyResolver() {
  delete d; d = 0;
}

Kpgp::Result Kleo::KeyResolver::checkKeyNearExpiry( const GpgME::Key & key, const char * dontAskAgainName,
						    bool mine, bool sign, bool ca,
						    int recur_limit, const GpgME::Key & orig ) const {
  if ( recur_limit <= 0 ) {
    kdDebug() << "Kleo::KeyResolver::checkKeyNearExpiry(): key chain too long (>100 certs)" << endl;
    return Kpgp::Ok;
  }
  const GpgME::Subkey subkey = key.subkey(0);
  if ( d->alreadyWarnedFingerprints.count( subkey.fingerprint() ) )
    return Kpgp::Ok; // already warned about this one (and so about it's issuers)
  d->alreadyWarnedFingerprints.insert( subkey.fingerprint() );

  if ( subkey.neverExpires() )
    return Kpgp::Ok;
  static const double secsPerDay = 24 * 60 * 60;
  const int daysTillExpiry =
    1 + int( ::difftime( subkey.expirationTime(), time(0) ) / secsPerDay );
  kdDebug() << "Key 0x" << key.shortKeyID() << " expires in less than "
	    << daysTillExpiry << " days" << endl;
  const int threshold =
    ca
    ? ( key.isRoot()
	? ( sign
	    ? signingRootCertNearExpiryWarningThresholdInDays()
	    : encryptRootCertNearExpiryWarningThresholdInDays() )
	: ( sign
	    ? signingChainCertNearExpiryWarningThresholdInDays()
	    : encryptChainCertNearExpiryWarningThresholdInDays() ) )
    : ( sign
	? signingKeyNearExpiryWarningThresholdInDays()
	: encryptKeyNearExpiryWarningThresholdInDays() );
  if ( threshold > -1 && daysTillExpiry <= threshold ) {
    const QString msg =
      key.protocol() == GpgME::Context::OpenPGP
      ? ( mine ? sign
	  ? i18n("<p>Your OpenPGP signing key</p><p align=center><b>%1</b> (KeyID 0x%2)</p>"
		 "<p>expires in less than a day.</p>",
		 "<p>Your OpenPGP signing key</p><p align=center><b>%1</b> (KeyID 0x%2)</p>"
		 "<p>expires in less than %n days.</p>",
		 daysTillExpiry )
	  : i18n("<p>Your OpenPGP encryption key</p><p align=center><b>%1</b> (KeyID 0x%2)</p>"
		 "<p>expires in less than a day.</p>",
		 "<p>Your OpenPGP encryption key</p><p align=center><b>%1</b> (KeyID 0x%2)</p>"
		 "<p>expires in less than %n days.</p>",
		 daysTillExpiry )
	  : i18n("<p>The OpenPGP key for</p><p align=center><b>%1</b> (KeyID 0x%2)</p>"
		 "<p>expires in less than a day.</p>",
		 "<p>The OpenPGP key for</p><p align=center><b>%1</b> (KeyID 0x%2)</p>"
		 "<p>expires in less than %n days.</p>",
		 daysTillExpiry ) ).arg( QString::fromUtf8( key.userID(0).id() ),
					 key.shortKeyID() )
      : ( ca
	  ? ( key.isRoot()
	      ? ( mine ? sign
		  ? i18n("<p>The root certificate</p><p align=center><b>%3</b></p>"
			 "<p>for your S/MIME signing certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than a day.</p>",
			 "<p>The root certificate</p><p align=center><b>%3</b></p>"
			 "<p>for your S/MIME signing certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than %n days.</p>",
			 daysTillExpiry )
		  : i18n("<p>The root certificate</p><p align=center><b>%3</b></p>"
			 "<p>for your S/MIME encryption certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than a day.</p>",
			 "<p>The root certificate</p><p align=center><b>%3</b></p>"
			 "<p>for your S/MIME encryption certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than %n days.</p>",
			 daysTillExpiry )
		  : i18n("<p>The root certificate</p><p align=center><b>%3</b></p>"
			 "<p>for S/MIME certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than a day.</p>",
			 "<p>The root certificate</p><p align=center><b>%3</b></p>"
			 "<p>for S/MIME certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than %n days.</p>",
			 daysTillExpiry ) )
	      : ( mine ? sign
		  ? i18n("<p>The intermediate CA certificate</p><p align=center><b>%3</b></p>"
			 "<p>for your S/MIME signing certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than a day.</p>",
			 "<p>The intermediate CA certificate</p><p align=center><b>%3</b></p>"
			 "<p>for your S/MIME signing certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than %n days.</p>",
			 daysTillExpiry )
		  : i18n("<p>The intermediate CA certificate</p><p align=center><b>%3</b></p>"
			 "<p>for your S/MIME encryption certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than a day.</p>",
			 "<p>The intermediate CA certificate</p><p align=center><b>%3</b></p>"
			 "<p>for your S/MIME encryption certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than %n days.</p>",
			 daysTillExpiry )
		  : i18n("<p>The intermediate CA certificate</p><p align=center><b>%3</b></p>"
			 "<p>for S/MIME certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than a day.</p>",
			 "<p>The intermediate CA certificate</p><p align=center><b>%3</b></p>"
			 "<p>for S/MIME certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
			 "<p>expires in less than %n days.</p>",
			 daysTillExpiry ) ) ).arg( Kleo::DN( orig.userID(0).id() ).prettyDN(),
						   orig.issuerSerial(),
						   Kleo::DN( key.userID(0).id() ).prettyDN() )
	  : ( mine ? sign
	      ? i18n("<p>Your S/MIME signing certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
		     "<p>expires in less than a day.</p>",
		     "<p>Your S/MIME signing certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
		     "<p>expires in less than %n days.</p>",
		     daysTillExpiry )
	      : i18n("<p>Your S/MIME encryption certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
		     "<p>expires in less than a day.</p>",
		     "<p>Your S/MIME encryption certificate</p><p align=center><b>%1</b> (serial number %2)</p>"
		     "<p>expires in less than %n days.</p>",
		     daysTillExpiry )
	      : i18n("<p>The S/MIME certificate for</p><p align=center><b>%1</b> (serial number %2)</p>"
		     "<p>expires in less than a day.</p>",
		     "<p>The S/MIME certificate for</p><p align=center><b>%1</b> (serial number %2)</p>"
		     "<p>expires in less than %n days.</p>",
		     daysTillExpiry ) ).arg( Kleo::DN( key.userID(0).id() ).prettyDN(),
					     key.issuerSerial() ) );
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     key.protocol() == GpgME::Context::OpenPGP
					     ? i18n("OpenPGP Key Expires Soon" )
					     : i18n("S/MIME Certificate Expires Soon" ),
					     KStdGuiItem::cont(), dontAskAgainName )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
  }
  if ( key.isRoot() )
    return Kpgp::Ok;
  else if ( const char * chain_id = key.chainID() ) {
    const std::vector<GpgME::Key> issuer = lookup( chain_id, false );
    if ( issuer.empty() )
      return Kpgp::Ok;
    else
      return checkKeyNearExpiry( issuer.front(), dontAskAgainName, mine, sign,
				 true, recur_limit-1, ca ? orig : key );
  }
  return Kpgp::Ok;
}

Kpgp::Result Kleo::KeyResolver::setEncryptToSelfKeys( const QStringList & fingerprints ) {
  if ( !encryptToSelf() )
    return Kpgp::Ok;

  std::vector<GpgME::Key> keys = lookup( fingerprints );
  std::remove_copy_if( keys.begin(), keys.end(),
		       std::back_inserter( d->mOpenPGPEncryptToSelfKeys ),
		       NotValidTrustedOpenPGPEncryptionKey );
  std::remove_copy_if( keys.begin(), keys.end(),
		       std::back_inserter( d->mSMIMEEncryptToSelfKeys ),
		       NotValidTrustedSMIMEEncryptionKey );

  if ( d->mOpenPGPEncryptToSelfKeys.size() + d->mSMIMEEncryptToSelfKeys.size()
       < keys.size() ) {
    // too few keys remain...
    const QString msg = i18n("One or more of your configured OpenPGP encryption "
			     "keys or S/MIME certificates is not usable for "
			     "encryption. Please reconfigure your encryption keys "
			     "and certificates for this identity in the identity "
			     "configuration dialog.\n"
			     "If you choose to continue, and the keys are needed "
			     "later on, you will be prompted to specify the keys "
			     "to use.");
    return KMessageBox::warningContinueCancel( 0, msg, i18n("Unusable Encryption Keys"),
					       KStdGuiItem::cont(),
					       "unusable own encryption key warning" )
      == KMessageBox::Continue ? Kpgp::Ok : Kpgp::Canceled ;
  }

  // check for near-expiry:

  for ( std::vector<GpgME::Key>::const_iterator it = d->mOpenPGPEncryptToSelfKeys.begin() ; it != d->mOpenPGPEncryptToSelfKeys.end() ; ++it ) {
    const Kpgp::Result r = checkKeyNearExpiry( *it, "own encryption key expires soon warning",
					       true, false );
    if ( r != Kpgp::Ok )
      return r;
  }

  for ( std::vector<GpgME::Key>::const_iterator it = d->mSMIMEEncryptToSelfKeys.begin() ; it != d->mSMIMEEncryptToSelfKeys.end() ; ++it ) {
    const Kpgp::Result r = checkKeyNearExpiry( *it, "own encryption key expires soon warning",
					       true, false );
    if ( r != Kpgp::Ok )
      return r;
  }

  return Kpgp::Ok;
}

Kpgp::Result Kleo::KeyResolver::setSigningKeys( const QStringList & fingerprints ) {
  std::vector<GpgME::Key> keys = lookup( fingerprints, true ); // secret keys
  std::remove_copy_if( keys.begin(), keys.end(),
		       std::back_inserter( d->mOpenPGPSigningKeys ),
		       NotValidOpenPGPSigningKey );
  std::remove_copy_if( keys.begin(), keys.end(),
		       std::back_inserter( d->mSMIMESigningKeys ),
		       NotValidSMIMESigningKey );

  if ( d->mOpenPGPSigningKeys.size() + d->mSMIMESigningKeys.size() < keys.size() ) {
    // too few keys remain...
    const QString msg = i18n("One or more of your configured OpenPGP signing keys "
			     "or S/MIME signing certificates is not usable for "
			     "signing. Please reconfigure your signing keys "
			     "and certificates for this identity in the identity "
			     "configuration dialog.\n"
			     "If you choose to continue, and the keys are needed "
			     "later on, you will be prompted to specify the keys "
			     "to use.");
    return KMessageBox::warningContinueCancel( 0, msg, i18n("Unusable Signing Keys"),
					       KStdGuiItem::cont(),
					       "unusable signing key warning" )
      == KMessageBox::Continue ? Kpgp::Ok : Kpgp::Canceled ;
  }

  // check for near expiry:

  for ( std::vector<GpgME::Key>::const_iterator it = d->mOpenPGPSigningKeys.begin() ; it != d->mOpenPGPSigningKeys.end() ; ++it ) {
    const Kpgp::Result r = checkKeyNearExpiry( *it, "signing key expires soon warning",
					       true, true );
    if ( r != Kpgp::Ok )
      return r;
  }

  for ( std::vector<GpgME::Key>::const_iterator it = d->mSMIMESigningKeys.begin() ; it != d->mSMIMESigningKeys.end() ; ++it ) {
    const Kpgp::Result r = checkKeyNearExpiry( *it, "signing key expires soon warning",
					       true, true );
    if ( r != Kpgp::Ok )
      return r;
  }

  return Kpgp::Ok;
}

void Kleo::KeyResolver::setPrimaryRecipients( const QStringList & addresses ) {
  d->mPrimaryEncryptionKeys = getEncryptionItems( addresses );
}

void Kleo::KeyResolver::setSecondaryRecipients( const QStringList & addresses ) {
  d->mSecondaryEncryptionKeys = getEncryptionItems( addresses );
}

std::vector<Kleo::KeyResolver::Item> Kleo::KeyResolver::getEncryptionItems( const QStringList & addresses ) {
  std::vector<Item> items;
  items.reserve( addresses.size() );
  for ( QStringList::const_iterator it = addresses.begin() ; it != addresses.end() ; ++it ) {
    QString addr = canonicalAddress( *it ).lower();
    ContactPreferences& pref = lookupContactPreferences( addr );

    items.push_back( Item( *it, /*getEncryptionKeys( *it, true ),*/
			   pref.encryptionPreference,
			   pref.signingPreference,
			   pref.cryptoMessageFormat ) );
  }
  return items;
}

static Kleo::Action action( bool doit, bool ask, bool dont, bool requested ) {
  if ( requested && !dont )
    return Kleo::DoIt;
  if ( doit && !ask && !dont )
    return Kleo::DoIt;
  if ( !doit && ask && !dont )
    return Kleo::Ask;
  if ( !doit && !ask && dont )
    return requested ? Kleo::Conflict : Kleo::DontDoIt ;
  if ( !doit && !ask && !dont )
    return Kleo::DontDoIt ;
  return Kleo::Conflict;
}

Kleo::Action Kleo::KeyResolver::checkSigningPreferences( bool signingRequested ) const {

  if ( signingRequested && d->mOpenPGPSigningKeys.empty() && d->mSMIMESigningKeys.empty() )
    return Impossible;

  SigningPreferenceCounter count;
  count = std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
			 count );
  count = std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
			 count );

  unsigned int sign = count.numAlwaysSign();
  unsigned int ask = count.numAlwaysAskForSigning();
  const unsigned int dontSign = count.numNeverSign();
  if ( signingPossible() ) {
    sign += count.numAlwaysSignIfPossible();
    ask += count.numAskSigningWheneverPossible();
  }

  return action( sign, ask, dontSign, signingRequested );
}

bool Kleo::KeyResolver::signingPossible() const {
  return !d->mOpenPGPSigningKeys.empty() || !d->mSMIMESigningKeys.empty() ;
}

Kleo::Action Kleo::KeyResolver::checkEncryptionPreferences( bool encryptionRequested ) const {

  if ( d->mPrimaryEncryptionKeys.empty() && d->mSecondaryEncryptionKeys.empty() )
    return DontDoIt;

  if ( encryptionRequested && encryptToSelf() &&
       d->mOpenPGPEncryptToSelfKeys.empty() && d->mSMIMEEncryptToSelfKeys.empty() )
    return Impossible;

  EncryptionPreferenceCounter count( this, mOpportunisticEncyption ? AskWheneverPossible : UnknownPreference );
  count = std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
			 count );
  count = std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
			 count );

  unsigned int encrypt = count.numAlwaysEncrypt();
  unsigned int ask = count.numAlwaysAskForEncryption();
  const unsigned int dontEncrypt = count.numNeverEncrypt() + count.numNoKey();
  if ( encryptionPossible() ) {
    encrypt += count.numAlwaysEncryptIfPossible();
    ask += count.numAskWheneverPossible();
  }

  const Action act = action( encrypt, ask, dontEncrypt, encryptionRequested );
  if ( act != Ask ||
       std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
       std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		      EncryptionPreferenceCounter( this, UnknownPreference ) ) ).numAlwaysAskForEncryption() )
    return act;
  else
    return AskOpportunistic;
}

bool Kleo::KeyResolver::encryptionPossible() const {
  return std::find_if( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		       EmptyKeyList ) == d->mPrimaryEncryptionKeys.end()
    &&   std::find_if( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		       EmptyKeyList ) == d->mSecondaryEncryptionKeys.end() ;
}

Kpgp::Result Kleo::KeyResolver::resolveAllKeys( bool signingRequested, bool encryptionRequested ) {
  if ( !encryptionRequested && !signingRequested ) {
    // make a dummy entry with all recipients, but no signing or
    // encryption keys to avoid special-casing on the caller side:
    dump();
    d->mFormatInfoMap[OpenPGPMIMEFormat].splitInfos.push_back( SplitInfo( allRecipients() ) );
    dump();
    return Kpgp::Ok;
  }
  Kpgp::Result result = Kpgp::Ok;
  if ( encryptionRequested )
    result = resolveEncryptionKeys( signingRequested );
  if ( result != Kpgp::Ok )
    return result;
  if ( signingRequested )
    if ( encryptionRequested )
      result = resolveSigningKeysForEncryption();
    else
      result = resolveSigningKeysForSigningOnly();
  return result;
}

Kpgp::Result Kleo::KeyResolver::resolveEncryptionKeys( bool signingRequested ) {
  //
  // 1. Get keys for all recipients:
  //

  for ( std::vector<Item>::iterator it = d->mPrimaryEncryptionKeys.begin() ; it != d->mPrimaryEncryptionKeys.end() ; ++it ) {
    if ( !it->needKeys )
      continue;
    it->keys = getEncryptionKeys( it->address, false );
    if ( it->keys.empty() )
      return Kpgp::Canceled;
    QString addr = canonicalAddress( it->address ).lower();
    ContactPreferences& pref = lookupContactPreferences( addr );
    it->pref = pref.encryptionPreference;
    it->signPref = pref.signingPreference;
    it->format = pref.cryptoMessageFormat;
  }

  for ( std::vector<Item>::iterator it = d->mSecondaryEncryptionKeys.begin() ; it != d->mSecondaryEncryptionKeys.end() ; ++it ) {
    if ( !it->needKeys )
      continue;
    it->keys = getEncryptionKeys( it->address, false );
    if ( it->keys.empty() )
      return Kpgp::Canceled;
    QString addr = canonicalAddress( it->address ).lower();
    ContactPreferences& pref = lookupContactPreferences( addr );
    it->pref = pref.encryptionPreference;
    it->signPref = pref.signingPreference;
    it->format = pref.cryptoMessageFormat;
  }

  // 1a: Present them to the user

  const Kpgp::Result res = showKeyApprovalDialog();
  if ( res != Kpgp::Ok )
    return res;

  //
  // 2. Check what the primary recipients need
  //

  // 2a. Try to find a common format for all primary recipients,
  //     else use as many formats as needed

  const EncryptionFormatPreferenceCounter primaryCount
    = std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		     EncryptionFormatPreferenceCounter() );

  CryptoMessageFormat commonFormat = AutoFormat;
  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    if ( !( concreteCryptoMessageFormats[i] & mCryptoMessageFormats ) )
      continue;
    if ( signingRequested && signingKeysFor( concreteCryptoMessageFormats[i] ).empty() )
      continue;
    if ( encryptToSelf() && encryptToSelfKeysFor( concreteCryptoMessageFormats[i] ).empty() )
      continue;
    if ( primaryCount.numOf( concreteCryptoMessageFormats[i] ) == primaryCount.numTotal() ) {
      commonFormat = concreteCryptoMessageFormats[i];
      break;
    }
  }
  if ( commonFormat != AutoFormat )
    addKeys( d->mPrimaryEncryptionKeys, commonFormat );
  else
    addKeys( d->mPrimaryEncryptionKeys );

  collapseAllSplitInfos(); // these can be encrypted together

  // 2b. Just try to find _something_ for each secondary recipient,
  //     with a preference to a common format (if that exists)

  const EncryptionFormatPreferenceCounter secondaryCount
    = std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		     EncryptionFormatPreferenceCounter() );

  if ( commonFormat != AutoFormat &&
       secondaryCount.numOf( commonFormat ) == secondaryCount.numTotal() )
    addKeys( d->mSecondaryEncryptionKeys, commonFormat );
  else
    addKeys( d->mSecondaryEncryptionKeys );

  // 3. Check for expiry:

  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    const std::vector<SplitInfo> si_list = encryptionItems( concreteCryptoMessageFormats[i] );
    for ( std::vector<SplitInfo>::const_iterator sit = si_list.begin() ; sit != si_list.end() ; ++sit )
      for ( std::vector<GpgME::Key>::const_iterator kit = sit->keys.begin() ; kit != sit->keys.end() ; ++kit ) {
	const Kpgp::Result r = checkKeyNearExpiry( *kit, "other encryption key near expiry warning",
						   false, false );
	if ( r != Kpgp::Ok )
	  return r;
      }
  }

  // 4. Check that we have the right keys for encryptToSelf()

  if ( !encryptToSelf() )
    return Kpgp::Ok;

  // 4a. Check for OpenPGP keys

  if ( !encryptionItems( InlineOpenPGPFormat ).empty() ||
       !encryptionItems( OpenPGPMIMEFormat ).empty() ) {
    // need them
    if ( d->mOpenPGPEncryptToSelfKeys.empty() ) {
      const QString msg = i18n("Examination of recipient's encryption preferences "
			       "yielded that the message should be encrypted using "
			       "OpenPGP, at least for some recipients;\n"
			       "however, you have not configured valid trusted "
			       "OpenPGP encryption keys for this identity.\n"
			       "You may continue without encrypting to yourself, "
			       "but be aware that you will not be able to read your "
			       "own messages if you do so.");
      if ( KMessageBox::warningContinueCancel( 0, msg,
					       i18n("Unusable Encryption Keys"),
					       KStdGuiItem::cont(),
					       "encrypt-to-self will fail warning" )
	   == KMessageBox::Cancel )
	return Kpgp::Canceled;
      // FIXME: Allow selection
    }
    addToAllSplitInfos( d->mOpenPGPEncryptToSelfKeys,
			InlineOpenPGPFormat|OpenPGPMIMEFormat );
  }

  // 4b. Check for S/MIME certs:

  if ( !encryptionItems( SMIMEFormat ).empty() ||
       !encryptionItems( SMIMEOpaqueFormat ).empty() ) {
    // need them
    if ( d->mSMIMEEncryptToSelfKeys.empty() ) {
      // don't have one
      const QString msg = i18n("Examination of recipient's encryption preferences "
			       "yielded that the message should be encrypted using "
			       "S/MIME, at least for some recipients;\n"
			       "however, you have not configured valid "
			       "S/MIME encryption certificates for this identity.\n"
			       "You may continue without encrypting to yourself, "
			       "but be aware that you will not be able to read your "
			       "own messages if you do so.");
      if ( KMessageBox::warningContinueCancel( 0, msg,
					       i18n("Unusable Encryption Keys"),
					       KStdGuiItem::cont(),
					       "encrypt-to-self will fail warning" )
	   == KMessageBox::Cancel )
	return Kpgp::Canceled;
      // FIXME: Allow selection
    }
    addToAllSplitInfos( d->mSMIMEEncryptToSelfKeys,
			SMIMEFormat|SMIMEOpaqueFormat );
  }

  // FIXME: Present another message if _both_ OpenPGP and S/MIME keys
  // are missing.

  return Kpgp::Ok;
}

Kpgp::Result Kleo::KeyResolver::resolveSigningKeysForEncryption() {
  if ( ( !encryptionItems( InlineOpenPGPFormat ).empty() ||
	 !encryptionItems( OpenPGPMIMEFormat ).empty() )
       && d->mOpenPGPSigningKeys.empty() ) {
    const QString msg = i18n("Examination of recipient's signing preferences "
			     "yielded that the message should be signed using "
			     "OpenPGP, at least for some recipients;\n"
			     "however, you have not configured valid "
			     "OpenPGP signing certificates for this identity.");
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     i18n("Unusable Signing Keys"),
					     i18n("Do Not OpenPGP-Sign"),
					     "signing will fail warning" )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
    // FIXME: Allow selection
  }
  if ( ( !encryptionItems( SMIMEFormat ).empty() ||
	 !encryptionItems( SMIMEOpaqueFormat ).empty() )
       && d->mSMIMESigningKeys.empty() ) {
    const QString msg = i18n("Examination of recipient's signing preferences "
			     "yielded that the message should be signed using "
			     "S/MIME, at least for some recipients;\n"
			     "however, you have not configured valid "
			     "S/MIME signing certificates for this identity.");
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     i18n("Unusable Signing Keys"),
					     i18n("Do Not S/MIME-Sign"),
					     "signing will fail warning" )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
    // FIXME: Allow selection
  }

  // FIXME: Present another message if _both_ OpenPGP and S/MIME keys
  // are missing.

  for ( std::map<CryptoMessageFormat,FormatInfo>::iterator it = d->mFormatInfoMap.begin() ; it != d->mFormatInfoMap.end() ; ++it )
    if ( !it->second.splitInfos.empty() ) {
      dump();
      it->second.signKeys = signingKeysFor( it->first );
      dump();
    }

  return Kpgp::Ok;
}

Kpgp::Result Kleo::KeyResolver::resolveSigningKeysForSigningOnly() {
  //
  // we don't need to distinguish between primary and secondary
  // recipients here:
  //
  SigningFormatPreferenceCounter count;
  count = std::for_each( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
			 count );
  count = std::for_each( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
			 count );

  // try to find a common format that works for all (and that we have signing keys for):

  CryptoMessageFormat commonFormat = AutoFormat;

  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    if ( !( concreteCryptoMessageFormats[i] & mCryptoMessageFormats ) )
      continue;
    if ( signingKeysFor( concreteCryptoMessageFormats[i] ).empty() )
      continue; // skip;
    if ( count.numOf( concreteCryptoMessageFormats[i] ) == count.numTotal() ) {
      commonFormat = concreteCryptoMessageFormats[i];
      break;
    }
  }

  if ( commonFormat != AutoFormat ) { // found
    dump();
    FormatInfo & fi = d->mFormatInfoMap[ commonFormat ];
    fi.signKeys = signingKeysFor( commonFormat );
    fi.splitInfos.resize( 1 );
    fi.splitInfos.front() = SplitInfo( allRecipients() );
    dump();
    return Kpgp::Ok;
  }

  return Kpgp::Failure;
}

std::vector<GpgME::Key> Kleo::KeyResolver::signingKeysFor( CryptoMessageFormat f ) const {
  if ( isOpenPGP( f ) )
    return d->mOpenPGPSigningKeys;
  if ( isSMIME( f ) )
    return d->mSMIMESigningKeys;
  return std::vector<GpgME::Key>();
}

std::vector<GpgME::Key> Kleo::KeyResolver::encryptToSelfKeysFor( CryptoMessageFormat f ) const {
  if ( isOpenPGP( f ) )
    return d->mOpenPGPEncryptToSelfKeys;
  if ( isSMIME( f ) )
    return d->mSMIMEEncryptToSelfKeys;
  return std::vector<GpgME::Key>();
}

QStringList Kleo::KeyResolver::allRecipients() const {
  QStringList result;
  std::transform( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		  std::back_inserter( result ), ItemDotAddress );
  std::transform( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		  std::back_inserter( result ), ItemDotAddress );
  return result;
}

void Kleo::KeyResolver::collapseAllSplitInfos() {
  dump();
  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    std::map<CryptoMessageFormat,FormatInfo>::iterator pos =
      d->mFormatInfoMap.find( concreteCryptoMessageFormats[i] );
    if ( pos == d->mFormatInfoMap.end() )
      continue;
    std::vector<SplitInfo> & v = pos->second.splitInfos;
    if ( v.size() < 2 )
      continue;
    SplitInfo & si = v.front();
    for ( std::vector<SplitInfo>::const_iterator it = v.begin() + 1; it != v.end() ; ++it ) {
      si.keys.insert( si.keys.end(), it->keys.begin(), it->keys.end() );
      qCopy( it->recipients.begin(), it->recipients.end(), std::back_inserter( si.recipients ) );
    }
    v.resize( 1 );
  }
  dump();
}

void Kleo::KeyResolver::addToAllSplitInfos( const std::vector<GpgME::Key> & keys, unsigned int f ) {
  dump();
  if ( !f || keys.empty() )
    return;
  for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i ) {
    if ( !( f & concreteCryptoMessageFormats[i] ) )
      continue;
    std::map<CryptoMessageFormat,FormatInfo>::iterator pos =
      d->mFormatInfoMap.find( concreteCryptoMessageFormats[i] );
    if ( pos == d->mFormatInfoMap.end() )
      continue;
    std::vector<SplitInfo> & v = pos->second.splitInfos;
    for ( std::vector<SplitInfo>::iterator it = v.begin() ; it != v.end() ; ++it )
      it->keys.insert( it->keys.end(), keys.begin(), keys.end() );
  }
  dump();
}

void Kleo::KeyResolver::dump() const {
#ifndef NDEBUG
  if ( d->mFormatInfoMap.empty() )
    std::cerr << "Keyresolver: Format info empty" << std::endl;
  for ( std::map<CryptoMessageFormat,FormatInfo>::const_iterator it = d->mFormatInfoMap.begin() ; it != d->mFormatInfoMap.end() ; ++it ) {
    std::cerr << "Format info for " << Kleo::cryptoMessageFormatToString( it->first )
	      << ":" << std::endl
	      << "  Signing keys: ";
    for ( std::vector<GpgME::Key>::const_iterator sit = it->second.signKeys.begin() ; sit != it->second.signKeys.end() ; ++sit )
      std::cerr << sit->shortKeyID() << " ";
    std::cerr << std::endl;
    unsigned int i = 0;
    for ( std::vector<SplitInfo>::const_iterator sit = it->second.splitInfos.begin() ; sit != it->second.splitInfos.end() ; ++sit, ++i ) {
      std::cerr << "  SplitInfo #" << i << " encryption keys: ";
      for ( std::vector<GpgME::Key>::const_iterator kit = sit->keys.begin() ; kit != sit->keys.end() ; ++kit )
	std::cerr << kit->shortKeyID() << " ";
      std::cerr << std::endl
		<< "  SplitInfo #" << i << " recipients: "
		<< sit->recipients.join(", ").utf8() << std::endl;
    }
  }
#endif
}

Kpgp::Result Kleo::KeyResolver::showKeyApprovalDialog() {
  const bool showKeysForApproval = showApprovalDialog()
    || std::find_if( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		     ApprovalNeeded ) != d->mPrimaryEncryptionKeys.end()
    || std::find_if( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		     ApprovalNeeded ) != d->mSecondaryEncryptionKeys.end() ;

  if ( !showKeysForApproval )
    return Kpgp::Ok;

  std::vector<Kleo::KeyApprovalDialog::Item> items;
  items.reserve( d->mPrimaryEncryptionKeys.size() +
	         d->mSecondaryEncryptionKeys.size() );
  std::copy( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
	     std::back_inserter( items ) );
  std::copy( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
	     std::back_inserter( items ) );

  std::vector<GpgME::Key> senderKeys;
  senderKeys.reserve( d->mOpenPGPEncryptToSelfKeys.size() +
	              d->mSMIMEEncryptToSelfKeys.size() );
  std::copy( d->mOpenPGPEncryptToSelfKeys.begin(), d->mOpenPGPEncryptToSelfKeys.end(),
	     std::back_inserter( senderKeys ) );
  std::copy( d->mSMIMEEncryptToSelfKeys.begin(), d->mSMIMEEncryptToSelfKeys.end(),
	     std::back_inserter( senderKeys ) );

  const KCursorSaver idle( KBusyPtr::idle() );

  Kleo::KeyApprovalDialog dlg( items, senderKeys );

  if ( dlg.exec() == QDialog::Rejected )
    return Kpgp::Canceled;

  items = dlg.items();
  senderKeys = dlg.senderKeys();

  if ( dlg.preferencesChanged() ) {
    for ( uint i = 0; i < items.size(); ++i ) {
      ContactPreferences& pref = lookupContactPreferences( items[i].address );
      pref.encryptionPreference = items[i].pref;
      saveContactPreference( items[i].address, pref );
    }
  }

  // show a warning if the user didn't select an encryption key for
  // herself:
  if ( encryptToSelf() && senderKeys.empty() ) {
    const QString msg = i18n("You did not select an encryption key for yourself "
			     "(encrypt to self). You will not be able to decrypt "
			     "your own message if you encrypt it.");
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     i18n("Missing Key Warning"),
					     i18n("&Encrypt") )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
    else
      mEncryptToSelf = false;
  }

  // count empty key ID lists
  const unsigned int emptyListCount =
    std::count_if( items.begin(), items.end(), EmptyKeyList );

  // show a warning if the user didn't select an encryption key for
  // some of the recipients
  if ( items.size() == emptyListCount  ) {
    const QString msg = ( d->mPrimaryEncryptionKeys.size() +
			  d->mSecondaryEncryptionKeys.size() == 1 )
                  ? i18n("You did not select an encryption key for the "
                         "recipient of this message; therefore, the message "
                         "will not be encrypted.")
                  : i18n("You did not select an encryption key for any of the "
                         "recipients of this message; therefore, the message "
                         "will not be encrypted.");
    if ( KMessageBox::warningContinueCancel( 0, msg,
					     i18n("Missing Key Warning"),
					     i18n("Send &Unencrypted") )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
  } else if ( emptyListCount > 0 ) {
    const QString msg = ( emptyListCount == 1 )
                  ? i18n("You did not select an encryption key for one of "
                         "the recipients: this person will not be able to "
                         "decrypt the message if you encrypt it.")
                  : i18n("You did not select encryption keys for some of "
                         "the recipients: these persons will not be able to "
                         "decrypt the message if you encrypt it." );
    KCursorSaver idle( KBusyPtr::idle() );
    if ( KMessageBox::warningContinueCancel( 0, msg,
						 i18n("Missing Key Warning"),
						 i18n("&Encrypt") )
	 == KMessageBox::Cancel )
      return Kpgp::Canceled;
  }

  std::transform( d->mPrimaryEncryptionKeys.begin(), d->mPrimaryEncryptionKeys.end(),
		  items.begin(),
		  d->mPrimaryEncryptionKeys.begin(),
		  CopyKeysAndEncryptionPreferences );
  std::transform( d->mSecondaryEncryptionKeys.begin(), d->mSecondaryEncryptionKeys.end(),
		  items.begin() + d->mPrimaryEncryptionKeys.size(),
		  d->mSecondaryEncryptionKeys.begin(),
		  CopyKeysAndEncryptionPreferences );

  d->mOpenPGPEncryptToSelfKeys.clear();
  d->mSMIMEEncryptToSelfKeys.clear();

  std::remove_copy_if( senderKeys.begin(), senderKeys.end(),
		       std::back_inserter( d->mOpenPGPEncryptToSelfKeys ),
		       NotValidTrustedOpenPGPEncryptionKey );
  std::remove_copy_if( senderKeys.begin(), senderKeys.end(),
		       std::back_inserter( d->mSMIMEEncryptToSelfKeys ),
		       NotValidTrustedSMIMEEncryptionKey );

  return Kpgp::Ok;
}

std::vector<Kleo::KeyResolver::SplitInfo> Kleo::KeyResolver::encryptionItems( Kleo::CryptoMessageFormat f ) const {
  dump();
  std::map<CryptoMessageFormat,FormatInfo>::const_iterator it =
    d->mFormatInfoMap.find( f );
  return it != d->mFormatInfoMap.end() ? it->second.splitInfos : std::vector<SplitInfo>() ;
}

std::vector<GpgME::Key> Kleo::KeyResolver::signingKeys( CryptoMessageFormat f ) const {
  dump();
  std::map<CryptoMessageFormat,FormatInfo>::const_iterator it =
    d->mFormatInfoMap.find( f );
  return it != d->mFormatInfoMap.end() ? it->second.signKeys : std::vector<GpgME::Key>() ;
}

//
//
// Private helper methods below:
//
//


std::vector<GpgME::Key> Kleo::KeyResolver::selectKeys( const QString & person, const QString & msg, const std::vector<GpgME::Key> & selectedKeys ) const {
  Kleo::KeySelectionDialog dlg( i18n("Encryption Key Selection"),
				msg, selectedKeys,
				Kleo::KeySelectionDialog::ValidTrustedEncryptionKeys,
				true, true ); // multi-selection and "remember choice" box

  if ( dlg.exec() != QDialog::Accepted )
    return std::vector<GpgME::Key>();
  const std::vector<GpgME::Key> keys = dlg.selectedKeys();
  if ( !keys.empty() && dlg.rememberSelection() )
    setKeysForAddress( person, dlg.pgpKeyFingerprints(), dlg.smimeFingerprints() );
  return keys;
}


std::vector<GpgME::Key> Kleo::KeyResolver::getEncryptionKeys( const QString & person, bool quiet ) const {

  const QString address = canonicalAddress( person ).lower();

  // First look for this person's address in the address->key dictionary
  const QStringList fingerprints = keysForAddress( address );

  if ( !fingerprints.empty() ) {
    kdDebug() << "Using encryption keys 0x"
	      << fingerprints.join( ", 0x" )
	      << " for " << person << endl;
    std::vector<GpgME::Key> keys = lookup( fingerprints );
    if ( !keys.empty() ) {
      // Check if all of the keys are trusted and valid encryption keys
      if ( std::find_if( keys.begin(), keys.end(),
			 NotValidTrustedEncryptionKey ) == keys.end() )
	return keys;

      // not ok, let the user select: this is not conditional on !quiet,
      // since it's a bug in the configuration and the user should be
      // notified about it as early as possible:
      keys = selectKeys( person,
			 i18n("if in your language something like "
			      "'key(s)' isn't possible please "
			      "use the plural in the translation",
			      "There is a problem with the "
			      "encryption key(s) for \"%1\".\n\n"
			      "Please re-select the key(s) which should "
			      "be used for this recipient.").arg(person),
			 keys );
      if ( !keys.empty() )
	return keys;
      // hmmm, should we not return the keys in any case here?
    }
  }

  // Now search all public keys for matching keys
  std::vector<GpgME::Key> matchingKeys = lookup( person );
  matchingKeys.erase( std::remove_if( matchingKeys.begin(), matchingKeys.end(),
				      NotValidTrustedEncryptionKey ),
		      matchingKeys.end() );
  // if no keys match the complete address look for keys which match
  // the canonical mail address
  if ( matchingKeys.empty() ) {
    matchingKeys = lookup( address );
    matchingKeys.erase( std::remove_if( matchingKeys.begin(), matchingKeys.end(),
					NotValidTrustedEncryptionKey ),
			matchingKeys.end() );
  }

  if ( quiet || matchingKeys.size() == 1 )
    return matchingKeys;

  // no match until now, or more than one key matches; let the user
  // choose the key(s)
  // FIXME: let user get the key from keyserver
  return selectKeys( person,
		     matchingKeys.empty()
		     ? i18n("if in your language something like "
			    "'key(s)' isn't possible please "
			    "use the plural in the translation",
			    "No valid and trusted encryption key was "
			    "found for \"%1\".\n\n"
			    "Select the key(s) which should "
			    "be used for this recipient.").arg(person)
		     : i18n("if in your language something like "
			    "'key(s)' isn't possible please "
			    "use the plural in the translation",
			    "More than one key matches \"%1\".\n\n"
			    "Select the key(s) which should "
			    "be used for this recipient.").arg(person),
		     matchingKeys );
}


std::vector<GpgME::Key> Kleo::KeyResolver::lookup( const QStringList & patterns, bool secret ) const {
  if ( patterns.empty() )
    return std::vector<GpgME::Key>();
  kdDebug() << "Kleo::KeyResolver::lookup( \"" << patterns.join( "\", \"" )
	    << "\", " << secret << " )" << endl;
  std::vector<GpgME::Key> result;
  if ( mCryptoMessageFormats & (InlineOpenPGPFormat|OpenPGPMIMEFormat) )
    if ( const Kleo::CryptoBackend::Protocol * p = Kleo::CryptoBackendFactory::instance()->openpgp() ) {
      std::auto_ptr<Kleo::KeyListJob> job( p->keyListJob( false, false, true ) ); // use validating keylisting
      if ( job.get() ) {
	std::vector<GpgME::Key> keys;
	job->exec( patterns, secret, keys );
	result.insert( result.end(), keys.begin(), keys.end() );
      }
    }
  if ( mCryptoMessageFormats & (SMIMEFormat|SMIMEOpaqueFormat) )
    if ( const Kleo::CryptoBackend::Protocol * p = Kleo::CryptoBackendFactory::instance()->smime() ) {
      std::auto_ptr<Kleo::KeyListJob> job( p->keyListJob( false, false, true ) ); // use validating keylisting
      if ( job.get() ) {
	std::vector<GpgME::Key> keys;
	job->exec( patterns, secret, keys );
	result.insert( result.end(), keys.begin(), keys.end() );
      }
    }
  kdDebug() << "  returned " << result.size() << " keys" << endl;
  return result;
}

void Kleo::KeyResolver::addKeys( const std::vector<Item> & items, CryptoMessageFormat f ) {
  dump();
  for ( std::vector<Item>::const_iterator it = items.begin() ; it != items.end() ; ++it ) {
    SplitInfo si( it->address );
    std::remove_copy_if( it->keys.begin(), it->keys.end(),
			 std::back_inserter( si.keys ), IsNotForFormat( f ) );
    dump();
    kdWarning( si.keys.empty() )
      << "Kleo::KeyResolver::addKeys(): Fix EncryptionFormatPreferenceCounter. "
      << "It detected a common format, but the list of such keys for recipient \""
      << it->address << "\" is empty!" << endl;
    d->mFormatInfoMap[ f ].splitInfos.push_back( si );
  }
  dump();
}

void Kleo::KeyResolver::addKeys( const std::vector<Item> & items ) {
  dump();
  for ( std::vector<Item>::const_iterator it = items.begin() ; it != items.end() ; ++it ) {
    SplitInfo si( it->address );
    CryptoMessageFormat f = AutoFormat;
    for ( unsigned int i = 0 ; i < numConcreteCryptoMessageFormats ; ++i )
      if ( mCryptoMessageFormats & concreteCryptoMessageFormats[i] & it->format ) {
	f = concreteCryptoMessageFormats[i];
	break;
      }
    if ( f == AutoFormat )
      kdWarning() << "Kleo::KeyResolver::addKeys(): Something went wrong. Didn't find a format for \""
		  << it->address << "\"" << endl;
    else
      std::remove_copy_if( it->keys.begin(), it->keys.end(),
			   std::back_inserter( si.keys ), IsNotForFormat( f ) );
    d->mFormatInfoMap[ f ].splitInfos.push_back( si );
  }
  dump();
}

Kleo::KeyResolver::ContactPreferences& Kleo::KeyResolver::lookupContactPreferences( const QString& address ) const
{
  Private::ContactPreferencesMap::iterator pos =
    d->mContactPreferencesMap.find( address );
  if ( pos == d->mContactPreferencesMap.end() ) {
    KABC::AddressBook *ab = KABC::StdAddressBook::self();
    KABC::Addressee::List res = ab->findByEmail( address );
    ContactPreferences pref;
    if ( !res.isEmpty() ) {
      KABC::Addressee addr = res.first();
      QString encryptPref = addr.custom( "KADDRESSBOOK", "CRYPTOENCRYPTPREF" );
      pref.encryptionPreference = Kleo::stringToEncryptionPreference( encryptPref );
      QString signPref = addr.custom( "KADDRESSBOOK", "CRYPTOSIGNPREF" );
      pref.signingPreference = Kleo::stringToSigningPreference( signPref );
      QString cryptoFormats = addr.custom( "KADDRESSBOOK", "CRYPTOPROTOPREF" );
      pref.cryptoMessageFormat = Kleo::stringToCryptoMessageFormat( cryptoFormats );
      pref.pgpKeyFingerprints = QStringList::split( ',', addr.custom( "KADDRESSBOOK", "OPENPGPFP" ) );
      pref.smimeCertFingerprints = QStringList::split( ',', addr.custom( "KADDRESSBOOK", "SMIMEFP" ) );
    }
    // insert into map and grab resulting iterator
    pos = d->mContactPreferencesMap.insert(
      Private::ContactPreferencesMap::value_type( address, pref ) ).first;
  }
  return (*pos).second;
}

void Kleo::KeyResolver::saveContactPreference( const QString& email, const ContactPreferences& pref ) const
{
  KABC::AddressBook *ab = KABC::StdAddressBook::self( true );
  KABC::Addressee::List res = ab->findByEmail( email );

  KABC::Addressee addr;
  if ( res.isEmpty() ) {
     bool ok = true;
     QString fullName = KInputDialog::getText( i18n( "Name Selection" ), i18n( "Which name shall the contact '%1' have in your addressbook?" ).arg( email ), QString::null, &ok );
    if ( ok ) {
      addr.setNameFromString( fullName );
      addr.insertEmail( email, true );
    } else
      return;
  } else
    addr = res.first();

  addr.insertCustom( "KADDRESSBOOK", "CRYPTOENCRYPTPREF", Kleo::encryptionPreferenceToString( pref.encryptionPreference ) );
  addr.insertCustom( "KADDRESSBOOK", "CRYPTOSIGNPREF", Kleo::signingPreferenceToString( pref.signingPreference ) );
  addr.insertCustom( "KADDRESSBOOK", "CRYPTOPROTOPREF", cryptoMessageFormatToString( pref.cryptoMessageFormat ) );
  addr.insertCustom( "KADDRESSBOOK", "OPENPGPFP", pref.pgpKeyFingerprints.join( "," ) );
  addr.insertCustom( "KADDRESSBOOK", "SMIMEFP", pref.smimeCertFingerprints.join( "," ) );

  ab->insertAddressee( addr );
  KABC::Ticket *ticket = ab->requestSaveTicket( addr.resource() );
  if ( ticket )
    ab->save( ticket );

  // Assumption: 'pref' comes from d->mContactPreferencesMap already, no need to update that
}

Kleo::KeyResolver::ContactPreferences::ContactPreferences()
  : encryptionPreference( UnknownPreference ),
    signingPreference( UnknownSigningPreference ),
    cryptoMessageFormat( AutoFormat )
{
}

QStringList Kleo::KeyResolver::keysForAddress( const QString & address ) const {
  if( address.isEmpty() ) {
    return QStringList();
  }
  QString addr = canonicalAddress( address ).lower();
  ContactPreferences& pref = lookupContactPreferences( addr );
  return pref.pgpKeyFingerprints + pref.smimeCertFingerprints;
}

void Kleo::KeyResolver::setKeysForAddress( const QString& address, const QStringList& pgpKeyFingerprints, const QStringList& smimeCertFingerprints ) const {
  if( address.isEmpty() ) {
    return;
  }
  QString addr = canonicalAddress( address ).lower();
  ContactPreferences& pref = lookupContactPreferences( addr );
  pref.pgpKeyFingerprints = pgpKeyFingerprints;
  pref.smimeCertFingerprints = smimeCertFingerprints;
  saveContactPreference( addr, pref );
}
