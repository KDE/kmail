/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "snoozeinfo.h"
#include <Akonadi/Item>
#include <QObject>

class KJob;

/**
 * Snoozes one message: set its SnoozeAttribute, which is what hides it in the
 * message list, then link it into the virtual Snooze collection when there is one.
 */
class SnoozeItemJob : public QObject
{
    Q_OBJECT
public:
    explicit SnoozeItemJob(const Snooze::SnoozeInfo &info, QObject *parent = nullptr);
    ~SnoozeItemJob() override;

    void start();

    [[nodiscard]] bool canStart() const;

Q_SIGNALS:
    void snoozeDone();
    void snoozeFailed(const QString &error);

private:
    void slotItemFetchDone(KJob *job);
    void slotItemModifyDone(KJob *job);
    void slotLinkDone(KJob *job);
    void finished();
    void failed(const QString &error);

    const Snooze::SnoozeInfo mInfo;
    Akonadi::Item mItem;
};
