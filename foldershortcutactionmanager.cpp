/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "foldershortcutactionmanager.h"

#include "foldercollection.h"
#include "kmcommands.h"
#include "kmkernel.h"
#include "kmmainwidget.h"

#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityDisplayAttribute>
#include <akonadi/entitymimetypefiltermodel.h>
#include <Akonadi/EntityTreeModel>

#include <KAction>
#include <KActionCollection>

using namespace KMail;

FolderShortcutActionManager::FolderShortcutActionManager( KMMainWidget *parent,
                                                          KActionCollection *actionCollection )
  : QObject( parent ),
    mActionCollection( actionCollection ),
    mParent( parent )
{
}

void FolderShortcutActionManager::createActions()
{
  // When this function is called, the ETM has not finished loading yet. Therefore, when new
  // rows are inserted in the ETM, see if we have new collections that we can assign shortcuts
  // to.
  const QAbstractItemModel *model = KMKernel::self()->collectionModel();
  connect( model, SIGNAL( rowsInserted( const QModelIndex &, int, int ) ),
           this, SLOT( slotRowsInserted( const QModelIndex &, int, int ) ), Qt::UniqueConnection );
  connect( KMKernel::self()->monitor(), SIGNAL( collectionRemoved( const Akonadi::Collection & ) ),
           this, SLOT( slotCollectionRemoved( const Akonadi::Collection& ) ), Qt::UniqueConnection );

  if ( model->rowCount() > 0 )
    updateShortcutsForIndex( QModelIndex(), 0, model->rowCount() - 1 );
}

void FolderShortcutActionManager::slotRowsInserted( const QModelIndex &parent, int start, int end )
{
  updateShortcutsForIndex( parent, start, end );
}

void FolderShortcutActionManager::updateShortcutsForIndex( const QModelIndex &parent, int start, int end )
{
  QAbstractItemModel *model = KMKernel::self()->collectionModel();
  for ( int i = start; i <= end; i++ ) {
    const QModelIndex child = model->index( i, 0, parent );
    Akonadi::Collection collection =
        model->data( child, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    if ( collection.isValid() ) {
      shortcutChanged( collection );
    }
    if ( model->rowCount( child ) > 0 ) {
      updateShortcutsForIndex( child, 0, model->rowCount( child ) - 1 );
    }
  }
}

void FolderShortcutActionManager::slotCollectionRemoved( const Akonadi::Collection &col )
{
  delete mFolderShortcutCommands.take( col.id() );
}

void FolderShortcutActionManager::shortcutChanged( const Akonadi::Collection &col )
{
  // remove the old one, no autodelete in Qt4
  slotCollectionRemoved( col );
  QSharedPointer<FolderCollection> folderCollection( FolderCollection::forCollection( col ) );
  if ( folderCollection->shortcut().isEmpty() )
    return;

  FolderShortcutCommand *command = new FolderShortcutCommand( mParent, col );
  mFolderShortcutCommands.insert( col.id(), command );

  KIcon icon( "folder" );
  if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() &&
       !col.attribute<Akonadi::EntityDisplayAttribute>()->iconName().isEmpty() ) {
    icon = KIcon( col.attribute<Akonadi::EntityDisplayAttribute>()->iconName() );
  }

  const QString actionLabel = i18n( "Folder Shortcut %1", col.name() );
  QString actionName = i18n( "Folder Shortcut %1", folderCollection->idString() );
  actionName.replace( ' ', '_' );
  KAction *action = mActionCollection->addAction( actionName );
  // The folder shortcut is set in the folder shortcut dialog.
  // The shortcut set in the shortcut dialog would not be saved back to
  // the folder settings correctly.
  action->setShortcutConfigurable( false );
  action->setText( actionLabel );
  action->setShortcuts( folderCollection->shortcut() );
  action->setIcon( icon );

  connect( action, SIGNAL( triggered( bool ) ), command, SLOT( start() ) );
  command->setAction( action ); // will be deleted along with the command
}

#include "foldershortcutactionmanager.moc"
