/*
    identitycombo.cpp

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

#include "identitycombo.h"

#include "kmidentity.h"
#include "identitymanager.h"
#include "kmkernel.h"

#include <klocale.h>

#include <qstringlist.h>
#include <qstring.h>

#include "assert.h"

IdentityCombo::IdentityCombo( QWidget * parent, const char * name )
  : QComboBox( false, parent, name )
{
  reloadCombo();
  reloadUoidList();
  connect( this, SIGNAL(activated(int)), SLOT(slotEmitChanged(int)) );
  connect( kernel->identityManager(), SIGNAL(changed()),
	   SLOT(slotIdentityManagerChanged()) );
}

QString IdentityCombo::currentIdentityName() const {
  return kernel->identityManager()->identities()[ currentItem() ];
}

uint IdentityCombo::currentIdentity() const {
  return mUoidList[ currentItem() ];
}

void IdentityCombo::setCurrentIdentity( const KMIdentity & identity ) {
  setCurrentIdentity( identity.uoid() );
}

void IdentityCombo::setCurrentIdentity( const QString & name ) {
  int idx = kernel->identityManager()->identities().findIndex( name );
  if ( idx < 0 ) return;
  if ( idx == currentItem() ) return;

  blockSignals( true ); // just in case Qt gets fixed to emit activated() here
  setCurrentItem( idx );
  blockSignals( false );

  slotEmitChanged( idx );
}

void IdentityCombo::setCurrentIdentity( uint uoid ) {
  int idx = mUoidList.findIndex( uoid );
  if ( idx < 0 ) return;
  if ( idx == currentItem() ) return;

  blockSignals( true ); // just in case Qt gets fixed to emit activated() here
  setCurrentItem( idx );
  blockSignals( false );

  slotEmitChanged( idx );
}

void IdentityCombo::reloadCombo() {
  QStringList identities = kernel->identityManager()->identities();
  // the IM should prevent this from happening:
  assert( !identities.isEmpty() );
  identities.first() = i18n("%1 (Default)").arg( identities.first() );
  clear();
  insertStringList( identities );
}

void IdentityCombo::reloadUoidList() {
  const IdentityManager * im = kernel->identityManager();
  mUoidList.clear();
  for ( IdentityManager::ConstIterator it = im->begin() ; it != im->end() ; ++it )
    mUoidList << (*it).uoid();
}

void IdentityCombo::slotIdentityManagerChanged() {
  uint oldIdentity = mUoidList[ currentItem() ];

  reloadUoidList();
  int idx = mUoidList.findIndex( oldIdentity );

  blockSignals( true );
  reloadCombo();
  setCurrentItem( idx < 0 ? 0 : idx );
  blockSignals( false );

  if ( idx < 0 )
    // apparently our oldIdentity got deleted:
    slotEmitChanged( currentItem() );
}

void IdentityCombo::slotEmitChanged( int idx ) {
  emit identityChanged( kernel->identityManager()->identities()[idx] );
  emit identityChanged( mUoidList[idx] );
}

#include "identitycombo.moc"
