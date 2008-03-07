/*
    Copyright (c) 2008 Pradeepto K. Bhattacharya <pradeepto@kde.org>
      ( adapted from kdepim/kmail/kmfolderseldlg.cpp and simplefoldertree.h  )

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef KMAIL_TREEBASE_H
#define KMAIL_TREEBASE_H

#include "kmfolder.h"
#include "kmfoldertree.h"

#include <kdebug.h>
#include <klistview.h>

namespace KMail {

class TreeItemBase;

class TreeBase : public KListView
{
    Q_OBJECT
    public:
    TreeBase( QWidget * parent, KMFolderTree *folderTree,
        const QString &preSelection, bool mustBeReadWrite );
	

     const KMFolder * folder() const;
    /** Set the current folder */
    void setFolder( KMFolder *folder );

    inline void setFolder( const QString& idString )
    {
      setFolder( kmkernel->findFolderById( idString ) );
    }

    void reload( bool mustBeReadWrite, bool showOutbox, bool showImapFolders,
                   const QString& preSelection = QString::null );

    const int folderColumn() const {  return mFolderColumn; }
    void setFolderColumn( const int folderCol ) { mFolderColumn = folderCol; }
    const int pathColumn() const { return mPathColumn; }
    void setPathColumn( const int pathCol ) { mPathColumn = pathCol; }

    public slots:
	void addChildFolder();
    protected slots:
	void slotContextMenuRequested( QListViewItem *lvi,
					      const QPoint &p );
    void recolorRows();
protected:
    virtual QListViewItem* createItem( QListView* ) = 0;
    virtual QListViewItem* createItem( QListView*, QListViewItem* ) = 0;
    virtual QListViewItem* createItem( QListViewItem* ) = 0;
    virtual QListViewItem* createItem( QListViewItem*, QListViewItem* ) = 0;

  protected:
      KMFolderTree* mFolderTree;
      QString mFilter;
      bool mLastMustBeReadWrite;
      bool mLastShowOutbox;
      bool mLastShowImapFolders;
      /** Folder and path column IDs. */
      int mFolderColumn;
      int mPathColumn;
     
};
}
#endif
