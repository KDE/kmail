/******************************************************************************
 *
 * Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 * Based on kmfoldertree.cpp which was Copyrighted by the KMail Development
 * Team and didn't report detailed author list.
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

#include "mainfolderview.h"

#include "globalsettings.h"

#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmmainwidget.h"
#include "util.h"

#include <kmenu.h>
#include <kicon.h>
#include <klocale.h>

#include <QMenu>
#include <QAction>
#include <QVariant>

#include <libkdepim/broadcaststatus.h>

namespace KMail
{

MainFolderView::MainFolderView( KMMainWidget *mainWidget, FolderViewManager *manager, QWidget *parent, const char *name )
: FolderView( mainWidget, manager, parent, "MainFolderView", name )
{
  setIconSize( QSize( 16, 16 ) ); // default to smaller icons
}

FolderViewItem * MainFolderView::createItem(
    FolderViewItem *parent,
    const QString &label,
    KMFolder *folder,
    KPIM::FolderTreeWidgetItem::Protocol proto,
    KPIM::FolderTreeWidgetItem::FolderType type
  )
{
  if ( parent )
    return new FolderViewItem( parent, label, folder, proto, type );
  return new FolderViewItem( this, label, folder, proto, type );
}

//=====================================================================================
// Contextual menu related stuff
//

void MainFolderView::fillContextMenuTreeStructureRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection )
{
  KMFolder *folder = item->folder();

  if ( !multiSelection )
  {
    // Treat the special case of the root and account folders 
    if ( ( !folder ) || ( folder->noContent() && ( item->folderType() == FolderViewItem::Root ) ) )
    {
      if ( folder || ( ( item->folderType() == FolderViewItem::Root ) && ( item->protocol() == FolderViewItem::Local ) ) )
      {
        // local root or account folder
        QString createChild = folder ? i18n( "&New Subfolder..." ) : i18n( "&New Folder..." );

        menu->addAction(
            KIcon("folder-new"), createChild,
            this, SLOT( slotAddChildFolder() )
          );
      }
    } else {
      // regular (non root, non account) folders

      if ( !folder->noChildren() )
        menu->addAction(
            KIcon( "folder-new" ), i18n( "&New Subfolder..." ),
            this, SLOT( slotAddChildFolder() )
          );
    }
  }

  if ( folder )
  {
    // These two will work also for multiple folders

    if( item->folderType() != FolderViewItem::Root ) // disable copy/move for root folders since the underlying copy/move code screws up completly on this
    {
      // copy folder
      QMenu *copyMenu = menu->addMenu( KIcon("edit-copy"),
                                       multiSelection ? i18n( "&Copy Folders To" ) :
                                                        i18n( "&Copy Folder To" ) );
      folderToPopupMenu( CopyFolder, this, copyMenu );

      // FIXME: if AT LEAST ONE folder is moveable
      if ( folder && folder->isMoveable() )
      {
        QMenu *moveMenu = menu->addMenu( KIcon( "go-jump" ),
                                         multiSelection ? i18n( "&Move Folders To" ) :
                                                          i18n( "&Move Folder To" ) );
        folderToPopupMenu( MoveFolder, this, moveMenu );
      }
    }

    if ( !multiSelection && !folder->isSystemFolder() )
      menu->addAction( mainWidget()->action( "delete_folder" ) );
  }
}


void MainFolderView::fillContextMenuViewStructureRelatedActions( KMenu *menu, FolderViewItem *item, bool /*multiSelection*/ )
{
  KMFolder * folder = item->folder();
  if ( !folder )
    return;

  if ( folder->noContent() )
    return;

  if ( !GlobalSettings::self()->enableFavoriteFolderView() )
    return;

  // this works for multiple folders too
  menu->addAction(
     KIcon("bookmark-new"), i18n("Add to Favorite Folders"),
     this, SLOT( slotAddToFavorites() )
   );
}

//=====================================================================================
// Folder Tree Structure to Popup Menu
//

