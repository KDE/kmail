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

#include "messagelistview/core/filter.h"

namespace KMail
{

namespace MessageListView
{

namespace Core
{

Filter::Filter()
  : mStatusMask( 0 )
{
}

bool Filter::match( const QString &subject, const QString &sender, const QString &receiver, qint32 status ) const
{
  if ( mStatusMask != 0 )
  {
    if ( ! ( mStatusMask & status ) )
      return false;
  }

  if ( !mSearchString.isEmpty() )
  {
    if ( subject.indexOf( mSearchString, 0, Qt::CaseInsensitive ) >= 0 )
      return true;
    if ( sender.indexOf( mSearchString, 0, Qt::CaseInsensitive ) >= 0 )
      return true;
    if ( receiver.indexOf( mSearchString, 0, Qt::CaseInsensitive ) >= 0 )
      return true;

    return false;
  }

  return true;
}

bool Filter::isEmpty() const
{
  if ( mStatusMask != 0 )
    return false;

  if ( !mSearchString.isEmpty() )
    return false;

  return true;
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail
