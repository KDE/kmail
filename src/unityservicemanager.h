/*
   SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/Collection>
#include <QModelIndex>
#include <QObject>
class QDBusServiceWatcher;
class QAbstractItemModel;
namespace KMail
{
class KMSystemTray;
class UnityServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit UnityServiceManager(QObject *parent = nullptr);
    ~UnityServiceManager() override;

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
    QDBusServiceWatcher *const mUnityServiceWatcher;
    KMail::KMSystemTray *mSystemTray = nullptr;
    int mCount = 0;
    bool mUnityServiceAvailable = false;
};
}
