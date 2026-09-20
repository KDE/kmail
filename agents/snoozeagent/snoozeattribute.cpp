/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snoozeattribute.h"
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

SnoozeAttribute::SnoozeAttribute() = default;

SnoozeAttribute::~SnoozeAttribute() = default;

QByteArray SnoozeAttribute::type() const
{
    static const QByteArray sType("SnoozeAttribute");
    return sType;
}

SnoozeAttribute *SnoozeAttribute::clone() const
{
    auto expireAttr = new SnoozeAttribute();
    expireAttr->setWakeUpDateTime(mWakeUpDateTime);
    expireAttr->setOriginalCollection(mOriginalCollection);
    expireAttr->setMarkAsUnread(mMarkAsUnread);
    return expireAttr;
}

QByteArray SnoozeAttribute::serialized() const
{
    QByteArray result;
    QDataStream s(&result, QIODevice::WriteOnly);
    s << mWakeUpDateTime;
    s << mOriginalCollection;
    s << mMarkAsUnread;
    return result;
}

void SnoozeAttribute::deserialize(const QByteArray &data)
{
    QDataStream s(data);
    s >> mWakeUpDateTime;
    s >> mOriginalCollection;
    s >> mMarkAsUnread;
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
