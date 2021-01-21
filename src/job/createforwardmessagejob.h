/*
   SPDX-FileCopyrightText: 2017-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CREATEFORWARDMESSAGEJOB_H
#define CREATEFORWARDMESSAGEJOB_H

#include "kmail_private_export.h"
#include <QObject>
#include <QUrl>
#include <KMime/Message>
#include <AkonadiCore/Item>
#include <MessageComposer/MessageFactoryNG>

struct KMAILTESTS_TESTS_EXPORT CreateForwardMessageJobSettings
{
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

#endif // CREATEFORWARDMESSAGEJOB_H
