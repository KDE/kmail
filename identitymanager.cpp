/*  -*- c++ -*-
    identitymanager.h

    KMail, the KDE mail client.
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

// config keys:
static const char * configKeyDefaultIdentity = "Default Identity";

#include "identitymanager.h"

#include "kmidentity.h" // for IdentityList::{export,import}Data
#ifndef KMAIL_TESTING
#include "kmkernel.h"
#endif

#include <kemailsettings.h> // for IdentityEntry::fromControlCenter()
#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qregexp.h>
#include <qtl.h>

#include <pwd.h> // for struct pw;
#include <unistd.h> // for getuid
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
    << "IdentityManager: There were uncommited changes!" << endl;
}

void IdentityManager::commit()
{
  if ( !hasPendingChanges() ) return;
  mIdentities = mShadowIdentities;
  writeConfig();
  emit changed();
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

// hmm, anyone can explain why I need to explicitely instantate qHeapSort?
//template void qHeapSort( QValueList<KMIdentity> & );

void IdentityManager::sort() {
  qHeapSort( mShadowIdentities );
}

void IdentityManager::writeConfig() const {
  QStringList identities = groupList();
  KConfig * config = kapp->config();
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
      general.writeEntry( configKeyDefaultIdentity, (*it).identityName() );
    }
  }
#ifndef KMAIL_TESTING
  kernel->slotRequestConfigSync();
#else
  config->sync();
#endif
}

void IdentityManager::readConfig() {
  mIdentities.clear();

  QStringList identities = groupList();
  if ( identities.isEmpty() ) return; // nothing to be done...

  KConfigGroup general( kapp->config(), "General" );
  QString defaultIdentity = general.readEntry( configKeyDefaultIdentity );
  bool haveDefault = false;

  for ( QStringList::Iterator group = identities.begin() ;
	group != identities.end() ; ++group ) {
    KConfigGroup config( kapp->config(), *group );
    mIdentities << KMIdentity();
    mIdentities.last().readConfig( &config );
    if ( !haveDefault && mIdentities.last().identityName() == defaultIdentity ) {
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
  return kapp->config()->groupList().grep( QRegExp("^Identity #\\d+$") );
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
  for ( ConstIterator it = begin() ; it != end() ; ++it )
    if ( (*it).identityName() == name ) return (*it);
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

const KMIdentity & IdentityManager::identityForAddress( const QString & addressList ) const
{
  for ( ConstIterator it = begin() ; it != end() ; ++it )
    if ( addressList.find( (*it).emailAddr(), 0, false ) != -1 )
      return (*it);
  return KMIdentity::null;
}

KMIdentity & IdentityManager::identityForName( const QString & name )
{
  for ( Iterator it = begin() ; it != end() ; ++it )
    if ( (*it).identityName() == name ) return (*it);
  kdWarning( 5006 ) << "IdentityManager::identityForName() used as newFromScratch() replacement!"
		    << "\n  name == \"" << name << "\"" << endl;
  return newFromScratch( name );
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
  if ( !name.isNull() )
    result.setIdentityName( name );
  return result;
}

void IdentityManager::createDefaultIdentity() {
  struct passwd * pw = getpwuid( getuid() );

  QString fullName, emailAddr;
  if ( pw ) {
    // extract possible full name from /etc/passwd
    fullName = QString::fromLocal8Bit( pw->pw_gecos );
    int i = fullName.find(',');
    if ( i > 0 ) fullName.truncate( i );

    // extract login name from /etc/passwd and get hostname to form an
    // educated guess about the possible email address:
    char str[256];
    if ( !gethostname( str, 255 ) )
      // str need not be NUL-terminated if it has full length:
      str[255] = 0;
    else
      str[0] = 0;
    emailAddr = QString::fromLatin1("%1@%2")
      .arg( QString::fromLocal8Bit( pw->pw_name ) )
      .arg( QString::fromLatin1( *str ? str : "localhost" ) );
  }
  mShadowIdentities << KMIdentity( i18n("Default"), fullName, emailAddr );
  mShadowIdentities.last().setIsDefault( true );
}

#include "identitymanager.moc"
