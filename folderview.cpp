/******************************************************************************
 *
 * Copyright (c) 2007 Volker Krause <vkrause@kde.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************/

#include "folderview.h"

#include <libkdepim/maillistdrag.h> // KPIM::MailList
#include <libkdepim/broadcaststatus.h> // KPIM::BroadcastStatus

#include "kmaccount.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldermgr.h"
#include "kmfoldercachedimap.h"
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "kmailicalifaceimpl.h"

#include "accountmanager.h"
#include "acljobs.h"
#include "expirypropertiesdialog.h"
#include "favoritefolderview.h" // ugly, derived class: should be moved from here :/
#include "mainfolderview.h" // ugly, derived class: should be moved from here :/
#include "foldershortcutdialog.h"
#include "folderstorage.h"
#include "messagecopyhelper.h"
#include "newfolderdialog.h"
#include "util.h"

#include <klocale.h>
#include <kdebug.h>
#include <kicon.h>
#include <kconfiggroup.h>
#include <kglobalsettings.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kcolorscheme.h>
#include <kconfig.h>
#include <kiconloader.h>

#include <QActionGroup>
#include <QContextMenuEvent>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFontMetrics>
#include <QHash>
#include <QHeaderView>
#include <QHelpEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTime>
#include <QTimer>
#include <QToolTip>
#include <QVariant>

// TODO: Use QDockWidget for this ?

// TODO: Option to show / hide inactive folders "on-the-fly". That is, hide folders
//       which don't have unread messages, aren't close to quota etc...

// Timeout after that we execute a delayed slotUpdateCounts() (milliseconds)
static const int gLazyUpdateCountsTimeout = 500;
// The maximum time that we can spend "at once" inside updateCounts() (milliseconds)
static const int gUpdateCountsProcessingInterval = 200;
// The time in msecs an user must wait over a parent item to get it automatically expanded (dnd)
static const int gDNDAutoExpandDelay = 500;


