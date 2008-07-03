/******************************************************************************
 *
 * KMail Folder Selection Dialog
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

#include "folderselectiondialog.h"
#include "folderselectiontreewidget.h"
#include "kmfoldertree.h"
#include "kmfolder.h"
#include "kmmainwidget.h"
#include "globalsettings.h"

#include <kvbox.h>
#include <kconfiggroup.h>

#include <QLayout>

namespace KMail {

FolderSelectionDialog::FolderSelectionDialog( KMMainWidget * parent, const QString& caption,
    bool mustBeReadWrite, bool useGlobalSettings )
  : KDialog( parent ), // mainwin as parent, modal
    mUseGlobalSettings( useGlobalSettings )
{
  setCaption( caption );
  init( parent->folderTree(), mustBeReadWrite );
}

FolderSelectionDialog::FolderSelectionDialog( QWidget * parent, KMFolderTree * tree,
    const QString& caption, bool mustBeReadWrite, bool useGlobalSettings )
  : KDialog( parent ), // anywidget as parent, modal
    mUseGlobalSettings( useGlobalSettings )
{
  setCaption( caption );
  init( tree, mustBeReadWrite );
}

void FolderSelectionDialog::init( KMFolderTree *tree, bool mustBeReadWrite )
{
  setButtons( Ok | Cancel | User1 );
  setObjectName( "folder dialog" );
  setButtonGuiItem( User1, KGuiItem( i18n("&New Subfolder..."), "folder-new",
         i18n("Create a new subfolder under the currently selected folder") ) );

  QString preSelection = mUseGlobalSettings ?
    GlobalSettings::self()->lastSelectedFolder() : QString();
  QWidget *vbox = new KVBox( this );
  setMainWidget( vbox );
  mTreeView = new KMail::FolderSelectionTreeWidget( vbox, tree );
  mTreeView->setSortingEnabled( false );
  mTreeView->reload( mustBeReadWrite, true, true, preSelection );
  mTreeView->setFocus();
  connect( mTreeView, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ),
           this, SLOT( slotSelect() ) );
  connect( mTreeView, SIGNAL( itemSelectionChanged() ),
           this, SLOT( slotUpdateBtnStatus() ) );
  connect(this, SIGNAL( user1Clicked() ), mTreeView, SLOT( addChildFolder() ) );
  readConfig();
  mTreeView->resizeColumnToContents(0);
}

FolderSelectionDialog::~FolderSelectionDialog()
{
  const KMFolder * cur = folder();
  if ( cur && mUseGlobalSettings ) {
    GlobalSettings::self()->setLastSelectedFolder( cur->idString() );
  }

  writeConfig();
}


KMFolder * FolderSelectionDialog::folder( void ) const
{
  return mTreeView->folder();
}

void FolderSelectionDialog::setFolder( KMFolder* folder )
{
  mTreeView->setFolder( folder );
}

void FolderSelectionDialog::slotSelect()
{
  if( !folder() )
    return;
  accept();
}

void FolderSelectionDialog::slotUpdateBtnStatus()
{
  enableButton( User1, folder() &&
                ( !folder()->noContent() && !folder()->noChildren() ) );
}

void FolderSelectionDialog::setFlags( bool mustBeReadWrite, bool showOutbox,
                               bool showImapFolders )
{
  mTreeView->reload( mustBeReadWrite, showOutbox, showImapFolders );
}

static const char * myConfigGroupName = "FolderSelectionDialog";

void FolderSelectionDialog::readConfig()
{
  KSharedConfigPtr config = KGlobal::config();
  KConfigGroup group( config, myConfigGroupName );

  QSize size = group.readEntry( "Size", QSize() );
  if ( !size.isEmpty() )
    resize( size );
  else
    resize( 500, 300 );

  if ( !mTreeView->restoreLayout( group ) ) 
  {
    int colWidth = width() / 2;
    mTreeView->setColumnWidth( mTreeView->nameColumnIndex(), colWidth );
    mTreeView->setColumnWidth( mTreeView->pathColumnIndex(), colWidth );
  }
}

void FolderSelectionDialog::writeConfig()
{
  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group( config, myConfigGroupName );
  group.writeEntry( "Size", size() );

  mTreeView->saveLayout( group ); // assume success (there's nothing we can do anyway)
}

} // namespace KMail

#include "folderselectiondialog.moc"
