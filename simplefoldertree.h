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

#include "kmfolder.h"
#include "kmfoldertree.h"
#include "treebase.h"

#include <kdebug.h>
#include <klistview.h>
#include <kpopupmenu.h>
#include <kiconloader.h>

class KMFolder;
class KMFolderTree;

namespace KMail {

static int recurseFilter( QListViewItem * item, const QString& filter, int column )
{
    if ( item == 0 )
      return 0;

    QListViewItem * child;
    child = item->firstChild();

    int enabled = 0;
    while ( child ) {
      enabled += recurseFilter( child, filter, column );
      child = child->nextSibling();
    }

    if ( filter.length() == 0 ||
         item->text( column ).find( filter, 0, false ) >= 0 ) {
      item->setVisible( true );
      ++enabled;
    }
    else {
          item->setVisible( !!enabled );
           item->setEnabled( false );
    }

         return enabled;
}

class TreeItemBase 
{
   public :
   TreeItemBase( QListView *parent = 0 ): mFolder( 0 ) {

     kdDebug(5006) << k_funcinfo << endl;
   }
   virtual ~TreeItemBase() { }

   void setFolder( KMFolder * folder ) { mFolder = folder; };
   const KMFolder * folder() { return mFolder; };

    // Set the flag which determines if this is an alternate row
   void setAlternate ( bool alternate ) {
	mAlternate = alternate;
   }

   private:
     KMFolder * mFolder;
     bool mAlternate;
			
};

template <class T> class SimpleFolderTreeItem : public T, public TreeItemBase
{
  public:
    SimpleFolderTreeItem( QListView * listView ) : 
        T( listView ), TreeItemBase( listView) 
    {
     kdDebug(5006) << k_funcinfo << endl;
    }
    SimpleFolderTreeItem( QListView * listView, QListViewItem * afterListViewItem ) :
            T( listView, afterListViewItem ) , TreeItemBase( listView) 
    {
     kdDebug(5006) << k_funcinfo << endl;
    }
    SimpleFolderTreeItem( QListViewItem * listViewItem ) : T( listViewItem ) , TreeItemBase() 
    {
     kdDebug(5006) << k_funcinfo << endl;
    }

    SimpleFolderTreeItem( QListViewItem * listViewItem, QListViewItem * afterListViewItem ) :
            T( listViewItem, afterListViewItem ) , TreeItemBase() 
    {
     kdDebug(5006) << k_funcinfo << endl;
    }

};

template <> class SimpleFolderTreeItem<QCheckListItem> : public QCheckListItem, public TreeItemBase
{
  public:
    SimpleFolderTreeItem( QListView * listView ) :
        QCheckListItem( listView, QString(), CheckBox ), TreeItemBase( listView )  {}
    SimpleFolderTreeItem( QListView * listView, QListViewItem * afterListViewItem ) :
            QCheckListItem( listView, afterListViewItem, QString(), CheckBox ), TreeItemBase( listView )  {}
    SimpleFolderTreeItem( QListViewItem * listViewItem ) :
            QCheckListItem( listViewItem, QString(), CheckBox ) {}
    SimpleFolderTreeItem( QListViewItem * listViewItem, QListViewItem * afterListViewItem ) :
            QCheckListItem( listViewItem, afterListViewItem, QString(), CheckBox ) {}

};


template <class T> class SimpleFolderTreeBase : public TreeBase
{
  
   public:


    inline SimpleFolderTreeBase( QWidget * parent, KMFolderTree *folderTree,
                      const QString &preSelection, bool mustBeReadWrite )
        : TreeBase( parent, folderTree, preSelection, mustBeReadWrite )
    {
      assert( folderTree );
      setFolderColumn( addColumn( i18n( "Folder" ) ) );
      setPathColumn( addColumn( i18n( "Path" ) ) );
      
      setRootIsDecorated( true );
      setSorting( -1 );

      reload( mustBeReadWrite, true, true, preSelection );

    }

    virtual SimpleFolderTreeItem<T>* createItem( QListView * parent )
    {
        return new SimpleFolderTreeItem<T>( parent );
    }

    virtual SimpleFolderTreeItem<T>* createItem( QListView * parent, QListViewItem* afterListViewItem  )
    {
        return new SimpleFolderTreeItem<T>( parent, afterListViewItem );
    }

    virtual SimpleFolderTreeItem<T>* createItem( QListViewItem * parent, QListViewItem* afterListViewItem )
    {
        return new SimpleFolderTreeItem<T>( parent, afterListViewItem );
    }

    virtual SimpleFolderTreeItem<T>* createItem( QListViewItem * parent )
    {
        return new SimpleFolderTreeItem<T>( parent );
    }

    inline void keyPressEvent( QKeyEvent *e )
    {
      kdDebug(5006) << k_funcinfo << endl;
      int ch = e->ascii();

      if ( ch >= 32 && ch <= 126 )
        applyFilter( mFilter + ch );

      else if ( ch == 8 || ch == 127 ) {
        if ( mFilter.length() > 0 ) {
          mFilter.truncate( mFilter.length()-1 );
          applyFilter( mFilter );
        }
      }

     else
      KListView::keyPressEvent( e );
    }

    void applyFilter( const QString& filter )
    {
      kdDebug(5006) << k_funcinfo << filter << endl ;
      // Reset all items to visible, enabled, and open
      QListViewItemIterator clean( this );
      while ( clean.current() ) {
        QListViewItem * item = clean.current();
        item->setEnabled( true );
        item->setVisible( true );
        item->setOpen( true );
        ++clean;
      }

      mFilter = filter;

      if ( filter.isEmpty() ) {
        setColumnText( pathColumn(), i18n("Path") );
        return;
      }

      // Set the visibility and enabled status of each list item.
      // The recursive algorithm is necessary because visiblity
      // changes are automatically applied to child nodes by Qt.
      QListViewItemIterator it( this );
      while ( it.current() ) {
        QListViewItem * item = it.current();
        if ( item->depth() <= 0 )
          recurseFilter( item, filter, pathColumn() );
        ++it;
      }

      // Recolor the rows appropriately
      recolorRows();

      // Iterate through the list to find the first selectable item
      QListViewItemIterator first ( this );
      while ( first.current() ) {
        SimpleFolderTreeItem<T> * item = static_cast< SimpleFolderTreeItem<T> * >( first.current() );

        if ( item->isVisible() && item->isSelectable() ) {
          setSelected( item, true );
          ensureItemVisible( item );
          break;
        }

        ++first;
      }

      // Display and save the current filter
      if ( filter.length() > 0 )
        setColumnText( pathColumn(), i18n("Path") + "  ( " + filter + " )" );
      else
        setColumnText( pathColumn(), i18n("Path") );

      mFilter = filter;
    }

};

typedef SimpleFolderTreeBase<KListViewItem> SimpleFolderTree;

}

#endif
