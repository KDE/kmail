/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeattribute.h"

SnoozeAttribute::SnoozeAttribute() = default;

SnoozeAttribute::~SnoozeAttribute() = default;

QByteArray SnoozeAttribute::type() const
{
    static const QByteArray sType("SnoozeAttribute");
    return sType;
}

SnoozeAttribute *SnoozeAttribute::clone() const
{
    // TODO copy all members over.
    return new SnoozeAttribute();
}

QByteArray SnoozeAttribute::serialized() const
{
    // TODO serialize wake-up date (UTC, ISO 8601), original collection and flags.
    return {};
}

void SnoozeAttribute::deserialize(const QByteArray &data)
{
    // TODO parse what serialized() wrote.
    Q_UNUSED(data)
}

QDateTime SnoozeAttribute::wakeUpDateTime() const
{
    return mWakeUpDateTime;
}

void SnoozeAttribute::setWakeUpDateTime(const QDateTime &dateTime)
{
    mWakeUpDateTime = dateTime;
}

Akonadi::Collection::Id SnoozeAttribute::originalCollection() const
{
    return mOriginalCollection;
}

void SnoozeAttribute::setOriginalCollection(Akonadi::Collection::Id id)
{
    mOriginalCollection = id;
}

bool SnoozeAttribute::markAsUnread() const
{
    return mMarkAsUnread;
}

void SnoozeAttribute::setMarkAsUnread(bool markAsUnread)
{
    mMarkAsUnread = markAsUnread;
}
