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

class KLineEdit;
class QTimer;
class QToolButton;

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
  QToolButton * mStatusFilterButton;
  QToolButton * mSortOrderButton;
  QToolButton * mAggregationButton;
  QToolButton * mThemeButton;

  StorageModel * mStorageModel;          ///< The currently displayed storage, an owned copy
  Aggregation * mAggregation;            ///< The currently set aggregation mode, an owned copy (eventually even modificable)
  Theme * mTheme;                          ///< The currently set theme, an owned copy (eventually even modificable)
  Filter * mFilter;                      ///< The currently applied filter, an owned copy
  bool mStorageUsesPrivateTheme;
  bool mStorageUsesPrivateAggregation;
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
  void aggregationSelected( bool );
  void statusMenuAboutToShow();
  void statusSelected( QAction *action );
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
  void setDefaultAggregationForStorageModel( const StorageModel * storageModel );
  void setDefaultThemeForStorageModel( const StorageModel * storageModel );
  void applyFilter();
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail


#endif //!__KMAIL_MESSAGELISTVIEW_CORE_WIDGET_H__

