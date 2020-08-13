/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef REMOVECOLLECTIONJOB_H
#define REMOVECOLLECTIONJOB_H

#include <QObject>
#include <AkonadiCore/Collection>
class KMMainWidget;
class KJob;
class RemoveCollectionJob : public QObject
{
    Q_OBJECT
public:
    explicit RemoveCollectionJob(QObject *parent = nullptr);
    ~RemoveCollectionJob();

    void setMainWidget(KMMainWidget *mainWidget);

    void start();

    void setCurrentFolder(const Akonadi::Collection &currentFolder);

Q_SIGNALS:
    void clearCurrentFolder();

private:
    Q_DISABLE_COPY(RemoveCollectionJob)
    void slotDelayedRemoveFolder(KJob *job);
    void slotDeletionCollectionResult(KJob *job);

    KMMainWidget *mMainWidget = nullptr;
    Akonadi::Collection mCurrentCollection;
};

#endif // REMOVECOLLECTIONJOB_H
