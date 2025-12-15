/*
   SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Collection>
#include <KMime/Message>
#include <QObject>

class KMComposerCreateNewComposerJob : public QObject
{
    Q_OBJECT
public:
    explicit KMComposerCreateNewComposerJob(QObject *parent = nullptr);
    ~KMComposerCreateNewComposerJob() override;
    void start();
    void setCollectionForNewMessage(const Akonadi::Collection &collectionForNewMessage);

    void setCurrentIdentity(uint currentIdentity);

private:
    void slotCreateNewComposer(bool forceCursorPosition);
    Akonadi::Collection mCollectionForNewMessage;
    std::shared_ptr<KMime::Message> mMsg = nullptr;
    uint mCurrentIdentity = 0;
};
