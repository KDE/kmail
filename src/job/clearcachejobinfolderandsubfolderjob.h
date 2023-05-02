/*
  SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <Akonadi/Collection>
#include <QObject>
class KJob;
namespace KPIM
{
class ProgressItem;
}
class ClearCacheJobInFolderAndSubFolderJob : public QObject
{
    Q_OBJECT
public:
    explicit ClearCacheJobInFolderAndSubFolderJob(QObject *parent = nullptr, QWidget *parentWidget = nullptr);
    ~ClearCacheJobInFolderAndSubFolderJob() override;

    void start();

    void setTopLevelCollection(const Akonadi::Collection &topLevelCollection);

private:
    void slotFetchCollectionFailed();
    void slotFetchCollectionDone(const Akonadi::Collection::List &list);
    void slotFinished(KJob *job);
    void slotRemoveDuplicatesUpdate(KJob *job, const QString &description);
    void slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item);
    Akonadi::Collection mTopLevelCollection;
    QWidget *const mParentWidget;
};
