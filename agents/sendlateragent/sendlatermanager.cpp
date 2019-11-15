/*
   Copyright (C) 2013-2019 Montel Laurent <montel@kde.org>

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

#include "sendlatermanager.h"
#include "sendlaterinfo.h"
#include "sendlaterutil.h"
#include "sendlaterjob.h"

#include "MessageComposer/AkonadiSender"
#include <MessageComposer/Util>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KMessageBox>
#include <KLocalizedString>
#include "sendlateragent_debug.h"

#include <QRegularExpression>
#include <QStringList>
#include <QTimer>

SendLaterManager::SendLaterManager(QObject *parent)
    : QObject(parent)
    , mSender(new MessageComposer::AkonadiSender)
{
    mConfig = KSharedConfig::openConfig();
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &SendLaterManager::slotCreateJob);
}

SendLaterManager::~SendLaterManager()
{
    stopAll();
    delete mSender;
}

void SendLaterManager::stopAll()
{
    stopTimer();
    qDeleteAll(mListSendLaterInfo);
    mListSendLaterInfo.clear();
    mCurrentJob = nullptr;
    mCurrentInfo = nullptr;
}

void SendLaterManager::load(bool forcereload)
{
    stopAll();
    if (forcereload) {
        mConfig->reparseConfiguration();
    }

    const QStringList itemList = mConfig->groupList().filter(QRegularExpression(QStringLiteral("SendLaterItem \\d+")));
    const int numberOfItems = itemList.count();
    for (int i = 0; i < numberOfItems; ++i) {
        KConfigGroup group = mConfig->group(itemList.at(i));
        SendLater::SendLaterInfo *info = new SendLater::SendLaterInfo(group);
        if (info->isValid()) {
            mListSendLaterInfo.append(info);
        } else {
            delete info;
        }
    }
    createSendInfoList();
}

void SendLaterManager::createSendInfoList()
{
    mCurrentInfo = nullptr;
    std::sort(mListSendLaterInfo.begin(), mListSendLaterInfo.end(), SendLater::SendLaterUtil::compareSendLaterInfo);

    //Look at QQueue
    if (mSendLaterQueue.isEmpty()) {
        if (!mListSendLaterInfo.isEmpty()) {
            mCurrentInfo = mListSendLaterInfo.first();
            const QDateTime now = QDateTime::currentDateTime();
            const qint64 seconds = now.secsTo(mCurrentInfo->dateTime());
            if (seconds > 0) {
                //qCDebug(SENDLATERAGENT_LOG)<<" seconds"<<seconds;
                mTimer->start(seconds * 1000);
            } else {
                //Create job when seconds <0
                slotCreateJob();
            }
        } else {
            qCDebug(SENDLATERAGENT_LOG) << " list is empty";
            mTimer->stop();
        }
    } else {
        SendLater::SendLaterInfo *info = searchInfo(mSendLaterQueue.dequeue());
        if (info) {
            mCurrentInfo = info;
            slotCreateJob();
        } else { //If removed.
            createSendInfoList();
        }
    }
}

void SendLaterManager::stopTimer()
{
    if (mTimer->isActive()) {
        mTimer->stop();
    }
}

SendLater::SendLaterInfo *SendLaterManager::searchInfo(Akonadi::Item::Id id)
{
    for (SendLater::SendLaterInfo *info : qAsConst(mListSendLaterInfo)) {
        if (info->itemId() == id) {
            return info;
        }
    }
    return nullptr;
}

void SendLaterManager::sendNow(Akonadi::Item::Id id)
{
    if (!mCurrentJob) {
        SendLater::SendLaterInfo *info = searchInfo(id);
        if (info) {
            mCurrentInfo = info;
            slotCreateJob();
        } else {
            qCDebug(SENDLATERAGENT_LOG) << " can't find info about current id: " << id;
            if (!itemRemoved(id)) {
                qCWarning(SENDLATERAGENT_LOG) << "Impossible to remove id" << id;
            }
        }
    } else {
        //Add to QQueue
        mSendLaterQueue.enqueue(id);
    }
}

void SendLaterManager::slotCreateJob()
{
    if (mCurrentJob) {
        qCDebug(SENDLATERAGENT_LOG) << " Problem we have already a job" << mCurrentJob;
        return;
    }
    mCurrentJob = new SendLaterJob(this, mCurrentInfo);
    mCurrentJob->start();
}

bool SendLaterManager::itemRemoved(Akonadi::Item::Id id)
{
    if (mConfig->hasGroup(SendLater::SendLaterUtil::sendLaterPattern().arg(id))) {
        removeInfo(id);
        mConfig->reparseConfiguration();
        Q_EMIT needUpdateConfigDialogBox();
        return true;
    }
    return false;
}

void SendLaterManager::removeInfo(Akonadi::Item::Id id)
{
    KConfigGroup group = mConfig->group(SendLater::SendLaterUtil::sendLaterPattern().arg(id));
    group.deleteGroup();
    group.sync();
}

void SendLaterManager::sendError(SendLater::SendLaterInfo *info, ErrorType type)
{
    mCurrentJob = nullptr;
    if (info) {
        switch (type) {
        case UnknownError:
        case ItemNotFound:
            //Don't try to resend it. Remove it.
            removeLaterInfo(info);
            break;
        case MailDispatchDoesntWork:
            //Force to make online maildispatcher
            //Don't remove it.
            if (!MessageComposer::Util::sendMailDispatcherIsOnline(nullptr)) {
                qCWarning(SENDLATERAGENT_LOG) << " Impossible to make online send dispatcher";
            }
            //Remove item which create error ?
            if (!info->isRecurrence()) {
                removeLaterInfo(info);
            }
            break;
        case TooManyItemFound:
        case CanNotFetchItem:
        case CanNotCreateTransport:
            if (KMessageBox::No == KMessageBox::questionYesNo(nullptr, i18n("An error was found. Do you want to resend it?"), i18n("Error found"))) {
                removeLaterInfo(info);
            }
            break;
        }
    }
    recreateSendList();
}

void SendLaterManager::recreateSendList()
{
    mCurrentJob = nullptr;
    Q_EMIT needUpdateConfigDialogBox();
    QTimer::singleShot(0, this, &SendLaterManager::createSendInfoList);
}

void SendLaterManager::sendDone(SendLater::SendLaterInfo *info)
{
    if (info) {
        if (info->isRecurrence()) {
            SendLater::SendLaterUtil::changeRecurrentDate(info);
        } else {
            removeLaterInfo(info);
        }
    }
    recreateSendList();
}

void SendLaterManager::removeLaterInfo(SendLater::SendLaterInfo *info)
{
    mListSendLaterInfo.removeAll(mCurrentInfo);
    mCurrentInfo = nullptr;
    removeInfo(info->itemId());
}

QString SendLaterManager::printDebugInfo() const
{
    QString infoStr;
    if (mListSendLaterInfo.isEmpty()) {
        infoStr = QStringLiteral("No mail");
    } else {
        for (SendLater::SendLaterInfo *info : qAsConst(mListSendLaterInfo)) {
            if (!infoStr.isEmpty()) {
                infoStr += QLatin1Char('\n');
            }
            infoStr += infoToStr(info);
        }
    }
    return infoStr;
}

QString SendLaterManager::infoToStr(SendLater::SendLaterInfo *info) const
{
    QString infoStr = QLatin1String("Recusive ") + (info->isRecurrence() ? QStringLiteral("true") : QStringLiteral("false"));
    infoStr += QLatin1String("Item id :") + QString::number(info->itemId());
    infoStr += QLatin1String("Send date:") + info->dateTime().toString();
    infoStr += QLatin1String("Last saved date: ") + info->lastDateTimeSend().toString();
    infoStr += QLatin1String("Mail subject :") + info->subject();
    infoStr += QLatin1String("To: ") + info->to();
    return infoStr;
}

MessageComposer::AkonadiSender *SendLaterManager::sender() const
{
    return mSender;
}
