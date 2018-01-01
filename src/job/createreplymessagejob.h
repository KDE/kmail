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

#ifndef CREATEREPLYMESSAGEJOB_H
#define CREATEREPLYMESSAGEJOB_H

#include <QObject>
#include <AkonadiCore/Item>
#include <KMime/Message>
#include <MessageComposer/MessageFactoryNG>
#include <QUrl>

struct CreateReplyMessageJobSettings
{
    CreateReplyMessageJobSettings()
    {
    }

    QUrl mUrl;
    QString mSelection;
    QString mTemplate;
    Akonadi::Item mItem;
    KMime::Message::Ptr mMsg;
    MessageComposer::ReplyStrategy m_replyStrategy = MessageComposer::ReplySmart;
    bool mNoQuote = false;
};

class CreateReplyMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateReplyMessageJob(QObject *parent = nullptr);
    ~CreateReplyMessageJob();

    void start();

    void setSettings(const CreateReplyMessageJobSettings &settings);

private:
    MessageComposer::MessageFactoryNG *mMessageFactory = nullptr;
    void slotCreateReplyDone(const MessageComposer::MessageFactoryNG::MessageReply &reply);
    CreateReplyMessageJobSettings mSettings;
};

#endif // CREATEREPLYMESSAGEJOB_H
