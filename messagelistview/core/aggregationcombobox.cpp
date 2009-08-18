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
#include "messagelistview/core/aggregationcombobox.h"

#include "messagelistview/core/aggregation.h"
#include "messagelistview/core/manager.h"

using namespace KMail::MessageListView::Core;

AggregationComboBox::AggregationComboBox( QWidget * parent )
: KComboBox( parent )
{
  slotLoadAggregations();
}

static bool aggregationNameLessThan( const Aggregation * lhs, const Aggregation * rhs )
{
  return lhs->name() < rhs->name();
}

void AggregationComboBox::slotLoadAggregations()
{
  clear();

  // Get all message list aggregations and sort them into alphabetical order.
  QList< Aggregation * > aggregations = Manager::instance()->aggregations().values();
  qSort( aggregations.begin(), aggregations.end(), aggregationNameLessThan );

  foreach( const Aggregation * aggregation, aggregations )
  {
    addItem( aggregation->name(), QVariant( aggregation->id() ) );
  }
}

void AggregationComboBox::setCurrentAggregation( const Aggregation * aggregation )
{
  Q_ASSERT( aggregation != 0 );

  const QString aggregationID = aggregation->id();
  const int aggregationIndex = findData( QVariant( aggregationID ) );
  setCurrentIndex( aggregationIndex );
}

const Aggregation * AggregationComboBox::currentAggregation() const
{
  const QVariant currentAggregationVariant = itemData( currentIndex() );
  const QString currentAggregationID = currentAggregationVariant.toString();
  return Manager::instance()->aggregation( currentAggregationID );
}

