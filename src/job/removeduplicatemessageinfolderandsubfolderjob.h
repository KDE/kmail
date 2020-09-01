/*
  SPDX-FileCopyrightText: 2015-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#ifndef REMOVEDUPLICATEMESSAGEINFOLDERANDSUBFOLDERJOB_H
#define REMOVEDUPLICATEMESSAGEINFOLDERANDSUBFOLDERJOB_H

#include <QObject>
#include <AkonadiCore/Collection>
class KJob;
namespace KPIM {
class ProgressItem;
}
class RemoveDuplicateMessageInFolderAndSubFolderJob : public QObject
{
    Q_OBJECT
public:
    explicit RemoveDuplicateMessageInFolderAndSubFolderJob(QObject *parent = nullptr, QWidget *parentWidget = nullptr);
    ~RemoveDuplicateMessageInFolderAndSubFolderJob();

    void start();

    void setTopLevelCollection(const Akonadi::Collection &topLevelCollection);

private:
    Q_DISABLE_COPY(RemoveDuplicateMessageInFolderAndSubFolderJob)
    void slotFetchCollectionFailed();
    void slotFetchCollectionDone(const Akonadi::Collection::List &list);
    void slotFinished(KJob *job);
    void slotRemoveDuplicatesUpdate(KJob *job, const QString &description);
    void slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item);
    Akonadi::Collection mTopLevelCollection;
    QWidget *const mParentWidget;
};

#endif // REMOVEDUPLICATEMESSAGEINFOLDERANDSUBFOLDERJOB_H
