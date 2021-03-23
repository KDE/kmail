/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/Collection>
#include <QObject>
class KMMainWidget;
class KJob;
class RemoveCollectionJob : public QObject
{
    Q_OBJECT
public:
    explicit RemoveCollectionJob(QObject *parent = nullptr);
    ~RemoveCollectionJob() override;

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