namespace KMail
{



// The one and only dragged folder list
static QList< QPointer< KMFolder > > gDraggedFolderList;

QString DraggedFolderList::mimeDataType()
{
  return QString( "application/x-kmail-folder-list" );
}

void DraggedFolderList::set( QList< QPointer< KMFolder > > &list )
{
  gDraggedFolderList = list;
}

void DraggedFolderList::clear()
{
  gDraggedFolderList.clear();
}

QList< QPointer< KMFolder > > DraggedFolderList::get()
{
  return gDraggedFolderList;
}





FolderViewManager::FolderViewManager( KMMainWidget *, const char *name )
 : QObject( 0 )
{
  setObjectName( name );
}

void FolderViewManager::attachView( FolderView *view )
{
  mFolderViews.removeAll( view ); // make sure we don't store it twice (should never happen tough)
  mFolderViews.append( view );
}

void FolderViewManager::detachView( FolderView *view )
{
  mFolderViews.removeAll( view );
}

void FolderViewManager::viewFolderActivated( FolderView *view, KMFolder *fld, bool middleButton )
{
  for ( QList<FolderView *>::Iterator it = mFolderViews.begin(); it != mFolderViews.end(); ++it )
  {
    if ( ( *it ) != view )
      ( *it )->notifyFolderActivated( fld );
  }

  emit folderActivated( fld );
  emit folderActivated( fld, middleButton );
}






FolderView::FolderView( KMMainWidget *mainWidget, FolderViewManager *manager, QWidget *parent, const QString &configPrefix, const char *name )
 : KPIM::FolderTreeWidget( parent, name ),
   mMainWidget( mainWidget ),
   mManager( manager ),
   mConfigPrefix( configPrefix ),
   mToolTipDisplayPolicy( DisplayAlways ),
   mSortingPolicy( SortByCurrentColumn ),
   mDnDMoveTargetItem( 0 ),
   mDnDSortReferenceItem( 0 )
{
  mManager->attachView( this );

  mUpdateCountsTimer = new QTimer( this );

  connect( mUpdateCountsTimer, SIGNAL( timeout() ),
           SLOT( slotUpdateCounts() ) );

  setDragEnabled( true );

  // Disable auto-expanding, because it will actually do auto-collapsing, too,
  // which makes manually reordering folders unusable.
  // Trolltech issue N220934
  //setAutoExpandDelay( gDNDAutoExpandDelay );
  setAutoExpandDelay( -1 );

  setSelectionMode( ExtendedSelection );

  setDropIndicatorShown( false );

  header()->setMinimumSectionSize( 20 ); // empiric :)
  header()->setStretchLastSection( false );

  setIconSize( QSize( 22, 22 ) );

  addLabelColumn( i18n( "Folder" ) );
  addUnreadColumn( i18nc( "@title:column Column showing the number of unread email messages.",
                          "Unread" ) );
  addTotalColumn( i18nc( "@title:column Column showing the total number of messages",
                         "Total" ) );
  addDataSizeColumn( i18nc( "@title:column Size of the folder.", "Size" ) );

  header()->setResizeMode( LabelColumn, QHeaderView::Interactive );
  header()->setResizeMode( UnreadColumn, QHeaderView::Interactive);
  header()->setResizeMode( TotalColumn, QHeaderView::Interactive );
  header()->setResizeMode( DataSizeColumn, QHeaderView::Interactive );

  connect( this, SIGNAL( columnVisibilityChanged( int ) ),
           SLOT( slotColumnVisibilityChanged( int ) ) );

  // Connect local folder manager
  connect( kmkernel->folderMgr(), SIGNAL( changed() ),
           SLOT( slotReload() ) );
  connect( kmkernel->folderMgr(), SIGNAL( folderRemoved( KMFolder * ) ),
           SLOT( slotFolderRemoved( KMFolder * ) ) );
  connect( kmkernel->folderMgr(), SIGNAL( folderMoveOrCopyOperationFinished() ),
           SLOT( slotFolderMoveOrCopyOperationFinished() ) );

  // Connect the IMAP folder manager
  connect( kmkernel->imapFolderMgr(), SIGNAL( changed() ),
           SLOT( slotReload() ) );
  connect( kmkernel->imapFolderMgr(), SIGNAL( folderRemoved( KMFolder * ) ),
           SLOT( slotFolderRemoved( KMFolder * ) ) );

  // Connect the Cached Imap folder manager
  connect( kmkernel->dimapFolderMgr(), SIGNAL( changed() ),
           SLOT( slotReload() ) );
  connect( kmkernel->dimapFolderMgr(), SIGNAL( folderRemoved( KMFolder * ) ),
           SLOT( slotFolderRemoved( KMFolder * ) ) );

  // Connect the Search virtual folder manager
  connect( kmkernel->searchFolderMgr(), SIGNAL( changed() ),
           SLOT( slotReload() ) );
  connect( kmkernel->searchFolderMgr(), SIGNAL( folderRemoved( KMFolder * ) ),
           SLOT( slotFolderRemoved( KMFolder * ) ) );

  // Track account removals
  connect( kmkernel->acctMgr(), SIGNAL( accountRemoved( KMAccount * ) ),
           SLOT( slotAccountRemoved( KMAccount * ) ) );

  // Track folder expands and collapses (dynamic tree updates)
  connect( this, SIGNAL( itemExpanded( QTreeWidgetItem * ) ),
           SLOT( slotFolderExpanded( QTreeWidgetItem * ) ) );
  connect( this, SIGNAL( itemCollapsed( QTreeWidgetItem * ) ),
           SLOT( slotFolderCollapsed( QTreeWidgetItem * ) ) );

  connect( this, SIGNAL( itemClicked ( QTreeWidgetItem *, int ) ),
           SLOT( slotItemClicked ( QTreeWidgetItem *, int ) ) );

}

FolderView::~FolderView()
{
  // We don't use mManager here since we want to be really sure
  // that KMMainWidget didn't delete it yet (may happen at shutdown).
  if ( mainWidget()->folderViewManager() )
    mainWidget()->folderViewManager()->detachView( this );
}

void FolderView::readConfig()
{
  readColorConfig();

  // Custom/System font support
  KConfigGroup conf( KMKernel::config(), "Fonts" );
  if (!conf.readEntry( "defaultFonts", true ) )
    setFont( conf.readEntry("folder-font", KGlobalSettings::generalFont() ) );
  else
    setFont( KGlobalSettings::generalFont() );

  if ( !restoreLayout( KMKernel::config(), "Geometry", mConfigPrefix + "Layout" ) )
  {
    // hide all the columns but the first (the default)
    setColumnHidden( DataSizeColumn, true );
    setColumnHidden( UnreadColumn, true );
    setColumnHidden( TotalColumn, true );
    // default sort order is ascending
    header()->setSortIndicator( LabelColumn, Qt::AscendingOrder );

    // Use a singleshot, as there is no layout here yet, and therefore no correct sizes
    QTimer::singleShot( 0, this, SLOT( setDefaultColumnSizes() ) );
  }

  KConfigGroup myGroup( KMKernel::config(), mConfigPrefix );
  int iIconSize = myGroup.readEntry( "IconSize", iconSize().width() );
  if ( iIconSize < 16 || iIconSize > 32 )
    iIconSize = 22;
  setIconSize( QSize( iIconSize, iIconSize ) );

  setToolTipDisplayPolicy( ( ToolTipDisplayPolicy )myGroup.readEntry( "ToolTipDisplayPolicy", ( int ) toolTipDisplayPolicy() ) );
  setSortingPolicy( ( SortingPolicy )myGroup.readEntry( "SortingPolicy", ( int ) sortingPolicy() ) );

  // we don't restore item states here: we'll do it in reload() instead.
}

void FolderView::writeConfig()
{
  saveItemStates();
  saveLayout( KMKernel::config(), "Geometry", mConfigPrefix + "Layout" );

  KConfigGroup myGroup( KMKernel::config(), mConfigPrefix );
  myGroup.writeEntry( "IconSize", iconSize().width() );
  myGroup.writeEntry( "ToolTipDisplayPolicy", ( int ) toolTipDisplayPolicy() );
  myGroup.writeEntry( "SortingPolicy", ( int ) sortingPolicy() );
}

void FolderView::setSortingPolicy( SortingPolicy policy )
{
  mSortingPolicy = policy;
  switch ( mSortingPolicy )
  {
    case SortByCurrentColumn:
      header()->setClickable( true );
      header()->setSortIndicatorShown( true );
      setSortingEnabled( true );
    break;
    case SortByDragAndDropKey:
      header()->setClickable( false );
      header()->setSortIndicatorShown( false );
      //
      // Qt 4.5 introduced a nasty bug here:
      // Sorting must be enabled in order to sortByColumn() to work.
      // If sorting is disabled it disconnects some internal signal/slot pairs
      // and calling sortByColumn() silently has no effect.
      // This is a bug as we actually DON'T want automatic sorting to be
      // performed by the view whenever it wants. We want to control sorting.
      //
      setSortingEnabled( true ); // hack for qutie bug: the param here should be false
      sortByColumn( LabelColumn, Qt::AscendingOrder );
      setSortingEnabled( false ); // hack for qutie bug: this call shouldn't be here at all
      fixSortingKeysForChildren( invisibleRootItem() );
    break;
    default:
      // should never happen
    break;
  }
}

void FolderView::saveItemStates()
{
  FolderViewItem * fvi;

  QTreeWidgetItemIterator it( this );
  while ( ( fvi = static_cast< FolderViewItem * >( *it ) ) )
  {
    QString configGroupName = fvi->configGroupName();
    if ( !configGroupName.isEmpty() )
    {
      KConfigGroup conf( KMKernel::config(), configGroupName );

      conf.writeEntry( mConfigPrefix + "ItemIsExpanded", fvi->isExpanded() );
      conf.writeEntry( mConfigPrefix + "ItemIsSelected", fvi->isSelected() );
      conf.writeEntry( mConfigPrefix + "ItemDnDSortingKey", fvi->dndSortingKey() );
    }
    ++it;
  }
}

void FolderView::restoreItemStates()
{
  // This function attempts to restore the state of the items stored
  // in the configuration file. State includes "Expanded" state, "Hidden"
  // state and selection.

  // At the time of writing QTreeWidgetIterator can't be used to loop on items
  // while setting properties on them. In particular setExpanded()/setHidden()
  // or setSelected() (lost no time to investigate which one exactly) mess
  // up the iteration pointer and we end up traversing twice some parts
  // of the tree while not traversing at all some other.

  // This is why we use a QHash here.
  QHash< QString, FolderViewItem * > itemHash;

  QTreeWidgetItemIterator it( this );
  FolderViewItem * fvi;

  while ( ( fvi = static_cast< FolderViewItem * >( *it ) ) )
  {
    QString configGroupName = fvi->configGroupName();
    if ( !configGroupName.isEmpty() )
      itemHash.insert( configGroupName, fvi );
    ++it;
  }

  QHash< QString, FolderViewItem * >::Iterator mit;

  for( mit = itemHash.begin(); mit != itemHash.end(); ++mit )
  {
    fvi = mit.value();

    KConfigGroup conf( KMKernel::config(), mit.key() );

    if ( conf.hasKey( mConfigPrefix + "ItemIsExpanded" ) )
      fvi->setExpanded( conf.readEntry( mConfigPrefix + "ItemIsExpanded", false ) );
    fvi->setSelected( conf.readEntry( mConfigPrefix + "ItemIsSelected", false ) );
    fvi->setDndSortingKey( conf.readEntry( mConfigPrefix + "ItemDnDSortingKey", -1 ) );
  }
}

void FolderView::cleanupConfigFile()
{
  // This is invoked by KMMainWidget to cleanup the configuration
  // file from stale entries.

  if ( topLevelItemCount() == 0 )
    return; // just in case reload wasn't called before

  KConfig* config = KMKernel::config();
  FolderViewItem * fvi;
  QHash<QString,bool> folderMap;

  // Gather a map of currently displayed folders
  QTreeWidgetItemIterator it(this);
  while ( ( fvi = static_cast< FolderViewItem * >( *it ) ) )
  {
    QString configGroupName = fvi->configGroupName();
    if ( !configGroupName.isEmpty() )
      folderMap.insert( configGroupName, true );
    ++it;
  }

  // Loop through the config groups and try to kill the ones
  // that are not present in the map above
  QStringList groupList = config->groupList();

  for ( QStringList::Iterator grpIt = groupList.begin(); grpIt != groupList.end(); ++grpIt)
  {
    if ( !( *grpIt ).startsWith( KMFOLDER_CONFIG_GROUP_NAME_PREFIX ) )
      continue;

    if ( !folderMap.contains( *grpIt ) )
    {
      // Folder not displayed in the tree
      // Check if it's a special hidden folder...
      QString folderId = ( *grpIt ).mid( KMFOLDER_CONFIG_GROUP_NAME_PREFIX_LEN );

      KMFolder* folder = kmkernel->findFolderById( folderId );
      if ( folder ) {
          // The folder, in fact, exists.
          if (
              kmkernel->iCalIface().hideResourceFolder( folder ) ||
              kmkernel->iCalIface().hideResourceAccountRoot( folder )
            )
            continue; // hidden IMAP resource folder, don't delete info
      }

      // Nope.. it either doesn't exist at all or it's not displayed
      // for some reason (may happen ?)... kill the group.
      config->deleteGroup( *grpIt, KConfig::Localized );
    }
  }
}

void FolderView::setDefaultColumnSizes()
{
    // OK: what follows is totally empiric.. but it looks nice ;)

  //kDebug() << "Setting default column sizes!";

  // A shown column usually causes the horizontal scrollbar to appear.
  // The user is then forced to readjust the size of the column manually,
  // sometimes with non trivial operations (try it: #ifdef this code and see :D).
  // This tends to be very annoying thus a section relayout is a good idea.

  // We do our own resizing since none of the defaults provided by Qt 4.4 seem to
  // be nice enough. QHeaderView::Interactive and QHeaderView::Fixed are clearly
  // useless here. QHeaderView::Stretch makes all the columns the same size (which looks ugly)
  // and QHeaderView::ResizeToContents often will cause the scrollbar to appear again.

  // If you feel afraid when mantaining the following code, be aware
  // that nothing really relies on it...

  if ( !isColumnHidden( LabelColumn ) )
  {
    int count = header()->count();
    int vwidth = viewport()->width();

    // all the sections but the "LabelColumn" get 70 pixels (yes, this is heuristic)
    for ( int c = 0; c < count ; c++ )
    {
      if ( !isColumnHidden( c ) && ( c != LabelColumn ) )
      {
        header()->resizeSection( c, 70 );
        vwidth -= 70;
      }
    }
    // then the LabelSection gets the remaining space (but at least 180 pixels)
    header()->resizeSection( LabelColumn, vwidth >= 180 ? vwidth : 180 );
  }
}

void FolderView::slotColumnVisibilityChanged( int logicalIndex )
{
  // This is called when one of our columns is hidden or shown either
  // by the means of the header popup menu or programmatically.

  //kDebug() << "Column visibility changed!";

  bool nowVisible = !isColumnHidden( logicalIndex );

  switch( logicalIndex )
  {
    case LabelColumn:
      // Ugh.. should never happen :/
    break;
    case UnreadColumn:
    case TotalColumn:
    case DataSizeColumn:
      reload( nowVisible );
    break;
  }

  setDefaultColumnSizes();

  emit columnsChanged();
}

void FolderView::mousePressEvent( QMouseEvent * e )
{
  mLastButtonPressedWasMiddle = e->button() == Qt::MidButton;

  KPIM::FolderTreeWidget::mousePressEvent( e );
}

void FolderView::slotItemClicked( QTreeWidgetItem *item, int column )
{
  Q_UNUSED( column );
  FolderViewItem *folderItem = dynamic_cast<FolderViewItem*>( item );
  Q_ASSERT( folderItem );
  bool multiSelection = ( QApplication::keyboardModifiers() & Qt::ControlModifier ) ||
                        ( QApplication::keyboardModifiers() & Qt::ShiftModifier );
  if ( folderItem && !multiSelection )
    activateItem( folderItem, false, mLastButtonPressedWasMiddle );
}

void FolderView::slotReload()
{
  // This is called mainly because folders have been added/deleted: update the tree
  reload( false );
}

void FolderView::slotDelayedReload()
{
  QTimer::singleShot( 0, this, SLOT( slotReload() ) );
}

void FolderView::reloadInternal( QTreeWidgetItem *item )
{
  // The recursive part of reload().
  // Makes connections on every item.
  // Scans the tree from bottom to top by updating the counts (from cache)

  // This is also very likely to be faster than QTreeWidgetIterator.

  int cc = item->childCount();
  int i = 0;
  while ( i < cc )
  {
    FolderViewItem *fvi = static_cast< FolderViewItem * >( item->child( i ) );

    if ( fvi )
    {
      reloadInternal( fvi );

      fvi->updateCounts();

      KMFolder *fld = fvi->folder();

      if ( fld )
      {
        Util::reconnectSignalSlotPair( fld, SIGNAL( iconsChanged() ), fvi, SLOT( slotIconsChanged() ) );
        Util::reconnectSignalSlotPair( fld, SIGNAL( nameChanged() ), fvi, SLOT( slotNameChanged() ) );
        Util::reconnectSignalSlotPair( fld, SIGNAL( noContentChanged() ), fvi, SLOT( slotNoContentChanged() ) );

        // we want to be noticed of changes to update the unread/total columns
        Util::reconnectSignalSlotPair( fld, SIGNAL( msgAdded( KMFolder *, quint32 ) ), fvi, SLOT( slotFolderCountsChanged( KMFolder * ) ) );
        Util::reconnectSignalSlotPair( fld, SIGNAL( msgRemoved( KMFolder * ) ), fvi, SLOT( slotFolderCountsChanged( KMFolder * ) ) );
        Util::reconnectSignalSlotPair( fld, SIGNAL( numUnreadMsgsChanged( KMFolder * ) ), fvi, SLOT( slotFolderCountsChanged( KMFolder * ) ) );
        Util::reconnectSignalSlotPair( fld, SIGNAL( folderSizeChanged( KMFolder * ) ), fvi, SLOT( slotFolderCountsChanged( KMFolder * ) ) );

        Util::reconnectSignalSlotPair( fld, SIGNAL( shortcutChanged( KMFolder * ) ), mMainWidget, SLOT( slotShortcutChanged( KMFolder * ) ) );
      }
    }
    i++;
  }

}

void FolderView::reload( bool openFoldersForUpdate )
{
  // Clears and re-fills the tree.

  // This code attempts to be a "generic" reload
  // routine for all the folder views.

  // The basic idea is that we need to loop on all the folder
  // trees exposed by the folder managers. On each node
  // we create a view item.

  // The trees themselves are not consistent and a couple of
  // them need a fake root to be added. This function takes
  // care of that.

  // The magic differences between all the views are added
  // in the createItem() method which is a pure virtual and
  // must be overridden by the child classes.

  // createItem() is passed the information about the folder
  // we want to create a view item for and it's position in
  // the hierarchy. The child view is then allowed to mess
  // up a bit by either skipping the item creation (favorite folder
  // view does that), by modifying the position in the hierarchy
  // or by changing the item displayable properties (such as the label).

  setUpdatesEnabled( false );
  setSortingEnabled( false ); // disable sorting while inserting items

  // remember the scroll bar positions so we can restore them later
  int oldLeft = horizontalScrollBar()->value();
  int oldTop = verticalScrollBar()->value();

  // save the current item states, if any
  saveItemStates();

  // clean up the view
  clear();

  // construct the (fake) root of the local folders
  FolderViewItem *root = createItem(
      0, i18n( "Local Folders" ),
      0, FolderViewItem::Local, FolderViewItem::Root
    );
  // note that root MIGHT be 0 here (if the view filtered it out)
  addDirectory( false, &( kmkernel->folderMgr()->dir() ), root );

  // make a better first impression by showing the "Local Folders" expanded per default
  // Further down the last saved state (expanded/collapsed) will be restored
  if ( root )
    root->setExpanded( true );

  // each imap-account creates it's own root
  addDirectory( true, &( kmkernel->imapFolderMgr()->dir() ), 0);

  // each dimap-account creates it's own root
  addDirectory( true, &( kmkernel->dimapFolderMgr()->dir() ), 0);

  // construct the root of the search folder hierarchy:
  root = createItem(
      0, i18n( "Searches" ),
      0, FolderViewItem::Search, FolderViewItem::Root
    );
  // note that root MIGHT be 0 here (if the view filtered it out)
  addDirectory( false, &( kmkernel->searchFolderMgr()->dir() ), root );

  // Do connections on all the items
  reloadInternal( invisibleRootItem() );

  // Trigger a complete count update, if requested
  if ( openFoldersForUpdate )
  {
    // We open all folders to update the count
    for( QTreeWidgetItemIterator itr( this ); QTreeWidgetItem * twitem = (*itr); ++itr )
    {
      FolderViewItem *fvi = static_cast< FolderViewItem * >( twitem );

      if ( !fvi->folder() )
        continue;

      fvi->setCountsDirty( true );
    }

    triggerImmediateUpdateCounts(); // this will trigger after a zero timeout.
  }

  // restore the item states that we have saved earlier
  restoreItemStates();

  // restore scrollbars
  horizontalScrollBar()->setValue( oldLeft );
  verticalScrollBar()->setValue( oldTop );

  // fix up sorting, if needed and re-enable the updates
  setUpdatesEnabled( true );
  if ( sortingPolicy() == SortByCurrentColumn )
    setSortingEnabled( true );
  else {
    //
    // Qt 4.5 introduced a nasty bug here:
    // Sorting must be enabled in order to sortByColumn() to work.
    // If sorting is disabled it disconnects some internal signal/slot pairs
    // and calling sortByColumn() silently has no effect.
    // This is a bug as we actually DON'T want automatic sorting to be
    // performed by the view whenever it wants. We want to control sorting.
    //
    setSortingEnabled( true ); // hack for qutie bug: this call shouldn't be here at all
    sortByColumn( LabelColumn, Qt::AscendingOrder );
    setSortingEnabled( false ); // hack for qutie bug: this call shouldn't be here at all
    fixSortingKeysForChildren( invisibleRootItem() );
  }
}

void FolderView::fixSortingKeysForChildren( QTreeWidgetItem *item )
{
  int cc = item->childCount();
  int idx = 0;
  while ( idx < cc )
  {
    FolderViewItem *fvi = static_cast<FolderViewItem *>( item->child( idx ) );
    if ( fvi )
    {
      fvi->setDndSortingKey( idx );
      fixSortingKeysForChildren( fvi );
    }
    ++idx;
  }
}

void FolderView::addDirectory( bool createRoots, KMFolderDir * fdir, FolderViewItem *parent )
{
  if ( !fdir )
    return; // should never happen, but well....

  QList<KMFolderNode*>::const_iterator it;

  for ( it = fdir->constBegin(); it != fdir->constEnd() ; ++it )
  {
    KMFolderNode * node = *it;
    if ( !node )
      continue;

    if ( node->isDir() )
      continue;

    KMFolder * folder = static_cast<KMFolder*>(node);
    FolderViewItem * fvi = 0;

    if ( createRoots )
    {
      // create new root-item, but only if this is not the root of a
      // "groupware folders only" account
      if ( kmkernel->iCalIface().hideResourceAccountRoot( folder ) )
        continue;
      // it needs a folder e.g. to save it's state (open/close)
      fvi = createItem(
          parent, folder->label(), folder,
          FolderViewItem::protocolByFolder( folder ),
          FolderViewItem::Root
        );
      // note that root MIGHT be 0 here (if the view filtered it out)
      if ( fvi )
        fvi->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

      // add child-folders
      if ( folder->child() )
        addDirectory( false, folder->child(), fvi );

    } else {
      // hide local inbox if unused (that is if option enabled, no messages, no accounts)
      if ( ( kmkernel->inboxFolder() == folder ) && hideLocalInbox() )
      {
        // make sure we're reloaded when the precondition for hiding the local inbox changes
        Util::reconnectSignalSlotPair( kmkernel->inboxFolder(), SIGNAL( msgAdded( KMFolder*, quint32 ) ),
                                       this, SLOT( slotReload() ) );
        continue; // skip child creation
      }

      // create new child
      fvi = createItem(
          parent, folder->label(), folder,
          FolderViewItem::protocolByFolder( folder ),
          FolderViewItem::folderTypeByFolder( folder )
        );
      // set folders explicitly to exandable when they have children
      // this way we can do a listing for IMAP folders when the user expands them
      // even when the child folders are not created yet
      if ( fvi && ( folder->storage()->hasChildren() == FolderStorage::HasChildren ) )
        fvi->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

      // add child-folders
      if ( folder->child() )
        addDirectory( false, folder->child(), fvi );

      if ( fvi )
      {
        // Check if this is an IMAP resource folder or a no-content parent only
        // containing groupware folders
        if (
            fvi &&
            ( kmkernel->iCalIface().hideResourceFolder( folder ) || folder->noContent() ) &&
            ( fvi->childCount() == 0 )
          )
        {
          delete fvi;
          // still, it might change in the future, so we better check the change signals
          connect ( folder, SIGNAL( noContentChanged() ), SLOT( slotDelayedReload() ) );
          continue;
        }

        connect( fvi, SIGNAL( iconChanged( FolderViewItem * ) ),
                 this, SIGNAL( iconChanged( FolderViewItem * ) ) );
        connect( fvi, SIGNAL( nameChanged( FolderViewItem * ) ),
                 this, SIGNAL( nameChanged( FolderViewItem * ) ) );
      }
    }
  } // for-end
}

void FolderView::triggerLazyUpdateCounts()
{
  // Called to trigger a delayed update of the counts of the folders marked with countsDirty().
  if ( mUpdateCountsTimer->isActive() )
    return; // update already pending
  mUpdateCountsTimer->start( gLazyUpdateCountsTimeout );
}

void FolderView::triggerImmediateUpdateCounts()
{
  if ( mUpdateCountsTimer->isActive() )
  {
    if ( mUpdateCountsTimer->interval() == 0 )
      return; // immediate update already pending
    // need to restart
    mUpdateCountsTimer->stop();
  }
  mUpdateCountsTimer->start( 0 );
}


FolderView::UpdateCountsResult FolderView::updateCountsForChildren( QTreeWidgetItem *it, const QTime &tStart )
{
  // Update counts for the children of it, recursively: first go to the children
  // of the children and if any were dirty, mark as dirty the direct child too.
  // Break (and resume later) if running for too long.

  int cc = it->childCount();
  int i = 0;
  bool childrenWereDirty = false;
  int elapsed;

  while ( i < cc )
  {
    FolderViewItem *fvi = static_cast< FolderViewItem *>( it->child( i ) );
    i++;
    if ( !fvi )
      continue;

    // go down to the children
    switch ( updateCountsForChildren( fvi, tStart ) )
    {
      case OperationCompletedDirty:
        // some children were dirty
        fvi->setCountsDirty( false ); // propagate dirty flag to this item
        childrenWereDirty = true; // and propagate up

        // At least one item was dirty and has been updated.
        // We can check the timeout now and eventually interrupt the operation.
        // Checking that at least one item was updated ensures that we will
        // not end up in endless loops trying to update and never making progress.
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > gUpdateCountsProcessingInterval ) || ( elapsed < 0 ) )
          return OperationInterrupted; // it's taking too long, will continue later.
      break;
      case OperationCompletedClean:
        // no children were dirty
      break;
      case OperationInterrupted:
        // the operation was interrupted due to a timeout: propagate up
        return OperationInterrupted;
      break;
    }

