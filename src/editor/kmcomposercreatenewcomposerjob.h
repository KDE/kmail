/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/Collection>
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
    Q_DISABLE_COPY(KMComposerCreateNewComposerJob)
    void slotCreateNewComposer(bool forceCursorPosition);
    Akonadi::Collection mCollectionForNewMessage;
    KMime::Message::Ptr mMsg = nullptr;
    uint mCurrentIdentity = 0;
};

