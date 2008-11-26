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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_OPTIONSETEDITOR_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_OPTIONSETEDITOR_H__

#include <KTabWidget>

class KLineEdit;
class KTextEdit;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class OptionSet;

/**
 * The base class for the OptionSet editors. Provides common functionality.
 */
class OptionSetEditor : public KTabWidget
{
  Q_OBJECT

public:
  OptionSetEditor( QWidget *parent );
  ~OptionSetEditor();

private:
  KLineEdit * mNameEdit;                       ///< The editor for the OptionSet name
  KTextEdit * mDescriptionEdit;                ///< The editor for the OptionSet description

protected:

  /**
   * Returns the editor for the name of the OptionSet.
   * Derived classes are responsable of filling this UI element and reading back data from it.
   */
  KLineEdit * nameEdit() const
    { return mNameEdit; };

  /**
   * Returns the editor for the description of the OptionSet.
   * Derived classes are responsable of filling this UI element and reading back data from it.
   */
  KTextEdit * descriptionEdit() const
    { return mDescriptionEdit; };

protected slots:
  /**
   * Handles editing of the name field.
   * Pure virtual slot. Derived classes must provide an implementation.
   */
  virtual void slotNameEditTextEdited( const QString &newName ) = 0;

};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_OPTIONSETEDITOR_H__
