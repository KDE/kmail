/* Copyright 2009 Klarälvdalens Datakonsult AB

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
#include "folderutil.h"

#include "kmfolder.h"
#include "kmfolderimap.h"
#include "kmfoldercachedimap.h"
#include "kmfoldermgr.h"

#include <kmessagebox.h>

using namespace KMail;
using namespace FolderUtil;

KMFolder *KMail::FolderUtil::createSubFolder( KMFolder *parentFolder, KMFolderDir *parentDir,
                                              const QString &folderName, const QString &namespaceName,
                                              KMFolderType localFolderType )
{
  KMFolder *newFolder = 0;

  if ( parentFolder && parentFolder->folderType() == KMFolderTypeImap ) {
    KMFolderImap* selectedStorage = static_cast<KMFolderImap*>( parentFolder->storage() );
    KMAcctImap *anAccount = selectedStorage->account();
    // check if a connection is available BEFORE creating the folder
    if (anAccount->makeConnection() == ImapAccountBase::Connected) {
      newFolder = kmkernel->imapFolderMgr()->createFolder( folderName, false, KMFolderTypeImap, parentDir );
      if ( newFolder ) {
        QString imapPath, parent;
        if ( !namespaceName.isEmpty() ) {
          // create folder with namespace
          parent = anAccount->addPathToNamespace( namespaceName );
          imapPath = anAccount->createImapPath( parent, folderName );
        } else {
          imapPath = anAccount->createImapPath( selectedStorage->imapPath(), folderName );
        }
        KMFolderImap* newStorage = static_cast<KMFolderImap*>( newFolder->storage() );
        selectedStorage->createFolder(folderName, parent); // create it on the server
        newStorage->initializeFrom( selectedStorage, imapPath, QString::null );
        static_cast<KMFolderImap*>(parentFolder->storage())->setAccount( selectedStorage->account() );
        return newFolder;
      }
    }
  } else if ( parentFolder && parentFolder->folderType() == KMFolderTypeCachedImap ) {
    newFolder = kmkernel->dimapFolderMgr()->createFolder( folderName, false, KMFolderTypeCachedImap,
                                                          parentDir );
    if ( newFolder ) {
      KMFolderCachedImap* selectedStorage = static_cast<KMFolderCachedImap*>( parentFolder->storage() );
      KMFolderCachedImap* newStorage = static_cast<KMFolderCachedImap*>( newFolder->storage() );
      newStorage->initializeFrom( selectedStorage );
      if ( !namespaceName.isEmpty() ) {
        // create folder with namespace
        QString path = selectedStorage->account()->createImapPath(
            namespaceName, folderName );
        newStorage->setImapPathForCreation( path );
      }
      return newFolder;
    }
  } else {
    // local folder
    Q_ASSERT( localFolderType == KMFolderTypeMaildir || localFolderType == KMFolderTypeMbox );
    newFolder = kmkernel->folderMgr()->createFolder( folderName, false, localFolderType,
                                                     parentDir );
    return newFolder;
  }

  return newFolder;
}

void KMail::FolderUtil::deleteFolder( KMFolder *folderToDelete, QWidget *parent )
{
  if ( folderToDelete->hasAccounts() ) {
    // this folder has an account, so we need to change that to the inbox
    for ( AccountList::Iterator it (folderToDelete->acctList()->begin() ),
           end( folderToDelete->acctList()->end() ); it != end; ++it ) {
      (*it)->setFolder( kmkernel->inboxFolder() );
      KMessageBox::information(parent,
          i18n("<qt>The folder you deleted was associated with the account "
            "<b>%1</b> which delivered mail into it. The folder the account "
            "delivers new mail into was reset to the main Inbox folder.</qt>").arg( (*it)->name()));
    }
  }
  if (folderToDelete->folderType() == KMFolderTypeImap)
    kmkernel->imapFolderMgr()->remove(folderToDelete);
  else if (folderToDelete->folderType() == KMFolderTypeCachedImap) {
    // Deleted by user -> tell the account (see KMFolderCachedImap::listDirectory2)
    KMFolderCachedImap* storage = static_cast<KMFolderCachedImap*>( folderToDelete->storage() );
    KMAcctCachedImap* acct = storage->account();
    if ( acct )
      acct->addDeletedFolder( folderToDelete );

    kmkernel->dimapFolderMgr()->remove(folderToDelete);
  }
  else if (folderToDelete->folderType() == KMFolderTypeSearch)
    kmkernel->searchFolderMgr()->remove(folderToDelete);
  else
    kmkernel->folderMgr()->remove(folderToDelete);
}
