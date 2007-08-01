/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>
    Copyright (c) Stefan Taferner <taferner@kde.org>

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

#ifndef KMAIL_SIMPLEFOLDERTREE_H
#define KMAIL_SIMPLEFOLDERTREE_H

#include "kmfoldertree.h"

#include <kdebug.h>
#include <klistview.h>

class KMFolder;
class KMFolderTree;

namespace KMail {

template <class T> class SimpleFolderTreeItem : public T
{
  public:
    SimpleFolderTreeItem( QListView * listView ) : T( listView ), mFolder( 0 ) {}
    SimpleFolderTreeItem( QListView * listView, QListViewItem * afterListViewItem ) :
        T( listView, afterListViewItem ), mFolder( 0 ) {}
    SimpleFolderTreeItem( QListViewItem * listViewItem ) : T( listViewItem ), mFolder( 0 ) {}
    SimpleFolderTreeItem( QListViewItem * listViewItem, QListViewItem * afterListViewItem ) :
        T( listViewItem, afterListViewItem ), mFolder( 0 ) {}

    void setFolder( KMFolder * folder ) { mFolder = folder; };
    const KMFolder * folder() { return mFolder; };

  private:
    KMFolder * mFolder;
};

template <> class SimpleFolderTreeItem<QCheckListItem> : public QCheckListItem
{
  public:
    SimpleFolderTreeItem( QListView * listView ) :
        QCheckListItem( listView, QString(), CheckBox ), mFolder( 0 ) {}
    SimpleFolderTreeItem( QListView * listView, QListViewItem * afterListViewItem ) :
        QCheckListItem( listView, afterListViewItem, QString(), CheckBox ), mFolder( 0 ) {}
    SimpleFolderTreeItem( QListViewItem * listViewItem ) :
        QCheckListItem( listViewItem, QString(), CheckBox ), mFolder( 0 ) {}
    SimpleFolderTreeItem( QListViewItem * listViewItem, QListViewItem * afterListViewItem ) :
        QCheckListItem( listViewItem, afterListViewItem, QString(), CheckBox ), mFolder( 0 ) {}

    void setFolder( KMFolder * folder ) { mFolder = folder; };
    const KMFolder * folder() { return mFolder; };

  private:
    KMFolder * mFolder;
};

template <class T> class SimpleFolderTreeBase : public KListView
{
  public:
    inline SimpleFolderTreeBase( QWidget * parent, KMFolderTree *folderTree,
                      const QString &preSelection, bool mustBeReadWrite )
        : KListView( parent )
    {
      assert( folderTree );
      int columnIdx = addColumn( i18n( "Folder" ) );
      setRootIsDecorated( true );
      setSorting( -1 );

      SimpleFolderTreeItem<T> * lastItem = 0;
      SimpleFolderTreeItem<T> * lastTopItem = 0;
      SimpleFolderTreeItem<T> * selectedItem = 0;
      int lastDepth = 0;

      for ( QListViewItemIterator it( folderTree ) ; it.current() ; ++it ) {
        KMFolderTreeItem * fti = static_cast<KMFolderTreeItem *>( it.current() );

        if ( !fti || fti->protocol() == KFolderTreeItem::Search )
          continue;

        int depth = fti->depth();// - 1;
        //kdDebug( 5006 ) << "LastDepth=" << lastDepth << "\tdepth=" << depth
        //                << "\tname=" << fti->text( 0 ) << endl;
        SimpleFolderTreeItem<T> * item = 0;
        if ( depth <= 0 ) {
          // top level - first top level item or after last existing top level item
          if ( lastTopItem )
            item = new SimpleFolderTreeItem<T>( this, lastTopItem );
          else
            item = new SimpleFolderTreeItem<T>( this );
          lastTopItem = item;
          depth = 0;
        }
        else {
          if ( depth > lastDepth ) {
            // next lower level - parent node will get opened
            item = new SimpleFolderTreeItem<T>( lastItem );
            lastItem->setOpen( true );
          }
          else {
            if ( depth == lastDepth )
              // same level - behind previous item
              item = new SimpleFolderTreeItem<T>( lastItem->parent(), lastItem );
            else if ( depth < lastDepth ) {
              // above previous level - might be more than one level difference
              // but highest possibility is top level
              while ( ( depth <= --lastDepth ) && lastItem->parent() ) {
                lastItem = static_cast<SimpleFolderTreeItem<T> *>( lastItem->parent() );
              }
              if ( lastItem->parent() )
                item = new SimpleFolderTreeItem<T>( lastItem->parent(), lastItem );
              else {
                // chain somehow broken - what does cause this ???
                kdDebug( 5006 ) << "You shouldn't get here: depth=" << depth
                                << "folder name=" << fti->text( 0 ) << endl;
                item = new SimpleFolderTreeItem<T>( this );
                lastTopItem = item;
              }
            }
          }
        }

        item->setText( columnIdx, fti->text( 0 ) );
        // Make items without folders and top level items unselectable
        // (i.e. root item Local Folders and IMAP accounts)
        if ( !fti->folder() || depth == 0 || ( mustBeReadWrite && fti->folder()->isReadOnly() ) ) {
          item->setSelectable( false );
        } else {
          item->setFolder( fti->folder() );
          if ( preSelection == item->folder()->idString() )
            selectedItem = item;
        }
        lastItem = item;
        lastDepth = depth;
      }

      if ( selectedItem ) {
        setSelected( selectedItem, true );
        ensureItemVisible( selectedItem );
      }
    }

    inline const KMFolder * folder() const
    {
      QListViewItem * item = currentItem();
      if( item ) {
        const KMFolder * folder = static_cast<SimpleFolderTreeItem<T> *>( item )->folder();
        return folder;
      }
      return 0;
    }
};

typedef SimpleFolderTreeBase<KListViewItem> SimpleFolderTree;

}


#endif
