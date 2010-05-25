/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009, 2010 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef FOLDERTREEWIDGET_H
#define FOLDERTREEWIDGET_H

#include "kmail_export.h"
#include <QWidget>

#include <QAbstractItemView>
#include <akonadi/collection.h>

class KXMLGUIClient;
class QItemSelectionModel;
class FolderTreeView;
class ReadableCollectionProxyModel;
class KLineEdit;

namespace Akonadi {
  class StatisticsProxyModel;
}

/**
 * This is the widget that shows the main folder tree in KMail.
 *
 * It consists of the view (FolderTreeView) and a search line.
 * Internally, several proxy models are used on top of a entity tree model.
 */
class KMAIL_EXPORT FolderTreeWidget : public QWidget
{
  Q_OBJECT
public:
  enum TreeViewOption
  {
    None = 0,
    ShowUnreadCount = 1,
    UseLineEditForFiltering = 2
  };
  Q_DECLARE_FLAGS( TreeViewOptions, TreeViewOption )

  FolderTreeWidget( QWidget *parent = 0, KXMLGUIClient *xmlGuiClient = 0,
                    TreeViewOptions option = ShowUnreadCount );
  ~FolderTreeWidget();

  /**
   * The possible tooltip display policies.
   */
  enum ToolTipDisplayPolicy
  {
    DisplayAlways,           ///< Always display a tooltip when hovering over an item
    DisplayWhenTextElided,   ///< Display the tooltip if the item text is actually elided
    DisplayNever             ///< Nevery display tooltips
  };

  /**
   * The available sorting policies.
   */
  enum SortingPolicy
  {
    SortByCurrentColumn,      ///< Columns are clickable, sorting is by the current column
    SortByDragAndDropKey      ///< Columns are NOT clickable, sorting is done by drag and drop
  };



  void selectCollectionFolder( const Akonadi::Collection & col );

  void setSelectionMode( QAbstractItemView::SelectionMode mode );

  QAbstractItemView::SelectionMode selectionMode() const;

  QItemSelectionModel * selectionModel () const;

  QModelIndex currentIndex() const;

  Akonadi::Collection selectedCollection() const;

  Akonadi::Collection::List selectedCollections() const;

  FolderTreeView *folderTreeView();

  Akonadi::StatisticsProxyModel * statisticsProxyModel();

  ReadableCollectionProxyModel *readableCollectionProxyModel();

  void quotaWarningParameters( const QColor &color, qreal threshold );
  void readQuotaConfig();

  KLineEdit *filterFolderLineEdit();
  void applyFilter( const QString& );

  void disableContextMenuAndExtraColumn();

  void readConfig();

protected:
  void changeToolTipsPolicyConfig( ToolTipDisplayPolicy );

protected slots:
  void slotChangeTooltipsPolicy( FolderTreeWidget::ToolTipDisplayPolicy );

private:
  class FolderTreeWidgetPrivate;
  FolderTreeWidgetPrivate * const d;
};

#endif

