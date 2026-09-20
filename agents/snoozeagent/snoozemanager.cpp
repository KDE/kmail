/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozemanager.h"
#include "job/snoozewakeupjob.h"
#include "snoozeinfo.h"
#include "snoozeutil.h"

#include <QTimer>

using namespace Snooze;

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
    // TODO read every "SnoozeItem <n>" group into mListSnoozeInfo, then createSnoozeList().
    Q_UNUSED(forceReloadConfig)
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
    // TODO forget the info and re-arm the timer.
    Q_UNUSED(id)
    return false;
}

void SnoozeManager::wakeUpNow(Akonadi::Item::Id id)
{
    // TODO start a SnoozeWakeUpJob straight away for this item.
    Q_UNUSED(id)
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
    // TODO join infoToStr() over the list.
    return {};
}

QString SnoozeManager::infoToStr(Snooze::SnoozeInfo *info) const
{
    // Don't translate it. => debug info.
    Q_UNUSED(info)
    return {};
}

#include "moc_snoozemanager.cpp"
