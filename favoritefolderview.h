/******************************************************************************
 *
 *  Copyright (c) 2007 Volker Krause <vkrause@kde.org>
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

#ifndef __KMAIL_FAVORITEFOLDERVIEW_H__
#define __KMAIL_FAVORITEFOLDERVIEW_H__

#include "folderview.h"


namespace KMail
{

/**
 * @brief The view of favorite folders
 *
 * This widget shows a subset of all the folders managed by KMail.
 * The subset is defined in GlobalSettings::favoriteFolderIds().
 */
class FavoriteFolderView : public FolderView
{
  Q_OBJECT
public:
  /**
   * Create the favorite folders view attacching it to the
   * specified main widget and the specified folder manager.
   */
  FavoriteFolderView( KMMainWidget *mainWidget, FolderViewManager *manager, QWidget *parent, const char *name = 0 );
public:
  /**
   * Allows adding a favorite folder from outside. MainFolderView uses this actually.
   */
  void addFolder( KMFolder *fld );

protected:
  /**
   * Reimplemented pure virtual from base class. This creates only the items that are present in the
   * favorite folders list (in GlobalSettings).
   */
  virtual FolderViewItem * createItem(
      FolderViewItem *parent,
      const QString &label,
      KMFolder *folder,
      KPIM::FolderTreeWidgetItem::Protocol proto,
      KPIM::FolderTreeWidgetItem::FolderType type
    );

  /**
   * Reimplemented in order to fill in the view structure related actions
   */
  virtual void fillContextMenuViewStructureRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection );

  /**
   * Reimplemented to add a "Add Favorite Folder" entry
   */
  virtual void fillContextMenuNoItem( KMenu *mneu );

  /**
   * Reimplemented in order to allow for both sorting and moving folders around.
   */
  virtual void handleFoldersDragMoveEvent( QDragMoveEvent *e );

  /**
   * Reimplemented in order to allow for both sorting and moving folders around.
   */
  virtual void handleFoldersDropEvent( QDropEvent *e );

  /**
   * Reimplemented in order to handle groupware folders
   */
  virtual void activateItemInternal( FolderViewItem *fvi, bool keepSelection, bool notifyManager, bool middleButton );

  /**
   * Internal helper to add a folder to the favorites view
   */
  FolderViewItem * addFolderInternal( KMFolder *fld );

  /**
   * Stores all the folders in the favorites view in the GlobalSettings
   * This internally iterates over the list items, do not call this when the
   * favorite folder view is not fully loaded!
   */
  void storeFavorites();

public Q_SLOTS:
  /**
   * Attempts to check mail for all the favorite folders.
   */
  void checkMail();

  /**
   * Shows the "add favorite folder" dialog.
   */
  void addFolder();

protected Q_SLOTS:
  /**
   * Starts editing of the currently selected folder
   */
  void renameFolder();

  /**
   * Removes all the currently selected folders from favorites
   */
  void removeFolders();

private:

  /**
   * Adds the "Add Favorite Folder..." action to a menu
   */
  void appendAddFolderActionToMenu( KMenu *menu ) const;
};

}

#endif
