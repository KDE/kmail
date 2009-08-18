/* Copyright 2009 James Bendig <james@imptalk.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "messagelistview/core/themecombobox.h"

#include "messagelistview/core/manager.h"
#include "messagelistview/core/theme.h"

using namespace KMail::MessageListView::Core;

ThemeComboBox::ThemeComboBox( QWidget * parent )
: KComboBox( parent )
{
  slotLoadThemes();
}

static bool themeNameLessThan( const Theme * lhs, const Theme * rhs )
{
  return lhs->name() < rhs->name();
}

void ThemeComboBox::slotLoadThemes()
{
  clear();

  // Get all message list themes and sort them into alphabetical order.
  QList< Theme * > themes = Manager::instance()->themes().values();
  qSort( themes.begin(), themes.end(), themeNameLessThan );

  foreach( const Theme * theme, themes )
  {
    addItem( theme->name(), QVariant( theme->id() ) );
  }
}

void ThemeComboBox::setCurrentTheme( const Theme * theme )
{
  Q_ASSERT( theme != 0 );

  const QString themeID = theme->id();
  const int themeIndex = findData( QVariant( themeID ) );
  setCurrentIndex( themeIndex );
}

const Theme * ThemeComboBox::currentTheme() const
{
  const QVariant currentThemeVariant = itemData( currentIndex() );
  const QString currentThemeID = currentThemeVariant.toString();
  return Manager::instance()->theme( currentThemeID );
}

