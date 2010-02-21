/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

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

#ifndef FOLDERSELECTIONTREEVIEW_H
#define FOLDERSELECTIONTREEVIEW_H

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
  class EntityTreeModel;
  class StatisticsProxyModel;
}
class KMAIL_EXPORT FolderSelectionTreeView : public QWidget
{
  Q_OBJECT
public:
  FolderSelectionTreeView( QWidget *parent = 0, KXMLGUIClient *xmlGuiClient = 0, bool showUnreadCount = true );
  ~FolderSelectionTreeView();

  /**
   * The possible tooltip display policies.
   */
  enum ToolTipDisplayPolicy
  {
    DisplayAlways,           ///< Always display a tooltip when hovering over an item
    DisplayWhenTextElided,   ///< Display the tooltip if the item text is actually elided
    DisplayNever             ///< Nevery display tooltips
  };

  void selectCollectionFolder( const Akonadi::Collection & col );


  void setSelectionMode( QAbstractItemView::SelectionMode mode );

  QAbstractItemView::SelectionMode selectionMode() const;

  QItemSelectionModel * selectionModel () const;

  QModelIndex currentIndex() const;

  Akonadi::Collection selectedCollection() const;

  Akonadi::Collection::List selectedCollections() const;

  FolderTreeView *folderTreeView();
  Akonadi::EntityTreeModel *entityModel();

  Akonadi::StatisticsProxyModel * statisticsProxyModel();

  ReadableCollectionProxyModel *readableCollectionProxyModel();

  void quotaWarningParameters( const QColor &color, qreal threshold );
  void readQuotaConfig();

  KLineEdit *filterFolderLineEdit();

  void disableContextMenuAndExtraColumn();

  void readConfig();
protected:
  void changeToolTipsPolicyConfig( ToolTipDisplayPolicy );

protected slots:
  void slotChangeTooltipsPolicy( FolderSelectionTreeView::ToolTipDisplayPolicy );


private:
  class FolderSelectionTreeViewPrivate;
  FolderSelectionTreeViewPrivate * const d;
};

#endif /* FOLDERSELECTIONTREEVIEW_H */

