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


#include "messagetree.h"
#include "kmmsgbase.h"

namespace KMail
{

MessageTreeCollection::MessageTreeCollection()
  : mTreeList( 0 )
{
}

MessageTreeCollection::~MessageTreeCollection()
{
  if ( mTreeList )
  {
    qDeleteAll( *mTreeList );
    delete mTreeList;
  }
}

void MessageTreeCollection::addTree( MessageTree * tree )
{
  if ( !mTreeList )
    mTreeList = new QList< MessageTree * >();
  mTreeList->append( tree );
}

QList< KMMsgBase * > MessageTreeCollection::toFlatList()
{
  QList< KMMsgBase * > ret;

  if ( !mTreeList )
    return ret;

  for ( QList< MessageTree * >::Iterator it = mTreeList->begin(); it != mTreeList->end(); ++it )
    ( *it )->appendToList( ret );

  return ret;
}



MessageTree::MessageTree( KMMsgBase * msg )
  : mMsgBase( msg ), mMessageSerial( msg->getMsgSerNum() ), mChildList( 0 )
{
}

MessageTree::~MessageTree()
{
  if ( mChildList )
  {
    qDeleteAll( *mChildList );
    delete mChildList;
  }
}

void MessageTree::addChild( MessageTree * it )
{
  if ( !mChildList )
    mChildList = new QList< MessageTree * >();
  mChildList->append( it );
}

QList< KMMsgBase * > MessageTree::toFlatList()
{
  QList< KMMsgBase * > ret;
  appendToList( ret );
  return ret;
}

void MessageTree::appendToList( QList< KMMsgBase * > &list )
{
  list.append( mMsgBase );
  if ( !mChildList )
    return;

  for ( QList< MessageTree * >::Iterator it = mChildList->begin(); it != mChildList->end(); ++it )
    ( *it )->appendToList( list );
}

} // namespace KMail
