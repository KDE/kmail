/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KMCOMPOSERCREATENEWCOMPOSERJOB_H
#define KMCOMPOSERCREATENEWCOMPOSERJOB_H

#include <QObject>
#include <KMime/Message>
#include <AkonadiCore/Collection>

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

#endif // KMCOMPOSERCREATENEWCOMPOSERJOB_H
