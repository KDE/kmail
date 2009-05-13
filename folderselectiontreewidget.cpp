/******************************************************************************
 *
 * KMail Folder Selection Tree Widget
 *
 * Copyright (c) 1997-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2004-2005 Carsten Burghardt <burghardt@kde.org>
 * Copyright (c) 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *****************************************************************************/

#include "folderselectiontreewidget.h"
#include "mainfolderview.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "util.h"

#include <kaction.h>
#include <kmenu.h>
#include <kiconloader.h>
#include <kconfiggroup.h>

using namespace KMail::Util;

namespace KMail {

class FolderSelectionTreeWidgetItem : public KPIM::FolderTreeWidgetItem
{
public:
  FolderSelectionTreeWidgetItem(
      KPIM::FolderTreeWidget * listView,
      const FolderViewItem * srcItem
    )
    : KPIM::FolderTreeWidgetItem(
          listView, srcItem->labelText(),
          srcItem->protocol(), srcItem->folderType()
        ), mFolder( 0 ) {};

  FolderSelectionTreeWidgetItem(
      KPIM::FolderTreeWidgetItem * listViewItem,
      const FolderViewItem * srcItem
    )
    : KPIM::FolderTreeWidgetItem(
          listViewItem, srcItem->labelText(),
          srcItem->protocol(), srcItem->folderType()
        ), mFolder( 0 ) {};

public:
  void setFolder( KMFolder * folder )
    { mFolder = folder; };

