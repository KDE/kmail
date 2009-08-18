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
#include "messagelistview/core/aggregationconfigbutton.h"

#include "messagelistview/core/aggregationcombobox.h"
#include "messagelistview/core/configureaggregationsdialog.h"
#include "messagelistview/core/manager.h"

#include <klocale.h>

using namespace KMail::MessageListView::Core;

AggregationConfigButton::AggregationConfigButton( QWidget * parent, const AggregationComboBox * aggregationComboBox )
: KPushButton( i18n( "Configure..." ), parent ),
  mAggregationComboBox( aggregationComboBox )
{
  connect( this, SIGNAL( pressed() ),
           this, SLOT( slotConfigureAggregations() ) );

  // Keep aggregation combo up-to-date with any changes made in the configure dialog.
  if ( mAggregationComboBox != 0 )
    connect( this, SIGNAL( configureDialogCompleted() ),
             mAggregationComboBox, SLOT( slotLoadAggregations() ) );
}

void AggregationConfigButton::slotConfigureAggregations()
{
  QString currentAggregationID;
  if ( mAggregationComboBox != 0 )
    currentAggregationID = mAggregationComboBox->currentAggregation()->id();
  Manager::instance()->showConfigureAggregationsDialog( static_cast< QWidget * >( parent() ), currentAggregationID );

  connect( ConfigureAggregationsDialog::instance(), SIGNAL( okClicked() ),
           this, SIGNAL( configureDialogCompleted() ) );
}

