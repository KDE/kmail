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

#include "messagelistview/core/delegate.h"
#include "messagelistview/core/groupheaderitem.h"
#include "messagelistview/core/messageitem.h"
#include "messagelistview/core/view.h"


namespace KMail
{

namespace MessageListView
{

namespace Core
{

Delegate::Delegate( View *pParent )
  : ThemeDelegate( pParent )
{
}

Delegate::~Delegate()
{
}

Item * Delegate::itemFromIndex( const QModelIndex &index ) const
{
  return static_cast< Item * >( index.internalPointer() );
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail
