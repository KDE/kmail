/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeinfo.h"

#include <KConfigGroup>
#include <QDebug>

using namespace Snooze;

SnoozeInfo::SnoozeInfo() = default;

SnoozeInfo::SnoozeInfo(const KConfigGroup &config)
{
    readConfig(config);
}

void SnoozeInfo::readConfig(const KConfigGroup &config)
{
    mSubject = config.readEntry("Subject");
    mUniqueIdentifier = config.readEntry("identifier", -1);
    // TODO add more
}

void SnoozeInfo::writeConfig(KConfigGroup &config, qint32 identifier)
{
    // TODO persist the info under "SnoozeItem <identifier>".
    Q_UNUSED(config)
    setUniqueIdentifier(identifier);
}

bool SnoozeInfo::isValid() const
{
    return mItemId != -1 && mWakeUpDateTime.isValid();
}

Akonadi::Item::Id SnoozeInfo::itemId() const
{
    return mItemId;
}

void SnoozeInfo::setItemId(Akonadi::Item::Id id)
{
    mItemId = id;
}

QDateTime SnoozeInfo::wakeUpDateTime() const
{
    return mWakeUpDateTime;
}

void SnoozeInfo::setWakeUpDateTime(const QDateTime &dateTime)
{
    mWakeUpDateTime = dateTime;
}

QString SnoozeInfo::subject() const
{
    return mSubject;
}

void SnoozeInfo::setSubject(const QString &subject)
{
    mSubject = subject;
}

QString SnoozeInfo::from() const
{
    return mFrom;
}

void SnoozeInfo::setFrom(const QString &from)
{
    mFrom = from;
}

qint32 SnoozeInfo::uniqueIdentifier() const
{
    return mUniqueIdentifier;
}

void SnoozeInfo::setUniqueIdentifier(qint32 uniqueIdentifier)
{
    mUniqueIdentifier = uniqueIdentifier;
}

bool SnoozeInfo::operator==(const SnoozeInfo &other) const
{
    return mItemId == other.mItemId && mWakeUpDateTime == other.mWakeUpDateTime && mSubject == other.mSubject && mFrom == other.mFrom
        && mUniqueIdentifier == other.mUniqueIdentifier;
}

QDebug operator<<(QDebug debug, const Snooze::SnoozeInfo &info)
{
    debug.space() << "Item id:" << info.itemId();
    debug.space() << "Wake up:" << info.wakeUpDateTime();
    debug.space() << "Subject:" << info.subject();
    debug.space() << "From:" << info.from();
    debug.space() << "Unique identifier:" << info.uniqueIdentifier();
    return debug;
}