    // update this item, if needed
    if ( fvi->countsDirty() )
    {
      childrenWereDirty = true;
      fvi->updateCounts();

      // maybe this took time, check again, unless this is the last item
      if ( i < cc )
      {
        elapsed = tStart.msecsTo( QTime::currentTime() );
        if ( ( elapsed > gUpdateCountsProcessingInterval ) || ( elapsed < 0 ) )
          return OperationInterrupted; // it's taking too long, will continue later.
      }
    }
  }

  return childrenWereDirty ? OperationCompletedDirty : OperationCompletedClean;
}


void FolderView::slotUpdateCounts()
{
  // This is connected to the mUpdateCountsTimer timeout() signal.

  // We attempt to update as more items as possible but also avoid
  // blocking the UI for too much. We do the job in timed chunks.
  // If an update takes too long, we ask a timer to call us in a while
  // and jump out to let the application handle some events.

  // FIXME: Update visible items first. Children should come before parents too.

  mUpdateCountsTimer->stop();

  QTime tStart = QTime::currentTime();

  // We scan the children starting from the leafs and going up
  // This ensures coherent children counts
  if ( updateCountsForChildren( invisibleRootItem(), tStart ) == OperationInterrupted )
  {
    // the operation has been interrupted due to a timeout, restart it in a while
    mUpdateCountsTimer->start( 0 );
  }

  viewport()->update();
}

void FolderView::mouseReleaseEvent( QMouseEvent *e )
{
  FolderViewItem * fvi = dynamic_cast<FolderViewItem *>( currentItem() );

  // When the middle button is used on a folder with a mailing list
  // start composing a message to that mailing list too

  // FIXME: Middle click is also used to open the folder in a new tab!

  if (
       fvi &&
       fvi->folder() &&
       (e->button() == Qt::MidButton) &&
       fvi->folder()->isMailingListEnabled()
    )
  {
    KMCommand *command = new KMMailingListPostCommand( this, fvi->folder() );
    command->start();
  }

  KPIM::FolderTreeWidget::mouseReleaseEvent( e );
}


void FolderView::slotFolderRemoved( KMFolder *folder )
{
  FolderViewItem * fvi = static_cast< FolderViewItem * >( findItemByFolder( folder ) );
  if ( !fvi )
    return;
  // (in)sanity check
  if ( fvi->folder() != folder)
    return; // insane...

  if ( ( (QTreeWidgetItem *)fvi ) == currentItem() )
  {
    QTreeWidgetItem * newcurrent = itemAbove( fvi );
    if ( !newcurrent )
      newcurrent = itemBelow( fvi );

    FolderViewItem * ncfvi = static_cast< FolderViewItem * >( newcurrent );

    activateItem( ncfvi );
  }

  delete fvi;
  updateCopyActions();
}

void FolderView::setCurrentFolder( KMFolder *folder )
{
  FolderViewItem * fvi;

  if ( !folder )
    fvi = static_cast< FolderViewItem * >(
              findItemByProperties(
                  KPIM::FolderTreeWidgetItem::Local,
                  KPIM::FolderTreeWidgetItem::Root
                )
            ); // activate Local Root
  else
    fvi = static_cast< FolderViewItem * >(
              findItemByFolder(
                  folder
                )
            ); // activate the item with the specified folder

  if ( !fvi )
  {
    clearSelection();
    return;
  }

  activateItem( fvi );
}

void FolderView::activateItem( FolderViewItem *fvi, bool keepSelection, bool middleButton )
{
  activateItemInternal( fvi, keepSelection, true, middleButton );
}

void FolderView::activateItemInternal( FolderViewItem *fvi, bool keepSelection, bool notifyManager, bool middleButton )
{
  if ( !fvi )
    return; // nothing to do: we keep everything as it is

  KMFolder* folder = fvi->folder();

  if ( fvi->isHidden() )
    fvi->setHidden( false );

  if ( !keepSelection )
    clearSelection();
  if ( currentItem() != ( ( QTreeWidgetItem * ) fvi ) )
    setCurrentItem( fvi );
  if ( !keepSelection && !fvi->isSelected() )
    fvi->setSelected( true );

  scrollToItem( fvi );

  if ( notifyManager )
    mManager->viewFolderActivated( this, folder, middleButton );

  if ( folder )
  {
    // update counts up to top-level parent
    while ( fvi )
    {
      if ( !fvi->updateCounts() )
         break; // nothing has changed at this level: parents can't change
      fvi = static_cast< FolderViewItem * >( static_cast< QTreeWidgetItem *>( fvi )->parent() );
    }

  }

  updateCopyActions();
}

void FolderView::notifyFolderActivated( KMFolder *folder )
{
  FolderViewItem * fvi;

  if ( !folder )
    fvi = static_cast< FolderViewItem * >(
              findItemByProperties(
                  KPIM::FolderTreeWidgetItem::Local,
                  KPIM::FolderTreeWidgetItem::Root
                )
            ); // activate Local Root
  else
    fvi = static_cast< FolderViewItem * >(
              findItemByFolder(
                  folder
                )
            ); // activate the item with the specified folder

  if ( !fvi )
  {
    clearSelection();
    return;
  }

  activateItemInternal( fvi, false, false, false );
}

