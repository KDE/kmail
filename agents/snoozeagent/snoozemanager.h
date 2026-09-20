/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <KSharedConfig>
#include <QList>
#include <QObject>
#include <chrono>

class QTimer;
class SnoozeWakeUpJob;

namespace Snooze
{
class SnoozeInfo;
}

/**
 * Keeps the snooze index sorted by wake-up date and fires the wake-up job.
 *
 * The timer is re-armed against the wall clock with a hard cap
 * (see kMaxSleep) instead of being set to the full remaining delay: QTimer
 * stores its interval as an int in milliseconds, so anything past ~24.8 days
 * overflows, and CLOCK_MONOTONIC does not advance across a system suspend --
 * which every overnight snooze crosses.
 */
class SnoozeManager : public QObject
{
    Q_OBJECT
public:
    explicit SnoozeManager(QObject *parent = nullptr);
    ~SnoozeManager() override;

    static constexpr std::chrono::minutes kMaxSleep{10};

    void load(bool forceReloadConfig = false);
    void stopAll();

    /** Takes ownership. */
    void addSnooze(Snooze::SnoozeInfo *info);

    /** Returns true when @p id was known and got dropped. */
    [[nodiscard]] bool itemRemoved(Akonadi::Item::Id id);

    /** Wake @p id up now, regardless of its deadline. */
    void wakeUpNow(Akonadi::Item::Id id);

    [[nodiscard]] QString printDebugInfo() const;

Q_SIGNALS:
    void needUpdateConfigDialogBox();

private:
    void createSnoozeList();
    void rearmTimer();
    void processDue();
    void slotWakeUpJobFinished();
    void stopTimer();
    void removeInfo(Akonadi::Item::Id id);
    [[nodiscard]] Snooze::SnoozeInfo *searchInfo(Akonadi::Item::Id id) const;
    [[nodiscard]] QString infoToStr(Snooze::SnoozeInfo *info) const;

    KSharedConfig::Ptr mConfig;
    QList<Snooze::SnoozeInfo *> mListSnoozeInfo;
    Snooze::SnoozeInfo *mCurrentInfo = nullptr;
    SnoozeWakeUpJob *mCurrentJob = nullptr;
    QTimer *const mTimer;
};
