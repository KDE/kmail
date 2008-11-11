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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_DELEGATE_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_DELEGATE_H__

#include "messagelistview/core/themedelegate.h"

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class View;

class Delegate : public ThemeDelegate
{
public:
  Delegate( View *pParent );
  ~Delegate();

protected:
  /**
   * Returns the Item for the specified model index. Reimplemented from ThemeDelegate.
   */
  virtual Item * itemFromIndex( const QModelIndex &index ) const;
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_DELEGATE_H__