void MainFolderView::folderToPopupMenuInternal(
      MenuAction action, QObject * target, QMenu *menu, QTreeWidgetItem * parentItem
    )
{
  int childCount = parentItem->childCount();

  for ( int idx = 0; idx < childCount; idx++ )
  {
    FolderViewItem * fvi = static_cast< FolderViewItem * >( parentItem->child( idx ) );

    if ( !fvi )
      continue;

    // skip search folders
    if ( fvi->protocol() == KPIM::FolderTreeWidgetItem::Search )
      continue;

    QString label = fvi->labelText();
    label.replace( '&',"&&" );

    if ( fvi->childCount() > 0 )
    {
      // new level
      QMenu* popup = new QMenu( menu );

      popup->setObjectName( "subMenu" );
      popup->setTitle( label );

      folderToPopupMenuInternal( action, target, popup, fvi );

      bool subMenu = false;
      if ( ( action == MoveMessage || action == CopyMessage ) &&
           fvi->folder() && !fvi->folder()->noContent() )
        subMenu = true;
      if ( ( action == MoveFolder || action == CopyFolder )
          && ( !fvi->folder() || ( fvi->folder() && !fvi->folder()->noChildren() ) ) )
        subMenu = true;

      QString sourceFolderName;
      FolderViewItem* srcItem = static_cast< FolderViewItem * >( currentItem() );
      if ( srcItem )
        sourceFolderName = srcItem->text( 0 );

      if (
          (action == MoveFolder || action == CopyFolder) &&
          fvi->folder() && fvi->folder()->child() &&
          fvi->folder()->child()->hasNamedFolder( sourceFolderName )
        )
        subMenu = false;

      if ( subMenu )
      {
        QAction* act;
        if ( action == MoveMessage || action == MoveFolder )
          act = popup->addAction( i18n("Move to This Folder") );
        else
          act = popup->addAction( i18n("Copy to This Folder") );
        popup->addSeparator();

        act->setData( QVariant::fromValue<void *>( (void *)( fvi->folder() ) ) );
      }

      menu->addMenu( popup );

    } else {

      // insert an item
      QAction* act = menu->addAction( label );
      if ( fvi->folder() )
        act->setData( QVariant::fromValue<void *>( (void *)( fvi->folder() ) ) );
      bool enabled = (fvi->folder() ? true : false);
      if ( fvi->folder() &&
           ( fvi->folder()->isReadOnly() || fvi->folder()->noContent() ) )
        enabled = false;
      act->setEnabled( enabled );
    }
  }
}

void MainFolderView::folderToPopupMenu( MenuAction action, QObject * target, QMenu *menu )
{
  // The public version of the function
  if ( !menu )
    return; // sanity check

  menu->clear();

  // (re-)connect the signals
  switch ( action )
  {
    case MoveMessage:
      Util::reconnectSignalSlotPair( menu, SIGNAL( triggered( QAction * ) ), target, SLOT( slotMoveSelectedMessagesToFolder( QAction* ) ) );
    break;
    case MoveFolder:
      Util::reconnectSignalSlotPair( menu, SIGNAL( triggered( QAction * ) ), target, SLOT( slotCopySelectedFoldersToFolder( QAction* ) ) );
    break;
    case CopyMessage:
     Util::reconnectSignalSlotPair( menu, SIGNAL( triggered( QAction * ) ), target, SLOT( slotCopySelectedMessagesToFolder( QAction* ) ) );
    break;
    case CopyFolder:
     Util::reconnectSignalSlotPair( menu, SIGNAL( triggered( QAction * ) ), target, SLOT( slotCopySelectedFoldersToFolder( QAction* ) ) );
    break;
  }

  if ( topLevelItemCount() < 1 )
    return; // done

  folderToPopupMenuInternal( action, target, menu, invisibleRootItem() );
}

void MainFolderView::slotMoveSelectedFoldersToFolder( QAction* act )
{
  KMFolder * f = static_cast<KMFolder *>( act->data().value<void *>() );
  if ( !f )
    return;
  moveOrCopyFolders( selectedFolders(), f, true /*move*/ );
}

void MainFolderView::slotCopySelectedFoldersToFolder( QAction* act )
{
  KMFolder * f = static_cast<KMFolder *>( act->data().value<void *>() );
  if ( !f )
    return;
  moveOrCopyFolders( selectedFolders(), f, false /*copy, don't move*/ );
}


//=======================================================================================
// DND Machinery: we allow moving folders around and eventually sorting stuff by dnd
//

