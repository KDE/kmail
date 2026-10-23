/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozemanager.h"
#include "job/snoozewakeupjob.h"
#include "snoozeinfo.h"
#include "snoozeutil.h"

#include <KConfigGroup>

#include <QRegularExpression>
#include <QTimer>

using namespace Snooze;
using namespace Qt::Literals::StringLiterals;
SnoozeManager::SnoozeManager(QObject *parent)
    : QObject(parent)
    , mConfig(SnoozeUtil::defaultConfig())
    , mTimer(new QTimer(this))
{
    connect(mTimer, &QTimer::timeout, this, &SnoozeManager::processDue);
}

SnoozeManager::~SnoozeManager()
{
    stopAll();
}

void SnoozeManager::stopAll()
{
    stopTimer();
    qDeleteAll(mListSnoozeInfo);
    mListSnoozeInfo.clear();
    mCurrentInfo = nullptr;
    mCurrentJob = nullptr;
}

void SnoozeManager::load(bool forceReloadConfig)
{
    if (forceReloadConfig) {
        mConfig->reparseConfiguration();
    }
    const QStringList itemList = mConfig->groupList().filter(QRegularExpression(SnoozeUtil::snoozePattern()));
    const int numberOfItems = itemList.count();
    QList<SnoozeInfo *> snoozeInfo;
    for (int i = 0; i < numberOfItems; ++i) {
        KConfigGroup group = mConfig->group(itemList.at(i));

        if (auto info = new SnoozeInfo(group); info->isValid()) {
            snoozeInfo.append(std::move(info));
        } else {
            delete info;
        }
    }
    qDeleteAll(mListSnoozeInfo);
    mListSnoozeInfo = std::move(snoozeInfo);
    createSnoozeList();
}

void SnoozeManager::createSnoozeList()
{
    // TODO sort with SnoozeUtil::compareSnoozeInfo, then rearmTimer().
}

void SnoozeManager::rearmTimer()
{
    // TODO: compare the head of the list against QDateTime::currentDateTimeUtc()
    // and start mTimer for min(remaining, kMaxSleep).
}

void SnoozeManager::processDue()
{
    // TODO: wake up every info whose deadline has passed, then rearmTimer().
}

void SnoozeManager::slotWakeUpJobFinished()
{
    mCurrentJob = nullptr;
    mCurrentInfo = nullptr;
    // TODO drop the info from the index and re-arm.
}

void SnoozeManager::addSnooze(Snooze::SnoozeInfo *info)
{
    if (info && info->isValid()) {
        SnoozeUtil::writeSnoozeInfo(SnoozeUtil::defaultConfig(), info, true);
    } else {
        delete info;
    }
}

bool SnoozeManager::itemRemoved(Akonadi::Item::Id id)
{
    auto info = searchInfo(id);
    if (!info) {
        // TODO error
        return false;
    }
    // TODO forget the info and re-arm the timer.
    return false;
}

void SnoozeManager::wakeUpNow(Akonadi::Item::Id id)
{
    auto info = searchInfo(id);
    if (!info) {
        // TODO error
        return;
    }
    auto job = new SnoozeWakeUpJob(*info, this);
    connect(job, &SnoozeWakeUpJob::wakeUpDone, this, [this, id]() {
        removeInfo(id);
    });
    connect(job, &SnoozeWakeUpJob::wakeUpFailed, this, [](const QString &errorStr) {
        // TODO
        Q_UNUSED(errorStr)
    });

    job->start();
}

void SnoozeManager::stopTimer()
{
    if (mTimer->isActive()) {
        mTimer->stop();
    }
}

void SnoozeManager::removeInfo(Akonadi::Item::Id id)
{
    // TODO remove from mListSnoozeInfo and from the config.
    Q_UNUSED(id)
}

SnoozeInfo *SnoozeManager::searchInfo(Akonadi::Item::Id id) const
{
    for (SnoozeInfo *info : std::as_const(mListSnoozeInfo)) {
        if (info->itemId() == id) {
            return info;
        }
    }
    return nullptr;
}

QString SnoozeManager::printDebugInfo() const
{
    QString infoStr;
    if (mListSnoozeInfo.isEmpty()) {
        infoStr = u"No mail"_s;
    } else {
        for (SnoozeInfo *info : std::as_const(mListSnoozeInfo)) {
            if (!infoStr.isEmpty()) {
                infoStr += u'\n';
            }
            infoStr += infoToStr(info);
        }
    }
    return infoStr;
}

QString SnoozeManager::infoToStr(Snooze::SnoozeInfo *info) const
{
    QString infoStr = u"****************************************"_s;
    infoStr += u"From :%1\n"_s.arg(info->from());
    infoStr += u"itemId :%1\n"_s.arg(info->itemId());
    infoStr += u"Subject :%1\n"_s.arg(info->subject());
    infoStr += u"WakeUpDateTime :%1\n"_s.arg(info->wakeUpDateTime().toString());
    infoStr += u"****************************************\n"_s;
    return infoStr;
}

#include "moc_snoozemanager.cpp"
