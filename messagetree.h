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

#ifndef __KMAIL_MESSAGETREE_H__
#define __KMAIL_MESSAGETREE_H__

#include <QList>

class KMMsgBase;

namespace KMail
{

class MessageTree;


class MessageTreeCollection
{
public:
  MessageTreeCollection();
  ~MessageTreeCollection();

private:
  QList< MessageTree * > * mTreeList;

public:

  bool isEmpty() const
    { return mTreeList == 0; };

  QList< MessageTree * > * treeList() const
    { return mTreeList; };

  void addTree( MessageTree * tree );

  QList< KMMsgBase * > toFlatList();

};


/**
 * A tree of KMMsgBase objects.
 */
class MessageTree
{
  friend class MessageTreeCollection;
private:
  KMMsgBase * mMsgBase;
  unsigned long mMessageSerial;
  QList< MessageTree * > * mChildList;

public:
  MessageTree( KMMsgBase * msg );
  ~MessageTree();

  void addChild( MessageTree * it );

  QList< MessageTree * > * childList() const
    { return mChildList; };

  KMMsgBase * message() const
    { return mMsgBase; };

  unsigned long serial() const
    { return mMessageSerial; };

  QList< KMMsgBase * > toFlatList();

protected:
  void appendToList( QList< KMMsgBase * > &list );
};

} // namespace KMail


#endif //!__KMAIL_MESSAGETREE_H__
