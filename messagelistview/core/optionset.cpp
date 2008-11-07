/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "messagelistview/core/optionset.h"

#include <time.h> // for ::time( time_t * )

#include <QDataStream>

static const int gOptionSetInitialMarker = 0xcafe; // don't change
static const int gOptionSetFinalMarker = 0xbabe; // don't change

static const int gOptionSetCurrentVersion = 0x1001; // increase if you add new fields of change the meaning of some

namespace KMail
{

namespace MessageListView
{

namespace Core
{

OptionSet::OptionSet()
{
  generateUniqueId();
}

OptionSet::OptionSet( const OptionSet &set )
  : mId( set.mId ), mName( set.mName ), mDescription( set.mDescription )
{
}

OptionSet::OptionSet( const QString &name, const QString &description )
  : mName( name ), mDescription( description )
{
  generateUniqueId();
}

OptionSet::~OptionSet()
{
}

void OptionSet::generateUniqueId()
{
  static int nextUniqueId = 0;
  nextUniqueId++;
  mId = QString("%1-%2").arg( (unsigned int)::time(0) ).arg( nextUniqueId );
}

QString OptionSet::saveToString() const
{
  QByteArray raw;

  {
    QDataStream s( &raw, QIODevice::WriteOnly );

    s << gOptionSetInitialMarker;
    s << gOptionSetCurrentVersion;
    s << mId;
    s << mName;
    s << mDescription;  

    save( s );

    s << gOptionSetFinalMarker;
  }

  return QString::fromAscii( raw.toHex() );
}

bool OptionSet::loadFromString( QString &data )
{
  QByteArray raw = QByteArray::fromHex( data.toAscii() );

  QDataStream s( &raw, QIODevice::ReadOnly );

  int marker;

  s >> marker;

  if ( marker != gOptionSetInitialMarker )
    return false; // invalid configuration

  int version;

  s >> version;
  if ( version != gOptionSetCurrentVersion)
    return false; // invalid configuration

  s >> mId;

  if ( mId.isEmpty() )
    return false; // invalid configuration

  s >> mName;

  if ( mName.isEmpty() )
    return false; // invalid configuration

  s >> mDescription;

  if ( !load( s ) )
    return false; // invalid configuration

  s >> marker;

  if ( marker != gOptionSetFinalMarker )
    return false; // invalid configuration
    
  return true;
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

