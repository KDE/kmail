/*  -*- mode: C++; c-file-style: "gnu" -*-
    identitymanager.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

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

// config keys:
static const char * configKeyDefaultIdentity = "Default Identity";

#include "identitymanager.h"

#include "kmidentity.h" // for IdentityList::{export,import}Data
#ifndef KMAIL_TESTING
#include "kmkernel.h"
#endif
#include "kmmessage.h" // for static KMMessage helper functions

#include <kemailsettings.h> // for IdentityEntry::fromControlCenter()
#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kuser.h>

#include <qregexp.h>

#include <assert.h>

IdentityManager::IdentityManager( QObject * parent, const char * name )
  : ConfigManager( parent, name )
{
  readConfig();
  mShadowIdentities = mIdentities;
  // we need at least a default identity:
  if ( mIdentities.isEmpty() ) {
    kdDebug( 5006 ) << "IdentityManager: No identity found. Creating default." << endl;
    createDefaultIdentity();
    commit();
  }
}

IdentityManager::~IdentityManager()
{
  kdWarning( hasPendingChanges(), 5006 )
    << "IdentityManager: There were uncommitted changes!" << endl;
}

void IdentityManager::commit()
{
  // early out:
  if ( !hasPendingChanges() ) return;

  QValueList<uint> seenUOIDs;
  for ( QValueList<KMIdentity>::ConstIterator it = mIdentities.begin() ;
	it != mIdentities.end() ; ++it )
    seenUOIDs << (*it).uoid();

  // find added and changed identities:
  for ( QValueList<KMIdentity>::ConstIterator it = mShadowIdentities.begin() ;
	it != mShadowIdentities.end() ; ++it ) {
    QValueList<uint>::Iterator uoid = seenUOIDs.find( (*it).uoid() );
    if ( uoid != seenUOIDs.end() ) {
      const KMIdentity & orig = identityForUoid( *uoid );
      if ( *it != orig ) {
	// changed identity
	kdDebug( 5006 ) << "emitting changed() for identity " << *uoid << endl;
	emit changed( *it );
	emit changed( *uoid );
      }
      seenUOIDs.remove( uoid );
    } else {
      // new identity
      kdDebug( 5006 ) << "emitting added() for identity " << (*it).uoid() << endl;
      emit added( *it );
    }
  }

  // what's left are deleted identities:
  for ( QValueList<uint>::ConstIterator it = seenUOIDs.begin() ;
	it != seenUOIDs.end() ; ++it ) {
    kdDebug( 5006 ) << "emitting deleted() for identity " << (*it) << endl;
    emit deleted( *it );
  }

  mIdentities = mShadowIdentities;
  writeConfig();
  emit ConfigManager::changed();
}

void IdentityManager::rollback()
{
  mShadowIdentities = mIdentities;
}

bool IdentityManager::hasPendingChanges() const
{
  return mIdentities != mShadowIdentities;
}

QStringList IdentityManager::identities() const
{
  QStringList result;
  for ( ConstIterator it = mIdentities.begin() ;
	it != mIdentities.end() ; ++it )
    result << (*it).identityName();
  return result;
}

QStringList IdentityManager::shadowIdentities() const
{
  QStringList result;
  for ( ConstIterator it = mShadowIdentities.begin() ;
	it != mShadowIdentities.end() ; ++it )
    result << (*it).identityName();
  return result;
}

// hmm, anyone can explain why I need to explicitly instantate qHeapSort?
//template void qHeapSort( QValueList<KMIdentity> & );

void IdentityManager::sort() {
  qHeapSort( mShadowIdentities );
}

void IdentityManager::writeConfig() const {
  QStringList identities = groupList();
  KConfig * config = KMKernel::config();
  for ( QStringList::Iterator group = identities.begin() ;
	group != identities.end() ; ++group )
    config->deleteGroup( *group );
  int i = 0;
  for ( ConstIterator it = mIdentities.begin() ;
	it != mIdentities.end() ; ++it, ++i ) {
    KConfigGroup cg( config, QString::fromLatin1("Identity #%1").arg(i) );
    (*it).writeConfig( &cg );
    if ( (*it).isDefault() ) {
      // remember which one is default:
      KConfigGroup general( config, "General" );
      general.writeEntry( configKeyDefaultIdentity, (*it).uoid() );
    }
  }
#ifndef KMAIL_TESTING
  kmkernel->slotRequestConfigSync();
#else
  config->sync();
#endif
}

void IdentityManager::readConfig() {
  mIdentities.clear();

  QStringList identities = groupList();
  if ( identities.isEmpty() ) return; // nothing to be done...

  KConfigGroup general( KMKernel::config(), "General" );
  uint defaultIdentity = general.readUnsignedNumEntry( configKeyDefaultIdentity );
  bool haveDefault = false;

  for ( QStringList::Iterator group = identities.begin() ;
	group != identities.end() ; ++group ) {
    KConfigGroup config( KMKernel::config(), *group );
    mIdentities << KMIdentity();
    mIdentities.last().readConfig( &config );
    if ( !haveDefault && mIdentities.last().uoid() == defaultIdentity ) {
      haveDefault = true;
      mIdentities.last().setIsDefault( true );
    }
  }
  if ( !haveDefault ) {
    kdWarning( 5006 ) << "IdentityManager: There was no default identity. Marking first one as default." << endl;
    mIdentities.first().setIsDefault( true );
  }
  qHeapSort( mIdentities );
}

QStringList IdentityManager::groupList() const {
  return KMKernel::config()->groupList().grep( QRegExp("^Identity #\\d+$") );
}

IdentityManager::ConstIterator IdentityManager::begin() const {
  return mIdentities.begin();
}

IdentityManager::ConstIterator IdentityManager::end() const {
  return mIdentities.end();
}

IdentityManager::Iterator IdentityManager::begin() {
  return mShadowIdentities.begin();
}

IdentityManager::Iterator IdentityManager::end() {
  return mShadowIdentities.end();
}

const KMIdentity & IdentityManager::identityForName( const QString & name ) const
{
  kdWarning( 5006 )
    << "deprecated method IdentityManager::identityForName() called!" << endl;
  for ( ConstIterator it = begin() ; it != end() ; ++it )
    if ( (*it).identityName() == name ) return (*it);
  return KMIdentity::null;
}

const KMIdentity & IdentityManager::identityForUoid( uint uoid ) const {
  for ( ConstIterator it = begin() ; it != end() ; ++it )
    if ( (*it).uoid() == uoid ) return (*it);
  return KMIdentity::null;
}

const KMIdentity & IdentityManager::identityForNameOrDefault( const QString & name ) const
{
  const KMIdentity & ident = identityForName( name );
  if ( ident.isNull() )
    return defaultIdentity();
  else
    return ident;
}

const KMIdentity & IdentityManager::identityForUoidOrDefault( uint uoid ) const
{
  const KMIdentity & ident = identityForUoid( uoid );
  if ( ident.isNull() )
    return defaultIdentity();
  else
    return ident;
}

const KMIdentity & IdentityManager::identityForAddress( const QString & addresses ) const
{
  QStringList addressList = KMMessage::splitEmailAddrList( addresses );
  for ( ConstIterator it = begin() ; it != end() ; ++it ) {
    for( QStringList::ConstIterator addrIt = addressList.begin();
         addrIt != addressList.end(); ++addrIt ) {
      // I use QString::utf8() instead of QString::latin1() because I want
      // a QCString and not a char*. It doesn't matter because emailAddr()
      // returns a 7-bit string.
      if( (*it).emailAddr().utf8().lower() ==
          KMMessage::getEmailAddr( *addrIt ).lower() )
        return (*it);
    }
  }
  return KMIdentity::null;
}

bool IdentityManager::thatIsMe( const QString & addressList ) const {
  return !identityForAddress( addressList ).isNull();
}

KMIdentity & IdentityManager::identityForName( const QString & name )
{
  for ( Iterator it = begin() ; it != end() ; ++it )
    if ( (*it).identityName() == name ) return (*it);
  kdWarning( 5006 ) << "IdentityManager::identityForName() used as newFromScratch() replacement!"
		    << "\n  name == \"" << name << "\"" << endl;
  return newFromScratch( name );
}

KMIdentity & IdentityManager::identityForUoid( uint uoid )
{
  for ( Iterator it = begin() ; it != end() ; ++it )
    if ( (*it).uoid() == uoid ) return (*it);
  kdWarning( 5006 ) << "IdentityManager::identityForUoid() used as newFromScratch() replacement!"
		    << "\n  uoid == \"" << uoid << "\"" << endl;
  return newFromScratch( i18n("Unnamed") );
}

const KMIdentity & IdentityManager::defaultIdentity() const {
  for ( ConstIterator it = begin() ; it != end() ; ++it )
    if ( (*it).isDefault() ) return (*it);
  (mIdentities.isEmpty() ? kdFatal( 5006 ) : kdWarning( 5006 ) )
    << "IdentityManager: No default identity found!" << endl;
  return *begin();
}

bool IdentityManager::setAsDefault( const QString & name ) {
  // First, check if the identity actually exists:
  QStringList names = shadowIdentities();
  if ( names.find( name ) == names.end() ) return false;
  // Then, change the default as requested:
  for ( Iterator it = begin() ; it != end() ; ++it )
    (*it).setIsDefault( (*it).identityName() == name );
  // and re-sort:
  sort();
  return true;
}

bool IdentityManager::setAsDefault( uint uoid ) {
  // First, check if the identity actually exists:
  bool found = false;
  for ( ConstIterator it = mShadowIdentities.begin() ;
	it != mShadowIdentities.end() ; ++it )
    if ( (*it).uoid() == uoid ) {
      found = true;
      break;
    }
  if ( !found ) return false;

  // Then, change the default as requested:
  for ( Iterator it = begin() ; it != end() ; ++it )
    (*it).setIsDefault( (*it).uoid() == uoid );
  // and re-sort:
  sort();
  return true;
}

bool IdentityManager::removeIdentity( const QString & name ) {
  for ( Iterator it = begin() ; it != end() ; ++it )
    if ( (*it).identityName() == name ) {
      bool removedWasDefault = (*it).isDefault();
      mShadowIdentities.remove( it );
      if ( removedWasDefault )
	mShadowIdentities.first().setIsDefault( true );
      return true;
    }
  return false;
}

KMIdentity & IdentityManager::newFromScratch( const QString & name ) {
  return newFromExisting( KMIdentity( name ) );
}

KMIdentity & IdentityManager::newFromControlCenter( const QString & name ) {
  KEMailSettings es;
  es.setProfile( es.defaultProfileName() );

  return newFromExisting( KMIdentity( name,
			       es.getSetting( KEMailSettings::RealName ),
			       es.getSetting( KEMailSettings::EmailAddress ),
			       es.getSetting( KEMailSettings::Organization ),
			       es.getSetting( KEMailSettings::ReplyToAddress )
			       ) );
}

KMIdentity & IdentityManager::newFromExisting( const KMIdentity & other,
					       const QString & name ) {
  mShadowIdentities << other;
  KMIdentity & result = mShadowIdentities.last();
  result.setIsDefault( false ); // we don't want two default identities!
  result.setUoid( newUoid() ); // we don't want two identies w/ same UOID
  if ( !name.isNull() )
    result.setIdentityName( name );
  return result;
}

void IdentityManager::createDefaultIdentity() {
  KUser user;
  QString fullName = user.fullName();

  QString emailAddress = user.loginName();
  if ( !emailAddress.isEmpty() ) {
    KConfigGroup general( KMKernel::config(), "General" );
    QString defaultdomain = general.readEntry( "Default domain" );
    if( !defaultdomain.isEmpty() ) {
      emailAddress += '@' + defaultdomain;
    }
    else {
      /* ### This code is commented out because the guessed email address
         ### is most likely anyway wrong (because it contains the full
         ### hostname while most email addresses only contain the domain name).
         ### Marc wanted to keep this stuff in the code for a future Config
         ### Wizard.
      // get hostname to form an educated guess about the possible email
      // address:
      char str[256];
      if ( !gethostname( str, 255 ) )
        // str need not be NUL-terminated if it has full length:
        str[255] = 0;
      else
        str[0] = 0;
      emailAddress += '@' + QString::fromLatin1( *str ? str : "localhost" );
      */
      // since we can't guess the domain part (see above) we clear the address
      emailAddress = QString::null;
    }
  }
  mShadowIdentities << KMIdentity( i18n("Default"), fullName, emailAddress );
  mShadowIdentities.last().setIsDefault( true );
  mShadowIdentities.last().setUoid( newUoid() );
}

int IdentityManager::newUoid()
{
  int uoid;

  // determine the UOIDs of all saved identities
  QValueList<uint> usedUOIDs;
  for ( QValueList<KMIdentity>::ConstIterator it = mIdentities.begin() ;
	it != mIdentities.end() ; ++it )
    usedUOIDs << (*it).uoid();

  if ( hasPendingChanges() ) {
    // add UOIDs of all shadow identities. Yes, we will add a lot of duplicate
    // UOIDs, but avoiding duplicate UOIDs isn't worth the effort.
    for ( QValueList<KMIdentity>::ConstIterator it = mShadowIdentities.begin() ;
          it != mShadowIdentities.end() ; ++it ) {
      usedUOIDs << (*it).uoid();
    }
  }

  usedUOIDs << 0; // no UOID must be 0 because this value always refers to the
                  // default identity

  do {
    uoid = kapp->random();
  } while ( usedUOIDs.find( uoid ) != usedUOIDs.end() );

  return uoid;
}

#include "identitymanager.moc"
