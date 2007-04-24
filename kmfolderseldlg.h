/*
  KMail Folder Selection Dialog

  Copyright (c) 1997-1998 Stefan Taferner <taferner@kde.org>
  Copyright (c) 2004-2005 Carsten Burghardt <burghardt@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef kmfolderseldlg_h
#define kmfolderseldlg_h

#include <kdialog.h>
#include <kfoldertree.h>

class KMFolder;
class KMFolderTree;
class KMMainWidget;

namespace KMail {

  class SimpleFolderTree : public KFolderTree
  {
    Q_OBJECT

    public:
      SimpleFolderTree( QWidget * parent, KMFolderTree * folderTree,
                        const QString & preSelection, bool mustBeReadWrite );

      /** Reload the tree and select what folders to show and what not */
      void reload( bool mustBeReadWrite, bool showOutbox, bool showImapFolders,
                   const QString& preSelection = QString() );

      /** Return the current folder */
      const KMFolder * folder() const;

      /** Set the current folder */
      void setFolder( KMFolder* );
      void setFolder( const QString& idString );

      /** Apply the given filter. */
      void applyFilter( const QString& filter );

    public slots:
      void addChildFolder();

    protected slots:
      void slotContextMenuRequested( Q3ListViewItem *, const QPoint & );

    protected:
      /** Read color options and set palette. */
      virtual void readColorConfig( void );
      virtual void keyPressEvent( QKeyEvent *e );

      /** Folder and path column IDs. */
      friend class KMFolderSelDlg;
      int mFolderColumn;
      int mPathColumn;

    private:
      KMFolderTree* mFolderTree;
      QString mFilter;
      bool mLastMustBeReadWrite;
      bool mLastShowOutbox;
      bool mLastShowImapFolders;
};

  //-----------------------------------------------------------------------------
  class KMFolderSelDlg: public KDialog
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
      KMFolderSelDlg( KMMainWidget * parent, const QString& caption,
          bool mustBeReadWrite, bool useGlobalSettings = true );
      /**
       * Constructor with separate KMFolderTree
       * @param mustBeReadWrite if true, readonly folders are disabled
       * @param useGlobalSettings if true, the current folder is read and
       *        written to GlobalSettings
       */
      KMFolderSelDlg( QWidget * parent, KMFolderTree * tree,
          const QString& caption, bool mustBeReadWrite,
          bool useGlobalSettings = true );

      virtual ~KMFolderSelDlg();

      /** Returns selected folder */
      virtual KMFolder* folder( void );

      /** Set selected folder */
      void setFolder( KMFolder* folder );

      /** Set some flags what folders to show and what not */
      void setFlags( bool mustBeReadWrite, bool showOutbox, bool showImapFolders );

    protected slots:
      void slotSelect();
      void slotUser1();
      void slotUpdateBtnStatus();

    protected:
      void readConfig();
      void writeConfig();
      /** Init the dialog */
      void init();

      SimpleFolderTree * mTreeView;
      bool mUseGlobalSettings;
  };

} // namespace KMail

#endif /*kmfolderseldlg_h*/
