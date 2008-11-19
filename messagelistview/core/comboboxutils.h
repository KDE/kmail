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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_COMBOBOXUTILS_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_COMBOBOXUTILS_H__

#include <QList>
#include <QPair>

class KComboBox;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

/**
 * Namespace containing some helper functions for KComboBox widgets.
 */
namespace ComboBoxUtils
{
  /**
   * Fills the specified KComboBox with the options available in optionDescriptors.
   * Each option descriptor contains a description string and a distinct integer (possibly enum)
   * identifier value.
   */
  void fillIntegerOptionCombo( KComboBox *combo, const QList< QPair< QString, int > > &optionDescriptors );

  /**
   * Returns the identifier of the currently selected option in the specified combo.
   * If the combo has no current selection or something goes wrong then the defaultValue
   * is returned instead.
   */
  int getIntegerOptionComboValue( KComboBox *combo, int defaultValue );

  /**
   * Sets the currently selected option in the specified combo.
   */
  void setIntegerOptionComboValue( KComboBox *combo, int value );

} // namespace ComboBoxUtils

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_COMBOBOXUTILS_H__
