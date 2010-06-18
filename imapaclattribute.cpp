/*
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "imapaclattribute.h"

#include <QtCore/QByteArray>

using namespace Akonadi;

ImapAclAttribute::ImapAclAttribute()
{
}

ImapAclAttribute::ImapAclAttribute( const QMap<QByteArray, KIMAP::Acl::Rights> &rights,
                                    const QMap<QByteArray, KIMAP::Acl::Rights> &oldRights )
  : mRights( rights ), mOldRights( oldRights )
{
}

void ImapAclAttribute::setRights( const QMap<QByteArray, KIMAP::Acl::Rights> &rights )
{
  mOldRights = mRights;
  mRights = rights;
}

QMap<QByteArray, KIMAP::Acl::Rights> ImapAclAttribute::rights() const
{
  return mRights;
}

QMap<QByteArray, KIMAP::Acl::Rights> ImapAclAttribute::oldRights() const
{
  return mOldRights;
}

QByteArray ImapAclAttribute::type() const
{
  return "imapacl";
}

Akonadi::Attribute* ImapAclAttribute::clone() const
{
  return new ImapAclAttribute( mRights, mOldRights );
}

QByteArray ImapAclAttribute::serialized() const
{
  QByteArray result = "";

  bool added = false;
  foreach ( const QByteArray &id, mRights.keys() ) {
    result+= id;
    result+= ' ';
    result+= KIMAP::Acl::rightsToString( mRights[id] );
    result+= " % "; // We use this separator as '%' is not allowed in keys or values
    added = true;
  }

  if ( added )
    result.chop( 3 );

  result+= " %% ";

  added = false;
  foreach ( const QByteArray &id, mOldRights.keys() ) {
    result+= id;
    result+= ' ';
    result+= KIMAP::Acl::rightsToString( mOldRights[id] );
    result+= " % "; // We use this separator as '%' is not allowed in keys or values
    added = true;
  }

  if ( added )
    result.chop( 3 );

  return result;
}

void ImapAclAttribute::deserialize( const QByteArray &data )
{
  mRights.clear();
  mOldRights.clear();
  const int pos = data.indexOf( " %% " );
  if ( pos == -1 )
    return;

  const QByteArray leftPart = data.left( pos );
  const QByteArray rightPart = data.mid( pos + 4 );
  QList<QByteArray> rights = leftPart.split( '%' );
  QList<QByteArray> oldRights = rightPart.split( '%' );

  foreach ( const QByteArray &right, rights ) {
    const QByteArray trimmed = right.trimmed();
    const int wsIndex = trimmed.indexOf( ' ' );
    const QByteArray id = trimmed.mid( 0, wsIndex ).trimmed();
    if ( !id.isEmpty() ) {
      const QByteArray value = trimmed.mid( wsIndex + 1, right.length() - wsIndex ).trimmed();
      mRights[id] = KIMAP::Acl::rightsFromString( value );
    }
  }

  foreach ( const QByteArray &right, oldRights ) {
    const QByteArray trimmed = right.trimmed();
    const int wsIndex = trimmed.indexOf( ' ' );
    const QByteArray id = trimmed.mid( 0, wsIndex ).trimmed();
    if ( !id.isEmpty() ) {
      const QByteArray value = trimmed.mid( wsIndex + 1, right.length() - wsIndex ).trimmed();
      mOldRights[id] = KIMAP::Acl::rightsFromString( value );
    }
  }
}
