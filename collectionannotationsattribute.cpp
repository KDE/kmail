/*
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#include "collectionannotationsattribute.h"

#include <KDebug>
#include <QByteArray>
#include <akonadi/attribute.h>

using namespace Akonadi;

CollectionAnnotationsAttribute::CollectionAnnotationsAttribute()
{
}

CollectionAnnotationsAttribute::CollectionAnnotationsAttribute( const QMap<QByteArray, QByteArray> &annotations )
  : mAnnotations( annotations )
{
}

void CollectionAnnotationsAttribute::setAnnotations( const QMap<QByteArray, QByteArray> &annotations )
{
  mAnnotations = annotations;
}

QMap<QByteArray, QByteArray> CollectionAnnotationsAttribute::annotations() const
{
  return mAnnotations;
}

QByteArray CollectionAnnotationsAttribute::type() const
{
  return "collectionannotations";
}

Akonadi::Attribute* CollectionAnnotationsAttribute::clone() const
{
  return new CollectionAnnotationsAttribute( mAnnotations );
}

QByteArray CollectionAnnotationsAttribute::serialized() const
{
  QByteArray result = "";

  foreach ( const QByteArray &key, mAnnotations.keys() ) {
    result+= key;
    result+= ' ';
    result+= mAnnotations[key];
    result+= " % "; // We use this separator as '%' is not allowed in keys or values
  }
  result.chop( 3 );

  return result;
}

void CollectionAnnotationsAttribute::deserialize( const QByteArray &data )
{
  mAnnotations.clear();
  const QList<QByteArray> lines = data.split( '%' );

  for ( int i = 0; i < lines.size(); ++i ) {
    QByteArray line = lines[i];
    if ( i != 0 && line.startsWith( ' ' ) )
      line = line.mid( 1 );
    if ( i != lines.size() - 1 && line.endsWith( ' ' ) )
      line.chop( 1 );
    if ( line.trimmed().isEmpty() )
      continue;
    int wsIndex = line.indexOf( ' ' );
    if ( wsIndex > 0 ) {
      const QByteArray key = line.mid( 0, wsIndex );
      const QByteArray value = line.mid( wsIndex+1 );
      mAnnotations[key] = value;
    } else {
      mAnnotations.insert( line, QByteArray() );
    }
  }
}
