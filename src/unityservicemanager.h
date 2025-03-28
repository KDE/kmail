/*
   SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Collection>
#include <QModelIndex>
#include <QObject>
class QDBusServiceWatcher;
class QAbstractItemModel;
class QWindow;
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
    [[nodiscard]] bool haveSystemTrayApplet() const;

    [[nodiscard]] bool canQueryClose();
    void toggleSystemTray(QWidget *parent);
    void initListOfCollection();
    [[nodiscard]] bool excludeFolder(const Akonadi::Collection &collection) const;
    [[nodiscard]] bool ignoreNewMailInFolder(const Akonadi::Collection &collection);
    void updateCount();

    void setSystemTryAssociatedWindow(QWindow *window);

private:
    void unreadMail(const QAbstractItemModel *model, const QModelIndex &parentIndex = {});
    void slotCollectionStatisticsChanged(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &);
    void initUnity();
    [[nodiscard]] bool hasUnreadMail() const;
    QDBusServiceWatcher *const mUnityServiceWatcher;
    KMail::KMSystemTray *mSystemTray = nullptr;
    int mCount = 0;
    bool mUnityServiceAvailable = false;
};
}
