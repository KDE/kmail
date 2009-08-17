/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef __AKONADI_MESSAGELISTVIEW_PANE_H__
#define __AKONADI_MESSAGELISTVIEW_PANE_H__

#include <QtCore/QHash>
#include <QtGui/QTabWidget>

class QAbstractItemModel;
class QAbstractProxyModel;
class QItemSelectionModel;
class QItemSelection;
class QToolButton;

#include "kmail_export.h"

namespace Akonadi
{

namespace MessageListView
{

class Widget;

/**
 * This is the main MessageListView panel for Akonadi applications.
 * It contains multiple MessageListView::Widget tabs
 * so it can actually display multiple folder sets at once.
 */
class KMAIL_EXPORT Pane : public QTabWidget
{
  Q_OBJECT

public:
  /**
   * Create a Pane wrapping the specified model and selection.
   */
  explicit Pane( QAbstractItemModel *model, QItemSelectionModel *selectionModel, QWidget *parent = 0 );
  ~Pane();

private slots:
  void onSelectionChanged( const QItemSelection &selected, const QItemSelection &deselected );
  void onNewTabClicked();
  void onCloseTabClicked();
  void onCurrentTabChanged();
  void onTabContextMenuRequest( const QPoint &pos );

private:
  void createNewTab();
  QItemSelection mapSelectionToSource( const QItemSelection &selection ) const;
  QItemSelection mapSelectionFromSource( const QItemSelection &selection ) const;
  void updateTabControls();

  QAbstractItemModel *mModel;
  QItemSelectionModel *mSelectionModel;

  QHash<Widget*, QItemSelectionModel*> mWidgetSelectionHash;
  QList<const QAbstractProxyModel*> mProxyStack;

  QToolButton *mNewTabButton;
  QToolButton *mCloseTabButton;
};

} // namespace MessageListView

} // namespace Akonadi

#endif //!__AKONADI_MESSAGELISTVIEW_PANE_H__
