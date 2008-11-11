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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_CONFIGUREAGGREGATIONSDIALOG_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_CONFIGUREAGGREGATIONSDIALOG_H__

#include <KDialog>

#include <QListWidget>

class QPushButton;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class Manager;
class AggregationEditor;
class Aggregation;
class AggregationListWidgetItem;

/**
 * The widget that lists the available Aggregations.
 *
 * At the moment of writing, derived from QListWidget only to override sizeHint().
 */
class AggregationListWidget : public QListWidget
{
  Q_OBJECT
public:
  AggregationListWidget( QWidget * parent )
    : QListWidget( parent )
    {};
public:

  // need a larger but shorter QListWidget
  QSize sizeHint() const
    { return QSize( 450, 128 ); };
};


/**
 * The dialog used for configuring MessageListView::Aggregation sets.
 *
 * This is managed by MessageListView::Manager. Take a look at it first
 * if you want to display this dialog.
 */
class ConfigureAggregationsDialog : public KDialog
{
  friend class Manager;

  Q_OBJECT
protected:
  ConfigureAggregationsDialog( QWidget *parent = 0 );
  ~ConfigureAggregationsDialog();

private:
  static ConfigureAggregationsDialog * mInstance;
  AggregationListWidget *mAggregationList;
  AggregationEditor *mEditor;
  QPushButton *mNewAggregationButton;
  QPushButton *mCloneAggregationButton;
  QPushButton *mDeleteAggregationButton;

protected:
  /**
   * Called by MessageListView::Manager to display an instance of this dialog.
   * If an instance is already existing (and maybe visible) it will be just activated.
   */
  static void display( QWidget * parent, const QString &preselectAggregationId = QString() );

  /**
   * This is called when MessageListView::Manager is being destroyed: kill
   * the dialog if it's still there.
   */
  static void cleanup();

public:
  /**
   * The one and only ConfigureAggregationsDialog. May be 0.
   *
   * See MessageListView::Manager if you want to display this dialog.
   */
  static ConfigureAggregationsDialog * instance()
    { return mInstance; };

private:

  // Private implementation

  void fillAggregationList();
  QString uniqueNameForAggregation( QString baseName, Aggregation * skipAggregation = 0 );
  AggregationListWidgetItem * findAggregationItemByName( const QString &name, Aggregation * skipAggregation = 0 );
  AggregationListWidgetItem * findAggregationItemByAggregation( Aggregation * set );
  AggregationListWidgetItem * findAggregationItemById( const QString &aggregationId );
  void commitEditor();
  void selectAggregationById( const QString &aggregationId );

private slots:

  // Private implementation

  void aggregationListCurrentItemChanged( QListWidgetItem * cur, QListWidgetItem * prev );
  void newAggregationButtonClicked();
  void cloneAggregationButtonClicked();
  void deleteAggregationButtonClicked();
  void editedAggregationNameChanged();
  void okButtonClicked();
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_CONFIGUREAGGREGATIONSDIALOG_H__
