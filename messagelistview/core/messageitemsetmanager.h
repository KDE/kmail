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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_MESSAGEITEMSETMANAGER_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_MESSAGEITEMSETMANAGER_H__

#include <QHash>
#include <QList>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class MessageItem;

typedef unsigned long int MessageItemSetReference;

/**
 * This class manages sets of messageitem references.
 * It can be used to create a set, add some messages to it
 * and get a reference that later can be used to retrieve
 * the stored messages.
 *
 * It's used by Model to keep track of jobs requested
 * from outside that operate on sets of MessageItem instances.
 * Model takes care of removing the deleted MessageItem objects
 * from the sets in order to avoid invalid references.
 */
class MessageItemSetManager
{
public:
  MessageItemSetManager();
  ~MessageItemSetManager();

private:
  QHash< MessageItemSetReference, QHash< MessageItem *, MessageItem * > * > * mSets;

public:
  void clearAllSets();
  int setCount() const
    { return mSets->count(); };
  void removeSet( MessageItemSetReference ref );
  void removeMessageItemFromAllSets( MessageItem * mi );
  QList< MessageItem * > messageItems( MessageItemSetReference ref );
  MessageItemSetReference createSet();
  bool addMessageItem( MessageItemSetReference ref, MessageItem * mi );
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_MESSAGEITEMSETMANAGER_H__
