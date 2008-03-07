// kmfolderseldlg.cpp

#include <config.h>
#include "kmfolderseldlg.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "globalsettings.h"

#include <kdebug.h>
#include <klineedit.h>
#include <kpopupmenu.h>
#include <kiconloader.h>

#include <qlayout.h>
#include <qtoolbutton.h>


using namespace KMail;
//-----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg( KMMainWidget * parent, const QString& caption,
    bool mustBeReadWrite, bool useGlobalSettings )
  : KDialogBase( parent, "folder dialog", true, caption,
                 Ok|Cancel|User1, Ok, true,
                 KGuiItem(i18n("&New Subfolder..."), "folder_new",
                   i18n("Create a new subfolder under the currently selected folder"))
               ), // mainwin as parent, modal
    mUseGlobalSettings( useGlobalSettings )
{
  KMFolderTree * ft = parent->folderTree();
  assert( ft );

  QString preSelection = mUseGlobalSettings ?
    GlobalSettings::self()->lastSelectedFolder() : QString::null;
  mTreeView = new KMail::SimpleFolderTree( makeVBoxMainWidget(), ft,
                                           preSelection, mustBeReadWrite );
  init();
}

//----------------------------------------------------------------------------
KMFolderSelDlg::KMFolderSelDlg( QWidget * parent, KMFolderTree * tree,
    const QString& caption, bool mustBeReadWrite, bool useGlobalSettings )
  : KDialogBase( parent, "folder dialog", true, caption,
                 Ok|Cancel|User1, Ok, true,
                 KGuiItem(i18n("&New Subfolder..."), "folder_new",
                   i18n("Create a new subfolder under the currently selected folder"))
               ), // mainwin as parent, modal
    mUseGlobalSettings( useGlobalSettings )
{
  QString preSelection = mUseGlobalSettings ?
    GlobalSettings::self()->lastSelectedFolder() : QString::null;
  mTreeView = new KMail::SimpleFolderTree( makeVBoxMainWidget(), tree,
                                           preSelection, mustBeReadWrite );
  init();
}

//-----------------------------------------------------------------------------
void KMFolderSelDlg::init()
{
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
    GlobalSettings::self()->setLastSelectedFolder( cur->idString() );
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
  else resize( 500, 300 );

  QValueList<int> widths = config->readIntListEntry( "ColumnWidths" );
  if ( !widths.isEmpty() ) {
    mTreeView->setColumnWidth(mTreeView->folderColumn(), widths[0]);
    mTreeView->setColumnWidth(mTreeView->pathColumn(),   widths[1]);
  }
  else {
    int colWidth = width() / 2;
    mTreeView->setColumnWidth(mTreeView->folderColumn(), colWidth);
    mTreeView->setColumnWidth(mTreeView->pathColumn(),   colWidth);
  }
}

void KMFolderSelDlg::writeConfig()
{
  KConfig *config = KGlobal::config();
  config->setGroup( "FolderSelectionDialog" );
  config->writeEntry( "Size", size() );

  QValueList<int> widths;
  widths.push_back(mTreeView->columnWidth(mTreeView->folderColumn()));
  widths.push_back(mTreeView->columnWidth(mTreeView->pathColumn()));
  config->writeEntry( "ColumnWidths", widths );
}


#include "kmfolderseldlg.moc"