void FolderView::slotAddToFavorites()
{
  KMail::FavoriteFolderView *favView = mMainWidget->favoriteFolderView();
  if ( !favView )
    return; // ugh :/
  foreach ( const QPointer<KMFolder> &f, selectedFolders() )
    favView->addFolder( f );
}

void FolderView::updateCopyActions()
{
  QAction *copy = mMainWidget->action("copy_folder");
  QAction *cut = mMainWidget->action("cut_folder");
  QAction *paste = mMainWidget->action("paste_folder");

  QList< QPointer< KMFolder > > selected = selectedFolders();

  if ( selected.isEmpty() )
  {
    copy->setEnabled( false );
    cut->setEnabled( false );

  } else {

    bool bCanMove = true;
    for( QList< QPointer< KMFolder > >::Iterator it = selected.begin(); it != selected.end(); ++it )
    {
      if ( !( *it ) )
      {
        // yes, it can happen
        bCanMove = false; // stay safe
        break;
      }

      if( !( *it )->isMoveable() )
      {
        bCanMove = false;
        break;
      }
    }

    copy->setEnabled( true );
    cut->setEnabled( bCanMove );
  }

  paste->setEnabled( !mFolderClipboard.isEmpty() );
}

bool FolderView::event( QEvent *e )
{
  // We catch ToolTip events and pass everything else

  if( e->type() != QEvent::ToolTip )
    return KPIM::FolderTreeWidget::event( e );

  if ( mToolTipDisplayPolicy == DisplayNever )
    return true; // never display tooltips

  QHelpEvent * he = dynamic_cast< QHelpEvent * >( e );
  if ( !he )
    return true; // eh ?

  QPoint pnt = viewport()->mapFromGlobal( mapToGlobal( he->pos() ) );

  if ( pnt.y() < 0 )
    return true; // don't display the tooltip for items hidden under the header

  QTreeWidgetItem * it = itemAt( pnt );
  if ( !it )
    return true; // may be

  FolderViewItem * fvi = static_cast< FolderViewItem * >( it );

  if ( ( mToolTipDisplayPolicy == DisplayWhenTextElided ) && ( !fvi->labelTextElided() ) )
    return true; // don't display if text not elided

  QString bckColor = palette().color( QPalette::ToolTipBase ).name();
  QString txtColor = palette().color( QPalette::ToolTipText ).name();

  QString tip = QString::fromLatin1(
      "<table width=\"100%\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"
    );

  tip += QString::fromLatin1(
        "<tr>" \
          "<td bgcolor=\"%1\" colspan=\"2\" align=\"left\" valign=\"middle\">" \
            "<div style=\"color: %2; font-weight: bold;\">" \
             "%3" \
            "</div>" \
          "</td>" \
        "</tr>"
    ).arg( txtColor ).arg( bckColor ).arg( fvi->labelText() );


  tip += QString::fromLatin1(
        "<tr>" \
          "<td align=\"left\" valign=\"top\">"
    );

  if ( KMFolder * fld = fvi->folder() )
  {
    if ( !fld->noContent() )
    {
      tip += QString::fromLatin1(
              "<strong>%1</strong>: %2<br>" \
              "<strong>%3</strong>: %4<br><br>"
        ).arg( i18n("Total Messages") ).arg( fld->count( !fld->isOpened() ) )
         .arg( i18n("Unread Messages") ).arg( fld->countUnread() );

      if ( KMFolderCachedImap * imap = dynamic_cast< KMFolderCachedImap * >( fld->storage() ) )
      {
        QuotaInfo info = imap->quotaInfo();
        if ( info.isValid() && !info.isEmpty() )
          tip += QString( "<strong>%1</strong>: %2<br>" ).arg( i18n( "Quota" ) ).arg( info.toString() );
      }

      if ( fld->storage()->folderSize() >= 0 )
        tip += QString::fromLatin1(
                "<strong>%1</strong>: %2<br>"
          ).arg( i18n("Storage Size") ).arg( KIO::convertSize( (KIO::filesize_t)( fld->storage()->folderSize() ) ) );

      fvi->setCountsDirty( true ); // make sure the counts in the view get updated
      triggerLazyUpdateCounts();
    }

    if (fvi->childrenDataSize() >= 0 )
      tip += QString::fromLatin1(
              "<strong>%1</strong>: %2<br>"
        ).arg( i18n("Subfolder Storage Size") ).arg( KIO::convertSize( (KIO::filesize_t)( fvi->childrenDataSize() ) ) );

    // FIXME prettyUrl() ?

    /*
    This makes the tooltip too big :/
    QString location = fld->location();
    if ( !location.isEmpty() )
    {
      tip += QString::fromLatin1(
          "<strong>%1</strong>: <nobr>%2</nobr><br>"
        ).arg( i18n("Path") ).arg( fontMetrics().elidedText( location, Qt::ElideRight, 250 ) );
    }
    */
  }

  int icon_sizes[] = { /* 128, 64, 48, */ 32, 22 };

  QString iconPath;

  for ( int i = 0; i < 2; i++ )
  {
    iconPath = KIconLoader::global()->iconPath( fvi->normalIcon(), -icon_sizes[ i ], true );
    if ( !iconPath.isEmpty() )
      break;
  }

  if ( iconPath.isEmpty() )
    iconPath = KIconLoader::global()->iconPath( "folder", -32, false );

  tip += QString::fromLatin1(
          "</td>" \
          "<td align=\"right\" valign=\"top\">" \
            "<table border=\"0\"><tr><td width=\"32\" height=\"32\" align=\"center\" valign=\"middle\">"
              "<img src=\"%1\">" \
            "</td></tr></table>" \
          "</td>" \
        "</tr>" \
    ) .arg( iconPath );


  tip += QString::fromLatin1(
      "</table>"
    );

  QToolTip::showText( he->globalPos(), tip, viewport(), visualItemRect( it ) );

  return true;
}

bool FolderView::fillHeaderContextMenu( KMenu * menu, const QPoint &clickPoint )
{
  if ( !KPIM::FolderTreeWidget::fillHeaderContextMenu( menu, clickPoint ) )
    return false;

  QAction *act;

  menu->addTitle( i18n( "Icon Size" ) );

  static int icon_sizes[] = { 16, 22, 32 /*, 48, 64, 128 */ };

  QActionGroup *grp = new QActionGroup( menu );

  for ( int i = 0; i < (int)( sizeof( icon_sizes ) / sizeof( int ) ); i++ )
  {
    act = menu->addAction( QString("%1x%2").arg( icon_sizes[ i ] ).arg( icon_sizes[ i ] ) );
    act->setCheckable( true );
    grp->addAction( act );
    if ( iconSize().width() == icon_sizes[ i ] )
      act->setChecked( true );
    act->setData( QVariant( icon_sizes[ i ] ) );

    connect( act, SIGNAL( triggered( bool ) ),
             SLOT( slotHeaderContextMenuChangeIconSize( bool ) ) );
  }

  menu->addTitle( i18n( "Display Tooltips" ) );

  grp = new QActionGroup( menu );

  act = menu->addAction( i18nc("@action:inmenu Always display tooltips", "Always") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mToolTipDisplayPolicy == DisplayAlways );
  act->setData( QVariant( (int)DisplayAlways ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeToolTipDisplayPolicy( bool ) ) );
  act = menu->addAction( i18nc("@action:inmenu", "When Text Obscured") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mToolTipDisplayPolicy == DisplayWhenTextElided );
  act->setData( QVariant( (int)DisplayWhenTextElided ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeToolTipDisplayPolicy( bool ) ) );

  act = menu->addAction( i18nc("@action:inmenu Never display tooltips.", "Never") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mToolTipDisplayPolicy == DisplayNever );
  act->setData( QVariant( (int)DisplayNever ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeToolTipDisplayPolicy( bool ) ) );

  menu->addTitle( i18nc("@action:inmenu", "Sort Items" ) );

  grp = new QActionGroup( menu );

  act = menu->addAction( i18nc("@action:inmenu", "Automatically, by Current Column") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mSortingPolicy == SortByCurrentColumn );
  act->setData( QVariant( (int)SortByCurrentColumn ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeSortingPolicy( bool ) ) );

  act = menu->addAction( i18nc("@action:inmenu", "Manually, by Drag And Drop") );
  act->setCheckable( true );
  grp->addAction( act );
  act->setChecked( mSortingPolicy == SortByDragAndDropKey );
  act->setData( QVariant( (int)SortByDragAndDropKey ) );
  connect( act, SIGNAL( triggered( bool ) ),
           SLOT( slotHeaderContextMenuChangeSortingPolicy( bool ) ) );

  return true;
}

void FolderView::slotHeaderContextMenuChangeSortingPolicy( bool )
{
  QAction *act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  int policy = data.toInt( &ok );
  if ( !ok )
    return;

  setSortingPolicy( ( SortingPolicy )policy );
}

void FolderView::slotHeaderContextMenuChangeToolTipDisplayPolicy( bool )
{
  QAction *act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  int policy = data.toInt( &ok );
  if ( !ok )
    return;

  setToolTipDisplayPolicy( ( ToolTipDisplayPolicy )policy );

  // stay in sync with ConfigureDialog
  writeConfig();
}

void FolderView::slotHeaderContextMenuChangeIconSize( bool )
{
  QAction *act = dynamic_cast< QAction * >( sender() );
  if ( !act )
    return;

  QVariant data = act->data();

  bool ok;
  int size = data.toInt( &ok );
  if ( !ok )
    return;

  setUpdatesEnabled( false );
  setIconSize( QSize( size, size ) );
  reload();
  setUpdatesEnabled( true );
}

//
// (Contextual) popup menu machinery
//

void FolderView::fillContextMenuAccountRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection )
{
  if ( multiSelection )
    return; // no multiSelection Account Related actions

  KMFolder *folder = item->folder();

  if ( !folder )
    return; // no actions for items without folders at this time

  // Send Mail / Check Mail (Account Related)

  // outbox specific, but there it's the most used action
  // If the current item is the outbox and it has messages inside, allow sending them
  if ( ( folder == kmkernel->outboxFolder() ) && ( folder->count() > 0 ) )
    menu->addAction( mMainWidget->action( "send_queued" ) );

  if ( ( folder->folderType() == KMFolderTypeImap ) || ( folder->folderType() == KMFolderTypeCachedImap ) )
  {
    if ( item->folderType() == FolderViewItem::Root )
    {
      // check mail in imap/dimap root folder
      menu->addAction(
          KIcon( "mail-receive" ), i18n( "Check &Mail" ),
          this, SLOT( slotCheckMail() )
        );
    } else {
      // check mail in imap/dimap subfolder
      if ( !folder->noContent() )
        menu->addAction( mMainWidget->action( "refresh_folder" ) );
    }

    menu->addAction(
        KIcon( "folder-bookmarks" ),
        i18n( "Serverside Subscription..." ), mMainWidget,
        SLOT( slotSubscriptionDialog() )
      );

    menu->addAction(
        KIcon( "folder-bookmarks" ),
        i18n( "Local Subscription..." ), mMainWidget,
        SLOT( slotLocalSubscriptionDialog() )
      );
  }
}

void FolderView::fillContextMenuMessageRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection )
{
  if ( multiSelection )
    return; // no multiSelection Account Related actions

  KMFolder *folder = item->folder();

  if ( !folder )
    return; // no actions without a folder at this time

  // Message Related Actions

  if ( folder->isMailingListEnabled() )
    menu->addAction( mMainWidget->action("post_message") );

  if ( !folder->noContent() )
  {
    // mark all messages as read
    menu->addAction( mMainWidget->action( "mark_all_as_read" ) );
    // search messages
    menu->addAction( mMainWidget->action( "search_messages" ) );
    // move all messages to trash
    menu->addAction( mMainWidget->action( "empty" ) );
  }
}

