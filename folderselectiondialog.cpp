/*
  KMail Folder Selection Dialog

  Copyright (c) 1997-1998 Stefan Taferner <taferner@kde.org>
  Copyright (c) 2004-2005 Carsten Burghardt <burghardt@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "folderselectiondialog.h"
#include "kmfoldertree.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "globalsettings.h"

#include <kdebug.h>
#include <klineedit.h>
#include <kmenu.h>
#include <kiconloader.h>
#include <kvbox.h>
#include <kconfiggroup.h>

#include <QLayout>
#include <QToolButton>

namespace KMail {

class FolderItem : public KFolderTreeItem
{
  public:
    FolderItem( KFolderTree * listView );
    FolderItem( KFolderTreeItem * listViewItem );

    void setFolder( KMFolder * folder ) { mFolder = folder; }
    const KMFolder * folder() { return mFolder; }

  private:
    KMFolder * mFolder;
};

//-----------------------------------------------------------------------------
FolderItem::FolderItem( KFolderTree * listView )
  : KFolderTreeItem( listView ),
    mFolder( 0 )
{}

//-----------------------------------------------------------------------------
FolderItem::FolderItem( KFolderTreeItem * listViewItem )
  : KFolderTreeItem( listViewItem ),
    mFolder( 0 )
{}

//-----------------------------------------------------------------------------
SimpleFolderTree::SimpleFolderTree( QWidget * parent,
                                    KMFolderTree * folderTree,
                                    const QString & preSelection,
                                    bool mustBeReadWrite )
  : KFolderTree( parent ), mFolderTree( folderTree )
{
  setSelectionModeExt( Single );
  //TODO: Enable this again once KFolderTree is ported to QTreeWidget
  //setAlternateBackground( QColor( 0xf0, 0xf0, 0xf0 ) );
  mFolderColumn = addColumn( i18n( "Folder" ), 0 );
  mPathColumn = addColumn( i18n( "Path" ) );

  reload( mustBeReadWrite, true, true, preSelection );
  readColorConfig();

  applyFilter( "" );

  connect( this, SIGNAL( contextMenuRequested( Q3ListViewItem*, const QPoint &, int ) ),
           this, SLOT( slotContextMenuRequested( Q3ListViewItem*, const QPoint & ) ) );
}

//-----------------------------------------------------------------------------
void SimpleFolderTree::reload( bool mustBeReadWrite, bool showOutbox,
                               bool showImapFolders, const QString& preSelection )
{
  mLastMustBeReadWrite = mustBeReadWrite;
  mLastShowOutbox = showOutbox;
  mLastShowImapFolders = showImapFolders;

  clear();
  FolderItem * lastItem = 0;
  FolderItem * lastTopItem = 0;
  FolderItem * selectedItem = 0;
  int lastDepth = 0;

  QString selected = preSelection;
  if ( selected.isEmpty() && folder() )
    selected = folder()->idString();

  mFilter = "";
  QString path;

  for ( Q3ListViewItemIterator it( mFolderTree ) ; it.current() ; ++it )
  {
    KMFolderTreeItem * fti = static_cast<KMFolderTreeItem *>( it.current() );

    // search folders are never shown
    if ( !fti || fti->protocol() == KFolderTreeItem::Search )
      continue;

    // imap folders?
    if ( fti->protocol() == KFolderTreeItem::Imap && !showImapFolders )
      continue;

    // the outbox?
    if ( fti->type() == KFolderTreeItem::Outbox && !showOutbox )
      continue;

    int depth = fti->depth();// - 1;
    FolderItem * item = 0;
    if ( depth <= 0 ) {
      // top level - first top level item or after last existing top level item
      item = new FolderItem( this );
      if ( lastTopItem )
        item->moveItem( lastTopItem );
      lastTopItem = item;
      depth = 0;
      path = "";
    }
    else {
      if ( depth > lastDepth ) {
        // next lower level - parent node will get opened
        item = new FolderItem( lastItem );
        lastItem->setOpen(true);
      }
      else {
        path = path.section( '/', 0, -2 - (lastDepth-depth) );

        if ( depth == lastDepth ) {
          // same level - behind previous item
          item = new FolderItem( static_cast<FolderItem*>(lastItem->parent()) );
          item->moveItem( lastItem );
        } else if ( depth < lastDepth ) {
          // above previous level - might be more than one level difference
          // but highest possibility is top level
          while ( ( depth <= --lastDepth ) && lastItem->parent() ) {
            lastItem = static_cast<FolderItem *>( lastItem->parent() );
          }
          if ( lastItem->parent() ) {
            item = new FolderItem( static_cast<FolderItem*>(lastItem->parent()) );
            item->moveItem( lastItem );
          } else {
            // chain somehow broken - what does cause this ???
            kDebug( 5006 ) <<"You shouldn't get here: depth=" << depth
                            << "folder name=" << fti->text( 0 );
            item = new FolderItem( this );
            lastTopItem = item;
          }
        }
      }
    }

    if ( depth > 0 )
      path += '/';
    path += fti->text( 0 );

    item->setText( mFolderColumn, fti->text( 0 ) );
    item->setText( mPathColumn, path );
    item->setProtocol( fti->protocol() );
    item->setType( fti->type() );

    // Make items without folders and readonly items unselectable
    // if we're told so
    if ( mustBeReadWrite && ( !fti->folder() || fti->folder()->isReadOnly() ) ) {
      item->setSelectable( false );
    } else {
      if ( fti->folder() ) {
        item->setFolder( fti->folder() );
        if ( selected == item->folder()->idString() )
          selectedItem = item;
      }
    }
    lastItem = item;
    lastDepth = depth;
  }

  if ( selectedItem ) {
    setSelected( selectedItem, true );
    ensureItemVisible( selectedItem );
  }
}

//-----------------------------------------------------------------------------
const KMFolder * SimpleFolderTree::folder() const
{
  Q3ListViewItem * item = currentItem();
  if ( item ) {
    const KMFolder * folder = static_cast<FolderItem *>( item )->folder();
    if( folder ) return folder;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void SimpleFolderTree::setFolder( KMFolder *folder )
{
  for ( Q3ListViewItemIterator it( this ) ; it.current() ; ++it )
  {
    const KMFolder *fld = static_cast<FolderItem *>( it.current() )->folder();
    if ( fld == folder )
    {
      setSelected( it.current(), true );
      ensureItemVisible( it.current() );
    }
  }
}

//-----------------------------------------------------------------------------
void SimpleFolderTree::setFolder( const QString& idString )
{
  setFolder( kmkernel->findFolderById( idString ) );
}

//-----------------------------------------------------------------------------
void SimpleFolderTree::addChildFolder()
{
  const KMFolder *fld = folder();
  if ( fld ) {
    mFolderTree->addChildFolder( (KMFolder *) fld, parentWidget() );
    reload( mLastMustBeReadWrite, mLastShowOutbox, mLastShowImapFolders );
    setFolder( (KMFolder *) fld );
  }
}

//-----------------------------------------------------------------------------
void SimpleFolderTree::slotContextMenuRequested( Q3ListViewItem *lvi,
                                                 const QPoint &p )
{
  if (!lvi)
    return;
  setCurrentItem( lvi );
  setSelected( lvi, true );

  const KMFolder * folder = static_cast<FolderItem *>( lvi )->folder();
  if ( !folder || folder->noContent() || folder->noChildren() )
    return;

  KMenu *folderMenu = new KMenu;
  folderMenu->addTitle( folder->label() );
  folderMenu->addSeparator();
  folderMenu->addAction( KIcon("folder-new"),
                         i18n("&New Subfolder..."), this,
                         SLOT(addChildFolder()) );
  kmkernel->setContextMenuShown( true );
  folderMenu->exec (p, 0);
  kmkernel->setContextMenuShown( false );
  delete folderMenu;
  folderMenu = 0;
}

//-----------------------------------------------------------------------------
void SimpleFolderTree::readColorConfig (void)
{
  QColor c1=QColor(qApp->palette().color( QPalette::Text ));
  QColor c2=QColor(qApp->palette().color( QPalette::Base ));

  mPaintInfo.colFore = c1;
  mPaintInfo.colBack = c2;

  QPalette newPal = qApp->palette();
  newPal.setColor( QPalette::Base, mPaintInfo.colBack );
  newPal.setColor( QPalette::Text, mPaintInfo.colFore );
  setPalette( newPal );
}

static int recurseFilter( Q3ListViewItem * item, const QString& filter, int column )
{
  if ( item == 0 )
    return 0;

  Q3ListViewItem * child;
  child = item->firstChild();

  int enabled = 0;
  while ( child ) {
    enabled += recurseFilter( child, filter, column );
    child = child->nextSibling();
  }

  if ( filter.length() == 0 ||
       item->text( column ).contains( filter, Qt::CaseInsensitive ) ) {
    item->setVisible( true );
    ++enabled;
  }
  else {
    item->setVisible( enabled );
    item->setEnabled( false );
  }

  return enabled;
}

void SimpleFolderTree::applyFilter( const QString& filter )
{
  // Reset all items to visible, enabled, and open
  Q3ListViewItemIterator clean( this );
  while ( clean.current() ) {
    Q3ListViewItem * item = clean.current();
    item->setEnabled( true );
    item->setVisible( true );
    item->setOpen( true );
    ++clean;
  }

  mFilter = filter;

  if ( filter.isEmpty() ) {
    setColumnText( mPathColumn, i18n("Path") );
    return;
  }

  // Set the visibility and enabled status of each list item.
  // The recursive algorithm is necessary because visiblity
  // changes are automatically applied to child nodes by Qt.
  Q3ListViewItemIterator it( this );
  while ( it.current() ) {
    Q3ListViewItem * item = it.current();
    if ( item->depth() <= 0 )
      recurseFilter( item, filter, mPathColumn );
    ++it;
  }

  // Iterate through the list to find the first selectable item
  Q3ListViewItemIterator first ( this );
  while ( first.current() ) {
    FolderItem * item = static_cast< FolderItem* >( first.current() );

    if ( item->isVisible() && item->isSelectable() ) {
      setSelected( item, true );
      ensureItemVisible( item );
      break;
    }

    ++first;
  }

  // Display and save the current filter
  if ( filter.length() > 0 )
    setColumnText( mPathColumn, i18n("Path") + "  ( " + filter + " )" );
  else
    setColumnText( mPathColumn, i18n("Path") );

  mFilter = filter;
}

//-----------------------------------------------------------------------------
void SimpleFolderTree::keyPressEvent( QKeyEvent *e ) {
  QString s = e->text();
  int ch = s.isEmpty() ? 0 : s[0].toAscii();

  if ( ch >= 32 && ch <= 126 )
    applyFilter( mFilter + ch );
  else if ( ch == 8 || ch == 127 ) {
    if ( mFilter.length() > 0 ) {
      mFilter.truncate( mFilter.length()-1 );
      applyFilter( mFilter );
    }
  }

  else
    K3ListView::keyPressEvent( e );
}


//-----------------------------------------------------------------------------
FolderSelectionDialog::FolderSelectionDialog( KMMainWidget * parent, const QString& caption,
    bool mustBeReadWrite, bool useGlobalSettings )
  : KDialog( parent ), // mainwin as parent, modal
    mUseGlobalSettings( useGlobalSettings )
{
  setCaption( caption );
  setButtons( Ok|Cancel|User1 );
  setObjectName( "folder dialog" );
  setButtonGuiItem( User1, KGuiItem(i18n("&New Subfolder..."), "folder-new",
         i18n("Create a new subfolder under the currently selected folder")) );
  KMFolderTree * ft = parent->folderTree();
  assert( ft );

  QString preSelection = mUseGlobalSettings ?
    GlobalSettings::self()->lastSelectedFolder() : QString();
  QWidget *vbox = new KVBox( this );
  setMainWidget( vbox );
  mTreeView = new KMail::SimpleFolderTree( vbox, ft,
                                           preSelection, mustBeReadWrite );
  init();
}

//----------------------------------------------------------------------------
FolderSelectionDialog::FolderSelectionDialog( QWidget * parent, KMFolderTree * tree,
    const QString& caption, bool mustBeReadWrite, bool useGlobalSettings )
  : KDialog( parent ), // mainwin as parent, modal
    mUseGlobalSettings( useGlobalSettings )
{
  setCaption( caption );
  setButtons( Ok|Cancel|User1 );
  setObjectName( "folder dialog" );
  setButtonGuiItem( User1, KGuiItem(i18n("&New Subfolder..."), "folder-new",
         i18n("Create a new subfolder under the currently selected folder") ) );
  QString preSelection = mUseGlobalSettings ?
    GlobalSettings::self()->lastSelectedFolder() : QString();
  QWidget *vbox = new KVBox( this );
  setMainWidget( vbox );
  mTreeView = new KMail::SimpleFolderTree( vbox, tree,
                                           preSelection, mustBeReadWrite );
  init();
}

//-----------------------------------------------------------------------------
void FolderSelectionDialog::init()
{
  mTreeView->setFocus();
  connect( mTreeView, SIGNAL( doubleClicked( Q3ListViewItem*, const QPoint&, int ) ),
           this, SLOT( slotSelect() ) );
  connect( mTreeView, SIGNAL( selectionChanged() ),
           this, SLOT( slotUpdateBtnStatus() ) );
  connect(this, SIGNAL(user1Clicked()),this,SLOT(slotUser1()));
  readConfig();
}

//-----------------------------------------------------------------------------
FolderSelectionDialog::~FolderSelectionDialog()
{
  const KMFolder * cur = folder();
  if ( cur && mUseGlobalSettings ) {
    GlobalSettings::self()->setLastSelectedFolder( cur->idString() );
  }

  writeConfig();
}


//-----------------------------------------------------------------------------
KMFolder * FolderSelectionDialog::folder( void )
{
  return ( KMFolder * ) mTreeView->folder();
}

//-----------------------------------------------------------------------------
void FolderSelectionDialog::setFolder( KMFolder* folder )
{
  mTreeView->setFolder( folder );
}

//-----------------------------------------------------------------------------
void FolderSelectionDialog::slotSelect()
{
  accept();
}

//-----------------------------------------------------------------------------
void FolderSelectionDialog::slotUser1()
{
  mTreeView->addChildFolder();
}

//-----------------------------------------------------------------------------
void FolderSelectionDialog::slotUpdateBtnStatus()
{
  enableButton( User1, folder() &&
                ( !folder()->noContent() && !folder()->noChildren() ) );
}

//-----------------------------------------------------------------------------
void FolderSelectionDialog::setFlags( bool mustBeReadWrite, bool showOutbox,
                               bool showImapFolders )
{
  mTreeView->reload( mustBeReadWrite, showOutbox, showImapFolders );
}

void FolderSelectionDialog::readConfig()
{
  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group( config, "FolderSelectionDialog" );
  QSize size = group.readEntry( "Size", QSize() );
  if ( !size.isEmpty() )
    resize( size );
  else
    resize( 500, 300 );

  QList<int> widths = group.readEntry( "ColumnWidths", QList<int>() );
  if ( !widths.isEmpty() ) {
    mTreeView->setColumnWidth( mTreeView->mFolderColumn, widths[0] );
    mTreeView->setColumnWidth( mTreeView->mPathColumn,   widths[1] );
  }
  else {
    int colWidth = width() / 2;
    mTreeView->setColumnWidth( mTreeView->mFolderColumn, colWidth );
    mTreeView->setColumnWidth( mTreeView->mPathColumn,   colWidth );
  }
}

void FolderSelectionDialog::writeConfig()
{
  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group( config, "FolderSelectionDialog" );
  group.writeEntry( "Size", size() );

  QList<int> widths;
  widths.push_back( mTreeView->columnWidth( mTreeView->mFolderColumn ) );
  widths.push_back( mTreeView->columnWidth( mTreeView->mPathColumn ) );
  group.writeEntry( "ColumnWidths", widths );
}

} // namespace KMail

#include "folderselectiondialog.moc"
