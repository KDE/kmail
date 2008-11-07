/******************************************************************************
 *
 * Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************/

#ifndef __KMAIL_MAINFOLDERVIEW_H__
#define __KMAIL_MAINFOLDERVIEW_H__

#include "folderview.h"

#include <kdemacros.h> // for KDE_DEPRECATED

#include <QMap>

class KMFolder;
class QAction;
class QMenu;

namespace KMail
{

class MainFolderViewFoldersDropAction;

/*
 * @brief The main folder tree view for the KMail main window
 *
 * Displays the tree of folders managed by KMail.
 *
 */
class MainFolderView : public FolderView
{
  Q_OBJECT
public:
  MainFolderView( KMMainWidget *mainWidget, FolderViewManager *manager, QWidget *parent, const char *name = 0 );

public:
  /**
   * Valid actions for the folderToPopup method
   */
  enum MenuAction
  {
    CopyMessage,
    MoveMessage,
    CopyFolder,
    MoveFolder
  };

  /**
   * Generate a popup menu that contains all folders that can have content
   * and thus can be target of a "copy" or "move" action.
   * The menu's triggered( QAction * ) signal is connected to the specified target
   * slotMoveSelectedMessagesToFolder() and slotCopySelectedMessagesToFolder()
   * in case of Message actions and slotMoveSelectedFoldersToFolder() and
   * slotCopySelectedFoldersToFolder() in case of Folder actions.
   * The QActions added to the menu have the KMFolder pointer set as data().
   *
   * @param action The purpose of the menu
   * @param target The target of the signals
   * @param menu The menu to be filled
   */
  void folderToPopupMenu( MenuAction action, QObject * target, QMenu *menu );

protected:
  /**
   * Reimplemented pure virtual from base class. This creates all the items unconditionally.
   */
  virtual FolderViewItem * createItem(
      FolderViewItem *parent,
      const QString &label,
      KMFolder *folder,
      KPIM::FolderTreeWidgetItem::Protocol proto,
      KPIM::FolderTreeWidgetItem::FolderType type
    );

  /**
   * Reimplemented in order to fill in the folder tree structure related actions
   */
  virtual void fillContextMenuTreeStructureRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection );

  /**
   * Reimplemented in order to fill in the view structure related actions
   */
  virtual void fillContextMenuViewStructureRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection );

  // DND Machinery

  /**
   * Return true if the specified item will accept the drop of
   * the currently dragged folders. Return false otherwise.
   */
  bool acceptDropCopyOrMoveFolders( FolderViewItem *item );

  /**
   * Reimplemented in order to allow for both sorting and moving folders around.
   */
  virtual void handleFoldersDragMoveEvent( QDragMoveEvent *e );

  /**
   * Reimplemented in order to allow for both sorting and moving folders around.
   */
  virtual void handleFoldersDropEvent( QDropEvent *e );

protected slots:
  /**
   * Moves the currently selected folders to the folder specified as the QAction payload.
   */
  void slotMoveSelectedFoldersToFolder( QAction *act );

  /**
   * Copies the currently selected folders to the folder specified as the QAction payload.
   */
  void slotCopySelectedFoldersToFolder( QAction *act );

private:
  /**
   * The internal version of the public folderToPopupMenu(). It takes
   * the initial item for which the children should be added to the menu.
   * Not meant to be called from extern.
   */
  void folderToPopupMenuInternal(
      MenuAction action, QObject * target, QMenu *menu, QTreeWidgetItem * parentItem
    );

  /**
   * Internal helper for the handleFolders*Event() DND related methods.
   */
  void computeFoldersDropAction( QDropEvent *e, MainFolderViewFoldersDropAction *act );
};

}

#endif //!__KMAIL_MAINFOLDERVIEW_H__
