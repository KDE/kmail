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
  connect( this, SIGNAL(activated(int)), SLOT(slotEmitChanged(int)) );
  connect( kernel->identityManager(), SIGNAL(changed()),
	   SLOT(slotIdentityManagerChanged()) );
}

QString IdentityCombo::currentIdentity() const {
  return kernel->identityManager()->identities()[ currentItem() ];
}

void IdentityCombo::setCurrentIdentity( const KMIdentity & identity ) {
  setCurrentIdentity( identity.identityName() );
}

void IdentityCombo::setCurrentIdentity( const QString & name ) {
  int idx = kernel->identityManager()->identities().findIndex( name );
  if ( idx < 0 ) return;
  setCurrentItem( idx );
  slotEmitChanged( idx );
}

void IdentityCombo::reloadCombo() {
  QStringList identities = kernel->identityManager()->identities();
  assert( !identities.isEmpty() );
  // the IM should prevent this from happening:
  identities.first() = i18n("%1 (Default)").arg( identities.first() );
  int idx = identities.findIndex( currentText() );
  clear();
  insertStringList( identities );
  setCurrentItem( idx >= 0 ? idx : 0 );
}


void IdentityCombo::slotIdentityManagerChanged() {
  QString oldIdentity = currentText();
  blockSignals( true );
  reloadCombo();
  blockSignals( false );
  if ( currentText() != oldIdentity )
    slotEmitChanged( currentItem() );
}

void IdentityCombo::slotEmitChanged( int idx ) {
  emit identityChanged( kernel->identityManager()->identities()[idx] );
}

#include "identitycombo.moc"
