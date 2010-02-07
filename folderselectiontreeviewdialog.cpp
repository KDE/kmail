/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "folderselectiontreeviewdialog.h"
#include <kinputdialog.h>
#include <kmessagebox.h>

#include <QVBoxLayout>

#include "folderselectiontreeview.h"
#include "foldertreeview.h"
#include "readablecollectionproxymodel.h"
#include <akonadi/collection.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/collectioncreatejob.h>

#include <KLocale>

FolderSelectionTreeViewDialog::FolderSelectionTreeViewDialog( QWidget *parent, SelectionFolderOptions options )
  :KDialog( parent )
{
  setButtons( Ok | Cancel | User1 );
  setObjectName( "folder dialog" );
  setButtonGuiItem( User1, KGuiItem( i18n("&New Subfolder..."), "folder-new",
                                     i18n("Create a new subfolder under the currently selected folder") ) );

  QWidget *widget = mainWidget();
  QVBoxLayout *layout = new QVBoxLayout( widget );
  treeview = new FolderSelectionTreeView( this, 0, ( options & ShowUnreadCount ));
  treeview->disableContextMenuAndExtraColumn();
  treeview->readableCollectionProxyModel()->setEnabledCheck( ( options & EnableCheck ) );
  treeview->readableCollectionProxyModel()->setAccessRights( Akonadi::Collection::CanCreateCollection );
  treeview->folderTreeView()->setTooltipsPolicy( FolderSelectionTreeView::DisplayNever );

  layout->addWidget( treeview );
  enableButton( KDialog::Ok, false );
  enableButton( KDialog::User1, false );
  connect(treeview->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotSelectionChanged()));
  connect( treeview->folderTreeView(), SIGNAL( doubleClicked(const QModelIndex&) ), this, SLOT( accept() ) );
  connect(this, SIGNAL( user1Clicked() ), this, SLOT( slotAddChildFolder() ) );
  readConfig();
}

FolderSelectionTreeViewDialog::~FolderSelectionTreeViewDialog()
{
  writeConfig();
}

bool FolderSelectionTreeViewDialog::canCreateCollection( Akonadi::Collection & parentCol )
{
  parentCol = selectedCollection();
  if ( !parentCol.isValid() )
    return false;
 if ( ( parentCol.rights() & Akonadi::Collection::CanCreateCollection )
       && parentCol.contentMimeTypes().contains( Akonadi::Collection::mimeType() ) ) {
   return true;
 }
 return false;
}

void FolderSelectionTreeViewDialog::slotAddChildFolder()
{
  Akonadi::Collection parentCol;
  if ( canCreateCollection( parentCol ) ){
    const QString name = KInputDialog::getText( i18nc( "@title:window", "New Folder"),
                                                i18nc( "@label:textbox, name of a thing", "Name"),
                                                QString(), 0, this );
    if ( name.isEmpty() )
      return;

    Akonadi::Collection col;
    col.setName( name );
    col.parentCollection().setId( parentCol.id() );
    Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( col );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(collectionCreationResult(KJob*)) );
  }
}

void FolderSelectionTreeViewDialog::collectionCreationResult(KJob* job)
{
  if ( job->error() ) {
    KMessageBox::error( this, i18n("Could not create folder: %1", job->errorString()),
                        i18n("Folder creation failed") );
  }
}

void FolderSelectionTreeViewDialog::slotSelectionChanged()
{
  const bool enablebuttons = ( treeview->selectionModel()->selectedIndexes().count() > 0 );
  enableButton(KDialog::Ok, enablebuttons);
  Akonadi::Collection parent;
  enableButton(KDialog::User1, canCreateCollection( parent ) );
}

void FolderSelectionTreeViewDialog::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
  treeview->setSelectionMode( mode );
}

QAbstractItemView::SelectionMode FolderSelectionTreeViewDialog::selectionMode() const
{
  return treeview->selectionMode();
}


Akonadi::Collection FolderSelectionTreeViewDialog::selectedCollection() const
{
  return treeview->selectedCollection();
}

void FolderSelectionTreeViewDialog::setSelectedCollection( const Akonadi::Collection &collection )
{
  treeview->selectCollectionFolder( collection );
}

Akonadi::Collection::List FolderSelectionTreeViewDialog::selectedCollections() const
{
  return treeview->selectedCollections();
}

static const char * myConfigGroupName = "FolderSelectionDialog";

void FolderSelectionTreeViewDialog::readConfig()
{
  KSharedConfigPtr config = KGlobal::config();
  KConfigGroup group( config, myConfigGroupName );

  QSize size = group.readEntry( "Size", QSize() );
  if ( !size.isEmpty() )
    resize( size );
  else
    resize( 500, 300 );
}

void FolderSelectionTreeViewDialog::writeConfig()
{
  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group( config, myConfigGroupName );
  group.writeEntry( "Size", size() );
}

#include "folderselectiontreeviewdialog.moc"