void FolderView::fillContextMenuTreeStructureRelatedActions( KMenu */*menu*/, FolderViewItem */*item*/, bool /*multiSelection*/ )
{
  // this is overridden in derived classes
}

void FolderView::fillContextMenuViewStructureRelatedActions( KMenu */*menu*/, FolderViewItem */*item*/, bool /*multiSelection*/ )
{
  // this is overridden in derived classes
}

void FolderView::fillContextMenuFolderServiceRelatedActions( KMenu *menu, FolderViewItem *item, bool multiSelection )
{
  if ( multiSelection )
    return; // no folder service actions for multifolder at this time

  KMFolder *folder = item->folder();

  if ( !folder )
  {
    // local root or searches
    menu->addAction( mMainWidget->action( "compact_all_folders" ) );
    menu->addAction( mMainWidget->action( "expire_all_folders" ) );
  }

  if ( !folder )
    return; // no more folder service actions for non-foldered items (notably for local root and searches)

  // Folder "Service" Actions

  if ( !folder->noContent() && ( ( folder->folderType() == KMFolderTypeImap ) || ( folder->folderType() == KMFolderTypeCachedImap ) ) )
  {
    if ( folder->folderType() == KMFolderTypeImap )
      menu->addAction(
          KIcon( "view-refresh" ), i18n( "Refresh Folder List" ),
          this, SLOT( slotResetFolderList() )
        );
  }

  // compaction is only supported for mbox folders
  // see FolderDialogMaintenanceTab::FolderDialogMaintenanceTab()
  if ( folder->folderType() == KMFolderTypeMbox )
    menu->addAction( mMainWidget->action( "compact" ) );

  if ( item->folderType() != FolderViewItem::Root ) // non account level folder
  {
    menu->addAction( mMainWidget->action( "folder_shortcut_command" ) );
    if ( !folder->noContent() )
      menu->addAction( i18n("Expire..."), item, SLOT( slotShowExpiryProperties() ) );

    // properties
    menu->addAction( mMainWidget->action("modify") );

    // FIXME: Couldn't we show the account edit dialog for imap roots ?
  }
}

void FolderView::fillContextMenuNoItem( KMenu *menu )
{
  Q_UNUSED( menu );
  // this is overridden in derived classes
}

// A little helper function: Adds a separator to the menu if it has more items
// than lastActionCount. Returns the number of new items.
static int insertSeparatorIfNeeded( int lastActionCount, QMenu *menu )
{
  if ( lastActionCount < menu->actions().count() )
    menu->insertSeparator( menu->actions().at( lastActionCount ) );
  return menu->actions().count();
}

void FolderView::contextMenuEvent( QContextMenuEvent *e )
{
  //
  // Build the item's contextual menu.
  //
  // This code __attempts__ to favor some sort of logical order in the menu entries.
  //

  FolderViewItem * fvi = static_cast<FolderViewItem *>( itemAt( e->pos() ) );

  KMenu * menu = new KMenu();
  if ( fvi ) {
    KMFolder *folder = fvi->folder();

    // make sure the folder is activated (so actions will get updated)
    // but keep the selection (so multifolder is possible)
    Q_ASSERT( mMainWidget );
    if ( mMainWidget->activeFolder() != folder )
      activateItem( fvi, true );

    bool multiSelection = selectedFolders().count() > 1;

    // First of all, the title
    if ( multiSelection )
    {
      // multiple folders selected: add only actions that can handle all of them
      menu->addTitle( i18n( "Multiple Folders" ) );
    } else {
      // single (or no) folder selected
      if ( folder )
        menu->addTitle( folder->label() );
      else
        menu->addTitle( fvi->text( LabelColumn ) );
    }

    fillContextMenuAccountRelatedActions( menu, fvi, multiSelection );

    // keep track of the number of actions so we can add separators in the right place, if needed
    int lastActionCount = menu->actions().count();

    fillContextMenuMessageRelatedActions( menu, fvi, multiSelection );
    lastActionCount = insertSeparatorIfNeeded( lastActionCount, menu );

    fillContextMenuTreeStructureRelatedActions( menu, fvi, multiSelection );
    lastActionCount = insertSeparatorIfNeeded( lastActionCount, menu );

    fillContextMenuViewStructureRelatedActions( menu, fvi, multiSelection );
    lastActionCount = insertSeparatorIfNeeded( lastActionCount, menu );

    fillContextMenuFolderServiceRelatedActions( menu, fvi, multiSelection );
    lastActionCount = insertSeparatorIfNeeded( lastActionCount, menu );
  }
  else
    fillContextMenuNoItem( menu );

  if ( !menu->isEmpty() ) {
    kmkernel->setContextMenuShown( true );
    menu->exec( e->globalPos() , 0 );
    kmkernel->setContextMenuShown( false );
  }

  delete menu;
}

KMFolder * FolderView::currentFolder() const
{
  FolderViewItem * fvi = static_cast<FolderViewItem *>( currentItem() );
  if ( fvi )
    return fvi->folder();
  return 0;
}

QString FolderView::currentItemFullPath() const
{
  QString ret;
  FolderViewItem * fvi = static_cast<FolderViewItem *>( currentItem() );
  if ( !fvi )
    return ret;
  ret = fvi->labelText();
  fvi = static_cast< FolderViewItem * >( static_cast< QTreeWidgetItem * >( fvi )->parent() );
  while ( fvi )
  {
    ret.prepend( "/" );
    ret.prepend( fvi->labelText() );
    fvi = static_cast< FolderViewItem * >( static_cast< QTreeWidgetItem * >( fvi )->parent() );
  }
  return ret;
}


QList< QPointer< KMFolder > > FolderView::selectedFolders()
{
  QList< QPointer< KMFolder > > rv;

  for ( QTreeWidgetItemIterator it( this ) ; *it ; ++it )
  {
    FolderViewItem * fvi = static_cast<FolderViewItem *>( *it );
    if ( fvi->isSelected() && fvi->folder() )
      rv.append( fvi->folder() );
  }

  return rv;
}

// FIXME: Does it *really* belong to here ?
void FolderView::moveOrCopyFolders( const QList<QPointer<KMFolder> > &sources, KMFolder* destination, bool move )
{
  // Disable drag during copy operation since it prevents from many crashes
  setDragEnabled( false );

  if ( ! moveOrCopyFoldersInternal( sources, destination, move ) )
    setDragEnabled( true ); // make sure we re-enable dragging if the above op fails
  // else drag will be re-enabled in slotFolderMoveOrCopyOperationFinished().
}

bool FolderView::moveOrCopyFoldersInternal( const QList<QPointer<KMFolder> > &sources, KMFolder* destination, bool move )
{
  KMFolderDir *parent = &(kmkernel->folderMgr()->dir());
  if ( destination )
    parent = destination->createChildFolder();

  QStringList sourceFolderNames;

  // check if move/copy is possible at all
  for ( QList<QPointer<KMFolder> >::ConstIterator it = sources.constBegin(); it != sources.constEnd(); ++it )
  {
    KMFolder* source = *it;

    // check if folder with same name already exits
    QString sourceFolderName;
    if ( source )
      sourceFolderName = source->label();

    if ( parent &&
         ( parent->hasNamedFolder( sourceFolderName ) ||
           sourceFolderNames.contains( sourceFolderName ) ) )
    {
      KMessageBox::error( this, i18n("<qt>Cannot move or copy folder <b>%1</b> here because a folder with the same name already exists.</qt>", sourceFolderName ) );
      return false;
    }
    sourceFolderNames.append( sourceFolderName );

    // don't move/copy a folder that's still not completely moved/copied
    KMFolder *f = source;
    while ( f )
    {
      if ( f->moveInProgress() )
      {
        KMessageBox::error( this, i18n("<qt>Cannot move or copy folder <b>%1</b> because it is not completely copied itself.</qt>", sourceFolderName ) );
        return false;
      }
      if ( f->parent() )
        f = f->parent()->owner();
    }

    QString message = i18n( "<qt>Cannot move or copy folder <b>%1</b> into a subfolder below itself.</qt>", sourceFolderName );

    KMFolderDir* folderDir = parent;
    // check that the folder can be moved
    if ( source && source->child() )
    {
      while ( folderDir && ( folderDir != &kmkernel->folderMgr()->dir() ) &&
          ( folderDir != source->parent() ) )
      {
        if ( folderDir->indexOf( source ) != -1 )
        {
          KMessageBox::error( this, message );
          return false;
        }
        folderDir = folderDir->parent();
      }
    }

    if( source && source->child() && parent &&
        ( parent->path().indexOf( source->child()->path() + '/' ) == 0 ) ) {
      KMessageBox::error( this, message );
      return false;
    }

    if( source && source->child()
        && ( parent == source->child() ) ) {
      KMessageBox::error( this, message );
      return false;
    }
  }

  // check if the source folders are independent of each other
  for ( QList<QPointer<KMFolder> >::ConstIterator it = sources.constBegin(); move && it != sources.constEnd(); ++it ) {
    KMFolderDir *parentDir = (*it)->child();
    if ( !parentDir )
      continue;
    for ( QList<QPointer<KMFolder> >::ConstIterator it2 = sources.constBegin(); it2 != sources.constEnd(); ++it2 ) {
      if ( *it == *it2 )
        continue;
      KMFolderDir *childDir = (*it2)->parent();
      do {
        if ( parentDir == childDir || parentDir->indexOf( childDir->owner() ) != -1 ) {
          KMessageBox::error( this, i18n("Moving the selected folders is not possible") );
          return false;
        }
        childDir = childDir->parent();
      }
      while ( childDir && childDir != &kmkernel->folderMgr()->dir() );
    }
  }

  // de-select moved source folders (can cause crash due to unGetMsg() in KMHeaders)
  if ( move )
    clearSelection();

  // do the actual move/copy
  for ( QList<QPointer<KMFolder> >::ConstIterator it = sources.constBegin(); it != sources.constEnd(); ++it )
  {
    KMFolder* source = *it;
    if ( move )
      kmkernel->folderMgr()->moveFolder( source, parent );
    else
      kmkernel->folderMgr()->copyFolder( source, parent );
  }

  return true;
}

void FolderView::slotFolderMoveOrCopyOperationFinished()
{
  setDragEnabled( true );
}

void FolderView::slotCheckMail()
{
  KMFolder * folder = currentFolder();

  if ( !folder )
    return;

  if ( folder->storage() )
  {
    if ( KMAccount *acct = folder->storage()->account() )
       kmkernel->acctMgr()->singleCheckMail( acct, true );
  }
}

void FolderView::slotNewMessageToMailingList()
{
  KMFolder * folder = currentFolder();

  if ( !folder )
    return;

  KMCommand *command = new KMMailingListPostCommand( this, folder );
  command->start();
}

void FolderView::slotAccountRemoved( KMAccount * )
{
  FolderViewItem * fvi = static_cast< FolderViewItem * >( firstItem() );
  activateItem( fvi );
}

// little static helper
static bool folderHasCreateRights( const KMFolder *folder )
{
  bool createRights = true; // we don't have acls for local folders yet
  if ( folder && folder->folderType() == KMFolderTypeImap ) {
    const KMFolderImap *imapFolder = static_cast<const KMFolderImap*>( folder->storage() );
    createRights = imapFolder->userRights() == 0 || // hack, we should get the acls
      ( imapFolder->userRights() > 0 && ( imapFolder->userRights() & KMail::ACLJobs::Create ) );
  } else if ( folder && folder->folderType() == KMFolderTypeCachedImap ) {
    const KMFolderCachedImap *dimapFolder = static_cast<const KMFolderCachedImap*>( folder->storage() );
    createRights = dimapFolder->userRights() == 0 ||
      ( dimapFolder->userRights() > 0 && ( dimapFolder->userRights() & KMail::ACLJobs::Create ) );
  }
  return createRights;
}

