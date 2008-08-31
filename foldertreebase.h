/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef KMAIL_FOLDERTREEBASE_H
#define KMAIL_FOLDERTREEBASE_H

#include <libkdepim/kfoldertree.h>

class KMFolder;
class KMMainWidget;

namespace KMail {

class FolderTreeBase : public KPIM::KFolderTree
{
  Q_OBJECT
  public:
    explicit FolderTreeBase( KMMainWidget *mainWidget, QWidget *parent = 0, const char *name = 0 );

    /** Returns the main widget that this widget is a child of. */
    KMMainWidget* mainWidget() const { return mMainWidget; }

    /** Find index of given folder. Returns 0 if not found */
    virtual Q3ListViewItem* indexOfFolder( const KMFolder *folder ) const
    {
       if ( mFolderToItem.contains( folder ) )
         return mFolderToItem[ folder ];
       else
         return 0;
    }

    void insertIntoFolderToItemMap( const KMFolder *folder, Q3ListViewItem *item )
    {
      mFolderToItem.insert( folder, item );
    }

    void removeFromFolderToItemMap( const KMFolder *folder )
    {
      mFolderToItem.remove( folder );
    }

  signals:
    /** Messages have been dropped onto a folder */
    void folderDrop(KMFolder*);

    /** Messages have been dropped onto a folder with Ctrl */
    void folderDropCopy(KMFolder*);

    void triggerRefresh();

  public slots:
    /** Update the total and unread columns (if available, or if forced) */
    void slotUpdateCounts(KMFolder *folder, bool force = false );

  protected:
    enum {
      DRAG_COPY = 0,
      DRAG_MOVE = 1,
      DRAG_CANCEL = 2
    };
    int dndMode( bool alwaysAsk = false );
    void contentsDropEvent( QDropEvent *e );

    /** Catch palette changes */
    virtual bool event(QEvent *e);

    /** Read color options and set palette. */
    virtual void readColorConfig();

    /** Checks if the local inbox should be hidden. */
    bool hideLocalInbox() const;

    /** Handle drop of a MailList object. */
    void handleMailListDrop( QDropEvent *event, KMFolder *destination );

  protected:
    KMMainWidget *mMainWidget;
    QMap<const KMFolder*, Q3ListViewItem*> mFolderToItem;
};

}

#endif
