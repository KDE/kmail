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

#include <boost/shared_ptr.hpp>
#include <kmime/kmime_message.h>

class QAbstractItemModel;
class QAbstractProxyModel;
class QItemSelectionModel;
class QItemSelection;
class QToolButton;

#include "kmail_export.h"

typedef boost::shared_ptr<KMime::Message> MessagePtr;

namespace KPIM
{
  class MessageStatus;
}

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

signals:
  /**
   * Emitted when a message is selected (that is, single clicked and thus made current in the view)
   * Note that this message CAN be 0 (when the current item is cleared, for example).
   *
   * This signal is emitted when a SINGLE message is selected in the view, probably
   * by clicking on it or by simple keyboard navigation. When multiple items
   * are selected at once (by shift+clicking, for example) then you will get
   * this signal only for the last clicked message (or at all, if the last shift+clicked
   * thing is a group header...). You should handle selection changed in this case.
   */
  void messageSelected( MessagePtr msg );

  /**
   * Emitted when a message is doubleclicked or activated by other input means
   */
  void messageActivated( MessagePtr msg );

  /**
   * Emitted when the selection in the view changes.
   */
  void selectionChanged();

  /**
   * Emitted when a message wants its status to be changed
   */
  void messageStatusChangeRequest( MessagePtr msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear );

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
