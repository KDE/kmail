// kmfolderseldlg.cpp

#include <config.h>
#include "kmfolderseldlg.h"
#include "kmfoldertree.h"
#include "kmmainwidget.h"
#include "globalsettings.h"

#include <kdebug.h>
#include <klineedit.h>

#include <qlayout.h>
#include <qtoolbutton.h>

namespace KMail {

class FolderItem : public KListViewItem
{
  public:
    FolderItem( QListView * listView );
    FolderItem( QListView * listView, QListViewItem * afterListViewItem );
    FolderItem( QListViewItem * listViewItem );
    FolderItem( QListViewItem * listViewItem, QListViewItem * afterListViewItem );

    void setFolder( KMFolder * folder ) { mFolder = folder; };
    const KMFolder * folder() { return mFolder; };

  private:
    KMFolder * mFolder;
};

//-----------------------------------------------------------------------------
FolderItem::FolderItem( QListView * listView )
  : KListViewItem( listView ),
    mFolder( 0 )
{}

//-----------------------------------------------------------------------------
FolderItem::FolderItem( QListView * listView, QListViewItem * afterListViewItem )
  : KListViewItem( listView, afterListViewItem ),
    mFolder( 0 )
{}

//-----------------------------------------------------------------------------
FolderItem::FolderItem( QListViewItem * listViewItem )
  : KListViewItem( listViewItem ),
    mFolder( 0 )
{}

//-----------------------------------------------------------------------------
FolderItem::FolderItem( QListViewItem * listViewItem, QListViewItem * afterListViewItem )
  : KListViewItem( listViewItem, afterListViewItem ),
    mFolder( 0 )
{}


//-----------------------------------------------------------------------------
SimpleFolderTree::SimpleFolderTree( QWidget * parent,
                                    KMFolderTree * folderTree,
                                    const QString & preSelection,
                                    bool mustBeReadWrite )
  : KListView( parent ), mFolderTree( folderTree )
{
  mFolderColumn = addColumn( i18n( "Folder" ) );
  setRootIsDecorated( true );
  setSorting( -1 );

  reload( mustBeReadWrite, true, true, preSelection );
}

void SimpleFolderTree::reload( bool mustBeReadWrite, bool showOutbox,
                               bool showImapFolders, const QString& preSelection )
{
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
      if ( lastTopItem )
        item = new FolderItem( this, lastTopItem );
      else
        item = new FolderItem( this );
      lastTopItem = item;
      depth = 0;
    }
    else {
      if ( depth > lastDepth ) {
        // next lower level - parent node will get opened
        item = new FolderItem( lastItem );
        lastItem->setOpen( true );
      }
      else {
        if ( depth == lastDepth )
          // same level - behind previous item
          item = new FolderItem( lastItem->parent(), lastItem );
        else if ( depth < lastDepth ) {
          // above previous level - might be more than one level difference
          // but highest possibility is top level
          while ( ( depth <= --lastDepth ) && lastItem->parent() ) {
            lastItem = static_cast<FolderItem *>( lastItem->parent() );
          }
          if ( lastItem->parent() )
            item = new FolderItem( lastItem->parent(), lastItem );
          else {
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
    // Make items without folders and top level items unselectable
    // (i.e. root item Local Folders and IMAP accounts)
    if ( !fti->folder() || depth == 0 || ( mustBeReadWrite && fti->folder()->isReadOnly() ) ) {
      item->setSelectable( false );
    } else {
      item->setFolder( fti->folder() );
      if ( selected == item->folder()->idString() )
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
KMFolderSelDlg::KMFolderSelDlg( KMMainWidget * parent, const QString& caption, 
    bool mustBeReadWrite, bool useGlobalSettings )
  : KDialogBase( parent, "folder dialog", true, caption,
                 Ok|Cancel, Ok, true ), // mainwin as parent, modal
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

  resize(220, 300);
}

//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg( QWidget * parent, KMFolderTree * tree,
    const QString& caption, bool mustBeReadWrite, bool useGlobalSettings )
  : KDialogBase( parent, "folder dialog", true, caption,
                 Ok|Cancel, Ok, true ), // mainwin as parent, modal
    mUseGlobalSettings( useGlobalSettings )             
{
  QString global = mUseGlobalSettings ? 
    GlobalSettings::lastSelectedFolder() : QString::null;
  mTreeView = new KMail::SimpleFolderTree( makeVBoxMainWidget(), tree,
                                           global, mustBeReadWrite );
                                          
  mTreeView->setFocus();
  connect( mTreeView, SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
           this, SLOT( slotSelect() ) );

  resize(220, 300);
}

//-----------------------------------------------------------------------------
KMFolderSelDlg::~KMFolderSelDlg()
{
  const KMFolder * cur = folder();
  if ( cur && mUseGlobalSettings ) {
    GlobalSettings::setLastSelectedFolder( cur->idString() );
  }
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
void KMFolderSelDlg::setFlags( bool mustBeReadWrite, bool showOutbox, 
                               bool showImapFolders )
{
  mTreeView->reload( mustBeReadWrite, showOutbox, showImapFolders );
}

} // namespace KMail

#include "kmfolderseldlg.moc"
