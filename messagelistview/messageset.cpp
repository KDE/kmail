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

#include "messagelistview/messageset.h"
#include "messagelistview/pane.h"

#include "kmfolder.h"
#include "kmmessage.h"
#include "kmmainwidget.h"

namespace KMail
{

namespace MessageListView
{

using namespace MessageList;

MessageSet::MessageSet( Pane * pane, Widget * widget, KMFolder * folder, Core::MessageItemSetReference ref )
  : QObject( 0 ), mPane( pane ), mWidget( widget ), mFolder( folder ), mMessageItemSetReference( ref )
{
  connect( mPane, SIGNAL( destroyed() ),
           SLOT( slotPaneDestroyed() ) );

  connect( mFolder, SIGNAL( destroyed() ),
           SLOT( slotFolderDestroyed() ) );
}

MessageSet::~MessageSet()
{
  if ( mPane )
    mPane->messageSetDestroyed( this );
}

void MessageSet::slotPaneDestroyed()
{
  mFolder = 0;
  mPane = 0;
}

void MessageSet::slotFolderDestroyed()
{
  mFolder = 0;
}

QList< KMMsgBase * > MessageSet::contentsAsMsgBaseList()
{
  if ( !mPane || !mFolder )
    return QList< KMMsgBase * >();

  return mPane->messageSetContentsAsMsgBaseList( this );
}

void MessageSet::markAsAboutToBeRemoved( bool bMark )
{
  if ( !mPane || !mFolder )
    return;

  mPane->messageSetMarkAsAboutToBeRemoved( this, bMark );
}

void MessageSet::select( bool clearSelectionFirst )
{
  if ( !mPane || !mFolder )
    return;

  mPane->messageSetSelect( this, clearSelectionFirst );
}


} // namespace MessageListView

} // namespace KMail

