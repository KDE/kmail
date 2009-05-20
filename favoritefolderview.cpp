/******************************************************************************
 *
 *  Copyright (c) 2007 Volker Krause <vkrause@kde.org>
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "favoritefolderview.h"

#include "folderstorage.h"
#include "folderselectiondialog.h"
#include "globalsettings.h"
#include "kmacctcachedimap.h"
#include "kmfolder.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmmainwidget.h"
#include "korghelper.h"
#include "mainfolderview.h"

#include <kicon.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmenu.h>

#include <libkdepim/broadcaststatus.h>
#include <libkdepim/kaddrbookexternal.h>

#include <QDBusConnection>
#include <QDBusInterface>


namespace KMail
{

FavoriteFolderView::FavoriteFolderView( KMMainWidget *mainWidget, FolderViewManager *manager, QWidget *parent, const char *name )
: FolderView( mainWidget, manager, parent, "FavoriteFolderView", name )
{
  setRootIsDecorated( false ); // we have only toplevel items
  setSortingPolicy( SortByDragAndDropKey );
  setColumnText( LabelColumn, i18n( "Favorite Folders" ) );
}

FolderViewItem * FavoriteFolderView::createItem(
      FolderViewItem *,
      const QString &label,
      KMFolder *folder,
      KPIM::FolderTreeWidgetItem::Protocol proto,
      KPIM::FolderTreeWidgetItem::FolderType type
    )
{
  if ( !folder )
    return 0;

  int idx = GlobalSettings::self()->favoriteFolderIds().indexOf( folder->id() );
  if ( idx < 0 )
  {
    // Not in favorites. If the folder is an inbox which we haven't seen before
    // (i.e. a new account), then add it, else return here already.

    // The old FavoriteFolderView code contains a snippet that automatically
    // adds the inboxes of all the accounts to the view.
    // While this is a quiestionable rule, Thomas says that it might be for some
    // usability reason so I'm adding it here too....
    if ( type != FolderViewItem::Inbox )
      return 0; // not an inbox
    if ( GlobalSettings::self()->favoriteFolderViewSeenInboxes().contains( folder->id() ) )
      return 0; // already seen

    QList<int> seenInboxes = GlobalSettings::self()->favoriteFolderViewSeenInboxes();
    seenInboxes.append( folder->id() ); // FIXME: this list never shrinks
    GlobalSettings::self()->setFavoriteFolderViewSeenInboxes( seenInboxes );
  }

  // If we reached this point, we want to add the folder to the favorite folder
  // view

  QString name;
  bool newFavorite = !( idx >= 0 && idx < GlobalSettings::self()->favoriteFolderNames().count() );
  if ( !newFavorite )
    name = GlobalSettings::self()->favoriteFolderNames().at( idx );

  if ( name.isEmpty() )
  {
    name = label;

    // locate the item's parent
    if ( proto == FolderViewItem::Local )
      name += QString(" (%1)").arg( i18n("Local Folders") );
    else {
      KMFolder *owner = folder;
      while( owner->ownerFolder() )
        owner = owner->ownerFolder();

      name += QString(" (%1)").arg( owner->label() );
    }

    // Ok, we created a new folder here, so make sure it is also in the list of
    // favorite folder ids and names. Otherwise, the item would not be added on
    // the next reload()
    QList<int> idList = GlobalSettings::self()->favoriteFolderIds();
    QList<QString> nameList = GlobalSettings::self()->favoriteFolderNames();
    idList.append( folder->id() );
    nameList.append( name );
    GlobalSettings::self()->setFavoriteFolderIds( idList );
    GlobalSettings::self()->setFavoriteFolderNames( nameList );
  }

  FolderViewItem *item = new FolderViewItem( this, name, folder, proto, type );
  item->setAlwaysDisplayCounts( true );
  return item;
}

void FavoriteFolderView::storeFavorites()
{
  QList<int> lIds;
  QStringList lNames;

  QTreeWidgetItemIterator it( this );
  while( FolderViewItem * item = static_cast<FolderViewItem *>( *it ) )
  {
    lIds.append( item->folder()->id() );  
    lNames.append( item->labelText() );
    ++it;
  }

  GlobalSettings::self()->setFavoriteFolderIds( lIds );
  GlobalSettings::self()->setFavoriteFolderNames( lNames );
}

void FavoriteFolderView::appendAddFolderActionToMenu( KMenu *menu ) const
{
  menu->addAction( KIcon( "bookmark-new" ), i18n( "Add Favorite Folder..." ),
                   this, SLOT( addFolder() ) );
}

void FavoriteFolderView::fillContextMenuViewStructureRelatedActions( KMenu *menu, FolderViewItem */*item*/, bool multiSelection )
{
  menu->addAction( KIcon( "edit-delete" ), i18n( "Remove From Favorites" ), this, SLOT( removeFolders() ) );
  if ( !multiSelection )
    menu->addAction( KIcon( "edit-rename" ), i18n( "Rename Favorite..." ), this, SLOT( renameFolder() ) );
  appendAddFolderActionToMenu( menu );
}

