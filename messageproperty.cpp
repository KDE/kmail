/*  Message Property
    
    This file is part of KMail, the KDE mail client.
    Copyright (c) Don Sanders <sanders@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include "messageproperty.h"

#include <kmime/kmime_content.h>
using namespace KMail;

QMap<Akonadi::Item::Id, Akonadi::Collection> MessageProperty::sFolders;

bool MessageProperty::filtering( const Akonadi::Item &item )
{
  return sFolders.contains( item.id() );
}

void MessageProperty::setFiltering( const Akonadi::Item &item, bool filter )
{
  if ( filter )
    sFolders.insert( item.id(), Akonadi::Collection() );
  else
    sFolders.remove( item.id() );
}


void MessageProperty::setFilterFolder( const Akonadi::Item &item, const Akonadi::Collection &folder)
{
  sFolders.insert( item.id(), folder );
}

Akonadi::Collection MessageProperty::filterFolder( const Akonadi::Item &item )
{
  return sFolders.value( item.id() );
}

void MessageProperty::forget( KMime::Content *msgBase )
{
}

#include "messageproperty.moc"
