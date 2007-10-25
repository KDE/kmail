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

#include "favoritefolderview.h"

#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmfolderseldlg.h"
#include "kmmainwidget.h"
#include "kmailicalifaceimpl.h"
#include "folderstorage.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmacctcachedimap.h"
#include "folderviewtooltip.h"

#include <libkdepim/maillistdrag.h>

#include <kdebug.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kio/global.h>

#include <qheader.h>
#include <qtimer.h>

#include <cassert>

using namespace KMail;

FavoriteFolderViewItem::FavoriteFolderViewItem(FavoriteFolderView * parent, const QString & name, KMFolder * folder)
  : KMFolderTreeItem( parent, name, folder ),
  mOldName( folder->label() )
{
  // same stuff as in KMFolderTreeItem again, this time even with virtual methods working
  init();
  connect( folder, SIGNAL(nameChanged()), SLOT(nameChanged()) );
  connect( folder, SIGNAL(iconsChanged()), SLOT(slotIconsChanged()) );

  connect( folder, SIGNAL(msgAdded(KMFolder*,Q_UINT32)), SLOT(updateCount()) );
  connect( folder, SIGNAL(numUnreadMsgsChanged(KMFolder*)), SLOT(updateCount()) );
  connect( folder, SIGNAL(msgRemoved(KMFolder*)), SLOT(updateCount()) );
  connect( folder, SIGNAL(folderSizeChanged( KMFolder* )), SLOT(updateCount()) );
  
  QTimer::singleShot( 0, this, SLOT(updateCount()) );

  if ( unreadCount() > 0 )
    setPixmap( 0, unreadIcon( iconSize() ) );
  else
    setPixmap( 0, normalIcon( iconSize() ) );
}

void FavoriteFolderViewItem::nameChanged()
{
  QString txt = text( 0 );
  txt.replace( mOldName, folder()->label() );
  setText( 0, txt );
  mOldName = folder()->label();
}

QValueList<FavoriteFolderView*> FavoriteFolderView::mInstances;

FavoriteFolderView::FavoriteFolderView( KMMainWidget *mainWidget, QWidget * parent) :
    FolderTreeBase( mainWidget, parent ),
    mContextMenuItem( 0 ),
    mReadingConfig( false )
{
  assert( mainWidget );
  addColumn( i18n("Favorite Folders") );
  setResizeMode( LastColumn );
  header()->setClickEnabled( false );
  setDragEnabled( true );
  setAcceptDrops( true );
  setRootIsDecorated( false );
  setSelectionModeExt( KListView::Single );
  setSorting( -1 );
  setShowSortIndicator( false );

  connect( this, SIGNAL(selectionChanged()), SLOT(selectionChanged()) );
  connect( this, SIGNAL(clicked(QListViewItem*)), SLOT(itemClicked(QListViewItem*)) );
  connect( this, SIGNAL(dropped(QDropEvent*,QListViewItem*)), SLOT(dropped(QDropEvent*,QListViewItem*)) );
  connect( this, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint &, int)),
           SLOT(contextMenu(QListViewItem*,const QPoint&)) );
  connect( this, SIGNAL(moved()), SLOT(notifyInstancesOnChange()) );
  connect( this, SIGNAL(triggerRefresh()), SLOT(refresh()) );

  connect( kmkernel->folderMgr(), SIGNAL(changed()), SLOT(initializeFavorites()) );
  connect( kmkernel->dimapFolderMgr(), SIGNAL(changed()), SLOT(initializeFavorites()) );
  connect( kmkernel->imapFolderMgr(), SIGNAL(changed()), SLOT(initializeFavorites()) );
  connect( kmkernel->searchFolderMgr(), SIGNAL(changed()), SLOT(initializeFavorites()) );

  connect( kmkernel->folderMgr(), SIGNAL(folderRemoved(KMFolder*)), SLOT(folderRemoved(KMFolder*)) );
  connect( kmkernel->dimapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)), SLOT(folderRemoved(KMFolder*)) );
  connect( kmkernel->imapFolderMgr(), SIGNAL(folderRemoved(KMFolder*)), SLOT(folderRemoved(KMFolder*)) );
  connect( kmkernel->searchFolderMgr(), SIGNAL(folderRemoved(KMFolder*)), SLOT(folderRemoved(KMFolder*)) );

  QFont f = font();
  f.setItalic( true );
  setFont( f );

  new FolderViewToolTip( this );

  mInstances.append( this );
}