void FavoriteFolderView::fillContextMenuNoItem( KMenu *mneu )
{
  appendAddFolderActionToMenu( mneu );
}

//=======================================================================================
// DND Machinery: we allow adding items from outside and maybe sorting stuff by dnd.
//

class FoldersDropAction
{
public:
  enum Action
  {
    Accept,
    Reject
  };

public:
  // in
  QDropEvent *event;
  FavoriteFolderView *view;
  // out 
  Action action;
  FolderViewItem *reference;
  FolderView::DropInsertPosition position;
  QRect validityRect;
  QString description;
};

static void computeFoldersDropAction( FoldersDropAction *act )
{
  act->reference = static_cast<FolderViewItem *>( act->view->itemAt( act->event->pos() ) );

  if ( !act->reference )
  {
    // not over an item: try to use the last item in the view as reference
    int cc = act->view->topLevelItemCount();
    if ( cc < 1 )
    {
      // nothing in the view at all: totally new items, accept on the whole viewport
      act->action = FoldersDropAction::Accept;
      act->validityRect = act->view->viewport()->rect();
      act->description = i18n("Add Folders to Favorites");
      return;
    }

    act->reference = static_cast<FolderViewItem *>( act->view->topLevelItem( cc - 1 ) );
    // now item != 0 (and item is visible)
  }

  QRect r = act->view->visualItemRect( act->reference );
  QRect mouseRect( act->event->pos().x() - 1, act->event->pos().y() - 1, 2, 2 );

  // set defaults
  act->action = FoldersDropAction::Reject;
  act->validityRect = mouseRect;

  // make sure we're not dragging stuff over itself

  QList<QPointer<KMFolder> > lFolders = DraggedFolderList::get();
  if ( lFolders.isEmpty() )
    return; // nothing we can accept

  for ( QList< QPointer< KMFolder > >::Iterator it = lFolders.begin(); it != lFolders.end(); ++it )
  {
    if ( !( *it ) )
      return; // one of the folders was lost in the way: don't bother
    if ( ( *it ) == act->reference->folder() )
      return; // inserting above or below itself
  }

  act->action = FoldersDropAction::Accept;

  if ( act->event->pos().y() < ( r.top() + ( r.height() / 2 ) ) )
  {
    act->position = FolderView::AboveReference;
    act->validityRect = r;
    act->validityRect.setHeight( r.height() / 2 );
    act->description = i18n( "Insert Folders Above %1", act->reference->labelText() );
    return;
  }

  r.setTop( r.top() + ( r.height() / 2 ) );  
  r.setHeight( r.height() / 2 );
  act->validityRect = r.united( mouseRect );
  act->position = FolderView::BelowReference;  
  act->description = i18n( "Insert Folders Below %1", act->reference->labelText() );
}

