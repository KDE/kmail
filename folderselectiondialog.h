#ifndef __FOLDERSELECTIONDIALOG_H__
#define __FOLDERSELECTIONDIALOG_H__

/******************************************************************************
 *
 * KMail Folder Selection Dialog
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

#include <kdialog.h>          // kdelibs

class KMFolder;
class KMFolderTree;
class KMMainWidget;

namespace KMail {

class FolderSelectionTreeWidget;


/**
 * @brief A dialog used to select a mail folder
 */
class FolderSelectionDialog: public KDialog
{
  Q_OBJECT

public:
  /**
   * Constructor with KMMainWidget
   * @p parent @em must be a KMMainWin, because we
   *    need its foldertree.
   * @param mustBeReadWrite if true, readonly folders are disabled
   * @param useGlobalSettings if true, the current folder is read and
   *        written to GlobalSettings
   */
  FolderSelectionDialog(
      KMMainWidget *parent,
      const QString& caption,
      bool mustBeReadWrite,
      bool useGlobalSettings = true
    );

  /**
   * Constructor with separate KMFolderTree
   * @param mustBeReadWrite if true, readonly folders are disabled
   * @param useGlobalSettings if true, the current folder is read and
   *        written to GlobalSettings
   */
  FolderSelectionDialog(
      QWidget * parent,
      KMFolderTree * tree,
      const QString& caption,
      bool mustBeReadWrite,
      bool useGlobalSettings = true
    );

  virtual ~FolderSelectionDialog();

public:
  /**
   * Returns the currently selected folder, or 0 if no folder is selected (yet)
   */
  KMFolder * folder() const;

  /**
   * Set the selected folder. Forwarded to FolderSelectionTreeWidget.
   */
  void setFolder( KMFolder *folder );

  /**
   * Set some flags what folders to show and what not.
   * Triggers a reload of the folders.
   */
  void setFlags( bool mustBeReadWrite, bool showOutbox, bool showImapFolders );

protected slots:
  /**
   * Called to select a folder and close the dialog
   */
  void slotSelect();

  /**
   * Called when selection in the tree view changes in order
   * to update the enabled/disabled state of the dialog buttons.
   */
  void slotUpdateBtnStatus();

protected:
  /**
   * Read the global dialog configuration.
   */
  void readConfig();

  /**
   * Save the global dialog configuration.
   */
  void writeConfig();
  /**
   * Common initialization for all the constructors.
   */
  void init( KMFolderTree *tree, bool mustBeReadWrite );

private:
  FolderSelectionTreeWidget * mTreeView;
  bool mUseGlobalSettings;

};

} // namespace KMail

#endif /*!__FOLDERSELECTIONDIALOG_H__*/
