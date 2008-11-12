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


#ifndef __KMAIL_MESSAGELISTVIEW_CORE_ENUMS_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_ENUMS_H__

namespace KMail
{

namespace MessageListView
{

namespace Core
{

  /**
   * Pre-selection is the action of automatically selecting a message just after the folder
   * has finished loading. We may want to select the message that was selected the last
   * time this folder has been open, or we may want to select the first unread message.
   * We also may want to do no pre-selection at all (for example, when the user
   * starts navigating the view before the pre-selection could actually be made
   * and pre-selecting would confuse him). This member holds the option.
   *
   * See Model::setStorageModel() for more information.
   */
  enum PreSelectionMode
  {
    PreSelectNone,                // no pre-selection at all
    PreSelectLastSelected,        // pre-select the last message that was selected in this folder (default)
    PreSelectFirstUnread,         // pre-select the first unread message
    PreSelectFirstUnreadCentered  // pre-select the first unread message and center it
  };

}

}

}

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_ENUMS_H__