bool MainFolderView::acceptDropCopyOrMoveFolders( FolderViewItem *item )
{
  if ( !item )
    return false; // obviously

  if ( item->protocol() == FolderViewItem::Search )
    return false; // nothing can be dragged into search folders

  QList< QPointer< KMFolder > > lFolders = DraggedFolderList::get();
  if ( lFolders.isEmpty() )
    return false; // nothing we can accept

  KMFolder *fld = item->folder();

  if ( fld )
  {
    if ( fld->isReadOnly() || fld->noContent() )
      return false; // content can't be added to folder

    // make sure ALL of the items can be copied or moved to this folder

    // parents can't be moved into children so make sure that fld is not
    // a child of one of the dragged folders
    for ( QList< QPointer< KMFolder > >::Iterator it = lFolders.begin(); it != lFolders.end(); ++it )
    {
      if ( !( *it ) )
        return false; // one of the folders was lost in the way: don't bother with the copy
      if ( ( *it ) == fld )
        return false; // dragging a folder over itself
      if ( ( *it ) == fld->ownerFolder() )
        return false; // dragging a folder over its direct owner (parent)
      if ( ( *it )->hasDescendant( fld ) )
        return false; // dragging a folder over one of its descendants
      if ( ! ( ( *it )->ownerFolder() || ( ( *it )->folderType() == KMFolderTypeMbox ) || ( ( *it )->folderType() == KMFolderTypeMaildir ) ) )
        return false; // dragging a non local root
    }

  } else {
    if (
         ( item->protocol() != FolderViewItem::Local ) ||
         ( item->folderType() != FolderViewItem::Root )
      )
      return false; // non local top-level folder (can be searches, but nothing can be dragged into at the moment)

    // local root for sure: this is a toplevel item, no parents
    // just make sure that no dragged folder is already a direct child of local root
    for ( QList< QPointer< KMFolder > >::Iterator it = lFolders.begin(); it != lFolders.end(); ++it )
    {
      if ( !( *it ) )
        return false; // one of the folders was lost in the way: don't bother with the copy
      if ( ( *it )->parent() == &( kmkernel->folderMgr()->dir() ) )
        return false; // dragged folder is a direct child of local top-level
      if ( ! ( ( *it )->ownerFolder() || ( ( *it )->folderType() == KMFolderTypeMbox ) || ( ( *it )->folderType() == KMFolderTypeMaildir ) ) )
        return false; // dragging a non local root
    }     
  }

  return true; // item would accept a copy/move of all the dragged folders
}


class MainFolderViewFoldersDropAction
{
public:
  enum DropFoldersStatus
  {
    Accept,   ///< Drag is accepted: we can do something with it
    Reject    ///< Drag is rejected: can't do anything with it at the moment
  };

public:
  // out
  QRect validityRect;                                  ///< The rect where the results of this structure remain valid
  DropFoldersStatus status;                            ///< Do we accept or reject ?
  // out if status == Accept
  bool doCopyOrMove;                                   ///< A Copy/Move is required ?
  FolderViewItem *moveTarget;                          ///< Copy/Move where ? (valid only if mustCopyOrMove is true)
  bool doPositionalInsert;
  MainFolderView::DropInsertPosition insertPosition;   ///< Where to insert the data with respect to insertReference ?
  FolderViewItem *reference;                           ///< The reference item that defines our insertions
  QString description;                                 ///< Description of the operation that will be done
};

