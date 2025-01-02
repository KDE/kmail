/*
  SPDX-FileCopyrightText: 2023-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <Akonadi/Collection>
#include <QObject>
namespace KPIM
{
class ProgressItem;
}
namespace Akonadi
{
class ClearCacheFoldersJob;
}
class ClearCacheJobInFolderAndSubFolderJob : public QObject
{
    Q_OBJECT
public:
    explicit ClearCacheJobInFolderAndSubFolderJob(QObject *parent = nullptr, QWidget *parentWidget = nullptr);
    ~ClearCacheJobInFolderAndSubFolderJob() override;

    void start();

    void setTopLevelCollection(const Akonadi::Collection &topLevelCollection);

Q_SIGNALS:
    void clearCacheDone();

private:
    void slotFetchCollectionFailed();
    void slotFetchCollectionDone(const Akonadi::Collection::List &list);
    void slotFinished(Akonadi::ClearCacheFoldersJob *job);
    void slotClearAkonadiCacheUpdate(Akonadi::ClearCacheFoldersJob *job, const QString &description);
    void slotClearAkonadiCacheCanceled(KPIM::ProgressItem *item);
    Akonadi::Collection mTopLevelCollection;
    QWidget *const mParentWidget;
};
