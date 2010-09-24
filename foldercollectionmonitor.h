/*
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

#ifndef FOLDERCOLLECTIONMONITOR_H
#define FOLDERCOLLECTIONMONITOR_H

#include <QObject>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <ksharedconfig.h>

#include <QModelIndex>

class MailCommon;

class QAbstractItemModel;

namespace Akonadi {
  class ChangeRecorder;
  class Collection;
}

class FolderCollectionMonitor : public QObject
{
  Q_OBJECT
public:
  FolderCollectionMonitor(QObject *parent, MailCommon* mailCommon);
  ~FolderCollectionMonitor();

  Akonadi::ChangeRecorder * monitor() const;
  void expireAllFolders( bool immediate, QAbstractItemModel* collectionModel );
  void expunge( const Akonadi::Collection& );
private slots:
  void slotExpungeJob( KJob *job );
  void slotDeleteJob( KJob *job );

protected:
  void expireAllCollection( const QAbstractItemModel *model, bool immediate, const QModelIndex& parentIndex = QModelIndex() );

private:
  Akonadi::ChangeRecorder *mMonitor;
  MailCommon* mMailCommon;
};



#endif /* FOLDERCOLLECTIONMONITOR_H */

