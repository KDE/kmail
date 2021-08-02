/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QQueue>

#include <Item>

#include <KSharedConfig>

namespace MessageComposer
{
class AkonadiSender;
class SendLaterInfo;
}

class QTimer;
class SendLaterJob;
class SendLaterManager : public QObject
{
    Q_OBJECT
public:
    enum ErrorType {
        ItemNotFound = 0,
        TooManyItemFound = 1,
        CanNotFetchItem = 2,
        MailDispatchDoesntWork = 3,
        CanNotCreateTransport = 4,
        UnknownError = 5,
    };

    explicit SendLaterManager(QObject *parent);
    ~SendLaterManager() override;

    void sendDone(MessageComposer::SendLaterInfo *info);
    void sendError(MessageComposer::SendLaterInfo *info, ErrorType type);
    Q_REQUIRED_RESULT QString printDebugInfo() const;

    void stopAll();
    Q_REQUIRED_RESULT bool itemRemoved(Akonadi::Item::Id id);

    MessageComposer::AkonadiSender *sender() const;

    void sendNow(Akonadi::Item::Id id);

Q_SIGNALS:
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void load(bool forcereload = false);

private:
    Q_DISABLE_COPY(SendLaterManager)
    void slotCreateJob();
    void createSendInfoList();
    QString infoToStr(MessageComposer::SendLaterInfo *info) const;
    void removeLaterInfo(MessageComposer::SendLaterInfo *info);
    MessageComposer::SendLaterInfo *searchInfo(Akonadi::Item::Id id);
    void recreateSendList();
    void stopTimer();
    void removeInfo(Akonadi::Item::Id id);
    KSharedConfig::Ptr mConfig;
    QList<MessageComposer::SendLaterInfo *> mListSendLaterInfo;
    MessageComposer::SendLaterInfo *mCurrentInfo = nullptr;
    SendLaterJob *mCurrentJob = nullptr;
    QTimer *const mTimer;
    MessageComposer::AkonadiSender *const mSender;
    QQueue<Akonadi::Item::Id> mSendLaterQueue;
};