void MainFolderView::computeFoldersDropAction( QDropEvent *e, MainFolderViewFoldersDropAction *act )
{
  // Ok, this is quite complex.
  // 
  // It attempts to determine what to do with the currently dragged folders
  // if they were dropped on the item in the current mouse position.
  //
  // The rules:
  //
  // - If we're in the upper part of an item and dnd sorting is enabled
  //   then we might be wanting to just place the dragged stuff above the item.
  //
  // - If we're in the lower part of an item and dnd sorting is enabled
  //   then we might be wanting to just place the dragged stuff below the item.
  //
  // - If we're in the middle of an item then we might be wanting to
  //   move/copy the dragged stuff INSIDE the item.
  //
  // - If we're in the upper or lower part of an item but the dragged stuff
  //   has a parent that is not the items's parent then the sorting
  //   operation might ALSO require a copy or move (to the parent).
  //
  // - If dnd sorting is not enabled then we always attempt to copy or move stuff INSIDE o->item.
  //
  // - To complicate the things a bit more, copying/moving might be not possible
  //   for several reasons...
  //
  // This function attempts to deal with all these rules and by the meantime
  // give to the user some visual feedback about what is going on.
  //

  // first of all, we allow only folder drags coming from THIS view
  if ( e->source() != this )
  {
    act->validityRect = viewport()->rect();
    act->status = MainFolderViewFoldersDropAction::Reject;
    return;
  }

  act->reference = static_cast<FolderViewItem *>( itemAt( e->pos() ) );
  int draggedFolderCount = DraggedFolderList::get().count();

  bool bWasOutsideItem = false;

  if ( !act->reference )
  {
    // not over an item: try to use the last item in the view as reference
    bWasOutsideItem = true;

    int cc = topLevelItemCount();
    if ( cc < 1 )
    {
      // nothing in the view at all ? ... this is senseless
      act->validityRect = viewport()->rect();
      act->status = MainFolderViewFoldersDropAction::Reject;
      return;
    }

    act->reference = static_cast<FolderViewItem *>( topLevelItem( cc - 1 ) );
    // now item != 0 (and item is visible)
  }

  QRect r = visualItemRect( act->reference );
  QRect mouseRect( e->pos().x() - 1, e->pos().y() - 1, 2, 2 );

  // set defaults
  act->status = MainFolderViewFoldersDropAction::Reject;
  act->validityRect = mouseRect;

  bool bCopyOrMoveToCurrentIsPossible = \
         !bWasOutsideItem && /* a copy/move operation would look bad in this case */
         ( act->reference->flags() & Qt::ItemIsDropEnabled ) && /* item has drops disabled */
         acceptDropCopyOrMoveFolders( act->reference ); /* item can't accept the drop for some other reason */

  if ( sortingPolicy() != SortByDragAndDropKey )
  {
    // when sorting by dnd is disabled then we always act on the current item
    act->validityRect = r;

    if ( bCopyOrMoveToCurrentIsPossible )
    {
      // sorting by dnd is disabled but we can copy/move messages to the reference folder.
      act->moveTarget = act->reference;
      act->status = MainFolderViewFoldersDropAction::Accept;
      act->doCopyOrMove = true;
      act->doPositionalInsert = false;
      act->description = i18np( "Move or copy folder to %2",
                                "Move or copy %1 folders to %2", draggedFolderCount,
                                act->moveTarget->labelText() );
    } // else if we cannot move folders to the current item and sorting by dnd is disabled
      // hence there is nothing more we can do here: ignore.
    return;
  }

  // sortingPolicy() == SortByDragAndDropKey

  // now try to actually compute the drop position

  act->doPositionalInsert = false;

  if ( e->pos().y() < ( r.top() + ( r.height() / 4 ) ) )
  {
    // would drop above item
    act->validityRect = QRect( r.left(), r.top(), r.width(), r.height() / 4 );
    act->doPositionalInsert = true;
    act->insertPosition = AboveReference;
  } else if ( e->pos().y() > ( r.bottom() - ( r.height() / 4 ) ) )
  {
    act->validityRect = QRect( r.left(), r.bottom() - ( r.height() / 4 ), r.width(), r.height() / 4 );
    act->doPositionalInsert = true;
    act->insertPosition = BelowReference;
  }

  if ( !act->doPositionalInsert )
  {
    // we're actually exactly over the item
    act->validityRect = QRect( r.left(), r.top() + ( r.height() / 4 ), r.width(), r.height() / 2 );

    if ( bCopyOrMoveToCurrentIsPossible )
    {
      // we can move the folders.
      act->moveTarget = act->reference;
      act->status = MainFolderViewFoldersDropAction::Accept;
      act->doCopyOrMove = true;
      act->description = i18np( "Move or copy folder to %2",
                                "Move or copy %1 folders to %2",
                                draggedFolderCount, act->moveTarget->labelText() );
    } // else if we cannot move folders to the current item thus ignore

    return;
  }

  // Ok, now we're trying to do positional insert for sure.

  // If we're moving items from the act->reference's parent then
  // the thingie is easy: no copy or move is needed.

  QTreeWidgetItem *commonParent = static_cast< QTreeWidgetItem * >( act->reference )->parent();
  // commonParent might be null here!

  // please note that the drag comes from our view so we're actually
  // dragging the selected items!
  bool bMovingFromCommonParent = true;

  QTreeWidgetItemIterator it( this );
  while ( *it )
  {
    if ( ( *it )->isSelected() )
    {
      // this item is being dragged
      if ( ( *it )->parent() != commonParent )
      {
        bMovingFromCommonParent = false;
        break;
      }
    }
    ++it;
  }

  if ( bMovingFromCommonParent )
  {
    // this is just a sort inside a single hierarchy level
    act->status = MainFolderViewFoldersDropAction::Accept;
    act->doCopyOrMove = false;
    if ( act->insertPosition == AboveReference )
      act->description = i18np( "Order folder above %2",
                                "Order %1 folders above %2",
                                draggedFolderCount, act->reference->labelText() );
    else
      act->description = i18np( "Order folder below %2",
                                "Order %1 folders below %2",
                                draggedFolderCount, act->reference->labelText() );

    viewport()->update();
    return;
  }

  // not dragging from common parent: both a "copy or move" operation and a positional insert
  // is needed...

  // check if commonParent can accept a "copy or move" operation.
  act->moveTarget = static_cast< FolderViewItem * >( commonParent );

  // if moving to common parent is not possible then sorting is also not possible
  if ( !act->moveTarget )
    return;
  if ( !( act->moveTarget->flags() & Qt::ItemIsDropEnabled ) )
    return;
  if ( !acceptDropCopyOrMoveFolders( act->moveTarget ) )
    return;

  // moving to a common parent (act->moveTarget) is possible

  act->status = MainFolderViewFoldersDropAction::Accept;
  act->doCopyOrMove = true;
  if ( act->insertPosition == AboveReference )
    act->description = i18np( "Move or copy folder to %2, order above %3",
                              "Move or copy %1 folders to %2, order above %3",
                              draggedFolderCount,
                              act->moveTarget->labelText(), act->reference->labelText() );
  else
    act->description = i18np( "Move or copy folder to %2, order below %3",
                              "Move or copy %1 folders to %2, order below %3",
                              draggedFolderCount,
                              act->moveTarget->labelText(), act->reference->labelText() );
}