void FolderView::addChildFolder( KMFolder *fld, QWidget *par )
{
  // Create a subfolder.
  // Requires creating the appropriate subdirectory and show a dialog
  if ( fld )
  {
    if ( !fld->createChildFolder() )
      return;
    if ( !folderHasCreateRights( fld ) )
    {
      // FIXME: change this message to "Cannot create folder under ..." or similar
      const QString message = i18n( "<qt>Cannot create folder <b>%1</b> because of insufficient "
                                    "permissions on the server. If you think you should be able to create "
                                    "subfolders here, ask your administrator to grant you rights to do so."
                                    "</qt> " , fld->label() );
      KMessageBox::error( par, message );
      return;
    }
  }

  ( new KMail::NewFolderDialog( par, fld ) )->show();
}


void FolderView::slotAddChildFolder()
{
  addChildFolder( currentFolder(), this );
}

void FolderView::slotCopyFolder()
{
  mFolderClipboard = selectedFolders();
  mFolderClipboardMode = CopyFolders;
  updateCopyActions();
}

void FolderView::slotCutFolder()
{
  mFolderClipboard = selectedFolders();
  mFolderClipboardMode = CutFolders;
  updateCopyActions();
}

void FolderView::slotPasteFolder()
{
  if ( mFolderClipboard.isEmpty() )
    return;

  KMFolder *fld = currentFolder();
  if ( !fld )
    return;

  if ( mFolderClipboard.contains( fld ) )
    mFolderClipboard.removeAll( fld );

  if ( mFolderClipboard.isEmpty() )
    return;

  moveOrCopyFolders( mFolderClipboard, fld, mFolderClipboardMode == CutFolders );

  updateCopyActions();
}

bool FolderView::selectFolderIfUnread(FolderViewItem *fvi, bool confirm)
{
  if (
       !(
         fvi && fvi->folder() &&
         ( !fvi->folder()->ignoreNewMail() ) &&
         ( fvi->folder()->countUnread() > 0 )
        )
     )
    return false;

  // yes: folder has unread messages.

  // Don't change into the trash or outbox folders.
  if (
       fvi->folderType() == FolderViewItem::Trash ||
       fvi->folderType() == FolderViewItem::Outbox
     )
    return false;

  if ( !confirm )
  {
    activateItem( fvi );
    return true; // OK
  }

  // Need confirmation.

  // Skip drafts, sent mail and templates as well, when reading mail with the
  // space bar - but not when changing into the next folder with unread mail
  // via ctrl+ or ctrl- so we do this only if (confirm == true), which means
  // we are doing readOn.

  if (
       fvi->folderType() == FolderViewItem::Drafts ||
       fvi->folderType() == FolderViewItem::Templates ||
       fvi->folderType() == FolderViewItem::SentMail
     )
    return false;

  // warn user that going to next folder - but keep track of
  // whether he wishes to be notified again in "AskNextFolder"
  // parameter (kept in the config file for kmail)
  if (
       KMessageBox::questionYesNo(
           this,
           i18n( "<qt>Go to the next unread message in folder <b>%1</b>?</qt>" , fvi->folder()->label() ),
           i18n( "Go to Next Unread Message" ),
           KGuiItem( i18n( "Go To" ) ),
           KGuiItem( i18n( "Do Not Go To" ) ), // defaults
           "AskNextFolder",
           false
         ) == KMessageBox::No
     )
    return true; // assume selected (do not continue looping)

  activateItem( fvi );

  // FIXME: Which is the purpose of this signal ? Doesn't folderSelected() suffice ?
  //emit folderSelectedUnread( fvi->folder() );

  return true;
}

void FolderView::focusItem( QTreeWidgetItem *item )
{
  if( !item )
    return;
  scrollToItem( item );
  setFocus();
  setCurrentItem( item, 0, QItemSelectionModel::NoUpdate );
}

void FolderView::slotFocusNextFolder()
{
  if ( topLevelItemCount() < 1 )
    return;

  QTreeWidgetItem * item = currentItem();
  if ( item ) {
    QTreeWidgetItem *below = itemBelow( item );
    focusItem( below );
  }
}

void FolderView::slotFocusPrevFolder()
{
  if ( topLevelItemCount() < 1 )
    return;

  QTreeWidgetItem * item = currentItem();
  if ( item ) {
    QTreeWidgetItem *above = itemAbove( item );
    focusItem( above );
  }
}

void FolderView::slotSelectFocusedFolder()
{
  FolderViewItem *folderItem = dynamic_cast<FolderViewItem *>( currentItem() );
  if ( !folderItem )
    return;
  activateItem( folderItem );
}

//
// Methods for navigating folders with the keyboard
//

bool FolderView::selectNextUnreadFolder( bool confirm )
{
  // first try from the current item (if any) to the end

  QTreeWidgetItem * current = currentItem();

  if ( current )
  {
    QTreeWidgetItemIterator it( current, QTreeWidgetItemIterator::NotHidden );
    ++it; // skip current

    while ( *it )
    {
      bool folderSelected = selectFolderIfUnread( static_cast< FolderViewItem * >( *it ),
                                                  confirm );
      if ( folderSelected )
        return true;
      ++it;
    }
  }

  // didn't find a folder to select: try from the beginning up to current

  QTreeWidgetItem * begin = firstItem();

  if ( begin )
  {
    QTreeWidgetItemIterator it( begin, QTreeWidgetItemIterator::NotHidden );

    while ( *it )
    {
      if ( *it == current )
        return false; // reached current

      bool folderSelected = selectFolderIfUnread( static_cast< FolderViewItem * >( *it ),
                                                  confirm );
      if ( folderSelected )
        return true;
      ++it;
    }
  }

  // No folder selected when we reach here
  return false;
}

bool FolderView::selectPrevUnreadFolder()
{
  // first try from the current item (if any) to the beginning

  QTreeWidgetItem * current = currentItem();

  if ( current )
  {
    QTreeWidgetItemIterator it( current, QTreeWidgetItemIterator::NotHidden );
    --it; // skip current

    while ( *it )
    {
      bool folderSelected = selectFolderIfUnread( static_cast< FolderViewItem * >( *it ),
                                                  false /* don't confirm*/ );
      if ( folderSelected )
        return true;
      --it;
    }
  }

  // didn't find a folder to select: try from the end up to current

  QTreeWidgetItem * end = lastItem();

  if ( end )
  {
    QTreeWidgetItemIterator it( end, QTreeWidgetItemIterator::NotHidden );

    while ( *it )
    {
      if ( *it == current )
        return false; // reached current

      bool folderSelected = selectFolderIfUnread( static_cast< FolderViewItem * >( *it ),
                                                  false /* don't confirm*/ );
      if ( folderSelected )
        return true;
      --it;
    }
  }

  // No folder selected when we reach here
  return false;
}

void FolderView::readColorConfig()
{
  KConfigGroup cg = KMKernel::config()->group( "Reader" );

  // Custom/System color support
  if ( !cg.readEntry( "defaultColors", true ) )
  {
    setUnreadCountColor( cg.readEntry( "UnreadMessage", QColor( Qt::blue ) ) );
    setCloseToQuotaWarningColor( cg.readEntry( "CloseToQuotaColor", QColor( Qt::red ) ) );
  } else {
    setUnreadCountColor( Qt::blue );
    setCloseToQuotaWarningColor( Qt::blue );
  }
}

bool FolderView::hideLocalInbox() const
{
  if ( !GlobalSettings::self()->hideLocalInbox() )
    return false;
  KMFolder *localInbox = kmkernel->inboxFolder();
  assert( localInbox );
  // check if it is empty
  KMFolderOpener openInbox( localInbox, "FolderView" );
  if ( localInbox->count() > 0 )
    return false;
  // check if it has subfolders
  if ( localInbox->child() && !localInbox->child()->isEmpty() )
    return false;
  // check if it is an account target
  if ( localInbox->hasAccounts() )
    return false;
  return true;
}

void FolderView::handleMailListDrop(QDropEvent * event, KMFolder *destination )
{
  KPIM::MailList list = KPIM::MailList::fromMimeData( event->mimeData() );
  if ( list.isEmpty() ) {
    kWarning() << "Could not decode drag data!";
  } else {
    QList<uint> serNums = MessageCopyHelper::serNumListFromMailList( list );
    int action;
    if ( MessageCopyHelper::inReadOnlyFolder( serNums ) )
      action = DragCopy;
    else
      action = dragMode();
    if ( action == DragCopy || action == DragMove ) {
      new MessageCopyHelper( serNums, destination, action == DragMove, this );
    }
  }
}

FolderViewItem * FolderView::findItemByFolder( const KMFolder *folder )
{
  QTreeWidgetItemIterator it( this );
  while( FolderViewItem * fvi = dynamic_cast<FolderViewItem *>( *it ) )
  {
    if ( fvi->folder() == folder )
      return fvi;
    ++it;
  }
  return 0;
}

