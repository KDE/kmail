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

#ifndef __AKONADI_MESSAGELISTVIEW_WIDGET_H__
#define __AKONADI_MESSAGELISTVIEW_WIDGET_H__

#include "messagelistview/core/widgetbase.h"

#include <akonadi/item.h>

#include <boost/shared_ptr.hpp>
#include <kmime/kmime_message.h>

#include "kmail_export.h"

class QWidget;

typedef boost::shared_ptr<KMime::Message> MessagePtr;

namespace Akonadi
{

namespace MessageListView
{

/**
 * The KMail specific implementation of the Core::Widget.
 *
 * Provides an interface over a KMFolder. In the future
 * it's expected to wrap Akonadi::MessageModel.
 */
class KMAIL_EXPORT Widget : public KMail::MessageListView::Core::Widget
{
  Q_OBJECT

public:
  /**
   * Create a Widget wrapping the specified folder.
   */
  explicit Widget( QWidget *parent );
  ~Widget();

  /**
   * Returns true if this drag can be accepted by the underlying view
   */
  bool canAcceptDrag( const QDragMoveEvent *e );

protected:
  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewMessageSelected( KMail::MessageListView::Core::MessageItem *msg );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewMessageActivated( KMail::MessageListView::Core::MessageItem *msg );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewSelectionChanged();

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewMessageListContextPopupRequest( const QList< KMail::MessageListView::Core::MessageItem * > &selectedItems, const QPoint &globalPos );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewGroupHeaderContextPopupRequest( KMail::MessageListView::Core::GroupHeaderItem *group, const QPoint &globalPos );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewDragEnterEvent( QDragEnterEvent * e );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewDragMoveEvent( QDragMoveEvent * e );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewDropEvent( QDropEvent * e );

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewStartDragRequest();

  /**
   * Reimplemented from MessageListView::Core::Widget
   */
  virtual void viewMessageStatusChangeRequest( KMail::MessageListView::Core::MessageItem *msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear );

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
  void messageSelected( const Akonadi::Item &item );

  /**
   * Emitted when a message is doubleclicked or activated by other input means
   */
  void messageActivated( const Akonadi::Item &item );

  /**
   * Emitted when the selection in the view changes.
   */
  void selectionChanged();

  /**
   * Emitted when a message wants its status to be changed
   */
  void messageStatusChangeRequest( const Akonadi::Item &item, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear );

private:
  Item::List selectionAsItems() const;
  Item itemForRow( int row ) const;
  MessagePtr messageForRow( int row ) const;

  int mLastSelectedMessage;
};

} // namespace MessageListView

} // namespace Akonadi

#endif //!__AKONADI_MESSAGELISTVIEW_WIDGET_H__
