/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SENDLATERJOB_H
#define SENDLATERJOB_H

#include <QObject>

#include "sendlatermanager.h"

#include <KMime/Message>
#include <ItemFetchScope>
#include <Item>

namespace MessageComposer {
class SendLaterInfo;
}
class KJob;
class SendLaterJob : public QObject
{
    Q_OBJECT
public:
    SendLaterJob(SendLaterManager *manager, MessageComposer::SendLaterInfo *info, QObject *parent = nullptr);
    ~SendLaterJob() override;

    void start();

private:
    void sendDone();
    void sendError(const QString &error, SendLaterManager::ErrorType type);
    void slotMessageTransfered(const Akonadi::Item::List &);
    void slotJobFinished(KJob *);
    void slotDeleteItem(KJob *);
    void updateAndCleanMessageBeforeSending(const KMime::Message::Ptr &msg);
    Akonadi::ItemFetchScope mFetchScope;
    SendLaterManager *const mManager;
    MessageComposer::SendLaterInfo *const mInfo;
    Akonadi::Item mItem;
};

#endif // SENDLATERJOB_H
