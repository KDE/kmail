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
    auto snoozeAttr = new SnoozeAttribute();
    snoozeAttr->setWakeUpDateTime(mWakeUpDateTime);
    return snoozeAttr;
}

QByteArray SnoozeAttribute::serialized() const
{
    QByteArray result;
    QDataStream s(&result, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s << mWakeUpDateTime;
    return result;
}

void SnoozeAttribute::deserialize(const QByteArray &data)
{
    QDataStream s(data);
    s >> mWakeUpDateTime;
}

QDateTime SnoozeAttribute::wakeUpDateTime() const
{
    return mWakeUpDateTime;
}

void SnoozeAttribute::setWakeUpDateTime(const QDateTime &dateTime)
{
    mWakeUpDateTime = dateTime;
}
