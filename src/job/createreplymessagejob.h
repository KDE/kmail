/*
   SPDX-FileCopyrightText: 2017-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CREATEREPLYMESSAGEJOB_H
#define CREATEREPLYMESSAGEJOB_H

#include <QObject>
#include "kmail_private_export.h"
#include <AkonadiCore/Item>
#include <KMime/Message>
#include <MessageComposer/MessageFactoryNG>
#include <QUrl>

struct KMAILTESTS_TESTS_EXPORT CreateReplyMessageJobSettings
{
    CreateReplyMessageJobSettings()
    = default;

    QUrl url;
    QString selection;
    QString templateStr;
    Akonadi::Item item;
    KMime::Message::Ptr msg;
    MessageComposer::ReplyStrategy replyStrategy = MessageComposer::ReplySmart;
    bool noQuote = false;
    bool replyAsHtml = false;
};

class KMAILTESTS_TESTS_EXPORT CreateReplyMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateReplyMessageJob(QObject *parent = nullptr);
    ~CreateReplyMessageJob();

    void start();

    void setSettings(const CreateReplyMessageJobSettings &settings);

private:
    Q_DISABLE_COPY(CreateReplyMessageJob)
    void slotCreateReplyDone(const MessageComposer::MessageFactoryNG::MessageReply &reply);
    MessageComposer::MessageFactoryNG *mMessageFactory = nullptr;
    CreateReplyMessageJobSettings mSettings;
};

#endif // CREATEREPLYMESSAGEJOB_H
