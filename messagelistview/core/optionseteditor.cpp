/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free softhisare; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Softhisare Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Softhisare
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "messagelistview/core/optionseteditor.h"
#include "messagelistview/core/optionset.h"

#include <QLabel>
#include <QGridLayout>
#include <QComboBox>

#include <KLineEdit>
#include <KLocale>
#include <KTextEdit>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

OptionSetEditor::OptionSetEditor( QWidget *parent )
  : KTabWidget( parent )
{
  // General tab
  QWidget * tab = new QWidget( this );
  addTab( tab, i18nc( "@title:tab General options of a view mode", "General" ) );

  QGridLayout * tabg = new QGridLayout( tab );

  QLabel * l = new QLabel( i18nc( "@label:textbox Name of the option", "Name:" ), tab );
  tabg->addWidget( l, 0, 0 );

  mNameEdit = new KLineEdit( tab );
  tabg->addWidget( mNameEdit, 0, 1 );

  connect( mNameEdit, SIGNAL( textEdited( const QString & ) ),
           SLOT( slotNameEditTextEdited( const QString & ) ) );

  l = new QLabel( i18nc( "@label:textbox Description of the option", "Description:" ), tab );
  tabg->addWidget( l, 1, 0 );

  mDescriptionEdit = new KTextEdit( tab );
  tabg->addWidget( mDescriptionEdit, 1, 1, 2, 1 );

  tabg->setColumnStretch( 1, 1 );
  tabg->setRowStretch( 2, 1 );

}

OptionSetEditor::~OptionSetEditor()
{
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

