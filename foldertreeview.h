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

#ifndef FOLDERTREEVIEW_H
#define FOLDERTREEVIEW_H

#include <akonadi/entitytreeview.h>
#include <akonadi/collection.h>
#include "foldertreewidget.h"

class QMouseEvent;

namespace Akonadi
{
class CollectionStatisticsDelegate;
}

class MailCommon;

/**
 * This is an enhanced EntityTreeView specially suited for the folders in KMail's
 * main folder widget.
 */
class FolderTreeView : public Akonadi::EntityTreeView
{
  Q_OBJECT
public:
  explicit FolderTreeView( MailCommon* mailCommon, QWidget *parent = 0, bool showUnreadCount = true );

  explicit FolderTreeView( MailCommon* mailCommon, KXMLGUIClient *xmlGuiClient, QWidget *parent = 0, bool showUnreadCount = true );

  virtual ~FolderTreeView();

  void selectNextUnreadFolder( bool confirm = false);
  void selectPrevUnreadFolder( bool confirm = false);

  void showStatisticAnimation( bool anim );

  void disableContextMenuAndExtraColumn();

  void setTooltipsPolicy( FolderTreeWidget::ToolTipDisplayPolicy );

  Akonadi::Collection currentFolder() const;

protected:
  enum Move { Next = 0, Previous = 1};
  void init( bool showUnreadCount );
  void selectModelIndex( const QModelIndex & );
  void setCurrentModelIndex( const QModelIndex & );
  QModelIndex selectNextFolder( const QModelIndex & current );
  bool isUnreadFolder( const QModelIndex & current, QModelIndex &nextIndex,FolderTreeView::Move move, bool confirm);
  void readConfig();
  void writeConfig();

  void setSortingPolicy( FolderTreeWidget::SortingPolicy policy );

  virtual void mousePressEvent( QMouseEvent *e );


public slots:
  void slotFocusNextFolder();
  void slotFocusPrevFolder();
  void slotSelectFocusFolder();

protected slots:
  void slotHeaderContextMenuRequested( const QPoint& );
  void slotHeaderContextMenuChangeIconSize( bool );
  void slotHeaderContextMenuChangeHeader( bool );
  void slotHeaderContextMenuChangeToolTipDisplayPolicy( bool );
  void slotHeaderContextMenuChangeSortingPolicy( bool );

signals:
  void changeTooltipsPolicy( FolderTreeWidget::ToolTipDisplayPolicy );
  void manualSortingChanged( bool actif );
  void prefereCreateNewTab( bool );
private:
  FolderTreeWidget::ToolTipDisplayPolicy mToolTipDisplayPolicy;
  FolderTreeWidget::SortingPolicy mSortingPolicy;
  Akonadi::CollectionStatisticsDelegate *mCollectionStatisticsDelegate;
  bool mbDisableContextMenuAndExtraColumn;
  bool mLastButtonPressedWasMiddle;
  MailCommon* mMailCommon;
};



#endif /* FOLDERTREEVIEW_H */

