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

#include "messagelistview/core/messageitem.h"

namespace KMail
{

namespace MessageListView
{

namespace Core
{

MessageItem::MessageItem()
  : Item( Message ), ModelInvariantIndex(),
    mThreadingStatus( MessageItem::ParentMissing ), mAboutToBeRemoved( false )
{
  mTagList = 0;
  mUniqueId = 0;
}

MessageItem::~MessageItem()
{
  if ( mTagList )
  {
    qDeleteAll( *mTagList );
    delete mTagList;
    mTagList = 0;
  }
}

void MessageItem::setTagList( QList< Tag * > * list )
{
  if ( mTagList )
  {
    qDeleteAll( *mTagList );
    delete mTagList;
  }
  mTagList = list;
}

MessageItem * MessageItem::topmostMessage()
{
  if ( !parent() )
    return this;
  if ( parent()->type() == Item::Message )
    return static_cast< MessageItem * >( parent() )->topmostMessage();
  return this;
}

void MessageItem::subTreeToList( QList< MessageItem * > &list )
{
  list.append( this );
  QList< Item * > * childList = childItems();
  if ( !childList )
    return;
  for ( QList< Item * >::Iterator it = childList->begin(); it != childList->end(); ++it )
  {
    Q_ASSERT( ( *it )->type() == Item::Message );
    static_cast< MessageItem * >( *it )->subTreeToList( list );
  }
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail
