/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <QDateTime>
#include <QString>

class KConfigGroup;
class QDebug;

namespace Snooze
{
/** Scheduling index entry for one snoozed message. */
class SnoozeInfo
{
public:
    SnoozeInfo();
    explicit SnoozeInfo(const KConfigGroup &config);

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] Akonadi::Item::Id itemId() const;
    void setItemId(Akonadi::Item::Id id);

    /** Always UTC. */
    [[nodiscard]] QDateTime wakeUpDateTime() const;
    void setWakeUpDateTime(const QDateTime &dateTime);

    [[nodiscard]] QString subject() const;
    void setSubject(const QString &subject);

    [[nodiscard]] QString from() const;
    void setFrom(const QString &from);

    [[nodiscard]] qint32 uniqueIdentifier() const;
    void setUniqueIdentifier(qint32 uniqueIdentifier);

    void writeConfig(KConfigGroup &config, qint32 identifier);

    [[nodiscard]] bool operator==(const SnoozeInfo &other) const;

private:
    void readConfig(const KConfigGroup &config);

    QDateTime mWakeUpDateTime;
    QString mSubject;
    QString mFrom;
    Akonadi::Item::Id mItemId = -1;
    qint32 mUniqueIdentifier = -1;
};
}

QDebug operator<<(QDebug debug, const Snooze::SnoozeInfo &info);
