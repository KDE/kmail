/*
   Copyright (C) 2018-2019 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef UNITYSERVICEMANAGER_H
#define UNITYSERVICEMANAGER_H

#include <QModelIndex>
#include <QObject>
#include <AkonadiCore/Collection>
class QDBusServiceWatcher;
class QAbstractItemModel;
namespace KMail {
class KMSystemTray;
class UnityServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit UnityServiceManager(QObject *parent = nullptr);
    ~UnityServiceManager();

    void updateSystemTray();
    Q_REQUIRED_RESULT bool haveSystemTrayApplet() const;

    Q_REQUIRED_RESULT bool canQueryClose();
    void toggleSystemTray(QWidget *parent);
    void initListOfCollection();
    Q_REQUIRED_RESULT bool excludeFolder(const Akonadi::Collection &collection) const;
    Q_REQUIRED_RESULT bool ignoreNewMailInFolder(const Akonadi::Collection &collection);
    void updateCount();
private:
    Q_DISABLE_COPY(UnityServiceManager)
    void unreadMail(const QAbstractItemModel *model, const QModelIndex &parentIndex = {});
    void slotCollectionStatisticsChanged(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &);
    void initUnity();
    Q_REQUIRED_RESULT bool hasUnreadMail() const;
    QDBusServiceWatcher *mUnityServiceWatcher = nullptr;
    KMail::KMSystemTray *mSystemTray = nullptr;
    int mCount = 0;
    bool mUnityServiceAvailable = false;
};
}
#endif // UNITYSERVICEMANAGER_H
