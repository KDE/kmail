/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Andras Mantia <andras@kdab.com>

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

#ifndef MAILCOMMON_H
#define MAILCOMMON_H
#include <QObject>

#include <ksharedconfig.h>

#include "akonadi/collection.h"
#include "akonadi/kmime/specialmailcollections.h"


namespace KMail {
  class JobScheduler;
}

namespace Akonadi {
  class ChangeRecorder;
  class EntityMimeTypeFilterModel;
}

namespace KPIMIdentities {
  class IdentityManager;
}

/** Deals with common mail application related operations. It needs to be
 * initialized with all the setters before usage. There should be one MailCommon
 * object per application and either used as an argument for different other
 * classes, so they work on the same config file, collectionmodel, etc., or stored
 * in an application global static object (think KMKernel in KMail desktop).
 *
 * Never delete any MailCommon pointer you got!
 */

class MailCommon : public QObject {
  Q_OBJECT  
public:
  MailCommon( QObject* parent = 0 );

  /**
   * Returns a model of all folders in KMail. This is basically the same as entityTreeModel(),
   * but with items filtered out, the model contains only collections.
   */
  Akonadi::EntityMimeTypeFilterModel *collectionModel() const;
  void setCollectionModel( Akonadi::EntityMimeTypeFilterModel *collectionModel );

  KSharedConfig::Ptr config();
  void setConfig( KSharedConfig::Ptr config );
  void syncConfig();

    /** return the pointer to the identity manager */
  KPIMIdentities::IdentityManager *identityManager();
  void setIdentityManager( KPIMIdentities::IdentityManager *identityManager );

  KMail::JobScheduler* jobScheduler() { return mJobScheduler; }
  void setJobScheduler( KMail::JobScheduler* jobScheduler ) { mJobScheduler = jobScheduler; }

  Akonadi::ChangeRecorder *folderCollectionMonitor() { return mFolderCollectionMonitor; }
  void setFolderCollectionMonitor( Akonadi::ChangeRecorder* monitor ) { mFolderCollectionMonitor = monitor; }
    /**
  * Returns the collection associated with the given @p id, or an invalid collection if not found.
  * The EntityTreeModel of the kernel is searched for the collection. Since the ETM is loaded
  * async, this method will not find the collection right after startup, when the ETM is not yet
  * fully loaded.
  */
  Akonadi::Collection collectionFromId( const Akonadi::Collection::Id& id ) const;

  /**
  * Converts @p idString into a number and returns the collection for it.
  * @see collectionFromId( qint64 )
  */
  Akonadi::Collection collectionFromId( const QString &idString ) const;

  Akonadi::Collection inboxCollectionFolder();
  Akonadi::Collection outboxCollectionFolder();
  Akonadi::Collection sentCollectionFolder();
  Akonadi::Collection trashCollectionFolder();
  Akonadi::Collection draftsCollectionFolder();
  Akonadi::Collection templatesCollectionFolder();

  bool isSystemFolderCollection( const Akonadi::Collection &col );

  /** Returns true if this folder is the inbox on the local disk */
  bool isMainFolderCollection( const Akonadi::Collection &col );

  void initFolders();

  void emergencyExit( const QString& reason );

  void updateSystemTray();
  
private:  
  void findCreateDefaultCollection( Akonadi::SpecialMailCollections::Type );

private Q_SLOTS:  
  void createDefaultCollectionDone( KJob * job);
  void slotDefaultCollectionsChanged();

Q_SIGNALS:
  void requestConfigSync();
  void requestSystemTrayUpdate();

private:
  KSharedConfig::Ptr mConfig;
  Akonadi::EntityMimeTypeFilterModel *mCollectionModel;
  KPIMIdentities::IdentityManager *mIdentityManager;
  KMail::JobScheduler* mJobScheduler;
  Akonadi::ChangeRecorder *mFolderCollectionMonitor;

  Akonadi::Collection::Id the_inboxCollectionFolder;
  Akonadi::Collection::Id the_outboxCollectionFolder;
  Akonadi::Collection::Id the_sentCollectionFolder;
  Akonadi::Collection::Id the_trashCollectionFolder;
  Akonadi::Collection::Id the_draftsCollectionFolder;
  Akonadi::Collection::Id the_templatesCollectionFolder;
};

#endif
