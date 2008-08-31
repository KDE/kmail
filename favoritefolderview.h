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

#ifndef KMAIL_FAVORITEFOLDERVIEW_H
#define KMAIL_FAVORITEFOLDERVIEW_H

#include "kmfoldertree.h"

class QEvent;

namespace KMail {

class FavoriteFolderView;

class FavoriteFolderViewItem : public KMFolderTreeItem
{
  Q_OBJECT
  public:
    FavoriteFolderViewItem( FavoriteFolderView *parent, const QString & name, KMFolder* folder );

  protected:
    bool useTopLevelIcon() const { return false; }
    int iconSize() const { return 22; }

  private slots:
    void nameChanged();

  private:
    QString mOldName;
};

class FavoriteFolderView : public FolderTreeBase
{
  Q_OBJECT

  public:
    explicit FavoriteFolderView( KMMainWidget *mainWidget, QWidget *parent = 0 );
    ~FavoriteFolderView();

    void readConfig();
    void writeConfig();

    KMFolderTreeItem* addFolder( KMFolder *folder, const QString &name = QString(),
                                 Q3ListViewItem *after = 0 );
    void addFolder( KMFolderTreeItem *fti );

  public slots:
    void folderTreeSelectionChanged( KMFolder *folder );
    void checkMail();

  protected:
    bool acceptDrag(QDropEvent* e) const;
    virtual void contentsDragEnterEvent( QDragEnterEvent *e );
    void readColorConfig();

  private:
    static QString prettyName( KMFolderTreeItem* fti );
    KMFolderTreeItem* findFolderTreeItem( KMFolder* folder ) const;
    void handleGroupwareFolder( KMFolderTreeItem *fti );

  private slots:
    void selectionChanged();
    void itemClicked( int button, Q3ListViewItem *item );
    void folderRemoved( KMFolder *folder );
    void dropped( QDropEvent *e, Q3ListViewItem *after );
    void contextMenu( Q3ListViewItem *item, const QPoint &point );
    void removeFolder();
    void initializeFavorites();
    void renameFolder();
    void addFolder();
    void notifyInstancesOnChange();
    void refresh();

  private:
    QPointer<KMFolderTreeItem> mContextMenuItem;
    static QList<FavoriteFolderView*> mInstances;
    bool mReadingConfig;
};

}

#endif
