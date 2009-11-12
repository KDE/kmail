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

ImapAclAttribute::ImapAclAttribute( const QMap<QByteArray, KIMAP::Acl::Rights> &rights )
  : mRights( rights )
{
}

void ImapAclAttribute::setRights( const QMap<QByteArray, KIMAP::Acl::Rights> &rights )
{
  mRights = rights;
}

QMap<QByteArray, KIMAP::Acl::Rights> ImapAclAttribute::rights() const
{
  return mRights;
}

QByteArray ImapAclAttribute::type() const
{
  return "imapacl";
}

Akonadi::Attribute* ImapAclAttribute::clone() const
{
  return new ImapAclAttribute( mRights );
}

QByteArray ImapAclAttribute::serialized() const
{
  QByteArray result = "";

  foreach ( const QByteArray &id, mRights.keys() ) {
    result+= id;
    result+= ' ';
    result+= KIMAP::Acl::rightsToString( mRights[id] );
    result+= " % "; // We use this separator as '%' is not allowed in keys or values
  }
  result.chop( 3 );

  return result;

}

void ImapAclAttribute::deserialize( const QByteArray &data )
{
  mRights.clear();
  QList<QByteArray> lines = data.split( '%' );

  foreach ( const QByteArray &line, lines ) {
    QByteArray trimmed = line.trimmed();
    int wsIndex = trimmed.indexOf( ' ' );
    const QByteArray id = trimmed.mid( 0, wsIndex ).trimmed();
    const QByteArray value = trimmed.mid( wsIndex+1, line.length()-wsIndex ).trimmed();
    mRights[id] = KIMAP::Acl::rightsFromString( value );
  }
}
