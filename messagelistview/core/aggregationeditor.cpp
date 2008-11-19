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

#include "messagelistview/core/aggregationeditor.h"
#include "messagelistview/core/aggregation.h"
#include "messagelistview/core/comboboxutils.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QGridLayout>
#include <QTreeWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>

#include <KComboBox>
#include <KLocale>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

AggregationEditor::AggregationEditor( QWidget *parent )
  : OptionSetEditor( parent )
{
  mCurrentAggregation = 0;

  // Grouping, Threading and Sorting tab
  QWidget * tab = new QWidget( this );
  addTab( tab, i18n( "Groups, Threads and Sorting" ) );

  QGridLayout * tabg = new QGridLayout( tab );

  tabg->addWidget( new QLabel( i18n( "Grouping:" ), tab ), 0, 0 );
  mGroupingCombo = new KComboBox( tab );
  tabg->addWidget( mGroupingCombo, 0, 1 );

  connect( mGroupingCombo, SIGNAL( activated( int ) ),
           SLOT( groupingComboActivated( int ) ) );

  tabg->addWidget( new QLabel( i18n( "Group Sorting:" ), tab ), 1, 0 );
  mGroupSortingCombo = new KComboBox( tab );
  tabg->addWidget( mGroupSortingCombo, 1, 1 );

  connect( mGroupSortingCombo, SIGNAL( activated( int ) ),
           SLOT( groupSortingComboActivated( int ) ) );

  tabg->addWidget( new QLabel( i18n( "Group Sort Direction:" ), tab ), 2, 0 );
  mGroupSortDirectionCombo = new KComboBox( tab );
  tabg->addWidget( mGroupSortDirectionCombo, 2, 1 );

  tabg->addWidget( new QLabel( i18n( "Group Expand Policy:" ), tab ), 3, 0 );
  mGroupExpandPolicyCombo = new KComboBox( tab );
  tabg->addWidget( mGroupExpandPolicyCombo, 3, 1 );

  tabg->addWidget( new QLabel( i18n( "Threading:" ), tab ), 4, 0 );
  mThreadingCombo = new KComboBox( tab );
  tabg->addWidget( mThreadingCombo, 4, 1 );

  connect( mThreadingCombo, SIGNAL( activated( int ) ),
           SLOT( threadingComboActivated( int ) ) );

  tabg->addWidget( new QLabel( i18n( "Thread Leader:" ), tab ), 5, 0 );
  mThreadLeaderCombo = new KComboBox( tab );
  tabg->addWidget( mThreadLeaderCombo, 5, 1 );

  tabg->addWidget( new QLabel( i18n( "Thread Expand Policy:" ), tab ), 6, 0 );
  mThreadExpandPolicyCombo = new KComboBox( tab );
  tabg->addWidget( mThreadExpandPolicyCombo, 6, 1 );

  tabg->addWidget( new QLabel( i18n( "Message Sorting:" ), tab ), 7, 0 );
  mMessageSortingCombo = new KComboBox( tab );
  tabg->addWidget( mMessageSortingCombo, 7, 1 );

  connect( mMessageSortingCombo, SIGNAL( activated( int ) ),
           SLOT( messageSortingComboActivated( int ) ) );

  tabg->addWidget( new QLabel( i18n( "Message Sort Direction:" ), tab ), 8, 0 );
  mMessageSortDirectionCombo = new KComboBox( tab );
  tabg->addWidget( mMessageSortDirectionCombo, 8, 1 );

  tabg->setColumnStretch( 1, 1 );
  tabg->setRowStretch( 9, 1 );

  // Advanced tab
  tab = new QWidget( this );
  addTab( tab, i18nc( "@title:tab Advanced settings tab for aggregation mode", "Advanced" ) );

  tabg = new QGridLayout( tab );

  tabg->addWidget( new QLabel( i18n( "Fill View Strategy:" ), tab ), 0, 0 );
  mFillViewStrategyCombo = new KComboBox( tab );
  tabg->addWidget( mFillViewStrategyCombo, 0, 1 );

  tabg->setColumnStretch( 1, 1 );
  tabg->setRowStretch( 1, 1 );
}

