/*
   SPDX-FileCopyrightText: 2017-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <Akonadi/Item>
#include <KMime/Message>
#include <MessageComposer/MessageFactoryNG>
#include <QObject>
#include <QUrl>

struct KMAILTESTS_TESTS_EXPORT CreateReplyMessageJobSettings {
    CreateReplyMessageJobSettings() = default;

    QUrl url;
    QString selection;
    QString templateStr;
    Akonadi::Item item;
    std::shared_ptr<KMime::Message> msg;
    MessageComposer::ReplyStrategy replyStrategy = MessageComposer::ReplySmart;
    bool noQuote = false;
    bool replyAsHtml = false;
};

class KMAILTESTS_TESTS_EXPORT CreateReplyMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateReplyMessageJob(QObject *parent = nullptr);
    ~CreateReplyMessageJob() override;

    void start();

    void setSettings(const CreateReplyMessageJobSettings &settings);

private:
    KMAIL_NO_EXPORT void slotCreateReplyDone(const MessageComposer::MessageFactoryNG::MessageReply &reply);
    MessageComposer::MessageFactoryNG *mMessageFactory = nullptr;
    CreateReplyMessageJobSettings mSettings;
};
