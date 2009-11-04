/*
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

#ifndef FOLDERCOLLECTIONMONITOR_H
#define FOLDERCOLLECTIONMONITOR_H

#include <QObject>
namespace Akonadi {
  class ChangeRecorder;
}

class FolderCollectionMonitor : public QObject
{
  Q_OBJECT
public:
  FolderCollectionMonitor(QObject *parent);
  ~FolderCollectionMonitor();

  Akonadi::ChangeRecorder * monitor();
  void expireAllFolders(bool immediate );
  void compactAllFolders( bool immediate );
private:
  Akonadi::ChangeRecorder *mMonitor;
};



#endif /* FOLDERCOLLECTIONMONITOR_H */

