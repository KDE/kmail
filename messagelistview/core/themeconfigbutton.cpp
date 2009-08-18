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
#include "messagelistview/core/themeconfigbutton.h"

#include "messagelistview/core/theme.h"
#include "messagelistview/core/themecombobox.h"
#include "messagelistview/core/configurethemesdialog.h"
#include "messagelistview/core/manager.h"

#include <klocale.h>

using namespace KMail::MessageListView::Core;

ThemeConfigButton::ThemeConfigButton( QWidget * parent, const ThemeComboBox * themeComboBox )
: KPushButton( i18n( "Configure..." ), parent ),
  mThemeComboBox( themeComboBox )
{
  connect( this, SIGNAL( pressed() ),
           this, SLOT( slotConfigureThemes() ) );

  //Keep theme combo up-to-date with any changes made in the configure dialog.
  if ( mThemeComboBox != 0 )
    connect( this, SIGNAL( configureDialogCompleted() ),
             mThemeComboBox, SLOT( slotLoadThemes() ) );
}

void ThemeConfigButton::slotConfigureThemes()
{
  QString currentThemeID;
  if ( mThemeComboBox != 0 )
    currentThemeID = mThemeComboBox->currentTheme()->id();
  Manager::instance()->showConfigureThemesDialog( static_cast< QWidget * >( parent() ), currentThemeID );

  connect( ConfigureThemesDialog::instance(), SIGNAL( okClicked() ),
           this, SIGNAL( configureDialogCompleted() ) );
}

