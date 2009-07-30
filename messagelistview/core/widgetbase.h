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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_WIDGETBASE_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_WIDGETBASE_H__

#include <QWidget>
#include <QString>
#include <QList>

#include "messagelistview/core/enums.h"
#include "messagelistview/core/sortorder.h"

class KLineEdit;
class QTimer;
class QToolButton;
class QActionGroup;
class QHeaderView;
class KMenu;
class KComboBox;

namespace KPIM
{
  class MessageStatus;
}

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class Aggregation;
class Filter;
class GroupHeaderItem;
class Manager;
class MessageItem;
class Theme;
class StorageModel;
class View;

/**
 * Provides a widget which has the messagelist and the most important helper widgets,
 * like the search line and the comboboxes for changing status filtering, aggregation etc.
 */
class Widget : public QWidget
{
  friend class View;
  friend class Manager;

  Q_OBJECT
public:
  Widget( QWidget *parent );
  ~Widget();

private:
  View *mView;
  QString mLastAggregationId;
  QString mLastThemeId;
  KLineEdit * mSearchEdit;
  QTimer * mSearchTimer;
  KComboBox * mStatusFilterCombo;

  StorageModel * mStorageModel;          ///< The currently displayed storage. The storage itself
                                         ///  is owned by MessageListView::Widget.
  Aggregation * mAggregation;            ///< The currently set aggregation mode, a deep copy
  Theme * mTheme;                        ///< The currently set theme, a deep copy
  SortOrder mSortOrder;                  ///< The currently set sort order
  Filter * mFilter;                      ///< The currently applied filter, owned by us.
  bool mStorageUsesPrivateTheme;         ///< true if the current folder does not use the global theme
  bool mStorageUsesPrivateAggregation;   ///< true if the current folder does not use the global aggregation
  bool mStorageUsesPrivateSortOrder;     ///< true if the current folder does not use the global sort order
  int mFirstTagInComboIndex;             ///< the index of the combobox where the first tag starts
public:
  /**
   * Sets the storage model for this Widget.
   *
   * Pre-selection is the action of automatically selecting a message just after the folder
   * has finished loading. See Model::setStorageModel() for more information.
   */
  void setStorageModel( StorageModel * storageModel, PreSelectionMode preSelectionMode = PreSelectLastSelected );

  /**
   * Returns the StorageModel currently set. May be 0.
   */
  StorageModel * storageModel() const
    { return mStorageModel; };

  /**
   * Returns the search line of this widget. Can be 0 if the quick search
   * is disabled in the global configuration.
   */
  KLineEdit *quickSearch() const
    { return mSearchEdit; }

  /**
   * Returns the View attached to this Widget. Never 0.
   */
  View * view() const
    { return mView; };

  /**
   * Returns the KPIM::MessageStatus in the current quicksearch field.
   */
  KPIM::MessageStatus currentFilterStatus() const;

  /**
   * Returns the search term in the current quicksearch field.
   */
  QString currentFilterSearchString() const;

  /**
   * Returns the id of the MessageItem::Tag currently set in the quicksearch field.
   */
  QString currentFilterTagId() const;

public slots:

  /**
   * This is called to setup the status filter's KComboBox.
   */
  void populateStatusFilterCombo();

protected:
  /**
   * This is called by Manager when the option sets stored within have changed.
   */
  void aggregationsChanged();

  /**
   * This is called by Manager when the option sets stored within have changed.
   */
  void themesChanged();

  /**
   * Called when the "Message Status/Tag" filter menu is opened by the user.
   * You may override this function in order to add some "custom tag" entries
   * to the menu. The entries should be placed in a QActionGroup which should be returned
   * to the caller. The QAction objects associated to the entries should have
   * the string id of the tag set as data() and the tag icon set as icon().
   * The default implementation does nothing.
   */
  virtual void fillMessageTagCombo( KComboBox * combo );

  /**
   * This is called by View when a message is single-clicked (thus selected and made current)
   */
  virtual void viewMessageSelected( MessageItem *msg );

  /**
   * This is called by View when a message is double-clicked or activated by other input means
   */
  virtual void viewMessageActivated( MessageItem *msg );

  /**
   * This is called by View when selection changes.
   */
  virtual void viewSelectionChanged();

  /**
   * This is called by View when a message is right clicked.
   */
  virtual void viewMessageListContextPopupRequest( const QList< MessageItem * > &selectedItems, const QPoint &globalPos );

  /**
   * This is called by View when a group header is right clicked.
   */
  virtual void viewGroupHeaderContextPopupRequest( GroupHeaderItem *group, const QPoint &globalPos );

  /**
   * This is called by View when a drag enter event is received
   */
  virtual void viewDragEnterEvent( QDragEnterEvent * e );

  /**
   * This is called by View when a drag move event is received
   */
  virtual void viewDragMoveEvent( QDragMoveEvent * e );

  /**
   * This is called by View when a drop event is received
   */
  virtual void viewDropEvent( QDropEvent * e );

  /**
   * This is called by View when a drag can possibly be started
   */
  virtual void viewStartDragRequest();

  /**
   * This is called by View when a message item is manipulated by the user
   * in a way that it's status should change. (e.g, by clicking on a status icon, for example).
   */
  virtual void viewMessageStatusChangeRequest( MessageItem *msg, const KPIM::MessageStatus &set, const KPIM::MessageStatus &clear );

  /**
   * This is called by View to signal a start of a (possibly lengthy) job batch.
   */
  virtual void viewJobBatchStarted();

  /**
   * This is called by View to signal the end of a (possibly lengthy) job batch.
   */
  virtual void viewJobBatchTerminated();

  void tagIdSelected( QVariant data );

signals:
  /**
   * Emitted when a full search is requested.
   */
  void fullSearchRequest();

protected slots:

  void themeMenuAboutToShow();
  void aggregationMenuAboutToShow();
  void themeSelected( bool );
  void configureThemes();
  void setPrivateThemeForStorage();
  void setPrivateAggregationForStorage();
  void setPrivateSortOrderForStorage();
  void aggregationSelected( bool );
  void statusSelected( int index );
  void searchEditTextEdited( const QString &text );
  void searchTimerFired();
  void searchEditClearButtonClicked();
  void sortOrderMenuAboutToShow();
  void messageSortingSelected( QAction *action );
  void messageSortDirectionSelected( QAction *action );
  void groupSortingSelected( QAction *action );
  void groupSortDirectionSelected( QAction *action );

  /**
   * Handles header section clicks switching the Aggregation MessageSorting on-the-fly.
   */
  void slotViewHeaderSectionClicked( int logicalIndex );

private:

  /**
   * Small helper for switching SortOrder::MessageSorting and SortOrder::SortDirection
   * on the fly.
   * After doing this, the sort indicator in the header is updated.
   */
  void switchMessageSorting( SortOrder::MessageSorting messageSorting,
                             SortOrder::SortDirection sortDirection,
                             int logicalHeaderColumnIndex );

  /**
   * Check if our sort order can still be used with this aggregation.
   * This can happen if the global aggregation changed, for example we can now
   * have "most recent in subtree" sorting with an aggregation without threading.
   * If this happens, reset to the default sort order and don't use the global sort
   * order.
   */
  void checkSortOrder( const StorageModel *storageModel );

  void setDefaultAggregationForStorageModel( const StorageModel * storageModel );
  void setDefaultThemeForStorageModel( const StorageModel * storageModel );
  void setDefaultSortOrderForStorageModel( const StorageModel * storageModel );
  void applyFilter();
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail


#endif //!__KMAIL_MESSAGELISTVIEW_CORE_WIDGET_H__

