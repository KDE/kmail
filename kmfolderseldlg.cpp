// kmfolderseldlg.cpp

#include <config.h>
#include "kmfolderseldlg.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "globalsettings.h"

#include <kdebug.h>
#include <qvbox.h>

#include <assert.h>

//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg( KMMainWidget * parent, const QString& caption, bool mustBeReadWrite )
  : KDialogBase( parent, "folder dialog", true, caption,
                 Ok|Cancel, Ok, true ) // mainwin as parent, modal
{
  KMFolderTree * ft = parent->folderTree();
  assert( ft );

  mTreeView = new KMail::SimpleFolderTree( makeVBoxMainWidget(), ft,
      GlobalSettings::self()->lastSelectedFolder(), mustBeReadWrite );
  mTreeView->setFocus();
  connect( mTreeView, SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
           this, SLOT( slotSelect() ) );

  resize(220, 300);
}


//-----------------------------------------------------------------------------
KMFolderSelDlg::~KMFolderSelDlg()
{
  const KMFolder * cur = folder();
  if ( cur ) {
    GlobalSettings::self()->setLastSelectedFolder( cur->idString() );
  }
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