void MainFolderView::handleFoldersDragMoveEvent( QDragMoveEvent *e )
{
  MainFolderViewFoldersDropAction act;

  computeFoldersDropAction( e, &act );
  if ( act.status == MainFolderViewFoldersDropAction::Accept )
  {
    e->accept( act.validityRect );
    setDropIndicatorData(
        act.doCopyOrMove ? act.moveTarget : 0,
        act.doPositionalInsert ? act.reference : 0, 
        act.insertPosition
     );
  } else {
    e->ignore( act.validityRect );
    setDropIndicatorData( 0, 0 );
  }

  KPIM::BroadcastStatus::instance()->setStatusMsg( act.description );
}

void MainFolderView::handleFoldersDropEvent( QDropEvent *e )
{
  MainFolderViewFoldersDropAction act;

  computeFoldersDropAction( e, &act );
  if ( act.status != MainFolderViewFoldersDropAction::Accept )
  {
    e->ignore();
    return;
  }

  e->accept();

  if ( !act.doCopyOrMove )
  {
    // assert( act.doPositionalInsert );

    // only a move (within the same parent)

    QTreeWidgetItem *commonParent = static_cast< QTreeWidgetItem * >( act.reference )->parent();

    int refIdx = commonParent ? commonParent->indexOfChild( act.reference ) : indexOfTopLevelItem( act.reference );
    if ( act.insertPosition == AboveReference )
      refIdx--;

    // we're dragging items from THIS view
    setUpdatesEnabled( false );

    QList< QTreeWidgetItem * > lSelected = selectedItems();


    for ( QList< QTreeWidgetItem * >::Iterator it = lSelected.begin(); it != lSelected.end(); ++it )
    {
      if ( !( *it ) )
        continue; // umphf
 
      FolderViewItem *moved = static_cast< FolderViewItem * >( *it );

      int removedIdx = commonParent ? commonParent->indexOfChild( moved ) : indexOfTopLevelItem( moved );
      if ( commonParent )
        commonParent->takeChild( removedIdx );
      else
        takeTopLevelItem( removedIdx );

      if ( removedIdx > refIdx )
        refIdx++;

      if ( commonParent )
        commonParent->insertChild( refIdx, moved );
      else
        insertTopLevelItem( refIdx, moved );
    }

    setUpdatesEnabled( true );

    fixSortingKeysForChildren( commonParent ? commonParent : invisibleRootItem() );
    sortByColumn( LabelColumn, Qt::AscendingOrder );

    viewport()->update();

    return;
  }

  // must do a copy or move
  // folders must be first copied/moved and then sorted

  // Get the dragged folder list
  QList<QPointer<KMFolder> > lFolders = DraggedFolderList::get();
  if ( lFolders.isEmpty() )
    return; // nothing we can accept

  bool bCanMove = true;

  // Check that each pointer is not null and compute possible dnd modes
  for ( QList<QPointer<KMFolder> >::Iterator it = lFolders.begin(); it != lFolders.end(); ++it )
  {
    if ( !( *it ) )
      return; // yes, that's pessimistic, but if we lost a folder then something weird happened anyway: stay safe
    if ( !( ( *it )->isMoveable() ) )
      bCanMove = false; // at least one folder is not moveable, allow copy only
  }

  // ask for dnd mode now
  int action = dragMode( true, bCanMove );
  switch ( action )
  {
    case DragCopy:
      e->setDropAction( Qt::CopyAction );
    break;
    case DragMove:
      e->setDropAction( Qt::MoveAction );
    break;
    default:
      return; // cancelled
    break;
  }

  if ( act.doPositionalInsert )
  {
    // will need positional insertion AFTER the items are copied
    // FIXME: this is missing... let's see if somebody complains before I implement it :P
  }

  moveOrCopyFolders( lFolders, act.moveTarget->folder(), (action == DragMove) );
}



}

#include "mainfolderview.moc"
