#ifndef __FOLDERSELECTIONTREEWIDGET_H__
#define __FOLDERSELECTIONTREEWIDGET_H__

/******************************************************************************
 *
 * KMail Folder Selection Tree Widget
 *
 * Copyright (c) 1997-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2004-2005 Carsten Burghardt <burghardt@kde.org>
 * Copyright (c) 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *****************************************************************************/

#include <libkdepim/foldertreewidget.h>

class KMFolder;
class KAction;

namespace KMail {

class MainFolderView;
class FolderViewItem;
class FolderSelectionTreeWidgetItem;

/**
 * @brief A simple tree of folders useful for a "quick selection"
 * 
 * This widget displays a two column tree of folders with the folder
 * name on the left and its full path on the right. The tree is filled
 * by fetching data from a FolderTreeWiget.
 *
 * Items can be filtered by typing in a string to be matched in the
 * folder path.
 */
class FolderSelectionTreeWidget : public KPIM::FolderTreeWidget
{
  Q_OBJECT

public:
  /**
   * Construct the simple folder tree.
   * Note that the widget is initially empty and you must call reload()
   * to fill it up.
   *
   * @param parent The parent widget
   * @param folderTree The folder tree to fetch the hierarchy of folders from
   */
  FolderSelectionTreeWidget(
      QWidget *parent,
      ::KMail::MainFolderView *folderTree
   );

public:
  /**
   * Reload the tree and select which folders to show and which not
   *
   * @param mustBeReadWrite If true, the read-only folders become non selectable
   * @param showOutbox If trye, the otbox folder is shown
   * @param showImapFolders Whether to show the IMAP folder hierarchy
   * @param preSelection The initial folder to select
   */
  void reload(
      bool mustBeReadWrite,
      bool showOutbox,
      bool showImapFolders,
      const QString &preSelection = QString()
    );

  /**
   * Return the currently selected folder, or 0 if no folder is selected (yet)
   */
  KMFolder * folder() const;

  /**
   * Set the current folder.
   * The folder parameter must come from the KMFolderTree specified in the
   * constructor.
   */
  void setFolder( KMFolder *folder );
  /**
   * Set the current folder.
   * This is an overload that first lookups the folder by id in kmkernel.
   */
  void setFolder( const QString &idString );

  /**
   * Apply the given filter string.
   * Folders NOT matching the string are hidden and disabled (can't be selected).
   */
  void applyFilter( const QString &filter );

  /**
   * Returns the folder name column logical index.
   */
  int nameColumnIndex() const
    { return mNameColumnIndex; };

  /**
   * Returns the folder path column logical index.
   */
  int pathColumnIndex() const
    { return mPathColumnIndex; };

public slots:
  /**
   * Invokes the child folder creation dialog on the currently selected
   * folder in the widget. Nothing happens if there is no current folder.
   */
  void addChildFolder();

protected slots:
  /**
   * Pops up a contextual menu for the currently selected folder.
   * At the moment of writing the menu allows to invoke the addChildFolder()
   * method.
   */
  void slotContextMenuRequested( const QPoint & );

  /**
   * Selects the folder that was added. Connected to the folderAdded signal
   * when creating a new subfolder.
   */
  void slotFolderAdded( KMFolder *addedFolder );

  /**
   * Called when the selection changes.
   * See documentation for QTreeWidget::itemSelectionChanged()
   */
  void slotItemSelectionChanged();

protected:
  /**
   * Handles key presses for the purpose of filtering.
   */
  virtual void keyPressEvent( QKeyEvent *e );

  /**
   * Recursively fetches folder items from the FolderTreeWiget
   * by starting at fti (as root). This is internal api: use reload() instead.
   */
  void recursiveReload( FolderViewItem *fti, FolderSelectionTreeWidgetItem *parent );

signals:
  /**
   * Emitted when the tree widget selection changes, to inform the parent dialogue
   * of the actions that are allowed for the selected folder.
   *
   * @param allowOk if true, the OK action (accepting the selected folder) is allowed.
   * @param allowCreate if true, the "New Subfolder" action is allowed.
   */
  void actionsAllowed( bool allowOk, bool allowCreate );

private:
  int mNameColumnIndex;                 ///< The index of the folder name column
  int mPathColumnIndex;                 ///< The index of the path column
  KMail::MainFolderView* mFolderTree;   ///< The MainFolderView to fetch the data from
  QString mFilter;                      ///< The current folder path filter string

  bool mLastMustBeReadWrite;            ///< Internal state for reload()
  bool mLastShowOutbox;                 ///< Internal state for reload()
  bool mLastShowImapFolders;            ///< Internal state for reload()

  KAction *mCreateFolderAction;         ///< "New Subfolder" action for popup menu
};

} // namespace KMail

#endif /*!__FOLDERSELECTIONTREEWIDGET_H__*/
