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

#include "messagelistview/core/comboboxutils.h"

#include <QVariant>

#include <KComboBox>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

namespace ComboBoxUtils
{

void fillIntegerOptionCombo( KComboBox *combo, const QList< QPair< QString, int > > &optionDescriptors )
{
  int val = getIntegerOptionComboValue( combo, -1 );
  combo->clear();
  int valIdx = -1;
  int idx = 0;
  for( QList< QPair< QString, int > >::ConstIterator it = optionDescriptors.begin(); it != optionDescriptors.end(); ++it )
  {
    if ( val == (*it).second )
      valIdx = idx;
    combo->addItem( (*it).first, QVariant( (*it).second ) );
    ++idx;
  }
  if ( idx == 0)
  {
    combo->addItem( QString("-"), QVariant( (int)0 ) ); // always default to 0
    combo->setEnabled( false );
  } else {
    if ( !combo->isEnabled() )
      combo->setEnabled( true );
    if ( valIdx >= 0 )
      combo->setCurrentIndex( valIdx );
    if ( combo->count() == 1 )
      combo->setEnabled( false ); // disable when there is no choice
  }
}

void setIntegerOptionComboValue( KComboBox *combo, int value )
{
  if ( !combo->isEnabled() )
    return;

  int c = combo->count();
  int i = 0;
  while( i < c )
  {
    QVariant data = combo->itemData( i );
    bool ok;
    int val = data.toInt( &ok );
    if ( ok )
    {
      if ( val == value )
      {
        combo->setCurrentIndex( i );
        return;
      }
    }
    ++i;
  }

  combo->setCurrentIndex( 0 ); // default
}

int getIntegerOptionComboValue( KComboBox *combo, int defaultValue )
{
  int idx = combo->currentIndex();
  if ( idx < 0 )
    return defaultValue;
  
  QVariant data = combo->itemData( idx );
  bool ok;
  int val = data.toInt( &ok );
  if ( !ok )
    return defaultValue;
  return val;
}

} // namespace ComboBoxUtils

} // namespace Core

} // namespace MessageListView

} // namespace KMail