void FavoriteFolderView::handleFoldersDragMoveEvent( QDragMoveEvent *e )
{
  FoldersDropAction act;
  act.event = e;
  act.view = this;

  computeFoldersDropAction( &act );
  if ( act.action == FoldersDropAction::Accept )
  {
    e->accept( act.validityRect );
    setDropIndicatorData( 0, act.reference, act.position );
  } else {
    e->ignore( act.validityRect );
    setDropIndicatorData( 0, 0 );
  }

  KPIM::BroadcastStatus::instance()->setStatusMsg( act.description );
}

void FavoriteFolderView::handleFoldersDropEvent( QDropEvent *e )
{
  FoldersDropAction act;
  act.event = e;
  act.view = this;

  computeFoldersDropAction( &act );

  if ( act.action != FoldersDropAction::Accept )
  {
    e->ignore();
    return;
  }

  e->accept();

  int refIdx;
  if ( act.reference )
  {
    refIdx = indexOfTopLevelItem( act.reference );
    if ( act.position == FolderView::AboveReference )
      refIdx--;
  } else {
    refIdx = -1;
  }

  QList<QPointer<KMFolder> > lFolders = DraggedFolderList::get();
  if ( lFolders.isEmpty() )
    return; // nothing we can accept

  setUpdatesEnabled( false );

  for ( QList<QPointer<KMFolder> >::Iterator it = lFolders.begin(); it != lFolders.end(); ++it )
  {
    if ( !( *it ) )
      continue; // umphf

    FolderViewItem *moved = findItemByFolder( *it );
    if ( !moved )
      moved = addFolderInternal( *it );
    if ( !moved )
      continue; // umphf x 2
    int removedIdx = indexOfTopLevelItem( moved );
    takeTopLevelItem( removedIdx );
    if ( removedIdx >= refIdx )
      refIdx++;
    insertTopLevelItem( refIdx, moved );
  }

  setUpdatesEnabled( true );

  if ( sortingPolicy() == SortByDragAndDropKey )
  {
    fixSortingKeysForChildren( invisibleRootItem() );
    sortByColumn( LabelColumn, Qt::AscendingOrder );
  }
}

void FavoriteFolderView::addFolder( KMFolder *fld )
{
  if ( findItemByFolder( fld ) )
    return; // already there

  if ( !addFolderInternal( fld ) )
    return;

  if ( sortingPolicy() == SortByDragAndDropKey )
    fixSortingKeysForChildren( invisibleRootItem() );
}

FolderViewItem * FavoriteFolderView::addFolderInternal( KMFolder *fld )
{
  QList<int> lIds = GlobalSettings::self()->favoriteFolderIds();
  if ( lIds.contains( fld->id() ) )
    return 0; // ugh

  // add it to the list so createItem won't filter it out
  lIds.append( fld->id() );
  GlobalSettings::self()->setFavoriteFolderIds( lIds );

  QStringList lNames = GlobalSettings::self()->favoriteFolderNames();
  lNames.append(QString());
  GlobalSettings::self()->setFavoriteFolderNames( lNames );

  FolderViewItem * it = createItem(
      0, ( fld )->label(), ( fld ),
      FolderViewItem::protocolByFolder( fld ),
      FolderViewItem::folderTypeByFolder( fld )
    );

  // re-store favorites with the right item name
  storeFavorites();

  return it;
}

void FavoriteFolderView::addFolder()
{
  FolderSelectionDialog dlg( mainWidget(), i18n("Add Favorite Folder"), false );
  if ( dlg.exec() != QDialog::Accepted )
    return;
  KMFolder *folder = dlg.folder();
  if ( !folder )
    return;
  addFolderInternal( folder );
}