  KMFolder * folder() const
    { return mFolder; };

private:
  KMFolder * mFolder;

};


FolderSelectionTreeWidget::FolderSelectionTreeWidget( QWidget * parent, ::KMail::MainFolderView * folderTree )
  : KPIM::FolderTreeWidget( parent ), mFolderTree( folderTree )
{
  setSelectionMode( QTreeWidget::SingleSelection );

  mNameColumnIndex = addColumn( i18n( "Folder" ) );
  mPathColumnIndex = addColumn( i18n( "Path" ) );

  setContextMenuPolicy( Qt::CustomContextMenu );
  connect( this, SIGNAL( customContextMenuRequested( const QPoint & ) ),
           this, SLOT( slotContextMenuRequested( const QPoint & ) ) );
  connect( this, SIGNAL( itemSelectionChanged() ),
           this, SLOT( slotItemSelectionChanged() ) );

  mCreateFolderAction = new KAction( KIcon( "folder-new" ),
                                     i18n("&New Subfolder..."), this );
  connect( mCreateFolderAction, SIGNAL( triggered() ),
           this, SLOT( addChildFolder() ) );
}

void FolderSelectionTreeWidget::recursiveReload( FolderViewItem *fti, FolderSelectionTreeWidgetItem *parent )
{
  // search folders are never shown
  if ( fti->protocol() == KPIM::FolderTreeWidgetItem::Search )
    return;

  // imap folders?
  if ( fti->protocol() == KPIM::FolderTreeWidgetItem::Imap && !mLastShowImapFolders )
    return;

  // the outbox?
  if ( fti->folderType() == KPIM::FolderTreeWidgetItem::Outbox && !mLastShowOutbox )
    return;

  // top level
  FolderSelectionTreeWidgetItem *item = parent ? new FolderSelectionTreeWidgetItem( parent, fti )
                                               : new FolderSelectionTreeWidgetItem( this, fti );

  item->setText( mNameColumnIndex, fti->labelText() );
  item->setIcon( 0, fti->icon( 0 ) );
  // Build the path (ParentItemPath/CurrentItemName)
  QString path;
  if( parent )
    path = parent->text( mPathColumnIndex ) + '/';
  path += fti->labelText();

  item->setText( mPathColumnIndex, path );

  // Make readonly and nocoontent items unselectable, if we're told so
  if ( ( mLastMustBeReadWrite && ( fti->folder() && fti->folder()->isReadOnly() ) ) ||
       ( fti->folder() && fti->folder()->noContent() ) ) {
    item->setFlags( item->flags() & ~Qt::ItemIsSelectable );
  } else {
    item->setFolder( fti->folder() );
  }

  int cc = fti->childCount();
  int i = 0;

  while ( i < cc )
  {
    FolderViewItem *child = dynamic_cast<FolderViewItem *>( ( ( QTreeWidgetItem * )fti)->child( i ) );
    if ( child )
      recursiveReload( child, item );
    i++;
  }
}

void FolderSelectionTreeWidget::reload( bool mustBeReadWrite, bool showOutbox,
                               bool showImapFolders, const QString& preSelection )
{
  mLastMustBeReadWrite = mustBeReadWrite;
  mLastShowOutbox = showOutbox;
  mLastShowImapFolders = showImapFolders;

  clear();

  QString selected = preSelection;
  if ( selected.isEmpty() && folder() )
    selected = folder()->idString();

  mFilter.clear();

  int cc = mFolderTree->topLevelItemCount();

  int i = 0;

  // Calling setUpdatesEnabled() here causes weird effects (including crashes)
  // in the folder requester (used by the filtering dialog).
  // So disable it for now, this makes the folderselection dialog appear much
  // slower though :(
  //setUpdatesEnabled( false );

  while ( i < cc )
  {
    FolderViewItem *child = dynamic_cast<FolderViewItem *>( mFolderTree->topLevelItem( i ) );
    if ( child )
      recursiveReload( child, 0 );
    i++;
  }

  // we do this here in one go after all items have been created, as that is
  // faster than expanding each item, which triggers a lot of updates
  expandAll();

  if ( !preSelection.isEmpty() )
    setFolder( preSelection );
}

KMFolder * FolderSelectionTreeWidget::folder() const
{
  QTreeWidgetItem * item = currentItem();
  if ( item ) {
    if ( item->flags() & Qt::ItemIsSelectable )
      return static_cast<FolderSelectionTreeWidgetItem *>( item )->folder();
  }
  return 0;
}

void FolderSelectionTreeWidget::setFolder( KMFolder *folder )
{
  for ( QTreeWidgetItemIterator it( this ) ; *it ; ++it )
  {
    const KMFolder *fld = static_cast<FolderSelectionTreeWidgetItem *>( *it )->folder();
    if ( fld == folder )
    {
      ( *it )->setSelected( true );
      scrollToItem( *it );
      setCurrentItem( *it );
      return;
    }
  }
}

void FolderSelectionTreeWidget::setFolder( const QString& idString )
{
  setFolder( kmkernel->findFolderById( idString ) );
}

void FolderSelectionTreeWidget::addChildFolder()
{
  reconnectSignalSlotPair( kmkernel->folderMgr(), SIGNAL( folderAdded(KMFolder*) ),
           this, SLOT( slotFolderAdded(KMFolder*) ) );
  reconnectSignalSlotPair( kmkernel->imapFolderMgr(), SIGNAL( folderAdded(KMFolder*) ),
           this, SLOT( slotFolderAdded(KMFolder*) ) );
  reconnectSignalSlotPair( kmkernel->dimapFolderMgr(), SIGNAL( folderAdded(KMFolder*) ),
           this, SLOT( slotFolderAdded(KMFolder*) ) );
  mFolderTree->addChildFolder( folder(), parentWidget() );
}

void FolderSelectionTreeWidget::slotContextMenuRequested( const QPoint &p )
{
  QTreeWidgetItem * lvi = itemAt( p );

  if ( !lvi )
    return;
  setCurrentItem( lvi );
  lvi->setSelected( true );

  KMenu *folderMenu = new KMenu;
  folderMenu->addTitle( static_cast<FolderSelectionTreeWidgetItem *>( lvi )->labelText() );
  folderMenu->addAction( mCreateFolderAction );

  kmkernel->setContextMenuShown( true );
  folderMenu->exec ( viewport()->mapToGlobal( p ), 0);
  kmkernel->setContextMenuShown( false );
  delete folderMenu;
  folderMenu = 0;
}

void FolderSelectionTreeWidget::slotFolderAdded( KMFolder *addedFolder )
{
  reload( mLastMustBeReadWrite, mLastShowOutbox, mLastShowImapFolders );
  setFolder( addedFolder );
  disconnect( kmkernel->folderMgr(), SIGNAL( folderAdded(KMFolder*) ),
             this, SLOT( slotFolderAdded(KMFolder*) ) );
  disconnect( kmkernel->imapFolderMgr(), SIGNAL( folderAdded(KMFolder*) ),
             this, SLOT( slotFolderAdded(KMFolder*) ) );
  disconnect( kmkernel->dimapFolderMgr(), SIGNAL( folderAdded(KMFolder*) ),
             this, SLOT( slotFolderAdded(KMFolder*) ) );
}

void FolderSelectionTreeWidget::slotItemSelectionChanged()
{
  bool allowOk = true;
  bool allowCreate = true;

  const QList<QTreeWidgetItem *> selItems = selectedItems();
  if ( selItems.isEmpty() )				// no selection
    allowOk = allowCreate = false;
  else
  {
    const KMFolder *fld = static_cast<FolderSelectionTreeWidgetItem *>( selectedItems().first() )->folder();
    if ( !fld )						// "Local Folders" root
      allowOk = !mLastMustBeReadWrite;
    else						// any other folder
    {
      allowCreate = !fld->noChildren() && !fld->isReadOnly();
      if ( mLastMustBeReadWrite )
          allowOk = !fld->noContent() && !fld->isReadOnly();
    }
  }

  mCreateFolderAction->setEnabled( allowCreate );
  emit actionsAllowed( allowOk, allowCreate );
}

void FolderSelectionTreeWidget::applyFilter( const QString& filter )
{
  // We would like to set items that do not match the filter to disabled,
  // but that also disables all the children of that item (qt bug 181410,
  // closed as WONTFIX).
  // So instead, we mark the items as not selectable. That unfortunalty does not
  // give us visual feedback, though.
  // In keyPressEvent(), we change the behavior of the up/down arrow to skip
  // non-selectable items.


  if ( filter.isEmpty() )
  {
    // Empty filter:
    // reset all items to enabled, visible, expanded and not selected
    QTreeWidgetItemIterator clean( this );
    while ( QTreeWidgetItem *item = *clean )
    {
      item->setHidden( false );
      item->setSelected( false );
      item->setFlags( item->flags() | Qt::ItemIsSelectable );
      ++clean;
    }

    setColumnText( mPathColumnIndex, i18n("Path") );
    return;
  }

  // Not empty filter.
  // Reset all items to disabled, hidden, closed and not selected
  QTreeWidgetItemIterator clean( this );
  while ( QTreeWidgetItem *item = *clean )
  {
    item->setHidden( true );
    item->setSelected( false );
    item->setFlags( item->flags() & ~Qt::ItemIsSelectable );
    ++clean;
  }

  // Now search...
  QList<QTreeWidgetItem *> lItems = findItems( mFilter, Qt::MatchContains | Qt::MatchRecursive, mPathColumnIndex );

  for( QList<QTreeWidgetItem *>::Iterator it = lItems.begin(); it != lItems.end(); ++it )
  {
    ( *it )->setFlags( ( *it )->flags() | Qt::ItemIsSelectable );
    ( *it )->setHidden( false );

    // Open all the parents up to this item
    QTreeWidgetItem * p = ( *it )->parent();
    while( p )
    {
      p->setHidden( false );
      p = p->parent();
    }
  }

  // Iterate through the list to find the first selectable item
  QTreeWidgetItemIterator first( this );
  while ( FolderSelectionTreeWidgetItem * item = static_cast< FolderSelectionTreeWidgetItem* >( *first ) )
  {
    if ( ( !item->isHidden() ) && ( !item->isDisabled() ) &&
          ( item->flags() & Qt::ItemIsSelectable ) )
    {
      item->setSelected( true );
      setCurrentItem( item );
      scrollToItem( item );
      break;
    }
    ++first;
  }

  // Display and save the current filter
  if ( filter.length() > 0 )
    setColumnText( mPathColumnIndex, i18n("Path") + "  ( " + filter + " )" );
  else
    setColumnText( mPathColumnIndex, i18n("Path") );
}

void FolderSelectionTreeWidget::keyPressEvent( QKeyEvent *e )
{
  // Handle keyboard filtering.
  // Each key with text is appended to our search filter (which gets displayed
  // in the header for the Path column). Backpace removes text from the filter
  // while the del button clears the filter completely.

  switch( e->key() )
  {
    case Qt::Key_Backspace:
      if ( mFilter.length() > 0 )
        mFilter.truncate( mFilter.length()-1 );
      applyFilter( mFilter );
      return;
    break;
    case Qt::Key_Delete:
      mFilter = "";
      applyFilter( mFilter);
      return;
    break;

    // Reimplement up/down arrow handling to skip non-selectable items
    case Qt::Key_Up:
    {
      QTreeWidgetItem *newCurrent = currentItem();
      do {
        newCurrent = itemAbove( newCurrent );
      } while ( newCurrent && !( newCurrent->flags() & Qt::ItemIsSelectable ) );
      if ( newCurrent )
        setCurrentItem( newCurrent );
      return;
    }
    break;
    case Qt::Key_Down:
    {
      QTreeWidgetItem *newCurrent = currentItem();
      do {
        newCurrent = itemBelow( newCurrent );
      } while ( newCurrent && !( newCurrent->flags() & Qt::ItemIsSelectable ) );
      if ( newCurrent )
        setCurrentItem( newCurrent );
      return;
    }
    break;

    default:
    {
      QString s = e->text();
      if ( !s.isEmpty() && s.at( 0 ).isPrint() ) {
         mFilter += s;
        applyFilter( mFilter );
        return;
      }
    }
    break;
  }

  KPIM::FolderTreeWidget::keyPressEvent( e );
}

} // namespace KMail

#include "folderselectiontreewidget.moc"
