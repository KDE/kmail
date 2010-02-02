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

#ifndef FOLDERTREEVIEW_H
#define FOLDERTREEVIEW_H

#include <akonadi/entitytreeview.h>
#include <akonadi/collection.h>
#include "folderselectiontreeview.h"

namespace Akonadi
{
class CollectionStatisticsDelegate;
}

class FolderTreeView : public Akonadi::EntityTreeView
{
  Q_OBJECT
public:
  explicit FolderTreeView( QWidget *parent = 0 );

  explicit FolderTreeView( KXMLGUIClient *xmlGuiClient, QWidget *parent = 0 );

  virtual ~FolderTreeView();

  void selectNextUnreadFolder( bool confirm = false);
  void selectPrevUnreadFolder( bool confirm = false);


  void disableContextMenuAndExtraColumn();

  void setTooltipsPolicy( FolderSelectionTreeView::ToolTipDisplayPolicy );

  Akonadi::Collection currentFolder();
protected:
  enum Move { Next = 0, Previous = 1};
  void init();
  void selectModelIndex( const QModelIndex & );
  QModelIndex selectNextFolder( const QModelIndex & current );
  bool isUnreadFolder( const QModelIndex & current, QModelIndex &nextIndex,FolderTreeView::Move move, bool confirm);
  void readConfig();
  void writeConfig();

public slots:
  void slotFocusNextFolder();
  void slotFocusPrevFolder();

protected slots:
  void slotHeaderContextMenuRequested( const QPoint& );
  void slotHeaderContextMenuChangeIconSize( bool );
  void slotHeaderContextMenuChangeHeader( bool );
  void slotHeaderContextMenuChangeToolTipDisplayPolicy( bool );


signals:
  void changeTooltipsPolicy( FolderSelectionTreeView::ToolTipDisplayPolicy );

private:
  bool mbDisableContextMenuAndExtraColumn;
  FolderSelectionTreeView::ToolTipDisplayPolicy mToolTipDisplayPolicy;
  Akonadi::CollectionStatisticsDelegate *mCollectionStatisticsDelegate;
};



#endif /* FOLDERTREEVIEW_H */