void FavoriteFolderView::renameFolder()
{
  FolderViewItem * it = static_cast<FolderViewItem *>( currentItem() );
  if ( !it )
    return;

  // We would REALLY like to use the nice Qt item renaming method but we can't
  // since KMMainWidget assigns the return key to a QAction default shortcut.
  // We never get the return key and thus never can end editing successfully.
  // An action-disabling workaround requires too much code to be implemented.

  bool ok;
  QString name = KInputDialog::getText( i18n( "Rename Favorite" ),
                                        i18nc( "@label:textbox New name of the folder.", "Name:" ),
                                        it->labelText(), &ok, this );
  if ( !ok )
    return;

  it->setLabelText( name );

  storeFavorites();
}

void FavoriteFolderView::removeFolders()
{
  QList<QTreeWidgetItem *> lSelected = selectedItems();
  if ( lSelected.isEmpty() )
    return;

  for( QList<QTreeWidgetItem *>::Iterator it = lSelected.begin(); it != lSelected.end(); ++it )
  {
    FolderViewItem * item = static_cast<FolderViewItem *>( *it );
    if ( !item )
      continue; // hum
    if ( !item->folder() )
      continue;

    delete item;
  }

  storeFavorites();
}

void FavoriteFolderView::checkMail()
{
  bool found = false;

  QTreeWidgetItemIterator it( this );

  while( *it )
  {
    FolderViewItem *fti = static_cast<FolderViewItem*>( *it );

    if (
        fti->folder()->folderType() == KMFolderTypeImap ||
        fti->folder()->folderType() == KMFolderTypeCachedImap
      )
    {
      if ( !found )
        if ( !kmkernel->askToGoOnline() )
          break;

      found = true;

      if ( fti->folder()->folderType() == KMFolderTypeImap )
      {
        KMFolderImap *imap = static_cast<KMFolderImap*>( fti->folder()->storage() );
        imap->getAndCheckFolder();
      } else if ( fti->folder()->folderType() == KMFolderTypeCachedImap )
      {
        KMFolderCachedImap* f = static_cast<KMFolderCachedImap*>( fti->folder()->storage() );
        f->account()->processNewMailInFolder( fti->folder() );
      }
    }

    ++it;
  }

}

static void selectKontactPlugin( const QString &plugin )
{
  QDBusInterface *kontact = new QDBusInterface( "org.kde.kontact",
      "/KontactInterface", "org.kde.kontact.KontactInterface",
       QDBusConnection::sessionBus() );
  if ( kontact->isValid() )
    kontact->call( "selectPlugin", plugin );
  delete kontact;
}

void FavoriteFolderView::activateItemInternal( FolderViewItem *fvi, bool keepSelection, bool notifyManager, bool middleButton )
{
  FolderView::activateItemInternal( fvi, keepSelection, notifyManager, middleButton );

  // handle groupware folders
  if ( !fvi || !fvi->folder() || !fvi->folder()->storage() )
    return;
  switch ( fvi->folder()->storage()->contentsType() )
  {
    case KMail::ContentsTypeContact:
        KPIM::KAddrBookExternal::openAddressBook( this );
      break;
    case KMail::ContentsTypeNote:
        selectKontactPlugin( "kontact_knotesplugin" );
      break;
    case KMail::ContentsTypeCalendar:
    case KMail::ContentsTypeTask:
    case KMail::ContentsTypeJournal:
    {
      KMail::KorgHelper::ensureRunning();
      QString plugin;
      switch ( fvi->folder()->storage()->contentsType() )
      {
        case KMail::ContentsTypeCalendar:
          plugin = QLatin1String( "kontact_korganizerplugin" ); break;
        case KMail::ContentsTypeTask:
          plugin = QLatin1String( "kontact_todoplugin" ); break;
        case KMail::ContentsTypeJournal:
          plugin = QLatin1String( "kontact_journalplugin" ); break;
        default: assert( false );
      }
      selectKontactPlugin( plugin );
    }
    break;
    default: // make gcc happy
    break;
  }
}

} // namespace KMail

#include "favoritefolderview.moc"
