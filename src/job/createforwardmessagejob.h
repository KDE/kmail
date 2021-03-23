/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kmail_private_export.h"
#include <AkonadiCore/Item>
#include <KMime/Message>
#include <MessageComposer/MessageFactoryNG>
#include <QObject>
#include <QUrl>

struct KMAILTESTS_TESTS_EXPORT CreateForwardMessageJobSettings {
    QUrl url;
    Akonadi::Item item;
    KMime::Message::Ptr msg = nullptr;
    QString templateStr;
    QString selection;
    uint identity = 0;
};

class KMAILTESTS_TESTS_EXPORT CreateForwardMessageJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateForwardMessageJob(QObject *parent = nullptr);
    ~CreateForwardMessageJob() override;
    void start();

    void setSettings(const CreateForwardMessageJobSettings &value);

private:
    Q_DISABLE_COPY(CreateForwardMessageJob)
    MessageComposer::MessageFactoryNG *mMessageFactory = nullptr;
    void slotCreateForwardDone(const KMime::Message::Ptr &msg);
    CreateForwardMessageJobSettings mSettings;
};

