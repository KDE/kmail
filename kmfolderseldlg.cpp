// kmfolderseldlg.cpp

#include <config.h>
#include "kmfolderseldlg.h"
#include "kmfoldertree.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "globalsettings.h"

#include <kdebug.h>
#include <klineedit.h>
#include <kpopupmenu.h>
#include <kiconloader.h>

#include <qlayout.h>
#include <qtoolbutton.h>

namespace KMail {

class FolderItem : public KFolderTreeItem
{
  public:
    FolderItem( KFolderTree * listView );
    FolderItem( KFolderTreeItem * listViewItem );

    void setFolder( KMFolder * folder ) { mFolder = folder; };
    const KMFolder * folder() { return mFolder; };

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
  mFolderColumn = addColumn( i18n( "Folder" ) );

  reload( mustBeReadWrite, true, true, preSelection );

  connect( this, SIGNAL( contextMenuRequested( QListViewItem*, const QPoint &, int ) ),
           this, SLOT( slotContextMenuRequested( QListViewItem*, const QPoint & ) ) );
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

  for ( QListViewItemIterator it( mFolderTree ) ; it.current() ; ++it ) 
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
    }
    else {
      if ( depth > lastDepth ) {
        // next lower level - parent node will get opened
        item = new FolderItem( lastItem );
      }
      else {
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
            kdDebug( 5006 ) << "You shouldn't get here: depth=" << depth
                            << "folder name=" << fti->text( 0 ) << endl;
            item = new FolderItem( this );
            lastTopItem = item;
          }
        }
      }
    }

    item->setText( mFolderColumn, fti->text( 0 ) );
    item->setProtocol( fti->protocol() );
    item->setType( fti->type() );
    item->setOpen( fti->isOpen() );
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
  QListViewItem * item = currentItem();
  if ( item ) {
    const KMFolder * folder = static_cast<FolderItem *>( item )->folder();
    if( folder ) return folder;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void SimpleFolderTree::setFolder( KMFolder *folder )
{
  for ( QListViewItemIterator it( this ) ; it.current() ; ++it ) 
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
void SimpleFolderTree::slotContextMenuRequested( QListViewItem *lvi,
                                                 const QPoint &p )
{
  if (!lvi)
    return;
  setCurrentItem( lvi );
  setSelected( lvi, TRUE );

  const KMFolder * folder = static_cast<FolderItem *>( lvi )->folder();
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
  folderMenu = 0;
}


//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg( KMMainWidget * parent, const QString& caption, 
    bool mustBeReadWrite, bool useGlobalSettings )
  : KDialogBase( parent, "folder dialog", true, caption,
                 Ok|Cancel|User1, Ok, true, 
                 KGuiItem(i18n("&New Subfolder"), "folder_new",
                   i18n("Create a new subfolder under the currently selected folder"))
               ), // mainwin as parent, modal
    mUseGlobalSettings( useGlobalSettings )             
{
  KMFolderTree * ft = parent->folderTree();
  assert( ft );

  QString global = mUseGlobalSettings ? 
    GlobalSettings::lastSelectedFolder() : QString::null;
  mTreeView = new KMail::SimpleFolderTree( makeVBoxMainWidget(), ft,
                                           global, mustBeReadWrite );
                                           
  mTreeView->setFocus();
  connect( mTreeView, SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
           this, SLOT( slotSelect() ) );
  connect( mTreeView, SIGNAL( selectionChanged() ),
           this, SLOT( slotUpdateBtnStatus() ) );

  readConfig();
}

//----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg( QWidget * parent, KMFolderTree * tree,
    const QString& caption, bool mustBeReadWrite, bool useGlobalSettings )
  : KDialogBase( parent, "folder dialog", true, caption,
                 Ok|Cancel|User1, Ok, true,
                 KGuiItem(i18n("&New Subfolder"), "folder_new",
                   i18n("Create a new subfolder under the currently selected folder"))
               ), // mainwin as parent, modal
    mUseGlobalSettings( useGlobalSettings )             
{
  QString global = mUseGlobalSettings ? 
    GlobalSettings::lastSelectedFolder() : QString::null;
  mTreeView = new KMail::SimpleFolderTree( makeVBoxMainWidget(), tree,
                                           global, mustBeReadWrite );
                                          
  mTreeView->setFocus();
  connect( mTreeView, SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
           this, SLOT( slotSelect() ) );
  connect( mTreeView, SIGNAL( selectionChanged() ),
           this, SLOT( slotUpdateBtnStatus() ) );

  readConfig();
}

//-----------------------------------------------------------------------------
KMFolderSelDlg::~KMFolderSelDlg()
{
  const KMFolder * cur = folder();
  if ( cur && mUseGlobalSettings ) {
    GlobalSettings::setLastSelectedFolder( cur->idString() );
  }

  writeConfig();
}


//-----------------------------------------------------------------------------
KMFolder * KMFolderSelDlg::folder( void )
{
  return ( KMFolder * ) mTreeView->folder();
}

//-----------------------------------------------------------------------------
void KMFolderSelDlg::setFolder( KMFolder* folder )
{
  mTreeView->setFolder( folder );
}

//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotSelect()
{
  accept();
}

//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotUser1()
{
  mTreeView->addChildFolder();
}

//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotUpdateBtnStatus()
{
  enableButton( User1, folder() && 
                ( !folder()->noContent() && !folder()->noChildren() ) );
}

//-----------------------------------------------------------------------------
void KMFolderSelDlg::setFlags( bool mustBeReadWrite, bool showOutbox, 
                               bool showImapFolders )
{
  mTreeView->reload( mustBeReadWrite, showOutbox, showImapFolders );
}

void KMFolderSelDlg::readConfig()
{
  KConfig *config = KGlobal::config();
  config->setGroup( "FolderSelectionDialog" );
  QSize size = config->readSizeEntry( "Size" );
  if ( !size.isEmpty() ) resize( size );
  else resize( 220, 300 );
}

void KMFolderSelDlg::writeConfig()
{
  KConfig *config = KGlobal::config();
  config->setGroup( "FolderSelectionDialog" );
  config->writeEntry( "Size", size() );
}

} // namespace KMail

#include "kmfolderseldlg.moc"