FolderViewItem * FolderView::findItemByProperties( KPIM::FolderTreeWidgetItem::Protocol proto, KPIM::FolderTreeWidgetItem::FolderType type )
{
  QTreeWidgetItemIterator it( this );
  while( FolderViewItem * fvi = dynamic_cast<FolderViewItem *>( *it ) )
  {
    if ( ( fvi->protocol() == proto ) && ( fvi->folderType() == type ) )
      return fvi;
    ++it;
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DND Related stuff
//

void FolderView::setDropIndicatorData( FolderViewItem * moveTargetItem, FolderViewItem *sortReferenceItem, DropInsertPosition sortReferencePosition)
{
  mDnDMoveTargetItem = moveTargetItem;
  mDnDSortReferenceItem = sortReferenceItem;
  mDnDSortReferencePosition = sortReferencePosition;
}

void FolderView::paintEvent( QPaintEvent *e )
{
  FolderTreeWidget::paintEvent( e );

  if ( !( mDnDMoveTargetItem || mDnDSortReferenceItem ) )
    return;

  QPainter p( viewport() );

  if ( mDnDMoveTargetItem )
  {
    QRect r = visualItemRect( mDnDMoveTargetItem );
    if ( r.isValid() )
    {
      p.setPen( QPen( palette().brush( QPalette::Text ), 3 ) );
      p.drawRect( r );
    }
  }

  if ( mDnDSortReferenceItem )
  {
    QRect r = visualItemRect( mDnDSortReferenceItem );
    if ( r.isValid() )
    {
      p.setPen( QPen( palette().brush( QPalette::Text ), 3 ) );
      if ( mDnDSortReferencePosition == AboveReference )
        p.drawLine( r.left(), r.top(), r.right(), r.top() );
      else
        p.drawLine( r.left(), r.bottom(), r.right(), r.bottom() );
    }
  }
}


FolderView::DragMode FolderView::dragMode( bool alwaysAsk, bool canMove )
{
  DragMode action = DragCancel;
  int keybstate = kapp->keyboardModifiers();
  if ( keybstate & Qt::CTRL )
  {
    action = DragCopy;
  } else if ( keybstate & Qt::SHIFT )
  {
    action = DragMove;
  } else {
    // FIXME: anybody sets this option to false ?
    if ( GlobalSettings::self()->showPopupAfterDnD() || alwaysAsk )
    {
      KMenu menu;
      QAction *moveAction = menu.addAction( KIcon( "go-jump"), i18n( "&Move Here" ) );
      if ( ! canMove )
        moveAction->setEnabled( false ); // keep it there, but disable
      QAction *copyAction = menu.addAction( KIcon( "edit-copy" ), i18n( "&Copy Here" ) );
      menu.addSeparator();
      menu.addAction( KIcon( "dialog-cancel" ), i18n( "C&ancel" ) );

      QAction *menuChoice = menu.exec( QCursor::pos() );
      if ( menuChoice == moveAction )
        action = canMove ? DragMove : DragCopy;
      else if ( menuChoice == copyAction )
        action = DragCopy;
      else
        action = DragCancel;
    } else
      action = canMove ? DragMove : DragCopy;
  }
  return action;
}

void FolderView::buildDragItemList( QTreeWidgetItem *item, QList< FolderViewItem * > &lDragItemList )
{
  if ( !item )
    return;

  if ( item->isSelected() )
  {
    if ( item != invisibleRootItem() ) // sanity check
    {
      lDragItemList.append( static_cast< FolderViewItem * >( item ) );
      return;
    }
  }

  int cc = item->childCount();
  int i = 0;
  while ( i < cc )
  {
    QTreeWidgetItem * c = item->child( i );
    if ( c )
      buildDragItemList( c, lDragItemList );
    i++;
  }
}


void FolderView::startDrag( Qt::DropActions )
{
  // We don't use selectedFolders() here since:
  // - We want a coherent set of dragged folders: if dragging a parent
  //   don't drag the children (since the children are contained in parent)
  // - We eventually want to start drag also for items without a folder
  //   (so they can be sorted by dnd)

  QList< FolderViewItem * > lDraggedItems;
  buildDragItemList( invisibleRootItem(), lDraggedItems );

  if ( lDraggedItems.isEmpty() )
    return; // nothing to drag

  // By the way, unselect the items that aren't really dragged
  setUpdatesEnabled( false );
  clearSelection();

  QList< QPointer< KMFolder > > lDraggedFolders;
  for ( QList< FolderViewItem * >::Iterator it = lDraggedItems.begin(); it != lDraggedItems.end(); ++it )
  {
    if ( ( *it )->folder() )
      lDraggedFolders.append( ( *it )->folder() );
    ( *it )->setSelected( true );
  }

  setUpdatesEnabled( true );

  viewport()->update();

  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData();
  QByteArray dummy( 1, 'x' );

  mimeData->setData( DraggedFolderList::mimeDataType(), dummy );

  drag->setMimeData( mimeData );
  drag->setPixmap( SmallIcon( "folder" ) );
  // We use a trick to force Qt to display two different cursors depending
  // on our actions (even if they are not actually copy or move)
  drag->setDragCursor( QCursor( Qt::SplitVCursor ).pixmap(), Qt::MoveAction );


  DraggedFolderList::set( lDraggedFolders );

  drag->exec( Qt::CopyAction | Qt::MoveAction );

  DraggedFolderList::clear();
}


QStringList FolderView::mimeTypes() const
{
  QStringList ret;
  ret.append( DraggedFolderList::mimeDataType() );
  ret.append( KPIM::MailList::mimeDataType() );
  return ret;
}

Qt::DropActions FolderView::supportedDropActions() const
{
  return Qt::MoveAction | Qt::CopyAction;
}

void FolderView::dragEnterEvent( QDragEnterEvent *e )
{
  if (
      e->mimeData()->hasFormat( DraggedFolderList::mimeDataType() ) ||
      e->mimeData()->hasFormat( KPIM::MailList::mimeDataType() )
    )
  {
    // show all the items (we might want to drop anywhere)
    QTreeWidgetItemIterator it( this );
    while ( *it )
    {
      if ( ( *it )->isHidden() )
        ( *it )->setHidden( false );
      ++it;
    }

    e->acceptProposedAction();
  } else
    e->ignore();
}



bool FolderView::acceptDropCopyOrMoveMessages( FolderViewItem *item )
{
  if ( !item )
    return false; // obviously

  if ( item->protocol() == FolderViewItem::Search )
    return false; // nothing can be dragged into search folders

  KMFolder *fld = item->folder();

  if ( !fld )
    return false; // no folder to drop into

  if (
       fld->moveInProgress() ||
       fld->isReadOnly() ||
       (fld->noContent() && item->childCount() == 0) ||
       (fld->noContent() && item->isExpanded())
    )
    return false; // content can't be added to folder

  return true;
}

void FolderView::handleMessagesDropEvent( QDropEvent *e )
{
  FolderViewItem * item = static_cast<FolderViewItem *>( itemAt( e->pos() ) );
  if ( !item )
    return;

  if ( !acceptDropCopyOrMoveMessages( item ) )
    return; // hum.. can't accept it

  int action;
  if ( mainWidget()->folder() && !mainWidget()->folder()->canDeleteMessages() )
    action = DragCopy;
  else
    action = dragMode();

  e->setDropAction( Qt::CopyAction );
  e->accept();

  // KMHeaders does copy/move itself
  if ( action == DragCancel )
     return;
  else if ( action == DragMove && item->folder() )
     emit folderDrop( item->folder() );
  else if ( action == DragCopy && item->folder() )
     emit folderDropCopy( item->folder() );
  else
     handleMailListDrop( e, item->folder() );
}

void FolderView::handleMessagesDragMoveEvent( QDragMoveEvent *e )
{
  FolderViewItem * item = static_cast<FolderViewItem *>( itemAt( e->pos() ) );

  // dragging messages
  if ( item && acceptDropCopyOrMoveMessages( item ) )
  {
    e->accept( visualItemRect( item ) );
    KPIM::BroadcastStatus::instance()->setStatusMsg( i18n("Copy or Move Messages to %1", item->labelText() ) );
    setDropIndicatorData( item, 0 );
  } else {
    e->ignore( visualItemRect( item ) );
    KPIM::BroadcastStatus::instance()->setStatusMsg( "" );
    setDropIndicatorData( 0, 0 );
  }
}

void FolderView::handleFoldersDragMoveEvent( QDragMoveEvent *e )
{
  e->ignore();
}

void FolderView::handleFoldersDropEvent( QDropEvent *e )
{
  e->ignore();
}

void FolderView::dragMoveEvent( QDragMoveEvent *e )
{
  // Something is actually being dragged over our widget

  // First of all call the base class implementation in order to
  // handle scrolling and item expanding
  FolderTreeWidget::dragMoveEvent( e );

  if ( e->mimeData()->hasFormat( DraggedFolderList::mimeDataType() ) )
    handleFoldersDragMoveEvent( e );
  else if ( e->mimeData()->hasFormat( KPIM::MailList::mimeDataType() ) )
    handleMessagesDragMoveEvent( e );
  else
    e->ignore();

  viewport()->update(); // trigger a repaint
}

void FolderView::dropEvent( QDropEvent *e )
{
  if ( e->mimeData()->hasFormat( DraggedFolderList::mimeDataType() ) )
    handleFoldersDropEvent( e );
  else if ( e->mimeData()->hasFormat( KPIM::MailList::mimeDataType() ) )
    handleMessagesDropEvent( e );
  else e->ignore(); // ignore by default

  mDnDMoveTargetItem = 0;
  mDnDSortReferenceItem = 0;
  viewport()->update(); // trigger a repaint
}

void FolderView::dragLeaveEvent( QDragLeaveEvent * )
{
  mDnDMoveTargetItem = 0;
  mDnDSortReferenceItem = 0;
  viewport()->update(); // trigger a repaint
}

void FolderView::slotFolderExpanded( QTreeWidgetItem *item )
{
  FolderViewItem * fvi = static_cast<FolderViewItem *>( item );
  if ( !fvi || !fvi->folder() || !fvi->folder()->storage() )
    return;

  fvi->setDataSize( fvi->folder()->storage()->folderSize() );

  if( fvi->folder()->folderType() != KMFolderTypeImap )
    return; // nothing more to do

  KMFolderImap *folder = static_cast<KMFolderImap*>( fvi->folder()->storage() );

  // if we should list all folders we limit this to the root folder
  if (
      !folder->account() ||
      (
        ( !folder->account()->listOnlyOpenFolders() ) &&
        static_cast<QTreeWidgetItem *>( fvi )->parent()
      )
    )
    return;

  if ( folder->getSubfolderState() != KMFolderImap::imapNoInformation )
    return;

  // the tree will be reloaded after that
  bool success = folder->listDirectory();
  if (!success)
    fvi->setExpanded( false ); // failed to list the directory

  if ( ( fvi->childCount() == 0 ) && static_cast<QTreeWidgetItem *>( fvi )->parent() )
    fvi->setChildIndicatorPolicy( QTreeWidgetItem::DontShowIndicatorWhenChildless ); // no children after listing
}

void FolderView::slotFolderCollapsed( QTreeWidgetItem* item )
{
  FolderViewItem * fvi = static_cast<FolderViewItem *>( item );
  if ( !fvi )
    return; // sanity check

  resetFolderList( fvi, false );
}

void FolderView::createFolderList( QStringList *folderNames, QList< QPointer< KMFolder > > *folders, int flags )
{
  // Clear the buffers unless explicitly forbidden
  if ( !( flags & AppendListing ) )
  {
    if ( folderNames )
      folderNames->clear();
    if ( folders )
      folders->clear();
  }

  // iterate
  for ( QTreeWidgetItemIterator it( this ); *it; ++it )
  {
    FolderViewItem *fvi = static_cast< FolderViewItem * >( *it );

    if ( !fvi->folder() )
      continue;

    switch( fvi->folder()->folderType() )
    {
      case KMFolderTypeImap:
        if( !( flags & IncludeImapFolders ) )
          continue;
      break;
      case KMFolderTypeCachedImap:
        if( !( flags & IncludeCachedImapFolders ) )
          continue;
      break;
      case KMFolderTypeMbox:
        if( !( flags & IncludeLocalFolders ) )
          continue;
      break;
      case KMFolderTypeMaildir:
        if( !( flags & IncludeLocalFolders ) )
          continue;
      break;
      case KMFolderTypeSearch:
        if( !( flags & IncludeSearchFolders ) )
          continue;
      break;
      case KMFolderTypeUnknown:
        if( !( flags & IncludeUnknownFolders ) )
          continue;
      break;
    }
    if ( flags & SkipFoldersWithNoContent )
    {
      if ( fvi->folder()->noContent() )
        continue;
    }
    if ( flags & SkipFoldersWithNoChildren )
    {
      if ( fvi->folder()->noChildren() )
        continue;
    }

    if ( folderNames )
    {
      if ( flags & IncludeFolderIndentation )
      {
        // compute depths
        int depth = 0;
        QTreeWidgetItem *father = ((QTreeWidgetItem *)fvi)->parent();
        while ( father )
        {
          father = father->parent();
          depth++;
        }
        QString prefix;
        prefix.fill( ' ', 2 * depth );
        folderNames->append( prefix + fvi->labelText() );
      } else {
        folderNames->append( fvi->labelText() );
      }
    }

    if ( folders )
      folders->append(fvi->folder());
  }
}

void FolderView::resetFolderList( FolderViewItem *item, bool startList )
{
  if ( !item )
    item = static_cast< FolderViewItem * >( currentItem() );
  if ( !item )
    return;
  if ( !item->folder() )
    return;
  if ( item->folder()->folderType() != KMFolderTypeImap )
    return;

  KMFolderImap *folder = static_cast< KMFolderImap * >( item->folder()->storage() );
  folder->setSubfolderState( KMFolderImap::imapNoInformation );
  if ( startList )
    folder->listDirectory();
}

void FolderView::slotResetFolderList()
{
  resetFolderList( 0, false );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FolderViewItem
//

FolderViewItem::FolderViewItem(
      FolderView *parent,
      const QString & name,
      KMFolder *folder,
      FolderViewItem::Protocol protocol,
      FolderViewItem::FolderType type
    )
  : QObject( 0 ),
    KPIM::FolderTreeWidgetItem( parent, name, protocol, type ),
    mFolder( folder ), mFlags( 0 )
{
  setObjectName( name );
  setIcon( FolderView::LabelColumn, KIcon( normalIcon() ) );
}

FolderViewItem::FolderViewItem(
      FolderViewItem *parent,
      const QString & name,
      KMFolder *folder,
      FolderViewItem::Protocol protocol,
      FolderViewItem::FolderType type
    )
  : QObject( 0 ),
    KPIM::FolderTreeWidgetItem( parent, name, protocol, type ),
    mFolder( folder ), mFlags( 0 )
{
  setObjectName( name );
  setIcon( FolderView::LabelColumn, KIcon( normalIcon() ) );
}

QString FolderViewItem::normalIcon() const
{
  QString icon;

  KMFolder * fld = folder();

  // Roots
  if ( folderType() == Root )
  {
    switch ( protocol() )
    {
      case Imap:
      case CachedImap:
      case News:
        icon = "network-server";
      break;
      case Search:
        icon = "system-search";
      break;
      default:
        icon = "folder";
      break;
    }
  } else {
    // special folders
    switch ( folderType() )
    {
      case Inbox:
        icon = "mail-folder-inbox";
      break;
      case Outbox:
        icon = "mail-folder-outbox";
      break;
      case SentMail:
        icon = "mail-folder-sent";
      break;
      case Trash:
        icon = "user-trash";
      break;
      case Drafts:
        icon = "document-properties";
      break;
      case Templates:
        icon = "document-new";
      break;
      default:
      {
        //If not a resource folder don't try to use icalIface folder pixmap
        if ( kmkernel->iCalIface().isResourceFolder( mFolder ) ) {
          icon = kmkernel->iCalIface().folderPixmap( folderType() );
        }
      }
      break;
    }

    // non-root search folders
    if ( protocol() == Search )
      icon = "edit-find-mail";

    // empty folders with no associated content
    if ( fld && fld->noContent() )
      icon = "folder-grey";
  }

  // allow icon customization by the folder itself
  if ( fld && fld->useCustomIcons() )
    icon = fld->normalIconPath();

  // fallback
  if ( icon.isEmpty() )
    icon = "folder";

  return icon;
}

QString FolderViewItem::unreadIcon() const
{
  KMFolder * fld = folder();

  QString icon;

  if (
      !fld || ( folderType() == Root ) || fld->isSystemFolder() ||
      kmkernel->folderIsTrash( fld ) || kmkernel->folderIsTemplates( fld ) ||
      kmkernel->folderIsDraftOrOutbox( fld )
    )
    icon = normalIcon();

  if ( fld && fld->useCustomIcons() )
  {
    icon = fld->unreadIconPath();
    if ( icon.isEmpty() )
      icon = fld->normalIconPath();
  }

  if ( icon.isEmpty() )
  {
    if ( fld && fld->noContent() )
      icon = "folder-grey";
    else {
      //If not a resource folder don't try to use icalIface folder pixmap
      if( kmkernel->iCalIface().isResourceFolder( mFolder ) ) {
        icon = kmkernel->iCalIface().folderPixmap( folderType() );
      }
      if ( icon.isEmpty() ) {
         icon = "folder-open";
      }
    }
  }

  return icon;
}

QString FolderViewItem::configGroupName() const
{
  KMFolder * fld = folder();

  if ( fld )
    return fld->configGroupName();

  if ( folderType() != Root )
    return QString();

  if ( protocol() == Local ) // local root item
    return "Folder_local_root"; // FIXME: Why we're using a different prefix here ? Folder-Virtual-Local-Root wouldn't be nice ?

  if ( protocol() == Search )
    return "Folder_search";     // FIXME: Why we're using a different prefix here ? Folder-Virtual-Search wouldn't be nice ?

  return QString();
}

void FolderViewItem::slotIconsChanged()
{
  // This slot is connected to the folder()'s iconsChanged() signal.
  KMFolder *fld = folder();

  if ( !fld )
    return; // should never happen.. but well

  KPIM::FolderTreeWidgetItem::FolderType newType = folderType();
  if ( kmkernel->iCalIface().isResourceFolder( fld ) )
    newType = kmkernel->iCalIface().folderType( fld );

  // reload the folder tree if the type changed, needed because of the
  // various type-dependent folder hiding options
  if ( folderType() != newType )
    static_cast<FolderView*>( treeWidget() )->slotDelayedReload();
  setFolderType( newType );

  if ( unreadCount() > 0 )
    setIcon( FolderView::LabelColumn, KIcon( unreadIcon() ) );
  else
    setIcon( FolderView::LabelColumn, KIcon( normalIcon() ) );

  emit iconChanged( this );
}

void FolderViewItem::slotNameChanged()
{
  // This slot is connected to the folder()'s nameChanged() signal.
  KMFolder *fld = folder();

  if ( !fld )
    return; // should never happen.. but well

  setLabelText( fld->label() );

  emit nameChanged( this );
}

void FolderViewItem::slotNoContentChanged()
{
  // reload the folder tree if the no content state changed, needed because
  // we hide no-content folders if their child nodes are hidden
  QTimer::singleShot( 0, static_cast<FolderView*>( treeWidget() ),
                      SLOT( slotReload() ) );
}

void FolderViewItem::slotFolderCountsChanged( KMFolder * )
{
  // This slot is connected to several folder() signals related
  // to unread, total and size count changes.
  // We mark this item as "counts dirty" and trigger
  // a delayed update in the tree widget.

  if ( mFlags & CountsDirty )
    return; // already marked as dirty (this means that an update in the view is pending)
  mFlags |= CountsDirty;

  FolderView * tree = dynamic_cast< FolderView *>( treeWidget() );
  if ( !tree )
    return; // hum.. should never happen
  tree->triggerLazyUpdateCounts();
}


bool FolderViewItem::updateCounts()
{
  // This is usually called by the FolderView in response
  // to a delayed trigger (see slotFolderCountsChanged())
  bool openFolder = mFlags & OpenFolderForCountUpdate;

  // Clear dirty mark
  mFlags &= ~( CountsDirty | OpenFolderForCountUpdate );

  // sanity check
  FolderView * tree = dynamic_cast< FolderView *>( treeWidget() );
  if ( !tree )
    return false; // hum.. should never happen

  KMFolder * fld = folder();

  if ( fld && openFolder )
  {
    if ( fld->isOpened() )
      openFolder = false; // already open, no need to close it
    else
      fld->open( "updatecount" ); // shouldn't we check the error code ?
  }

  // We will force text updates if the children counts have changed
  bool someCountsChanged = false;

  if ( childCount() > 0 )
    someCountsChanged = updateChildrenCounts();

  if ( fld && !fld->noContent() )
  {
    int count = fld->countUnread();
    if ( someCountsChanged || ( count != unreadCount() ) )
    {
      someCountsChanged = true;
      adjustUnreadCount( count );
    }

    if ( tree->totalColumnVisible() )
    {
      // get the cached count if the folder is not open
      count = fld->count( !fld->isOpened() );
      if ( someCountsChanged || ( count != totalCount() ) )
      {
        someCountsChanged = true;
        setTotalCount( count );
      }
    }

    if ( tree->dataSizeColumnVisible() )
    {
      qint64 size = fld->storage()->folderSize();
      if ( someCountsChanged || ( size != dataSize() ) )
      {
        someCountsChanged = true;
        setDataSize( size );
      }
    }

    if ( fld && ( isCloseToQuota() != fld->storage()->isCloseToQuota() ) )
        setIsCloseToQuota( fld->storage()->isCloseToQuota() );

    if ( openFolder ) // we opened it
      fld->close( "updatecount" );

  } else {

    if ( someCountsChanged || ( unreadCount() != 0 ) )
    {
      someCountsChanged = true;
      adjustUnreadCount( 0 );
    }
    if ( ( someCountsChanged || ( totalCount() != 0 ) ) && tree->totalColumnVisible() )
    {
      someCountsChanged = true;
      setTotalCount( 0 );
    }
    if ( ( someCountsChanged || ( dataSize() != -1 ) ) && tree->dataSizeColumnVisible() )
    {
      someCountsChanged = true;
      setDataSize( -1 );
    }
    if ( isCloseToQuota() )
      setIsCloseToQuota( false );
  }

  // tell the kernel that one of the counts has changed
  kmkernel->messageCountChanged(); // FIXME: does this really need to be triggered from here ?

  return someCountsChanged;
}

void FolderViewItem::adjustUnreadCount( int newUnreadCount )
{
  int oldUnreadCount = unreadCount();

  // adjust the icons if the folder is now newly unread or
  // now newly not-unread
  if ( newUnreadCount != 0 && oldUnreadCount == 0 )
    setIcon( FolderView::LabelColumn, KIcon( unreadIcon() ) );
  if ( oldUnreadCount != 0 && newUnreadCount == 0 )
    setIcon( FolderView::LabelColumn, KIcon( normalIcon() ) );

  setUnreadCount( newUnreadCount );
}

void FolderViewItem::slotShowExpiryProperties()
{
  if ( !folder() )
    return;

  FolderView * fv = dynamic_cast< FolderView * >( treeWidget() );
  if ( !fv )
    return;

  MainFolderView *mfv = fv->mainWidget()->mainFolderView();
  if ( !mfv )
    return;

  KMail::ExpiryPropertiesDialog *dlg = new KMail::ExpiryPropertiesDialog( mfv, folder() );
  dlg->show();
  //fv->mainWidget()->slotModifyFolder( KMMainWidget::PropsExpire );
}

bool FolderViewItem::operator < ( const QTreeWidgetItem &other ) const
{
  FolderView * w = static_cast< FolderView * >( treeWidget() );
  const FolderViewItem * oitem = static_cast< const FolderViewItem * >( &other );

  if ( w->sortingPolicy() == FolderView::SortByDragAndDropKey )
    return mDnDSortingKey < oitem->dndSortingKey();

  return FolderTreeWidgetItem::operator < ( other );
}


KPIM::FolderTreeWidgetItem::Protocol FolderViewItem::protocolByFolder( KMFolder *fld )
{
  if ( !fld )
    return NONE; // sanity check

  switch ( fld->folderType() )
  {
    case KMFolderTypeImap:
      return Imap;
    case KMFolderTypeCachedImap:
      return CachedImap;
    case KMFolderTypeMbox:
    case KMFolderTypeMaildir:
      return Local;
    case KMFolderTypeSearch:
      return Search;
    default: // make gcc happy
      break;
  }

  return NONE;
}

KPIM::FolderTreeWidgetItem::FolderType FolderViewItem::folderTypeByFolder( KMFolder *fld )
{
  if ( !fld )
    return Other; // sanity check

  if ( fld == kmkernel->inboxFolder() )
    return Inbox;

  if ( kmkernel->folderIsDraftOrOutbox( fld ) )
    return ( fld == kmkernel->outboxFolder() ) ? Outbox : Drafts;

  if ( kmkernel->folderIsSentMailFolder( fld ) )
    return SentMail;

  if ( kmkernel->folderIsTrash( fld ) )
    return Trash;

  if ( kmkernel->folderIsTemplates( fld ) )
    return Templates;

  if( kmkernel->iCalIface().isResourceFolder( fld ) )
    return kmkernel->iCalIface().folderType( fld );

  // System folders on dimap or imap which are not resource folders are
  // inboxes. Urgs.
  if (
       fld->isSystemFolder() &&
       !kmkernel->iCalIface().isResourceFolder( fld ) &&
       ( fld->folderType() == KMFolderTypeImap || fld->folderType() == KMFolderTypeCachedImap )
    )
    return Inbox;

  return Other; // dunno what is this
}

} // namespace KMail


#include "folderview.moc"