FavoriteFolderView::~FavoriteFolderView()
{
  mInstances.remove( this );
}

void FavoriteFolderView::readConfig()
{
  mReadingConfig = true;
  clear();
  QValueList<int> folderIds = GlobalSettings::self()->favoriteFolderIds();
  QStringList folderNames = GlobalSettings::self()->favoriteFolderNames();
  QListViewItem *afterItem = 0;
  for ( uint i = 0; i < folderIds.count(); ++i ) {
    KMFolder *folder = kmkernel->folderMgr()->findById( folderIds[i] );
    if ( !folder )
      folder = kmkernel->imapFolderMgr()->findById( folderIds[i] );
    if ( !folder )
      folder = kmkernel->dimapFolderMgr()->findById( folderIds[i] );
    if ( !folder )
      folder = kmkernel->searchFolderMgr()->findById( folderIds[i] );
    QString name;
    if ( folderNames.count() > i )
      name = folderNames[i];
    afterItem = addFolder( folder, name, afterItem );
  }
  if ( firstChild() )
    ensureItemVisible( firstChild() );

  // folder tree is not yet populated at this point
  QTimer::singleShot( 0, this, SLOT(initializeFavorites()) );

  readColorConfig();
  mReadingConfig = false;
}

void FavoriteFolderView::writeConfig()
{
  QValueList<int> folderIds;
  QStringList folderNames;
  for ( QListViewItemIterator it( this ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    folderIds << fti->folder()->id();
    folderNames << fti->text( 0 );
  }
  GlobalSettings::self()->setFavoriteFolderIds( folderIds );
  GlobalSettings::self()->setFavoriteFolderNames( folderNames );
}

bool FavoriteFolderView::acceptDrag(QDropEvent * e) const
{
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  if ( e->provides( "application/x-qlistviewitem" ) &&
       (e->source() == ft->viewport() || e->source() == viewport() ) )
    return true;
  return FolderTreeBase::acceptDrag( e );
}

KMFolderTreeItem* FavoriteFolderView::addFolder(KMFolder * folder, const QString &name, QListViewItem *after)
{
  if ( !folder )
    return 0;
  KMFolderTreeItem *item = new FavoriteFolderViewItem( this, name.isEmpty() ? folder->label() : name, folder );
  if ( after )
    item->moveItem( after );
  else
    item->moveItem( lastItem() );
  ensureItemVisible( item );
  insertIntoFolderToItemMap( folder, item );
  notifyInstancesOnChange();
  return item;
}

void FavoriteFolderView::selectionChanged()
{
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( selectedItem() );
  if ( !fti )
    return;
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  assert( fti );
  ft->showFolder( fti->folder() );
}

void FavoriteFolderView::itemClicked(QListViewItem * item)
{
  if ( item && !item->isSelected() )
    item->setSelected( true );
  item->repaint();
}

void FavoriteFolderView::folderTreeSelectionChanged(KMFolder * folder)
{
  blockSignals( true );
  bool found = false;
  for ( QListViewItemIterator it( this ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->folder() == folder && !fti->isSelected() ) {
      fti->setSelected( true );
      setCurrentItem( fti );
      ensureItemVisible( fti );
      fti->repaint();
      found = true;
    } else if ( fti->folder() != folder && fti->isSelected() ) {
      fti->setSelected( false );
      fti->repaint();
    }
  }
  blockSignals( false );
  if ( !found ) {
    clearSelection();
    setSelectionModeExt( KListView::NoSelection );
    setSelectionModeExt( KListView::Single );
  }
}

void FavoriteFolderView::folderRemoved(KMFolder * folder)
{
  QValueList<KMFolderTreeItem*> delItems;
  for ( QListViewItemIterator it( this ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->folder() == folder )
      delItems << fti;
    if ( fti == mContextMenuItem )
      mContextMenuItem = 0;
  }
  for ( uint i = 0; i < delItems.count(); ++i )
    delete delItems[i];
  removeFromFolderToItemMap(folder);
}

void FavoriteFolderView::dropped(QDropEvent * e, QListViewItem * after)
{
  QListViewItem* afterItem = after;
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  if ( e->source() == ft->viewport() && e->provides( "application/x-qlistviewitem" ) ) {
    for ( QListViewItemIterator it( ft ); it.current(); ++it ) {
      if ( !it.current()->isSelected() )
        continue;
      KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
      if ( !fti->folder() )
        continue;
      afterItem = addFolder( fti->folder(), prettyName( fti ), afterItem );
    }
    e->accept();
  }
}

void FavoriteFolderView::contextMenu(QListViewItem * item, const QPoint & point)
{
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( item );
  mContextMenuItem = fti;
  KPopupMenu contextMenu;
  if ( fti && fti->folder() ) {
    contextMenu.insertItem( SmallIconSet("editdelete"), i18n("Remove From Favorites"),
                            this, SLOT(removeFolder()) );
    contextMenu.insertItem( SmallIconSet("edit"), i18n("Rename Favorite"), this, SLOT(renameFolder()) );
    contextMenu.insertSeparator();

    mainWidget()->action("mark_all_as_read")->plug( &contextMenu );
    if ( fti->folder()->folderType() == KMFolderTypeImap || fti->folder()->folderType() == KMFolderTypeCachedImap )
      mainWidget()->action("refresh_folder")->plug( &contextMenu );
    if ( fti->folder()->isMailingListEnabled() )
      mainWidget()->action("post_message")->plug( &contextMenu );

    contextMenu.insertItem( SmallIconSet("configure_shortcuts"), i18n("&Assign Shortcut..."), fti, SLOT(assignShortcut()) );
    contextMenu.insertItem( i18n("Expire..."), fti, SLOT(slotShowExpiryProperties()) );
    mainWidget()->action("modify")->plug( &contextMenu );
  } else {
    contextMenu.insertItem( SmallIconSet("bookmark_add"), i18n("Add Favorite Folder..."),
                            this, SLOT(addFolder()) );
  }
  contextMenu.exec( point, 0 );
}

void FavoriteFolderView::removeFolder()
{
  delete mContextMenuItem;
  mContextMenuItem = 0;
  notifyInstancesOnChange();
}

void FavoriteFolderView::initializeFavorites()
{
  QValueList<int> seenInboxes = GlobalSettings::self()->favoriteFolderViewSeenInboxes();
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  for ( QListViewItemIterator it( ft ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->type() == KFolderTreeItem::Inbox && fti->folder() && !seenInboxes.contains( fti->folder()->id() ) ) {
      seenInboxes.append( fti->folder()->id() );
      if ( fti->folder() == kmkernel->inboxFolder() && hideLocalInbox() )
        continue;
      if ( kmkernel->iCalIface().hideResourceFolder( fti->folder() ) )
        continue;
      addFolder( fti->folder(), prettyName( fti ) );
    }
  }
  GlobalSettings::self()->setFavoriteFolderViewSeenInboxes( seenInboxes );
}

void FavoriteFolderView::renameFolder()
{
  if ( !mContextMenuItem )
    return;
  bool ok;
  QString name = KInputDialog::getText( i18n("Rename Favorite"), i18n("Name"), mContextMenuItem->text( 0 ), &ok, this );
  if ( !ok )
    return;
  mContextMenuItem->setText( 0, name );
  notifyInstancesOnChange();
}

QString FavoriteFolderView::prettyName(KMFolderTreeItem * fti)
{
  assert( fti );
  assert( fti->folder() );
  QString name = fti->folder()->label();
  QListViewItem *accountFti = fti;
  while ( accountFti->parent() )
    accountFti = accountFti->parent();
  if ( fti->type() == KFolderTreeItem::Inbox ) {
    if ( fti->protocol() == KFolderTreeItem::Local || fti->protocol() == KFolderTreeItem::NONE ) {
      name = i18n( "Local Inbox" );
    } else {
      name = i18n( "Inbox of %1" ).arg( accountFti->text( 0 ) );
    }
  } else {
    if ( fti->protocol() != KFolderTreeItem::Local && fti->protocol() != KFolderTreeItem::NONE ) {
      name = i18n( "%1 on %2" ).arg( fti->text( 0 ) ).arg( accountFti->text( 0 ) );
    } else {
      name = i18n( "%1 (local)" ).arg( fti->text( 0 ) );
    }
  }
  return name;
}

void FavoriteFolderView::contentsDragEnterEvent(QDragEnterEvent * e)
{
  if ( e->provides( "application/x-qlistviewitem" ) ) {
    setDropVisualizer( true );
    setDropHighlighter( false );
  } else if ( e->provides( KPIM::MailListDrag::format() ) ) {
    setDropVisualizer( false );
    setDropHighlighter( true );
  } else {
    setDropVisualizer( false );
    setDropHighlighter( false );
  }
  FolderTreeBase::contentsDragEnterEvent( e );
}

void FavoriteFolderView::readColorConfig()
{
  FolderTreeBase::readColorConfig();
  KConfig* conf = KMKernel::config();
  // Custom/System color support
  KConfigGroupSaver saver(conf, "Reader");
  QColor c = KGlobalSettings::alternateBackgroundColor();
  if ( !conf->readBoolEntry("defaultColors", true) )
    mPaintInfo.colBack = conf->readColorEntry( "AltBackgroundColor",&c );
  else
    mPaintInfo.colBack = c;

  QPalette newPal = palette();
  newPal.setColor( QColorGroup::Base, mPaintInfo.colBack );
  setPalette( newPal );
}

void FavoriteFolderView::addFolder()
{
  KMFolderSelDlg dlg( mainWidget(), i18n("Add Favorite Folder"), false );
  if ( dlg.exec() != QDialog::Accepted )
    return;
  KMFolder *folder = dlg.folder();
  if ( !folder )
    return;
  KMFolderTreeItem *fti = findFolderTreeItem( folder );
  addFolder( folder, fti ? prettyName( fti ) : folder->label() );
}

KMFolderTreeItem * FavoriteFolderView::findFolderTreeItem(KMFolder * folder) const
{
  assert( folder );
  KMFolderTree *ft = mainWidget()->folderTree();
  assert( ft );
  for ( QListViewItemIterator it( ft ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->folder() == folder )
      return fti;
  }
  return 0;
}

void FavoriteFolderView::checkMail()
{
  bool found = false;
  for ( QListViewItemIterator it( this ); it.current(); ++it ) {
    KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>( it.current() );
    if ( fti->folder()->folderType() == KMFolderTypeImap || fti->folder()->folderType() == KMFolderTypeCachedImap ) {
      if ( !found )
        if ( !kmkernel->askToGoOnline() )
          break;
      found = true;
      if ( fti->folder()->folderType() == KMFolderTypeImap ) {
        KMFolderImap *imap = static_cast<KMFolderImap*>( fti->folder()->storage() );
        imap->getAndCheckFolder();
      } else if ( fti->folder()->folderType() == KMFolderTypeCachedImap ) {
        KMFolderCachedImap* f = static_cast<KMFolderCachedImap*>( fti->folder()->storage() );
        f->account()->processNewMailSingleFolder( fti->folder() );
      }
    }
  }
}

void FavoriteFolderView::notifyInstancesOnChange()
{
  if ( mReadingConfig )
    return;
  writeConfig();
  for ( QValueList<FavoriteFolderView*>::ConstIterator it = mInstances.begin(); it != mInstances.end(); ++it ) {
    if ( (*it) == this || (*it)->mReadingConfig )
      continue;
    (*it)->readConfig();
  }
}

void FavoriteFolderView::refresh()
{    
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) {
    KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(it.current());
    if (!fti || !fti->folder())
      continue;
    fti->repaint();
  }
  update();
} 

#include "favoritefolderview.moc"