AggregationEditor::~AggregationEditor()
{
}

void AggregationEditor::editAggregation( Aggregation *set )
{
  mCurrentAggregation = set;

  if ( !mCurrentAggregation )
  {
    setEnabled( false );
    return;
  }

  setEnabled( true );

  nameEdit()->setText( set->name() );
  descriptionEdit()->setText( set->description() );

  fillGroupingCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mGroupingCombo, (int)mCurrentAggregation->grouping() );
  fillGroupSortingCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mGroupSortingCombo, (int)mCurrentAggregation->groupSorting() );
  fillGroupSortDirectionCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mGroupSortDirectionCombo, (int)mCurrentAggregation->groupSortDirection() );
  fillGroupExpandPolicyCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mGroupExpandPolicyCombo, (int)mCurrentAggregation->groupExpandPolicy() );
  fillThreadingCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mThreadingCombo, (int)mCurrentAggregation->threading() );
  fillThreadLeaderCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mThreadLeaderCombo, (int)mCurrentAggregation->threadLeader() );
  fillThreadExpandPolicyCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mThreadExpandPolicyCombo, (int)mCurrentAggregation->threadExpandPolicy() );
  fillMessageSortingCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mMessageSortingCombo, (int)mCurrentAggregation->messageSorting() );
  fillMessageSortDirectionCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mMessageSortDirectionCombo, (int)mCurrentAggregation->messageSortDirection() );
  fillFillViewStrategyCombo();
  ComboBoxUtils::setIntegerOptionComboValue( mFillViewStrategyCombo, (int)mCurrentAggregation->fillViewStrategy() );
}

void AggregationEditor::commit()
{
  mCurrentAggregation->setName( nameEdit()->text() );
  mCurrentAggregation->setDescription( descriptionEdit()->toPlainText() );


  mCurrentAggregation->setGrouping(
      (Aggregation::Grouping)ComboBoxUtils::getIntegerOptionComboValue( mGroupingCombo, 0 )
    );

  mCurrentAggregation->setGroupSorting(
      (Aggregation::GroupSorting)ComboBoxUtils::getIntegerOptionComboValue( mGroupSortingCombo, 0 )
    );

  mCurrentAggregation->setGroupSortDirection(
      (Aggregation::SortDirection)ComboBoxUtils::getIntegerOptionComboValue( mGroupSortDirectionCombo, 0 )
    );

  mCurrentAggregation->setGroupExpandPolicy(
      (Aggregation::GroupExpandPolicy)ComboBoxUtils::getIntegerOptionComboValue( mGroupExpandPolicyCombo, 0 )
    );

  mCurrentAggregation->setThreading(
      (Aggregation::Threading)ComboBoxUtils::getIntegerOptionComboValue( mThreadingCombo, 0 )
    );

  mCurrentAggregation->setThreadLeader(
      (Aggregation::ThreadLeader)ComboBoxUtils::getIntegerOptionComboValue( mThreadLeaderCombo, 0 )
    );

  mCurrentAggregation->setThreadExpandPolicy(
      (Aggregation::ThreadExpandPolicy)ComboBoxUtils::getIntegerOptionComboValue( mThreadExpandPolicyCombo, 0 )
    );

  mCurrentAggregation->setMessageSorting(
      (Aggregation::MessageSorting)ComboBoxUtils::getIntegerOptionComboValue( mMessageSortingCombo, 0 )
    );

  mCurrentAggregation->setMessageSortDirection(
      (Aggregation::SortDirection)ComboBoxUtils::getIntegerOptionComboValue( mMessageSortDirectionCombo, 0 )
    );

  mCurrentAggregation->setFillViewStrategy(
      (Aggregation::FillViewStrategy)ComboBoxUtils::getIntegerOptionComboValue( mFillViewStrategyCombo, 0 )
    );
}

