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

struct KMAILTESTS_TESTS_EXPORT CreateForwardMessageJobSettings {
    QUrl url;
    Akonadi::Item item;
    std::shared_ptr<KMime::Message> msg = nullptr;
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
    KMAIL_NO_EXPORT void slotCreateForwardDone(const std::shared_ptr<KMime::Message> &msg);
    MessageComposer::MessageFactoryNG *mMessageFactory = nullptr;
    CreateForwardMessageJobSettings mSettings;
};
