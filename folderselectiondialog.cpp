/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009, 2010 Montel Laurent <montel@kde.org>

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

#include "folderselectiondialog.h"
#include <QKeyEvent>
#include <kinputdialog.h>
#include <kmessagebox.h>

#include <QVBoxLayout>

#include "foldertreewidget.h"
#include "foldertreeview.h"
#include "readablecollectionproxymodel.h"
#include <akonadi/collection.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/collectioncreatejob.h>

#include <KLocale>
class FolderSelectionDialog::FolderSelectionDialogPrivate
{
public:
  FolderSelectionDialogPrivate()
    : folderTreeWidget( 0 ), mNotAllowToCreateNewFolder( false )
  {
  }
  QString mFilter;
  FolderTreeWidget *folderTreeWidget;
  bool mNotAllowToCreateNewFolder;
};

FolderSelectionDialog::FolderSelectionDialog( QWidget *parent, SelectionFolderOptions options )
  :KDialog( parent ), d( new FolderSelectionDialogPrivate() )
{
  setObjectName( "folder dialog" );

  d->mNotAllowToCreateNewFolder = ( options & FolderSelectionDialog::NotAllowToCreateNewFolder );
  if ( d->mNotAllowToCreateNewFolder )
    setButtons( Ok | Cancel );
  else {
    setButtons( Ok | Cancel | User1 );
    setButtonGuiItem( User1, KGuiItem( i18n("&New Subfolder..."), "folder-new",
                                       i18n("Create a new subfolder under the currently selected folder") ) );
  }

  QWidget *widget = mainWidget();
  QVBoxLayout *layout = new QVBoxLayout( widget );
  FolderTreeWidget::TreeViewOptions opt= FolderTreeWidget::None;
  if ( options & FolderSelectionDialog::ShowUnreadCount )
    opt |= FolderTreeWidget::ShowUnreadCount;

  d->folderTreeWidget = new FolderTreeWidget( this, 0, opt);
  d->folderTreeWidget->disableContextMenuAndExtraColumn();
  d->folderTreeWidget->readableCollectionProxyModel()->setEnabledCheck( ( options & EnableCheck ) );
  d->folderTreeWidget->readableCollectionProxyModel()->setAccessRights( Akonadi::Collection::CanCreateCollection );
  d->folderTreeWidget->folderTreeView()->setTooltipsPolicy( FolderTreeWidget::DisplayNever );
  d->folderTreeWidget->folderTreeView()->setDragDropMode( QAbstractItemView::NoDragDrop );
  layout->addWidget( d->folderTreeWidget );
  enableButton( KDialog::Ok, false );
  if ( !d->mNotAllowToCreateNewFolder ) {
    enableButton( KDialog::User1, false );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotAddChildFolder() ) );
  }

  connect( d->folderTreeWidget->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
           this, SLOT(slotSelectionChanged()) );
  connect( d->folderTreeWidget->readableCollectionProxyModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( rowsInserted( const QModelIndex&, int, int ) ) );

  connect( d->folderTreeWidget->folderTreeView(), SIGNAL( doubleClicked(const QModelIndex&) ),
           this, SLOT( accept() ) );
  readConfig();
}

FolderSelectionDialog::~FolderSelectionDialog()
{
  writeConfig();
  delete d;
}

void FolderSelectionDialog::rowsInserted( const QModelIndex&, int, int )
{
  d->folderTreeWidget->folderTreeView()->expandAll();
}

bool FolderSelectionDialog::canCreateCollection( Akonadi::Collection & parentCol )
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

void FolderSelectionDialog::slotAddChildFolder()
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

void FolderSelectionDialog::collectionCreationResult(KJob* job)
{
  if ( job->error() ) {
    KMessageBox::error( this, i18n("Could not create folder: %1", job->errorString()),
                        i18n("Folder creation failed") );
  }
}

void FolderSelectionDialog::slotSelectionChanged()
{
  const bool enablebuttons = ( d->folderTreeWidget->selectionModel()->selectedIndexes().count() > 0 );
  enableButton(KDialog::Ok, enablebuttons);

  if ( !d->mNotAllowToCreateNewFolder ) {
    Akonadi::Collection parent;
    enableButton(KDialog::User1, canCreateCollection( parent ) );
  }
}

void FolderSelectionDialog::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
  d->folderTreeWidget->setSelectionMode( mode );
}

QAbstractItemView::SelectionMode FolderSelectionDialog::selectionMode() const
{
  return d->folderTreeWidget->selectionMode();
}


Akonadi::Collection FolderSelectionDialog::selectedCollection() const
{
  return d->folderTreeWidget->selectedCollection();
}

void FolderSelectionDialog::setSelectedCollection( const Akonadi::Collection &collection )
{
  d->folderTreeWidget->selectCollectionFolder( collection );
}

Akonadi::Collection::List FolderSelectionDialog::selectedCollections() const
{
  return d->folderTreeWidget->selectedCollections();
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
}

void FolderSelectionDialog::writeConfig()
{
  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup group( config, myConfigGroupName );
  group.writeEntry( "Size", size() );
}

void FolderSelectionDialog::keyPressEvent( QKeyEvent *e )
{

  switch( e->key() )
  {
    case Qt::Key_Backspace:
      if ( d->mFilter.length() > 0 )
        d->mFilter.truncate( d->mFilter.length()-1 );
      d->folderTreeWidget->applyFilter( d->mFilter );
      return;
    break;
    case Qt::Key_Delete:
      d->mFilter = "";
      d->folderTreeWidget->applyFilter( d->mFilter);
      return;
    break;
    default:
    {
      QString s = e->text();
      if ( !s.isEmpty() && s.at( 0 ).isPrint() ) {
         d->mFilter += s;
        d->folderTreeWidget->applyFilter( d->mFilter );
        return;
      }
    }
    break;
  }
  KDialog::keyPressEvent( e );
}

#include "folderselectiondialog.moc"
