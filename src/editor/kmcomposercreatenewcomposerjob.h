/*
   Copyright (C) 2017-2018 Laurent Montel <montel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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
    ~KMComposerCreateNewComposerJob();
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
