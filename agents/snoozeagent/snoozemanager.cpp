/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozemanager.h"
#include "job/snoozeitemjob.h"
#include "job/snoozewakeupjob.h"
#include "snoozeagent_debug.h"
#include "snoozeinfo.h"
#include "snoozeutil.h"

#include <KConfigGroup>

#include <QRegularExpression>
#include <QTimer>

#include <algorithm>

using namespace Snooze;
using namespace Qt::Literals::StringLiterals;
SnoozeManager::SnoozeManager(QObject *parent)
    : QObject(parent)
    , mConfig(SnoozeUtil::defaultConfig())
    , mTimer(new QTimer(this))
{
    mTimer->setSingleShot(true);
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
    std::sort(mListSnoozeInfo.begin(), mListSnoozeInfo.end(), SnoozeUtil::compareSnoozeInfo);
    rearmTimer();
}

void SnoozeManager::rearmTimer()
{
    stopTimer();
    const auto it = std::find_if(mListSnoozeInfo.cbegin(), mListSnoozeInfo.cend(), [this](SnoozeInfo *info) {
        return !mWakingUp.contains(info->itemId());
    });
    if (it == mListSnoozeInfo.cend()) {
        return;
    }
    const std::chrono::milliseconds remaining{QDateTime::currentDateTimeUtc().msecsTo((*it)->wakeUpDateTime())};
    mTimer->start(std::clamp<std::chrono::milliseconds>(remaining, std::chrono::milliseconds::zero(), kMaxSleep));
}

void SnoozeManager::processDue()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QList<SnoozeInfo *> infos = mListSnoozeInfo;
    for (SnoozeInfo *info : infos) {
        if (info->wakeUpDateTime() > now) {
            break;
        }
        startWakeUpJob(*info);
    }
    rearmTimer();
}

void SnoozeManager::startWakeUpJob(const Snooze::SnoozeInfo &info, bool silent)
{
    const Akonadi::Item::Id id = info.itemId();
    if (mWakingUp.contains(id)) {
        return;
    }
    mWakingUp.insert(id);
    auto job = new SnoozeWakeUpJob(info, this);
    job->setSilent(silent);
    connect(job, &SnoozeWakeUpJob::wakeUpDone, this, [this, id]() {
        slotWakeUpJobFinished(id);
    });
    connect(job, &SnoozeWakeUpJob::wakeUpFailed, this, [this, id](const QString &errorStr) {
        // The message list shows it again anyway once the deadline has passed:
        // drop the entry rather than retrying forever.
        qCWarning(SNOOZEAGENT_LOG) << "Wake up failed for item" << id << errorStr;
        slotWakeUpJobFinished(id);
    });
    job->start();
}

void SnoozeManager::slotWakeUpJobFinished(Akonadi::Item::Id id)
{
    mWakingUp.remove(id);
    removeInfo(id);
    rearmTimer();
}

void SnoozeManager::addSnooze(Snooze::SnoozeInfo *info)
{
    if (!info || !info->isValid()) {
        delete info;
        return;
    }
    // Snoozing an already snoozed message again only moves its deadline.
    if (SnoozeInfo *existing = searchInfo(info->itemId())) {
        info->setUniqueIdentifier(existing->uniqueIdentifier());
        mListSnoozeInfo.removeOne(existing);
        delete existing;
    }
    // We are the agent: update the list in place instead of reloading ourselves over D-Bus.
    SnoozeUtil::writeSnoozeInfo(mConfig, info, false);
    mListSnoozeInfo.append(info);
    createSnoozeList();

    const Akonadi::Item::Id id = info->itemId();
    auto job = new SnoozeItemJob(*info, this);
    connect(job, &SnoozeItemJob::snoozeFailed, this, [this, id](const QString &errorStr) {
        // Without its attribute the message is not hidden: nothing to wake up.
        qCWarning(SNOOZEAGENT_LOG) << "Snooze failed for item" << id << errorStr;
        removeInfo(id);
        rearmTimer();
    });
    job->start();
}

bool SnoozeManager::itemRemoved(Akonadi::Item::Id id)
{
    if (!searchInfo(id)) {
        return false;
    }
    removeInfo(id);
    rearmTimer();
    return true;
}

void SnoozeManager::wakeUpNow(Akonadi::Item::Id id)
{
    const SnoozeInfo *info = searchInfo(id);
    if (!info) {
        qCWarning(SNOOZEAGENT_LOG) << "No snoozed message with item id" << id;
        return;
    }
    startWakeUpJob(*info);
    rearmTimer();
}

void SnoozeManager::cancelSnooze(Akonadi::Item::Id id)
{
    SnoozeInfo info;
    if (const SnoozeInfo *knownInfo = searchInfo(id)) {
        info = *knownInfo;
    } else {
        // Not indexed (any more): still clean the item up, a stale attribute would keep it hidden.
        info.setItemId(id);
        info.setWakeUpDateTime(QDateTime::currentDateTimeUtc());
    }
    startWakeUpJob(info, true);
    rearmTimer();
}

bool SnoozeManager::isSnoozed(Akonadi::Item::Id id) const
{
    return searchInfo(id) != nullptr;
}

void SnoozeManager::stopTimer()
{
    if (mTimer->isActive()) {
        mTimer->stop();
    }
}

void SnoozeManager::removeInfo(Akonadi::Item::Id id)
{
    SnoozeInfo *info = searchInfo(id);
    if (!info) {
        return;
    }
    mListSnoozeInfo.removeOne(info);
    (void)SnoozeUtil::removeSnoozeInfo(mConfig, {info->uniqueIdentifier()}, false);
    delete info;
    Q_EMIT needUpdateConfigDialogBox();
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
    infoStr += u"****************************************\n";
    return infoStr;
}

#include "moc_snoozemanager.cpp"
