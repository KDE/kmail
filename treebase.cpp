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

#include "treebase.h"
#include "kmfolder.h"
#include "kmfoldertree.h"
#include "simplefoldertree.h"

#include <kdebug.h>
#include <klistview.h>

using namespace KMail;

TreeBase::TreeBase( QWidget *parent, KMFolderTree *folderTree,
        const QString &preSelection, bool mustBeReadWrite )
       : KListView( parent ), mFolderTree( folderTree )
{
     kdDebug(5006) << k_funcinfo << endl;

     connect(this, SIGNAL(collapsed(QListViewItem*)), SLOT(recolorRows()));
     connect(this, SIGNAL(expanded(QListViewItem*)),  SLOT(recolorRows()));
     connect( this, SIGNAL( contextMenuRequested( QListViewItem*, const QPoint &, int ) ),
           this, SLOT( slotContextMenuRequested( QListViewItem*, const QPoint & ) ) );

}

const KMFolder * TreeBase::folder() const
{
    QListViewItem * item = currentItem();
    if( item ) {
      TreeItemBase *base = dynamic_cast<TreeItemBase*>( item );
      assert(base);
      const KMFolder * folder = base->folder();
      return folder;
    }
    return 0;
}

void TreeBase::setFolder( KMFolder *folder )
 {
      for ( QListViewItemIterator it( this ) ; it.current() ; ++it )
      {
        const KMFolder *fld = dynamic_cast<TreeItemBase*>( it.current() )->folder();
        if ( fld == folder )
        {
           setSelected( it.current(), true );
           ensureItemVisible( it.current() );
        }
      }
}

void TreeBase::addChildFolder()
{
     kdDebug(5006) << k_funcinfo << endl;

      const KMFolder *fld = folder();
      if ( fld ) {
         mFolderTree->addChildFolder( (KMFolder *) fld, parentWidget() );
	reload( mLastMustBeReadWrite, mLastShowOutbox, mLastShowImapFolders );
	setFolder( (KMFolder *) fld );
      }
}

void TreeBase::slotContextMenuRequested( QListViewItem *lvi,  const QPoint &p )
{
     kdDebug(5006) << k_funcinfo << endl;

      if (!lvi)
        return;
        setCurrentItem( lvi );
        setSelected( lvi, TRUE );

      const KMFolder * folder = dynamic_cast<TreeItemBase*>( lvi )->folder();
      if ( !folder || folder->noContent() || folder->noChildren() )
        return;

      KPopupMenu *folderMenu = new KPopupMenu;
      folderMenu->insertTitle( folder->label() );
      folderMenu->insertSeparator();
      folderMenu->insertItem(SmallIconSet("folder_new"),
                          i18n("&New Subfolder..."), this,
                          SLOT(addChildFolder()));
      kmkernel->setContextMenuShown( true );
      folderMenu->exec (p, 0);
      kmkernel->setContextMenuShown( false );
      delete folderMenu;

}

void TreeBase::recolorRows()
{
     kdDebug(5006) << k_funcinfo << endl;

       // Iterate through the list to set the alternate row flags.
       int alt = 0;
       QListViewItemIterator it ( this );
       while ( it.current() ) {
        QListViewItem * item = it.current() ;
        if ( item->isVisible() ) {
           bool visible = true;
           QListViewItem * parent = item->parent();
           while ( parent ) {
           if (!parent->isOpen()) {
             visible = false;
             break;
           }
           parent = parent->parent();
         }

         if ( visible ) {
          TreeItemBase * treeItemBase = static_cast<TreeItemBase*>( item );
          treeItemBase->setAlternate( alt );
          alt = !alt;
         }
       }
       ++it;
      }
}

void TreeBase::reload( bool mustBeReadWrite, bool showOutbox, bool showImapFolders,
                   const QString& preSelection )
{
      clear();

      mLastMustBeReadWrite = mustBeReadWrite;
      mLastShowOutbox = showOutbox;
      mLastShowImapFolders = showImapFolders;

      QListViewItem * lastItem = 0;
      QListViewItem * lastTopItem = 0;
      QListViewItem * selectedItem = 0;
      int lastDepth = 0;

      mFilter = "";
      QString path;

      for ( QListViewItemIterator it( mFolderTree ) ; it.current() ; ++it ) {
        KMFolderTreeItem * fti = dynamic_cast<KMFolderTreeItem *>( it.current() );

        if ( !fti || fti->protocol() == KFolderTreeItem::Search )
          continue;

        int depth = fti->depth();// - 1;
        //kdDebug( 5006 ) << "LastDepth=" << lastDepth << "\tdepth=" << depth
        //                << "\tname=" << fti->text( 0 ) << endl;
        QListViewItem * item = 0;
        if ( depth <= 0 ) {
          // top level - first top level item or after last existing top level item
          if ( lastTopItem )
            item = createItem( this, lastTopItem );
          else
            item = createItem( this );
          lastTopItem = item;
          depth = 0;
          path  = "";
        }
        else {
          if ( depth > lastDepth ) {
            // next lower level - parent node will get opened
            item = createItem( lastItem );
            lastItem->setOpen( true );
          }
          else {

            path = path.section( '/', 0, -2 - (lastDepth-depth) );
            if ( depth == lastDepth )
              // same level - behind previous item
              item = createItem( lastItem->parent(), lastItem );
            else if ( depth < lastDepth ) {
              // above previous level - might be more than one level difference
              // but highest possibility is top level
              while ( ( depth <= --lastDepth ) && lastItem->parent() ) {
                lastItem = static_cast<QListViewItem *>( lastItem->parent() );
              }
              if ( lastItem->parent() )
                item = createItem( lastItem->parent(), lastItem );
              else {
                // chain somehow broken - what does cause this ???
                kdDebug( 5006 ) << "You shouldn't get here: depth=" << depth
                                << "folder name=" << fti->text( 0 ) << endl;
                item = createItem( this );
                lastTopItem = item;
              }
            }
          }
        }

        if ( depth > 0 )
          path += "/";
        path += fti->text( 0 );


        item->setText( mFolderColumn, fti->text( 0 ) );
        item->setText( mPathColumn, path );
        // Make items without folders and top level items unselectable
        // (i.e. root item Local Folders and IMAP accounts)
        if ( !fti->folder() || depth == 0 || ( mustBeReadWrite && fti->folder()->isReadOnly() ) ) {
          item->setSelectable( false );
        } else {
          TreeItemBase * treeItemBase = dynamic_cast<TreeItemBase*>( item );
          assert(treeItemBase);
          treeItemBase->setFolder( fti->folder() );
          if ( preSelection == treeItemBase->folder()->idString() )
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

#include "treebase.moc"