void AggregationEditor::slotNameEditTextEdited( const QString &newName )
{
  if( !mCurrentAggregation )
    return;
  mCurrentAggregation->setName( newName );
  emit aggregationNameChanged();
}

void AggregationEditor::fillGroupingCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mGroupingCombo,
      Aggregation::enumerateGroupingOptions()
    );
}

void AggregationEditor::groupingComboActivated( int )
{
  fillGroupSortingCombo();
  fillGroupSortDirectionCombo();
  fillGroupExpandPolicyCombo();
  fillThreadLeaderCombo();
}

void AggregationEditor::fillGroupSortingCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mGroupSortingCombo,
      Aggregation::enumerateGroupSortingOptions(
          (Aggregation::Grouping) ComboBoxUtils::getIntegerOptionComboValue( mGroupingCombo, Aggregation::NoGrouping )
        )
    );
}

void AggregationEditor::groupSortingComboActivated( int )
{
  fillGroupSortDirectionCombo();
  fillGroupExpandPolicyCombo();
}

void AggregationEditor::fillGroupSortDirectionCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mGroupSortDirectionCombo,
      Aggregation::enumerateGroupSortDirectionOptions(
          (Aggregation::Grouping) ComboBoxUtils::getIntegerOptionComboValue( mGroupingCombo, Aggregation::NoGrouping ),
          (Aggregation::GroupSorting) ComboBoxUtils::getIntegerOptionComboValue( mGroupSortingCombo, Aggregation::NoGroupSorting )
        )
    );
}

void AggregationEditor::fillGroupExpandPolicyCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mGroupExpandPolicyCombo,
      Aggregation::enumerateGroupExpandPolicyOptions(
          (Aggregation::Grouping) ComboBoxUtils::getIntegerOptionComboValue( mGroupingCombo, Aggregation::NoGrouping )
        )
    );
}

void AggregationEditor::fillThreadingCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mThreadingCombo,
      Aggregation::enumerateThreadingOptions()
    );
}

void AggregationEditor::threadingComboActivated( int )
{
  fillThreadLeaderCombo();
  fillThreadExpandPolicyCombo();
  fillMessageSortingCombo();
  fillMessageSortDirectionCombo();
}

void AggregationEditor::fillThreadLeaderCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mThreadLeaderCombo,
      Aggregation::enumerateThreadLeaderOptions(
          (Aggregation::Grouping) ComboBoxUtils::getIntegerOptionComboValue( mGroupingCombo, Aggregation::NoGrouping ),
          (Aggregation::Threading) ComboBoxUtils::getIntegerOptionComboValue( mThreadingCombo, Aggregation::NoThreading )
        )
    );
}

void AggregationEditor::fillThreadExpandPolicyCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mThreadExpandPolicyCombo,
      Aggregation::enumerateThreadExpandPolicyOptions(
          (Aggregation::Threading) ComboBoxUtils::getIntegerOptionComboValue( mThreadingCombo, Aggregation::NoThreading )
        )
    );
}

void AggregationEditor::fillMessageSortingCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mMessageSortingCombo,
      Aggregation::enumerateMessageSortingOptions(
          (Aggregation::Threading) ComboBoxUtils::getIntegerOptionComboValue( mThreadingCombo, Aggregation::NoThreading )
        )
    );
}

void AggregationEditor::messageSortingComboActivated( int )
{
  fillMessageSortDirectionCombo();
}

void AggregationEditor::fillMessageSortDirectionCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mMessageSortDirectionCombo,
      Aggregation::enumerateMessageSortDirectionOptions(
          (Aggregation::MessageSorting) ComboBoxUtils::getIntegerOptionComboValue( mMessageSortingCombo, Aggregation::NoMessageSorting )
        )
    );
}

void AggregationEditor::fillFillViewStrategyCombo()
{
  ComboBoxUtils::fillIntegerOptionCombo(
      mFillViewStrategyCombo,
      Aggregation::enumerateFillViewStrategyOptions()
    );
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

