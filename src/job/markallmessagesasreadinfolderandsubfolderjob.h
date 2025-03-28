/*
  SPDX-FileCopyrightText: 2015-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#pragma once

#include <QObject>

#include <Akonadi/Collection>
#include <Akonadi/MarkAsCommand>

class MarkAllMessagesAsReadInFolderAndSubFolderJob : public QObject
{
    Q_OBJECT
public:
    explicit MarkAllMessagesAsReadInFolderAndSubFolderJob(QObject *parent = nullptr);
    ~MarkAllMessagesAsReadInFolderAndSubFolderJob() override;

    void setTopLevelCollection(const Akonadi::Collection &topLevelCollection);

    void start();

private:
    void slotFetchCollectionFailed();
    void slotFetchCollectionDone(const Akonadi::Collection::List &list);
    void slotMarkAsResult(Akonadi::MarkAsCommand::Result result);
    Akonadi::Collection mTopLevelCollection;
};
