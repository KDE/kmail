// kmfolderseldlg.cpp

#include <config.h>
#include "kmfolderseldlg.h"
#include "kmfoldertree.h"
#include "kmmainwidget.h"

#include <kdebug.h>
#include <qvbox.h>

#include <assert.h>

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
                  KMFolderTree * folderTree, QString & preSelection )
  : KListView( parent )
{
  assert( folderTree );
  
  int columnIdx = addColumn( i18n( "Folder" ) );
  setRootIsDecorated( true );
  setSorting( -1 );

  FolderItem * lastItem = 0;
  FolderItem * lastTopItem = 0;
  FolderItem * selectedItem = 0;
  int lastDepth = 0;
  
  for ( QListViewItemIterator it( folderTree ) ; it.current() ; ++it ) {
    KMFolderTreeItem * fti = static_cast<KMFolderTreeItem *>( it.current() );
    
    if ( !fti || fti->protocol() == KFolderTreeItem::Search )
      continue;

    int depth = fti->depth();// - 1;
    //kdDebug( 5006 ) << "LastDepth=" << lastDepth << "\tdepth=" << depth 
    //                << "\tname=" << fti->text( 0 ) << endl;
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
    
    item->setText( columnIdx, fti->text( 0 ) );
    // Make items without folders and top level items unselectable
    // (i.e. root item Local Folders and IMAP accounts)
    if ( !fti->folder() || depth == 0 )
      item->setSelectable( false );
    else {
      item->setFolder( fti->folder() );
      if ( !preSelection.isNull() && preSelection == item->folder()->idString() )
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
  if( item ) {
    const KMFolder * folder = static_cast<FolderItem *>( item )->folder();
    if( folder ) return folder;
  }
  return 0;
}

} // namespace KMail


//-----------------------------------------------------------------------------
QString KMFolderSelDlg::oldSelection;

//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg( KMMainWidget * parent, const QString& caption )
  : KDialogBase( parent, "folder dialog", true, caption,
                 Ok|Cancel, Ok, true ) // mainwin as parent, modal
{
  KMFolderTree * ft = parent->folderTree();
  assert( ft );

  mTreeView = new KMail::SimpleFolderTree( makeVBoxMainWidget(), ft, oldSelection );
  mTreeView->setFocus();
  connect( mTreeView, SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ), 
           this, SLOT( slotSelect() ) );

  resize(220, 300);
}


//-----------------------------------------------------------------------------
KMFolderSelDlg::~KMFolderSelDlg()
{
  const KMFolder * cur = folder();
  if( cur )
    oldSelection = cur->idString();
}


//-----------------------------------------------------------------------------
KMFolder * KMFolderSelDlg::folder( void )
{
  return ( KMFolder * ) mTreeView->folder();
}

//-----------------------------------------------------------------------------
void KMFolderSelDlg::slotSelect()
{
  accept();
}


//-----------------------------------------------------------------------------
#include "kmfolderseldlg.moc"
