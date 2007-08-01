/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "foldersetselector.h"

#include "globalsettings.h"
#include "kmfoldertree.h"
#include "simplefoldertree.h"
#include "kmfoldercachedimap.h"

#include <qvbox.h>

using namespace KMail;

FolderSetSelector::FolderSetSelector( KMFolderTree *ft, QWidget * parent )
  : KDialogBase( parent, "FolderSetSelector", true, QString(), Ok|Cancel, Ok, true )
{
  assert( ft );

  mTreeView = new KMail::SimpleFolderTreeBase<QCheckListItem>( makeVBoxMainWidget(), ft,
      GlobalSettings::self()->lastSelectedFolder(), false );
  mTreeView->setFocus();

  QListViewItemIterator it( mTreeView );
  while ( it.current() ) {
    SimpleFolderTreeItem<QCheckListItem> *item = dynamic_cast<SimpleFolderTreeItem<QCheckListItem>*>( it.current() );
    ++it;
    if ( !item )
      continue;
    if ( !item->folder() ) {
      item->setEnabled( false );
      continue;
    }
    if ( item->folder()->folderType() == KMFolderTypeCachedImap
         && static_cast<const KMFolderCachedImap*>( item->folder()->storage() )->imapPath() == "/INBOX/" ) {
        item->setOn( true );
    }
    if ( item->folder()->folderType() != KMFolderTypeCachedImap ) {
      item->setEnabled( false );
    }
  }

}

QValueList< int > FolderSetSelector::selectedFolders()
{
  QValueList<int> rv;
  QListViewItemIterator it( mTreeView );
  while ( it.current() ) {
    SimpleFolderTreeItem<QCheckListItem> *item = dynamic_cast<SimpleFolderTreeItem<QCheckListItem>*>( it.current() );
    if ( item && item->isOn() && item->folder() )
      rv.append( item->folder()->id() );
    ++it;
  }
  return rv;
}

void FolderSetSelector::setSelectedFolders(const QValueList< int > & folderIds)
{
  QListViewItemIterator it( mTreeView );
  while ( it.current() ) {
    SimpleFolderTreeItem<QCheckListItem> *item = dynamic_cast<SimpleFolderTreeItem<QCheckListItem>*>( it.current() );
    if ( item && item->folder() ) {
      if ( folderIds.contains( item->folder()->id() ) )
        item->setOn( true );
      else
        item->setOn( false );
    }
    ++it;
  }
}

#include "foldersetselector.moc"
