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
#include "messagelistview/core/messageitem.h"

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

bool Filter::match( const MessageItem * item ) const
{
  if ( mStatusMask != 0 )
  {
    if ( ! ( mStatusMask & item->status().toQInt32() ) )
      return false;
  }

  if ( !mSearchString.isEmpty() )
  {
    bool searchMatches = false;
    if ( item->subject().indexOf( mSearchString, 0, Qt::CaseInsensitive ) >= 0 )
      searchMatches = true;
    if ( item->sender().indexOf( mSearchString, 0, Qt::CaseInsensitive ) >= 0 )
      searchMatches = true;
    if ( item->receiver().indexOf( mSearchString, 0, Qt::CaseInsensitive ) >= 0 )
      searchMatches = true;
    if ( !searchMatches )
      return false;
  }

  if ( !mTagId.isEmpty() ) {
    const bool tagMatches = item->findTag( mTagId ) != 0;
    if ( !tagMatches )
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

  if ( !mTagId.isEmpty() )
    return false;

  return true;
}

void Filter::clear()
{
  mStatusMask = 0;
  mSearchString.clear();
  mTagId.clear();
}


} // namespace Core

} // namespace MessageListView

} // namespace KMail
