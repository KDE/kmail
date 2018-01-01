/*
   Copyright (C) 2013-2018 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef SENDLATERMANAGER_H
#define SENDLATERMANAGER_H

#include <QObject>
#include <QQueue>

#include <Item>

#include <KSharedConfig>

namespace SendLater {
class SendLaterInfo;
}

namespace MessageComposer {
class AkonadiSender;
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
        UnknownError = 5
    };

    explicit SendLaterManager(QObject *parent);
    ~SendLaterManager();

    void sendDone(SendLater::SendLaterInfo *info);
    void sendError(SendLater::SendLaterInfo *info, ErrorType type);
    QString printDebugInfo();

    void stopAll();
    void itemRemoved(Akonadi::Item::Id id);

    MessageComposer::AkonadiSender *sender() const;

    void sendNow(Akonadi::Item::Id id);

Q_SIGNALS:
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void load(bool forcereload = false);

private:
    void slotCreateJob();
    void createSendInfoList();
    QString infoToStr(SendLater::SendLaterInfo *info);
    void removeLaterInfo(SendLater::SendLaterInfo *info);
    SendLater::SendLaterInfo *searchInfo(Akonadi::Item::Id id);
    void recreateSendList();
    void stopTimer();
    void removeInfo(Akonadi::Item::Id id);
    KSharedConfig::Ptr mConfig;
    QList<SendLater::SendLaterInfo *> mListSendLaterInfo;
    SendLater::SendLaterInfo *mCurrentInfo = nullptr;
    SendLaterJob *mCurrentJob = nullptr;
    QTimer *mTimer = nullptr;
    MessageComposer::AkonadiSender *mSender = nullptr;
    QQueue<Akonadi::Item::Id> mSendLaterQueue;
};

#endif // SENDLATERMANAGER_H
