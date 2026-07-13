/*
   SPDX-FileCopyrightText: 2018-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmsystemtray.h"
#include <Akonadi/Collection>
#include <QModelIndex>
#include <QObject>
#include <QPointer>
class QAbstractItemModel;
class QWindow;
namespace KMail
{
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
    [[nodiscard]] bool hasUnreadMail() const;
    QPointer<KMail::KMSystemTray> mSystemTray;
    int mCount = 0;
};
}
