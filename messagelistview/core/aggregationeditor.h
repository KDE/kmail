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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_AGGREGATIONEDITOR_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_AGGREGATIONEDITOR_H__

#include "messagelistview/core/optionseteditor.h"

#include <QTreeWidget>

class KComboBox;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class Aggregation;


/**
 * A widget that allows editing a single MessageListView::Aggregation.
 *
 * Used by ConfigureAggregationsDialog.
 */
class AggregationEditor : public OptionSetEditor
{
  Q_OBJECT
public:
  AggregationEditor( QWidget *parent );
  ~AggregationEditor();

private:
  Aggregation * mCurrentAggregation; // shallow, may be null!

  // Grouping, Threading and Sorting tab
  KComboBox * mGroupingCombo;
  KComboBox * mGroupSortingCombo;
  KComboBox * mGroupSortDirectionCombo;
  KComboBox * mGroupExpandPolicyCombo;
  KComboBox * mThreadingCombo;
  KComboBox * mThreadLeaderCombo;
  KComboBox * mThreadExpandPolicyCombo;
  KComboBox * mMessageSortingCombo;
  KComboBox * mMessageSortDirectionCombo;
  // Advanced tab
  KComboBox * mFillViewStrategyCombo;

public:
  /**
   * Sets the Aggregation to be edited.
   * Saves and forgets any previously Aggregation that was being edited.
   * The set parameter may be 0: in this case the editor is simply disabled.
   */
  void editAggregation( Aggregation *set );

  /**
   * Returns the Aggregation currently edited by this AggregationEditor.
   * May be 0.
   */
  Aggregation * editedAggregation() const
    { return mCurrentAggregation; };

  /**
   * Explicitly commits the changes in the editor to the edited Aggregation, if any.
   */
  void commit();

signals:
  /**
   * This is triggered when the aggregation name changes in the editor text field.
   * It's connected to the Aggregation configuration dialog which updates
   * the list of aggregations with the new name.
   */
  void aggregationNameChanged();

private:

  // Helpers for filling the various editing elements

  void fillGroupingCombo();
  void fillGroupSortingCombo();
  void fillGroupSortDirectionCombo();
  void fillGroupExpandPolicyCombo();
  void fillThreadingCombo();
  void fillThreadLeaderCombo();
  void fillThreadExpandPolicyCombo();
  void fillMessageSortingCombo();
  void fillMessageSortDirectionCombo();
  void fillFillViewStrategyCombo();

private slots:

  // Internal handlers for editing element interaction

  void groupingComboActivated( int idx );
  void groupSortingComboActivated( int idx );
  void threadingComboActivated( int idx );
  void messageSortingComboActivated( int idx );
  virtual void slotNameEditTextEdited( const QString &newName );

};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_AGGREGATIONEDITOR_H__
