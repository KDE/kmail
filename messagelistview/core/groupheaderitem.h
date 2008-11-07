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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_GROUPHEADERITEM_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_GROUPHEADERITEM_H__

#include <QString>

#include "messagelistview/core/item.h"

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class GroupHeaderItem : public Item
{
  friend class Model;
  friend class SkinPreviewDelegate;

protected:
  GroupHeaderItem( const QString &label );
  virtual ~GroupHeaderItem();

private:
  QString mLabel;

public:
  const QString & label() const
    { return mLabel; };

  void setLabel( const QString &label )
    { mLabel = label; };

  
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_GROUPHEADERITEM_H__
